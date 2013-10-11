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
#include "osnib_v2_0.h"
#include "bootlogic.h"
#include "platform/platform.h"

#define OSNIB_MAGIC		"NIB"

enum osnib_command {
	OSNIB_SIZE,
	OSNIB_POPULATE_GENERIC,
	OSNIB_POPULATE_PLATFORM,
};

static struct osnib_generic osnib_gen;

static int osnib_command(UINT16 version, VOID *osnib, enum osnib_command command)
{
	if (osnib) {
		struct osnib_header *h = osnib;
		version = h->version_minor | (h->version_major << 8);
	}

	debug(L"OSNIB version is 0x%x\n", version);

	int size;
	switch(version) {
	case 0x0200:
	{
		struct osnib_v2_0 *nib = (struct osnib_v2_0 *)osnib;
		size = sizeof(*nib);
		switch(command) {
		case OSNIB_SIZE:
			goto out;
		case OSNIB_POPULATE_GENERIC:
			osnib_gen.wdt_counter = nib->wdt_counter;
			osnib_gen.target_mode = nib->target_mode;
			osnib_gen.rtc_alarm_charging = nib->rtc_alarm_charging;
			break;
		case OSNIB_POPULATE_PLATFORM:
			nib->wdt_counter = osnib_gen.wdt_counter;
			nib->target_mode = osnib_gen.target_mode;
			nib->rtc_alarm_charging = osnib_gen.rtc_alarm_charging;
			break;
		default:
			error(L"Unknown OSNIB command 0x%x\n", command);
		}
		break;
	}
	default:
		error(L"Unknown OSNIB version 0x%x\n", version);
		return -1;
	}
out:
	return size;
}

static EFI_STATUS init_osnib(VOID **buffer, UINT16 version)
{
	int size = osnib_command(version, NULL, OSNIB_SIZE);
	if (size == -1)
		return EFI_INVALID_PARAMETER;

	*buffer = malloc(size);
	if (!*buffer)
		return EFI_OUT_OF_RESOURCES;

	memset((CHAR8 *)*buffer, 0, size);

	struct osnib_header *header;
	header = *buffer;
	memcpy(header->magic, (CHAR8 *)OSNIB_MAGIC, sizeof(OSNIB_MAGIC));
	header->version_major = (version & 0xff00) >> 8;
	header->version_minor = version & 0x00ff;

	return EFI_SUCCESS;
}

static EFI_STATUS populate_osnib_generic(VOID *osnib)
{
	memcpy((CHAR8 *)&osnib_gen, osnib, sizeof(struct osnib_header));

	return osnib_command(0, osnib, OSNIB_POPULATE_GENERIC) == -1 ?
		EFI_INVALID_PARAMETER : EFI_SUCCESS;
}

EFI_STATUS get_platform_osnib(void **o)
{
	EFI_STATUS ret = EFI_SUCCESS;

	*o = loader_ops.get_osnib();

	if (!*o) {
		error(L"Failed to retrieve OSNIB. Creating it.\n");

		UINT16 version = loader_ops.get_osnib_default_version();
		ret = init_osnib(o, version);
		if (EFI_ERROR(ret)) {
			error(L"Failed to create OSNIB\n");
			return ret;
		}
	}
	return ret;
}

static EFI_STATUS get_or_create_osnib(void)
{
	EFI_STATUS ret;
	void *o;
	ret = get_platform_osnib(&o);
	if (EFI_ERROR(ret))
		goto out;

	ret = populate_osnib_generic(o);
	if (EFI_ERROR(ret))
		error(L"Failed to populate OSNIB: %r\n", ret);
	free(o);
out:
	return ret;
}

static UINT64 _get_osnib_field(UINTN offset)
{
	if (!osnib_gen.header.magic[0])
		if (EFI_ERROR(get_or_create_osnib()))
			return -1;

	return *((CHAR8 *)&osnib_gen + offset);
}

#define get_osnib_field(field)				\
	_get_osnib_field(offsetof(struct osnib_generic, field))

enum targets osnib_get_target_mode(void)
{
	return get_osnib_field(target_mode);
}

int osnib_get_wdt_counter(void)
{
	return get_osnib_field(wdt_counter);
}

int osnib_get_rtc_alarm_charging(void)
{
	return get_osnib_field(rtc_alarm_charging);
}

EFI_STATUS osnib_write(void)
{
	VOID *o;
	EFI_STATUS ret;

	ret = get_platform_osnib(&o);
	if (EFI_ERROR(ret)) {
		error("Failed to write OSNIB: %r\n", ret);
		goto out;
	}

	int size;
	size = osnib_command(0, o, OSNIB_POPULATE_PLATFORM);
	loader_ops.set_osnib(o, size);
out:
	return ret;
}
