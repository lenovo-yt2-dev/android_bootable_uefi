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

#include <efi.h>
#include <efilib.h>
#include "efilinux.h"
#include "bootlogic.h"
#include "protocol.h"
#include "uefi_utils.h"
#include "splash.h"

static EFI_STATUS get_esp_handle(EFI_HANDLE **esp)
{
	EFI_STATUS ret;
	UINTN no_handles;
	EFI_HANDLE *handles;
	CHAR16 *description;
	EFI_DEVICE_PATH *device;

	ret = find_device_partition(&EfiPartTypeSystemPartitionGuid, &handles, &no_handles);
	if (EFI_ERROR(ret)) {
		error(L"Failed to found partition: %r\n", ret);
		goto out;
	}

	if (LOGLEVEL(DEBUG)) {
		UINTN i;
		debug(L"Found %d devices\n", no_handles);
		for (i = 0; i < no_handles; i++) {
			device = DevicePathFromHandle(handles[i]);
			description = DevicePathToStr(device);
			debug(L"Device : %s\n", description);
			free_pool(description);
		}
	}

	if (no_handles == 0) {
		error(L"Can't find loader partition!\n");
		ret = EFI_NOT_FOUND;
		goto out;
	}
	if (no_handles > 1) {
		error(L"Multiple loader partition found!\n");
		goto free_handles;
	}
	*esp = handles[0];
	return EFI_SUCCESS;

free_handles:
	if (handles)
		free(handles);
out:
	return ret;
}

EFI_STATUS uefi_display_splash(void)
{
	EFI_STATUS ret;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
	UINTN blt_size;
	UINTN height;
	UINTN width;

	Blt = NULL;
	ret = ConvertBmpToGopBlt(splash_bmp, splash_bmp_size, (void **)&Blt, &blt_size, &height, &width);
 	if (EFI_ERROR(ret)) {
		error(L"Failed to convert bmp to blt: %r\n", ret);
		goto error;
	}

	ret = gop_display_blt(Blt, blt_size, height, width);
 	if (EFI_ERROR(ret)) {
		error(L"Failed to display blt: %r\n", ret);
		goto error;
	}

error:
	if (EFI_ERROR(ret))
		error(L"Failed to display splash:%r\n", ret);
	return ret;
}
