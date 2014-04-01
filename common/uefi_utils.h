/*
 * Copyright (c) 2013, Intel Corporation
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

#ifndef __UEFI_UTILS_H__
#define __UEFI_UTILS_H__

#include <efi.h>

#define UINTN_MAX ((UINTN)-1);
#define offsetof(TYPE, MEMBER) ((UINTN) &((TYPE *)0)->MEMBER)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))
#define max(x,y) (x < y ? y : x)

struct EFI_LOAD_OPTION {
	UINT32 Attributes;
	UINT16 FilePathListLength;
} __attribute__((packed));

EFI_STATUS ConvertBmpToGopBlt (VOID *BmpImage, UINTN BmpImageSize,
			       VOID **GopBlt, UINTN *GopBltSize,
			       UINTN *PixelHeight, UINTN *PixelWidth);
EFI_STATUS gop_display_blt(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt, UINTN height, UINTN width);
EFI_STATUS get_esp_handle(EFI_HANDLE **esp);
EFI_STATUS get_esp_fs(EFI_FILE_IO_INTERFACE **esp_fs);
EFI_STATUS uefi_read_file(EFI_FILE_IO_INTERFACE *io, CHAR16 *filename, void **data, UINTN *size);
EFI_STATUS uefi_write_file(EFI_FILE_IO_INTERFACE *io, CHAR16 *filename, void *data, UINTN *size);
EFI_STATUS find_device_partition(const EFI_GUID *guid, EFI_HANDLE **handles, UINTN *no_handles);
void uefi_reset_system(EFI_RESET_TYPE reset_type);
void uefi_shutdown(void);
EFI_STATUS uefi_delete_file(EFI_FILE_IO_INTERFACE *io, CHAR16 *filename);
BOOLEAN uefi_exist_file(EFI_FILE *parent, CHAR16 *filename);
BOOLEAN uefi_exist_file_root(EFI_FILE_IO_INTERFACE *io, CHAR16 *filename);
EFI_STATUS uefi_create_directory(EFI_FILE *parent, CHAR16 *dirname);
EFI_STATUS uefi_create_directory_root(EFI_FILE_IO_INTERFACE *io, CHAR16 *dirname);
EFI_STATUS uefi_set_simple_var(char *name, EFI_GUID *guid, int size, void *data,
			       BOOLEAN persistent);
INT8 uefi_get_simple_var(char *name, EFI_GUID *guid);
EFI_STATUS uefi_usleep(UINTN useconds);
EFI_STATUS uefi_msleep(UINTN mseconds);

EFI_STATUS str_to_stra(CHAR8 *dst, CHAR16 *src, UINTN len);
CHAR16 *stra_to_str(CHAR8 *src);
VOID StrNCpy(OUT CHAR16 *dest, IN const CHAR16 *src, UINT32 n);
UINT8 getdigit(IN CHAR16 *str);
EFI_STATUS string_to_guid(IN CHAR16 *in_guid_str, OUT EFI_GUID *guid);
UINT32 swap_bytes32(UINT32 n);
UINT16 swap_bytes16(UINT16 n);
void copy_and_swap_guid(EFI_GUID *dst, const EFI_GUID *src);
EFI_STATUS open_partition(
                IN const EFI_GUID *guid,
                OUT UINT32 *MediaIdPtr,
                OUT EFI_BLOCK_IO **BlockIoPtr,
                OUT EFI_DISK_IO **DiskIoPtr);
void path_to_dos(CHAR16 *path);
CHAR8 *append_strings(CHAR8 *s1, CHAR8 *s2);
UINTN strtoul16(const CHAR16 *nptr, CHAR16 **endptr, UINTN base);
UINTN split_cmdline(CHAR16 *cmdline, UINTN max_token, CHAR16 *args[]);

void dump_buffer(CHAR8 *b, UINTN size);

/* Basic port I/O */
static inline void outb(UINT16 port, UINT8 value)
{
	asm volatile("outb %0,%1" : : "a" (value), "dN" (port));
}

static inline UINT8 inb(UINT16 port)
{
	UINT8 value;
	asm volatile("inb %1,%0" : "=a" (value) : "dN" (port));
	return value;
}


#endif /* __UEFI_UTILS_H__ */
