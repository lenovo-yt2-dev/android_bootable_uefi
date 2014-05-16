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
#include <gpt.h>
#include "flash.h"


EFI_STATUS flash(VOID *data, UINTN size, CHAR16 *label)
{
	struct gpt_partition_interface gparti;
	EFI_STATUS ret;
	UINT64 offset;

	ret = gpt_get_partition_by_label(label, &gparti);
	if (EFI_ERROR(ret)) {
		error(L"Failed to get partition %s, error %r\n", label, ret);
		return ret;
	}

	offset = gparti.part.starting_lba * gparti.bio->Media->BlockSize;

	debug(L"Write %d bytes at offset 0x%x\n", size, offset);
	ret = uefi_call_wrapper(gparti.dio->WriteDisk, 5, gparti.dio, gparti.bio->Media->MediaId, offset, size, data);
	if (EFI_ERROR(ret))
		error(L"Failed to write bytes: %r\n", ret);

	return ret;
}

EFI_STATUS flash_file(EFI_HANDLE image, CHAR16 *filename, CHAR16 *label)
{
	EFI_STATUS ret;
	EFI_FILE_IO_INTERFACE *io = NULL;
	VOID *buffer = NULL;
	UINTN size = 0;

	ret = uefi_call_wrapper(BS->HandleProtocol, 3, image, &FileSystemProtocol, (void *)&io);
	if (EFI_ERROR(ret)) {
		error(L"Failed to get FileSystemProtocol: %r\n", ret);
		goto out;
	}

	ret = uefi_read_file(io, filename, &buffer, &size);
	if (EFI_ERROR(ret)) {
		error(L"Failed to read file %s: %r\n", filename, ret);
		goto out;
	}

	ret = flash(buffer, size, label);
	if (EFI_ERROR(ret)) {
		error(L"Failed to flash file %s on partition %s: %r\n", filename, label, ret);
		goto free_buffer;
	}

free_buffer:
	FreePool(buffer);
out:
	return ret;

}

/* It is faster to erase multiple block at once
 * 4096 * 512 => 2MB
 */
#define N_BLOCK (4096)

EFI_STATUS erase(CHAR16 *label)
{
	struct gpt_partition_interface gparti;
	EFI_STATUS ret;
	UINT64 i;
	UINT64 lba;
	VOID *emptyblock;
	UINT64 *eblock;
	UINT64 size;

	ret = gpt_get_partition_by_label(label, &gparti);
	if (EFI_ERROR(ret)) {
		error(L"Failed to get partition %s, error %r\n", label, ret);
		return ret;
	}
	emptyblock = AllocatePool(gparti.bio->Media->BlockSize * N_BLOCK);
	if (!emptyblock)
		return EFI_OUT_OF_RESOURCES;

	eblock = emptyblock;

	for (i = 0; i < N_BLOCK * gparti.bio->Media->BlockSize / sizeof(*eblock); i++)
		eblock[i] = 0xFFFFFFFFFFFFFFFFLL;

	/* size in MB for debug */
	size = gparti.bio->Media->BlockSize * (gparti.part.ending_lba - gparti.part.starting_lba + 1) / (1024 * 1024);
	debug(L"Partition info\n\tblock size %d\n\tStart %ld\n\tEnd %ld\n\tSize %ld MB\n", gparti.bio->Media->BlockSize, gparti.part.starting_lba, gparti.part.ending_lba, size);

	for (lba = gparti.part.starting_lba; lba <= gparti.part.ending_lba; lba += N_BLOCK) {
		if (lba + N_BLOCK > gparti.part.ending_lba + 1)
			size = gparti.part.ending_lba - lba + 1;
		else
			size = N_BLOCK;

		ret = uefi_call_wrapper(gparti.bio->WriteBlocks, 5, gparti.bio, gparti.bio->Media->MediaId, lba, gparti.bio->Media->BlockSize * size, emptyblock);
		if (EFI_ERROR(ret)) {
			error(L"Failed to get erase block %ld: %r\n", lba, ret);
			goto free_block;
		}
	}
	debug(L"Partition Successfully Erased\n");

free_block:
	FreePool(emptyblock);

	return ret;
}
