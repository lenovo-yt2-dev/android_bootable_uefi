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
 *
 */

#include <efi.h>
#include <efilib.h>
#include "efilinux.h"
#include "bootlogic.h"
#include "acpi.h"
#include "em.h"

#define DEVICE_INFO_PROTOCOL {0xE4F3260B, 0xD35F, 0x4AF1, {0xB9, 0x0E, 0x91, 0x0F, 0x5A, 0xD2, 0xE3, 0x26}}
static EFI_GUID DeviceInfoProtocolGuid = DEVICE_INFO_PROTOCOL;

#define USB_CHARGER_SDP (1 << 0);
#define USB_CHARGER_DCP (1 << 1);
#define USB_CHARGER_CDP (1 << 2);
#define USB_CHARGER_ACA (1 << 3);

typedef UINT8 USB_CHARGER_TYPE;
typedef UINT8 BATT_CAPACITY;
typedef UINT16 BATT_VOLTAGE;

typedef EFI_STATUS (EFIAPI *GET_BATTERY_STATUS) (
	OUT BOOLEAN *BatteryPresent,
	OUT BOOLEAN *BatteryValid,
	OUT BOOLEAN *CapacityReadable,
	OUT BATT_VOLTAGE *BatteryVoltageLevel,
	OUT BATT_CAPACITY *BatteryCapacityLevel);

typedef EFI_STATUS (EFIAPI *GET_ACDC_CHARGER_STATUS) (
	OUT BOOLEAN *ACDCChargerPresent);

typedef EFI_STATUS (EFIAPI *GET_USB_CHARGER_STATUS) (
	OUT BOOLEAN *UsbChargerPresent,
	OUT USB_CHARGER_TYPE *UsbChargerType);

struct _DEVICE_INFO_PROTOCOL {
	UINT32 Revision;
	GET_BATTERY_STATUS GetBatteryStatus;
	GET_ACDC_CHARGER_STATUS GetAcDcChargerStatus;
	GET_USB_CHARGER_STATUS GetUsbChargerStatus;
};

struct battery_status {
	BOOLEAN BatteryPresent;
	BOOLEAN BatteryValid;
	BOOLEAN CapacityReadable;
	BATT_VOLTAGE BatteryVoltageLevel;
	BATT_CAPACITY BatteryCapacityLevel;
};

static EFI_STATUS uefi_get_battery_status(struct battery_status *status)
{
	struct _DEVICE_INFO_PROTOCOL *dev_info;
	EFI_STATUS ret;

	ret = LibLocateProtocol(&DeviceInfoProtocolGuid, (VOID **)&dev_info);
	if (EFI_ERROR(ret))
		goto error;

	ret = dev_info->GetBatteryStatus(
		&status->BatteryPresent,
		&status->BatteryValid,
		&status->CapacityReadable,
		&status->BatteryVoltageLevel,
		&status->BatteryCapacityLevel);
	if (EFI_ERROR(ret))
		goto error;
	return ret;

error:
	error(L"Failed to get battery status: %r\n", ret);
	return ret;
}

static enum batt_levels uefi_get_battery_level(void)
{
	struct battery_status status;
	EFI_STATUS ret;
	UINTN value, threshold;

	ret = uefi_get_battery_status(&status);
	if (EFI_ERROR(ret))
		goto error;

	if (em1_get_cap_or_volt() == EM_1_USE_IA_APPS_CAP) {
		value = status.BatteryCapacityLevel;
		threshold = em1_get_ia_apps_cap();
		debug(L"Battery: %d%% Threshold: %d%%\n", value, threshold);
	}
	else {
		value = status.BatteryVoltageLevel;
		threshold = em1_get_ia_apps_run();
		debug(L"Battery: %dmA Threshold: %dmA\n", value, threshold);
	}

	return value > threshold ? BATT_BOOT_OS : BATT_BOOT_CHARGING;

error:
	error(L"Failed to get battery level: %r\n", ret);
	return BATT_ERROR;
}

static int uefi_is_battery_ok(void)
{
	struct battery_status status;
	EFI_STATUS ret;

	ret = uefi_get_battery_status(&status);
	if (EFI_ERROR(ret))
		goto error;

	if (!status.BatteryPresent)
		return 0;
	return status.BatteryValid || em1_get_boot_on_invalid_batt();

error:
	error(L"Failed to get battery status: %r\n", ret);
	return em1_get_boot_on_invalid_batt();
}

static void uefi_print_battery_infos(void)
{
	struct battery_status status;
	EFI_STATUS ret;

	ret = uefi_get_battery_status(&status);
	if (EFI_ERROR(ret))
		goto error;

	Print(L"BatteryPresent = 0x%x\n", status.BatteryPresent);
	Print(L"BatteryValid = 0x%x\n", status.BatteryValid);
	Print(L"CapacityReadable = 0x%x\n", status.CapacityReadable);
	Print(L"BatteryVoltageLevel = 0x%x\n", status.BatteryVoltageLevel);
	Print(L"BatteryCapacityLevel = 0x%x\n", status.BatteryCapacityLevel);

	Print(L"IA_APPS_RUN = 0x%x\n", em1_get_ia_apps_run());
	Print(L"IA_APPS_CAP = 0x%x\n", em1_get_ia_apps_cap());
	Print(L"CAP_OR_VOLT = 0x%x\n", em1_get_cap_or_volt());
	Print(L"BOOT_ON_INVALID_BATT = 0x%x\n", em1_get_boot_on_invalid_batt());
error:
	if (EFI_ERROR(ret))
		error(L"Failed to get battery status: %r\n", ret);
}

struct energy_mgmt_ops uefi_em_ops = {
	.get_battery_level = uefi_get_battery_level,
	.is_battery_ok = uefi_is_battery_ok,
	.print_battery_infos = uefi_print_battery_infos
};
