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
#include "platform/platform.h"
#include "config.h"

/* Warning: These macros requires that the data is a contained in a BYTE ! */
#define set_osnib_var(var, persistent)				\
	uefi_set_simple_var(#var, &osloader_guid, 1, &var, persistent)

#define get_osnib_var(var)			\
	uefi_get_simple_var(#var, &osloader_guid)

static EFI_STATUS uefi_set_simple_var(char *name, EFI_GUID *guid, int size, void *data,
				      BOOLEAN persistent)
{
	EFI_STATUS ret;
	CHAR16 *name16 = stra_to_str((CHAR8 *)name);

	if (persistent)
		ret = LibSetNVVariable(name16, guid, size, data);
	else
		ret = LibSetVariable(name16, guid, size, data);

	free(name16);
	return ret;
}

static INT8 uefi_get_simple_var(char *name, EFI_GUID *guid)
{
	void *buffer;
	UINT64 ret;
	UINTN size;
	CHAR16 *name16 = stra_to_str((CHAR8 *)name);
	buffer = LibGetVariableAndSize(name16, guid, &size);
	free(name16);

	if (buffer == NULL) {
		error(L"Failed to get variable %s\n", name16);
		return -1;
	}

	if (size > sizeof(ret)) {
		error(L"Tried to get UEFI variable larger than %d bytes (%d bytes)."
		      " Please use an appropriate retrieve method.\n", sizeof(ret), size);
		ret = -1;
		goto out;
	}

	ret = *(INT8 *)buffer;
out:
	free(buffer);
	return ret;
}

EFI_STATUS uefi_set_target_mode(enum targets TargetMode)
{
	return set_osnib_var(TargetMode, FALSE);
}

EFI_STATUS uefi_set_rtc_alarm_charging(int RtcAlarmCharging)
{
	return set_osnib_var(RtcAlarmCharging, TRUE);
}

EFI_STATUS uefi_set_wdt_counter(int WdtCounter)
{
	return set_osnib_var(WdtCounter, TRUE);
}

enum targets uefi_get_target_mode(void)
{
	return get_osnib_var(TargetMode);
}

int uefi_get_rtc_alarm_charging(void)
{
	return get_osnib_var(RtcAlarmCharging);
}

int uefi_get_wdt_counter(void)
{
	int counter = get_osnib_var(WdtCounter);
	return counter == -1 ? 0 : counter;
}

CHAR8 *uefi_get_extra_cmdline(void)
{
	return LibGetVariable(L"ExtraKernelCommandLine", &osloader_guid);
}

void uefi_populate_osnib_variables(void)
{
	struct int_var {
		int (*get_value)(void);
		char *name;
	} int_vars[] = {
		{ (int (*)(void))loader_ops.get_wake_source, "WakeSource" },
		{ (int (*)(void))loader_ops.get_reset_source, "ResetSource" },
		{ (int (*)(void))loader_ops.get_reset_type, "ResetType" },
		{ (int (*)(void))loader_ops.get_shutdown_source, "ShutdownSource" }
	};

	EFI_STATUS ret;
	int i;
	for (i = 0 ; i < sizeof(int_vars)/sizeof(int_vars[0]) ; i++) {
		struct int_var *var = int_vars + i;
		int value = var->get_value();

		ret = uefi_set_simple_var(var->name, &osloader_guid, 1, &value, FALSE);
		if (EFI_ERROR(ret))
			error(L"Failed to set %s osnib EFI variable", var->name, ret);
	}
}
