# ===========================================================================
# REAL gate: test-flair-trash-edges (bead initech-vnl4) -- the Finder Trash
# EDGE defects of the 2026-10-23 desktop audit
# (docs/audits/2026-10-23-desktop/REPORT.md):
#   L001 (P1) a FULL volume with no \TRASH could not be freed from the Finder:
#        staging needs \TRASH, making \TRASH needs a free cluster, so the drop
#        was refused and the user was stuck. FIX: \TRASH is still made at mount
#        while there is room (desktop_trash_ensure, design F1.4) and is
#        re-reserved whenever the Trash is wanted; when there is no room the
#        drop puts up the Delete Now confirm (the CAUTION alert, "remove ...
#        permanently?") and, on OK, removes the item in place
#        (finder_shell_delete_item) -- then reserves \TRASH in the freed space.
#        Period basis: "You can remove items from the disk to free up more
#        space" (Apple_Macintosh_System_Software_Users_Guide_V6.0.pdf printed
#        p. 262); the question is the Empty Trash confirm's own.
#   L006 Put Away of a multiple selection restored ONE item, silently.
#   L007 opening an unavailable Trash said "The item could not be moved".
#   L008 New Folder made a folder directly inside the Trash.
#
# Four boots of $(FLAIRTENANTS_HOLD_IMG) (the flagship tenants kernel with the
# 1500-tick life the long Finder traces need), each on a FRESH copy of its
# volume, traces locked in spec/flair_trash_edges_traces.mk. Judged by the
# serial chain, by MTOOLS on the stopped image (never our FAT code), by
# fsck.fat -n, and the alerts by the independent pixel graders
# (tools/ppm_flair_trash_check vs the s8_alert_modal.png capture; the CAUTION
# icon vs the mint-008 capture):
#   [1] FULL-ALERT  $(FINDER_FULL_DATA): README.TXT dropped on the Trash ->
#        TRASH-RESERVE-FAIL rc=-11, FINDER-DELETE-ALERT name=README.TXT; the
#        dump: two-button alert chrome + the CAUTION icon; mtools: README.TXT
#        untouched (nothing goes before OK).
#   [2] FULL-DELETE the same + OK -> FINDER-DELETE-NOW name=README.TXT
#        purged=1, then TRASH-CREATE, then the Trash OPENS (n=0); mtools:
#        README.TXT gone, ::/TRASH a directory, FILL.BIN intact; fsck clean.
#   [3] NOTRASH     $(FINDER_FULL_DATA): double-click the Trash ->
#        FINDER-WIN-OPEN-FAIL rc=-13 and the notice "The Trash could not be
#        opened because there is no room for it on the disk." (never "could
#        not be moved"); the dump: the notice chrome.
#   [4] PUTAWAY     the stock volume: README.TXT + AUTOEXEC.BAT staged, the
#        Trash opened, Ctrl-N (DARK there: no folder made), Ctrl-A, Ctrl-Y ->
#        FINDER-PUT-AWAY n=2; Ctrl-W, Ctrl-N in the root -> the ONE
#        FINDER-NEW-FOLDER, parent=0; mtools: both files home byte-identical,
#        NEWFOLD in the root, ::/TRASH empty; fsck clean.
# Mutation-proven by test-flair-trash-edges-mutant, each knob restoring the
# audited pre-fix code on the same hold image and the same oracle:
#   KMAIN_MUT_NO_DELETE_NOW          [2] RED (no Delete Now; README.TXT stays)
#   FINDER_MUT_NOTRASH_GENERIC       [3] RED (the "could not be moved" text)
#   FINDER_MUT_PUTAWAY_REPOPULATE    [4] RED (FINDER-PUT-AWAY n=1)
#   FINDER_CMD_MUT_NEWFOLDER_IN_TRASH [4] RED (a NEWFOLD made in \TRASH)
# Rule 14 clips: record-flair SCRIPT=trash_delete_now | trash_notrash_open |
# trash_putaway_multi (registered in the Makefile before record-flair).
# Rule 5: Bochs halts at the 640x480 guard before the Finder on these images
# (test-flair-trash-window-bochs), so no Bochs leg is added here.
# ===========================================================================

