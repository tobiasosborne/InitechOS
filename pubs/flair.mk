# ---------------------------------------------------------------------------
# The FLAIR manuals (bead initech-ipu5.11) -- included from pubs/pubs.mk.
# PUB-REF-0001 GC-01 (documented = shipped, both directions), GC-04 (screens
# regenerated), CP-10 (every menu and every item of every shipped menu, with
# keyboard equivalents); the F-MAC books FLR-UG, FLR-GS, FLR-ACC, FLR-QR
# (pubs/manuals/flair-*).  Host C + make + thin shell (Law 3).
#
#   make pubs-flair-inventory   the SHIPPED FLAIR surface, generated from
#                               source: build/pubs/correctness/flair.shipped
#                               (harness/pubs/flair_inventory.c: the SYSTEM,
#                               Apple, Finder, HELLO, NOTES and TENANTFX bars,
#                               the INITECH volume; flair_app_menus.c: the bar
#                               of every disk application written in C;
#                               harness/pubs/flair_alerts.sh: every alert's
#                               first line, quoted from the source)
#   make pubs-flair-gen         expand every committed FLAIR capture
#                               (pubs/manuals/flair-*/screens/*.grey) into the
#                               PNG the book prints (build/pubs/gen/flair/)
#   make test-pubs-flair        GC-01 over the FLAIR books together: their
#                               main.cmds (initech-syntax \RegisterCommand)
#                               against flair.shipped, both directions; every
#                               printed FLAIR figure has its trace, capture
#                               row and committed capture, every capture is
#                               printed, and every documented desktop
#                               operation is proved by a capture that settles
#                               on its marker (harness/pubs/check_flair.sh)
#   make pubs-flair-screens     re-capture every FLAIR figure from the running
#                               desktop into the source tree (review the diff)
#   make test-pubs-flair-screens  EMU GATE: re-capture into build/ and compare
#                               byte for byte with the committed captures;
#                               joined to test-pubs-screens
#   make test-pubs-flair-mutant the named Rule-6 mutants of the new rules
# ---------------------------------------------------------------------------
PUBS_FLAIR_BOOKS := $(patsubst $(PUBS_DIR)/manuals/%/main.tex,%,$(wildcard $(PUBS_DIR)/manuals/flair-*/main.tex))
PUBS_FLAIR_INV   := $(BUILD)/pubs_flair_inventory
PUBS_FLAIR_GREY  := $(BUILD)/pubs_flair_grey
PUBS_FLAIR_KMAIN := $(PUBS_CHECK)/flair_kmain.i
PUBS_FLAIR_APPS  := os/apps/calc/calc.c os/apps/calcda/calcda.c os/apps/ctenant/ctenant.c \
                    os/apps/itext/itext.c os/apps/paint/paint.c
PUBS_FLAIR_ALLOW := harness/pubs/flair.allow
# The image the FLAIR figures are captured on: the Rule-14 EXTRA-LONG record
# image of the shipped desktop (12000-tick pump; its FILE COPY bar steps per
# injected event, so the mid-copy figure is the same every run) with the
# transient COMMAND.COM linked against it (the exit_dos.mk recipe), so Exit
# to InitechDOS works in a figure.  pubs/screens/flair_capture.sh says why a
# record image: the shipped image's 250-tick pump ends before a manual-length
# trace is injected, and the 3000-tick one ran out under host load.
# The kernel: the extra-long record kernel's objects (kmain with
# -DFLAIR_TEN_TICK_BUDGET=12000 -DFILECOPY_RECORD_STEP=125, the record
# double-click bounds, the steady record console cursor) linked with the
# transient's import list, as $(BUILD)/kernel_flairtenants_recorddbl.elf is.
PUBS_FLAIR_KOBJS := $(filter-out $(KERNEL_FLAIRTENANTS_MAIN_OBJ) $(KERNEL_FINDER_DESKTOP_OBJ) $(KERNEL_CONSOLE_OBJ),$(KERNEL_FLAIRTENANTS_OBJS)) \
                    $(BUILD)/kmain_flairtenants_mut_recordxlong.o $(BUILD)/finder_desktop_recorddbl.o $(BUILD)/console_record.o
