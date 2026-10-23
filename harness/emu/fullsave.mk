# initech-mcep: a save that fails on a FULL data volume keeps the last saved
# file. Ref: audits docs/audits/2026-10-23-iword L001/L002 (InitechWord),
# -das L003 (InitechText), -i123 L001 (Initech 123); the fix is the tenant
# runtime's atomic save os/apps/tbx/tbx_save.h over spec/toolbox_gate.h
# Sec 10b (TBX_FILE_RENAME, DOS 3.3 AH=56h). Traces: spec/flair_fullsave_
# traces.mk. Volumes follow the initech-a5sr fixture-contract pattern
# (harness/fixture_volumes.mk declares FLAIR_FULLSAVE_DATA's contents).
# (spec/flair_fullsave_traces.mk is included by harness/emu/notepad.mk, first.)

# Gate-local mutant tenants: TBX_MUT_SAVE_IN_PLACE is the pre-mcep save
# (CREATE truncates the target, then the WRITE runs out of disk).
$(eval $(call tenant-exe-rules,iword_save_inplace,$(IWORD_APP_SRC),-Ios/iword/core -DTBX_MUT_SAVE_IN_PLACE,$(IWORD_APP_DEP),))
$(eval $(call tenant-exe-rules,itext_save_inplace,$(ITEXT_APP_SRC),$(ITEXT_APP_CF) -DTBX_MUT_SAVE_IN_PLACE,$(ITEXT_APP_DEP),--gc-sections))
# The collision mutant (reviewer 2026-10-23): the temporary is a plain CREATE.
$(eval $(call tenant-exe-rules,itext_save_tempcreate,$(ITEXT_APP_SRC),$(ITEXT_APP_CF) -DTBX_MUT_SAVE_TEMP_CREATE,$(ITEXT_APP_DEP),--gc-sections))
$(eval $(call tenant-exe-rules,i123_save_inplace,$(I123_APP_SRC),-Ios/i123/core -DTBX_MUT_SAVE_IN_PLACE,$(I123_APP_DEP),))

FULLSAVE_MINT_WP5 := $(WORDPERFECT51_DECOMP)/goldens/minted/MINT02.WP5
FULLSAVE_MINT_WK1 := $(LOTUS123_DECOMP)/goldens/minted/MINT01.WK1
FULLSAVE_ORIG_WP5 := $(BUILD)/fullsave-ORIGINAL.WP5
FULLSAVE_LONG_WP5 := $(BUILD)/fullsave-LONG.WP5
FULLSAVE_KEEP_TXT := $(BUILD)/fullsave-KEEP.TXT
FULLSAVE_KEEP_WK1 := $(BUILD)/fullsave-KEEP.WK1
# A file ALREADY under KEEP.TXT's first temporary name (a user's file, or the
# remnant of an interrupted save): the save must take KEEP.$$0 instead and
# leave it byte-identical. (Seeded and judged here, not in the a5sr contract:
# a '$' cannot travel through that contract's unquoted shell list.)
FULLSAVE_REM_NAME := KEEP.$$$$$$
FULLSAVE_REM      := $(BUILD)/fullsave-KEEP.REM
# The seeded originals: ORIGINAL.WP5 = the real MINT02 prefix + one line
# (the audit's evidence/prepare_full.py shape, 373 bytes); LONG.WP5 = the
# same prefix + 2,352 bytes of text (2,679 bytes, 6 clusters); KEEP.TXT / KEEP.WK1 are
# one-cluster files that are never read, only replaced (byte-compared).
$(FULLSAVE_ORIG_WP5) $(FULLSAVE_LONG_WP5) $(FULLSAVE_KEEP_TXT) $(FULLSAVE_KEEP_WK1) $(FULLSAVE_REM) &: $(FULLSAVE_MINT_WP5) harness/emu/fullsave.mk | $(BUILD)
	@head -c 327 $(FULLSAVE_MINT_WP5) > $(FULLSAVE_ORIG_WP5)
	@printf 'ORIGINAL SAFE TEXT. DO NOT LOSE THIS VERSION.\n' >> $(FULLSAVE_ORIG_WP5)
	@head -c 327 $(FULLSAVE_MINT_WP5) > $(FULLSAVE_LONG_WP5)
	@i=0; while [ $$i -lt 48 ]; do printf 'Saving tables to disk. This is the long version. '; i=$$((i+1)); done >> $(FULLSAVE_LONG_WP5)
	@printf 'KEEP.TXT: the last saved version of this document.\r' > $(FULLSAVE_KEEP_TXT)
	@printf 'KEEP.WK1: the last saved worksheet bytes.' > $(FULLSAVE_KEEP_WK1)
	@printf 'A file that was already named like a temporary.\r' > $(FULLSAVE_REM)
	@test "$$(wc -c < $(FULLSAVE_ORIG_WP5))" = 373 && test "$$(wc -c < $(FULLSAVE_LONG_WP5))" = 2679 \
		|| { printf '!!! fullsave seeds: wrong sizes\n'; rm -f $(FULLSAVE_ORIG_WP5) $(FULLSAVE_LONG_WP5); exit 1; }

