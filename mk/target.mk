# Uniform freestanding C objects. Every included header participates in rebuilds.
TARGET_INCLUDES ?=
$(OUT)/%.o: %.c $(MAKEFILE_LIST) | $(OUT)
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_INCLUDES) -MMD -MP -c $< -o $@
-include $(wildcard $(OUT)/*.d $(OUT)/*/*.d)
