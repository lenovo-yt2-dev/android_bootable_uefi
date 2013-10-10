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

#ifndef _OSNIB_V1_0_H_
#define _OSNIB_V1_0_H_

#include "osnib.h"

#define OSNIB_V1_0_OEM_RSVD_SIZE	32
#define OSNIB_V1_0_DEBUG_SIZE		15
#define OSNIB_V1_0_FW_RSVD_SIZE		3

struct osnib_v1_0 {
	struct osnib_header header;
	CHAR8 bootflow;
	struct {
		struct {
			CHAR8 kernel_watchdog:1;
			CHAR8 security_watchdog:1;
			CHAR8 pmc_watchdog:1;
			CHAR8 reserved:1;
			CHAR8 wdt_counter:4;
		} __attribute__ ((packed)) bf;
		CHAR8 wake_src;
		CHAR8 debug[OSNIB_V1_0_DEBUG_SIZE];
	} __attribute__ ((packed)) fw_to_os;

	CHAR8 firmware_reserved[OSNIB_V1_0_FW_RSVD_SIZE];

	struct {
		CHAR8 target_mode;
		struct {
			CHAR8 rtc_alarm_charger:1;
			CHAR8 fw_update:1;
			CHAR8 ramconsole:1;
			CHAR8 reserved:5;
		} bf;
	} __attribute__ ((packed)) os_to_fw;

	CHAR8 checksum;

	struct {
		CHAR8 reserved[OSNIB_V1_0_OEM_RSVD_SIZE];
	} __attribute__ ((packed)) oem;
} __attribute__ ((packed));

#endif /* _OSNIB_V1_0_H_ */
