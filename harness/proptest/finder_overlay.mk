# Ref: ADR-0014 DEC-14-9(a), Rule 6. A Finder mutation must perturb the
# overlaid body as well as its resident model. Keep the usual per-kernel
# identity/CRC packing; each variant replaces only its native COLD object.
FINDER_OVL_INC := -DFLAIR_LIVE_TENANTS -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/milton
define finder-ovl-object
$(BUILD)/finder_$(1)_ovl_$(2).o: $(3) $(4)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(OVL_SECTION_OPT) $(FINDER_OVL_INC) $(5) -c $$< -o $$@
endef
define finder-ovl-kernel
$(BUILD)/flairovl_$(1).elf: $(BUILD)/$(1).elf $(OVL_HDR_OBJ) $(2) $(OVL_LD) $(TRANSIENT_IMPORTS_LD)
	$(OBJCOPY) $$$$(sed 's/^/-N /' $(BUILD)/ovl_exports.txt) $$< $$@.syms
	$(LD) -m elf_i386 -T $(OVL_LD) --just-symbols=$$@.syms -o $$@ $(OVL_HDR_OBJ) $(2)
endef

# The resident templates record their original flags. Map the kernels whose
# moved bodies actually contain these knobs; unaffected variants share COLD.
OVL_ACTION_TAGS := te_no_delete_now locked_notice open_no_launch recordxlong
$(foreach t,$(OVL_ACTION_TAGS),$(eval $(call finder-ovl-object,actions,$(t),os/milton/finder_actions.c,os/milton/finder_actions.h,$(OVL_ACTION_FLAGS_$(t)))))
$(foreach t,$(OVL_ACTION_TAGS),$(eval $(call finder-ovl-kernel,kernel_flairtenants_mut_$(t),$(filter-out $(BUILD)/finder_actions_ovl.o,$(OVL_SECTION_OBJS)) $(BUILD)/finder_actions_ovl_$(t).o)))
# The extra-long record image uses the recordxlong kmain, including its
# event-driven FILE COPY stepping (same frames as the pre-overlay clip).
$(eval $(call finder-ovl-kernel,kernel_flairtenants_recordxlongdbl,$(filter-out $(BUILD)/finder_actions_ovl.o,$(OVL_SECTION_OBJS)) $(BUILD)/finder_actions_ovl_recordxlong.o))

OVL_OPS_TAGS := ops_trash_no_stage locked_stage finder_origin finder_replace te_putaway
OVL_OPS_FLAGS_te_putaway := -DFINDER_MUT_PUTAWAY_REPOPULATE
$(foreach t,$(OVL_OPS_TAGS),$(eval $(call finder-ovl-object,ops,$(t),os/flair/finder_ops_cold.c,os/flair/finder_ops_private.h,$(OVL_OPS_FLAGS_$(t)))))
$(foreach t,$(OVL_OPS_TAGS),$(eval $(call finder-ovl-kernel,kernel_flairtenants_mut_$(t),$(filter-out $(BUILD)/finder_ops_ovl.o,$(OVL_SECTION_OBJS)) $(BUILD)/finder_ops_ovl_$(t).o)))
$(foreach t,finder_origin finder_replace te_putaway,$(eval $(call finder-ovl-kernel,kernel_flairtenants_mut_$(t)_hold,$(filter-out $(BUILD)/finder_ops_ovl.o,$(OVL_SECTION_OBJS)) $(BUILD)/finder_ops_ovl_$(t).o)))

OVL_WINDOW_TAGS := locked_purge l002 purge_keeps_views finder_disk_alert finder_launch_alert finder_move_alert te_notrash
OVL_WINDOW_FLAGS_l002 := -DFINDER_WIN_MUT_PURGE_KEEPS_VIEWS -DFINDER_WIN_MUT_VIEW_UNCHECKED
OVL_WINDOW_FLAGS_te_notrash := -DFINDER_MUT_NOTRASH_GENERIC
$(foreach t,$(OVL_WINDOW_TAGS),$(eval $(call finder-ovl-object,windows,$(t),os/flair/finder_windows_cold.c,os/flair/finder_windows_private.h,$(OVL_WINDOW_FLAGS_$(t)))))
$(foreach t,$(OVL_WINDOW_TAGS),$(eval $(call finder-ovl-kernel,kernel_flairtenants_mut_$(t),$(filter-out $(BUILD)/finder_windows_ovl.o,$(OVL_SECTION_OBJS)) $(BUILD)/finder_windows_ovl_$(t).o)))
$(foreach t,finder_disk_alert finder_launch_alert finder_move_alert te_notrash,$(eval $(call finder-ovl-kernel,kernel_flairtenants_mut_$(t)_hold,$(filter-out $(BUILD)/finder_windows_ovl.o,$(OVL_SECTION_OBJS)) $(BUILD)/finder_windows_ovl_$(t).o)))

# Included source files participate in both host and resident dependencies.
$(KERNEL_FINDER_OPS_OBJ): os/flair/finder_ops_private.h os/flair/finder_ops_cold.c
$(KERNEL_FINDER_WINDOWS_OBJ): os/flair/finder_windows_private.h os/flair/finder_windows_cold.c
$(KERNEL_FLAIRTENANTS_MAIN_OBJ): os/milton/finder_actions.h
$(BUILD)/finder_ops_ovl.o: os/flair/finder_ops.h os/flair/finder_windows.h
$(BUILD)/finder_windows_ovl.o: os/flair/finder_windows.h spec/assets/chicago12.h
