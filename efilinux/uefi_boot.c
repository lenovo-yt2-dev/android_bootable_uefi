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
#include "bootlogic.h"
#include "protocol.h"
#include "uefi_utils.h"
#include "splash.h"

#define ANDROID_PREFIX		0x0100
#define BOOT_OPTION_NAME_LENGTH 10
#define BOOT_ORDER_NAME		L"BootOrder"
#define BOOT_CURRENT_NAME	L"BootCurrent"
#define BOOT_NEXT_NAME		L"BootNext"

struct android_boot_option {
	UINT16 id;
	CHAR16 *name;
} android_boot_options [] = {
	{ANDROID_PREFIX + TARGET_BOOT, L"Android Main OS"},
	{ANDROID_PREFIX + TARGET_RECOVERY, L"Android Recovery OS"},
	{ANDROID_PREFIX + TARGET_FASTBOOT, L"Android Droidboot OS"},
	{ANDROID_PREFIX + TARGET_TEST, L"Android Test OS"},
	{ANDROID_PREFIX + TARGET_CHARGING, L"Android Charging OS"},
};

static EFI_STATUS update_boot_order(void)
{
	EFI_STATUS ret;
	UINTN size;
	UINTN append_size;
	UINT16 *new_boot_order;
	UINT16 *current_boot_order = LibGetVariableAndSize(BOOT_ORDER_NAME, &EfiGlobalVariable, &size);
	if (!current_boot_order) {
		error(L"Failed to get BootOrder variable\n");
		ret = EFI_LOAD_ERROR;
		goto out;
	}

	append_size = sizeof(android_boot_options) / sizeof(*android_boot_options);
	UINT16 *tmp = malloc(append_size * sizeof(UINT16));
	if (!tmp) {
		ret = EFI_OUT_OF_RESOURCES;
		error(L"Failed to allocate bootorder buffer: %r\n", ret);
		goto free_current_boot_order;
	}

	/* Remove duplicates */
	UINTN i, j, index = 0;
	int dup;
	for (i = 0; i < append_size; i++) {
		dup = 0;
		for (j = 0; j < size / sizeof(UINT16); j++)
			if (android_boot_options[i].id == current_boot_order[j]) {
				dup = 1;
				break;
			}
		if (!dup)
			tmp[index++] = android_boot_options[i].id;
	}

	append_size = index * sizeof(UINT16);
	new_boot_order = malloc(size + append_size);
	memcpy((CHAR8 *)new_boot_order, (CHAR8 *)current_boot_order, size);
	memcpy((CHAR8 *)new_boot_order + size, (CHAR8 *)tmp, append_size);
	free(tmp);

	ret = LibSetNVVariable(L"BootOrder", &EfiGlobalVariable, size + append_size, new_boot_order);
	if (EFI_ERROR(ret))
		error(L"Failed to set Boot Order: %r\n", ret);

free_current_boot_order:
	free(current_boot_order);
out:
	return ret;
}

static void *create_load_option(UINTN size, CHAR16 *desc, EFI_DEVICE_PATH *device, UINTN device_length)
{
	CHAR8 *index;
	struct EFI_LOAD_OPTION *load_option = malloc(size);
	if (!load_option) {
		error(L"Failed to allocate load option: %r\n", EFI_OUT_OF_RESOURCES);
		return NULL;
	}

	load_option->Attributes = LOAD_OPTION_ACTIVE;
	load_option->FilePathListLength = device_length;

	index = (CHAR8 *)load_option + sizeof(*load_option);
	memcpy(index, (CHAR8 *)desc, StrSize(desc));
	index += StrSize(desc);
	memcpy(index, (CHAR8 *)device, device_length);

	return load_option;
}

static EFI_STATUS create_android_boot_options(EFI_DEVICE_PATH *device_path)
{
	EFI_STATUS ret;
	UINTN i;
	CHAR16 name[BOOT_OPTION_NAME_LENGTH];
	CHAR16 *description;
	UINTN load_option_size;
	void *load_option;
	UINTN device_path_length = DevicePathSize(device_path);

	for (i = 0; i < sizeof(android_boot_options) / sizeof(*android_boot_options); i++) {
		description = android_boot_options[i].name;
		SPrint(name, BOOT_OPTION_NAME_LENGTH * sizeof(CHAR16), L"Boot%4.0x", android_boot_options[i].id);
		debug(L"Create load option %s: %s\n", name, description);
		load_option_size = sizeof(struct EFI_LOAD_OPTION) + StrSize(description) + device_path_length;

		load_option = create_load_option(load_option_size, description, device_path, device_path_length);
		if (!load_option) {
			error(L"Can't create boot option\n");
			ret = EFI_OUT_OF_RESOURCES;
			goto out;
		}

		ret = LibSetNVVariable(name, &EfiGlobalVariable, load_option_size, load_option);
		if (EFI_ERROR(ret)) {
			error(L"Failed to set Boot option: %r\n", ret);
			free(load_option);
			goto out;
		}
		free(load_option);
	}
out:
	return ret;
}

