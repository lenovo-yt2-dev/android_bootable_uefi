LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)


ifeq ($(TARGET_ARCH),x86)
	EFI_ARCH := ia32
	SPECIFIC_GNU_EFI_SRC := ""
	LOCAL_CFLAGS := \
		-fPIC -fshort-wchar -ffreestanding -Wall -fno-stack-protector -mno-android -m32 \
		-D$(EFI_ARCH) -DUSE_SHIM=0\
		-DEFI_FUNCTION_WRAPPER
endif

ifeq ($(TARGET_ARCH),x86-64)
	EFI_ARCH := x86_64
	SPECIFIC_GNU_EFI_SRC := $(EFI_ARCH)/callwrap.c $(EFI_ARCH)/efi_stub.S
endif

EFI_TARGET := efi-app-$(EFI_ARCH)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/loaders \
	$(LOCAL_PATH)/android \
	$(LOCAL_PATH)/fs

LOCAL_SRC_FILES := \
	loaders/loader.c \
	loaders/bzimage/bzimage.c \
	loaders/bzimage/graphics.c \
	malloc.c \
	entry.c \
	fs/fs.c \
	android/recovery.c \
	android/boot.c \
	utils.c

EFILINUX_VERSION_STRING := $(shell cd $(LOCAL_PATH) ; git describe --abbrev=8 --dirty --always)
EFILINUX_VERSION_DATE := $(shell date -u)
LOCAL_CFLAGS +=  -DEFILINUX_VERSION_STRING='L"$(EFILINUX_VERSION_STRING)"'
LOCAL_CFLAGS +=  -DEFILINUX_VERSION_DATE='L"$(EFILINUX_VERSION_DATE)"'

LOCAL_STATIC_LIBRARY := libgnuefi
LOCAL_IMPORT_C_INCLUDE_DIRS_FROM_STATIC_LIBRARIES := libgnuefi
LOCAL_MODULE := efilinux
LOCAL_MODULE_EXTENSION := so
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_PATH := $(PRODUCT_OUT)

LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_FORCE_STATIC_EXECUTABLE := true

include $(BUILD_SYSTEM)/binary.mk

$(LOCAL_INTERMEDIATE_TARGETS): PRIVATE_TARGET_GLOBAL_CFLAGS :=
$(LOCAL_BUILT_MODULE) : PRIVATE_EFI_FILE := $(PRODUCT_OUT)/$(PRIVATE_MODULE).efi
$(LOCAL_BUILT_MODULE) : EFILINUX_OBJS := $(patsubst %.c, %.o , $(LOCAL_SRC_FILES))
$(LOCAL_BUILT_MODULE) : EFILINUX_OBJS := $(patsubst %.S, %.o , $(EFILINUX_OBJS))
$(LOCAL_BUILT_MODULE) : EFILINUX_OBJS := $(addprefix $(intermediates)/, $(EFILINUX_OBJS))

$(LOCAL_BUILT_MODULE): $(TARGET_LIBGCC) libgnuefi elf_$(EFI_ARCH)_efi.lds $(all_objects)
	@mkdir -p $(dir $@)
	@echo "linking $@"
	$(TARGET_LD) \
		-Bsymbolic \
		-shared \
		-nostdlib \
		-znocombreloc \
		-L$(shell dirname $(TARGET_LIBGCC)) \
		-L$(call intermediates-dir-for, STATIC_LIBRARIES, libgnuefi) \
		-T $(PRODUCT_OUT)/elf_$(EFI_ARCH)_efi.lds \
		$(EFILINUX_OBJS) \
		$(call intermediates-dir-for, STATIC_LIBRARIES, libgnuefi)/gnuefi/crt0-efi-$(EFI_ARCH).o -lgnuefi -lgcc -o $@
	@echo "checking symbols in $@"
	$(shell if [ `nm -u $@ | wc -l` -ne 0 ] ; then exit -1 ; fi )
	@echo "creating efilinux.efi - $(EFILINUX_VERSION_STRING) - $(EFILINUX_VERSION_DATE)"
	$(TARGET_OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
		-j .rela -j .reloc --target=$(EFI_TARGET) -S $@ $(PRIVATE_EFI_FILE)