# THE FULL VOLUME (the audit's evidence/prepare_fixtures.py full-data.img): the
# flagship volume plus FILL.BIN sized by MTOOLS to the volume's own free byte
# count, so zero clusters are free, and no \TRASH (the flagship volume has none
# until a boot makes one). Pinned epoch (Rule 11).
$(FINDER_FULL_DATA): $(FLAIR_DATA_IMG) harness/finder_trash_edges.mk | $(BUILD)
	cp -f $< $@.tmp
	@free=$$(mdir -i $@.tmp :: | sed -n 's/^ *\([0-9 ]*[0-9]\) bytes free$$/\1/p' | tr -d ' '); \
	[ -n "$$free" ] && [ "$$free" -gt 0 ] || { printf '!!! finder_full_fixture: mdir reported no free bytes to fill\n'; rm -f $@.tmp; exit 1; }; \
	head -c "$$free" /dev/zero | tr '\000' 'F' > $(BUILD)/finder_full_fill.bin
	SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -i $@.tmp $(BUILD)/finder_full_fill.bin ::FILL.BIN
	@mdir -i $@.tmp :: | grep -qx ' *0 bytes free' || { printf '!!! finder_full_fixture: the volume is not full after FILL.BIN\n'; rm -f $@.tmp; exit 1; }
	@! mdir -a -i $@.tmp ::TRASH >/dev/null 2>&1 || { printf '!!! finder_full_fixture: the full volume must have no TRASH\n'; rm -f $@.tmp; exit 1; }
	mv -f $@.tmp $@
	@$(call fixture-check,recipe,$@)
$(eval $(call fixture-contract,$(FINDER_FULL_DATA),$(FLAIR_FIXTURES) FILL.BIN))

TE_DELETE_MSG  := There is no room on the disk for the Trash. Are you sure you want to remove README.TXT permanently?
TE_NOTRASH_MSG := The Trash could not be opened because there is no room for it on the disk.
# The alert text the serial carries, re-joined: every FINDER-ALERT-LINE after
# marker $(2), up to the next non-line, joined by single spaces.
te-alert-text = awk -v m='$(2)' 'f && /^FINDER-ALERT-LINE /{sub(/^FINDER-ALERT-LINE /,""); t = t (t==""?"":" ") $$0; next} f{exit} index($$0,m)==1{f=1} END{print t}' $(1)
# The Delete Now confirm prints its lines BEFORE its marker.
te-delete-text = awk '/^FINDER-ALERT-LINE /{sub(/^FINDER-ALERT-LINE /,""); t = t (t==""?"":" ") $$0; next} /^FINDER-DELETE-ALERT /{print t; exit} {t=""}' $(1)
te-fsck-clean = out=$$(fsck.fat -n $(1) 2>&1) && ! printf '%s' "$$out" | grep -qiE 'reclaim|unused|lost|orphan|bad '

# The leg properties, as shell conditions over $(BUILD)/<name>.serial and
# $(BUILD)/<name>_data.img: shared VERBATIM by the clean gate (must hold) and
# its mutants (must NOT hold).
TE_FULL_ALERT = grep -qxF 'TRASH-INIT-FAIL rc=-11' $(BUILD)/$(1).serial \
	&& grep -qxF 'TRASH-RESERVE-FAIL rc=-11' $(BUILD)/$(1).serial \
	&& grep -qxF 'FINDER-DELETE-ALERT name=README.TXT' $(BUILD)/$(1).serial \
	&& [ "$$($(call te-delete-text,$(BUILD)/$(1).serial))" = '$(TE_DELETE_MSG)' ] \
	&& ! grep -q '^FINDER-DELETE-NOW' $(BUILD)/$(1).serial \
	&& mtype -i $(BUILD)/$(1)_data.img ::README.TXT | cmp -s - $(FLAIR_DATA_README)
TE_FULL_DELETE = grep -qxF 'FINDER-DELETE-ALERT name=README.TXT' $(BUILD)/$(1).serial \
	&& awk '/^FINDER-DELETE-NOW name=README.TXT purged=1$$/{d=1;next} d&&/^TRASH-CREATE$$/{c=1;next} c&&/^FINDER-OPEN-TRASH win=1 n=0 singleton=0$$/{ok=1} END{exit !ok}' $(BUILD)/$(1).serial \
	&& ! mdir -a -i $(BUILD)/$(1)_data.img ::README.TXT >/dev/null 2>&1 \
	&& mdir -a -i $(BUILD)/$(1)_data.img :: | grep -qE '^TRASH +<DIR>' \
	&& [ -z "$$(mdir -b -i $(BUILD)/$(1)_data.img ::/TRASH 2>/dev/null)" ] \
	&& mtype -i $(BUILD)/$(1)_data.img ::FILL.BIN | cmp -s - $(BUILD)/finder_full_fill.bin \
	&& $(call te-fsck-clean,$(BUILD)/$(1)_data.img)