# $(call fullsave-volume,<image>,<WORD.EXE>,<TEXT.EXE>,<123.EXE>): the
# flagship volume (with the given tenants), the seeds, then FILLER.BIN sized
# so EXACTLY two 512-byte clusters stay free (the desktop's DESKTOP.DB and
# TRASH). Every traced save needs >= 4 clusters with its target's 1 freed.
FULLSAVE_MCOPY = SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy
FULLSAVE_FREE = mdir -i $(1) :: | sed -n 's/^ *\([0-9][0-9 ]*\) bytes free.*/\1/p' | tr -d ' '
define fullsave-volume
$(1): $(FLAIR_DATA_IMG) $(2) $(3) $(4) $(FULLSAVE_MINT_WK1) $(FULLSAVE_ORIG_WP5) $(FULLSAVE_LONG_WP5) $(FULLSAVE_KEEP_TXT) $(FULLSAVE_KEEP_WK1) $(FULLSAVE_REM) harness/emu/fullsave.mk
	@$$(call fixture-preflight,$(FLAIR_DATA_IMG))
	cp -f $(FLAIR_DATA_IMG) $$@
	@$(FULLSAVE_MCOPY) -o -i $$@ $(2) ::APPS/WORD.EXE
	@$(FULLSAVE_MCOPY) -o -i $$@ $(3) ::APPS/TEXT.EXE
	@$(FULLSAVE_MCOPY) -o -i $$@ $(4) ::APPS/123.EXE
	@$(FULLSAVE_MCOPY) -i $$@ $(FULLSAVE_ORIG_WP5) ::ORIGINAL.WP5
	@$(FULLSAVE_MCOPY) -i $$@ $(FULLSAVE_LONG_WP5) ::LONG.WP5
	@$(FULLSAVE_MCOPY) -i $$@ $(FULLSAVE_KEEP_TXT) ::KEEP.TXT
	@$(FULLSAVE_MCOPY) -i $$@ $(FULLSAVE_KEEP_WK1) ::KEEP.WK1
	@$(FULLSAVE_MCOPY) -i $$@ $(FULLSAVE_MINT_WK1) ::MINT01.WK1
	@$(FULLSAVE_MCOPY) -i $$@ $(FULLSAVE_REM) '::$$(FULLSAVE_REM_NAME)' && mdir -b -i $$@ '::$$(FULLSAVE_REM_NAME)' | grep -qxF '::/$$(FULLSAVE_REM_NAME)' \
		|| { printf '!!! fullsave volume %s: the temporary-name remnant was not seeded\n' '$$@'; rm -f $$@; exit 1; }
	@free=$$$$($(call FULLSAVE_FREE,$$@)); [ -n "$$$$free" ] && [ "$$$$free" -gt 1024 ] || { printf '!!! fullsave volume %s: no free figure\n' '$$@'; rm -f $$@; exit 1; }; \
		head -c $$$$((free - 1024)) /dev/zero > $$@.filler; $(FULLSAVE_MCOPY) -i $$@ $$@.filler ::FILLER.BIN; rm -f $$@.filler; \
		left=$$$$($(call FULLSAVE_FREE,$$@)); [ "$$$$left" = 1024 ] || { printf '!!! fullsave volume %s: %s bytes free, want 1024\n' '$$@' "$$$$left"; rm -f $$@; exit 1; }
	@printf '>>> fullsave volume: %s (flagship + ORIGINAL.WP5 LONG.WP5 KEEP.TXT KEEP.WK1 MINT01.WK1 %s + FILLER.BIN; 1024 bytes free)\n' '$$@' '$$(FULLSAVE_REM_NAME)'
	@$$(call fixture-check,recipe,$$@)
