/*
 * Copyright (c) 2013, Intel Corporation
 * All rights reserved.
 *
 * Author: Andrew Boie <andrew.p.boie@intel.com>
 * Some Linux bootstrapping code adapted from efilinux by
 * Matt Fleming <matt.fleming@intel.com>
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
 *
 */
#include <efi.h>
#include <efilib.h>
#include <utils.h>
#include <stdlib.h>
#include <asm/bootparam.h>

#ifdef x86_64
#include "bzimage/x86_64.h"
#else
#include "bzimage/i386.h"
#endif

#if USE_SHIM
#include <shim.h>
#endif

#include <boot.h>
#include <efilinux.h>

typedef struct boot_img_hdr boot_img_hdr;

#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512
#define BOOT_EXTRA_ARGS_SIZE 1024
#define SETUP_HDR		0x53726448	/* 0x53726448 == "HdrS" */

typedef struct {
	UINT16 limit;
	UINT64 *base;
} __attribute__((packed)) dt_addr_t;

dt_addr_t gdt = { 0x800, (UINT64 *)0 };
dt_addr_t idt = { 0, 0 };

struct boot_img_hdr
{
    unsigned char magic[BOOT_MAGIC_SIZE];

    unsigned kernel_size;  /* size in bytes */
    unsigned kernel_addr;  /* physical load addr */

    unsigned ramdisk_size; /* size in bytes */
    unsigned ramdisk_addr; /* physical load addr */

    unsigned second_size;  /* size in bytes */
    unsigned second_addr;  /* physical load addr */

    unsigned tags_addr;    /* physical addr for kernel tags */
    unsigned page_size;    /* flash page size we assume */
    unsigned sig_size;     /* Size of signature block */
    unsigned unused;       /* future expansion: should be 0 */

    unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */

    unsigned char cmdline[BOOT_ARGS_SIZE];

    unsigned id[8]; /* timestamp / checksum / sha1 / etc */

    /* Supplemental command line data; kept here to maintain
     * binary compatibility with older versions of mkbootimg */
    unsigned char extra_cmdline[BOOT_EXTRA_ARGS_SIZE];
};

/*
** +-----------------+
** | boot header     | 1 page
** +-----------------+
** | kernel          | n pages
** +-----------------+
** | ramdisk         | m pages
** +-----------------+
** | second stage    | o pages
** +-----------------+
** | signature       | p pages
** +-----------------+
**
** n = (kernel_size + page_size - 1) / page_size
** m = (ramdisk_size + page_size - 1) / page_size
** o = (second_size + page_size - 1) / page_size
** p = (sig_size + page_size - 1) / page_size
**
** 0. all entities are page_size aligned in flash
** 1. kernel and ramdisk are required (size != 0)
** 2. second is optional (second_size == 0 -> no second)
** 3. load each element (kernel, ramdisk, second) at
**    the specified physical address (kernel_addr, etc)
** 4. prepare tags at tag_addr.  kernel_args[] is
**    appended to the kernel commandline in the tags.
** 5. r0 = 0, r1 = MACHINE_TYPE, r2 = tags_addr
** 6. if second_size != 0: jump to second_addr
**    else: jump to kernel_addr
** 7. signature is optional; size should be 0 if not
**    present. signature type specified by bootloader
*/


static UINT32 pages(struct boot_img_hdr *hdr, UINT32 blob_size)
{
        return (blob_size + hdr->page_size - 1) / hdr->page_size;
}


static UINTN bootimage_size(struct boot_img_hdr *aosp_header,
                BOOLEAN include_sig)
{
        UINTN size;

        size = (1 + pages(aosp_header, aosp_header->kernel_size) +
                pages(aosp_header, aosp_header->ramdisk_size) +
                pages(aosp_header, aosp_header->second_size)) *
                        aosp_header->page_size;
        if (include_sig)
                size += (pages(aosp_header, aosp_header->sig_size) *
                        aosp_header->page_size);
        return size;
}

