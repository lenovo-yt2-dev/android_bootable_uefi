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

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <efi.h>
#include "bootlogic.h"

struct osloader_ops {
	EFI_STATUS (*check_partition_table)(void);
	enum flow_types (*read_flow_type)(void);
	void (*do_cold_off)(void);
	EFI_STATUS (*populate_indicators)(void);
	EFI_STATUS (*load_target)(enum targets);
	enum wake_sources (*get_wake_source)(void);
	enum reset_sources (*get_reset_source)(void);
	enum shutdown_sources (*get_shutdown_source)(void);
	int (*is_osnib_corrupted)(void);
	enum batt_levels (*get_battery_level)(void);
	int (*is_battery_ok)(void);
	int (*combo_key)(enum combo_keys);
	EFI_STATUS (*set_target_mode)(enum targets);
	EFI_STATUS (*set_rtc_alarm_charging)(int);
	EFI_STATUS (*set_wdt_counter)(int);
	enum targets (*get_target_mode)(void);
	int (*get_rtc_alarm_charging)(void);
	int (*get_wdt_counter)(void);
	void (*hook_bootlogic_begin)(void);
	EFI_STATUS (*update_boot)(void);
	EFI_STATUS (*display_splash)(void);
	void (*print_battery_infos)(void);
};

extern struct osloader_ops loader_ops;

EFI_STATUS init_platform_functions(void);

#endif /* _PLATFORM_H_ */
