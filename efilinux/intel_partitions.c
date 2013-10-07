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
#include "stdlib.h"
#include "bootlogic.h"
#include "android/boot.h"

#define BOOT_GUID	{0x80868086, 0x8086, 0x8086, {0x80, 0x86, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00}}
#define RECOVERY_GUID	{0x80868086, 0x8086, 0x8086, {0x80, 0x86, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01}}
#define FASTBOOT_GUID	{0x80868086, 0x8086, 0x8086, {0x80, 0x86, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}}
#define TEST_GUID	{0x80868086, 0x8086, 0x8086, {0x80, 0x86, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04}}
#define NULL_GUID	{0}

struct name_guid {
	CHAR16 *name;
	EFI_GUID guid;
};

static struct name_guid android_guids[TARGET_UNKNOWN + 1] = {
	{L"boot"	, BOOT_GUID},
	{L"recovery"	, RECOVERY_GUID},
	{L"fastboot"	, FASTBOOT_GUID},
	{L"test"	, TEST_GUID},
	{NULL           , NULL_GUID},
	{NULL           , NULL_GUID},
};

int is_loadable_target(enum targets target)
{
	return android_guids[target].name != NULL;
}

EFI_STATUS intel_load_target(enum targets target)
{
	EFI_GUID part_guid;
	debug(L"Loading target %a\n", target_strings[target]);

	if (!is_loadable_target(target))
	    return EFI_INVALID_PARAMETER;

	part_guid = android_guids[target].guid;

	return android_image_start_partition(NULL, &part_guid, NULL);
}

EFI_STATUS name_to_guid(CHAR16 *name, EFI_GUID *guid) {
	int i;
	for (i = 0; i < sizeof(android_guids); i++) {
		if (!StrCmp(name, android_guids[i].name)) {
			memcpy((CHAR8 *)guid, (CHAR8 *)&android_guids[i].guid, sizeof(EFI_GUID));
			return EFI_SUCCESS;
		}
	}
	return EFI_INVALID_PARAMETER;
}

EFI_STATUS check_gpt(void) {
	/* TODO */
	debug(L"NOT IMPLEMENTED\n");
	return EFI_SUCCESS;
}