$(BUILD)/kernel_flairtenants_pubs.elf: $(PUBS_FLAIR_KOBJS) $(KERNEL_LD) $(TRANSIENT_IMPORTS_LD) | $(BUILD)
	$(KERNEL_LINK) -o $@ $(PUBS_FLAIR_KOBJS) @$(TRANSIENT_IMPORTS_LD)
$(BUILD)/kernel_flairtenants_pubs.bin: $(BUILD)/kernel_flairtenants_pubs.elf | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); \
	if [ "$$sz" -gt "$$max" ]; then printf '!!! kernel_flairtenants_pubs.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes)\n' "$$sz" "$$max"; exit 1; fi; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none
	$(call kernel-end-guard,$<,flairtenants-pubs)
$(eval $(call exitdos-transient-rules,pubs,$(BUILD)/kernel_flairtenants_pubs.elf))
PUBS_FLAIR_IMG   := $(BUILD)/flair_tenants_pubs.img
$(PUBS_FLAIR_IMG): $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_flairtenants_pubs.bin $(BUILD)/transient_pubs.bin $(IO_SYS_BIN) $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT) | $(BUILD)
	$(call exitdos-image-recipe,$(BUILD)/kernel_flairtenants_pubs.bin,$(BUILD)/transient_pubs.bin)
	@printf ">>> pubs: FLAIR capture image (extra-long record kernel + transient COMMAND.COM): %s\n" "$@"
PUBS_FLAIR_BOOT  := $(PUBS_FLAIR_IMG)
PUBS_FLAIR_STEP  := $(PUBS_FLAIR_IMG)
# The data volume of the Locked-notice figures: the shipped volume with
# README.TXT read-only (the test-flair-locked-trash fixture; no file of the
# volume as delivered is locked).  Rows name it with image=locked.
PUBS_FLAIR_LOCKED := $(BUILD)/record_locked_trash.img

$(PUBS_FLAIR_GREY): harness/pubs/flair_grey.c | $(BUILD)
	$(CC) $(CFLAGS) -O2 -o $@ $<
$(PUBS_FLAIR_INV): harness/pubs/flair_inventory.c os/flair/finder_menu.c os/flair/finder_menu.h \
		os/flair/finder_cmd.c os/flair/finder_cmd.h os/flair/apple_menu.c os/flair/apple_menu.h \
		spec/apple_menu_items.h os/flair/shell_menus.h spec/assets/menu_canon.h | $(BUILD)
	$(CC) -std=c11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -Wl,--gc-sections \
		$(MENU_INC) -o $@ $<
$(BUILD)/pubs_flair_app_%: harness/pubs/flair_app_menus.c os/apps/tbx/tbx.h spec/toolbox_gate.h $(PUBS_FLAIR_APPS) | $(BUILD)
	$(CC) -std=c11 -w -ffunction-sections -fdata-sections -Wl,--gc-sections \
		-DAPP_SRC='"../../$(filter %/$*.c,$(PUBS_FLAIR_APPS))"' \
		-Ispec -Ispec/assets -Ios/apps/tbx -Ios/apps/calcda -Ios/flair -Ios/flair/atkinson \
		-Ios/i123/core -Ios/i123 -Ios -Iseed -Ibuild -o $@ $<

.PHONY: pubs-flair-inventory pubs-flair-gen test-pubs-flair pubs-flair-screens \
	test-pubs-flair-screens test-pubs-flair-mutant
