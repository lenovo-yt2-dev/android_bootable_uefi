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
#include "acpi.h"
#include "bootlogic.h"
#include "platform/platform.h"

#define STRINGIZE(x) #x
char *target_strings[TARGET_UNKNOWN + 1] = {
	STRINGIZE(TARGET_BOOT),
	STRINGIZE(TARGET_RECOVERY),
	STRINGIZE(TARGET_FASTBOOT),
	STRINGIZE(TARGET_TEST),
	STRINGIZE(TARGET_COLD_OFF),
	STRINGIZE(TARGET_CHARGING),
	STRINGIZE(TARGET_UNKNOWN),
};

int batt_boot_os(void)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return 1;
}

enum targets boot_forced_shutdown(enum wake_sources ws)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return TARGET_UNKNOWN;
}

enum targets boot_fastboot_combo(enum wake_sources ws)
{
	if (!loader_ops.combo_key(COMBO_FASTBOOT_MODE))
		return TARGET_UNKNOWN;

	switch(loader_ops.get_batt_level()) {
	case BATT_ERROR:
		error(L"Failed to get battery level. Booting.\n");
	case BATT_BOOT_OS:
	case BATT_BOOT_CHARGING:
		return TARGET_FASTBOOT;
	case BATT_LOW:
		return TARGET_COLD_OFF;
	}

	return TARGET_UNKNOWN;
}

enum targets boot_power_key(enum wake_sources ws)
{
	if (ws != WAKE_POWER_BUTTON_PRESSED)
		return TARGET_UNKNOWN;

	switch(loader_ops.get_batt_level()) {
	case BATT_ERROR:
		error(L"Failed to get battery level. Booting\n");
	case BATT_BOOT_OS:
		return TARGET_BOOT;
	case BATT_BOOT_CHARGING:
		return TARGET_CHARGING;
	case BATT_LOW:
		return TARGET_COLD_OFF;
	}

	return TARGET_UNKNOWN;
}

enum targets boot_rtc(enum wake_sources ws)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return TARGET_UNKNOWN;
}

enum targets boot_battery_insertion(enum wake_sources ws)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return TARGET_UNKNOWN;
}

enum targets boot_charger_insertion(enum wake_sources ws)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return TARGET_UNKNOWN;
}

enum targets target_from_off(enum wake_sources ws)
{
	enum shutdown_sources shutdown_source;
	enum targets target = TARGET_UNKNOWN;
	enum targets (*boot_case[])(enum wake_sources wake_source) = {
		boot_forced_shutdown,
		boot_fastboot_combo,
		boot_power_key,
		boot_rtc,
		boot_battery_insertion,
		boot_charger_insertion,
	};

	shutdown_source = loader_ops.get_shutdown_source();

	if (shutdown_source == SHTDWN_POWER_BUTTON_OVERRIDE) {
		loader_ops.set_target_mode(TARGET_BOOT);
		loader_ops.set_rtc_alarm_charging(0);
		loader_ops.set_wdt_counter(0);
	}

	int i;
	for (i = 0; i < sizeof(boot_case) / sizeof(*boot_case); i++) {
		target = boot_case[i](ws);
		if (target != TARGET_UNKNOWN)
			break;
	}

	return target;
}

enum targets boot_fw_update(enum reset_sources rs)
{
	return rs == RESET_FW_UPDATE ? TARGET_BOOT : TARGET_UNKNOWN;
}

enum targets boot_reset(enum reset_sources rs)
{
	if (rs == RESET_OS_INITIATED || rs == RESET_FORCED)
		return loader_ops.get_target_mode();
	else
		return TARGET_UNKNOWN;
}

enum targets boot_watchdog(enum reset_sources rs)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return TARGET_UNKNOWN;
}

enum targets target_from_reset(enum reset_sources rs)
{
	enum targets target = TARGET_UNKNOWN;
	enum targets (*boot_case[])(enum reset_sources reset_source) = {
		boot_fw_update,
		boot_reset,
		boot_watchdog,
	};

	int i = 0;
	for (i = 0; i < sizeof(boot_case) / sizeof(*boot_case); i++) {
		target = boot_case[i](rs);
		if (target != TARGET_UNKNOWN)
			break;
	}

	return target;
}

enum targets target_from_inputs(enum flow_types flow_type)
{
	enum wake_sources ws;
	enum reset_sources rs;

	ws = loader_ops.get_wake_source();
	debug(L"Wake source = 0x%x\n", ws);
	if (ws == WAKE_ERROR) {
		error(L"Wake source couldn't be retrieved. Falling back in TARGET_BOOT\n");
		return TARGET_BOOT;
	}

	if (ws != WAKE_NOT_APPLICABLE)
		return target_from_off(ws);

	rs = loader_ops.get_reset_source();
	debug(L"Reset source = 0x%x\n", rs);
	if (rs == RESET_ERROR) {
		error(L"Reset source couldn't be retrieved. Falling back in TARGET_BOOT\n");
		return TARGET_BOOT;
	}

	if (rs != RESET_NOT_APPLICABLE)
		return target_from_reset(rs);

	return TARGET_UNKNOWN;
}

void display_splash(void)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return;
}

EFI_STATUS check_target(enum targets target)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return EFI_SUCCESS;
}

enum targets fallback_target(enum targets target)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return TARGET_BOOT;
}

EFI_STATUS start_boot_logic(void)
{
	EFI_STATUS ret;
	enum flow_types flow_type;
	enum targets target;

	ret = loader_ops.check_partition_table();
	if (EFI_ERROR(ret))
		goto error;

	flow_type = loader_ops.read_flow_type();

	target = target_from_inputs(flow_type);
	if (target == TARGET_UNKNOWN) {
		ret = EFI_LOAD_ERROR;
		goto error;
	}

	if (target == TARGET_COLD_OFF)
		loader_ops.do_cold_off();

	display_splash();

	while (check_target(target))
		target = fallback_target(target);

	ret = loader_ops.populate_indicators();
	if (EFI_ERROR(ret))
		goto error;

	debug(L"Booting target %a\n", target_strings[target]);

	ret = loader_ops.load_target(target);
	/* This code shouldn't be reached! */
	if (EFI_ERROR(ret))
		goto error;
error:
	return ret;
}