TE_NOTRASH = grep -qxF 'FINDER-WIN-OPEN-FAIL rc=-13' $(BUILD)/$(1).serial \
	&& grep -qxF 'FINDER-ALERT reason=notrash' $(BUILD)/$(1).serial \
	&& [ "$$($(call te-alert-text,$(BUILD)/$(1).serial,FINDER-ALERT reason=notrash))" = '$(TE_NOTRASH_MSG)' ] \
	&& ! grep -q '^FINDER-ALERT-LINE The item could not be moved' $(BUILD)/$(1).serial
TE_PUTAWAY = grep -qxF 'FINDER-OPEN-TRASH win=1 n=2 singleton=0' $(BUILD)/$(1).serial \
	&& grep -qxF 'FINDER-SELECT-ALL win=1 n=2' $(BUILD)/$(1).serial \
	&& grep -qxF 'FINDER-PUT-AWAY n=2' $(BUILD)/$(1).serial \
	&& mtype -i $(BUILD)/$(1)_data.img ::README.TXT | cmp -s - $(FLAIR_DATA_README) \
	&& mtype -i $(BUILD)/$(1)_data.img ::AUTOEXEC.BAT | cmp -s - $(BUILD)/trash_edge_autoexec.bat \
	&& [ -z "$$(mdir -b -i $(BUILD)/$(1)_data.img ::/TRASH 2>/dev/null)" ] \
	&& $(call te-fsck-clean,$(BUILD)/$(1)_data.img)
TE_NEWFOLDER = [ "$$(grep -c '^FINDER-NEW-FOLDER ' $(BUILD)/$(1).serial)" = 1 ] \
	&& grep -qxF 'FINDER-NEW-FOLDER name=NEWFOLD parent=0' $(BUILD)/$(1).serial \
	&& mdir -i $(BUILD)/$(1)_data.img :: | grep -qE '^NEWFOLD +<DIR>' \
	&& ! mdir -a -b -i $(BUILD)/$(1)_data.img ::/TRASH 2>/dev/null | grep -q NEWFOLD