static EFI_STATUS verify_boot_image(UINT8 *bootimage)
{
#if USE_SHIM
        SHIM_LOCK *shim_lock;
        EFI_GUID shim_guid = SHIM_LOCK_GUID;
        EFI_STATUS ret;
        UINTN sig_offset;
        UINTN sigsize;
        struct boot_img_hdr *aosp_header;

        aosp_header = (struct boot_img_hdr *)bootimage;
        ret = LibLocateProtocol(&shim_guid, (VOID **)&shim_lock);
        if (EFI_ERROR(ret)) {
                error(L"Couldn't instantiate shim protocol", ret);
                return ret;
        }

        sig_offset = bootimage_size(aosp_header, FALSE);
        sigsize = aosp_header->sig_size;

        if (!sigsize) {
                Print(L"Secure boot enabled, but Android boot image is unsigned\n");
                return EFI_ACCESS_DENIED;
        }

        ret = shim_lock->VerifyBlob(bootimage, sig_offset,
                        bootimage + sig_offset, sigsize);
        if (EFI_ERROR(ret))
                error(L"Boot image verification failed", ret);
        return ret;
#else
        return 0;
#endif
}


static EFI_STATUS setup_ramdisk(CHAR8 *bootimage)
{
        struct boot_img_hdr *aosp_header;
        struct boot_params *buf;
        UINT32 roffset, rsize;
        EFI_PHYSICAL_ADDRESS ramdisk_addr;
        EFI_STATUS ret;

        aosp_header = (struct boot_img_hdr *)bootimage;
        buf = (struct boot_params *)(bootimage + aosp_header->page_size);

        roffset = (1 + pages(aosp_header, aosp_header->kernel_size))
                        * aosp_header->page_size;
        rsize = aosp_header->ramdisk_size;
        buf->hdr.ramdisk_size = rsize;
        ret = emalloc(rsize, 0x1000, &ramdisk_addr);
        if (EFI_ERROR(ret))
                return ret;

        if ((UINTN)ramdisk_addr > buf->hdr.initrd_addr_max) {
                Print(L"Ramdisk address is too high!\n");
                ret = EFI_OUT_OF_RESOURCES;
                goto out_error;
        }
        memcpy((VOID *)(UINTN)ramdisk_addr, bootimage + roffset, rsize);
        buf->hdr.ramdisk_image = (UINT32)(UINTN)ramdisk_addr;
	debug("Ramdisk copied into address 0x%x", ramdisk_addr);
        return EFI_SUCCESS;
out_error:
        efree(ramdisk_addr, rsize);
        return ret;
}


static CHAR16 *get_serial_number(void)
{
        /* Per Android CDD, the value must be 7-bit ASCII and
         * match the regex ^[a-zA-Z0-9](0,20)$ */
        CHAR8 *tmp, *pos;
        CHAR16 *ret;
        CHAR8 *serialno;
        EFI_GUID guid;
        UINTN len;

        if (EFI_ERROR(LibGetSmbiosSystemGuidAndSerialNumber(&guid,
                        &serialno)))
                return NULL;

        len = strlena(serialno);
        tmp = AllocatePool(len + 1);
        if (!tmp)
                return NULL;
        tmp[len] = '\0';
        memcpy(tmp, serialno, strlena(serialno));

        pos = tmp;
        while (*pos) {
                /* Truncate if greater than 20 chars */
                if ((pos - tmp) >= 20) {
                        *pos = '\0';
                        break;
                }
                /* Replace foreign characters with zeroes */
                if (!((*pos >= '0' && *pos <= '9') ||
                            (*pos >= 'a' && *pos <= 'z') ||
                            (*pos >= 'A' && *pos <= 'Z')))
                        *pos = '0';
                pos++;
        }
        ret = stra_to_str(tmp);
        FreePool(tmp);
        return ret;
}


