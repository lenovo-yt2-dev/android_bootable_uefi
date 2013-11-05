LOCAL_PATH := $(call my-dir)

################################################################################
include $(CLEAR_VARS)

LOCAL_MODULE := efilinux
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(PRODUCT_OUT)

ifeq ($(BOARD_HAVE_LIMITED_POWERON_FEATURES),true)
	OSLOADER_EM_POLICY := fake
	LOCAL_CFLAGS += -DDISABLE_SECURE_BOOT
endif

ifneq ($(filter $(TARGET_BUILD_VARIANT),eng userdebug),)
	LOCAL_CFLAGS += -DRUNTIME_SETTINGS -DCONFIG_LOG_LEVEL=4 \
			-DCONFIG_LOG_FLUSH_TO_VARIABLE -DCONFIG_LOG_BUF_SIZE=51200 -DCONFIG_LOG_TIMESTAMP
	LOCAL_CFLAGS += -finstrument-functions -finstrument-functions-exclude-file-list=stack_chk.c,profiling.c,efilinux.h,malloc.c,stdlib.h,boot.c,log.c,platform/silvermont.c,loaders/ -finstrument-functions-exclude-function-list=handover_kernel,checkpoint,exit_boot_services,setup_efi_memory_map,Print,SPrint,VSPrint,memory_map,stub_get_current_time_us,rdtsc,rdmsr
	PROFILING_SRC_FILES := profiling.c
endif

ifeq ($(TARGET_BUILD_VARIANT),user)
	LOCAL_CFLAGS += -DCONFIG_LOG_LEVEL=1
endif

OSLOADER_EM_POLICY ?= uefi
LOCAL_CFLAGS += -DOSLOADER_EM_POLICY_OPS=$(OSLOADER_EM_POLICY)_em_ops

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/android \
	$(LOCAL_PATH)/loaders \
	$(LOCAL_PATH)/security \
	$(LOCAL_PATH)/platform \
	$(LOCAL_PATH)/bzimage \
	$(LOCAL_PATH)/fs

watchdog_src_files := \
	watchdog/tco_reset.c \
	watchdog/watchdog.c

security_src_files := \
	security/secure_boot.c

ifneq (, $(findstring isu,$(TARGET_OS_SIGNING_METHOD)))
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
	stack_chk.c \
	malloc.c \
	config.c \
	log.c \
	entry.c \
	android/boot.c \
	utils.c \
	acpi.c \
	bootlogic.c \
	intel_partitions.c \
	uefi_osnib.c \
	platform/platform.c \
	platform/silvermont.c \
	platform/airmont.c \
	platform/x86.c \
	uefi_keys.c \
	uefi_boot.c \
	uefi_utils.c \
	commands.c \
	em.c \
	fake_em.c \
	uefi_em.c \
	$(security_src_files) \
	$(watchdog_src_files) \
	$(PROFILING_SRC_FILES) \
	fs/fs.c

EFILINUX_VERSION_STRING := $(shell cd $(LOCAL_PATH) ; git describe --abbrev=12 --dirty --always)
EFILINUX_VERSION_DATE := $(shell cd $(LOCAL_PATH) ; git log --pretty=%cD HEAD^.. HEAD)
LOCAL_CFLAGS +=  -DEFILINUX_VERSION_STRING='L"$(EFILINUX_VERSION_STRING)"'
LOCAL_CFLAGS +=  -DEFILINUX_VERSION_DATE='L"$(EFILINUX_VERSION_DATE)"'
LOCAL_CFLAGS +=  -DEFILINUX_BUILD_STRING='L"$(BUILD_NUMBER) $(PRODUCT_NAME)"'

OSLOADER_FILE_PATH := EFI/BOOT/boot$(EFI_ARCH).efi
LOCAL_CFLAGS +=  -DOSLOADER_FILE_PATH='L"$(OSLOADER_FILE_PATH)"'

ifeq ($(BOARD_USE_WARMDUMP),true)
	LOCAL_CFLAGS +=  -DCONFIG_HAS_WARMDUMP
endif
WARMDUMP_FILE_PATH := EFI/Intel/warmdump.efi
LOCAL_CFLAGS +=  -DWARMDUMP_FILE_PATH='L"$(WARMDUMP_FILE_PATH)"'

include $(LOCAL_PATH)/uefi_executable.mk

################################################################################
include $(CLEAR_VARS)

LOCAL_MODULE := warmdump
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(PRODUCT_OUT)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/platform \
	$(LOCAL_PATH)/fs

LOCAL_SRC_FILES := \
	stack_chk.c \
	malloc.c \
	utils.c \
	acpi.c \
	uefi_utils.c \
	fs/fs.c \
	log.c \
	config.c \
	warmdump.c

WARMDUMP_VERSION_STRING := $(shell cd $(LOCAL_PATH) ; git describe --abbrev=12 --dirty --always)
WARMDUMP_VERSION_DATE := $(shell cd $(LOCAL_PATH) ; git log --pretty=%cD HEAD^..HEAD)
LOCAL_CFLAGS += -DWARMDUMP_VERSION_STRING='L"$(WARMDUMP_VERSION_STRING)"'
LOCAL_CFLAGS += -DWARMDUMP_VERSION_DATE='L"$(WARMDUMP_VERSION_DATE)"'
LOCAL_CFLAGS += -DWARMDUMP_BUILD_STRING='L"$(BUILD_NUMBER) $(PRODUCT_NAME)"'
LOCAL_CFLAGS += -DCONFIG_LOG_TAG='L"WARMDUMP"' -DCONFIG_LOG_LEVEL=4

include $(LOCAL_PATH)/uefi_executable.mk