endef
FLAIR_FULLSAVE_DATA := $(BUILD)/flair_fullsave_data.img
FLAIR_FULLSAVE_MUT_DATA := $(BUILD)/flair_fullsave_mut_data.img
FLAIR_FULLSAVE_COLLIDE_DATA := $(BUILD)/flair_fullsave_collide_data.img
$(eval $(call fullsave-volume,$(FLAIR_FULLSAVE_DATA),$(IWORD_EXE),$(ITEXT_EXE),$(I123_EXE)))
$(eval $(call fullsave-volume,$(FLAIR_FULLSAVE_COLLIDE_DATA),$(IWORD_EXE),$(BUILD)/tenant/itext_save_tempcreate.exe,$(I123_EXE)))
$(eval $(call fullsave-volume,$(FLAIR_FULLSAVE_MUT_DATA),$(BUILD)/tenant/iword_save_inplace.exe,$(BUILD)/tenant/itext_save_inplace.exe,$(BUILD)/tenant/i123_save_inplace.exe))

# Leg checks over a run named $(1) (serial $(BUILD)/$(1).serial, stopped
# volume $(BUILD)/$(1)_data.img). UP/TRIED/ERR must hold in BOTH polarities
# (the mutant is RED for the disk reason only); KEPT is the property.
FS_S = $(BUILD)/$(1).serial
FS_D = $(BUILD)/$(1)_data.img
# the save really ran: a CREATE (0x0080) or CREATE_NEW (0x008a, Sec 10b) of
# the target or of a temporary of it
FS_TRIED = grep -Eiq '^TENANT-GATE ax=0x008[0a] path="$(2)\.(\$$\$$[$$0-9]|$(3))" ' $(call FS_S,$(1))
# the volume really was full when the run stopped (<= 2 clusters free)
FS_FULL = [ "$$($(call FULLSAVE_FREE,$(call FS_D,$(1))))" -le 1024 ]
# the named file is byte-identical to its seed; no .$$$ temporary is left
FS_SAME = mcopy -o -i $(call FS_D,$(1)) ::$(2) $(BUILD)/$(1)-$(2) && cmp -s $(3) $(BUILD)/$(1)-$(2)
# the only '$' name left is the seeded remnant (no temporary of this run)
FS_NOTEMP = [ "$$(mdir -b -i $(call FS_D,$(1)) :: | grep -F '$$' | tr -d '\n')" = '::/$(FULLSAVE_REM_NAME)' ]
FS_ABSENT = ! mdir -b -i $(call FS_D,$(1)) ::$(2) >/dev/null 2>&1

FS_IWORD_UP = $(call APPL_ALIVE,$(call FS_S,$(1))) && $(call IWORD_UP,$(call FS_S,$(1))) && \
	grep -qF 's="ERROR: Disk Full--Press Any Key to Continue' $(call FS_S,$(1)) && \
	$(call FS_TRIED,$(1),ORIGINAL,WP5) && $(call FS_TRIED,$(1),NEW,WP5) && $(call FS_FULL,$(1))
FS_IWORD_KEPT = $(call FS_SAME,$(1),ORIGINAL.WP5,$(FULLSAVE_ORIG_WP5)) && $(call FS_ABSENT,$(1),NEW.WP5) && $(call FS_NOTEMP,$(1))
FS_ITEXT_UP = $(call APPL_ALIVE,$(call FS_S,$(1))) && $(call ITEXT_UP,$(call FS_S,$(1))) && \
	grep -qx 'TENANT-ALERT-LINE The document could not be saved.' $(call FS_S,$(1)) && \
	$(call FS_TRIED,$(1),KEEP,TXT) && $(call FS_FULL,$(1))
FS_ITEXT_KEPT = $(call FS_SAME,$(1),KEEP.TXT,$(FULLSAVE_KEEP_TXT)) && $(call FS_NOTEMP,$(1)) && \
	mcopy -o -i $(call FS_D,$(1)) '::$(FULLSAVE_REM_NAME)' $(BUILD)/$(1)-KEEP.REM && cmp -s $(FULLSAVE_REM) $(BUILD)/$(1)-KEEP.REM
