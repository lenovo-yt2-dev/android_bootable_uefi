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

struct EFI_LOAD_OPTION {
	UINT32 Attributes;
	UINT16 FilePathListLength;
} __attribute__((packed));

EFI_STATUS ConvertBmpToGopBlt (VOID *BmpImage, UINTN BmpImageSize,
			       VOID **GopBlt, UINTN *GopBltSize,
			       UINTN *PixelHeight, UINTN *PixelWidth);
EFI_STATUS gop_display_blt(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt, UINTN blt_size, UINTN height, UINTN width);
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
EFI_STATUS uefi_file_get_size(EFI_HANDLE image, CHAR16 *filename, UINT64 *size);
EFI_STATUS uefi_call_image(IN EFI_HANDLE parent_image, IN EFI_HANDLE device,
			   IN CHAR16 *filename, OUT UINTN *exit_data_size, OUT CHAR16 **exit_data);
EFI_STATUS uefi_set_simple_var(char *name, EFI_GUID *guid, int size, void *data,
			       BOOLEAN persistent);
INT8 uefi_get_simple_var(char *name, EFI_GUID *guid);

#endif /* __UEFI_UTILS_H__ */
