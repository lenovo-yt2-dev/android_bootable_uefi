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

#include "gummiboot.h"
#include "android.h"
#include "efilinux.h"

struct setup_header {
        UINT8 setup_secs;        /* Sectors for setup code */
        UINT16 root_flags;
        UINT32 sys_size;
        UINT16 ram_size;
        UINT16 video_mode;
        UINT16 root_dev;
        UINT16 signature;        /* Boot signature */
        UINT16 jump;
        UINT32 header;
        UINT16 version;
        UINT16 su_switch;
        UINT16 setup_seg;
        UINT16 start_sys;
        UINT16 kernel_ver;
        UINT8 loader_id;
        UINT8 load_flags;
        UINT16 movesize;
        UINT32 code32_start;        /* Start of code loaded high */
        UINT32 ramdisk_start;        /* Start of initial ramdisk */
        UINT32 ramdisk_len;        /* Lenght of initial ramdisk */
        UINT32 bootsect_kludge;
        UINT16 heap_end;
        UINT8 ext_loader_ver;  /* Extended boot loader version */
        UINT8 ext_loader_type; /* Extended boot loader ID */
        UINT32 cmd_line_ptr;   /* 32-bit pointer to the kernel command line */
        UINT32 ramdisk_max;    /* Highest legal initrd address */
        UINT32 kernel_alignment; /* Physical addr alignment required for kernel */
        UINT8 relocatable_kernel; /* Whether kernel is relocatable or not */
        UINT8 _pad2[3];
        UINT32 cmdline_size;
        UINT32 hardware_subarch;
        UINT64 hardware_subarch_data;
        UINT32 payload_offset;
        UINT32 payload_length;
        UINT64 setup_data;
        UINT64 pref_address;
        UINT32 init_size;
        UINT32 handover_offset;
} __attribute__((packed));

struct efi_info {
        UINT32 efi_loader_signature;
        UINT32 efi_systab;
        UINT32 efi_memdesc_size;
        UINT32 efi_memdesc_version;
        UINT32 efi_memmap;
        UINT32 efi_memmap_size;
        UINT32 efi_systab_hi;
        UINT32 efi_memmap_hi;
};

struct e820_entry {
        UINT64 addr;                /* start of memory segment */
        UINT64 size;                /* size of memory segment */
        UINT32 type;                /* type of memory segment */
} __attribute__((packed));

struct screen_info {
        UINT8  orig_x;           /* 0x00 */
        UINT8  orig_y;           /* 0x01 */
        UINT16 ext_mem_k;        /* 0x02 */
        UINT16 orig_video_page;  /* 0x04 */
        UINT8  orig_video_mode;  /* 0x06 */
        UINT8  orig_video_cols;  /* 0x07 */
        UINT8  flags;            /* 0x08 */
        UINT8  unused2;          /* 0x09 */
        UINT16 orig_video_ega_bx;/* 0x0a */
        UINT16 unused3;          /* 0x0c */
        UINT8  orig_video_lines; /* 0x0e */
        UINT8  orig_video_isVGA; /* 0x0f */
        UINT16 orig_video_points;/* 0x10 */

        /* VESA graphic mode -- linear frame buffer */
        UINT16 lfb_width;        /* 0x12 */
        UINT16 lfb_height;       /* 0x14 */
        UINT16 lfb_depth;        /* 0x16 */
        UINT32 lfb_base;         /* 0x18 */
        UINT32 lfb_size;         /* 0x1c */
        UINT16 cl_magic, cl_offset; /* 0x20 */
        UINT16 lfb_linelength;   /* 0x24 */
        UINT8  red_size;         /* 0x26 */
        UINT8  red_pos;          /* 0x27 */
        UINT8  green_size;       /* 0x28 */
        UINT8  green_pos;        /* 0x29 */
        UINT8  blue_size;        /* 0x2a */
        UINT8  blue_pos;         /* 0x2b */
        UINT8  rsvd_size;        /* 0x2c */
        UINT8  rsvd_pos;         /* 0x2d */
        UINT16 vesapm_seg;       /* 0x2e */
        UINT16 vesapm_off;       /* 0x30 */
        UINT16 pages;            /* 0x32 */
        UINT16 vesa_attributes;  /* 0x34 */
        UINT32 capabilities;     /* 0x36 */
        UINT8  _reserved[6];     /* 0x3a */
} __attribute__((packed));