static EFI_STATUS setup_command_line(UINT8 *bootimage,
                CHAR16 *install_id)
{
        CHAR16 *cmdline16 = NULL;
        CHAR16 *serialno;
        CHAR16 *tmp;

        EFI_PHYSICAL_ADDRESS cmdline_addr;
        CHAR8 *full_cmdline;
        CHAR8 *cmdline;
        UINTN cmdlen;
        EFI_STATUS ret;
        struct boot_img_hdr *aosp_header;
        struct boot_params *buf;

        aosp_header = (struct boot_img_hdr *)bootimage;
        buf = (struct boot_params *)(bootimage + aosp_header->page_size);

        full_cmdline = AllocatePool(BOOT_ARGS_SIZE + BOOT_EXTRA_ARGS_SIZE);
        if (!full_cmdline) {
                ret = EFI_OUT_OF_RESOURCES;
                goto out;
        }
        memcpy(full_cmdline, aosp_header->cmdline, (BOOT_ARGS_SIZE - 1));
        if (aosp_header->cmdline[BOOT_ARGS_SIZE - 2]) {
                memcpy(full_cmdline + (BOOT_ARGS_SIZE - 1),
                                aosp_header->extra_cmdline,
                                BOOT_EXTRA_ARGS_SIZE);
        }
        cmdline16 = stra_to_str(full_cmdline);
        if (!cmdline16) {
                ret = EFI_OUT_OF_RESOURCES;
                goto out;
        }

        /* Append serial number from DMI */
        serialno = get_serial_number();
        if (serialno) {
                tmp = cmdline16;
                cmdline16 = PoolPrint(L"%s androidboot.serialno=%s",
                        cmdline16, serialno);
                FreePool(tmp);
                FreePool(serialno);
                if (!cmdline16) {
                        ret = EFI_OUT_OF_RESOURCES;
                        goto out;
                }
        }

        if (install_id) {
                CHAR16 *pos = install_id;

                /* install_id should be a hex sequence and nothing else */
                while (*pos) {
                        if (!((*pos >= '0' && *pos <= '9') ||
                                    (*pos >= 'a' && *pos <= 'f') ||
                                    (*pos >= 'A' && *pos <= 'F'))) {
                                ret = EFI_INVALID_PARAMETER;
                                goto out;
                        }
                        pos++;
                }
                tmp = cmdline16;
                cmdline16 = PoolPrint(L"%s androidboot.install_id=ANDROID!%s",
                        cmdline16, install_id);
                FreePool(tmp);
                if (!cmdline16) {
                        ret = EFI_OUT_OF_RESOURCES;
                        goto out;
                }
        }

        /* Documentation/x86/boot.txt: "The kernel command line can be located
         * anywhere between the end of the setup heap and 0xA0000" */
        cmdline_addr = 0xA0000;
        cmdlen = StrLen(cmdline16);
        ret = allocate_pages(AllocateMaxAddress, EfiLoaderData,
                             EFI_SIZE_TO_PAGES(cmdlen + 1),
                             &cmdline_addr);
        if (EFI_ERROR(ret))
                goto out;

        cmdline = (CHAR8 *)(UINTN)cmdline_addr;
        ret = str_to_stra(cmdline, cmdline16, cmdlen + 1);
        if (EFI_ERROR(ret)) {
                Print(L"Non-ascii characters in command line\n");
                free_pages(cmdline_addr, EFI_SIZE_TO_PAGES(cmdlen + 1));
                goto out;
        }

        buf->hdr.cmd_line_ptr = (UINT32)(UINTN)cmdline;
	buf->hdr.cmdline_size = cmdlen + 1;
        ret = EFI_SUCCESS;
out:
        FreePool(cmdline16);
        FreePool(full_cmdline);

        return ret;
}

