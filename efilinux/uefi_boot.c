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
#include "uefi_utils.h"
#include "splash.h"
#include "intel_partitions.h"

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

static const CHAR16	*target_mode_name      = L"LoaderEntryOneShot";
static const CHAR16	*last_target_mode_name = L"LoaderEntryLast";

static enum targets get_target_from_var(const CHAR16 *varname)
{
	CHAR16 *name;
	enum targets target;

	name = LibGetVariable((CHAR16 *)varname, (EFI_GUID *)&osloader_guid);
	if (!name)
		return TARGET_UNKNOWN;
	return EFI_ERROR(name_to_target(name, &target)) ? TARGET_UNKNOWN : target;
}

enum targets get_entry_oneshot(void)
{
	return get_target_from_var(target_mode_name);
}

enum targets get_entry_last(void)
{
	return get_target_from_var(last_target_mode_name);
}

EFI_STATUS set_entry_last(enum targets target)
{
	CHAR16 *name;
	EFI_STATUS status;

	status = target_to_name(target, &name);
	if (EFI_ERROR(status)) {
		error(L"Target name not found for target_id=%x\n", target);
		return status;
	}

	status = LibDeleteVariable((CHAR16 *)target_mode_name, &osloader_guid);
	if (status != EFI_NOT_FOUND)
		warning(L"Failed to delete %s variable\n", target_mode_name);

	return LibSetNVVariable((CHAR16 *)last_target_mode_name, (EFI_GUID *)&osloader_guid,
				StrSize(name), name);
}
