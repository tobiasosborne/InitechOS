TOP := ..
COMPONENT := flair
include $(TOP)/mk/config.mk

SDK_HEADERS := $(wildcard $(TOP)/sdk/include/*.h)
INCLUDES := -I. -Iinclude -I$(OUT)/generated -Iinclude/assets -I$(TOP)/flair/region/include -I$(TOP)/sdk/include
HOST_INCLUDES := $(INCLUDES) -Itests -Itests/goldens -Itests/render -I$(TOP)/factory/testlib
HOST_INCLUDES += -DSYSTEM7_DECOMP='"$(abspath $(SYSTEM7_DECOMP))"' -DWIN31_DECOMP='"$(abspath $(WIN31_DECOMP))"' -DCHROME_FID_CAPTURE_DIR='"$(abspath $(SYSTEM7_DECOMP))/goldens/captures"'
TARGET_SRCS := $(wildcard *.c)
TARGET_OUT := $(OUT)/target
TARGET_INCLUDES := $(INCLUDES)
TARGET_LIB := $(OUT)/libflair.a
include $(TOP)/mk/target.mk
GENERATED := $(addprefix $(OUT)/generated/,chicago12.h geneva12.h palette.h color_canon.h chicago_packed_data.h geneva12_packed_data.h asset_lz_data.h flair_dac.h boot_logo.h)
$(TARGET_OBJS): $(GENERATED)
TOOLS := font_art2h geneva12_art2h palette_extract color_canon_extract chicago_pack geneva12_pack asset_lz flair_dac_gen

$(OUT)/tools/%: tools/%.c FORCE | $(OUT)
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -I$(OUT)/generated -Iinclude/assets -MMD -MP -MF $@.d -o $@ $<)
$(OUT)/tools/geneva12_pack: tools/chicago_pack.c FORCE | $(OUT)
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -I$(OUT)/generated -Iinclude/assets -DPACK_GENEVA12 -MMD -MP -MF $@.d -o $@ $<)

define generate
$(OUT)/generated/$(1): $(OUT)/tools/$(2) $(filter-out --header,$(3))
	@mkdir -p $$(@D)
	@$$< $(3) > $$@.tmp && mv $$@.tmp $$@
endef
$(eval $(call generate,chicago12.h,font_art2h,include/assets/chicago12.art))
$(eval $(call generate,geneva12.h,geneva12_art2h,include/assets/geneva12.art))
$(eval $(call generate,palette.h,palette_extract,--header include/assets/palette.json))
$(eval $(call generate,color_canon.h,color_canon_extract,include/assets/color_canon.json))
$(eval $(call generate,flair_dac.h,flair_dac_gen,include/assets/clut.json include/assets/color_canon.json))
$(eval $(call generate,chicago_packed_data.h,chicago_pack,))
$(eval $(call generate,geneva12_packed_data.h,geneva12_pack,))
$(eval $(call generate,asset_lz_data.h,asset_lz,))
$(eval $(call generate,boot_logo.h,boot_logo_gen,))
$(OUT)/tools/boot_logo_gen: tools/boot_logo_gen.c FORCE | $(OUT)
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -MMD -MP -MF $@.d -o $@ $< -lm)
$(OUT)/tools/mint_finder_small_icons: tools/mint_finder_small_icons.c $(GENERATED) FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -I$(OUT)/generated -Iinclude/assets -MMD -MP -MF $@.d -o $@ $< -lpng)

# Public hosted Dialog binding. Private rendering headers/sources remain here;
# clients use include/{dialog,cp_dialog_items}.h and this archive only.
HOST_AR ?= ar
HOST_DIALOG_SRCS := event.c dialog.c control.c window.c surface.c blitter.c text.c chrome.c heap.c flair_look.c region/region.c tests/render/render.c
HOST_DIALOG_OBJS := $(addprefix $(OUT)/host-dialog/,$(HOST_DIALOG_SRCS:.c=.o))
HOST_DIALOG_LIB := $(OUT)/host/libdialog.a
.PHONY: host-dialog
host-dialog: $(HOST_DIALOG_LIB)
$(OUT)/host-dialog/%.o: %.c $(GENERATED) FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $(INCLUDES) -Itests/render -MMD -MP -c $< -o $@)
$(HOST_DIALOG_LIB): $(HOST_DIALOG_OBJS) FORCE
	$(call run-command,$(HOST_AR) rcsD $@ $(HOST_DIALOG_OBJS))
-include $(HOST_DIALOG_OBJS:.o=.d)
$(OUT)/tools/chicago_pack: $(OUT)/generated/chicago12.h
$(OUT)/tools/geneva12_pack: $(OUT)/generated/geneva12.h
$(OUT)/tools/asset_lz: $(OUT)/generated/boot_logo.h

# Link this object alongside libflair.a; keep the region implementation owned
# and built by its component, with the same target flags as the Toolbox.
.PHONY: region-target
region-target:
	+$(MAKE) -C $(TOP)/flair/region OUT=$(TOP)/build/flair/region TESTS= MAKEOVERRIDES= all
all: $(OUT)/libflair.a region-target

include $(TOP)/mk/import-check.mk

test_blitter_SRCS := tests/test_blitter.c blitter.c $(TOP)/flair/region/region.c surface.c
test_blitter_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_canon_SRCS := tests/test_canon.c
test_canon_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_chicago_metrics_SRCS := tests/test_chicago_metrics.c text.c surface.c
test_chicago_metrics_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_chrome_SRCS := tests/test_chrome.c chrome.c tests/render/render.c surface.c heap.c $(TOP)/flair/region/region.c flair_look.c
test_chrome_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_chrome_fidelity_SRCS := tests/test_chrome_fidelity.c chrome.c tests/render/render.c surface.c heap.c $(TOP)/flair/region/region.c flair_look.c window.c
test_chrome_fidelity_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_chrome_fidelity_LIBS := -lpng
test_clut_SRCS := tests/test_clut.c
test_clut_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_color_canon_SRCS := tests/test_color_canon.c
test_color_canon_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_control_SRCS := tests/test_control.c control.c blitter.c text.c surface.c $(TOP)/flair/region/region.c tests/render/render.c heap.c chrome.c flair_look.c
test_control_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_control_LIBS := -lpng
test_control_record_SRCS := tests/test_control_record.c
test_control_record_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_cursor_SRCS := tests/test_cursor.c cursor.c
test_cursor_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_cursor_LIBS := -lpng
test_dialog_SRCS := tests/test_dialog.c dialog.c control.c text.c event.c window.c blitter.c surface.c chrome.c heap.c $(TOP)/flair/region/region.c tests/render/render.c flair_look.c
test_dialog_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_dialog_LIBS := -lpng
test_dialog_record_SRCS := tests/test_dialog_record.c
test_dialog_record_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_dialog_tracking_SRCS := tests/test_dialog_tracking.c dialog.c control.c text.c event.c window.c blitter.c surface.c chrome.c heap.c $(TOP)/flair/region/region.c tests/render/render.c flair_look.c
test_dialog_tracking_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_dialog_tracking_LIBS := -lpng
test_drawing_ops_SRCS := tests/test_drawing_ops.c
test_drawing_ops_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_event_SRCS := tests/test_event.c event.c
test_event_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_flair_dac_SRCS := tests/test_flair_dac.c
test_flair_headers_SRCS := tests/test_flair_headers.c
test_flair_headers_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_flair_heap_SRCS := tests/test_flair_heap.c heap.c
test_flair_heap_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_flair_mechanism_colorblind_SRCS := tests/test_flair_mechanism_colorblind.c chrome.c tests/render/render.c surface.c heap.c $(TOP)/flair/region/region.c tests/flair_look_sentinel_stub.c
test_flair_mechanism_colorblind_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_font_styles_SRCS := tests/test_font_styles.c text.c surface.c $(TOP)/flair/region/region.c
test_font_styles_CFLAGS := -D_POSIX_C_SOURCE=200809L -DFLAIR_HOSTED=1
test_geneva12_metrics_SRCS := tests/test_geneva12_metrics.c text.c surface.c
test_geneva12_metrics_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_interact_SRCS := tests/test_interact.c window.c $(TOP)/flair/region/region.c
test_interact_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_list_SRCS := tests/test_list.c list.c heap.c $(TOP)/flair/region/region.c
test_list_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_menu_SRCS := tests/test_menu.c menu.c text.c blitter.c surface.c $(TOP)/flair/region/region.c tests/render/render.c heap.c flair_look.c
test_menu_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_menu_look_SRCS := tests/test_menu_look.c menu.c text.c blitter.c surface.c $(TOP)/flair/region/region.c tests/render/render.c heap.c flair_look.c
test_menu_look_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_menu_look_LIBS := -lpng
test_menu_record_COUNT_PATTERN := ^test-menu-record: ([0-9]+) checks.*$$
test_menu_record_SRCS := tests/test_menu_record.c
test_menu_record_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_process_SRCS := tests/test_process.c process.c window.c heap.c $(TOP)/flair/region/region.c
test_process_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_process_activate_SRCS := tests/test_process_activate.c process.c window.c heap.c $(TOP)/flair/region/region.c
test_process_activate_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_process_budget_SRCS := tests/test_process_budget.c process.c window.c heap.c $(TOP)/flair/region/region.c
test_process_budget_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_process_teardown_SRCS := tests/test_process_teardown.c process.c window.c heap.c $(TOP)/flair/region/region.c
test_process_teardown_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_process_update_SRCS := tests/test_process_update.c process.c window.c heap.c $(TOP)/flair/region/region.c
test_process_update_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_resource_SRCS := tests/test_resource.c resource.c heap.c
test_resource_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_scrap_SRCS := tests/test_scrap.c scrap.c process.c window.c heap.c $(TOP)/flair/region/region.c
test_scrap_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_scrollbar_look_SRCS := tests/test_scrollbar_look.c control.c blitter.c text.c surface.c $(TOP)/flair/region/region.c tests/render/render.c heap.c chrome.c flair_look.c
test_scrollbar_look_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_scrollbar_look_LIBS := -lpng
test_stdfile_SRCS := tests/test_stdfile.c stdfile.c list.c heap.c $(TOP)/flair/region/region.c
test_stdfile_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_tbx_menu_SRCS := tests/test_tbx_menu.c menu.c text.c blitter.c surface.c $(TOP)/flair/region/region.c tests/render/render.c heap.c flair_look.c
test_tbx_menu_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_te_surface_SRCS := tests/test_te_surface.c textedit.c scrap.c heap.c
test_te_surface_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_text_SRCS := tests/test_text.c text.c
test_text_CFLAGS := -D_POSIX_C_SOURCE=200809L -DFLAIR_HOSTED=1
test_text_clip_SRCS := tests/test_text_clip.c surface.c $(TOP)/flair/region/region.c
test_text_clip_CFLAGS := -D_POSIX_C_SOURCE=200809L -DFLAIR_HOSTED=1
test_textedit_SRCS := tests/test_textedit.c textedit.c scrap.c heap.c
test_textedit_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_window_SRCS := tests/test_window.c window.c $(TOP)/flair/region/region.c
test_window_CFLAGS := -DWINDOW_ENABLE_R1_OPS
test_winscroll_SRCS := tests/test_winscroll.c window.c $(TOP)/flair/region/region.c
test_winscroll_CFLAGS := -D_POSIX_C_SOURCE=200809L -DWINDOW_ENABLE_R1_OPS
test_asset_lz_SRCS := tests/test_asset_lz.c
test_asset_lz_CFLAGS :=
test_chicago_pack_SRCS := tests/test_chicago_pack.c
test_chicago_pack_CFLAGS :=
test_geneva12_pack_SRCS := tests/test_geneva12_pack.c
test_geneva12_pack_CFLAGS :=
test_flair_part_map_SRCS := tests/test_flair_part_map.c flair_look.c surface.c
test_flair_part_map_CFLAGS :=
test_skin_teal_SRCS := tests/test_skin_teal.c
test_skin_teal_CFLAGS :=
test_skin_era_frozen_SRCS := tests/test_skin_era_frozen.c
test_skin_era_frozen_CFLAGS :=
test_flair_routing_SRCS := tests/test_flair_routing.c tests/flair_routing_stubs.c tests/platform_routing.c tbxgate.c event.c process.c window.c heap.c $(TOP)/flair/region/region.c chrome.c surface.c flair_look.c tests/render/render.c
test_flair_routing_CFLAGS := -no-pie
HOST_TESTS := test_blitter test_canon test_chicago_metrics test_chrome test_chrome_fidelity test_clut test_color_canon test_control test_control_record test_cursor test_dialog test_dialog_record test_dialog_tracking test_drawing_ops test_event test_flair_dac test_flair_headers test_flair_heap test_flair_mechanism_colorblind test_font_styles test_geneva12_metrics test_interact test_list test_menu test_menu_look test_menu_record test_process test_process_activate test_process_budget test_process_teardown test_process_update test_resource test_scrap test_scrollbar_look test_stdfile test_tbx_menu test_te_surface test_text test_text_clip test_textedit test_window test_winscroll test_asset_lz test_chicago_pack test_geneva12_pack test_flair_part_map test_skin_teal test_skin_era_frozen test_flair_routing
# Host integration binding: real DOS, confined to this one program.
HOST_TESTS += test_tbx_file test_boot_logo test_event_record
test_event_record_SRCS := tests/test_event.c event.c
test_event_record_CFLAGS := -DFINDER_DBLCLICK_TICKS=3000u
test_boot_logo_SRCS := tests/test_boot_logo.c
test_boot_logo_LIBS := -lm
test_tbx_file_SRCS := tests/test_tbx_file.c tests/platform_dos.c $(addprefix $(TOP)/kernel/,int21.c devices.c mcb.c sft.c psp.c irq.c)
test_tbx_file_CFLAGS := -I$(TOP)/kernel/include -Itests/platform -D_GNU_SOURCE
test_tbx_file_DEPS := $(GENERATED)

$(foreach t,$(HOST_TESTS),$(eval $(t)_DEPS += $(GENERATED) $(OUT)/host-config))
include $(TOP)/mk/host-test.mk

.PHONY: corpus-check smoke
corpus-check:
	@test -d "$(SYSTEM7_DECOMP)/goldens/captures" || { echo 'FAIL: System 7 corpus missing; set SYSTEM7_DECOMP' >&2; exit 1; }
	@test -d "$(WIN31_DECOMP)" || { echo 'FAIL: Windows 3.1 corpus missing; set WIN31_DECOMP' >&2; exit 1; }

test-build: corpus-check
$(HOST_TEST_BINS): | corpus-check
# Run one wrapper per program: isolates outputs, supplies runtime capture paths,
# and preserves the transcript comparison performed by the legacy recipe.
define host_test_run
n=$$((n+1)); \
if SYSTEM7_DECOMP="$(abspath $(SYSTEM7_DECOMP))" WIN31_DECOMP="$(abspath $(WIN31_DECOMP))" sh tests/run.sh "$(abspath $(HOST_OUT)/$(1))" "$(abspath $(OUT))" > $(HOST_OUT)/$(1).log 2>&1; then \
 echo "  ok    $(1)"; \
else \
 echo "  FAIL  $(1) (log: $(HOST_OUT)/$(1).log)"; tail -n 15 $(HOST_OUT)/$(1).log; fail=$$((fail+1)); \
fi;
endef
smoke:
	@echo 'FAIL: booted-image Toolbox smoke awaits kernel/desktop import; see tests/SMOKES.md' >&2
	@exit 1
MUTANT_PATCHES ?= $(wildcard mutants/*.patch)
MUTANT_JOBS ?= 16
.PHONY: mutants
mutants:
	+@sh tools/run-mutants.sh "$(abspath $(TOP))" "$(abspath $(OUT))" "$(MAKE)" "$(MUTANT_JOBS)" $(MUTANT_PATCHES)



.PHONY: force-config
force-config:
$(OUT)/host-config: force-config | $(OUT)
	@printf '%s\n' "$(abspath $(SYSTEM7_DECOMP))" "$(abspath $(WIN31_DECOMP))" > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

TEST_BUDGET_S := 30
CORPORA := SYSTEM7_DECOMP WIN31_DECOMP
include $(TOP)/mk/corpus.mk
-include $(wildcard $(OUT)/tools/*.d $(OUT)/host/*.d)