FS_I123_UP = $(call APPL_ALIVE,$(call FS_S,$(1))) && grep -qx 'TENANT-REGISTER ok app=123' $(call FS_S,$(1)) && \
	grep -qF 's="Cancel  Replace  Backup' $(call FS_S,$(1)) && grep -qF 's="File I/O error' $(call FS_S,$(1)) && \
	$(call FS_TRIED,$(1),keep,wk1) && $(call FS_FULL,$(1))
FS_I123_KEPT = $(call FS_SAME,$(1),KEEP.WK1,$(FULLSAVE_KEEP_WK1)) && $(call FS_NOTEMP,$(1))

FS_DEPS = $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG)
.PHONY: test-flair-fullsave test-flair-fullsave-mutant
test-flair-fullsave: $(FS_DEPS) $(FLAIR_FULLSAVE_DATA)
	$(call need_wp_goldens,test-flair-fullsave)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-flair-fullsave : a save that fails on a full disk keeps the last saved file (initech-mcep)\n'
	@printf '======================================================================\n'
	$(call appl-boot,fullsave_iword,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_FULLSAVE_DATA),FLAIR_FULLSAVE_IWORD_SPEC,)
	@$(call FS_IWORD_UP,fullsave_iword) || { printf '!!! test-flair-fullsave [1/3] FAIL: IWORD -- WORD up, both saves tried, Disk Full shown, the volume full\n'; grep -E '^(TENANT-(GATE ax=0x008|LOAD|REG|CRASH)|PANIC)' $(call FS_S,fullsave_iword) | tail -20; exit 1; }
	@$(call FS_IWORD_KEPT,fullsave_iword) || { printf '!!! test-flair-fullsave [1/3] FAIL: IWORD -- ORIGINAL.WP5 changed (%s bytes, seed 373) / NEW.WP5 left behind / a .$$$$$$ temporary left (mtools)\n' "$$(wc -c < $(BUILD)/fullsave_iword-ORIGINAL.WP5)"; mdir -i $(call FS_D,fullsave_iword) ::; exit 1; }
	@fsck.fat -n $(call FS_D,fullsave_iword) > /dev/null || { printf '!!! test-flair-fullsave [1/3] FAIL: IWORD -- fsck.fat\n'; exit 1; }
	@printf '>>> test-flair-fullsave [1/3] IWORD: Replace ORIGINAL.WP5 and new NEW.WP5 both "ERROR: Disk Full"; ORIGINAL.WP5 byte-identical (373 B), no NEW.WP5, no .$$$$$$ (mtools); fsck clean\n'
	$(call appl-boot,fullsave_itext,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_FULLSAVE_DATA),FLAIR_FULLSAVE_ITEXT_SPEC,)
	@$(call FS_ITEXT_UP,fullsave_itext) || { printf '!!! test-flair-fullsave [2/3] FAIL: ITEXT -- TEXT up, the save tried, the alert shown, the volume full\n'; grep -E '^(TENANT-(GATE ax=0x008|LOAD|REG|CRASH|ALERT)|PANIC)' $(call FS_S,fullsave_itext) | tail -20; exit 1; }
	@$(call FS_ITEXT_KEPT,fullsave_itext) || { printf '!!! test-flair-fullsave [2/3] FAIL: ITEXT -- KEEP.TXT changed (%s bytes) / a .$$$$$$ temporary left (mtools)\n' "$$(wc -c < $(BUILD)/fullsave_itext-KEEP.TXT)"; mdir -i $(call FS_D,fullsave_itext) ::; exit 1; }
	@fsck.fat -n $(call FS_D,fullsave_itext) > /dev/null || { printf '!!! test-flair-fullsave [2/3] FAIL: ITEXT -- fsck.fat\n'; exit 1; }
	@printf '>>> test-flair-fullsave [2/3] ITEXT: Save As KEEP.TXT + Replace (2,048 chars) "could not be saved"; KEEP.TXT and the pre-existing KEEP.$$$$$$ byte-identical, no temporary left (mtools); fsck clean\n'
	$(call appl-boot,fullsave_i123,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_FULLSAVE_DATA),FLAIR_FULLSAVE_I123_SPEC,)
	@$(call FS_I123_UP,fullsave_i123) || { printf '!!! test-flair-fullsave [3/3] FAIL: I123 -- 123 up, Replace chosen, File I/O error shown, the volume full\n'; grep -E '^(TENANT-(GATE ax=0x008|LOAD|REG|CRASH)|PANIC)' $(call FS_S,fullsave_i123) | tail -20; exit 1; }
	@$(call FS_I123_KEPT,fullsave_i123) || { printf '!!! test-flair-fullsave [3/3] FAIL: I123 -- KEEP.WK1 changed (%s bytes) / a .$$$$$$ temporary left (mtools)\n' "$$(wc -c < $(BUILD)/fullsave_i123-KEEP.WK1)"; mdir -i $(call FS_D,fullsave_i123) ::; exit 1; }
	@fsck.fat -n $(call FS_D,fullsave_i123) > /dev/null || { printf '!!! test-flair-fullsave [3/3] FAIL: I123 -- fsck.fat\n'; exit 1; }
	@printf '>>> test-flair-fullsave [3/3] I123: /File Save keep + Replace over the retrieved MINT01 "File I/O error"; KEEP.WK1 byte-identical, no .$$$$$$ (mtools); fsck clean\n'
	@printf '>>> test-flair-fullsave: green\n'

