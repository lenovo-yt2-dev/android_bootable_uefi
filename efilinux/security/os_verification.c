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
#include <stdlib.h>
#include <uefi_utils.h>
#include <log.h>
#include "os_verification.h"

EFI_GUID gOsVerificationProtocolGuid = INTEL_OS_VERIFICATION_PROTOCOL_GUID;

EFI_STATUS intel_os_verify(IN VOID *os, IN UINTN os_size,
		IN VOID *manifest, IN UINTN manifest_size)
{
	OS_VERIFICATION_PROTOCOL *ovp;
	EFI_STATUS ret;

	ret = LibLocateProtocol(&gOsVerificationProtocolGuid, (void **)&ovp);
	if (EFI_ERROR(ret) || !ovp) {
		error(L"%x failure\n", __func__);
		goto out;
	}

	ret = uefi_call_wrapper(ovp->VerifiyOsImage, 5, ovp,
			os, os_size,
			manifest, manifest_size);

out:
	return ret;
}

BOOLEAN is_secure_boot_enabled(void)
{
	OS_VERIFICATION_PROTOCOL *ovp;
	EFI_STATUS ret;
	BOOLEAN unsigned_allowed;

	ret = LibLocateProtocol(&gOsVerificationProtocolGuid, (void **)&ovp);
	if (EFI_ERROR(ret) || !ovp) {
		error(L"%x failure\n", __func__);
		goto out;
	}

	ret = uefi_call_wrapper(ovp->GetSecurityPolicy, 2, ovp,
			&unsigned_allowed);
	if (EFI_ERROR(ret))
		goto out;

	debug(L"unsigned_allowed = %x\n", unsigned_allowed);
	return (unsigned_allowed == FALSE);
out:
	return TRUE;
}