.PHONY: test-flair-trash-edges test-flair-trash-edges-mutant
test-flair-trash-edges: $(HARNESS_BIN) $(FLAIRTENANTS_HOLD_IMG) $(FINDER_FULL_DATA) $(FLAIR_DATA_IMG) $(PPM_FLAIR_TRASH_CHECK_BIN) $(PPM_FLAIR_ICONS_CHECK_BIN)
	@printf 'InitechOS (STAPLER) -- make test-flair-trash-edges : Finder Trash edges (bead initech-vnl4; audit 2026-10-23 desktop L001/L006/L007/L008)\n'
	@command -v mdir >/dev/null 2>&1 && command -v mtype >/dev/null 2>&1 && command -v fsck.fat >/dev/null 2>&1 || { printf '!!! test-flair-trash-edges FAIL: mtools / fsck.fat (dosfstools) missing\n'; exit 1; }
	@mtype -i $(FLAIR_DATA_IMG) ::AUTOEXEC.BAT > $(BUILD)/trash_edge_autoexec.bat
	$(call appl-boot,trash_edge_full_alert,$(FLAIRTENANTS_HOLD_IMG),$(FINDER_FULL_DATA),FLAIR_TRASH_EDGE_FULL_ALERT_SPEC,--screendump --screendump-after "FINDER-DELETE-ALERT name=README.TXT")
	@$(call APPL_ALIVE,$(BUILD)/trash_edge_full_alert.serial) || { printf '!!! test-flair-trash-edges [1] FAIL: the guest did not finish alive\n'; exit 1; }
	@$(call TE_FULL_ALERT,trash_edge_full_alert) || { grep -E '^(TRASH|FINDER|DESK)' $(BUILD)/trash_edge_full_alert.serial; printf '!!! test-flair-trash-edges [1] FAIL: the full-volume drop did not put up the Delete Now confirm (or something was removed before OK)\n'; exit 1; }
	@$(PPM_FLAIR_TRASH_CHECK_BIN) chrome $(BUILD)/trash_edge_full_alert.ppm || { printf '!!! test-flair-trash-edges [1] FAIL: the Delete Now confirm is not the two-button alert chrome on screen\n'; exit 1; }
	@$(PPM_FLAIR_ICONS_CHECK_BIN) "$(ICON_CAPTURES_DIR)" $(BUILD)/trash_edge_full_alert.ppm $(FLAIR_CAUTION_CELL) || { printf '!!! test-flair-trash-edges [1] FAIL: the Delete Now confirm does not carry the captured CAUTION icon\n'; exit 1; }
	@printf '>>> test-flair-trash-edges [1/4]: full volume, no Trash -> the Delete Now confirm (chrome + CAUTION icon == the captures); README.TXT untouched\n'
	$(call appl-boot,trash_edge_full_delete,$(FLAIRTENANTS_HOLD_IMG),$(FINDER_FULL_DATA),FLAIR_TRASH_EDGE_FULL_DELETE_SPEC,)
	@$(call APPL_ALIVE,$(BUILD)/trash_edge_full_delete.serial) || { printf '!!! test-flair-trash-edges [2] FAIL: the guest did not finish alive\n'; exit 1; }
	@$(call TE_FULL_DELETE,trash_edge_full_delete) || { grep -E '^(TRASH|FINDER|DESK)' $(BUILD)/trash_edge_full_delete.serial; fsck.fat -n $(BUILD)/trash_edge_full_delete_data.img | tail -3; printf '!!! test-flair-trash-edges [2] FAIL: OK did not remove README.TXT in place, reserve \\TRASH and open it (mtools/fsck)\n'; exit 1; }
	@printf '>>> test-flair-trash-edges [2/4]: OK -> README.TXT removed in place, \\TRASH made in the freed cluster and opened; mtools + fsck.fat clean\n'
	$(call appl-boot,trash_edge_notrash,$(FLAIRTENANTS_HOLD_IMG),$(FINDER_FULL_DATA),FLAIR_TRASH_EDGE_NOTRASH_OPEN_SPEC,--screendump --screendump-after "FINDER-ALERT reason=notrash")
	@$(call APPL_ALIVE,$(BUILD)/trash_edge_notrash.serial) || { printf '!!! test-flair-trash-edges [3] FAIL: the guest did not finish alive\n'; exit 1; }
	@$(call TE_NOTRASH,trash_edge_notrash) || { grep -E '^(TRASH|FINDER)' $(BUILD)/trash_edge_notrash.serial; printf '!!! test-flair-trash-edges [3] FAIL: opening the unavailable Trash did not name the Trash\n'; exit 1; }
	@$(PPM_FLAIR_TRASH_CHECK_BIN) notice $(BUILD)/trash_edge_notrash.ppm || { printf '!!! test-flair-trash-edges [3] FAIL: the notice is not on screen\n'; exit 1; }
	@printf '>>> test-flair-trash-edges [3/4]: the unavailable Trash -> "%s"\n' '$(TE_NOTRASH_MSG)'
	$(call appl-boot,trash_edge_putaway,$(FLAIRTENANTS_HOLD_IMG),$(FLAIR_DATA_IMG),FLAIR_TRASH_EDGE_PUTAWAY_SPEC,)
	@$(call APPL_ALIVE,$(BUILD)/trash_edge_putaway.serial) || { printf '!!! test-flair-trash-edges [4] FAIL: the guest did not finish alive\n'; exit 1; }
	@$(call TE_PUTAWAY,trash_edge_putaway) || { grep -E '^(TRASH|FINDER)' $(BUILD)/trash_edge_putaway.serial; printf '!!! test-flair-trash-edges [4] FAIL: Put Away did not restore BOTH selected items (mtools)\n'; exit 1; }
	@$(call TE_NEWFOLDER,trash_edge_putaway) || { grep -E '^(FINDER-NEW|FINDER-CMD)' $(BUILD)/trash_edge_putaway.serial; printf '!!! test-flair-trash-edges [4] FAIL: New Folder is not dark in the Trash / live in the root\n'; exit 1; }
	@printf '>>> test-flair-trash-edges [4/4]: Put Away n=2 (both home, byte-identical); New Folder dark in the Trash, live in the root (NEWFOLD parent=0 only)\n'
	@printf '>>> test-flair-trash-edges: green\n'

