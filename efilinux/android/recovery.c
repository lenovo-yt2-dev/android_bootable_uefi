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

#include <recovery.h>

EFI_STATUS android_load_bcb(
                IN EFI_FILE *root_dir,
                IN const EFI_GUID *bcb_guid,
                OUT CHAR16 **var)
{
        EFI_STATUS ret, outval;
        struct bootloader_message bcb;
        BOOLEAN reset;
        EFI_RESET_TYPE reset_type;

        outval = EFI_NOT_FOUND;
        reset = FALSE;

        ret = read_bcb(bcb_guid, &bcb);
        if (EFI_ERROR(ret))
                return ret;

        /* We own the status field; clear it in case there is any stale data */
        bcb.status[0] = '\0';

        if (!strcmpa(bcb.command, (CHAR8 *)"recovery-firmware")) {
                reset = update_firmware(root_dir, &bcb, TRUE, &reset_type);
        } else if (!strcmpa(bcb.command, (CHAR8 *)"update-firmware")) {
                reset = update_firmware(root_dir, &bcb, FALSE, &reset_type);
        }

        if (!strncmpa(bcb.command, (CHAR8 *)"boot-", 5)) {
                *var = stra_to_str(bcb.command + 5);
                outval = EFI_SUCCESS;
                debug(L"BCB boot target: '%s'\n", *var);
        }

        ret = write_bcb(bcb_guid, &bcb);
        if (EFI_ERROR(ret)) {
                Print(L"Unable to update BCB contents!\n");
                return ret;
        }
        if (reset) {
                debug ("I am about to reset the system");
                uefi_call_wrapper(RT->ResetSystem, 4, reset_type,
                                EFI_SUCCESS, 0, NULL);
        }

        return outval;
}

EFI_STATUS write_bcb(
                IN const EFI_GUID *bcb_guid,
                IN struct bootloader_message *bcb)
{
        EFI_STATUS ret;
        EFI_BLOCK_IO *BlockIo;
        EFI_DISK_IO *DiskIo;
        UINT32 MediaId;

        debug(L"Locating BCB\n");
        ret = open_partition(bcb_guid, &MediaId, &BlockIo, &DiskIo);
        if (EFI_ERROR(ret))
                return EFI_INVALID_PARAMETER;

        debug(L"Writing BCB\n");
        ret = uefi_call_wrapper(DiskIo->WriteDisk, 5, DiskIo, MediaId, 0,
                        sizeof(*bcb), bcb);
        if (EFI_ERROR(ret)) {
                error(L"WriteDisk (bcb)", ret);
                return ret;
        }
        debug(L"BCB written successfully\n");

        return EFI_SUCCESS;
}


/* Convenience function to update fields in the BCB */
VOID set_bcb(
                IN struct bootloader_message *bcb,
                IN CHAR16 *command,
                IN CHAR16 *status,
                IN EFI_STATUS code)
{
        if (status) {
                if (code != EFI_SUCCESS) {
                        CHAR16 *tmp;
                        tmp = PoolPrint(L"%s: %r", status, code);
                        if (tmp) {
                                str_to_stra(bcb->status, tmp, sizeof(bcb->status));
                                FreePool(tmp);
                        }
                } else {
                        str_to_stra(bcb->status, status, sizeof(bcb->status));
                }
        }
        if (command)
                str_to_stra(bcb->command, command, sizeof(bcb->command));
}

EFI_STATUS read_bcb(
                IN const EFI_GUID *bcb_guid,
                OUT struct bootloader_message *bcb)
{
        EFI_STATUS ret;
        EFI_BLOCK_IO *BlockIo;
        EFI_DISK_IO *DiskIo;
        UINT32 MediaId;

        debug(L"Locating BCB\n");
        ret = open_partition(bcb_guid, &MediaId, &BlockIo, &DiskIo);
        if (EFI_ERROR(ret))
                return EFI_INVALID_PARAMETER;

        debug(L"Reading BCB\n");
        ret = uefi_call_wrapper(DiskIo->ReadDisk, 5, DiskIo, MediaId, 0,
                        sizeof(*bcb), bcb);
        if (EFI_ERROR(ret)) {
                error(L"ReadDisk (bcb)", ret);
                return ret;
        }
        bcb->command[31] = '\0';
        bcb->status[31] = '\0';
        debug(L"BCB read completed successfully\n");

        return EFI_SUCCESS;
}

