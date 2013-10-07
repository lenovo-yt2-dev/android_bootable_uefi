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
	STRINGIZE(TARGET_UNKNOWN),
};

enum targets target_from_inputs(enum flow_types flow_type)
{
	debug(L"TO BE IMPLEMENTED\n");
	return TARGET_BOOT;
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
		ret = EFI_INVALID_PARAMETER;
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

	ret = loader_ops.load_target(target);
	/* This code shouldn't be reached! */
	if (EFI_ERROR(ret))
		goto error;
error:
	return ret;
}