# The flagship kernel's kmain.c, preprocessed with ITS -D set (the
# $(KERNEL_FLAIRTENANTS_MAIN_OBJ) recipe): the SYSTEM titles, HELLO's items
# and the NOTES bar are kmain data.
pubs-flair-inventory: $(PUBS_FLAIR_INV) $(foreach a,$(PUBS_FLAIR_APPS),$(BUILD)/pubs_flair_app_$(basename $(notdir $(a))))
	@mkdir -p $(PUBS_CHECK)
	@$(KERNEL_CC) $(KERNEL_CFLAGS) -E -P -DBOOT_FLAIR_LIVE -DFLAIR_LIVE_TENANTS -Ispec -Ispec/assets -Ios/flair \
		-Ios/flair/atkinson -Ios/apps -I$(KERNEL_DIR) -Ibuild $(KERNEL_MAIN_C) -o $(PUBS_FLAIR_KMAIN)
	@{ $(PUBS_FLAIR_INV) $(PUBS_FLAIR_KMAIN) os/apps/tenantfx.asm $(FLAIR_FIXTURES) && \
	   for a in $(PUBS_FLAIR_APPS); do $(BUILD)/pubs_flair_app_$$(basename $$a .c) || exit 1; done && \
	   sh harness/pubs/flair_alerts.sh; } > $(PUBS_CHECK)/flair.unsorted
	@LC_ALL=C sort $(PUBS_CHECK)/flair.unsorted > $(PUBS_CHECK)/flair.shipped
	@printf '>>> pubs-flair-inventory: %s menu items, %s files, %s alerts shipped\n' \
		"$$(grep -c '|menu$$' $(PUBS_CHECK)/flair.shipped)" "$$(grep -c '|file$$' $(PUBS_CHECK)/flair.shipped)" \
		"$$(grep -c '|alert$$' $(PUBS_CHECK)/flair.shipped)"

