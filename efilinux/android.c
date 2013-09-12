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
#include "stdlib.h"
#include "loaders/bzimage/bzimage.h"

#if USE_SHIM
#include <shim.h>
#endif

#include "android.h"
#include "efilinux.h"


#define FWUPDATE_DIR    L"fwupdate"

/* Bootloader Message
 *
 * This structure describes the content of a block in flash
 * that is used for recovery and the bootloader to talk to
 * each other.
 *
 * The command field is updated by linux when it wants to
 * reboot into recovery or to update radio or bootloader firmware.
 * It is also updated by the bootloader when firmware update
 * is complete (to boot into recovery for any final cleanup)
 *
 * The status field is written by the bootloader after the
 * completion of an "update-radio" or "update-hboot" command.
 *
 * The recovery field is only written by linux and used
 * for the system to send a message to recovery or the
 * other way around.
 */
struct bootloader_message {
        CHAR8 command[32];
        CHAR8 status[32];
        CHAR8 recovery[1024];
};

typedef struct boot_img_hdr boot_img_hdr;

#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512
#define BOOT_EXTRA_ARGS_SIZE 1024

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

typedef void(*handover_func)(void *, EFI_SYSTEM_TABLE *, struct boot_params *) \
            __attribute__((regparm(0)));

static inline void handover_jump(EFI_HANDLE image, struct boot_params *bp,
                                 EFI_PHYSICAL_ADDRESS kernel_start)
{
        UINTN offset = bp->hdr.handover_offset;
        handover_func hf;

        asm volatile ("cli");

#if __LP64__
        /* The 64-bit kernel entry is 512 bytes after the start. */
        kernel_start += 512;
#endif

        hf = (handover_func)((UINTN)kernel_start + offset);
        hf(image, ST, bp);
}


static VOID error(CHAR16 *str, EFI_STATUS ret)
{
        Print(L"ERROR %s: %r\n", str, ret);
        uefi_call_wrapper(BS->Stall, 1, 2 * 1000 * 1000);
}


static EFI_STATUS str_to_stra(CHAR8 *dst, CHAR16 *src, UINTN len)
{
        UINTN i;

        /* This is NOT how to do UTF16 to UTF8 conversion. For now we're just
         * going to hope that nobody's putting non-ASCII characters in
         * the source string! We'll at least abort with an error
         * if we see any funny stuff */
        for (i = 0; i < len; i++) {
                if (src[i] > 0x7F)
                        return EFI_INVALID_PARAMETER;

                dst[i] = (CHAR8)src[i];
                if (!src[i])
                        break;
        }
        return EFI_SUCCESS;
}

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


static EFI_STATUS setup_ramdisk(UINT8 *bootimage)
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
        buf->hdr.ramdisk_len = rsize;
        ret = emalloc(rsize, 0x1000, &ramdisk_addr);
        if (EFI_ERROR(ret))
                return ret;

        if ((UINTN)ramdisk_addr > buf->hdr.ramdisk_max) {
                Print(L"Ramdisk address is too high!\n");
                ret = EFI_OUT_OF_RESOURCES;
                goto out_error;
        }
        memcpy((VOID *)(UINTN)ramdisk_addr, bootimage + roffset, rsize);
        buf->hdr.ramdisk_start = (UINT32)(UINTN)ramdisk_addr;
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
        ret = EFI_SUCCESS;
out:
        FreePool(cmdline16);
        FreePool(full_cmdline);

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
        setup_sectors = buf->hdr.setup_secs;
        setup_sectors++; /* Add boot sector */
        setup_size = (UINT32)setup_sectors * 512;
        ksize = aosp_header->kernel_size - setup_size;
        kernel_start = buf->hdr.pref_address;
        init_size = buf->hdr.init_size;
        buf->hdr.loader_id = 0x1;
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
        handover_jump(parent_image, boot_params, kernel_start);
        /* Shouldn't get here */

        free_pages(boot_addr, EFI_SIZE_TO_PAGES(16384));
out:
        efree(kernel_start, ksize);
        return ret;
}


static UINT32 swap_bytes32(UINT32 n)
{
        return ((n & 0x000000FF) << 24) |
               ((n & 0x0000FF00) << 8 ) |
               ((n & 0x00FF0000) >> 8 ) |
               ((n & 0xFF000000) >> 24);
}


static UINT16 swap_bytes16(UINT16 n)
{
        return ((n & 0x00FF) << 8) | ((n & 0xFF00) >> 8);
}


static void copy_and_swap_guid(EFI_GUID *dst, const EFI_GUID *src)
{
        memcpy((CHAR8 *)&dst->Data4, (CHAR8 *)src->Data4, sizeof(src->Data4));
        dst->Data1 = swap_bytes32(src->Data1);
        dst->Data2 = swap_bytes16(src->Data2);
        dst->Data3 = swap_bytes16(src->Data3);
}