static EFI_STATUS setup_idt_gdt(void)
{
	EFI_STATUS err;

	err = emalloc(gdt.limit, 8, (EFI_PHYSICAL_ADDRESS *)&gdt.base);
	if (err != EFI_SUCCESS)
		return err;

	memset((CHAR8 *)gdt.base, 0x0, gdt.limit);

	/*
	 * 4Gb - (0x100000*0x1000 = 4Gb)
	 * base address=0
	 * code read/exec
	 * granularity=4096, 386 (+5th nibble of limit)
	 */
	gdt.base[2] = 0x00cf9a000000ffff;

	/*
	 * 4Gb - (0x100000*0x1000 = 4Gb)
	 * base address=0
	 * data read/write
	 * granularity=4096, 386 (+5th nibble of limit)
	 */
	gdt.base[3] = 0x00cf92000000ffff;

	/* Task segment value */
	gdt.base[4] = 0x0080890000000000;


	return EFI_SUCCESS;
}

static void setup_e820_map(struct boot_params *boot_params,
			   EFI_MEMORY_DESCRIPTOR *map_buf,
			   UINTN map_size,
			   UINTN desc_size)
{
	struct e820entry *e820_map = &boot_params->e820_map[0];
	/*
	 * Convert the EFI memory map to E820.
	 */
	int i, j = 0;
	for (i = 0; i < map_size / desc_size; i++) {
		EFI_MEMORY_DESCRIPTOR *d;
		unsigned int e820_type = 0;

		d = (EFI_MEMORY_DESCRIPTOR *)((unsigned long)map_buf + (i * desc_size));
		switch(d->Type) {
		case EfiReservedMemoryType:
		case EfiRuntimeServicesCode:
		case EfiRuntimeServicesData:
		case EfiMemoryMappedIO:
		case EfiMemoryMappedIOPortSpace:
		case EfiPalCode:
			e820_type = E820_RESERVED;
			break;

		case EfiUnusableMemory:
			e820_type = E820_UNUSABLE;
			break;

		case EfiACPIReclaimMemory:
			e820_type = E820_ACPI;
			break;

		case EfiLoaderCode:
		case EfiLoaderData:
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory:
			e820_type = E820_RAM;
			break;

		case EfiACPIMemoryNVS:
			e820_type = E820_NVS;
			break;

		default:
			continue;
		}

		if (j && e820_map[j-1].type == e820_type &&
			(e820_map[j-1].addr + e820_map[j-1].size) == d->PhysicalStart) {
			e820_map[j-1].size += d->NumberOfPages << EFI_PAGE_SHIFT;
		} else {
			e820_map[j].addr = d->PhysicalStart;
			e820_map[j].size = d->NumberOfPages << EFI_PAGE_SHIFT;
			e820_map[j].type = e820_type;
			j++;
		}
	}

	boot_params->e820_entries = j;
}

static EFI_STATUS setup_efi_memory_map(struct boot_params *boot_params, UINTN *map_key)

{
	UINTN map_size = 0;
	EFI_STATUS ret = EFI_SUCCESS;
	EFI_MEMORY_DESCRIPTOR *map_buf;
	UINTN descr_size;
	UINT32 descr_version;
	struct efi_info *efi = &boot_params->efi_info;

	ret = memory_map(&map_buf, &map_size, map_key, &descr_size, &descr_version);
	if (ret != EFI_SUCCESS)
		goto out;

	efi->efi_systab = (UINT32)(UINTN)sys_table;
	efi->efi_memdesc_size = descr_size;
	efi->efi_memdesc_version = descr_version;
	efi->efi_memmap = (UINT32)(UINTN)map_buf;
	efi->efi_memmap_size = map_size;
#ifdef x86_64
	efi->efi_systab_hi = (unsigned long)sys_table >> 32;
	efi->efi_memmap_hi = (unsigned long)map_buf >> 32;
#endif

	memcpy((CHAR8 *)&efi->efi_loader_signature,
	       (CHAR8 *)EFI_LOADER_SIGNATURE, sizeof(UINT32));

	setup_e820_map(boot_params, map_buf, map_size, descr_size);

out:
	return ret;
}

