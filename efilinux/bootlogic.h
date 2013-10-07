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

#ifndef _BOOTLOGIC_H_
#define _BOOTLOGIC_H_

/** RSCI Definitions **/

/* Wake sources */
#define WAKE_NOT_APPLICABLE		0x00
#define WAKE_BATTERY_INSERTED		0x01
#define WAKE_USB_CHARGER_INSERTED	0x02
#define WAKE_ACDC_CHARGER_INSERTED	0x03
#define WAKE_POWER_BUTTON_PRESSED	0x04
#define WAKE_RTC_TIMER			0x05

/* Reset sources */
#define RESET_NOT_APPLICABLE		0x00
#define RESET_OS_INITIATED		0x01
#define RESET_FW_UPDATE			0x03
#define RESET_KERNEL_WATCHDOG		0x04
#define RESET_SECURITY_WATCHDOG		0x05
#define RESET_SECURITY_INITIATED	0x06

/* Reset type */
#define NOT_APPLICABLE  0x00
#define WARM_RESET	0x01
#define COLD_RESET	0x02
#define GLOBAL_RESET	0x07

/* Shutdown sources */
#define SHTDWN_NOT_APPLICABLE		0x00
#define SHTDWN_POWER_BUTTON_OVERRIDE	0x01
#define SHTDWN_BATTERY_REMOVAL		0x02
#define SHTDWN_VCRIT			0x03
#define SHTDWN_THERMTRIP		0x04
#define SHTDWN_PMICTEMP			0x05
#define SHTDWN_SYSTEMP			0x06
#define SHTDWN_BATTEMP			0x07
#define SHTDWN_SYSUVP			0x08
#define SHTDWN_SYSOVP			0x09
#define SHTDWN_PMC_WATCHDOG		0x0C

/* Indicators */
#define FBR_NONE			0x00
#define FBR_WATCHDOG_COUNTER_EXCEEDED	0x01
#define FBR_NO_MATCHING_OS		0x02
#define FASTBOOT_BUTTONS_COMBO

enum targets {
	TARGET_BOOT,
	TARGET_RECOVERY,
	TARGET_FASTBOOT,
	TARGET_TEST,
	TARGET_COLD_OFF,
	TARGET_UNKNOWN,
};
extern char *target_strings[];

enum flow_types {
	FLOW_NORMAL,
	FLOW_3GPP,
	FLOW_UNKNOWN,
};

EFI_STATUS start_boot_logic(void);

#endif /* _BOOTLOGIC_H_ */
