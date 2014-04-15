ifeq ($(TARGET_ARCH),x86)
	EFI_ARCH := ia32
	LOCAL_TARGET_ARCH := -m32
	LOCAL_TARGET_CFLAGS := -DCONFIG_X86
	LOCAL_TARGET_LIBGCC := $(shell dirname $(TARGET_LIBGCC))
endif

ifneq (,$(filter $(TARGET_ARCH),x86-64)$(filter $(BOARD_USE_64BIT_KERNEL),true))
	EFI_ARCH := x86_64
	LOCAL_TARGET_ARCH := -m64
	LOCAL_TARGET_CFLAGS := -DCONFIG_X86_64
	LOCAL_TARGET_LIBGCC := $(shell dirname $(TARGET_LIBGCC))/..
endif

LOCAL_CFLAGS += -fPIC -fPIE -fshort-wchar -ffreestanding -Wall \
	-fstack-protector -Wl,-z,noexecstack -O2 \
	-D_FORTIFY_SOURCE=2 $(LOCAL_TARGET_ARCH) $(LOCAL_TARGET_CFLAGS)

LOCAL_CFLAGS  += -g -Wall -fshort-wchar -fno-strict-aliasing \
           -fno-merge-constants
