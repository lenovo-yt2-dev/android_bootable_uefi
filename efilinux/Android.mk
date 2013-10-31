LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)


ifeq ($(TARGET_ARCH),x86)
	EFI_ARCH := ia32
	SPECIFIC_GNU_EFI_SRC := ""
	LOCAL_CFLAGS := \
		-fPIC -fshort-wchar -ffreestanding -Wall -fno-stack-protector -m32 \
		-D$(EFI_ARCH)
endif

ifeq ($(TARGET_ARCH),x86-64)
	EFI_ARCH := x86_64
	SPECIFIC_GNU_EFI_SRC := $(EFI_ARCH)/callwrap.c $(EFI_ARCH)/efi_stub.S
endif

EFI_TARGET := efi-app-$(EFI_ARCH)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/android \
	$(LOCAL_PATH)/loaders \
	$(LOCAL_PATH)/security \
	$(LOCAL_PATH)/platform \
	$(LOCAL_PATH)/bzimage \
	$(LOCAL_PATH)/fs

security_src_files := \
	security/secure_boot.c

ifeq ($(TARGET_OS_SIGNING_METHOD),isu)
	LOCAL_CFLAGS += -DUSE_INTEL_OS_VERIFICATION=1 -DUSE_SHIM=0
	security_src_files += \
	      security/os_verification.c
endif

ifeq ($(TARGET_OS_SIGNING_METHOD),uefi)
	LOCAL_CFLAGS += -DUSE_SHIM=1 -DUSE_INTEL_OS_VERIFICATION=0
	security_src_files += \
	      security/shim_protocol.c
endif


LOCAL_SRC_FILES := \
	malloc.c \
	entry.c \
	android/boot.c \
	utils.c \
	acpi.c \
	bootlogic.c \
	intel_partitions.c \
	uefi_osnib.c \
	platform/platform.c \
	platform/cherrytrail.c \
	platform/x86.c \
	uefi_keys.c \
	uefi_boot.c \
	uefi_utils.c \
	commands.c \
	uefi_em.c \
	$(security_src_files) \
	fs/fs.c


SPLASH_BMP := $(LOCAL_PATH)/splash.bmp

EFILINUX_VERSION_STRING := $(shell cd $(LOCAL_PATH) ; git describe --abbrev=12 --dirty --always)
EFILINUX_VERSION_DATE := $(shell date -u)
LOCAL_CFLAGS +=  -DEFILINUX_VERSION_STRING='L"$(EFILINUX_VERSION_STRING)"'
LOCAL_CFLAGS +=  -DEFILINUX_VERSION_DATE='L"$(EFILINUX_VERSION_DATE)"'
LOCAL_CFLAGS +=  -DEFILINUX_BUILD_STRING='L"$(BUILD_NUMBER) $(PRODUCT_NAME)"'

OSLOADER_FILE_PATH := EFI/BOOT/boot$(EFI_ARCH).efi
PRIVATE_EFI_FILE := $(PRODUCT_OUT)/efilinux.efi
LOCAL_CFLAGS +=  -DOSLOADER_FILE_PATH='L"$(OSLOADER_FILE_PATH)"'

LOCAL_STATIC_LIBRARY := libgnuefi
LOCAL_IMPORT_C_INCLUDE_DIRS_FROM_STATIC_LIBRARIES := libgnuefi
LOCAL_MODULE := efilinux
LOCAL_MODULE_EXTENSION := so
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_PATH := $(PRODUCT_OUT)

LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_CFLAGS  += -g -Wall -fshort-wchar -fno-strict-aliasing \
           -fno-merge-constants -fno-stack-protector \
           -fno-stack-check

include $(BUILD_SYSTEM)/binary.mk

$(LOCAL_INTERMEDIATE_TARGETS): PRIVATE_TARGET_GLOBAL_CFLAGS :=
$(LOCAL_BUILT_MODULE) : EFILINUX_OBJS := $(patsubst %.c, %.o , $(LOCAL_SRC_FILES))
$(LOCAL_BUILT_MODULE) : EFILINUX_OBJS := $(patsubst %.S, %.o , $(EFILINUX_OBJS))
$(LOCAL_BUILT_MODULE) : EFILINUX_OBJS := $(addprefix $(intermediates)/, $(EFILINUX_OBJS))
$(LOCAL_BUILT_MODULE) : SPLASH_OBJ := $(addprefix $(intermediates)/, splash_bmp.o)

$(LOCAL_BUILT_MODULE): $(PRODUCT_OUT) $(TARGET_LIBGCC) libgnuefi elf_$(EFI_ARCH)_efi.lds $(all_objects)
	@mkdir -p $(dir $@)
	$(HOST_OUT_EXECUTABLES)/bin-to-hex splash_bmp < $(SPLASH_BMP) | $(TARGET_CC) -x c - -c $(TARGET_GLOBAL_CFLAGS) -o $(SPLASH_OBJ)
	@echo "linking $@"
	$(TARGET_LD).bfd \
		-Bsymbolic \
		-shared \
		-nostdlib \
		-znocombreloc \
		-L$(shell dirname $(TARGET_LIBGCC)) \
		-L$(call intermediates-dir-for, STATIC_LIBRARIES, libgnuefi) \
		-T $(PRODUCT_OUT)/elf_$(EFI_ARCH)_efi.lds \
		$(EFILINUX_OBJS) \
		$(SPLASH_OBJ) \
		$(call intermediates-dir-for, STATIC_LIBRARIES, libgnuefi)/gnuefi/crt0-efi-$(EFI_ARCH).o -lgnuefi -lgcc -o $@
	@echo "checking symbols in $@"
	$(shell if [ `nm -u $@ | wc -l` -ne 0 ] ; then exit -1 ; fi )
	@echo "creating efilinux.efi - $(EFILINUX_VERSION_STRING) - $(EFILINUX_VERSION_DATE) - $(BUILD_NUMBER) - signing: $(TARGET_OS_SIGNING_METHOD)"
	$(TARGET_OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
		-j .rela -j .reloc --target=$(EFI_TARGET) -S $@ $(PRIVATE_EFI_FILE)
