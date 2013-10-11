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
#include "bootlogic.h"

#define INTEL_OSNIB_GUID	{0x80868086, 0x8086, 0x8086, {0x80, 0x86, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00}}

static EFI_GUID osnib_guid = INTEL_OSNIB_GUID;

/* Warning: These macros requires that the data is a contained in a BYTE ! */
#define set_osnib_var(var)					\
	uefi_set_simple_var(#var, &osnib_guid, 1, &var)

#define get_osnib_var(var)			\
	uefi_get_simple_var(#var, &osnib_guid)

static EFI_STATUS uefi_set_simple_var(char *name, EFI_GUID *guid, int size, void *data)
{
	EFI_STATUS ret;
	CHAR16 *name16 = stra_to_str((CHAR8 *)name);

	ret = LibSetNVVariable(name16, guid, size, data);
	free(name16);
	return ret;
}

static CHAR8 uefi_get_simple_var(char *name, EFI_GUID *guid)
{
	void *buffer;
	UINT64 ret;
	UINTN size;
	CHAR16 *name16 = stra_to_str((CHAR8 *)name);
	buffer = LibGetVariableAndSize(name16, guid, &size);
	free(name16);

	if (size > sizeof(ret)) {
		error(L"Tried to get UEFI variable larger than %d bytes (%d bytes)."
		      " Please use an appropriate retrieve method.\n", sizeof(ret), size);
		ret = -1;
		goto out;
	}

	ret = *(CHAR8 *)buffer;
out:
	free(buffer);
	return ret;
}

EFI_STATUS uefi_set_target_mode(enum targets target_mode)
{
	return set_osnib_var(target_mode);
}

EFI_STATUS uefi_set_rtc_alarm_charging(int rtc_alarm_charging)
{
	return set_osnib_var(rtc_alarm_charging);
}

EFI_STATUS uefi_set_wdt_counter(int wdt_counter)
{
	return set_osnib_var(wdt_counter);
}

enum targets uefi_get_target_mode(void)
{
	return get_osnib_var(target_mode);
}

int uefi_get_rtc_alarm_charging(void)
{
	return get_osnib_var(rtc_alarm_charging);
}

int uefi_get_wdt_counter(void)
{
	return get_osnib_var(wdt_counter);
}

