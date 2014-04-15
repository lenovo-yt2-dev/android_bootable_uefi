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
#include <fs.h>
#include "efilinux.h"
#include "protocol.h"

extern EFI_GUID GraphicsOutputProtocol;

typedef struct {
  UINT8   Blue;
  UINT8   Green;
  UINT8   Red;
  UINT8   Reserved;
} BMP_COLOR_MAP;

typedef struct {
  CHAR8         CharB;
  CHAR8         CharM;
  UINT32        Size;
  UINT16        Reserved[2];
  UINT32        ImageOffset;
  UINT32        HeaderSize;
  UINT32        PixelWidth;
  UINT32        PixelHeight;
  UINT16        Planes;       // Must be 1
  UINT16        BitPerPixel;  // 1, 4, 8, or 24
  UINT32        CompressionType;
  UINT32        ImageSize;    // Compressed image size in bytes
  UINT32        XPixelsPerMeter;
  UINT32        YPixelsPerMeter;
  UINT32        NumberOfColors;
  UINT32        ImportantColors;
} __attribute((packed)) BMP_IMAGE_HEADER;

EFI_STATUS ConvertBmpToGopBlt (VOID *BmpImage, UINTN BmpImageSize,
			       VOID **GopBlt, UINTN *GopBltSize,
			       UINTN *PixelHeight, UINTN *PixelWidth)
{
  UINT8                         *Image;
  UINT8                         *ImageHeader;
  BMP_IMAGE_HEADER              *BmpHeader;
  BMP_COLOR_MAP                 *BmpColorMap;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  UINT64                        BltBufferSize;
  UINTN                         Index;
  UINTN                         Height;
  UINTN                         Width;
  UINTN                         ImageIndex;
  UINT32                        DataSizePerLine;
  BOOLEAN                       IsAllocated;
  UINT32                        ColorMapNum;



  if (sizeof (BMP_IMAGE_HEADER) > BmpImageSize) {
    return EFI_INVALID_PARAMETER;
  }

  BmpHeader = (BMP_IMAGE_HEADER *) BmpImage;

  if (BmpHeader->CharB != 'B' || BmpHeader->CharM != 'M') {
    return EFI_UNSUPPORTED;
  }

  //
  // Doesn't support compress.
  //
  if (BmpHeader->CompressionType != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Only support BITMAPINFOHEADER format.
  // BITMAPFILEHEADER + BITMAPINFOHEADER = BMP_IMAGE_HEADER
  //
  if (BmpHeader->HeaderSize != sizeof (BMP_IMAGE_HEADER) - offsetof(BMP_IMAGE_HEADER, HeaderSize))
    return EFI_UNSUPPORTED;

  //
  // The data size in each line must be 4 byte alignment.
  //
  DataSizePerLine = ((BmpHeader->PixelWidth * BmpHeader->BitPerPixel + 31) >> 3) & (~0x3);
  BltBufferSize = MultU64x32 (DataSizePerLine, BmpHeader->PixelHeight);
  if (BltBufferSize > (UINT32) ~0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((BmpHeader->Size != BmpImageSize) ||
      (BmpHeader->Size < BmpHeader->ImageOffset) ||
      (BmpHeader->Size - BmpHeader->ImageOffset !=  BmpHeader->PixelHeight * DataSizePerLine)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate Color Map offset in the image.
  //
  Image       = BmpImage;
  BmpColorMap = (BMP_COLOR_MAP *) (Image + sizeof (BMP_IMAGE_HEADER));
  if (BmpHeader->ImageOffset < sizeof (BMP_IMAGE_HEADER)) {
    return EFI_INVALID_PARAMETER;
  }

  if (BmpHeader->ImageOffset > sizeof (BMP_IMAGE_HEADER)) {
    switch (BmpHeader->BitPerPixel) {
      case 1:
        ColorMapNum = 2;
        break;
      case 4:
        ColorMapNum = 16;
        break;
      case 8:
        ColorMapNum = 256;
        break;
      default:
        ColorMapNum = 0;
        break;
      }
    if (BmpHeader->ImageOffset - sizeof (BMP_IMAGE_HEADER) != sizeof (BMP_COLOR_MAP) * ColorMapNum) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Calculate graphics image data address in the image
  //
  Image         = ((UINT8 *) BmpImage) + BmpHeader->ImageOffset;
  ImageHeader   = Image;

  //
  // Calculate the BltBuffer needed size.
  //
  BltBufferSize = MultU64x32 ((UINT64) BmpHeader->PixelWidth, BmpHeader->PixelHeight);
  //
  // Ensure the BltBufferSize * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) doesn't overflow
  //
  if (BltBufferSize > DivU64x32 ((UINTN) ~0, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), NULL)) {
    return EFI_UNSUPPORTED;
  }
  BltBufferSize = MultU64x32 (BltBufferSize, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

  IsAllocated   = FALSE;
  if (*GopBlt == NULL) {
    //
    // GopBlt is not allocated by caller.
    //
    *GopBltSize = (UINTN) BltBufferSize;
    *GopBlt     = AllocatePool (*GopBltSize);
    IsAllocated = TRUE;
    if (*GopBlt == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    //
    // GopBlt has been allocated by caller.
    //
    if (*GopBltSize < (UINTN) BltBufferSize) {
      *GopBltSize = (UINTN) BltBufferSize;
      return EFI_BUFFER_TOO_SMALL;
    }
  }

  *PixelWidth   = BmpHeader->PixelWidth;
  *PixelHeight  = BmpHeader->PixelHeight;

  //
  // Convert image from BMP to Blt buffer format
  //
  BltBuffer = *GopBlt;
  for (Height = 0; Height < BmpHeader->PixelHeight; Height++) {
    Blt = &BltBuffer[(BmpHeader->PixelHeight - Height - 1) * BmpHeader->PixelWidth];
    for (Width = 0; Width < BmpHeader->PixelWidth; Width++, Image++, Blt++) {
      switch (BmpHeader->BitPerPixel) {
      case 1:
        //
        // Convert 1-bit (2 colors) BMP to 24-bit color
        //
        for (Index = 0; Index < 8 && Width < BmpHeader->PixelWidth; Index++) {
          Blt->Red    = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Red;
          Blt->Green  = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Green;
          Blt->Blue   = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Blue;
          Blt++;
          Width++;
        }

        Blt--;
        Width--;
        break;

      case 4:
        //
        // Convert 4-bit (16 colors) BMP Palette to 24-bit color
        //
        Index       = (*Image) >> 4;
        Blt->Red    = BmpColorMap[Index].Red;
        Blt->Green  = BmpColorMap[Index].Green;
        Blt->Blue   = BmpColorMap[Index].Blue;
        if (Width < (BmpHeader->PixelWidth - 1)) {
          Blt++;
          Width++;
          Index       = (*Image) & 0x0f;
          Blt->Red    = BmpColorMap[Index].Red;
          Blt->Green  = BmpColorMap[Index].Green;
          Blt->Blue   = BmpColorMap[Index].Blue;
        }
        break;

      case 8:
        //
        // Convert 8-bit (256 colors) BMP Palette to 24-bit color
        //
        Blt->Red    = BmpColorMap[*Image].Red;
        Blt->Green  = BmpColorMap[*Image].Green;
        Blt->Blue   = BmpColorMap[*Image].Blue;
        break;

      case 24:
        //
        // It is 24-bit BMP.
        //
        Blt->Blue   = *Image++;
        Blt->Green  = *Image++;
        Blt->Red    = *Image;
        break;

      default:
        //
        // Other bit format BMP is not supported.
        //
        if (IsAllocated) {
          FreePool (*GopBlt);
          *GopBlt = NULL;
        }
        return EFI_UNSUPPORTED;
        break;
      };

    }

    ImageIndex = (UINTN) (Image - ImageHeader);
    if ((ImageIndex % 4) != 0) {
      //
      // Bmp Image starts each row on a 32-bit boundary!
      //
      Image = Image + (4 - (ImageIndex % 4));
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS find_device_partition(const EFI_GUID *guid, EFI_HANDLE **handles, UINTN *no_handles)
{
	EFI_STATUS ret;
	*handles = NULL;

	ret = LibLocateHandleByDiskSignature(
		MBR_TYPE_EFI_PARTITION_TABLE_HEADER,
		SIGNATURE_TYPE_GUID,
		(void *)guid,
		no_handles,
		handles);
	if (EFI_ERROR(ret) || *no_handles == 0)
		error(L"Failed to found partition %g\n", guid);
	return ret;
}

EFI_STATUS get_esp_handle(EFI_HANDLE **esp)
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
			if (!device) {
				error(L"No device!\n");
				goto free_handles;
			}
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

EFI_STATUS get_esp_fs(EFI_FILE_IO_INTERFACE **esp_fs)
{
	EFI_STATUS ret = EFI_SUCCESS;
	EFI_GUID SimpleFileSystemProtocol = SIMPLE_FILE_SYSTEM_PROTOCOL;
	EFI_HANDLE *esp_handle;
	EFI_FILE_IO_INTERFACE *esp;

	ret = get_esp_handle(&esp_handle);
	if (EFI_ERROR(ret)) {
		error(L"Failed to get ESP partition: %r\n", ret);
		return ret;
	}

	ret = handle_protocol(esp_handle, &SimpleFileSystemProtocol,
			      (void **)&esp);
	if (EFI_ERROR(ret)) {
		error(L"HandleProtocol", ret);
		return ret;
	}

	*esp_fs = esp;

	return ret;
}

EFI_STATUS uefi_read_file(EFI_FILE_IO_INTERFACE *io, CHAR16 *filename, void **data, UINTN *size)
{
	EFI_STATUS ret;
	EFI_FILE_INFO *info;
	UINTN info_size;
	EFI_FILE *file;

	ret = uefi_call_wrapper(io->OpenVolume, 2, io, &file);
	if (EFI_ERROR(ret))
		goto out;

	ret = uefi_call_wrapper(file->Open, 5, file, &file, filename, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(ret))
		goto out;

	info_size = SIZE_OF_EFI_FILE_INFO + 200;

	info = malloc(info_size);
	if (!info)
		goto close;

	ret = uefi_call_wrapper(file->GetInfo, 4, file, &GenericFileInfo, &info_size, info);
	if (EFI_ERROR(ret))
		goto free_info;

	*size = info->FileSize;
	*data = malloc(*size);

retry:
	ret = uefi_call_wrapper(file->Read, 3, file, size, *data);
	if (ret == EFI_BUFFER_TOO_SMALL) {
		free(*data);
		*data = malloc(*size);
		goto retry;
	}

	if (EFI_ERROR(ret))
		free(*data);

free_info:
	free(info);
close:
	uefi_call_wrapper(file->Close, 1, file);
out:
	if (EFI_ERROR(ret))
		error(L"Failed to read file %s:%r\n", filename, ret);
	return ret;
}

EFI_STATUS gop_display_blt(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt, UINTN blt_size, UINTN height, UINTN width)
{
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL pix = {0x00, 0x00, 0x00, 0x00};
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
	UINTN hres, vres = 0;
	UINTN posx, posy = 0;
	EFI_STATUS ret;

	ret = LibLocateProtocol(&GraphicsOutputProtocol, (void **)&gop);
 	if (EFI_ERROR(ret) || !gop)
		goto out;

	hres = gop->Mode->Info->HorizontalResolution;
	vres = gop->Mode->Info->VerticalResolution;
	posx = (hres/2) - (width/2);
	posy = (vres/2) - (height/2);

	ret = uefi_call_wrapper(gop->Blt, 10, gop, &pix, EfiBltVideoFill, 0, 0, 0, 0, hres, vres, 0);
 	if (EFI_ERROR(ret))
		goto out;

	ret = uefi_call_wrapper(gop->Blt, 10, gop, Blt, EfiBltBufferToVideo, 0, 0, posx, posy, width, height, 0);

out:
	if (EFI_ERROR(ret))
	    error(L"Failed to display blt\n");
	return ret;
}

EFI_STATUS uefi_write_file(EFI_FILE_IO_INTERFACE *io, CHAR16 *filename, void *data, UINTN *size)
{
	EFI_STATUS ret;
	EFI_FILE *file, *root;

	ret = uefi_call_wrapper(io->OpenVolume, 2, io, &root);
	if (EFI_ERROR(ret))
		goto out;

	ret = uefi_call_wrapper(root->Open, 5, root, &file, filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
	if (EFI_ERROR(ret))
		goto out;

	ret = uefi_call_wrapper(file->Write, 3, file, size, data);
	uefi_call_wrapper(file->Close, 1, file);

out:
	if (EFI_ERROR(ret))
		error(L"Failed to write file %s:%r\n", filename, ret);
	return ret;
}

void uefi_reset_system(EFI_RESET_TYPE reset_type)
{
	uefi_call_wrapper(RT->ResetSystem, 4, reset_type,
			  EFI_SUCCESS, 0, NULL);
}

void uefi_shutdown(void)
{
	uefi_reset_system(EfiResetShutdown);
}

EFI_STATUS uefi_delete_file(EFI_FILE_IO_INTERFACE *io, CHAR16 *filename)
{
	EFI_STATUS ret;
	EFI_FILE *file, *root;

	ret = uefi_call_wrapper(io->OpenVolume, 2, io, &root);
	if (EFI_ERROR(ret))
		goto out;

	ret = uefi_call_wrapper(root->Open, 5, root, &file, filename,
				EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
	if (EFI_ERROR(ret))
		goto out;

	ret = uefi_call_wrapper(file->Delete, 1, file);

out:
	if (EFI_ERROR(ret) || ret == EFI_WARN_DELETE_FAILURE)
		error(L"Failed to delete file %s:%r\n", filename, ret);

	return ret;
}

BOOLEAN uefi_exist_file(EFI_FILE *parent, CHAR16 *filename)
{
	EFI_STATUS ret;
	EFI_FILE *file;

	ret = uefi_call_wrapper(parent->Open, 5, parent, &file, filename,
				EFI_FILE_MODE_READ, 0);
	if (!EFI_ERROR(ret))
		uefi_call_wrapper(file->Close, 1, file);
	else if (ret != EFI_NOT_FOUND) // IO error
		error(L"Failed to found file %s:%r\n", filename, ret);

	return ret == EFI_SUCCESS;
}

BOOLEAN uefi_exist_file_root(EFI_FILE_IO_INTERFACE *io, CHAR16 *filename)
{
	EFI_STATUS ret;
	EFI_FILE *root;

	ret = uefi_call_wrapper(io->OpenVolume, 2, io, &root);
	if (EFI_ERROR(ret)) {
		error(L"Failed to open volume %s:%r\n", filename, ret);
		return FALSE;
	}

	return uefi_exist_file(root, filename);
}

EFI_STATUS uefi_create_directory(EFI_FILE *parent, CHAR16 *dirname)
{
	EFI_STATUS ret;
	EFI_FILE *dir;

	ret = uefi_call_wrapper(parent->Open, 5, parent, &dir, dirname,
				EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
				EFI_FILE_DIRECTORY);

	if (EFI_ERROR(ret)) {
		error(L"Failed to create directory %s:%r\n", dirname, ret);
	} else {
		uefi_call_wrapper(dir->Close, 1, dir);
	}

	return ret;
}

EFI_STATUS uefi_create_directory_root(EFI_FILE_IO_INTERFACE *io, CHAR16 *dirname)
{
	EFI_STATUS ret;
	EFI_FILE *root;

	ret = uefi_call_wrapper(io->OpenVolume, 2, io, &root);
	if (EFI_ERROR(ret)) {
		error(L"Failed to open volume %s:%r\n", dirname, ret);
		return ret;
	}

	return uefi_create_directory(root, dirname);
}

EFI_STATUS uefi_file_get_size(EFI_HANDLE image, CHAR16 *filename, UINT64 *size)
{
	EFI_STATUS ret;
	EFI_LOADED_IMAGE *info;
	struct file *file;
	UINT64 fsize;

	ret = handle_protocol(image, &LoadedImageProtocol, (void **)&info);
	if (EFI_ERROR(ret)) {
		error(L"HandleProtocol %s (%r)\n", filename, ret);
		return ret;
	}
	ret = file_open(info, filename, &file);
	if (EFI_ERROR(ret)) {
		error(L"FileOpen %s (%r)\n", filename, ret);
		return ret;
	}
	ret = file_size(file, &fsize);
	if (EFI_ERROR(ret)) {
		error(L"FileSize %s (%r)\n", filename, ret);
		return ret;
	}
	ret = file_close(file);
	if (EFI_ERROR(ret)) {
		error(L"FileClose %s (%r)\n", filename, ret);
		return ret;
	}

	*size = fsize;

	return ret;
}

EFI_STATUS uefi_call_image(
	IN EFI_HANDLE parent_image,
	IN EFI_HANDLE device,
	IN CHAR16 *filename,
	OUT UINTN *exit_data_size,
	OUT CHAR16 **exit_data)
{
	EFI_STATUS ret;
	EFI_DEVICE_PATH *path;
	UINT64 size;
	EFI_HANDLE image;

	debug(L"Call image file %s\n", filename);
	path = FileDevicePath(device, filename);
	if (!path) {
		error(L"Error getting device path : %s\n", filename);
		return EFI_INVALID_PARAMETER;
	}

	ret = uefi_file_get_size(parent_image, filename, &size);
	if (EFI_ERROR(ret)) {
		error(L"GetSize %s (%r)\n", filename, ret);
		goto out;
	}

	ret = uefi_call_wrapper(BS->LoadImage, 6, FALSE, parent_image, path,
				NULL, size, &image);
	if (EFI_ERROR(ret)) {
		error(L"LoadImage %s (%r)\n", filename, ret);
		goto out;
	}

	ret = uefi_call_wrapper(BS->StartImage, 3, image,
				exit_data_size, exit_data);
	if (EFI_ERROR(ret))
		info(L"StartImage returned error %s (%r)\n", filename, ret);

out:
	if (path)
		FreePool(path);

	return ret;
}

EFI_STATUS uefi_set_simple_var(char *name, EFI_GUID *guid, int size, void *data,
			       BOOLEAN persistent)
{
	EFI_STATUS ret;
	CHAR16 *name16 = stra_to_str((CHAR8 *)name);

	if (persistent)
		ret = LibSetNVVariable(name16, guid, size, data);
	else
		ret = LibSetVariable(name16, guid, size, data);

	free(name16);
	return ret;
}

INT8 uefi_get_simple_var(char *name, EFI_GUID *guid)
{
	void *buffer;
	UINT64 ret;
	UINTN size;
	CHAR16 *name16 = stra_to_str((CHAR8 *)name);
	buffer = LibGetVariableAndSize(name16, guid, &size);

	if (buffer == NULL) {
		error(L"Failed to get variable %s\n", name16);
		ret = -1;
		goto out;
	}

	if (size > sizeof(ret)) {
		error(L"Tried to get UEFI variable larger than %d bytes (%d bytes)."
		      " Please use an appropriate retrieve method.\n", sizeof(ret), size);
		ret = -1;
		goto out;
	}

	ret = *(INT8 *)buffer;
out:
	free(name16);
	if (buffer)
		free(buffer);
	return ret;
}

EFI_STATUS uefi_usleep(UINTN useconds)
{
	return uefi_call_wrapper(BS->Stall, 1, useconds);
}

EFI_STATUS uefi_msleep(UINTN mseconds)
{
	return uefi_msleep(mseconds * 1000);
}