static EFI_STATUS open_partition(
                IN const EFI_GUID *guid,
                OUT UINT32 *MediaIdPtr,
                OUT EFI_BLOCK_IO **BlockIoPtr,
                OUT EFI_DISK_IO **DiskIoPtr)
{
        EFI_STATUS ret;
        EFI_BLOCK_IO *BlockIo;
        EFI_DISK_IO *DiskIo;
        UINT32 MediaId;
        UINTN NoHandles = 0;
        EFI_HANDLE *HandleBuffer = NULL;

        /* Get a handle on the partition containing the boot image */
        ret = LibLocateHandleByDiskSignature(
                        MBR_TYPE_EFI_PARTITION_TABLE_HEADER,
                        SIGNATURE_TYPE_GUID,
                        (void *)guid,
                        &NoHandles,
                        &HandleBuffer);
        if (EFI_ERROR(ret) || NoHandles == 0) {
                /* Workaround for old installers which incorrectly wrote
                 * GUIDs strings as little-endian */
                EFI_GUID g;
                copy_and_swap_guid(&g, guid);
                ret = LibLocateHandleByDiskSignature(
                                MBR_TYPE_EFI_PARTITION_TABLE_HEADER,
                                SIGNATURE_TYPE_GUID,
                                (void *)&g,
                                &NoHandles,
                                &HandleBuffer);
                if (EFI_ERROR(ret)) {
                        error(L"LibLocateHandle", ret);
                        return ret;
                }
        }
        if (NoHandles != 1) {
                Print(L"%d handles found for GUID, expecting 1: %g\n",
                                NoHandles, guid);
                ret = EFI_VOLUME_CORRUPTED;
                goto out;
        }

        /* Instantiate BlockIO and DiskIO protocols so we can read various data */
        ret = uefi_call_wrapper(BS->HandleProtocol, 3, HandleBuffer[0],
                        &BlockIoProtocol,
                        (void **)&BlockIo);
        if (EFI_ERROR(ret)) {
                error(L"HandleProtocol (BlockIoProtocol)", ret);
                goto out;;
        }
        ret = uefi_call_wrapper(BS->HandleProtocol, 3, HandleBuffer[0],
                        &DiskIoProtocol, (void **)&DiskIo);
        if (EFI_ERROR(ret)) {
                error(L"HandleProtocol (DiskIoProtocol)", ret);
                goto out;
        }
        MediaId = BlockIo->Media->MediaId;

        *MediaIdPtr = MediaId;
        *BlockIoPtr = BlockIo;
        *DiskIoPtr = DiskIo;
out:
        FreePool(HandleBuffer);
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
                goto out;
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
        FreePool(fileinfo);
        FreePool(bootimage);
        return ret;
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
        if (buf->hdr.signature != 0xAA55) {
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

#if __LP64__
        if (!(buf->hdr.xloadflags & XLF_EFI_HANDOVER_64)) {
#else
        if (!(buf->hdr.xloadflags & XLF_EFI_HANDOVER_32)) {
#endif
                Print(L"EFI Handover protocol not supported\n");
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

        efree(buf->hdr.ramdisk_start, buf->hdr.ramdisk_len);
out_cmdline:
        free_pages(buf->hdr.cmd_line_ptr,
                        strlena((CHAR8 *)(UINTN)buf->hdr.cmd_line_ptr) + 1);
out_bootimage:
        FreePool(bootimage);
        return ret;
}


static VOID StrNCpy(OUT CHAR16 *dest, IN const CHAR16 *src, UINT32 n)
{
        UINT32 i;

        for (i = 0; i < n && src[i] != 0; i++)
                dest[i] = src[i];
        for ( ; i < n; i++)
                dest[i] = 0;
}


static UINT8 getdigit(IN CHAR16 *str)
{
        CHAR16 bytestr[3];
        bytestr[2] = 0;
        StrNCpy(bytestr, str, 2);
        return (UINT8)xtoi(bytestr);
}


EFI_STATUS string_to_guid(
                IN CHAR16 *in_guid_str,
                OUT EFI_GUID *guid)
{
        CHAR16 gstr[37];
        int i;

        StrNCpy(gstr, in_guid_str, 36);
        gstr[36] = 0;
        gstr[8] = 0;
        gstr[13] = 0;
        gstr[18] = 0;
        guid->Data1 = (UINT32)xtoi(gstr);
        guid->Data2 = (UINT16)xtoi(&gstr[9]);
        guid->Data3 = (UINT16)xtoi(&gstr[14]);

        guid->Data4[0] = getdigit(&gstr[19]);
        guid->Data4[1] = getdigit(&gstr[21]);
        for (i = 0; i < 6; i++)
                guid->Data4[i + 2] = getdigit(&gstr[24 + (i * 2)]);

        return EFI_SUCCESS;
}

static EFI_STATUS file_delete (
                IN EFI_FILE *root_dir,
                const CHAR16 *name)
{
        EFI_STATUS ret;
        EFI_FILE *src;
        ret = uefi_call_wrapper(root_dir->Open, 5, root_dir, &src, (CHAR16 *)name, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
        if (EFI_ERROR(ret))
                goto out;
        ret = uefi_call_wrapper(src->Delete, 1, src);
        if (EFI_ERROR(ret))
                error(L"Couldn't delete source file", ret);
        debug("Just deleted the file:%s", name);
out:
        return ret;
}

EFI_STATUS read_bcb(
                IN const EFI_GUID *bcb_guid,
                OUT struct bootloader_message *bcb)
{
        EFI_STATUS ret;
        EFI_BLOCK_IO *BlockIo;
        EFI_DISK_IO *DiskIo;
        UINT32 MediaId;

        debug("Locating BCB");
        ret = open_partition(bcb_guid, &MediaId, &BlockIo, &DiskIo);
        if (EFI_ERROR(ret))
                return EFI_INVALID_PARAMETER;

        debug("Reading BCB");
        ret = uefi_call_wrapper(DiskIo->ReadDisk, 5, DiskIo, MediaId, 0,
                        sizeof(*bcb), bcb);
        if (EFI_ERROR(ret)) {
                error(L"ReadDisk (bcb)", ret);
                return ret;
        }
        bcb->command[31] = '\0';
        bcb->status[31] = '\0';
        debug("BCB read completed successfully");

        return EFI_SUCCESS;
}



EFI_STATUS write_bcb(
                IN const EFI_GUID *bcb_guid,
                IN struct bootloader_message *bcb)
{
        EFI_STATUS ret;
        EFI_BLOCK_IO *BlockIo;
        EFI_DISK_IO *DiskIo;
        UINT32 MediaId;

        debug("Locating BCB");
        ret = open_partition(bcb_guid, &MediaId, &BlockIo, &DiskIo);
        if (EFI_ERROR(ret))
                return EFI_INVALID_PARAMETER;

        debug("Writing BCB");
        ret = uefi_call_wrapper(DiskIo->WriteDisk, 5, DiskIo, MediaId, 0,
                        sizeof(*bcb), bcb);
        if (EFI_ERROR(ret)) {
                error(L"WriteDisk (bcb)", ret);
                return ret;
        }
        debug("BCB written successfully");

        return EFI_SUCCESS;
}


/* Convenience function to update fields in the BCB */
VOID set_bcb(
                IN struct bootloader_message *bcb,
                IN CHAR16 *command,
                IN CHAR16 *status,
                IN EFI_STATUS code)
{
        if (status) {
                if (code != EFI_SUCCESS) {
                        CHAR16 *tmp;
                        tmp = PoolPrint(L"%s: %r", status, code);
                        if (tmp) {
                                str_to_stra(bcb->status, tmp, sizeof(bcb->status));
                                FreePool(tmp);
                        }
                } else {
                        str_to_stra(bcb->status, status, sizeof(bcb->status));
                }
        }
        if (command)
                str_to_stra(bcb->command, command, sizeof(bcb->command));
}


EFI_STATUS android_load_bcb(
                IN EFI_FILE *root_dir,
                IN const EFI_GUID *bcb_guid,
                OUT CHAR16 **var)
{
        EFI_STATUS ret, outval;
        struct bootloader_message bcb;
        BOOLEAN reset;
        EFI_RESET_TYPE reset_type;

        outval = EFI_NOT_FOUND;
        reset = FALSE;

        ret = read_bcb(bcb_guid, &bcb);
        if (EFI_ERROR(ret))
                return ret;

        /* We own the status field; clear it in case there is any stale data */
        bcb.status[0] = '\0';

        if (!strcmpa(bcb.command, (CHAR8 *)"recovery-firmware")) {
                reset = update_firmware(root_dir, &bcb, TRUE, &reset_type);
        } else if (!strcmpa(bcb.command, (CHAR8 *)"update-firmware")) {
                reset = update_firmware(root_dir, &bcb, FALSE, &reset_type);
        }

        if (!strncmpa(bcb.command, (CHAR8 *)"boot-", 5)) {
                *var = stra_to_str(bcb.command + 5);
                outval = EFI_SUCCESS;
                debug("BCB boot target: '%s'", *var);
        }

        ret = write_bcb(bcb_guid, &bcb);
        if (EFI_ERROR(ret)) {
                Print(L"Unable to update BCB contents!\n");
                return ret;
        }
        if (reset) {
                debug ("I am about to reset the system");
                uefi_call_wrapper(RT->ResetSystem, 4, reset_type,
                                EFI_SUCCESS, 0, NULL);
        }

        return outval;
}

/* vim: softtabstop=8:shiftwidth=8:expandtab
 */