static EFI_STATUS handover_kernel(CHAR8 *bootimage, EFI_HANDLE parent_image)
{
        EFI_PHYSICAL_ADDRESS kernel_start;
        EFI_PHYSICAL_ADDRESS boot_addr;
        struct boot_params *boot_params;
        UINT64 init_size;
        EFI_STATUS ret;
        struct boot_img_hdr *aosp_header;
        struct boot_params *buf;
        UINT8 setup_sectors;
        UINT32 setup_size;
        UINT32 ksize;
        UINT32 koffset;

        aosp_header = (struct boot_img_hdr *)bootimage;
        buf = (struct boot_params *)(bootimage + aosp_header->page_size);

        koffset = aosp_header->page_size;
        setup_sectors = buf->hdr.setup_sects;
        setup_sectors++; /* Add boot sector */
        setup_size = (UINT32)setup_sectors * 512;
        ksize = aosp_header->kernel_size - setup_size;
        kernel_start = buf->hdr.pref_address;
        init_size = buf->hdr.init_size;
        buf->hdr.type_of_loader = 0xff;

        memset((CHAR8 *)&buf->screen_info, 0x0, sizeof(buf->screen_info));

        ret = allocate_pages(AllocateAddress, EfiLoaderData,
                             EFI_SIZE_TO_PAGES(init_size), &kernel_start);
        if (EFI_ERROR(ret)) {
                /*
                 * We failed to allocate the preferred address, so
                 * just allocate some memory and hope for the best.
                 */
                ret = emalloc(init_size, buf->hdr.kernel_alignment, &kernel_start);
                if (EFI_ERROR(ret))
                        return ret;
        }
	debug("kernel_start = 0x%x", kernel_start);

        memcpy((CHAR8 *)(UINTN)kernel_start, bootimage + koffset + setup_size, ksize);

        boot_addr = 0x3fffffff;
        ret = allocate_pages(AllocateMaxAddress, EfiLoaderData,
                             EFI_SIZE_TO_PAGES(16384), &boot_addr);
        if (EFI_ERROR(ret))
                goto out;

        boot_params = (struct boot_params *)(UINTN)boot_addr;
        memset((void *)boot_params, 0x0, 16384);

        /* Copy first two sectors to boot_params */
        memcpy((CHAR8 *)boot_params, (CHAR8 *)buf, 2 * 512);
        boot_params->hdr.code32_start = (UINT32)((UINT64)kernel_start);

        ret = EFI_LOAD_ERROR;

	ret = setup_idt_gdt();
	if (EFI_ERROR(ret))
		goto out;

	UINTN map_key;
	ret = setup_efi_memory_map(boot_params, &map_key);
	if (EFI_ERROR(ret))
		goto out;

	ret = exit_boot_services(bootimage, map_key);
	if (EFI_ERROR(ret))
		goto out;

	asm volatile ("lidt %0" :: "m" (idt));
	asm volatile ("lgdt %0" :: "m" (gdt));

	kernel_jump(kernel_start, boot_params);
        /* Shouldn't get here */

        free_pages(boot_addr, EFI_SIZE_TO_PAGES(16384));
out:
	debug("Can't boot kernel");
        efree(kernel_start, ksize);
        return ret;
}


EFI_STATUS android_image_start_partition(
                IN EFI_HANDLE parent_image,
                IN const EFI_GUID *guid,
                IN CHAR16 *install_id)
{
        EFI_BLOCK_IO *BlockIo;
        EFI_DISK_IO *DiskIo;
        UINT32 MediaId;
        UINT32 img_size;
        UINT8 *bootimage;
        EFI_STATUS ret;
        struct boot_img_hdr aosp_header;

        debug("Locating boot image");
        ret = open_partition(guid, &MediaId, &BlockIo, &DiskIo);
        if (EFI_ERROR(ret))
                return ret;

        debug("Reading boot image header");
        ret = uefi_call_wrapper(DiskIo->ReadDisk, 5, DiskIo, MediaId, 0,
                        sizeof(aosp_header), &aosp_header);
        if (EFI_ERROR(ret)) {
                error(L"ReadDisk (header)", ret);
                return ret;
        }
        if (strncmpa((CHAR8 *)BOOT_MAGIC, aosp_header.magic, BOOT_MAGIC_SIZE)) {
                Print(L"This partition does not appear to contain an Android boot image\n");
                return EFI_INVALID_PARAMETER;
        }

        img_size = bootimage_size(&aosp_header, TRUE);
        bootimage = AllocatePool(img_size);
        if (!bootimage)
                return EFI_OUT_OF_RESOURCES;

        debug("Reading full boot image");
        ret = uefi_call_wrapper(DiskIo->ReadDisk, 5, DiskIo, MediaId, 0,
                        img_size, bootimage);
        if (EFI_ERROR(ret)) {
                error(L"ReadDisk", ret);
                goto out;
        }

        ret = android_image_start_buffer(parent_image, bootimage, install_id);
out:
        FreePool(bootimage);
        return ret;
}


