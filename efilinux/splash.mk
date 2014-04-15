$(SPLASH_OBJ): $(SPLASH_BMP) | $(HOST_OUT_EXECUTABLES)/prebuilt-bin-to-hex
	$(hide) mkdir -p $(@D)
	$(hide) prebuilt-bin-to-hex $(basename $(notdir $@)) < $< | $(TARGET_CC) -x c - -c $(TARGET_GLOBAL_CFLAGS) $(LOCAL_TARGET_ARCH) -o $@