# Rule 6: the pre-mcep in-place save (TBX_MUT_SAVE_IN_PLACE) on the same
# full volume -- each leg must be UP (the save tried, the error shown) and
# RED for the disk reason: the original no longer matches its seed.
define fullsave-mut-leg
	$(call appl-boot,fullsave_mut_$(1),$(FLAIR_IWORD_GATE_IMG),$(FLAIR_FULLSAVE_MUT_DATA),FLAIR_FULLSAVE_$(2)_SPEC,)
	@$(call FS_$(2)_UP,fullsave_mut_$(1)) || { printf '!!! test-flair-fullsave-mutant FAIL: SAVE_IN_PLACE/$(2) went RED for the wrong reason (want: up, the save tried, the error shown, the volume full)\n'; exit 1; }
	@if $(call FS_$(2)_KEPT,fullsave_mut_$(1)); then printf '!!! test-flair-fullsave-mutant FAIL: SAVE_IN_PLACE/$(2) kept the original -- the leg is decoration\n'; exit 1; fi
	@printf '>>> test-flair-fullsave-mutant: SAVE_IN_PLACE correctly RED on $(2): %s is now %s bytes\n' '$(3)' "$$(wc -c < $(BUILD)/fullsave_mut_$(1)-$(3))"
endef
test-flair-fullsave-mutant: test-flair-fullsave $(FS_DEPS) $(FLAIR_FULLSAVE_MUT_DATA) $(FLAIR_FULLSAVE_COLLIDE_DATA)
	$(call fullsave-mut-leg,iword,IWORD,ORIGINAL.WP5)
	$(call fullsave-mut-leg,itext,ITEXT,KEEP.TXT)
	$(call fullsave-mut-leg,i123,I123,KEEP.WK1)
	@# COLLIDE (TBX_MUT_SAVE_TEMP_CREATE): KEEP.TXT survives, but the file
	@# already named KEEP.$$$$$$ is truncated by the plain CREATE and deleted.
	$(call appl-boot,fullsave_mut_collide,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_FULLSAVE_COLLIDE_DATA),FLAIR_FULLSAVE_ITEXT_SPEC,)
	@$(call FS_ITEXT_UP,fullsave_mut_collide) && $(call FS_SAME,fullsave_mut_collide,KEEP.TXT,$(FULLSAVE_KEEP_TXT)) || { printf '!!! test-flair-fullsave-mutant FAIL: TEMP_CREATE went RED for the wrong reason (want: TEXT up, the save tried, KEEP.TXT kept)\n'; exit 1; }
	@if $(call FS_ITEXT_KEPT,fullsave_mut_collide); then printf '!!! test-flair-fullsave-mutant FAIL: TEMP_CREATE kept the remnant -- the collision leg is decoration\n'; exit 1; fi
	@! mdir -b -i $(call FS_D,fullsave_mut_collide) '::$(FULLSAVE_REM_NAME)' > /dev/null 2>&1 || { printf '!!! test-flair-fullsave-mutant FAIL: TEMP_CREATE: wrong RED reason (the remnant is still there)\n'; exit 1; }
	@printf '>>> test-flair-fullsave-mutant: TEMP_CREATE correctly RED: the file already named %s was destroyed\n' '$(FULLSAVE_REM_NAME)'
	@printf '>>> test-flair-fullsave-mutant: green\n'
TEST_EMU_GATES += test-flair-fullsave test-flair-fullsave-mutant
