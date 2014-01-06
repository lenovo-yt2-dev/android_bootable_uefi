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

#ifndef __LOG_H__
#define __LOG_H__

#define LEVEL_DEBUG		4
#define LEVEL_INFO		3
#define LEVEL_WARNING		2
#define LEVEL_ERROR		1

#include "config.h"

#define LOGLEVEL(level)	(log_level >= LEVEL_##level)

void log(UINTN level, const CHAR16 *prefix, const void *func, const INTN line,
	 const CHAR16* fmt, ...);

void log_save_to_variable(void);

#define debug(...) { \
		log(LEVEL_DEBUG, L"DEBUG [%a:%d] ", \
		    __func__, __LINE__, __VA_ARGS__); \
	}

#define info(...) { \
		log(LEVEL_INFO, L"INFO [%a:%d] ", \
		    __func__, __LINE__, __VA_ARGS__); \
	}

#define warning(...) { \
		log(LEVEL_WARNING, L"WARNING [%a:%d] ", \
		    __func__, __LINE__, __VA_ARGS__); \
	}

#define error(...) { \
		log(LEVEL_ERROR, L"ERROR [%a:%d] ", \
		    __func__, __LINE__, __VA_ARGS__); \
	}

#endif	/* __LOG_H__ */
