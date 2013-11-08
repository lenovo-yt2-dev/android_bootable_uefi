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
#include "utils.h"

#define BOOT_GUID	{0x80868086, 0x8086, 0x8086, {0x80, 0x86, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00}}
#define RECOVERY_GUID	{0x80868086, 0x8086, 0x8086, {0x80, 0x86, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01}}
#define FASTBOOT_GUID	{0x80868086, 0x8086, 0x8086, {0x80, 0x86, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}}
#define TEST_GUID	{0x80868086, 0x8086, 0x8086, {0x80, 0x86, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04}}
#define NULL_GUID	{0}

struct target_entry {
	enum targets target;
	CHAR16 *name;
	EFI_GUID guid;
	CHAR8 *cmdline;
};

static struct target_entry android_entries[] = {
	{TARGET_BOOT	 , L"boot"	, BOOT_GUID     , (CHAR8 *) "androidboot.mode=main"},
	{TARGET_RECOVERY , L"recovery"	, RECOVERY_GUID , (CHAR8 *) "androidboot.mode=fota"},
	{TARGET_FASTBOOT , L"fastboot"	, FASTBOOT_GUID , (CHAR8 *) "androidboot.mode=fastboot"},
	{TARGET_TEST	 , L"test"	, TEST_GUID     , (CHAR8 *) "androidboot.mode=test"},
	{TARGET_CHARGING , L"charging"	, BOOT_GUID     , (CHAR8 *) "androidboot.mode=charger"},
	{TARGET_ERROR	 , NULL		, NULL_GUID     , NULL},
	{TARGET_ERROR	 , NULL		, NULL_GUID     , NULL},
};

struct target_entry *get_target_entry(enum targets target)
{
	UINTN i;
	for (i = 0; i < sizeof(android_entries) / sizeof(*android_entries); i++)
		if (android_entries[i].target == target)
			return &android_entries[i];
	return NULL;
}

int is_loadable_target(struct target_entry *entry)
{
	return entry->name != NULL;
}

EFI_STATUS intel_load_target(enum targets target, CHAR8 *cmdline)
{
	CHAR8 *updated_cmdline;

	struct target_entry *entry = get_target_entry(target);
	if (!entry) {
		error(L"Target 0x%x not supported\n", target);
		return EFI_UNSUPPORTED;
	}

	if (!is_loadable_target(entry))
	    return EFI_INVALID_PARAMETER;

	updated_cmdline = append_strings(cmdline, entry->cmdline);
	if (cmdline)
		free(cmdline);

	debug(L"target cmdline = %a\n", cmdline);
	debug(L"Loading target %s\n", entry->name);

	return android_image_start_partition(NULL, &entry->guid, updated_cmdline);
}

EFI_STATUS name_to_guid(CHAR16 *name, EFI_GUID *guid) {
	int i;
	for (i = 0; i < sizeof(android_entries) / sizeof(*android_entries); i++) {
		if (!StrCmp(name, android_entries[i].name)) {
			memcpy((CHAR8 *)guid, (CHAR8 *)&android_entries[i].guid, sizeof(EFI_GUID));
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
