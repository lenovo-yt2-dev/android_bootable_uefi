ifeq ($(TARGET_ARCH),x86)
	EFI_ARCH := ia32
	LOCAL_CFLAGS += \
		-fPIC -fPIE -fshort-wchar -ffreestanding -Wall -m32 \
		-fstack-protector -Wl,-z,noexecstack -O2 \
		-D_FORTIFY_SOURCE=2 -DCONFIG_X86
endif

ifeq ($(TARGET_ARCH),x86-64)
	EFI_ARCH := x86_64
endif

EFI_TARGET := efi-app-$(EFI_ARCH)

SPLASH_BMP := $(LOCAL_PATH)/splash.bmp

LOCAL_STATIC_LIBRARY := libgnuefi
LOCAL_IMPORT_C_INCLUDE_DIRS_FROM_STATIC_LIBRARIES := libgnuefi
LOCAL_MODULE_EXTENSION := so

LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_CFLAGS  += -g -Wall -fshort-wchar -fno-strict-aliasing \
           -fno-merge-constants

GNUEFI_PATH := $(call intermediates-dir-for, STATIC_LIBRARIES, libgnuefi)
EFI_CRT0 := $(GNUEFI_PATH)/gnuefi/crt0-efi-$(EFI_ARCH).o
LDS := $(PRODUCT_OUT)/elf_$(EFI_ARCH)_efi.lds
EFI_SIGNING_TOOL := $(HOST_OUT_EXECUTABLES)/isu

include $(BUILD_SYSTEM)/binary.mk

$(LOCAL_BUILT_MODULE) : PRIVATE_EFI_FILE := $(PRODUCT_OUT)/$(PRIVATE_MODULE).unsigned.efi
$(LOCAL_INTERMEDIATE_TARGETS): PRIVATE_TARGET_GLOBAL_CFLAGS :=
$(LOCAL_BUILT_MODULE) : EFI_APP_OBJS := $(patsubst %.c, %.o , $(LOCAL_SRC_FILES))
$(LOCAL_BUILT_MODULE) : EFI_APP_OBJS := $(patsubst %.S, %.o , $(EFI_APP_OBJS))
$(LOCAL_BUILT_MODULE) : EFI_APP_OBJS := $(addprefix $(intermediates)/, $(EFI_APP_OBJS))
$(LOCAL_BUILT_MODULE) : SPLASH_OBJ := $(addprefix $(intermediates)/, splash_bmp.o)

$(LOCAL_BUILT_MODULE): EFI_UNSIGNED_IN := $(PRIVATE_EFI_FILE)
$(LOCAL_BUILT_MODULE): EFI_SIGNING_OUT := $(PRODUCT_OUT)/$(PRIVATE_MODULE)_manifest_
$(LOCAL_BUILT_MODULE): EFI_MANIFEST_OUT := $(PRODUCT_OUT)/$(PRIVATE_MODULE)_manifest__OS_manifest.bin
$(LOCAL_BUILT_MODULE): EFI_SIGNED_OUT := $(PRODUCT_OUT)/$(PRIVATE_MODULE).efi

$(LOCAL_BUILT_MODULE): $(GNUEFI_PATH)/libgnuefi.a $(LDS) $(all_objects) | $(HOST_OUT_EXECUTABLES)/prebuilt-bin-to-hex $(EFI_SIGNING_TOOL)
	@mkdir -p $(dir $@)
	prebuilt-bin-to-hex splash_bmp < $(SPLASH_BMP) | $(TARGET_CC) -x c - -c $(TARGET_GLOBAL_CFLAGS) -o $(SPLASH_OBJ)
	@echo "linking $@"
	$(TARGET_TOOLS_PREFIX)ld$(HOST_EXECUTABLE_SUFFIX).bfd \
		-Bsymbolic \
		-shared \
		-z relro -z now \
		-fstack-protector \
		-fPIC -pie \
		-nostdlib \
		-znocombreloc \
		-L$(shell dirname $(TARGET_LIBGCC)) \
		-L$(GNUEFI_PATH) \
		-T $(LDS) \
		$(EFI_APP_OBJS) \
		$(SPLASH_OBJ) \
		$(EFI_CRT0) -lgnuefi -lgcc -o $@
	@echo "checking symbols in $@"
	@$(hide) unknown=`nm -u $@` ; if [ -n "$$unknown" ] ; then echo "Unknown symbol(s): $$unknown" && exit -1 ; fi
	@echo "creating $(PRIVATE_MODULE).efi - $(LOCAL_MODULE_VERSION)"
	$(TARGET_OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
		-j .rela -j .reloc --target=$(EFI_TARGET) -S $@ $(PRIVATE_EFI_FILE)
	@echo "Signing $(EFI_SIGNED_OUT) with $(EFI_SIGNING_TOOL)"
	$(EFI_SIGNING_TOOL) -h 0 -i $(EFI_UNSIGNED_IN) -o $(EFI_SIGNING_OUT) \
		-l $(TARGET_BOOT_LOADER_PRIV_KEY) -k $(TARGET_BOOT_LOADER_PUB_KEY) -t 11 -p 2 -v 1
	@cat $(EFI_UNSIGNED_IN) $(EFI_MANIFEST_OUT) > $(EFI_SIGNED_OUT)
	@echo "Signed $(EFI_SIGNED_OUT)"

