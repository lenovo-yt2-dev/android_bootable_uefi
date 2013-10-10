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
#include <efilinux.h>
#include "platform.h"

void init_cherrytrail(void);

enum intel_platforms {
	PLATFORM_CHERRYTRAIL,
	PLATFORM_UNKNOWN,
};

enum intel_platforms identify_platform(void)
{
	/* TODO */
	debug(L"NOT IMPLEMENTED\n");
	return PLATFORM_CHERRYTRAIL;
}

EFI_STATUS init_platform_functions(void)
{
	EFI_STATUS ret = EFI_SUCCESS;
	switch (identify_platform()) {
	case PLATFORM_CHERRYTRAIL:
		init_cherrytrail();
		break;
	default:
		Print(L"Error: unknown platform!\n");
		ret = EFI_INVALID_PARAMETER;
	}
	return ret;
}

/* Stub implementation of loader ops */
static EFI_STATUS stub_check_partition_table(void)
{
	debug(L"WARNING: stubbed!\n");
	return EFI_SUCCESS;
}

static enum flow_types stub_read_flow_type(void)
{
	debug(L"WARNING: stubbed!\n");
	return FLOW_UNKNOWN;
}

static void stub_do_cold_off(void)
{
	debug(L"WARNING: stubbed!\n");
	return;
}

static EFI_STATUS stub_populate_indicators(void)
{
	debug(L"WARNING: stubbed!\n");
	return EFI_SUCCESS;
}

static EFI_STATUS stub_load_target(enum targets target)
{
	debug(L"WARNING: stubbed!\n");
	return EFI_LOAD_ERROR;
}

static enum wake_sources stub_get_wake_source(void)
{
	debug(L"WARNING: stubbed!\n");
	return WAKE_NOT_APPLICABLE;
}

static enum reset_sources stub_get_reset_source(void)
{
	debug(L"WARNING: stubbed!\n");
	return WAKE_NOT_APPLICABLE;
}

static enum shutdown_sources stub_get_shutdown_source(void)
{
	debug(L"WARNING: stubbed!\n");
	return SHTDWN_NOT_APPLICABLE;
}

static int stub_is_osnib_corrupted(void)
{
	debug(L"WARNING: stubbed!\n");
	return 0;
}

static void stub_reset_osnib(void)
{
	debug(L"WARNING: stubbed!\n");
	return;
}

static enum batt_levels stub_get_batt_level(void)
{
	debug(L"WARNING: stubbed!\n");
	return BATT_BOOT_OS;
}

static enum targets stub_get_target_mode(void)
{
	debug(L"WARNING: stubbed!\n");
	return BATT_BOOT_OS;
}

static int stub_combo_key(enum combo_keys combo)
{
	debug(L"WARNING: stubbed!\n");
	return 0;
}

static void *stub_get_osnib(void)
{
	debug(L"WARNING: stubbed!\n");
	return NULL;
}

static void stub_set_osnib(void * buffer, int size)
{
	debug(L"WARNING: stubbed!\n");
}

static UINT16 stub_get_osnib_default_version(void)
{
	debug(L"WARNING: stubbed!\n");
	return 0x0200;
}

static void stub_hook_bootlogic_end(void)
{
	debug(L"WARNING: stubbed!\n");
}

struct osloader_ops loader_ops = {
	.check_partition_table = stub_check_partition_table,
	.read_flow_type = stub_read_flow_type,
	.do_cold_off = stub_do_cold_off,
	.populate_indicators = stub_populate_indicators,
	.load_target = stub_load_target,
	.get_wake_source = stub_get_wake_source,
	.get_reset_source = stub_get_reset_source,
	.get_shutdown_source = stub_get_shutdown_source,
	.is_osnib_corrupted = stub_is_osnib_corrupted,
	.reset_osnib = stub_reset_osnib,
	.get_batt_level = stub_get_batt_level,
	.get_target_mode = stub_get_target_mode,
	.combo_key = stub_combo_key,
	.get_osnib = stub_get_osnib,
	.set_osnib = stub_set_osnib,
	.get_osnib_default_version = stub_get_osnib_default_version,
	.hook_bootlogic_end = stub_hook_bootlogic_end,
};