EFI_STATUS android_image_start_file(
                IN EFI_HANDLE parent_image,
                IN EFI_HANDLE device,
                IN CHAR16 *loader,
                IN CHAR16 *install_id)
{
        EFI_STATUS ret;
        VOID *bootimage;
        EFI_DEVICE_PATH *path;
        EFI_GUID SimpleFileSystemProtocol = SIMPLE_FILE_SYSTEM_PROTOCOL;
        EFI_GUID EfiFileInfoId = EFI_FILE_INFO_ID;
        EFI_FILE_IO_INTERFACE *drive;
        EFI_FILE_INFO *fileinfo = NULL;
        EFI_FILE *imagefile, *root;
        UINTN buffersize = sizeof(EFI_FILE_INFO);
        struct boot_img_hdr *aosp_header;
        UINTN bsize;

        debug("Locating boot image from file %s", loader);
        path = FileDevicePath(device, loader);
        if (!path) {
                Print(L"Error getting device path.");
                uefi_call_wrapper(BS->Stall, 1, 3 * 1000 * 1000);
                return EFI_INVALID_PARAMETER;
        }

        /* Open the device */
        ret = uefi_call_wrapper(BS->HandleProtocol, 3, device,
                        &SimpleFileSystemProtocol, (void **)&drive);
        if (EFI_ERROR(ret)) {
                error(L"HandleProtocol", ret);
                return ret;
        }
        ret = uefi_call_wrapper(drive->OpenVolume, 2, drive, &root);
        if (EFI_ERROR(ret)) {
                error(L"OpenVolume", ret);
                return ret;
        }

        /* Get information about the boot image file, we need to know
         * how big it is, and allocate a suitable buffer */
        ret = uefi_call_wrapper(root->Open, 5, root, &imagefile, loader,
                        EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR(ret)) {
                error(L"Open", ret);
                return ret;
        }
        fileinfo = AllocatePool(buffersize);
        if (!fileinfo)
                return EFI_OUT_OF_RESOURCES;

        ret = uefi_call_wrapper(imagefile->GetInfo, 4, imagefile,
                        &EfiFileInfoId, &buffersize, fileinfo);
        if (ret == EFI_BUFFER_TOO_SMALL) {
                /* buffersize updated with the required space for
                 * the request */
                FreePool(fileinfo);
                fileinfo = AllocatePool(buffersize);
                if (!fileinfo)
                        return EFI_OUT_OF_RESOURCES;
                ret = uefi_call_wrapper(imagefile->GetInfo, 4, imagefile,
                        &EfiFileInfoId, &buffersize, fileinfo);
        }
        if (EFI_ERROR(ret)) {
                error(L"GetInfo", ret);
                goto free_info;
        }
        buffersize = fileinfo->FileSize;
        bootimage = AllocatePool(buffersize);
        if (!bootimage) {
                ret = EFI_OUT_OF_RESOURCES;
                goto out;
        }

        /* Read the file into the buffer */
        ret = uefi_call_wrapper(imagefile->Read, 3, imagefile,
                        &buffersize, bootimage);
        if (ret == EFI_BUFFER_TOO_SMALL) {
                /* buffersize updated with the required space for
                 * the request. By the way it doesn't make any
                 * sense to me why this is needed since we supposedly
                 * got the file size from the GetInfo call but
                 * whatever... */
                FreePool(bootimage);
                bootimage = AllocatePool(buffersize);
                if (!fileinfo) {
                        ret = EFI_OUT_OF_RESOURCES;
                        goto out;
                }
                ret = uefi_call_wrapper(imagefile->Read, 3, imagefile,
                        &buffersize, bootimage);
        }
        if (EFI_ERROR(ret)) {
                error(L"Read", ret);
                goto out;
        }

        aosp_header = (struct boot_img_hdr *)bootimage;
        bsize = bootimage_size(aosp_header, TRUE);
        if (buffersize != bsize) {
                Print(L"Boot image size mismatch; got %ld expected %ld\n",
                                buffersize, bsize);
                ret = EFI_INVALID_PARAMETER;
                goto out;
        }

        ret = android_image_start_buffer(parent_image, bootimage, install_id);

out:
        FreePool(bootimage);
free_info:
        FreePool(fileinfo);

        return ret;
}

