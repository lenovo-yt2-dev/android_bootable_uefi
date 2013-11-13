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

#ifndef _OS_VERIFICATION_H
#define _OS_VERIFICATION_H

#include <efi.h>

#define MANIFEST_SIZE 1024

typedef struct _OS_VERIFICATION_PROTOCOL OS_VERIFICATION_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_OS_VERIFY) (
  IN OS_VERIFICATION_PROTOCOL *This,
  IN VOID                     *OsImagePtr,
  IN UINTN                    OsImageSize,
  IN VOID                     *ManifestPtr,
  IN UINTN                    ManifestSize
  );

typedef
EFI_STATUS
(EFIAPI *GET_SECURITY_POLICY) (
  IN     OS_VERIFICATION_PROTOCOL *This,
  IN OUT BOOLEAN                  *AllowUnsignedOS
);

struct _OS_VERIFICATION_PROTOCOL {
  EFI_OS_VERIFY           VerifiyOsImage;
  GET_SECURITY_POLICY     GetSecurityPolicy;
};

// {DAFB7EEC-B2E9-4ea6-A80A-37FED7909FF3}
#define INTEL_OS_VERIFICATION_PROTOCOL_GUID				\
	{								\
		0xdafb7eec, 0xb2e9, 0x4ea6,				\
		{ 0xa8, 0xa, 0x37, 0xfe, 0xd7, 0x90, 0x9f, 0xf3 }	\
	}

extern EFI_GUID gOsVerificationProtocolGuid;

EFI_STATUS intel_os_verify(IN VOID *os, IN UINTN os_size,
		IN VOID *manifest, IN UINTN manifest_size);

#endif