$(eval $(call flair-tenants-kmain-mutant-rules,FLAIR_TEN_TICK_BUDGET=1500 -DKMAIN_MUT_NO_DELETE_NOW,te_no_delete_now))
$(BUILD)/finder_windows_mut_te_notrash.o: spec/assets/finder_list_titles.h os/flair/finder_windows.c os/flair/finder_windows_cold.c os/flair/finder_windows_private.h os/flair/finder_windows.h os/flair/finder_desktop.h os/flair/finder_cmd.h os/flair/finder_icon.h os/flair/window.h os/flair/winscroll.h os/flair/process.h os/flair/heap.h os/flair/blitter.h os/flair/surface.h os/flair/flair_look.h spec/assets/desk_icons.h spec/assets/finder_icons.h spec/assets/chicago12.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/event_model.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_TENANTS_OPT) -DFINDER_MUT_NOTRASH_GENERIC -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -DFINDER_OVL_RESIDENT -c os/flair/finder_windows.c -o $@
$(BUILD)/finder_ops_mut_te_putaway.o: os/flair/finder_ops.c os/flair/finder_ops_cold.c os/flair/finder_ops_private.h os/flair/finder_ops.h os/flair/finder_windows.h os/flair/finder_desktop.h os/flair/window.h os/flair/winscroll.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(FLAIR_FINDER_OPS_OPT) -DFINDER_MUT_PUTAWAY_REPOPULATE -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -DFINDER_OVL_RESIDENT -c os/flair/finder_ops.c -o $@
$(BUILD)/finder_cmd_mut_te_newfolder.o: os/flair/finder_cmd.c os/flair/finder_cmd.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_TENANTS_OPT) -DFINDER_CMD_MUT_NEWFOLDER_IN_TRASH -Ios/flair -c os/flair/finder_cmd.c -o $@
$(eval $(call flair-tenants-objswap-mutant-rules,$(KERNEL_FLAIRTENANTS_MAIN_OBJ) $(KERNEL_FINDER_WINDOWS_OBJ),$(BUILD)/kmain_flairtenants_mut_hold.o $(BUILD)/finder_windows_mut_te_notrash.o,te_notrash))
$(eval $(call flair-tenants-objswap-mutant-rules,$(KERNEL_FLAIRTENANTS_MAIN_OBJ) $(KERNEL_FINDER_OPS_OBJ),$(BUILD)/kmain_flairtenants_mut_hold.o $(BUILD)/finder_ops_mut_te_putaway.o,te_putaway))
$(eval $(call flair-tenants-objswap-mutant-rules,$(KERNEL_FLAIRTENANTS_MAIN_OBJ) $(KERNEL_FINDER_CMD_OBJ),$(BUILD)/kmain_flairtenants_mut_hold.o $(BUILD)/finder_cmd_mut_te_newfolder.o,te_newfolder))

