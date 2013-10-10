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
#include "uefi_osnib.h"

#define INTEL_OSNIB_GUID	{0x80868086, 0x8086, 0x8086, {0x80, 0x86, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00}}
#define INTEL_OSNIB_VARNAME	L"IntelOsnib"

int uefi_is_osnib_corrupted(void)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return 0;
}

void uefi_reset_osnib(void)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return;
}

void *uefi_get_osnib(void)
{
	EFI_GUID osnib_guid = INTEL_OSNIB_GUID;
	return LibGetVariable(INTEL_OSNIB_VARNAME, &osnib_guid);
}

void uefi_set_osnib(VOID *osnib, int size)
{
	EFI_GUID osnib_guid = INTEL_OSNIB_GUID;
	EFI_STATUS ret;

	ret = LibSetNVVariable(INTEL_OSNIB_VARNAME, &osnib_guid, size, osnib);
	if (EFI_ERROR(ret))
		error(L"Failed to store OSNIB in NVRAM: %r\n", ret);
}