UINT8 is_secure_boot_enabled(void)
{
        UINT8 secure_boot;
        UINTN size;
        EFI_STATUS status;
        EFI_GUID global_var_guid = EFI_GLOBAL_VARIABLE;

        size = sizeof(secure_boot);
        status = uefi_call_wrapper(RT->GetVariable, 5, L"SecureBoot", (EFI_GUID *)&global_var_guid, NULL, &size, (void*)&secure_boot);

        if (EFI_ERROR(status))
                secure_boot = 0;

        return secure_boot;
}

EFI_STATUS android_image_start_buffer(
                IN EFI_HANDLE parent_image,
                IN VOID *bootimage,
                IN CHAR16 *install_id)
{
        struct boot_img_hdr *aosp_header;
        struct boot_params *buf;
        EFI_STATUS ret;

        aosp_header = (struct boot_img_hdr *)bootimage;
        buf = (struct boot_params *)(bootimage + aosp_header->page_size);

        /* Check boot sector signature */
        if (buf->hdr.boot_flag != 0xAA55) {
                Print(L"bzImage kernel corrupt\n");
                ret = EFI_INVALID_PARAMETER;
                goto out_bootimage;
        }

        if (buf->hdr.header != SETUP_HDR) {
                Print(L"Setup code version is invalid\n");
                ret = EFI_INVALID_PARAMETER;
                goto out_bootimage;
        }

        if (buf->hdr.version < 0x20c) {
                /* Protocol 2.12, kernel 3.8 required */
                Print(L"Kernel header version %x too old\n", buf->hdr.version);
                ret = EFI_INVALID_PARAMETER;
                goto out_bootimage;
        }

        if (!buf->hdr.relocatable_kernel) {
                Print(L"Expected relocatable kernel\n");
                ret = EFI_INVALID_PARAMETER;
                goto out_bootimage;
        }

        if (is_secure_boot_enabled()) {
                debug("Verifying the boot image");
                ret = verify_boot_image(bootimage);
                if (EFI_ERROR(ret)) {
                        error(L"boot image digital signature verification failed", ret);
                        goto out_bootimage;
                }
        }

        debug("Creating command line");
        ret = setup_command_line(bootimage, install_id);
        if (EFI_ERROR(ret)) {
                error(L"setup_command_line", ret);
                goto out_bootimage;
        }

        debug("Loading the ramdisk");
        ret = setup_ramdisk(bootimage);
        if (EFI_ERROR(ret)) {
                error(L"setup_ramdisk", ret);
                goto out_cmdline;
        }

        debug("Loading the kernel");
        ret = handover_kernel(bootimage, parent_image);
        error(L"handover_kernel", ret);

        efree(buf->hdr.ramdisk_image, buf->hdr.ramdisk_size);
out_cmdline:
        free_pages(buf->hdr.cmd_line_ptr,
                        strlena((CHAR8 *)(UINTN)buf->hdr.cmd_line_ptr) + 1);
out_bootimage:
        FreePool(bootimage);
        return ret;
}

/* vim: softtabstop=8:shiftwidth=8:expandtab
 */