test-flair-trash-edges-mutant: test-flair-trash-edges $(BUILD)/flair_tenants_mut_te_no_delete_now.img $(BUILD)/flair_tenants_mut_te_notrash.img $(BUILD)/flair_tenants_mut_te_putaway.img $(BUILD)/flair_tenants_mut_te_newfolder.img
	$(call appl-boot,te_mut_no_delete_now,$(BUILD)/flair_tenants_mut_te_no_delete_now.img,$(FINDER_FULL_DATA),FLAIR_TRASH_EDGE_FULL_DELETE_SPEC,)
	@$(call APPL_ALIVE,$(BUILD)/te_mut_no_delete_now.serial) || { printf '!!! te_mut_no_delete_now did not finish alive (not comparable)\n'; exit 1; }
	@if $(call TE_FULL_DELETE,te_mut_no_delete_now); then printf '!!! test-flair-trash-edges-mutant FAIL: KMAIN_MUT_NO_DELETE_NOW survived\n'; exit 1; fi
	@grep -qxF 'FINDER-MOVE-REFUSED reason=err name=README.TXT' $(BUILD)/te_mut_no_delete_now.serial && mdir -i $(BUILD)/te_mut_no_delete_now_data.img ::README.TXT >/dev/null 2>&1 \
		|| { printf '!!! test-flair-trash-edges-mutant FAIL: KMAIN_MUT_NO_DELETE_NOW went RED for the wrong reason\n'; exit 1; }
	@printf '>>> KMAIN_MUT_NO_DELETE_NOW: RED -- the pre-fix dead end (FINDER-MOVE-REFUSED reason=err; README.TXT still on the full volume)\n'
	$(call appl-boot,te_mut_notrash,$(BUILD)/flair_tenants_mut_te_notrash.img,$(FINDER_FULL_DATA),FLAIR_TRASH_EDGE_NOTRASH_OPEN_SPEC,)
	@$(call APPL_ALIVE,$(BUILD)/te_mut_notrash.serial) || { printf '!!! te_mut_notrash did not finish alive (not comparable)\n'; exit 1; }
	@if $(call TE_NOTRASH,te_mut_notrash); then printf '!!! test-flair-trash-edges-mutant FAIL: FINDER_MUT_NOTRASH_GENERIC survived\n'; exit 1; fi
	@grep -q '^FINDER-ALERT-LINE The item could not be moved' $(BUILD)/te_mut_notrash.serial \
		|| { printf '!!! test-flair-trash-edges-mutant FAIL: FINDER_MUT_NOTRASH_GENERIC went RED for the wrong reason\n'; exit 1; }
	@printf '>>> FINDER_MUT_NOTRASH_GENERIC: RED -- the open of the Trash said "The item could not be moved..."\n'
	$(call appl-boot,te_mut_putaway,$(BUILD)/flair_tenants_mut_te_putaway.img,$(FLAIR_DATA_IMG),FLAIR_TRASH_EDGE_PUTAWAY_SPEC,)
	@$(call APPL_ALIVE,$(BUILD)/te_mut_putaway.serial) || { printf '!!! te_mut_putaway did not finish alive (not comparable)\n'; exit 1; }
	@if $(call TE_PUTAWAY,te_mut_putaway); then printf '!!! test-flair-trash-edges-mutant FAIL: FINDER_MUT_PUTAWAY_REPOPULATE survived\n'; exit 1; fi
	@grep -qxF 'FINDER-PUT-AWAY n=1' $(BUILD)/te_mut_putaway.serial \
		|| { printf '!!! test-flair-trash-edges-mutant FAIL: FINDER_MUT_PUTAWAY_REPOPULATE went RED for the wrong reason\n'; exit 1; }
	@printf '>>> FINDER_MUT_PUTAWAY_REPOPULATE: RED -- FINDER-PUT-AWAY n=1 of a two-item selection (the audit L006 symptom)\n'
	$(call appl-boot,te_mut_newfolder,$(BUILD)/flair_tenants_mut_te_newfolder.img,$(FLAIR_DATA_IMG),FLAIR_TRASH_EDGE_PUTAWAY_SPEC,)
	@$(call APPL_ALIVE,$(BUILD)/te_mut_newfolder.serial) || { printf '!!! te_mut_newfolder did not finish alive (not comparable)\n'; exit 1; }
	@if $(call TE_NEWFOLDER,te_mut_newfolder); then printf '!!! test-flair-trash-edges-mutant FAIL: FINDER_CMD_MUT_NEWFOLDER_IN_TRASH survived\n'; exit 1; fi
	@grep -E '^FINDER-NEW-FOLDER name=NEWFOLD parent=[1-9]' $(BUILD)/te_mut_newfolder.serial >/dev/null \
		|| { printf '!!! test-flair-trash-edges-mutant FAIL: FINDER_CMD_MUT_NEWFOLDER_IN_TRASH went RED for the wrong reason\n'; exit 1; }
	@printf '>>> FINDER_CMD_MUT_NEWFOLDER_IN_TRASH: RED -- %s (a folder made inside the Trash)\n' "$$(grep -m1 -E '^FINDER-NEW-FOLDER name=NEWFOLD parent=[1-9]' $(BUILD)/te_mut_newfolder.serial)"
	@printf '>>> test-flair-trash-edges-mutant: green (4 named mutants RED for their reasons)\n'
TEST_EMU_GATES += test-flair-trash-edges test-flair-trash-edges-mutant