static EFI_STATUS get_esp_handle(EFI_HANDLE **esp)
{
	EFI_STATUS ret;
	UINTN no_handles;
	EFI_HANDLE *handles;
	CHAR16 *description;
	EFI_DEVICE_PATH *device;

	ret = find_device_partition(&EfiPartTypeSystemPartitionGuid, &handles, &no_handles);
	if (EFI_ERROR(ret)) {
		error(L"Failed to found partition: %r\n", ret);
		goto out;
	}

	if (LOGLEVEL(DEBUG)) {
		UINTN i;
		debug(L"Found %d devices\n", no_handles);
		for (i = 0; i < no_handles; i++) {
			device = DevicePathFromHandle(handles[i]);
			description = DevicePathToStr(device);
			debug(L"Device : %s\n", description);
			free_pool(description);
		}
	}

	if (no_handles == 0) {
		error(L"Can't find loader partition!\n");
		ret = EFI_NOT_FOUND;
		goto out;
	}
	if (no_handles > 1) {
		error(L"Multiple loader partition found!\n");
		goto free_handles;
	}
	*esp = handles[0];
	return EFI_SUCCESS;

free_handles:
	if (handles)
		free(handles);
out:
	return ret;
}

static EFI_STATUS get_osloader_device_path(EFI_DEVICE_PATH **device)
{
	EFI_STATUS ret = EFI_SUCCESS;
	CHAR16 *path = OSLOADER_FILE_PATH;
	EFI_HANDLE *esp;
	CHAR16 *description;


	ret = get_esp_handle(&esp);
	if (EFI_ERROR(ret)) {
		error(L"Failed to get ESP partition: %r\n", ret);
		goto out;
	}

	path_to_dos(path);
	*device = FileDevicePath(esp, path);
	if (!*device) {
		error(L"Failed to build osloader filepath\n");
		ret = EFI_NOT_FOUND;
		goto out;
	}

	if (LOGLEVEL(DEBUG)) {
		description = DevicePathToStr(*device);
		debug(L"Os Loader path: %s\n", description);
		free_pool(description);
	}
out:
	if (esp)
		free(esp);
	return ret;
}

int boot_order_contains_android(void)
{
	UINT16 *boot_order;
	UINTN size;
	UINTN i;
	boot_order = LibGetVariableAndSize(BOOT_ORDER_NAME, &EfiGlobalVariable, &size);
	for (i = 0; i < size / sizeof(*boot_order); i++) {
		if (boot_order[i] & ANDROID_PREFIX)
			return 1;
	}
	return 0;
}

enum targets uefi_boot_current(void)
{
	UINT16 *current;
	current = LibGetVariable(BOOT_CURRENT_NAME, &EfiGlobalVariable);
	if (!current) {
		error(L"Failed to get BootCurrent\n");
		return TARGET_ERROR;
	}
	debug(L"current=0x%x\n", *current);
	if (*current & ANDROID_PREFIX)
		return *current - ANDROID_PREFIX;
	else
		return TARGET_UNKNOWN;
}

EFI_STATUS uefi_set_boot_next(enum targets target)
{
	target += ANDROID_PREFIX;
	debug(L"Setting boot next to 0x%x\n", target);
	return LibSetNVVariable(BOOT_NEXT_NAME, &EfiGlobalVariable, sizeof(UINT16), &target);
}

EFI_STATUS uefi_update_boot(void)
{
	EFI_STATUS ret;
	EFI_DEVICE_PATH *device;

	ret = get_osloader_device_path(&device);
	if (EFI_ERROR(ret)) {
		error(L"Failed to retrieve osloader device path\n", ret);
		goto out;
	}

	ret = create_android_boot_options(device);
	if (EFI_ERROR(ret)) {
		error(L"Failed to create android boot options\n", ret);
		goto free_device;
	}

	ret = update_boot_order();

free_device:
	free(device);
out:
	return ret;
}

EFI_STATUS uefi_init_boot_options(void)
{
	if (boot_order_contains_android())
		return EFI_ALREADY_STARTED;

	return uefi_update_boot();
}

EFI_STATUS uefi_display_splash(void)
{
	EFI_STATUS ret;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
	UINTN blt_size;
	UINTN height;
	UINTN width;

	Blt = NULL;
	ret = ConvertBmpToGopBlt(splash_bmp, splash_bmp_size, (void **)&Blt, &blt_size, &height, &width);
 	if (EFI_ERROR(ret)) {
		error(L"Failed to convert bmp to blt: %r\n", ret);
		goto error;
	}

	ret = gop_display_blt(Blt, blt_size, height, width);
 	if (EFI_ERROR(ret)) {
		error(L"Failed to display blt: %r\n", ret);
		goto error;
	}

error:
	if (EFI_ERROR(ret))
		error(L"Failed to display splash:%r\n", ret);
	return ret;
}