struct boot_params {
        struct screen_info screen_info;
        UINT8 apm_bios_info[0x14];
        UINT8 _pad2[4];
        UINT64 tboot_addr;
        UINT8 ist_info[0x10];
        UINT8 _pad3[16];
        UINT8 hd0_info[16];
        UINT8 hd1_info[16];
        UINT8 sys_desc_table[0x10];
        UINT8 olpc_ofw_header[0x10];
        UINT8 _pad4[128];
        UINT8 edid_info[0x80];
        struct efi_info efi_info;
        UINT32 alt_mem_k;
        UINT32 scratch;
        UINT8 e820_entries;
        UINT8 eddbuf_entries;
        UINT8 edd_mbr_sig_buf_entries;
        UINT8 _pad6[6];
        struct setup_header hdr;
        UINT8 _pad7[0x290-0x1f1-sizeof(struct setup_header)];
        UINT32 edd_mbr_sig_buffer[16];
        struct e820_entry e820_map[128];
        UINT8 _pad8[48];
        UINT8 eddbuf[0x1ec];
        UINT8 _pad9[276];
};

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
        char command[32];
        char status[32];
        char recovery[1024];
};

typedef struct boot_img_hdr boot_img_hdr;

#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512

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
    unsigned unused[2];    /* future expansion: should be 0 */

    unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */

    unsigned char cmdline[BOOT_ARGS_SIZE];

    unsigned id[8]; /* timestamp / checksum / sha1 / etc */
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
**
** n = (kernel_size + page_size - 1) / page_size
** m = (ramdisk_size + page_size - 1) / page_size
** o = (second_size + page_size - 1) / page_size
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
*/

#if __LP64__
typedef void(*handover_func)(void *, EFI_SYSTEM_TABLE *, struct boot_params *);

static inline void handover_jump(EFI_HANDLE image, struct boot_params *bp,
                                 EFI_PHYSICAL_ADDRESS kernel_start)
{
        UINT32 offset = bp->hdr.handover_offset;
        handover_func hf;

        asm volatile ("cli");

        /* The 64-bit kernel entry is 512 bytes after the start. */
        kernel_start += 512;

        hf = (handover_func)(kernel_start + offset);
        hf(image, ST, bp);
}
#else
static inline void handover_jump(EFI_HANDLE image, struct boot_params *bp,
                                 EFI_PHYSICAL_ADDRESS kernel_start)
{
        kernel_start += bp->hdr.handover_offset;

        asm volatile ("cli                \n"
                      "pushl %0         \n"
                      "pushl %1         \n"
                      "pushl %2         \n"
                      "movl %3, %%ecx        \n"
                      "jmp *%%ecx        \n"
                      :: "m" (bp), "m" (ST),
                         "m" (image), "m" (kernel_start));
}
#endif


static VOID error(CHAR16 *str, EFI_STATUS ret)
{
        Print(L"%s: %r\n", str, ret);
}


static EFI_STATUS str_to_stra(CHAR8 *dst, CHAR16 *src, UINTN len)
{
        UINTN i;

        /* This is NOT how to do UTF16 to UTF8 conversion. For now we're just
         * going to hope that nobody's putting non-ASCII characters in
         * the kernel command line! We'll at least abort with an error
         * if we see any funny stuff */
        for (i = 0; i < len; i++) {
                if (src[i] > 0x7F) {
                        Print(L"Non-ascii characters in command line!\n");
                        FreePool(dst);
                        return EFI_INVALID_PARAMETER;
                }
                dst[i] = (CHAR8)src[i];
        }
        return EFI_SUCCESS;
}


