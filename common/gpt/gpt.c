/*
 * Copyright (c) 2014, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <efi.h>
#include <efilib.h>
#include <uefi_utils.h>
#include <log.h>
#include "gpt.h"

#define PROTECTIVE_MBR 0xEE
#define GPT_SIGNATURE "EFI PART"

struct gpt_header {
	char signature[8];
	UINT32 revision;
	UINT32 size;
	UINT32 header_crc32;
	UINT32 reserved_zero;
	UINT64 my_lba;
	UINT64 alternate_lba;
	UINT64 first_usable_lba;
	UINT64 last_usable_lba;
	EFI_GUID disk_uuid;
	UINT64 entries_lba;
	UINT32 number_of_entries;
	UINT32 size_of_entry;
	UINT32 entries_crc32;
	/* Remainder of sector is reserved and should be 0 */
} __attribute__((packed));

struct legacy_partition {
	UINT8	status;
	UINT8	f_head;
	UINT8	f_sect;
	UINT8	f_cyl;
	UINT8	type;
	UINT8	l_head;
	UINT8	l_sect;
	UINT8	l_cyl;
	UINT32	f_lba;
	UINT32	num_sect;
} __attribute__((packed));

struct pmbr {
	UINT8			bootcode[424];
	EFI_GUID		boot_guid;
	UINT32			disk_id;
	UINT8			magic[2];	// 0x1d, 0x9a
	struct legacy_partition part[4];
	UINT8			sig[2];	// 0x55, 0xaa
} __attribute__((packed));

static EFI_STATUS read_gpt_header(EFI_BLOCK_IO *bio, EFI_DISK_IO *dio, struct gpt_header *gpt)
{
	EFI_STATUS ret;

	ret = uefi_call_wrapper(dio->ReadDisk, 5, dio, bio->Media->MediaId, bio->Media->BlockSize, sizeof(*gpt), (VOID *)gpt);
	if (EFI_ERROR(ret))
		error(L"Failed to read disk for GPT header: %r\n", ret);

	return ret;
}

static BOOLEAN is_gpt_device(struct gpt_header *gpt)
{
	return CompareMem(gpt->signature, GPT_SIGNATURE, sizeof(gpt->signature)) == 0;
}

static EFI_STATUS read_gpt_partitions(EFI_BLOCK_IO *bio, EFI_DISK_IO *dio, struct gpt_header *gpt, struct gpt_partition **partitions)
{
	EFI_STATUS ret;
	UINTN offset;
	UINTN size;

	offset = bio->Media->BlockSize * gpt->entries_lba;
	size = gpt->number_of_entries * gpt->size_of_entry;

	*partitions = AllocatePool(size);
	if (!*partitions) {
		error(L"Failed to allocate %d bytes for partitions\n", size);
		return EFI_OUT_OF_RESOURCES;
	}

	ret = uefi_call_wrapper(dio->ReadDisk, 5, dio, bio->Media->MediaId, offset, size, *partitions);
	if (EFI_ERROR(ret)) {
		error(L"Failed to read GPT partitions: %r\n", ret);
		goto free_partitions;
	}
	return ret;

free_partitions:
	FreePool(*partitions);
	return ret;
}

static EFI_STATUS gpt_find_label_in_disk(CHAR16 *label, EFI_BLOCK_IO *bio, EFI_DISK_IO *dio, struct gpt_partition_interface *gpart)
{
	struct gpt_header gpt;
	struct gpt_partition *partitions;
	EFI_STATUS ret;

	ret = read_gpt_header(bio, dio, &gpt);
	if (EFI_ERROR(ret)) {
		error(L"Failed to read GPT header: %r\n", ret);
		return ret;
	}
	if (!is_gpt_device(&gpt))
		return EFI_NOT_FOUND;

	ret = read_gpt_partitions(bio, dio, &gpt, &partitions);
	if (EFI_ERROR(ret)) {
		error(L"Failed to read GPT partitions: %r\n", ret);
		return ret;
	}

	UINTN i;
	for (i = 0; i < gpt.number_of_entries; i++) {
		if (!CompareGuid(&partitions[i].type, &NullGuid) || StrCmp(label, partitions[i].name))
			continue;

		debug(L"Found label %s in partition %d\n", label, i + 1);
		CopyMem(&gpart->part, &partitions[i], sizeof(*partitions));
		gpart->bio = bio;
		gpart->dio = dio;
		FreePool(partitions);
		return EFI_SUCCESS;
	}
	FreePool(partitions);
	return EFI_NOT_FOUND;
}

EFI_STATUS gpt_get_partition_by_label(CHAR16 *label, struct gpt_partition_interface *gpart)
{
	EFI_STATUS ret;
	EFI_HANDLE *handles;
	UINTN buf_size = 0;
	UINTN nb_io;

	ret = uefi_call_wrapper(BS->LocateHandle, 5, ByProtocol, &BlockIoProtocol, NULL, &buf_size, NULL);
	if (ret != EFI_BUFFER_TOO_SMALL) {
		error(L"Failed to locate Block IO Protocol: %r\n", ret);
		goto out;
	}

	handles = AllocatePool(buf_size);
	if (!handles) {
		ret = EFI_OUT_OF_RESOURCES;
		error(L"Failed to allocate handles: %r\n", ret);
		goto out;
	}

	ret = uefi_call_wrapper(BS->LocateHandle, 5, ByProtocol, &BlockIoProtocol, NULL, &buf_size, (VOID *)handles);
	if (EFI_ERROR(ret)) {
		error(L"Failed to locate Block IO Protocol: %r\n", ret);
		goto free_handles;
	}

	nb_io = buf_size / sizeof(*handles);
	if (!nb_io) {
		ret = EFI_NOT_FOUND;
		error(L"No block io protocol found\n");
		goto free_handles;
	}
	debug(L"Found %d block io protocols\n", nb_io);

	UINTN i;
	for (i = 0; i < nb_io; i++) {
		EFI_BLOCK_IO *bio;
		EFI_DISK_IO *dio;

		ret = uefi_call_wrapper(BS->HandleProtocol, 3, handles[i], &BlockIoProtocol, (VOID *)&bio);
		if (EFI_ERROR(ret)) {
			error(L"Failed to get block io protocol: %r\n", ret);
			goto free_handles;
		}

		if (bio->Media->LogicalPartition != 0)
			continue;

		ret = uefi_call_wrapper(BS->HandleProtocol, 3, handles[i], &DiskIoProtocol, (VOID *)&dio);
		if (EFI_ERROR(ret)) {
			error(L"Failed to get disk io protocol: %r\n", ret);
			goto free_handles;
		}
		ret = gpt_find_label_in_disk(label, bio, dio, gpart);
		if (EFI_ERROR(ret))
			continue;

		FreePool(handles);
		return EFI_SUCCESS;
	}
	ret = EFI_NOT_FOUND;

free_handles:
	FreePool(handles);
out:
	return ret;
}
