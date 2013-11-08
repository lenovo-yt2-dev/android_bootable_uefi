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
#include "platform.h"
#include "intel_partitions.h"
#include "acpi.h"
#include "uefi_osnib.h"
#include "uefi_keys.h"
#include "uefi_boot.h"
#include "uefi_em.h"

#if USE_INTEL_OS_VERIFICATION
#include "os_verification.h"
#endif

#if USE_SHIM
#include "shim_protocol.h"
#endif

static void x86_hook_bootlogic_begin()
{
	uefi_init_boot_options();
}

void x86_ops(struct osloader_ops *ops)
{
	ops->check_partition_table = check_gpt;
	ops->read_flow_type = acpi_read_flow_type;
	ops->do_cold_off = acpi_cold_off;
	ops->populate_indicators = rsci_populate_indicators;
	ops->load_target = intel_load_target;
	ops->get_wake_source = rsci_get_wake_source;
	ops->get_reset_source = rsci_get_reset_source;
	ops->get_reset_type = rsci_get_reset_type;
	ops->get_shutdown_source = rsci_get_shutdown_source;
	ops->combo_key = uefi_combo_key;
	ops->set_target_mode = uefi_set_target_mode;
	ops->set_rtc_alarm_charging = uefi_set_rtc_alarm_charging;
	ops->set_wdt_counter = uefi_set_wdt_counter;
	ops->get_target_mode = uefi_boot_current;
	ops->get_rtc_alarm_charging = uefi_get_rtc_alarm_charging;
	ops->get_wdt_counter = uefi_get_wdt_counter;
	ops->hook_bootlogic_begin = x86_hook_bootlogic_begin;
	ops->update_boot = uefi_update_boot;
	ops->display_splash = uefi_display_splash;
	ops->get_battery_level = uefi_get_battery_level;
	ops->is_battery_ok = uefi_is_battery_ok;
	ops->print_battery_infos = uefi_print_battery_infos;
#if USE_INTEL_OS_VERIFICATION
	ops->hash_verify = intel_os_verify;
#endif

#if USE_SHIM
	ops->hash_verify = shim_blob_verify;
#endif
	ops->get_extra_cmdline = uefi_get_extra_cmdline;

}