static inline void memset(CHAR8 *dst, CHAR8 ch, UINTN size)
{
        UINTN i;

        for (i = 0; i < size; i++)
                dst[i] = ch;
}


static inline void memcpy(CHAR8 *dst, CHAR8 *src, UINTN size)
{
        UINTN i;

        for (i = 0; i < size; i++)
                *dst++ = *src++;
}


static UINT32 pages(struct boot_img_hdr *hdr, UINT32 blob_size)
{
        return (blob_size + hdr->page_size - 1) / hdr->page_size;
}


static EFI_STATUS verify_boot_image(UINT8 *bootimage)
{
        /* TODO: Read digital signature and verify that the boot image
         * is properly signed using UEFI Shim APIs. */

        return EFI_SUCCESS;
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


static EFI_STATUS setup_command_line(UINT8 *bootimage,
                CHAR16 *gummiboot_opts)
{
        CHAR16 *aosp_hdr_opts = NULL;
        CHAR16 *full_cmdline = NULL;
        EFI_PHYSICAL_ADDRESS cmdline_addr;
        CHAR8 *cmdline;
        UINTN cmdlen;
        EFI_STATUS ret;
        struct boot_img_hdr *aosp_header;
        struct boot_params *buf;

        aosp_header = (struct boot_img_hdr *)bootimage;
        buf = (struct boot_params *)(bootimage + aosp_header->page_size);

        aosp_hdr_opts = stra_to_str(aosp_header->cmdline);
        if (!aosp_hdr_opts) {
                ret = EFI_OUT_OF_RESOURCES;
                goto out;
        }
        full_cmdline = PoolPrint(L"%s %s", aosp_hdr_opts, gummiboot_opts);
        if (!full_cmdline) {
                ret = EFI_OUT_OF_RESOURCES;
                goto out;
        }
        /* Documentation/x86/boot.txt: "The kernel command line can be located
         * anywhere between the end of the setup heap and 0xA0000" */
        cmdline_addr = 0xA0000;
        cmdlen = StrLen(full_cmdline);
        ret = allocate_pages(AllocateMaxAddress, EfiLoaderData,
                             EFI_SIZE_TO_PAGES(cmdlen + 1),
                             &cmdline_addr);
        if (EFI_ERROR(ret))
                goto out;

        cmdline = (CHAR8 *)(UINTN)cmdline_addr;
        ret = str_to_stra(cmdline, full_cmdline, cmdlen + 1);
        if (EFI_ERROR(ret)) {
                free_pages(cmdline_addr, EFI_SIZE_TO_PAGES(cmdlen + 1));
                goto out;
        }

        buf->hdr.cmd_line_ptr = (UINT32)(UINTN)cmdline;
        ret = EFI_SUCCESS;
out:
        FreePool(gummiboot_opts);
        FreePool(aosp_hdr_opts);
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
        UINTN NoHandles;
        EFI_HANDLE *HandleBuffer = NULL;

        /* Get a handle on the partition containing the boot image */
        ret = LibLocateHandleByDiskSignature(
                        MBR_TYPE_EFI_PARTITION_TABLE_HEADER,
                        SIGNATURE_TYPE_GUID,
                        (void *)guid,
                        &NoHandles,
                        &HandleBuffer);
        if (EFI_ERROR(ret)) {
                error(L"LibLocateHandle", ret);
                return ret;
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



static EFI_STATUS load_bootimage(
                IN const EFI_GUID *guid,
                OUT UINT8 **bootimage_ptr)
{
        EFI_BLOCK_IO *BlockIo;
        EFI_DISK_IO *DiskIo;
        UINT32 MediaId;
        UINT32 img_size;
        struct boot_img_hdr aosp_header;
        UINT8 *bootimage;
        EFI_STATUS ret;

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

        /* TODO: will need revision when signature is added */
        img_size = (1 + pages(&aosp_header, aosp_header.kernel_size) +
                        pages(&aosp_header, aosp_header.ramdisk_size) +
                        pages(&aosp_header, aosp_header.second_size)) * aosp_header.page_size;
        bootimage = AllocatePool(img_size);
        if (!bootimage)
                return EFI_OUT_OF_RESOURCES;

        debug("Reading full boot image");
        ret = uefi_call_wrapper(DiskIo->ReadDisk, 5, DiskIo, MediaId, 0,
                        img_size, bootimage);
        if (EFI_ERROR(ret))
                FreePool(bootimage);
        else
                *bootimage_ptr = bootimage;
        return ret;
}


EFI_STATUS android_image_start(
                IN EFI_HANDLE parent_image,
                IN const EFI_GUID *guid,
                IN CHAR16 *gummiboot_opts)
{
        EFI_STATUS ret;
        struct boot_params *buf;
        struct boot_img_hdr *aosp_header;
        UINT8 *bootimage;

        debug("Locating boot image");
        ret = load_bootimage(guid, &bootimage);
        if (EFI_ERROR(ret))
                return ret;

        aosp_header = (struct boot_img_hdr *)bootimage;
        buf = (struct boot_params *)(bootimage + aosp_header->page_size);

        if (buf->hdr.version < 0x20b) {
                /* Protocol 2.11, kernel 3.6 required */
                Print(L"Kernel header version %x too old\n", buf->hdr.version);
                ret = EFI_INVALID_PARAMETER;
                goto out_bootimage;
        }

        debug("Verifying the boot image");
        ret = verify_boot_image(bootimage);
        if (EFI_ERROR(ret)) {
                error(L"boot image digital signature verification failed", ret);
                goto out_bootimage;
        }

        debug("Creating command line");
        ret = setup_command_line(bootimage, gummiboot_opts);
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


static VOID StrNCpy(OUT CHAR16 *dest, IN CHAR16 *src, UINT32 n)
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


/* GUIDs are written out little-endian. When reading in as a string,
 * need to byteswap data1, data2 and data3.
 * Note that all the efilib functions which print string representations
 * of EFI_GUIDs don't correctly do this!! */
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
        guid->Data1 = swap_bytes32((UINT32)xtoi(gstr));
        guid->Data2 = swap_bytes16((UINT16)xtoi(&gstr[9]));
        guid->Data3 = swap_bytes16((UINT16)xtoi(&gstr[14]));

        guid->Data4[0] = getdigit(&gstr[19]);
        guid->Data4[1] = getdigit(&gstr[21]);
        for (i = 0; i < 6; i++)
                guid->Data4[i + 2] = getdigit(&gstr[24 + (i * 2)]);

        return EFI_SUCCESS;
}

EFI_STATUS android_load_bcb(
                IN const EFI_GUID *bcb_guid,
                OUT CHAR16 **var)
{
        UINT32 MediaId;
        EFI_STATUS ret;
        EFI_BLOCK_IO *BlockIo;
        EFI_DISK_IO *DiskIo;
        struct bootloader_message bcb;
        CHAR16 *cmd;

        debug("Locating BCB");
        ret = open_partition(bcb_guid, &MediaId, &BlockIo, &DiskIo);
        if (EFI_ERROR(ret))
                return EFI_INVALID_PARAMETER;

        debug("Reading BCB");
        ret = uefi_call_wrapper(DiskIo->ReadDisk, 5, DiskIo, MediaId, 0,
                        sizeof(bcb), &bcb);
        if (EFI_ERROR(ret)) {
                error(L"ReadDisk (bcb)", ret);
                return ret;
        }

        if (strncmpa((CHAR8 *)bcb.command, (CHAR8 *)"boot-", 5))
                return EFI_NOT_FOUND;

        bcb.command[31] = '\0';
        cmd = stra_to_str((CHAR8 *)bcb.command + 5);
        *var = StrDuplicate(cmd);

        debug("BCB: '%s'\n", *var);

        return EFI_SUCCESS;
}

