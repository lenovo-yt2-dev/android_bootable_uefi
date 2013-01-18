/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GUMMIBOOT_ANDROID_H

#include "efi.h"
#include "efilib.h"

EFI_STATUS android_image_start(
                IN EFI_HANDLE parent_image,
                IN const EFI_GUID *guid,
                IN CHAR16 *gummiboot_opts);

/* Load the next boot target if specified in the BCB partition,
 * which we specify by partition GUID. Place the value in var,
 * which must be freed.
 * Return values:
 * EFI_SUCCESS - A valid value was found in the BCB
 * EFI_NOT_FOUND - BCB didn't specify a target; boot normally
 * EFI_INVALID_PARAMETER - BCB wasn't readble, bad GUID? */
EFI_STATUS android_load_bcb(
                IN const EFI_GUID *bcb_guid,
                OUT CHAR16 **var);

/* Convert a string GUID read from a config file into a real GUID
 * that we can do things with */
EFI_STATUS string_to_guid(
                IN CHAR16 *guid_str,
                OUT EFI_GUID *guid);
#endif
