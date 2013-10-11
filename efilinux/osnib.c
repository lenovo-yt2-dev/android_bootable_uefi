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
#include "osnib.h"
#include "osnib_v1_0.h"
#include "bootlogic.h"

#define OSNIB_MAGIC		"NIB"

EFI_STATUS init_osnib(VOID *buffer, CHAR8 version_major, CHAR8 version_minor)
{
	EFI_STATUS ret = EFI_SUCCESS;

	switch(version_major) {
	case 1:
	{
		struct osnib_header *header = buffer;
		memset((CHAR8 *)header, 0, sizeof(struct OSNIB_V1_0));
		memcpy(header->magic, (CHAR8 *)OSNIB_MAGIC, sizeof(OSNIB_MAGIC));
		header->version_major = version_major;
		header->version_minor = version_minor;
		break;
	}
	default:
		debug(L"Unknown osnib major version: 0x%x\n", version_major);
		ret = EFI_INVALID_PARAMETER;
	}

	return ret;
}

enum targets osnib_get_target_mode(void)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return TARGET_BOOT;
}
