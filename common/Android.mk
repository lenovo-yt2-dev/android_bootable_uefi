LOCAL_PATH := $(call my-dir)



define common_defs
$(strip \
    $(eval LOCAL_IMPORT_C_INCLUDE_DIRS_FROM_STATIC_LIBRARIES := libgnuefi) \
    $(eval LOCAL_CFLAGS += $(EFI_EXTRA_CFLAGS)) \
)
endef

include $(CLEAR_VARS)
LOCAL_SRC_FILES := log.c
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_WHOLE_STATIC_LIBRARIES := libuefi_utils libuefi_time
LOCAL_MODULE := libuefi_log
$(call common_defs)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := uefi_utils.c
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_MODULE := libuefi_utils
LOCAL_CFLAGS := -finstrument-functions
$(call common_defs)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/time
LOCAL_SRC_FILES := time/time.c time/time_silvermont.c
LOCAL_WHOLE_STATIC_LIBRARIES := libuefi_cpu
LOCAL_MODULE := libuefi_time
$(call common_defs)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/cpu
LOCAL_SRC_FILES := cpu/cpu.c
LOCAL_MODULE := libuefi_cpu
$(call common_defs)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := profiling_stub.c
LOCAL_MODULE := libuefi_profiling_stub
$(call common_defs)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := stack_chk.c
LOCAL_MODULE := libuefi_stack_chk
$(call common_defs)
include $(BUILD_STATIC_LIBRARY)

