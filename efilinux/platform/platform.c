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

static enum reset_types stub_get_reset_type(void)
{
	debug(L"WARNING: stubbed!\n");
	return NOT_APPLICABLE;
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

static enum batt_levels stub_get_batt_level(void)
{
	debug(L"WARNING: stubbed!\n");
	return BATT_BOOT_OS;
}

static int stub_combo_key(enum combo_keys combo)
{
	debug(L"WARNING: stubbed!\n");
	return 0;
}

static EFI_STATUS stub_set_target_mode(enum targets target)
{
	debug(L"WARNING: stubbed!\n");
	return EFI_SUCCESS;
}

static EFI_STATUS stub_set_rtc_alarm_charging(int val)
{
	debug(L"WARNING: stubbed!\n");
	return EFI_SUCCESS;
}

static EFI_STATUS stub_set_wdt_counter(int val)
{
	debug(L"WARNING: stubbed!\n");
	return EFI_SUCCESS;
}

static enum targets stub_get_target_mode(void)
{
	debug(L"WARNING: stubbed!\n");
	return TARGET_BOOT;
}

static int stub_get_rtc_alarm_charging(void)
{
	debug(L"WARNING: stubbed!\n");
	return 0;
}

static int stub_get_wdt_counter(void)
{
	debug(L"WARNING: stubbed!\n");
	return 0;
}

static void stub_hook_bootlogic_begin(void)
{
	debug(L"WARNING: stubbed!\n");
}

EFI_STATUS stub_update_boot(void)
{
	debug(L"WARNING: stubbed!\n");
	return EFI_SUCCESS;
}

EFI_STATUS stub_display_splash(void)
{
	debug(L"WARNING: stubbed!\n");
	return EFI_SUCCESS;
}

int stub_is_battery_ok(void)
{
	debug(L"WARNING: stubbed!\n");
	return 1;
}

void stub_print_battery_infos(void)
{
	debug(L"WARNING: stubbed!\n");
}

static EFI_STATUS stub_hash_verify(VOID *blob, UINTN blob_size,
		VOID *sig, UINTN sig_size)
{
	debug(L"WARNING: stubbed!\n");
	return EFI_SUCCESS;
}

struct osloader_ops loader_ops = {
	.check_partition_table = stub_check_partition_table,
	.read_flow_type = stub_read_flow_type,
	.do_cold_off = stub_do_cold_off,
	.populate_indicators = stub_populate_indicators,
	.load_target = stub_load_target,
	.get_wake_source = stub_get_wake_source,
	.get_reset_source = stub_get_reset_source,
	.get_reset_type = stub_get_reset_type,
	.get_shutdown_source = stub_get_shutdown_source,
	.is_osnib_corrupted = stub_is_osnib_corrupted,
	.get_battery_level = stub_get_batt_level,
	.combo_key = stub_combo_key,
	.set_target_mode = stub_set_target_mode,
	.set_rtc_alarm_charging = stub_set_rtc_alarm_charging,
	.set_wdt_counter = stub_set_wdt_counter,
	.get_target_mode = stub_get_target_mode,
	.get_rtc_alarm_charging = stub_get_rtc_alarm_charging,
	.get_wdt_counter = stub_get_wdt_counter,
	.hook_bootlogic_begin = stub_hook_bootlogic_begin,
	.update_boot = stub_update_boot,
	.display_splash = stub_display_splash,
	.is_battery_ok = stub_is_battery_ok,
	.print_battery_infos = stub_print_battery_infos,
	.hash_verify = stub_hash_verify,
};
