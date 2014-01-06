/*
 * Copyright (c) 2014, Intel Corporation
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

#include "efilinux.h"
#include "efistdarg.h"
#include "platform.h"
#include "log.h"

static CHAR16 buffer[LOG_BUF_SIZE / sizeof(CHAR16)];
static CHAR16 *cur = buffer;

void log(UINTN level, const CHAR16 *prefix, const void *func, const INTN line,
	 const CHAR16* fmt, ...)
{
	UINT64 time = loader_ops.get_current_time_us();
	UINT64 sec = time / 1000000;
	UINT64 usec = time - (sec * 1000000);
	CHAR16 *start = cur;
	va_list args;

	va_start(args, fmt);

	cur += SPrint(cur, sizeof(buffer) - (cur - buffer), L"[%5ld.%06ld] ",
		      sec, usec);
	cur += SPrint(cur, sizeof(buffer) - (cur - buffer), (CHAR16 *)prefix,
		      func, line);
	cur += VSPrint(cur, sizeof(buffer) - (cur - buffer), (CHAR16 *)fmt,
		       args);
	if (((cur - buffer) + LOG_LINE_LEN) * sizeof(CHAR16) >= LOG_BUF_SIZE)
		cur = buffer;

	va_end (args);

	if (log_level >= level)
		Print(L"EFILINUX %s", start);
}

void log_save_to_variable()
{
	if (log_flush_to_variable) {
		EFI_STATUS status = LibSetVariable(L"EfilinuxLogs", &osloader_guid,
						   (cur - buffer) * sizeof(CHAR16),
						   buffer);
		if (EFI_ERROR(status))
			error(L"Save log into EFI variable failed\n");
	}
}
