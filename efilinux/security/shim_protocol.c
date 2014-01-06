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
#include <utils.h>
#include <stdlib.h>
#include <shim.h>
#include "shim_protocol.h"

EFI_GUID gShimLockProtocolGuid = SHIM_LOCK_GUID;

EFI_STATUS shim_blob_verify(IN VOID *blob, IN UINTN blob_size,
		IN VOID *sig, IN UINTN sig_size) {

	SHIM_LOCK *shim_lock;
	EFI_STATUS ret;
	ret = LibLocateProtocol(&gShimLockProtocolGuid, (VOID **)&shim_lock);
	if (EFI_ERROR(ret)) {
		error(L"Couldn't instantiate shim protocol", ret);
		return ret;
	}

	if (!sigsize) {
		error(L"Secure boot enabled, "
		      "but Android boot image is unsigned\n");
		return EFI_ACCESS_DENIED;
	}

	ret = uefi_call_wrapper(VerifyBlob, 4, shim_lock,
			bootimage, blob_size,
			sig_offset, sig_size);
	if (EFI_ERROR(ret))
		error(L"Boot image verification failed", ret);
	return ret;
}