# Every committed capture -> build/pubs/gen/flair/<book>/<name>.png (on the
# books' TEXINPUTS through $(PUBS_GEN)//).
pubs-flair-gen: $(PUBS_FLAIR_GREY)
	@set -e; for b in $(PUBS_FLAIR_BOOKS); do \
		mkdir -p $(PUBS_GEN)/flair/$$b; \
		for g in $(PUBS_DIR)/manuals/$$b/screens/*.grey; do [ -f "$$g" ] || continue; \
			$(PUBS_FLAIR_GREY) png "$$g" $(PUBS_GEN)/flair/$$b/$$(basename "$$g" .grey).png; done; done
pubs test-pubs-build test-pubs-repro test-pubs-repro-mutant: pubs-flair-gen

ifneq ($(SKIP_PUBS),1)
test-pubs-flair: pubs-flair-inventory test-pubs-build $(PUBS_GATE)
	@# KIND operation is proved by check_flair.sh (a marker the source prints,
	@# replayed by a capture), not by the inventory: the gate gets the rest.
	@set -e; for b in $(PUBS_FLAIR_BOOKS); do \
		grep -v '|operation|' $(PUBS_BUILD)/gate/$$b/main.cmds > $(PUBS_CHECK)/flair-$$b.cmds; done
	@$(PUBS_GATE) commands $(PUBS_CHECK)/flair.shipped $(PUBS_FLAIR_ALLOW) \
		$(foreach b,$(PUBS_FLAIR_BOOKS),$(PUBS_CHECK)/flair-$(b).cmds)
	@sh harness/pubs/check_flair.sh $(PUBS_BUILD)/gate $(PUBS_FLAIR_BOOKS)
	@[ -z "$$(cat $(foreach b,$(PUBS_FLAIR_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.mentions) | grep -v '^#' || true)" ] || { \
		printf '!!! test-pubs RED COMMAND-SURFACE: a FLAIR book names an InitechDOS command with \\cmd (use \\file; the DOS books document commands)\n'; exit 1; }
	@printf '>>> test-pubs-flair: green (FLAIR books == shipped menus, files, alerts; figures and operations proved)\n'
test-pubs: test-pubs-flair
test-pubs-mutant: test-pubs-flair-mutant
test-pubs-flair-mutant: test-pubs-flair $(PUBS_FLAIR_GREY) $(PUBS_FLAIR_SCREEN_DEPS)
	@CC="$(CC)" MENU_INC="$(MENU_INC)" FLAIR_KMAIN_I=$(PUBS_FLAIR_KMAIN) FLAIR_FIXTURES="$(FLAIR_FIXTURES)" \
		HARNESS=$(HARNESS_BIN) FLAIR_BOOT=$(PUBS_FLAIR_BOOT) FLAIR_DATA=$(FLAIR_DATA_IMG) \
		CONSOLE_TEXT=$(CONSOLE_TEXT_BIN) VGABIOS=$(PUBS_VGABIOS) \
		sh harness/pubs/flair_mutants.sh $(PUBS_GATE) $(PUBS_FLAIR_GREY) $(PUBS_CHECK) $(PUBS_BUILD)/gate $(PUBS_FLAIR_ALLOW) $(PUBS_FLAIR_BOOKS)
else
test-pubs-flair test-pubs-flair-mutant:
	@printf '!!! $@ SKIPPED (SKIP_PUBS=1 opt-out) -- the FLAIR manuals were NOT checked. This is NOT a pass.\n'
endif

PUBS_FLAIR_SCREEN_DEPS := $(HARNESS_BIN) $(PUBS_FLAIR_BOOT) $(PUBS_FLAIR_STEP) $(FLAIR_DATA_IMG) $(PUBS_FLAIR_LOCKED) $(PUBS_FLAIR_GREY) $(CONSOLE_TEXT_BIN) \
                          $(PUBS_DIR)/screens/flair_capture.sh spec/assets/clut.json spec/assets/color_canon.json
pubs-flair-screens: $(PUBS_FLAIR_SCREEN_DEPS)
	@set -e; for b in $(PUBS_FLAIR_BOOKS); do \
		d=$(PUBS_DIR)/manuals/$$b/screens; [ -f $$d/FLAIR ] || continue; \
		sh $(PUBS_DIR)/screens/flair_capture.sh $(HARNESS_BIN) $(PUBS_FLAIR_BOOT) $(FLAIR_DATA_IMG) $(PUBS_FLAIR_GREY) \
			spec/assets/clut.json spec/assets/color_canon.json $(CONSOLE_TEXT_BIN) $(PUBS_VGABIOS) $$d/FLAIR $(PUBS_BUILD)/flair-screens/$$b $(PUBS_FLAIR_STEP) $(PUBS_FLAIR_LOCKED); \
		for f in $(PUBS_BUILD)/flair-screens/$$b/*.grey $(PUBS_BUILD)/flair-screens/$$b/*.console; do \
			[ -f "$$f" ] && cp -f "$$f" $$d/; done; \
		printf '>>> pubs-flair-screens: %s updated -- review `git diff %s`\n' "$$b" "$$d"; done
test-pubs-flair-screens: $(PUBS_FLAIR_SCREEN_DEPS) $(PUBS_GATE)
	@set -e; n=0; for b in $(PUBS_FLAIR_BOOKS); do \
		d=$(PUBS_DIR)/manuals/$$b/screens; [ -f $$d/FLAIR ] || continue; \
		rm -rf $(PUBS_BUILD)/flair-screens-check/$$b; \
		sh $(PUBS_DIR)/screens/flair_capture.sh $(HARNESS_BIN) $(PUBS_FLAIR_BOOT) $(FLAIR_DATA_IMG) $(PUBS_FLAIR_GREY) \
			spec/assets/clut.json spec/assets/color_canon.json $(CONSOLE_TEXT_BIN) $(PUBS_VGABIOS) $$d/FLAIR $(PUBS_BUILD)/flair-screens-check/$$b $(PUBS_FLAIR_STEP) $(PUBS_FLAIR_LOCKED); \
		for t in $(PUBS_BUILD)/flair-screens-check/$$b/*.grey $(PUBS_BUILD)/flair-screens-check/$$b/*.console; do \
			[ -f "$$t" ] || continue; \
			$(PUBS_GATE) same SCREEN-DRIFT $$d/$$(basename $$t) $$t || exit 1; n=$$((n + 1)); done; \
		for c in $$d/*.grey $$d/*.console; do [ -f "$$c" ] || continue; [ -f $(PUBS_BUILD)/flair-screens-check/$$b/$$(basename $$c) ] || { \
			printf '!!! test-pubs RED SCREEN-UNREPLAYED: %s has no FLAIR row\n' "$$c"; exit 1; }; done; done; \
	printf '>>> test-pubs-flair-screens: green (%s FLAIR screens in the manuals == live desktop output)\n' "$$n"
test-pubs-screens: test-pubs-flair-screens
