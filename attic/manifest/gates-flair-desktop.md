## test-exec-overlay-flair

Declared tier: outside active vector; #  recipe to execute (from 'harness/exec_overlay.mk', line 226):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/fat_ovl_flair.img build/.makefile.stamp`
```make
$(call ovl-flair-boot,$(FLAIRTENANTS_IMG),exec_overlay_flair)
@for m in FLAIR-SUSPEND $(OVL_GREEN_MARKERS) 'FLAIR-TEXT-EXIT rc=0' FLAIR-RESUME; do \
	grep -qx "$$m" $(BUILD)/exec_overlay_flair.txt || { cat $(BUILD)/exec_overlay_flair.txt; \
	printf '!!! test-exec-overlay-flair RED: marker %s missing\n' "$$m"; exit 1; }; done
@! grep -Eq '^(OVL-FAIL|PANIC)' $(BUILD)/exec_overlay_flair.txt
@printf '>>> test-exec-overlay-flair: green (flagship GUI text tenant loads, calls and refuses overlays, then resumes)\n'
```

## test-exec-overlay-flair-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/exec_overlay.mk', line 234):
Prerequisites: `test-exec-overlay-flair build/flair_tenants_mut_ovl_unbound.img build/.makefile.stamp`
```make
$(call ovl-flair-boot,$(BUILD)/flair_tenants_mut_ovl_unbound.img,exec_overlay_flair_mut_unbound)
@grep -qx 'OVL-FAIL mz-load' $(BUILD)/exec_overlay_flair_mut_unbound.txt \
	&& grep -qx 'FLAIR-TEXT-EXIT rc=1' $(BUILD)/exec_overlay_flair_mut_unbound.txt \
	&& ! grep -qx 'OVL-ALL-OK' $(BUILD)/exec_overlay_flair_mut_unbound.txt \
	|| { cat $(BUILD)/exec_overlay_flair_mut_unbound.txt; printf '!!! overlay backend unbound mutant survived\n'; exit 1; }
@printf '>>> test-exec-overlay-flair-mutant: KMAIN_MUT_OVL_UNBOUND RED (OVL-FAIL mz-load, rc=1)\n'
```

## test-finder-alerts

Declared tier: outside active vector; #  recipe to execute (from 'harness/finder_alerts.mk', line 317):
Prerequisites: `test-finder-input test-dialog-tracking test-finder-limits test-finder-notices test-finder-restore test-finder-replace test-finder-demo test-finder-diskfull test-finder-input-mutant test-dialog-tracking-mutant test-control-press-mutant test-finder-entries-mutant test-finder-window-limit-mutant test-finder-open-all-mutant test-finder-keys-mutant test-finder-disk-alert-mutant test-finder-launch-alert-mutant test-finder-move-alert-mutant test-finder-restore-mutant test-finder-replace-mutant test-finder-demo-mutant test-finder-diskfull-mutant build/.makefile.stamp`
```make
@printf '>>> test-finder-alerts: green (8 host suites; 14 named mutants RED)\n'
```

## test-finder-cmd

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13756):
Prerequisites: `build/test_finder_cmd build/.makefile.stamp`
```make
@printf ">>> test-finder-cmd: R3 Finder command-table spine -- hand-authored FINDER-CMD trace golden (mouse+key, enabled/disabled/unknown) + table structure (F4.4)\n"
@$(TEST_FINDER_CMD)
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -Os $(FINDER_CMD_INC) -c os/flair/finder_cmd.c -o $(BUILD)/finder_cmd_freestanding.o \
	|| { printf '!!! test-finder-cmd FAIL: finder_cmd.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-finder-cmd: green\n"
```

## test-finder-cmd-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13763):
Prerequisites: `build/test_finder_cmd_mutant_table_bypass build/test_finder_cmd_mutant_silent_unknown build/test_finder_cmd_mutant_pred_stuck_enabled build/.makefile.stamp`
```make
@printf ">>> test-finder-cmd-mutant: confirming all three implementation mutants go RED (Rule 6)\n"
@if $(TEST_FINDER_CMD_MUT_BYPASS) >/dev/null 2>&1; then printf '!!! test-finder-cmd-mutant FAIL: TABLE_BYPASS PASSED -- the both-paths-converge trace golden is decoration\n'; exit 1; else printf '>>> test-finder-cmd-mutant: green (TABLE_BYPASS correctly RED -- the src=key FINDER-CMD lines vanish)\n'; fi
@if $(TEST_FINDER_CMD_MUT_SILENT) >/dev/null 2>&1; then printf '!!! test-finder-cmd-mutant FAIL: SILENT_UNKNOWN PASSED -- the fail-loud unknown-command oracle is decoration\n'; exit 1; else printf '>>> test-finder-cmd-mutant: green (SILENT_UNKNOWN correctly RED -- FINDER-CMD-UNKNOWN vanishes)\n'; fi
@if $(TEST_FINDER_CMD_MUT_PRED) >/dev/null 2>&1; then printf '!!! test-finder-cmd-mutant FAIL: PRED_STUCK_ENABLED PASSED -- the enablement-predicate oracle is decoration\n'; exit 1; else printf '>>> test-finder-cmd-mutant: green (PRED_STUCK_ENABLED correctly RED -- disabled commands run their handler)\n'; fi
```

## test-finder-delete-now-mutant

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 14212):
Prerequisites: `build/test_finder_ops_mutant_delete_DELETE_NO_WALK build/test_finder_ops_mutant_delete_DELETE_KEEP_VIEW build/test_finder_ops_mutant_delete_LOCKED_PURGE build/.makefile.stamp`
```make
@set -eu; for pair in 'DELETE_NO_WALK:O18 Delete Now purges the folder DEPTH-FIRST' 'DELETE_KEEP_VIEW:O18 Delete Now retires BOTH the root folder view' 'LOCKED_PURGE:O18 a refused descendant survives'; do \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_finder_ops_mutant_delete_$$m; \
	if $$bin > $$bin.log 2>&1; then printf '!!! test-finder-delete-now-mutant FAIL: %s survived\n' "$$m"; exit 1; fi; \
	grep -F "$$why" $$bin.log | grep -q FAIL || { cat $$bin.log; printf '!!! Delete Now mutant RED for wrong reason: %s\n' "$$m"; exit 1; }; \
	printf '>>> test-finder-delete-now-mutant: %s correctly RED (%s)\n' "$$m" "$$why"; \
done
```

## test-finder-demo

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 30):
Prerequisites: `build/test_finder_demo build/.makefile.stamp`
```make
@$<
```

## test-finder-demo-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 195):
Prerequisites: `build/test_finder_demo_mutant build/.makefile.stamp`
```make
@if $< > $(BUILD)/finder_demo_mutant.log 2>&1; then exit 1; fi
@grep -m1 'FAIL.*DEMO_GRAY' $(BUILD)/finder_demo_mutant.log
@printf '>>> test-finder-demo-mutant: FINDER_MUT_DEMO_LIVE RED\n'
```

## test-finder-desktop

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13820):
Prerequisites: `build/test_finder_desktop build/.makefile.stamp`
```make
@printf ">>> test-finder-desktop: R3.2 DesktopMgr -- default placement, label geometry, hit/selection/marquee/double-click/drag trackers, hand-authored DESKTOP.DB bytes, and the composed-desktop underlay seam\n"
@$(TEST_FINDER_DESK)
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -Os $(FINDER_DESK_INC) -c os/flair/finder_desktop.c -o $(BUILD)/finder_desktop_freestanding.o \
	|| { printf '!!! test-finder-desktop FAIL: finder_desktop.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-finder-desktop: green\n"
```

## test-finder-desktop-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13827):
Prerequisites: `build/test_finder_desktop_mutant_dbltick_off build/test_finder_desktop_mutant_marquee_offbyone build/test_finder_desktop_mutant_no_underlay build/.makefile.stamp`
```make
@printf ">>> test-finder-desktop-mutant: confirming all three DesktopMgr mutants go RED (Rule 6)\n"
@if $(TEST_FINDER_DESK_MUT_DBL) >/dev/null 2>&1; then printf '!!! test-finder-desktop-mutant FAIL: DBLTICK_OFF PASSED -- the double-click synthesis oracle is decoration\n'; exit 1; else printf '>>> test-finder-desktop-mutant: green (DBLTICK_OFF correctly RED -- every double degrades to two singles)\n'; fi
@if $(TEST_FINDER_DESK_MUT_BAND) >/dev/null 2>&1; then printf '!!! test-finder-desktop-mutant FAIL: MARQUEE_OFFBYONE PASSED -- the hand-authored marquee selected sets are decoration\n'; exit 1; else printf '>>> test-finder-desktop-mutant: green (MARQUEE_OFFBYONE correctly RED -- the band loses its own edge pixel)\n'; fi
@if $(TEST_FINDER_DESK_MUT_UNDERLAY) >/dev/null 2>&1; then printf '!!! test-finder-desktop-mutant FAIL: NO_UNDERLAY PASSED -- the composed-desktop render leg is decoration\n'; exit 1; else printf '>>> test-finder-desktop-mutant: green (NO_UNDERLAY correctly RED -- the icons never reach the frame)\n'; fi
```

## test-finder-disk-alert-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 186):
Prerequisites: `build/finder_host_disk-alert build/.makefile.stamp`
```make
@if $< > build/finder_host_disk-alert.log 2>&1; then printf '!!! FINDER_MUT_NO_DISK_ALERT survived\n'; exit 1; fi
@grep 'FAIL.*The disk is full' build/finder_host_disk-alert.log
@printf '>>> test-finder-disk-alert-mutant: FINDER_MUT_NO_DISK_ALERT RED\n'
```

## test-finder-diskfull

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 25):
Prerequisites: `build/finder_diskfull_host.img build/.makefile.stamp`
```make
@$(BUILD)/test_finder_diskfull $<
```

## test-finder-diskfull-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 201):
Prerequisites: `build/test_finder_diskfull_mutant build/.makefile.stamp`
```make
@dd if=/dev/zero of=$(BUILD)/finder_diskfull_mut.img bs=512 count=2880 status=none
@mkfs.fat -F 12 $(BUILD)/finder_diskfull_mut.img >/dev/null
@if $< $(BUILD)/finder_diskfull_mut.img > $(BUILD)/finder_diskfull_mutant.log 2>&1; then exit 1; fi
@grep 'FAIL.*NO_PHANTOM' $(BUILD)/finder_diskfull_mutant.log
@printf '>>> test-finder-diskfull-mutant: DB_MUT_PHANTOM RED\n'
```

## test-finder-entries-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 182):
Prerequisites: `build/finder_host_entries build/.makefile.stamp`
```make
@if $< > build/finder_host_entries.log 2>&1; then printf '!!! FINDER_MUT_ICON64 survived\n'; exit 1; fi
@grep 'FAIL.*ICONS_HEAP' build/finder_host_entries.log
@printf '>>> test-finder-entries-mutant: FINDER_MUT_ICON64 RED\n'
```

## test-finder-icon-art

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13588):
Prerequisites: `build/test_finder_icon_art build/.makefile.stamp`
```make
@$(TEST_ICON_ART) "$(ICON_CAPTURES_DIR)"
```

## test-finder-icon-art-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13591):
Prerequisites: `build/test_finder_icon_art_mut_0 build/test_finder_icon_art_mut_1 build/test_finder_icon_art_mut_2 build/test_finder_icon_art_mut_3 build/test_finder_icon_art_mut_4 build/test_finder_icon_art_mut_5 build/test_finder_icon_art_mut_6 build/test_finder_icon_art_mut_7 build/test_finder_icon_art_mut_noselect build/.makefile.stamp`
```make
@printf ">>> test-finder-icon-art-mutant: one strike row off, per strike (Rule 6)\n"
@k=0; for n in $(TEST_ICON_ART_NAMES); do \
	pre=FLAIR_DESK_ICON; case $$n in FOLDER|DOC|APP) pre=FLAIR_FINDER_ICON;; CAUTION) pre=FLAIR_ALERT_ICON;; esac; \
	rc=0; $(BUILD)/test_finder_icon_art_mut_$$k "$(ICON_CAPTURES_DIR)" > $(BUILD)/test_finder_icon_art_mut_$$k.log 2>&1 || rc=$$?; \
	reds=$$(grep -c '^  RED ' $(BUILD)/test_finder_icon_art_mut_$$k.log); \
	if [ "$$rc" -eq 1 ] && grep -q "^  RED $${pre}_$$n UNSEL" $(BUILD)/test_finder_icon_art_mut_$$k.log && \
	   [ "$$(grep '^  RED ' $(BUILD)/test_finder_icon_art_mut_$$k.log | grep -vc " $${pre}_$$n ")" = 0 ]; then \
		printf '>>> ICON_ART_MUT_STRIKE=%s: RED on %s_%s only (%s grade(s)) -- %s\n' "$$k" "$$pre" "$$n" "$$reds" "$$(grep 'MUTANT' $(BUILD)/test_finder_icon_art_mut_$$k.log | sed 's/^ *MUTANT (Rule 6): //')"; \
	else cat $(BUILD)/test_finder_icon_art_mut_$$k.log; printf '!!! test-finder-icon-art-mutant FAIL: strike %s one row off did not go RED on that strike alone\n' "$$n"; exit 1; fi; \
	k=$$((k + 1)); done
@rc=0; $(BUILD)/test_finder_icon_art_mut_noselect "$(ICON_CAPTURES_DIR)" > $(BUILD)/test_finder_icon_art_mut_noselect.log 2>&1 || rc=$$?; \
[ "$$rc" -eq 1 ] && [ "$$(grep -c '^  RED .* SEL$$' $(BUILD)/test_finder_icon_art_mut_noselect.log)" = 7 ] \
	|| { cat $(BUILD)/test_finder_icon_art_mut_noselect.log; printf '!!! test-finder-icon-art-mutant FAIL: DESK_ICON_MUT_NO_SELECT did not turn all 7 selected grades RED\n'; exit 1; }; \
printf '>>> DESK_ICON_MUT_NO_SELECT: RED -- all 7 selected-state grades miss the captures\n'
@printf ">>> test-finder-icon-art-mutant: green\n"
```

## test-finder-icon-oracles

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 21846):
Prerequisites: `build/test_finder_icon_oracles build/.makefile.stamp`
```make
@$(TEST_ICON_ORACLES) spec/dos_folder_apps.txt $(FLAIR_ICONS_SCENE_CELLS)
```

## test-finder-icon-oracles-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 21848):
Prerequisites: `build/test_finder_icon_oracles build/test_finder_icon_oracles_mut_unselected build/.makefile.stamp`
```make
@rc=0; $(TEST_ICON_ORACLES)_mut_unselected spec/dos_folder_apps.txt $(FLAIR_ICONS_SCENE_CELLS) > $(TEST_ICON_ORACLES)_mut_unselected.log 2>&1 || rc=$$?; \
[ "$$rc" = 1 ] && grep -q '^FAIL FULL-state routing locked-full: selected' $(TEST_ICON_ORACLES)_mut_unselected.log && grep -q 'RED (1 failures)' $(TEST_ICON_ORACLES)_mut_unselected.log \
|| { cat $(TEST_ICON_ORACLES)_mut_unselected.log; exit 1; }; \
printf '>>> TRASH_CHECK_MUT_LOCKED_UNSELECTED: RED -- captured selected FULL can rejected (617/1024)\n'
@rc=0; $(TEST_ICON_ORACLES) spec/dos_folder_apps.txt $(ICON_SCENE_MUT_STALE_DOC) > $(TEST_ICON_ORACLES)_mut_stale_doc.log 2>&1 || rc=$$?; \
[ "$$rc" = 1 ] && grep -q '^FAIL scene APPS TPSPARM.DAT:' $(TEST_ICON_ORACLES)_mut_stale_doc.log && grep -q 'RED (1 failures)' $(TEST_ICON_ORACLES)_mut_stale_doc.log \
|| { cat $(TEST_ICON_ORACLES)_mut_stale_doc.log; exit 1; }; \
printf '>>> ICON_SCENE_MUT_STALE_DOC: RED -- DOC probe no longer names TPSPARM.DAT in the independent APPS inventory\n'
@printf '>>> test-finder-icon-oracles-mutant: green\n'
```

## test-finder-input

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 43):
Prerequisites: `build/test_finder_input build/.makefile.stamp`
```make
@$<
```

## test-finder-input-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 51):
Prerequisites: `build/test_finder_input_mutant build/.makefile.stamp`
```make
@if $< > $(BUILD)/finder_input_mutant.log 2>&1; then printf '!!! CLICK_THROUGH survived\n'; exit 1; fi
@grep 'FAIL.*ACTIVATE_ONLY consumes' $(BUILD)/finder_input_mutant.log
@printf '>>> test-finder-input-mutant: PROC_MUT_CLICK_THROUGH RED\n'
```

## test-finder-keys-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 185):
Prerequisites: `build/finder_host_keys build/.makefile.stamp`
```make
@if $< > build/finder_host_keys.log 2>&1; then printf '!!! FINDER_MUT_NO_KEYS survived\n'; exit 1; fi
@grep 'FAIL.*KEY_SELECT' build/finder_host_keys.log
@printf '>>> test-finder-keys-mutant: FINDER_MUT_NO_KEYS RED\n'
```

## test-finder-launch-alert-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 187):
Prerequisites: `build/finder_host_launch-alert build/.makefile.stamp`
```make
@if $< > build/finder_host_launch-alert.log 2>&1; then printf '!!! FINDER_MUT_NO_LAUNCH_ALERT survived\n'; exit 1; fi
@grep 'FAIL.*unexpectedly quit' build/finder_host_launch-alert.log
@printf '>>> test-finder-launch-alert-mutant: FINDER_MUT_NO_LAUNCH_ALERT RED\n'
```

## test-finder-limits

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 6):
Prerequisites: `build/test_finder_limits build/.makefile.stamp`
```make
@$<
```

## test-finder-list-view

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13916):
Prerequisites: `build/test_finder_list_view test-finder-small-icons-gen build/.makefile.stamp`
```make
@$(TEST_FINDER_LIST)
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -Os $(FINDER_WIN_INC) -DFINDER_OVL_RESIDENT -c os/flair/finder_windows.c -o $(BUILD)/finder_list_view_freestanding.o
@printf '>>> test-finder-list-view: green (routing, captured sort policy, columns, kind=4)\n'
```

## test-finder-list-view-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13920):
Prerequisites: `test-finder-list-view build/test_finder_list_view_mutant_SORT_UNSTABLE build/test_finder_list_view_mutant_SIZE_COLUMN_OFFBYONE build/test_finder_list_view_mutant_HEADER_IGNORED build/test_finder_list_view_mutant_SAVE_ICONS build/.makefile.stamp`
```make
@set -eu; for pair in 'SORT_UNSTABLE:V2 stable rank' 'SIZE_COLUMN_OFFBYONE:V3 column boundaries' 'HEADER_IGNORED:V4 Date header click' 'SAVE_ICONS:V5 Restart kind=4 bytes'; do \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_finder_list_view_mutant_$$m; \
	if $$bin > $$bin.log 2>&1; then printf '!!! test-finder-list-view-mutant FAIL: %s PASSED\n' "$$m"; exit 1; fi; \
	grep -F "$$why" $$bin.log | grep -q FAIL || { cat $$bin.log; printf '!!! mutant RED for wrong reason: %s\n' "$$m"; exit 1; }; \
	grep -F "$$why" $$bin.log | head -1; \
	printf '>>> test-finder-list-view-mutant: %s correctly RED\n' "$$m"; \
done
@printf '>>> test-finder-list-view-mutant: green (four named mutants RED)\n'
```

## test-finder-lock-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 30577):
Prerequisites: `build/test_finder_ops_mutant_LOCKED_STAGE build/test_finder_ops_mutant_locked_purge build/.makefile.stamp`
```make
@set -e; for pair in 'LOCKED_STAGE:L118 locked staging refused from live attributes' 'locked_purge:L118 late lock refused during mixed purge'; do \
  tag=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_finder_ops_mutant_$$tag; \
  if $$bin > $$bin.log 2>&1; then echo "FAIL: $$tag mutant passed"; exit 1; fi; \
  grep -F "$$why" $$bin.log | grep -q FAIL || { cat $$bin.log; exit 1; }; \
  printf 'VERDICT: PASS -- Finder mutant %s RED (%s)\n' "$$tag" "$$why"; \
done
```

## test-finder-menu

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15644):
Prerequisites: `build/test_finder_menu build/.makefile.stamp`
```make
@printf ">>> test-finder-menu: R3.5 Finder menu-bar resource -- hand-authored F4.2 structure + title columns + the graying truth table + MenuKey routing (F4.2/F4.4)\n"
@$(TEST_FINDER_MENU)
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -Os $(FINDER_MENU_INC) -c os/flair/finder_menu.c -o $(BUILD)/finder_menu_freestanding.o \
	|| { printf '!!! test-finder-menu FAIL: finder_menu.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-finder-menu: green\n"
```

## test-finder-menu-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15651):
Prerequisites: `build/test_finder_menu_mutant_enable_stuck build/test_finder_menu_mutant_cmdchar_dup build/test_finder_menu_mutant_dead_item build/.makefile.stamp`
```make
@printf ">>> test-finder-menu-mutant: confirming all three resource mutants go RED (Rule 6)\n"
@if $(TEST_FINDER_MENU_MUT_STUCK) >/dev/null 2>&1; then printf '!!! test-finder-menu-mutant FAIL: ENABLE_STUCK PASSED -- the graying truth table is decoration\n'; exit 1; else printf '>>> test-finder-menu-mutant: green (ENABLE_STUCK correctly RED -- the bar never grays)\n'; fi
@if $(TEST_FINDER_MENU_MUT_DUP) >/dev/null 2>&1; then printf '!!! test-finder-menu-mutant FAIL: CMDCHAR_DUP PASSED -- the command-key structure oracle is decoration\n'; exit 1; else printf '>>> test-finder-menu-mutant: green (CMDCHAR_DUP correctly RED -- a duplicate Cmd-N hides a command)\n'; fi
@if $(TEST_FINDER_MENU_MUT_DEAD) >/dev/null 2>&1; then printf '!!! test-finder-menu-mutant FAIL: DEAD_ITEM PASSED -- the rest-state enable oracle is decoration\n'; exit 1; else printf '>>> test-finder-menu-mutant: green (DEAD_ITEM correctly RED -- a functional item shipped unselectable; the SAME knob is the emu mutant of test-flair-disk-windows-mutant)\n'; fi
```

## test-finder-move-alert-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 188):
Prerequisites: `build/finder_host_move-alert build/.makefile.stamp`
```make
@if $< > build/finder_host_move-alert.log 2>&1; then printf '!!! FINDER_MUT_NO_MOVE_ALERT survived\n'; exit 1; fi
@grep 'FAIL.*inside itself' build/finder_host_move-alert.log
@printf '>>> test-finder-move-alert-mutant: FINDER_MUT_NO_MOVE_ALERT RED\n'
```

## test-finder-notices

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 11):
Prerequisites: `build/test_finder_notices build/.makefile.stamp`
```make
@$<
```

## test-finder-open-all-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 184):
Prerequisites: `build/finder_host_open-all build/.makefile.stamp`
```make
@if $< > build/finder_host_open-all.log 2>&1; then printf '!!! FINDER_MUT_OPEN_ONE survived\n'; exit 1; fi
@grep 'FAIL.*OPEN_ALL' build/finder_host_open-all.log
@printf '>>> test-finder-open-all-mutant: FINDER_MUT_OPEN_ONE RED\n'
```

## test-finder-ops

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14187):
Prerequisites: `build/test_finder_ops build/.makefile.stamp`
```make
@printf ">>> test-finder-ops: R3.4a drag-move + drag-to-Trash -- targets, highlight pixels, moves, the refusal ladder, Trash staging + suffix, kind=5 origins\n"
@$(TEST_FINDER_OPS)
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -Os $(FINDER_WIN_INC) -DFINDER_OVL_RESIDENT -c os/flair/finder_ops.c -o $(BUILD)/finder_ops_freestanding.o \
	|| { printf '!!! test-finder-ops FAIL: finder_ops.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-finder-ops: green\n"
```

## test-finder-ops-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14195):
Prerequisites: `test-finder-delete-now-mutant build/test_finder_ops_mutant_TRASH_NO_STAGE build/test_finder_ops_mutant_NO_CYCLE build/test_finder_ops_mutant_NO_ORIGIN build/test_finder_ops_mutant_NO_SERVICE_GUARD build/test_finder_ops_mutant_COUNT_CELL build/test_finder_ops_mutant_REMOVE_NO_META build/test_finder_ops_mutant_DROP_NO_META build/test_finder_ops_mutant_hilite build/test_finder_ops_mutant_abs build/test_finder_ops_mutant_arrange build/test_finder_ops_mutant_scrollpaint build/test_finder_ops_mutant_trashroot build/test_finder_ops_mutant_noconfirm build/test_finder_ops_mutant_powernosave build/.makefile.stamp`
```make
@for pair in 'TRASH_NO_STAGE:O6 the volume holds README.TXT inside' 'NO_SERVICE_GUARD:O12 the root TRASH dropped on APPS is refused SERVICE' 'NO_CYCLE:O5 a folder dropped into ITSELF is refused CYCLE' 'NO_ORIGIN:O6 a kind=5 origin' 'hilite:O2 highlighted FACE pixel' 'abs:O9 paint: APPS is drawn at the moved cell' 'arrange:O10 View > Arrange (by Name) ran' 'scrollpaint:O11 hit: a click on F18 where it is DRAWN' 'trashroot:O13 the Trash window lists exactly the staged README.TXT' 'COUNT_CELL:O13 the root window shows it in the lowest FREE cell' 'noconfirm:O14 dispatch alone deletes nothing' 'powernosave:O15 Special > Shut Down runs in the shell and notes 2' 'REMOVE_NO_META:O16 every SURVIVOR keeps its own metadata' 'DROP_NO_META:O17 arrival metadata matches the LIVE directory record'; do \
	m=$${pair%%:*}; why=$${pair#*:}; \
	bin=$(BUILD)/test_finder_ops_mutant_$$m; \
	if $$bin > $$bin.log 2>&1; then printf '!!! test-finder-ops-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
	grep -F "$$why" $$bin.log | grep -q FAIL || { printf '!!! test-finder-ops-mutant FAIL: %s went RED for the wrong reason\n' "$$m"; cat $$bin.log; exit 1; }; \
	printf '>>> test-finder-ops-mutant: %s correctly RED (%s)\n' "$$m" "$$why"; \
done
@printf '>>> test-finder-ops-mutant: green (all fourteen drop/command mutants RED for the named reason)\n'
```

## test-finder-replace

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 36):
Prerequisites: `build/test_finder_replace build/.makefile.stamp`
```make
@$<
```

## test-finder-replace-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 190):
Prerequisites: `build/finder_host_replace build/.makefile.stamp`
```make
@if $< > build/finder_host_replace.log 2>&1; then printf '!!! FINDER_MUT_REPLACE_CANCEL survived\n'; exit 1; fi
@grep 'FAIL.*REPLACE confirmed' build/finder_host_replace.log
@printf '>>> test-finder-replace-mutant: FINDER_MUT_REPLACE_CANCEL RED\n'
```

## test-finder-restore

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 16):
Prerequisites: `build/test_finder_restore build/.makefile.stamp`
```make
@$<
```

## test-finder-restore-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 189):
Prerequisites: `build/finder_host_restore build/.makefile.stamp`
```make
@if $< > build/finder_host_restore.log 2>&1; then printf '!!! FINDER_MUT_ORIGINAL_LOST survived\n'; exit 1; fi
@grep 'FAIL.*ORIGINAL_NAME encoded origin pair' build/finder_host_restore.log
@printf '>>> test-finder-restore-mutant: FINDER_MUT_ORIGINAL_LOST RED\n'
```

## test-finder-small-icons-gen

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 13911):
Prerequisites: `build/mint_finder_small_icons build/.makefile.stamp`
```make
@$< $(SYSTEM7_DECOMP)/goldens/captures/s8_finder_list_view.png $(SYSTEM7_DECOMP)/goldens/captures/s8_finder_list_view_bysize.png > $(BUILD)/finder_small_icons.regen.h
@cmp spec/assets/finder_small_icons.h $(BUILD)/finder_small_icons.regen.h
@printf '>>> test-finder-small-icons-gen: green (paired-background native16 mask recovery)\n'
```

## test-finder-window-limit-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 183):
Prerequisites: `build/finder_host_window-limit build/.makefile.stamp`
```make
@if $< > build/finder_host_window-limit.log 2>&1; then printf '!!! FINDER_MUT_WINDOW4 survived\n'; exit 1; fi
@grep 'FAIL.*WINDOWS_HEAP' build/finder_host_window-limit.log
@printf '>>> test-finder-window-limit-mutant: FINDER_MUT_WINDOW4 RED\n'
```

## test-finder-windows

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13936):
Prerequisites: `build/test_finder_windows build/.makefile.stamp`
```make
@printf ">>> test-finder-windows: R3.3 disk windows -- kind heuristic, VOLLABEL skip, the invisible grid, the LOCKED New Folder ladder, enumeration through a scribbling mock binding, spatial singleton + caps, row-major Clean Up, the kind=4 DESKTOP.DB record and view persistence\n"
@$(TEST_FINDER_WIN)
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -Os $(FINDER_WIN_INC) -DFINDER_OVL_RESIDENT -c os/flair/finder_windows.c -o $(BUILD)/finder_windows_freestanding.o \
	|| { printf '!!! test-finder-windows FAIL: finder_windows.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-finder-windows: green\n"
```

## test-finder-windows-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13969):
Prerequisites: `build/test_finder_windows_mutant_singleton_dup build/test_finder_windows_mutant_vollabel_shown build/test_finder_windows_mutant_cleanup_unsorted build/test_finder_windows_mutant_dots_shown build/test_finder_windows_mutant_service_shown build/test_finder_windows_mutant_purge_keeps_views build/test_finder_windows_mutant_view_unchecked build/test_finder_windows_mutant_open_app_dark build/test_finder_windows_mutant_open_no_app build/.makefile.stamp`
```make
@if $(TEST_FINDER_WIN_MUT_OPENDARK) > $(BUILD)/test_finder_windows_mutant_opendark.log 2>&1; then printf '!!! test-finder-windows-mutant FAIL: OPEN_APP_DARK PASSED -- the L004 application-enablement oracle is decoration\n'; exit 1; fi
@grep 'FAIL' $(BUILD)/test_finder_windows_mutant_opendark.log | grep -q 'L13 a selected APPLICATION lights File > Open' || { printf '!!! test-finder-windows-mutant FAIL: OPEN_APP_DARK went RED for the wrong reason\n'; cat $(BUILD)/test_finder_windows_mutant_opendark.log; exit 1; }
@printf '>>> test-finder-windows-mutant: green (OPEN_APP_DARK correctly RED -- a selected application leaves File > Open dark, L004)\n'
@if $(TEST_FINDER_WIN_MUT_OPENNOAPP) > $(BUILD)/test_finder_windows_mutant_opennoapp.log 2>&1; then printf '!!! test-finder-windows-mutant FAIL: OPEN_NO_APP PASSED -- the L004 Open-launches oracle is decoration\n'; exit 1; fi
@grep 'FAIL' $(BUILD)/test_finder_windows_mutant_opennoapp.log | grep -q 'L13 Cmd-O on the application is a successful Open' || { printf '!!! test-finder-windows-mutant FAIL: OPEN_NO_APP went RED for the wrong reason\n'; cat $(BUILD)/test_finder_windows_mutant_opennoapp.log; exit 1; }
@printf '>>> test-finder-windows-mutant: green (OPEN_NO_APP correctly RED -- Open lit for an application names no launch, L004)\n'
@if $(TEST_FINDER_WIN_MUT_KEEPVIEWS) > $(BUILD)/test_finder_windows_mutant_keepviews.log 2>&1; then printf '!!! test-finder-windows-mutant FAIL: PURGE_KEEPS_VIEWS PASSED -- the L002 view-retirement oracle is decoration\n'; exit 1; fi
@grep 'FAIL' $(BUILD)/test_finder_windows_mutant_keepviews.log | grep -q 'L12 every view of a purged folder is CLOSED' || { printf '!!! test-finder-windows-mutant FAIL: PURGE_KEEPS_VIEWS went RED for the wrong reason\n'; cat $(BUILD)/test_finder_windows_mutant_keepviews.log; exit 1; }
@printf '>>> test-finder-windows-mutant: green (PURGE_KEEPS_VIEWS correctly RED -- purged folders keep open windows, L002)\n'
@if $(TEST_FINDER_WIN_MUT_UNCHECKED) > $(BUILD)/test_finder_windows_mutant_unchecked.log 2>&1; then printf '!!! test-finder-windows-mutant FAIL: VIEW_UNCHECKED PASSED -- the L002 view-identity oracle is decoration\n'; exit 1; fi
@grep 'FAIL' $(BUILD)/test_finder_windows_mutant_unchecked.log | grep -q 'L12 New Folder through a STALE view' || { printf '!!! test-finder-windows-mutant FAIL: VIEW_UNCHECKED went RED for the wrong reason\n'; cat $(BUILD)/test_finder_windows_mutant_unchecked.log; exit 1; }
@printf '>>> test-finder-windows-mutant: green (VIEW_UNCHECKED correctly RED -- New Folder writes through a recycled-cluster view, L002)\n'
@if $(TEST_FINDER_WIN_MUT_SVC) > $(BUILD)/test_finder_windows_mutant_svc.log 2>&1; then printf '!!! test-finder-windows-mutant FAIL: SERVICE_SHOWN PASSED -- the bookkeeping-hidden oracle is decoration\n'; exit 1; fi
@grep 'FAIL' $(BUILD)/test_finder_windows_mutant_svc.log | grep -q 'L5b the root lists 4 of 7 entries' || { printf '!!! test-finder-windows-mutant FAIL: SERVICE_SHOWN went RED for the wrong reason\n'; cat $(BUILD)/test_finder_windows_mutant_svc.log; exit 1; }
@printf '>>> test-finder-windows-mutant: green (SERVICE_SHOWN correctly RED -- the root lists TRASH and DESKTOP.DB, tdnl.56)\n'
@if $(TEST_FINDER_WIN_MUT_DOTS) > $(BUILD)/test_finder_windows_mutant_dots.log 2>&1; then printf '!!! test-finder-windows-mutant FAIL: DOTS_SHOWN PASSED -- the dot-entry skip oracle is decoration\n'; exit 1; fi
@grep -q 'skips the "." and ".." entries' $(BUILD)/test_finder_windows_mutant_dots.log || { printf '!!! test-finder-windows-mutant FAIL: DOTS_SHOWN went RED for the wrong reason\n'; cat $(BUILD)/test_finder_windows_mutant_dots.log; exit 1; }
@printf '>>> test-finder-windows-mutant: green (DOTS_SHOWN correctly RED -- a subdirectory lists "." and "..")\n'
@printf ">>> test-finder-windows-mutant: confirming all three disk-window mutants go RED (Rule 6)\n"
@if $(TEST_FINDER_WIN_MUT_SINGLETON) >/dev/null 2>&1; then printf '!!! test-finder-windows-mutant FAIL: SINGLETON_DUP PASSED -- the spatial-singleton oracle is decoration\n'; exit 1; else printf '>>> test-finder-windows-mutant: green (SINGLETON_DUP correctly RED -- reopening a folder builds a second window)\n'; fi
@if $(TEST_FINDER_WIN_MUT_VOLLABEL) >/dev/null 2>&1; then printf '!!! test-finder-windows-mutant FAIL: VOLLABEL_SHOWN PASSED -- the volume-label skip oracle is decoration\n'; exit 1; else printf '>>> test-finder-windows-mutant: green (VOLLABEL_SHOWN correctly RED -- the volume name leaks into the listing)\n'; fi
@if $(TEST_FINDER_WIN_MUT_CLEANUP) >/dev/null 2>&1; then printf '!!! test-finder-windows-mutant FAIL: CLEANUP_UNSORTED PASSED -- the row-major Clean Up oracle is decoration\n'; exit 1; else printf '>>> test-finder-windows-mutant: green (CLEANUP_UNSORTED correctly RED -- the snap order mirrors)\n'; fi
```

## test-flair-alert-chrome

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33992):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_i123_file_gate.img build/flair_i123_quit_fixture.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
$(call appl-boot,flair_alert_chrome,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_QUIT_DATA),FLAIR_I123_QG_ALERT,--screendump --screendump-after "TENANT-ALERT up")
@f=$(BUILD)/flair_alert_chrome.serial; $(call APPL_ALIVE,$$f) && $(call QG_ASKED,$$f) && $(call QG_NO_EXIT,$$f) || { cat $(BUILD)/flair_alert_chrome.report; printf '!!! test-flair-alert-chrome FAIL: dirty 123 quit alert not reached alive\n'; exit 1; }
@$(PPM_FLAIR_TRASH_CHECK_BIN) chrome $(BUILD)/flair_alert_chrome.ppm
@printf '>>> test-flair-alert-chrome: green\n'
```

## test-flair-alert-chrome-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 232):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_i123_file_gate.img build/flair_i123_quit_fixture.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-alert-chrome-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_i123_file_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-alert-chrome-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_i123_quit_fixture.img build/flair-alert-chrome_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_i123_file_gate.img" --disk2 "build/flair-alert-chrome_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-alert-chrome_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-alert-chrome_bochs.report.txt" || true
@cat "$(BUILD)/flair-alert-chrome_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-alert-chrome-bochs "$(BUILD)/flair-alert-chrome_bochs.report.txt" "$(BUILD)/flair-alert-chrome_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-alert-chrome-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34007):
Prerequisites: `test-flair-alert-chrome build/alert_chrome_label.img build/alert_chrome_fringe.img build/alert_chrome_slab.img build/.makefile.stamp`
```make
$(call alert-chrome-mutant-leg,label,BUTTON-LABEL-BEVEL,CTRL_MUT_LABEL_OPAQUE)
$(call alert-chrome-mutant-leg,fringe,BUTTON-CORNERS,CTRL_MUT_BTN_FRINGE)
$(call alert-chrome-mutant-leg,slab,ALERT-BANDS,DIALOG_MUT_ALERT_SLAB)
@printf '>>> test-flair-alert-chrome-mutant: green (all three booted regressions RED)\n'
```

## test-flair-app-launch

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23302):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/flair_data_tfx_noreg.img build/flair_data_tfx_crash.img build/flair_data_tfx_badmbar.img build/ppm_flair_app_launch_check spec/flair_app_launch_trace.golden build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-app-launch : R3.7 app launch from disk\n'
@printf '  bead initech-tdnl.14; reconciliation DEC-AC3-1..4; D1.7 oracle set\n'
@printf '======================================================================\n'
$(call appl-boot,flair_appl_launch,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_APP_LAUNCH_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_appl_launch.serial) || { printf '!!! test-flair-app-launch [1/10] FAIL: the desktop did not survive the launch trace\n'; grep -E '^(PANIC|TENANT-|HALTED)' $(BUILD)/flair_appl_launch.serial; exit 1; }
@$(call APPL_TRACE,$(BUILD)/flair_appl_launch.serial) || { printf '!!! test-flair-app-launch [1/10] FAIL: the launch trace differs from the locked golden %s\n' '$(FLAIR_APPL_GOLDEN)'; diff -u $(BUILD)/flair_appl_launch.serial.golden $(BUILD)/flair_appl_launch.serial.trace; exit 1; }
@printf '>>> test-flair-app-launch [1/10] LAUNCH: %s trace lines byte-identical to the locked golden\n' "$$(wc -l < $(BUILD)/flair_appl_launch.serial.trace)"
$(call appl-boot,flair_appl_show,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_APP_LAUNCH_SHOW_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_appl_show.serial) || { printf '!!! test-flair-app-launch [2/10] FAIL: the desktop did not survive\n'; exit 1; }
@grep -qx 'FLAIR-DISPATCH app=TENANTFX' $(BUILD)/flair_appl_show.serial || { printf '!!! test-flair-app-launch [2/10] FAIL: the tenant never became foreground\n'; exit 1; }
@$(PPM_FLAIR_APPL_CHECK_BIN) show $(BUILD)/flair_appl_show.ppm || { printf '!!! test-flair-app-launch [2/10] FAIL: the resident tenant window is not what the tenant drew\n'; exit 1; }
@$(PPM_FLAIR_APPL_CHECK_BIN) bar-tenant $(BUILD)/flair_appl_show.ppm || { printf '!!! test-flair-app-launch [2/10] FAIL: band 2 is not the OWN menu bar of the foreground tenant (SETMBAR, initech-cnpm)\n'; exit 1; }
@printf '>>> test-flair-app-launch [2/10] SHOW: the window appears at the tenant bounds, front, with exactly its two text runs; band 2 is exactly its own bar\n'
$(call appl-boot,flair_appl_pre,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_APP_LAUNCH_PRE_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_appl_pre.serial) || { printf '!!! test-flair-app-launch [3/10] FAIL: the baseline boot did not survive\n'; exit 1; }
@if grep -q '^TENANT-' $(BUILD)/flair_appl_pre.serial; then printf '!!! test-flair-app-launch [3/10] FAIL: the baseline (single click) LAUNCHED something\n'; exit 1; fi
@cmp -s $(BUILD)/flair_appl_pre.ppm $(BUILD)/flair_appl_launch.ppm || { printf '!!! test-flair-app-launch [3/10] FAIL: the post-exit frame differs from the never-launched baseline (a tenant pixel, a stale foreground or a wrong band-2 bar survived the exit)\n'; cmp -l $(BUILD)/flair_appl_pre.ppm $(BUILD)/flair_appl_launch.ppm | wc -l; exit 1; }
@$(PPM_FLAIR_APPL_CHECK_BIN) bar-finder $(BUILD)/flair_appl_launch.ppm || { printf '!!! test-flair-app-launch [3/10] FAIL: after the exit band 2 is not the Finder bar (the previous foreground bar was not restored)\n'; exit 1; }
@printf '>>> test-flair-app-launch [3/10] RESTORE: the post-exit frame is byte-identical to the never-launched baseline; band 2 is exactly the Finder bar again\n'
$(call appl-boot,flair_appl_double,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_APP_LAUNCH_DOUBLE_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_appl_double.serial) && $(call APPL_DOUBLE,$(BUILD)/flair_appl_double.serial) || { printf '!!! test-flair-app-launch [4/10] FAIL: DOUBLE_LAUNCH -- want exactly one REGISTER, one SLOT-BUSY, one clean exit\n'; grep -E '^(TENANT-(REGISTER|SLOT|EXIT|LOAD)|PANIC)' $(BUILD)/flair_appl_double.serial; exit 1; }
@printf '>>> test-flair-app-launch [4/10] DOUBLE: second launch refused TENANT-SLOT-BUSY; the resident tenant undisturbed and still exits cleanly\n'
$(call appl-boot,flair_appl_cycle,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_APP_LAUNCH_CYCLE_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_appl_cycle.serial) && $(call APPL_CYCLE,$(BUILD)/flair_appl_cycle.serial) || { printf '!!! test-flair-app-launch [5/10] FAIL: EXIT_LEAK -- two cycles must exit cleanly with EQUAL heap avail\n'; grep -E '^(TENANT-(EXIT|HEAPAVAIL|LOAD)|PANIC)' $(BUILD)/flair_appl_cycle.serial; exit 1; }
@printf '>>> test-flair-app-launch [5/10] CYCLE: two launch/exit cycles, %s both times (avail-stable)\n' "$$(grep -m1 '^TENANT-HEAPAVAIL' $(BUILD)/flair_appl_cycle.serial)"
$(call appl-boot,flair_appl_close,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_APP_LAUNCH_CLOSE_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_appl_close.serial) && $(call APPL_CLOSE,$(BUILD)/flair_appl_close.serial) || { printf '!!! test-flair-app-launch [6/10] FAIL: CLOSE_STALE_FOCUS -- the close box must terminate the tenant and the next key must not reach it\n'; grep -E '^(TENANT-|FLAIR-CLOSE|FLAIR-TENANT-EXIT|FLAIR-EVT what=3|PANIC)' $(BUILD)/flair_appl_close.serial; exit 1; }
@printf '>>> test-flair-app-launch [6/10] CLOSE: the close box terminated the tenant; the next keystroke never reached it\n'
$(call appl-boot,flair_appl_noreg,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_TFX_NOREG_IMG),FLAIR_APP_LAUNCH_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_appl_noreg.serial) && $(call APPL_NOREG,$(BUILD)/flair_appl_noreg.serial) || { printf '!!! test-flair-app-launch [7/10] FAIL: an unregistered tenant must install nothing and leave the desktop intact\n'; grep -E '^(TENANT-|PANIC)' $(BUILD)/flair_appl_noreg.serial; exit 1; }
@printf '>>> test-flair-app-launch [7/10] NOREG: entry returned unregistered -> nothing installed, no event delivered, desktop intact (no watchdog, BC-4)\n'
$(call appl-boot,flair_appl_crash,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_TFX_CRASH_IMG),FLAIR_APP_LAUNCH_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_appl_crash.serial) && $(call APPL_CRASH,$(BUILD)/flair_appl_crash.serial) || { printf '!!! test-flair-app-launch [8/10] FAIL: a fault inside the tenant image must be killed, not panic the desktop\n'; grep -E '^(TENANT-|PANIC|HALTED)' $(BUILD)/flair_appl_crash.serial; exit 1; }
@printf '>>> test-flair-app-launch [8/10] CRASH: %s -> FlairProcess_kill; desktop survived\n' "$$(grep -m1 '^TENANT-CRASH' $(BUILD)/flair_appl_crash.serial)"
$(call appl-boot,flair_appl_badmbar,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_TFX_BADMBAR_IMG),FLAIR_APP_LAUNCH_SHOW_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_appl_badmbar.serial) && $(call APPL_BADMBAR,$(BUILD)/flair_appl_badmbar.serial) || { printf '!!! test-flair-app-launch [9/10] FAIL: BADMBAR -- an out-of-image MenuBar must be refused loudly, before AND after the window, the tenant still foreground\n'; grep -E '^(TENANT-|FLAIR-DISPATCH|PANIC)' $(BUILD)/flair_appl_badmbar.serial; exit 1; }
@$(PPM_FLAIR_APPL_CHECK_BIN) bar-photoshop $(BUILD)/flair_appl_badmbar.ppm || { printf '!!! test-flair-app-launch [9/10] FAIL: BADMBAR -- band 2 is not the shell fallback bar (a refused MenuBar reached band 2)\n'; exit 1; }
@printf '>>> test-flair-app-launch [9/10] BADMBAR: both out-of-image MenuBars refused TENANT-SETMBAR-BAD, and again after the window; band 2 is exactly the shell fallback\n'
$(call appl-boot,flair_appl_menu,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_APP_MENUBAR_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_appl_menu.serial) && $(call APPL_MENU,$(BUILD)/flair_appl_menu.serial) || { printf '!!! test-flair-app-launch [10/10] FAIL: MENU -- a band-2 click must drop the TENANT menu 131 (its own resource), select nothing on release, and the tenant must still exit cleanly\n'; grep -E '^(TENANT-|FLAIR-DISPATCH|FLAIR-MENU|PANIC)' $(BUILD)/flair_appl_menu.serial; exit 1; }
@printf '>>> test-flair-app-launch [10/10] MENU: band 2 dropped the tenant menu 131 from its own resource; release selected nothing; clean exit after\n'
@printf '>>> test-flair-app-launch: green\n'
```

## test-flair-app-launch-bochs

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23397):
Prerequisites: `build/bochs_harness build/qemu_harness build/flair_tenants_mut_tbx_smoke.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-app-launch-bochs : Rule 5 CPU-path differential\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-app-launch-bochs FAIL: bochs not found (a skipped oracle is worse than a red one; SKIP_BOCHS=1 to opt out loudly)\n'; exit 1; }
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(BUILD)/flair_appl_smoke_qemu_data.img
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(BUILD)/flair_appl_smoke_qemu_data.img,$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(BUILD)/flair_appl_smoke_bochs_data.img
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(BUILD)/flair_appl_smoke_bochs_data.img,$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIR_APPL_SMOKE_IMG)" --disk2 "$(BUILD)/flair_appl_smoke_qemu_data.img" --expect HALTED \
	--name flair_appl_smoke_qemu --out "$(BUILD)" --timeout-ms 20000 2> $(BUILD)/flair_appl_smoke_qemu.report || true
@$(BOCHS_BIN) --disk "$(FLAIR_APPL_SMOKE_IMG)" --disk2 "$(BUILD)/flair_appl_smoke_bochs_data.img" --expect HALTED \
	--name flair_appl_smoke_bochs --out "$(BUILD)" --timeout-ms 60000 2> $(BUILD)/flair_appl_smoke_bochs.report || true
@if grep -q 'triple_fault=1' $(BUILD)/flair_appl_smoke_qemu.report $(BUILD)/flair_appl_smoke_bochs.report; then printf '!!! test-flair-app-launch-bochs FAIL: TRIPLE FAULT\n'; exit 1; fi
@for e in qemu bochs; do \
	f=$(BUILD)/flair_appl_smoke_$$e.serial; \
	grep -qx 'TBX-SMOKE-OK list-empty' $$f || { printf '!!! test-flair-app-launch-bochs FAIL: %s did not complete the smoke with an empty process list\n' $$e; grep -E '^(TBX-|TENANT-|PANIC|HALTED)' $$f; exit 1; }; \
	if grep -q '^PANIC' $$f; then printf '!!! test-flair-app-launch-bochs FAIL: %s PANIC\n' $$e; exit 1; fi; \
	grep -E '^TENANT-' $$f > $$f.trace; \
done
@grep -qx 'TENANT-EXIT rc=0 via=exit' $(BUILD)/flair_appl_smoke_qemu.serial.trace || { printf '!!! test-flair-app-launch-bochs FAIL: the QEMU reference smoke did not exit cleanly\n'; exit 1; }
@cmp -s $(BUILD)/flair_appl_smoke_qemu.serial.trace $(BUILD)/flair_appl_smoke_bochs.serial.trace || { printf '!!! test-flair-app-launch-bochs FAIL: QEMU and Bochs disagree on the tenant CPU path (stop condition: investigate, do not pin to QEMU)\n'; diff -u $(BUILD)/flair_appl_smoke_qemu.serial.trace $(BUILD)/flair_appl_smoke_bochs.serial.trace; exit 1; }
@printf '>>> test-flair-app-launch-bochs: %s TENANT-* lines byte-identical on QEMU and Bochs (load, gate, register, update, mouseDown, exit)\n' "$$(wc -l < $(BUILD)/flair_appl_smoke_qemu.serial.trace)"
@printf '>>> test-flair-app-launch-bochs: green\n'
```

## test-flair-app-launch-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23354):
Prerequisites: `test-flair-app-launch build/qemu_harness build/flair_tenants.img build/flair_data.img build/flair_data_tfx_noreg.img build/flair_data_tfx_badmbar.img spec/flair_app_launch_trace.golden build/flair_tenants_mut_tbx_no_slot_guard.img build/flair_tenants_mut_tbx_exit_leak.img build/flair_tenants_mut_tenant_close_hide.img build/flair_tenants_mut_tbx_mbar_not_swapped.img build/flair_tenants_mut_tbx_mbar_no_bounds.img build/.makefile.stamp`
```make
$(call appl-boot,flair_applmut_noreg,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_TFX_NOREG_IMG),FLAIR_APP_LAUNCH_SPEC,)
@if $(call APPL_TRACE,$(BUILD)/flair_applmut_noreg.serial); then printf '!!! test-flair-app-launch-mutant FAIL: NO_REGISTER matched the golden -- the trace oracle is decoration\n'; exit 1; fi
@if grep -q '^TENANT-REGISTER ok' $(BUILD)/flair_applmut_noreg.serial; then printf '!!! test-flair-app-launch-mutant FAIL: NO_REGISTER went RED for the wrong reason (a REGISTER happened)\n'; exit 1; fi
@$(call APPL_ALIVE,$(BUILD)/flair_applmut_noreg.serial) || { printf '!!! test-flair-app-launch-mutant FAIL: NO_REGISTER took the desktop down (it must just never appear)\n'; exit 1; }
@printf '>>> test-flair-app-launch-mutant: NO_REGISTER RED for the named reason -- no TENANT-REGISTER, the golden trace diverges, desktop intact\n'
$(call appl-boot,flair_applmut_double,$(BUILD)/flair_tenants_mut_tbx_no_slot_guard.img,$(FLAIR_DATA_IMG),FLAIR_APP_LAUNCH_DOUBLE_SPEC,)
@if $(call APPL_DOUBLE,$(BUILD)/flair_applmut_double.serial); then printf '!!! test-flair-app-launch-mutant FAIL: DOUBLE_LAUNCH (TBX_MUT_NO_SLOT_GUARD) PASSED the one-slot leg\n'; exit 1; fi
@[ "$$(grep -c '^TENANT-REGISTER ok app=TENANTFX$$' $(BUILD)/flair_applmut_double.serial)" -ge 2 ] && ! grep -q '^TENANT-SLOT-BUSY' $(BUILD)/flair_applmut_double.serial || { printf '!!! test-flair-app-launch-mutant FAIL: DOUBLE_LAUNCH went RED for the wrong reason\n'; grep -E '^(TENANT-(REGISTER|SLOT|LOAD)|PANIC)' $(BUILD)/flair_applmut_double.serial; exit 1; }
@printf '>>> test-flair-app-launch-mutant: DOUBLE_LAUNCH RED for the named reason -- a second REGISTER installed over the resident tenant, no TENANT-SLOT-BUSY\n'
$(call appl-boot,flair_applmut_leak,$(BUILD)/flair_tenants_mut_tbx_exit_leak.img,$(FLAIR_DATA_IMG),FLAIR_APP_LAUNCH_CYCLE_SPEC,)
@if $(call APPL_CYCLE,$(BUILD)/flair_applmut_leak.serial); then printf '!!! test-flair-app-launch-mutant FAIL: EXIT_LEAK PASSED the avail-stable leg\n'; exit 1; fi
@a1=$$(grep '^TENANT-HEAPAVAIL n=' $(BUILD)/flair_applmut_leak.serial | sed -n '1s/.*n=//p'); a2=$$(grep '^TENANT-HEAPAVAIL n=' $(BUILD)/flair_applmut_leak.serial | sed -n '2s/.*n=//p'); \
	[ -n "$$a1" ] && [ -n "$$a2" ] && [ "$$a2" -lt "$$a1" ] || { printf '!!! test-flair-app-launch-mutant FAIL: EXIT_LEAK went RED for the wrong reason (avail did not drift DOWN: %s -> %s)\n' "$$a1" "$$a2"; exit 1; }; \
	printf '>>> test-flair-app-launch-mutant: EXIT_LEAK RED for the named reason -- TENANT-HEAPAVAIL drifted %s -> %s (the code block leaks each cycle)\n' "$$a1" "$$a2"
$(call appl-boot,flair_applmut_close,$(BUILD)/flair_tenants_mut_tenant_close_hide.img,$(FLAIR_DATA_IMG),FLAIR_APP_LAUNCH_CLOSE_SPEC,)
@if $(call APPL_CLOSE,$(BUILD)/flair_applmut_close.serial); then printf '!!! test-flair-app-launch-mutant FAIL: CLOSE_STALE_FOCUS PASSED the anti-8fhu leg\n'; exit 1; fi
@! grep -q '^TENANT-EXIT' $(BUILD)/flair_applmut_close.serial && grep -qx 'TENANT-SLOT-BUSY name=TENANTFX.EXE' $(BUILD)/flair_applmut_close.serial || { printf '!!! test-flair-app-launch-mutant FAIL: CLOSE_STALE_FOCUS went RED for the wrong reason (want: no teardown, and the relaunch refused by the zombie)\n'; grep -E '^(TENANT-|FLAIR-CLOSE|PANIC)' $(BUILD)/flair_applmut_close.serial; exit 1; }
@printf '>>> test-flair-app-launch-mutant: CLOSE_STALE_FOCUS RED for the named reason -- the hide-only close left the tenant resident (no TENANT-EXIT) and the relaunch hit TENANT-SLOT-BUSY on the windowless zombie\n'
$(call appl-boot,flair_applmut_mbarswap,$(BUILD)/flair_tenants_mut_tbx_mbar_not_swapped.img,$(FLAIR_DATA_IMG),FLAIR_APP_LAUNCH_SHOW_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@if $(PPM_FLAIR_APPL_CHECK_BIN) bar-tenant $(BUILD)/flair_applmut_mbarswap.ppm >/dev/null 2>&1; then printf '!!! test-flair-app-launch-mutant FAIL: MBAR_NOT_SWAPPED PASSED the band-2 leg -- the bar oracle is decoration\n'; exit 1; fi
@$(call APPL_ALIVE,$(BUILD)/flair_applmut_mbarswap.serial) && grep -qx 'TENANT-GATE ax=0x0050 bar=+0x00fc -> 0' $(BUILD)/flair_applmut_mbarswap.serial && grep -qx 'FLAIR-DISPATCH app=TENANTFX' $(BUILD)/flair_applmut_mbarswap.serial && $(PPM_FLAIR_APPL_CHECK_BIN) bar-photoshop $(BUILD)/flair_applmut_mbarswap.ppm >/dev/null || { printf '!!! test-flair-app-launch-mutant FAIL: MBAR_NOT_SWAPPED went RED for the wrong reason (want: SETMBAR reported 0, the tenant foreground, band 2 still the shell fallback)\n'; grep -E '^(TENANT-|FLAIR-DISPATCH|PANIC)' $(BUILD)/flair_applmut_mbarswap.serial; exit 1; }
@printf '>>> test-flair-app-launch-mutant: MBAR_NOT_SWAPPED RED for the named reason -- SETMBAR said 0 and the tenant is foreground, but band 2 is still the shell fallback bar\n'
$(call appl-boot,flair_applmut_mbarmenu,$(BUILD)/flair_tenants_mut_tbx_mbar_not_swapped.img,$(FLAIR_DATA_IMG),FLAIR_APP_MENUBAR_SPEC,)
@if $(call APPL_MENU,$(BUILD)/flair_applmut_mbarmenu.serial); then printf '!!! test-flair-app-launch-mutant FAIL: MBAR_NOT_SWAPPED PASSED the MENU leg -- the menu oracle is decoration\n'; exit 1; fi
@grep -qx 'FLAIR-DISPATCH app=TENANTFX' $(BUILD)/flair_applmut_mbarmenu.serial && grep -q '^FLAIR-MENU-DROP menu=' $(BUILD)/flair_applmut_mbarmenu.serial && ! grep -q '^FLAIR-MENU-DROP menu=131$$' $(BUILD)/flair_applmut_mbarmenu.serial || { printf '!!! test-flair-app-launch-mutant FAIL: MBAR_NOT_SWAPPED went RED on the MENU leg for the wrong reason (want: the band-2 click dropped a menu, but NOT the tenant menu 131)\n'; grep -E '^(TENANT-|FLAIR-DISPATCH|FLAIR-MENU|PANIC)' $(BUILD)/flair_applmut_mbarmenu.serial; exit 1; }
@printf '>>> test-flair-app-launch-mutant: MBAR_NOT_SWAPPED RED on the MENU leg too -- the band-2 click dropped %s, not the tenant menu 131\n' "$$(grep -m1 '^FLAIR-MENU-DROP' $(BUILD)/flair_applmut_mbarmenu.serial)"
$(call appl-boot,flair_applmut_mbarbounds,$(BUILD)/flair_tenants_mut_tbx_mbar_no_bounds.img,$(FLAIR_DATA_TFX_BADMBAR_IMG),FLAIR_APP_LAUNCH_SHOW_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@if $(call APPL_BADMBAR,$(BUILD)/flair_applmut_mbarbounds.serial); then printf '!!! test-flair-app-launch-mutant FAIL: MBAR_NO_BOUNDS PASSED the BADMBAR leg\n'; exit 1; fi
@grep -qx 'TENANT-GATE ax=0x0050 bar=+0xfff8 -> 0' $(BUILD)/flair_applmut_mbarbounds.serial && ! grep -q '^TENANT-SETMBAR-BAD why=bar-outside-image' $(BUILD)/flair_applmut_mbarbounds.serial || { printf '!!! test-flair-app-launch-mutant FAIL: MBAR_NO_BOUNDS went RED for the wrong reason (want: the PSP-resident MenuBar ACCEPTED, no bar-outside-image refusal)\n'; grep -E '^(TENANT-|FLAIR-DISPATCH|PANIC)' $(BUILD)/flair_applmut_mbarbounds.serial; exit 1; }
@if $(PPM_FLAIR_APPL_CHECK_BIN) bar-photoshop $(BUILD)/flair_applmut_mbarbounds.ppm >/dev/null 2>&1; then b2='band 2 still the fallback'; else b2='and the out-of-image bar REACHED band 2'; fi; \
	printf '>>> test-flair-app-launch-mutant: MBAR_NO_BOUNDS RED for the named reason -- the MenuBar in the PSP below imageBase was accepted (-> 0, no TENANT-SETMBAR-BAD), %s\n' "$$b2"
@printf '>>> test-flair-app-launch-mutant: green (all six mutants RED for their named reasons)\n'
```

## test-flair-apple-menu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33809):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_calcda_gate.img build/flair_data.img build/ppm_flair_calcda_check build/.makefile.stamp`
```make
$(call calcda-boot,flair_apple_drop,$(FLAIR_CALCDA_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_APPLE_DROP_SPEC,FLAIR-MENU-DROP menu=1)
@grep -qx 'APPLE-MENU n=6' $(BUILD)/flair_apple_drop.serial && grep -qx 'FLAIR-MENU-DROP menu=1' $(BUILD)/flair_apple_drop.serial && [ -s $(BUILD)/flair_apple_drop.ppm ] \
 || { printf '!!! test-flair-apple-menu FAIL [1/3] DROP: no Apple menu dropped from the band-1 glyph\n'; grep -E '^(APPLE|FLAIR-MENU)' $(BUILD)/flair_apple_drop.serial; exit 1; }
@$(PPM_FLAIR_CALCDA_CHECK) apple $(BUILD)/flair_apple_drop.ppm "$(CALCDA_S7_CAPTURES)/s8_apple_menu.png" | sed 's/^/    /'; \
 $(PPM_FLAIR_CALCDA_CHECK) apple $(BUILD)/flair_apple_drop.ppm "$(CALCDA_S7_CAPTURES)/s8_apple_menu.png" >/dev/null \
 || { printf '!!! test-flair-apple-menu FAIL [1/3] DROP: the panel does not match s8_apple_menu.png\n'; exit 1; }
@printf '>>> test-flair-apple-menu [1/3] DROP: Apple menu (About, divider, Calculator) dropped; block, panel, anatomy vs the Mac OS 8.1 capture\n'
@$(call CALCDA_UP,$(BUILD)/flair_apple_drop.serial) && grep -qx 'FLAIR-MENU menu=1 item=3 (sel=0x00010003)' $(BUILD)/flair_apple_drop.serial \
 || { printf '!!! test-flair-apple-menu FAIL [2/3] LAUNCH: Calculator did not launch from the Apple menu\n'; grep -E '^(APPLE|FLAIR-MENU|TENANT-(LOAD|REG|GATE ax=0x0020)|FLAIR-DISPATCH)' $(BUILD)/flair_apple_drop.serial; exit 1; }
@printf '>>> test-flair-apple-menu [2/3] LAUNCH: Apple > Calculator -> APPS\\CALCULAT.EXE registered, rDocProc window at (267,94)\n'
$(call calcda-boot,flair_apple_about,$(FLAIR_CALCDA_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_APPLE_ABOUT_SPEC,APPLE-ABOUT up)
@l=$$(grep -m1 '^APPLE-ABOUT mem_k=' $(BUILD)/flair_apple_about.serial); \
 mem=$$(printf '%s' "$$l" | sed -n 's/.*mem_k=\([0-9]*\).*/\1/p'); big=$$(printf '%s' "$$l" | sed -n 's/.*largest_k=\([0-9]*\).*/\1/p'); \
 grep -qx 'APPLE-ABOUT up' $(BUILD)/flair_apple_about.serial && [ -n "$$mem" ] && [ "$$mem" -ge 129024 ] && [ "$$mem" -le 131072 ] && [ -n "$$big" ] && [ "$$big" -gt 0 ] \
 || { printf '!!! test-flair-apple-menu FAIL [3/3] ABOUT: %s\n' "$$l"; exit 1; }; \
 printf '>>> test-flair-apple-menu [3/3] ABOUT: %s (QEMU 128 MiB; the MCB chain'"'"'s largest free block)\n' "$$l"
@printf '>>> test-flair-apple-menu: green\n'
```

## test-flair-apple-menu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 123):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_calcda_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-apple-menu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_calcda_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-apple-menu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-apple-menu_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_calcda_gate.img" --disk2 "$(BUILD)/flair-apple-menu_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-apple-menu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-apple-menu_bochs.report.txt" || true
@cat "$(BUILD)/flair-apple-menu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-apple-menu-bochs "$(BUILD)/flair-apple-menu_bochs.report.txt" "$(BUILD)/flair-apple-menu_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-apple-menu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33827):
Prerequisites: `test-flair-apple-menu build/flair_tenants_calcda_apple_dead.img build/.makefile.stamp`
```make
$(call calcda-boot,flair_apple_dead,$(BUILD)/flair_tenants_calcda_apple_dead.img,$(FLAIR_DATA_IMG),FLAIR_APPLE_DROP_SPEC,FLAIR-LIVE-OK)
@if grep -qx 'FLAIR-MENU-DROP menu=1' $(BUILD)/flair_apple_dead.serial; then printf '!!! test-flair-apple-menu-mutant: MENU_MUT_APPLE_DEAD still dropped the Apple menu -- decoration\n'; exit 1; fi
@grep -qx 'FLAIR-MENU menu=0 item=0 (sel=0x00000000)' $(BUILD)/flair_apple_dead.serial || { printf '!!! test-flair-apple-menu-mutant: RED for the wrong reason\n'; exit 1; }
@printf '>>> test-flair-apple-menu-mutant: MENU_MUT_APPLE_DEAD correctly RED (FLAIR-MENU menu=0 item=0: the F13 dead glyph)\n'
```

## test-flair-appswitch

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 20242):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_appswitch_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-appswitch : THE booted App Contract (O-5)\n'
@printf '  Click NOTES sliver -> raise+activate NOTES over HELLO + swap band 2 (File/Edit/Notes; no Apple).\n'
@printf '  Ref: ADR-0013 (FLAIR App Contract); beads initech-4w15 / initech-7tjp; spec/flair_appswitch_trace.mk. Law 2/4.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (PRE + POST captures of the co-resident tenants)\n' "$(FLAIRTENANTS_IMG)"
@printf 'Expecting : PRE distinct chimera + POST FLAIR-DISPATCH app=NOTES + grader PASS\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@# ---- PRE: the co-resident scene before the click (no mouse). ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_APPSW_PRE_NAME)" --out "$(BUILD)" \
	--screendump --screendump-after "FLAIR-TENANTS-READY" --timeout-ms 15000 \
	2> "$(FLAIR_APPSW_PRE_REPORT)" || true
@# ---- POST: inject the locked trace; dump after the dispatch marker. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_APPSW_POST_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_APPSWITCH_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-DISPATCH app=NOTES" --timeout-ms 15000 \
	2> "$(FLAIR_APPSW_POST_REPORT)" || true
@cat "$(FLAIR_APPSW_POST_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@# ---- 1. No triple-fault in either boot. ----
@if grep -q 'triple_fault=1' "$(FLAIR_APPSW_PRE_REPORT)" "$(FLAIR_APPSW_POST_REPORT)"; then \
	printf '!!! test-flair-appswitch FAIL: TRIPLE FAULT in a tenants boot\n'; exit 1; \
fi
@printf '>>> test-flair-appswitch [1/4]: no triple-fault (PRE + POST)\n'
@# ---- 2. Serial milestones. ----
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(FLAIR_APPSW_PRE_SERIAL)" \
	&& grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(FLAIR_APPSW_POST_SERIAL)" \
	|| { printf '!!! test-flair-appswitch FAIL: FLAIR-FAT-MOUNT-OK missing in PRE or POST\n'; exit 1; }
@grep -q '^FLAIR-TENANTS-READY' "$(FLAIR_APPSW_PRE_SERIAL)" \
	|| { printf '!!! test-flair-appswitch FAIL: FLAIR-TENANTS-READY missing in the PRE boot (the scene never composed)\n'; exit 1; }
@grep -q '^FLAIR-LIVE-READY$$' "$(FLAIR_APPSW_POST_SERIAL)" \
	|| { printf '!!! test-flair-appswitch FAIL: FLAIR-LIVE-READY missing -- the pump never armed\n'; exit 1; }
@grep -q '^FLAIR-DISPATCH app=NOTES$$' "$(FLAIR_APPSW_POST_SERIAL)" \
	|| { printf '!!! test-flair-appswitch FAIL: FLAIR-DISPATCH app=NOTES missing -- the click did not dispatch the switch\n'; grep '^FLAIR-' "$(FLAIR_APPSW_POST_SERIAL)" || true; exit 1; }
@printf '>>> test-flair-appswitch [2/4]: FLAIR-TENANTS-READY + FLAIR-LIVE-READY + FLAIR-DISPATCH app=NOTES\n'
@# ---- 3. Both screendumps captured. ----
@if [ ! -s "$(FLAIR_APPSW_PRE_PPM)" ] || [ ! -s "$(FLAIR_APPSW_POST_PPM)" ]; then \
	printf '!!! test-flair-appswitch FAIL: a screendump is missing (PRE %s / POST %s)\n' "$(FLAIR_APPSW_PRE_PPM)" "$(FLAIR_APPSW_POST_PPM)"; exit 1; \
fi
@printf '>>> test-flair-appswitch [3/4]: PRE + POST screendumps captured\n'
@# ---- 4. Grade the PRE->POST delta against the INDEPENDENT canon. ----
@$(PPM_FLAIR_APPSWITCH_CHECK_BIN) "$(FLAIR_APPSW_PRE_PPM)" "$(FLAIR_APPSW_POST_PPM)" \
	|| { printf '!!! test-flair-appswitch FAIL: the PRE->POST delta is not a raise+activate+menubar-swap (the App Contract app-switch is not wired)\n'; exit 1; }
@printf '>>> test-flair-appswitch [4/4]: grader PASS (DISTINCT-CHIMERA + POST-DISTINCT + TIER-A overlap + TIER-B accent + BAR1-STATIC + MENU-BAND swap)\n'
@printf 'VERDICT   : PASS -- clicking the background tenant RAISES + ACTIVATES it and swaps\n'
@printf '            its menubar; the booted FLAIR App Contract honours O-5 (ADR-0013)\n'
@printf '            (QEMU; Bochs boot leg = make test-flair-appswitch-bochs)\n'
@printf '======================================================================\n'
```

## test-flair-appswitch-bochs

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 20423):
Prerequisites: `build/bochs_harness build/qemu_harness build/ppm_bochs_check build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-appswitch-bochs : BOCHS leg (App Contract)\n'
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator). ADR-0013; beads initech-4e35; WL-0170.\n'
@printf '  Bochs 2.7: VESA 0101h 640x480x8 LFB + 8-bit DAC; the tenants desktop is\n'
@printf '  presented and graded against the QEMU 32bpp capture at the same line.\n'
@printf '  (No mouse injection on Bochs -> the app-switch is graded on QEMU by\n'
@printf '  test-flair-appswitch; the boot + present + pixels are graded here.)\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-appswitch-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2). Set SKIP_BOCHS=1 to explicitly (and loudly) opt out.\n'; exit 1; }
@printf 'Booting   : %s under QEMU (32bpp golden) then Bochs (0101h; RFB headless)\n' "$(FLAIRTENANTS_IMG)"
@printf 'Expecting : VBE-MODE=0101 + shared kernel/heap/mount markers + FLAIR-TENANTS-READY,\n'
@printf '            then Bochs == QEMU on all 307200 pixels (snapshot exact + RFB BGR233)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(call bochs-pixel-leg,$(FLAIR_APPSW_BOCHS_NAME),$(FLAIRTENANTS_IMG),$(FLAIRTENANTS_IMG),$(FLAIR_GATE_DATA),$(BOCHS_PIXEL_TENANTS_LINE))
@cat "$(FLAIR_APPSW_BOCHS_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@sh harness/emu/bochs_leg_check.sh test-flair-appswitch-bochs "$(FLAIR_APPSW_BOCHS_REPORT)" "$(FLAIR_APPSW_BOCHS_SERIAL)" vbe101 \
	FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK OVL-OK FLAIR-DESKTOP '$(BOCHS_PIXEL_TENANTS_LINE)' !PANIC
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- the flairtenants kernel booted under Bochs through the shared\n'
@printf '            kernel + FLAIR-heap + FAT-mount milestones (== QEMU) on the VESA 0101h\n'
@printf '            path, reached FLAIR-TENANTS-READY, and the presented tenants desktop\n'
@printf '            equals the QEMU golden pixel for pixel (no triple-fault).\n'
@printf '======================================================================\n'
```

## test-flair-appswitch-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 20317):
Prerequisites: `build/qemu_harness build/ppm_flair_appswitch_check build/flair_tenants.img build/flair_data.img build/flair_tenants_mut_ignore_refcon.img build/flair_tenants_mut_skip_activate.img build/flair_tenants_mut_drop_update.img build/flair_tenants_mut_no_menubar_swap.img build/flair_tenants_mut_notes_bar_sys.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-appswitch-mutant : Rule 6 (the gate BITES)\n'
@printf '  5 FLAIRTENANTS mutants, each MUST drive ppm_flair_appswitch_check RED\n'
@printf '  (no_paintall_clear retired to the HOST proof -- see the OMISSION note\n'
@printf '  at the mutant-image instantiations; test_drag.c leg (e)).\n'
@printf '======================================================================\n'
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@# ---- baseline: the CLEAN image must grade GREEN (same PRE, same grader). ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name flair_appswitch_pre --out "$(BUILD)" \
	--screendump --screendump-after "FLAIR-TENANTS-READY" --timeout-ms 15000 >/dev/null 2>&1 || true
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name flair_appswitch_post --out "$(BUILD)" \
	--mouse "$(FLAIR_APPSWITCH_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-DISPATCH app=NOTES" --timeout-ms 15000 >/dev/null 2>&1 || true
@if [ ! -s "$(BUILD)/flair_appswitch_pre.ppm" ]; then printf '!!! test-flair-appswitch-mutant FAIL: clean PRE screendump missing (cannot establish the baseline)\n'; exit 1; fi
@$(PPM_FLAIR_APPSWITCH_CHECK_BIN) "$(BUILD)/flair_appswitch_pre.ppm" "$(BUILD)/flair_appswitch_post.ppm" >/dev/null 2>&1 \
	|| { printf '!!! test-flair-appswitch-mutant FAIL: the CLEAN image did not grade GREEN -- the baseline is broken (not a mutant)\n'; exit 1; }
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_appswitch_pre.serial" \
	&& grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_appswitch_post.serial" \
	|| { printf '!!! test-flair-appswitch-mutant FAIL: clean mount marker missing\n'; exit 1; }
@printf '>>> baseline: the clean FLAIRTENANTS image grades GREEN (the grader is live)\n'
@# ---- each mutant: same clean PRE, mutant POST -> the grader MUST go RED. ----
@rc=0; \
for spec in "ignore_refcon:FLAIR_LIVE_MUTATE_IGNORE_REFCON:owner-recovery returns the foreground always -- no switch (TIER-A)" \
            "skip_activate:FLAIR_LIVE_MUTATE_SKIP_ACTIVATE:skip the deactivate/activate pair -- no accent (TIER-B)" \
            "drop_update:FLAIR_LIVE_MUTATE_DROP_UPDATE:skip flair_route_updates -- exposed overlap stale (TIER-A)" \
            "no_menubar_swap:FLAIR_LIVE_MUTATE_NO_MENUBAR_SWAP:skip DrawMenuBar -- band unchanged (MENU-BAND)" \
            "notes_bar_sys:KMAIN_MUT_NOTES_BAR_SYS:restore NOTES to bar_sys -- post bars identical (POST-DISTINCT)"; do \
	tag=$${spec%%:*}; rest=$${spec#*:}; macro=$${rest%%:*}; desc=$${rest#*:}; \
	img="$(BUILD)/flair_tenants_mut_$$tag.img"; \
	printf '%s\n' '----------------------------------------------------------------------'; \
	printf '>>> mutant %s (-D%s): %s\n' "$$tag" "$$macro" "$$desc"; \
	$(HARNESS_BIN) --disk "$$img" --disk2 "$(FLAIR_GATE_DATA)" \
		--expect FLAIR-FAT-MOUNT-OK --name "flair_appswitch_mut_$$tag" --out "$(BUILD)" \
		--mouse "$(FLAIR_APPSWITCH_SPEC)" --keys-after "FLAIR-LIVE-READY" \
		--screendump --screendump-after "FLAIR-DISPATCH app=NOTES" --timeout-ms 15000 \
		2> "$(BUILD)/flair_appswitch_mut_$$tag.report" || true; \
	if ! grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_appswitch_mut_$$tag.serial"; then \
		printf '!!! test-flair-appswitch-mutant FAIL: mutant %s missing mount marker\n' "$$tag"; rc=1; continue; \
	fi; \
	if grep -q 'triple_fault=1' "$(BUILD)/flair_appswitch_mut_$$tag.report"; then \
		printf '!!! test-flair-appswitch-mutant FAIL: mutant %s TRIPLE-FAULTED (cannot judge the oracle)\n' "$$tag"; rc=1; continue; \
	fi; \
	if [ ! -s "$(BUILD)/flair_appswitch_mut_$$tag.ppm" ]; then \
		if grep -q 'FLAIR-DISPATCH app=NOTES' "$(BUILD)/flair_appswitch_mut_$$tag.serial" 2>/dev/null; then \
			printf '!!! test-flair-appswitch-mutant FAIL: mutant %s produced no screendump despite FLAIR-DISPATCH (harness fault, not a mutant kill)\n' "$$tag"; rc=1; \
		else \
			printf '>>> mutant %s correctly RED: FLAIR-DISPATCH app=NOTES never fired, so the b3hl-era\n' "$$tag"; \
			printf '      marker-gated harness took NO dump (the strictly-louder missing-marker path)\n'; \
		fi; continue; \
	fi; \
	if $(PPM_FLAIR_APPSWITCH_CHECK_BIN) "$(BUILD)/flair_appswitch_pre.ppm" "$(BUILD)/flair_appswitch_mut_$$tag.ppm" > "$(BUILD)/flair_appswitch_mut_$$tag.chk" 2>&1; then \
		printf '!!! test-flair-appswitch-mutant FAIL: the %s oracle is DECORATION -- mutant %s PASSED ppm_flair_appswitch_check\n' "$$macro" "$$tag"; \
		rc=1; \
	else \
		printf '>>> mutant %s correctly RED:\n' "$$tag"; \
		grep -m3 'FAIL ' "$(BUILD)/flair_appswitch_mut_$$tag.chk" | sed 's/^/      /'; \
	fi; \
done; \
if [ "$$rc" != "0" ]; then exit 1; fi
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- all 5 mutants drive ppm_flair_appswitch_check RED (the gate bites; Rule 6;\n'
@printf '            no_paintall_clear is host-proven in test-drag-mutant leg (e))\n'
@printf '======================================================================\n'
```

## test-flair-appswitch-vga13-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 203):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-appswitch-vga13-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vga13"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-appswitch-vga13-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@test -r /usr/share/vgabios/vgabios.cirrus.bin || { printf '!!! test-flair-appswitch-vga13-bochs FAIL: ISA Cirrus VGA BIOS %s missing (apt install vgabios; bochs Depends: vgabios | seabios). A skipped oracle is worse than a red one (Law 2).\n' "/usr/share/vgabios/vgabios.cirrus.bin"; exit 1; }
cp -f build/flair_data.img $(BUILD)/flair-appswitch-vga13_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-appswitch-vga13_bochs_data.img" --vga-ext cirrus --vgabios /usr/share/vgabios/vgabios.cirrus.bin --expect HALTED --quit-after HALTED --name "flair-appswitch-vga13_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-appswitch-vga13_bochs.report.txt" || true
@cat "$(BUILD)/flair-appswitch-vga13_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-appswitch-vga13-bochs "$(BUILD)/flair-appswitch-vga13_bochs.report.txt" "$(BUILD)/flair-appswitch-vga13_bochs.serial" vga13 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK 'PANIC flair-desktop: LFB smaller than 640x480 w=320 h=200' HALTED
```

## test-flair-boot-screen

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34868):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_boothold.img build/vesa8/boothold.img build/flair_data.img build/ppm_flair_boot_screen_check`
```make
@test -s "$(BOOT_SCREEN_PNG)" || { printf '!!! test-flair-boot-screen FAIL: golden %s absent (system7-decomp mint-009)\n' "$(BOOT_SCREEN_PNG)"; exit 1; }
$(call boot-screen-boot,$(FLAIRTENANTS_BOOTHOLD_IMG),flair_boot_screen)
@$(call BOOT_SCREEN_ORDER,$(BUILD)/flair_boot_screen.serial) \
	|| { printf '!!! test-flair-boot-screen FAIL: serial order is not BOOT-SCREEN < FAT-MOUNT-OK < DESKTOP < LIVE-OK\n'; grep -E '^FLAIR-' $(BUILD)/flair_boot_screen.serial | head -20; exit 1; }
@! grep -q '^PANIC' $(BUILD)/flair_boot_screen.serial
@$(BOOT_SCREEN_CHECK) $(BUILD)/flair_boot_screen.ppm "$(BOOT_SCREEN_PNG)" | sed 's/^/    /'
@$(BOOT_SCREEN_CHECK) $(BUILD)/flair_boot_screen.ppm "$(BOOT_SCREEN_PNG)" >/dev/null \
	|| { printf '!!! test-flair-boot-screen FAIL: the live splash differs from s8_startup_welcome.png / the strike\n'; exit 1; }
@grep -q '^VIDEO 640x480 bpp=32 ' $(BUILD)/flair_boot_screen.serial
$(call boot-screen-boot,$(VESA8_DIR)/boothold.img,flair_boot_screen_8bpp)
@grep -q '^VIDEO 640x480 bpp=8 ' $(BUILD)/flair_boot_screen_8bpp.serial \
	|| { printf '!!! test-flair-boot-screen FAIL: the 8bpp leg did not boot VBE 0x101\n'; exit 1; }
@$(call BOOT_SCREEN_ORDER,$(BUILD)/flair_boot_screen_8bpp.serial)
@$(BOOT_SCREEN_CHECK) $(BUILD)/flair_boot_screen_8bpp.ppm "$(BOOT_SCREEN_PNG)" | sed 's/^/    8bpp: /'
@$(BOOT_SCREEN_CHECK) $(BUILD)/flair_boot_screen_8bpp.ppm "$(BOOT_SCREEN_PNG)" >/dev/null \
	|| { printf '!!! test-flair-boot-screen FAIL: the 8bpp (DAC) splash differs from s8_startup_welcome.png / the strike\n'; exit 1; }
@printf '>>> test-flair-boot-screen: green (live splash == Mac OS 8.1 Welcome panel cell for cell + the Initech strike, 32bpp AND 8bpp DAC; splash < FAT mount < desktop)\n'
```

## test-flair-boot-screen-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34886):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_bootnologo.img build/flair_data.img build/ppm_flair_boot_screen_check`
```make
$(call boot-screen-boot,$(BUILD)/flair_tenants_mut_bootnologo.img,flair_boot_screen_nologo)
@$(call BOOT_SCREEN_ORDER,$(BUILD)/flair_boot_screen_nologo.serial)
@if $(BOOT_SCREEN_CHECK) $(BUILD)/flair_boot_screen_nologo.ppm "$(BOOT_SCREEN_PNG)" > $(BUILD)/flair_boot_screen_nologo.pixels 2>&1; then printf '!!! test-flair-boot-screen-mutant FAIL: FLAIR_BOOT_MUT_NO_LOGO survived\n'; exit 1; fi
@grep -q 'ok   CHROME' $(BUILD)/flair_boot_screen_nologo.pixels && grep -q 'FAIL LOGO' $(BUILD)/flair_boot_screen_nologo.pixels \
	|| { cat $(BUILD)/flair_boot_screen_nologo.pixels; exit 1; }
@printf '>>> test-flair-boot-screen-mutant: FLAIR_BOOT_MUT_NO_LOGO correctly RED: %s\n' "$$(grep -m1 'FAIL LOGO' $(BUILD)/flair_boot_screen_nologo.pixels | sed 's/^ *//')"
```

## test-flair-box-track

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30417):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-box-track : close/zoom/collapse act on release inside (G02)\n'
@printf '  bead initech-tdnl.60; trace FLAIR_BOX_CANCEL_SPEC (spec/flair_input_traces.mk)\n'
@printf '======================================================================\n'
$(call held-boot,$(FLAIRTENANTS_IMG),flair_box_track,FLAIR_BOX_CANCEL_SPEC,FLAIR-TENANT-EXIT name=HELLO)
@$(call box-track-check,"$(BUILD)/flair_box_track.serial") || { printf '!!! test-flair-box-track FAIL: a box acted on the press or a drag-out did not cancel (want 3x in=0, then in=1 + exactly one FLAIR-CLOSE after the 4th mouseUp; no ZOOM/COLLAPSE)\n'; grep -E '^(FLAIR-(TRACKBOX|CLOSE|ZOOM|COLLAPSE|TENANT-EXIT|EVT what=[12])|TENANT-EXIT)' "$(BUILD)/flair_box_track.serial"; exit 1; }
@printf '>>> test-flair-box-track: three press-drag-out releases cancelled (close, zoom, collapse); press-out-back-in-release closed HELLO\n'
@printf '>>> test-flair-box-track: green\n'
```

## test-flair-box-track-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30427):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_goaway_on_press.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-box-track-mutant : Rule 6\n'
@printf '  Mutant: -DKMAIN_MUT_GOAWAY_ON_PRESS (the box acts on mouseDown)\n'
@printf '======================================================================\n'
$(call held-boot,$(BUILD)/flair_tenants_mut_goaway_on_press.img,flair_box_track_mut,FLAIR_BOX_CANCEL_SPEC,FLAIR-LIVE-OK)
@grep -qx 'FLAIR-LIVE-READY' "$(BUILD)/flair_box_track_mut.serial" || { printf '!!! test-flair-box-track-mutant: mutant never armed (not comparable)\n'; exit 1; }
@if $(call box-track-check,"$(BUILD)/flair_box_track_mut.serial"); then printf '!!! test-flair-box-track-mutant FAIL: act-on-press PASSED -- the gate is decoration\n'; exit 1; fi
@printf '>>> test-flair-box-track-mutant: RED as required -- %s\n' "$$(grep -m1 -E '^FLAIR-(CLOSE|ZOOM|COLLAPSE) ' $(BUILD)/flair_box_track_mut.serial || echo 'no verb')"
```

## test-flair-collapse

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23173):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_window_ops_check build/.makefile.stamp`
```make
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_COLLAPSE_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_COLLAPSE_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-COLLAPSE win 0 1" --timeout-ms 15000 \
	2> "$(BUILD)/$(FLAIR_COLLAPSE_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_COLLAPSE_NAME).report"; then printf '!!! test-flair-collapse FAIL: TRIPLE FAULT\n'; exit 1; fi
@grep -q '^FLAIR-COLLAPSE win 0 1$$' "$(BUILD)/$(FLAIR_COLLAPSE_NAME).serial" \
	|| { printf '!!! test-flair-collapse FAIL: collapse marker missing\n'; exit 1; }
@$(PPM_FLAIR_WINDOW_OPS_CHECK_BIN) collapse "$(BUILD)/$(FLAIR_COLLAPSE_NAME).ppm"
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/$(FLAIR_COLLAPSE_NAME).serial"
@printf '>>> test-flair-collapse: green -- title-band-only endpoint\n'
```

## test-flair-copybreak

Declared tier: EMU; #  recipe to execute (from 'harness/emu/copybreak.mk', line 23):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/copybreak_seed_data.img build/da_store_check build/.makefile.stamp`
```make
$(call appl-boot,$(COPYBREAK_TAG),$(COPYBREAK_IMAGE),$(COPYBREAK_DATA),COPYBREAK_SPEC,)
@sh harness/emu/copybreak_check.sh $(BUILD)/$(COPYBREAK_TAG).serial
@$(call APPL_ALIVE,$(BUILD)/$(COPYBREAK_TAG).serial)
@mcopy -o -i $(BUILD)/$(COPYBREAK_TAG)_data.img ::SCRAPBK.DAT $(BUILD)/$(COPYBREAK_TAG).dat
@$(DA_DISK) scrapbook $(BUILD)/$(COPYBREAK_TAG).dat 1 alpha alpha
```

## test-flair-copybreak-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/emu/copybreak.mk', line 80):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/copybreak_seed_data.img build/da_store_check build/.makefile.stamp`
```make
cp -f $(COPYBREAK_DATA) $(BUILD)/copybreak_bochs_data.img
@$(BOCHS_BIN) --disk $(COPYBREAK_IMAGE) --disk2 $(BUILD)/copybreak_bochs_data.img --name copybreak_bochs --out $(BUILD) --keys '$(COPYBREAK_SPEC)' --keys-after FLAIR-LIVE-READY --quit-after FLAIR-LIVE-OK --timeout-ms 180000 2>$(BUILD)/copybreak_bochs.report
@sh harness/emu/copybreak_check.sh $(BUILD)/copybreak_bochs.serial
@mcopy -o -i $(BUILD)/copybreak_bochs_data.img ::SCRAPBK.DAT $(BUILD)/copybreak_bochs.dat
@$(DA_DISK) scrapbook $(BUILD)/copybreak_bochs.dat 1 alpha alpha
```

## test-flair-copybreak-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/copybreak.mk', line 62):
Prerequisites: `build/qemu_harness build/copybreak_leak.img build/copybreak_seed_data.img build/.makefile.stamp`
```make
cp -f $(COPYBREAK_DATA) $(BUILD)/copybreak_leak_data.img
@$(HARNESS_BIN) --disk $(BUILD)/copybreak_leak.img --disk2 $(BUILD)/copybreak_leak_data.img --name copybreak_leak --out $(BUILD) --mouse '$(COPYBREAK_SPEC)' --keys-after FLAIR-LIVE-READY --quit-after 'INT21-EXIT rc=0' --timeout-ms 30000 2>$(BUILD)/copybreak_leak.report || true
@grep -q 'triple_fault=0' $(BUILD)/copybreak_leak.report
@if sh harness/emu/copybreak_check.sh $(BUILD)/copybreak_leak.serial >$(BUILD)/copybreak_leak.check 2>&1; then echo 'FAIL dual-ring mutant survived QEMU'; exit 1; fi
@grep -qx 'RED copybreak: GUI key reached DOS break/exit or input dropped' $(BUILD)/copybreak_leak.check
@grep -qx 'TENANT-SCRAP put n=5 count=1' $(BUILD)/copybreak_leak.serial
@grep -qx 'INT23-BREAK' $(BUILD)/copybreak_leak.serial && grep -qx 'INT21-EXIT rc=0' $(BUILD)/copybreak_leak.serial
@echo 'GREEN copybreak leak mutant: copied alpha, then INT23 terminated the desktop'
```

## test-flair-copybreak-record

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/copybreak.mk', line 71):
Prerequisites: `build/.makefile.stamp`
```make
+@$(MAKE) --no-print-directory record-flair SCRIPT=copybreak
@sh harness/emu/copybreak_check.sh $(RECORD_CLIPS_DIR)/rec_copybreak.serial
```

## test-flair-cursor

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 11734):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_cursor_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-cursor : M009 CursorMgr independent capture gate\n'
@printf '  Move center (320,240) -> hotspot (500,380); grade all 256 cells against the Mac OS 8 capture.\n'
@printf '  beads initech-tdnl.1/.98; PRD Sec 6.3; Law 2/4, Rules 6/11/12.\n'
@printf '======================================================================\n'
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIR_CURSOR_IMAGE)" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_CURSOR_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_CURSOR_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-CURSOR x=500 y=380" --timeout-ms 15000 \
	2> "$(FLAIR_CURSOR_REPORT)" || true
@cat "$(FLAIR_CURSOR_REPORT)"
@if grep -q 'triple_fault=1' "$(FLAIR_CURSOR_REPORT)"; then \
	printf '!!! test-flair-cursor FAIL: TRIPLE FAULT in CursorMgr boot\n'; exit 1; \
fi
@grep -q '^FLAIR-LIVE-READY$$' "$(FLAIR_CURSOR_SERIAL)" \
	|| { printf '!!! test-flair-cursor FAIL: FLAIR-LIVE-READY missing\n'; exit 1; }
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(FLAIR_CURSOR_SERIAL)" \
	|| { printf '!!! test-flair-cursor FAIL: FLAIR-FAT-MOUNT-OK missing\n'; exit 1; }
@grep -q '^FLAIR-CURSOR x=500 y=380$$' "$(FLAIR_CURSOR_SERIAL)" \
	|| { printf '!!! test-flair-cursor FAIL: expected position marker missing\n'; grep '^FLAIR-CURSOR ' "$(FLAIR_CURSOR_SERIAL)" || true; exit 1; }
@if [ ! -s "$(FLAIR_CURSOR_PPM)" ]; then \
	printf '!!! test-flair-cursor FAIL: marker-gated screendump missing\n'; exit 1; \
fi
@$(PPM_FLAIR_CURSOR_CHECK_BIN) "$(FLAIR_CURSOR_PPM)" $(CURSOR_CAPTURES) \
	|| { printf '!!! test-flair-cursor FAIL: independent Mac arrow crop differs\n'; exit 1; }
@printf 'VERDICT   : PASS -- booted CursorMgr tracks the injected mouse position\n'
@printf '======================================================================\n'
```

## test-flair-cursor-boot

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 11803):
Prerequisites: `build/qemu_harness build/flair_tenants_interactive.img build/flair_data.img build/ppm_flair_cursor_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-cursor-boot : the arrow is on screen at boot, NO input\n'
@printf '  leg A: still at FLAIR-LIVE-READY, arrow at the Mac OS 8.1 boot position (320,240);\n'
@printf '  leg B: M009 trace -> (500,380) (the cursor-free reference for leg A).\n'
@printf '  bead initech-vzjy; mint-results-009 Sec 1; PRD Sec 6.3; Law 2/4, Rules 6/11/12.\n'
@printf '======================================================================\n'
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_data.img
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_data.img,$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIR_CURSOR_BOOT_IMAGE)" --disk2 "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_data.img" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_CURSOR_BOOT_NAME)_a" --out "$(BUILD)" \
	--screendump --screendump-after "FLAIR-LIVE-READY" --timeout-ms 20000 \
	2> "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_a.report" || true
@$(HARNESS_BIN) --disk "$(FLAIR_CURSOR_BOOT_IMAGE)" --disk2 "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_data.img" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_CURSOR_BOOT_NAME)_b" --out "$(BUILD)" \
	--mouse "$(FLAIR_CURSOR_SPEC)" --keys-after "FLAIR-LIVE-READY" --keys-after-ms 15000 \
	--screendump --screendump-after "FLAIR-CURSOR x=500 y=380" --timeout-ms 20000 \
	2> "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_b.report" || true
@cat "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_a.report" "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_b.report"
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_a.report" "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_b.report"; then \
	printf '!!! test-flair-cursor-boot FAIL: TRIPLE FAULT\n'; exit 1; \
fi
@for leg in a b; do \
	grep -q '^FLAIR-LIVE-READY$$' "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_$$leg.serial" \
		|| { printf '!!! test-flair-cursor-boot FAIL: leg %s FLAIR-LIVE-READY missing\n' "$$leg"; exit 1; }; \
	grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_$$leg.serial" \
		|| { printf '!!! test-flair-cursor-boot FAIL: leg %s FLAIR-FAT-MOUNT-OK missing\n' "$$leg"; exit 1; }; \
	[ -s "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_$$leg.ppm" ] \
		|| { printf '!!! test-flair-cursor-boot FAIL: leg %s marker-gated screendump missing\n' "$$leg"; exit 1; }; \
done
@# Leg B first: the M009 arrow at (500,380) on the interactive image (its still
@# is then a proven cursor-free reference at the centre: save-under restored).
@grep -q '^FLAIR-CURSOR x=500 y=380$$' "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_b.serial" \
	|| { printf '!!! test-flair-cursor-boot FAIL: leg B position marker missing\n'; grep '^FLAIR-CURSOR ' "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_b.serial" || true; exit 1; }
@$(PPM_FLAIR_CURSOR_CHECK_BIN) "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_b.ppm" $(CURSOR_CAPTURES) \
	|| { printf '!!! test-flair-cursor-boot FAIL: leg B -- the tracked arrow at (500,380) differs from the Mac crop\n'; exit 1; }
@# Leg A: the pixels first (the mutant's RED line), then the serial order.
@$(PPM_FLAIR_CURSOR_CHECK_BIN) "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_a.ppm" $(CURSOR_CAPTURES) 320 240 "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_b.ppm" \
	|| { printf '!!! test-flair-cursor-boot FAIL: leg A -- NO arrow at the Mac OS 8.1 boot position (320,240) with no input\n'; exit 1; }
@if grep -q '^FLAIR-EVT ' "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_a.serial"; then \
	printf '!!! test-flair-cursor-boot FAIL: leg A saw input (FLAIR-EVT) -- not a no-input still\n'; exit 1; \
fi
@bs=$$(grep -n -m1 '^FLAIR-BOOT-SCREEN$$' "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_a.serial" | cut -d: -f1); \
 cu=$$(grep -n -m1 '^FLAIR-CURSOR x=320 y=240$$' "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_a.serial" | cut -d: -f1); \
 rd=$$(grep -n -m1 '^FLAIR-LIVE-READY$$' "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_a.serial" | cut -d: -f1); \
 [ -n "$$bs" ] && [ -n "$$cu" ] && [ "$$bs" -lt "$$cu" ] && [ "$$cu" -lt "$$rd" ] \
	|| { printf '!!! test-flair-cursor-boot FAIL: leg A serial order (BOOT-SCREEN=%s CURSOR=%s READY=%s): the arrow must be shown AFTER the splash->desktop repaint and BEFORE the pump arms\n' "$$bs" "$$cu" "$$rd"; exit 1; }; \
 printf '>>> test-flair-cursor-boot: leg A serial order FLAIR-BOOT-SCREEN(%s) < FLAIR-CURSOR x=320 y=240(%s) < FLAIR-LIVE-READY(%s)\n' "$$bs" "$$cu" "$$rd"
@if grep '^FLAIR-CURSOR ' "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_a.serial" | grep -qv '^FLAIR-CURSOR x=320 y=240$$'; then \
	printf '!!! test-flair-cursor-boot FAIL: leg A moved the arrow without input\n'; grep '^FLAIR-CURSOR ' "$(BUILD)/$(FLAIR_CURSOR_BOOT_NAME)_a.serial"; exit 1; \
fi
@printf 'VERDICT   : PASS -- the arrow is on the desktop at (320,240) before any input, survives the splash->desktop repaint, and tracks to (500,380)\n'
@printf '======================================================================\n'
```

## test-flair-cursor-boot-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 11862):
Prerequisites: `build/flair_tenants_mut_cursorhideboot.img build/qemu_harness build/flair_data.img build/ppm_flair_cursor_check build/.makefile.stamp`
```make
@rc=0; $(MAKE) --no-print-directory $(addprefix -o ,$(FLAIR_CURSOR_MUT_FROZEN) $(BUILD)/flair_tenants_mut_cursorhideboot.img) test-flair-cursor-boot FLAIR_GATE_DATA=$(BUILD)/$@_data.img FLAIR_CURSOR_BOOT_IMAGE=$(BUILD)/flair_tenants_mut_cursorhideboot.img FLAIR_CURSOR_BOOT_NAME=flair_cursor_boot_mut_hide \
	>$(BUILD)/flair_cursor_boot_mut_hide.log 2>&1 || rc=$$?; \
[ "$$rc" -ne 0 ] && grep -qF 'RED M009 hotspot=(320,240)' $(BUILD)/flair_cursor_boot_mut_hide.log \
	|| { cat $(BUILD)/flair_cursor_boot_mut_hide.log; printf '!!! test-flair-cursor-boot-mutant FAIL: CURSOR_MUT_HIDE_AT_BOOT survived -- the no-input still did not go RED on the pixels\n'; exit 1; }; \
printf '>>> test-flair-cursor-boot-mutant: green (CURSOR_MUT_HIDE_AT_BOOT RED -- %s)\n' "$$(grep -m1 'RED M009 hotspot=(320,240)' $(BUILD)/flair_cursor_boot_mut_hide.log | sed 's/^ppm_flair_cursor_check: //')"
```

## test-flair-cursor-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 11778):
Prerequisites: `build/flair_tenants_mut_cursor_squared.img build/qemu_harness build/flair_data.img build/ppm_flair_cursor_check build/.makefile.stamp`
```make
@rc=0; $(MAKE) --no-print-directory $(addprefix -o ,$(FLAIR_CURSOR_MUT_FROZEN) $(BUILD)/flair_tenants_mut_cursor_squared.img) test-flair-cursor FLAIR_GATE_DATA=$(BUILD)/$@_data.img FLAIR_CURSOR_IMAGE=$(BUILD)/flair_tenants_mut_cursor_squared.img FLAIR_CURSOR_NAME=flair_cursor_mut_squared \
	>$(BUILD)/flair_cursor_mut_squared.log 2>&1 || rc=$$?; \
[ "$$rc" -ne 0 ] && grep -qF 'RED M009 hotspot=(500,380) 48/256 differing cells' $(BUILD)/flair_cursor_mut_squared.log \
	|| { cat $(BUILD)/flair_cursor_mut_squared.log; printf '!!! test-flair-cursor-mutant FAIL: old strike did not fail on the independent crop\n'; exit 1; }; \
printf '>>> test-flair-cursor-mutant: green (CURSOR_MUT_SQUARED_STEM RED -- 48/256 cells against Mac capture)\n'
```

## test-flair-dac

Declared tier: HOST; #  recipe to execute (from 'harness/emu/vesa8.mk', line 36):
Prerequisites: `test-flair-dac-gen test-color-canon-gen test-color-canon test-clut build/test_flair_dac build/.makefile.stamp`
```make
@$(BUILD)/test_flair_dac
```

## test-flair-dac-gen

Declared tier: HOST; #  recipe to execute (from 'harness/emu/vesa8.mk', line 32):
Prerequisites: `build/flair_dac.h build/.makefile.stamp`
```make
@$(VESA8_GEN) spec/assets/clut.json spec/assets/color_canon.json > $(BUILD)/flair_dac.again.h
@cmp $(VESA8_HEADER) $(BUILD)/flair_dac.again.h
@echo 'GREEN test-flair-dac-gen: deterministic generated header equality'
```

## test-flair-dac-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/emu/vesa8.mk', line 38):
Prerequisites: `test-flair-dac build/flair_dac.h build/.makefile.stamp | build/vesa8`
```make
@set -eu; for i in 119 165; do \
 mkdir -p $(VESA8_DIR)/mut$$i; \
 $(CC) $(CFLAGS) -DDAC_GEN_MUT_OMIT=$$i tools/flair_dac_gen.c -o $(VESA8_DIR)/mut$$i/gen; \
 $(VESA8_DIR)/mut$$i/gen spec/assets/clut.json spec/assets/color_canon.json > $(VESA8_DIR)/mut$$i/flair_dac.h; \
 $(CC) $(CFLAGS) -I$(VESA8_DIR)/mut$$i -Ispec/assets harness/proptest/test_flair_dac.c -o $(VESA8_DIR)/mut$$i/check; \
 if $(VESA8_DIR)/mut$$i/check > $(VESA8_DIR)/mut$$i/red.log 2>&1; then echo 'FAIL DAC generator mutant survived'; exit 1; fi; \
 grep "^RED flair-dac idx=$$i " $(VESA8_DIR)/mut$$i/red.log; done
@echo 'GREEN test-flair-dac-mutant: both generated-table omissions RED'
```

## test-flair-data-volume

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 12350):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/desktop_db_empty.golden build/.makefile.stamp`
```make
@command -v mdir    >/dev/null 2>&1 || { printf '!!! test-flair-data-volume FAIL: mdir missing\n'; exit 1; }
@command -v mtype   >/dev/null 2>&1 || { printf '!!! test-flair-data-volume FAIL: mtype missing\n'; exit 1; }
@command -v mattrib >/dev/null 2>&1 || { printf '!!! test-flair-data-volume FAIL: mattrib missing\n'; exit 1; }
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_DATA_RUNTIME_IMG)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_DATA_RUNTIME_IMG),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_DATA_RUNTIME_IMG)" \
	--expect FLAIR-LIVE-OK --screendump --screendump-after FLAIR-LIVE-OK \
	--name flair_data_first --out "$(BUILD)" --timeout-ms 15000
@grep -qxF 'FLAIR-FAT-MOUNT-OK' $(BUILD)/flair_data_first.serial \
	|| { printf '!!! test-flair-data-volume FAIL: first boot missing FLAIR-FAT-MOUNT-OK\n'; exit 1; }
@grep -qxF 'DESKTOP-DB-CREATE' $(BUILD)/flair_data_first.serial \
	|| { printf '!!! test-flair-data-volume FAIL: first boot missing DESKTOP-DB-CREATE\n'; exit 1; }
@! grep -qF 'FLAIR-FAT-MOUNT-FAIL' $(BUILD)/flair_data_first.serial
@! grep -qF 'DESKTOP-DB-WRITE-FAIL' $(BUILD)/flair_data_first.serial
@mdir -a -i $(FLAIR_DATA_RUNTIME_IMG) :: > $(BUILD)/flair_data_first.mdir
@grep -Eq '^DESKTOP +DB +8 ' $(BUILD)/flair_data_first.mdir \
	|| { printf '!!! test-flair-data-volume FAIL: mdir does not show 8-byte DESKTOP.DB\n'; exit 1; }
@mdir -i $(FLAIR_DATA_RUNTIME_IMG) ::TRASH > $(BUILD)/flair_data_trash.mdir \
	|| { printf '!!! test-flair-data-volume FAIL: TRASH directory absent\n'; exit 1; }
@mtype -i $(FLAIR_DATA_RUNTIME_IMG) ::DESKTOP.DB > $(BUILD)/flair_data_desktop_db.bin
@cmp $(DESKTOP_DB_GOLDEN) $(BUILD)/flair_data_desktop_db.bin \
	|| { printf '!!! test-flair-data-volume FAIL: DESKTOP.DB != independent header golden\n'; exit 1; }
@mattrib -i $(FLAIR_DATA_RUNTIME_IMG) ::DESKTOP.DB 2>/dev/null | grep -Eq '^ +H ' \
	|| { printf '!!! test-flair-data-volume FAIL: DESKTOP.DB hidden attribute absent\n'; exit 1; }
@printf '>>> test-flair-data-volume [first]: MOUNT-OK + CREATE; mdir/mtype golden + TRASH green\n'
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_DATA_RUNTIME_IMG)" \
	--expect FLAIR-LIVE-OK --screendump --screendump-after FLAIR-LIVE-OK \
	--name flair_data_second --out "$(BUILD)" --timeout-ms 15000
@grep -qxF 'FLAIR-FAT-MOUNT-OK' $(BUILD)/flair_data_second.serial
@grep -qxF 'DESKTOP-DB-OK' $(BUILD)/flair_data_second.serial \
	|| { printf '!!! test-flair-data-volume FAIL: second boot missing DESKTOP-DB-OK\n'; exit 1; }
@! grep -qF 'DESKTOP-DB-CREATE' $(BUILD)/flair_data_second.serial
@! grep -qF 'DESKTOP-DB-REGEN' $(BUILD)/flair_data_second.serial
@printf '>>> test-flair-data-volume [second]: SAME volume persisted; DESKTOP-DB-OK green\n'
```

## test-flair-data-volume-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 12390):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_tenants_db_write_skip.img build/flair_data.img build/flair_data_corrupt_db.img build/desktop_db_empty.golden build/.makefile.stamp`
```make
@$(call fixture-preflight,$(FLAIR_DATA_CORRUPT_IMG))
cp -f $(FLAIR_DATA_CORRUPT_IMG) $(FLAIR_DATA_MUT_RUNTIME_IMG)
@$(if $(call fixture-spec,$(FLAIR_DATA_CORRUPT_IMG)),$(call fixture-check,harness,$(FLAIR_DATA_MUT_RUNTIME_IMG),$(call fixture-spec,$(FLAIR_DATA_CORRUPT_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_DATA_MUT_RUNTIME_IMG)" \
	--expect FLAIR-LIVE-OK --screendump --screendump-after FLAIR-LIVE-OK \
	--name flair_data_magic_flip --out "$(BUILD)" --timeout-ms 15000
@grep -qxF 'FLAIR-FAT-MOUNT-OK' $(BUILD)/flair_data_magic_flip.serial
@grep -q '^DESKTOP-DB-REGEN reason=' $(BUILD)/flair_data_magic_flip.serial \
	|| { printf '!!! test-flair-data-volume-mutant FAIL: DB_MAGIC_FLIP did not announce REGEN\n'; exit 1; }
@mtype -i $(FLAIR_DATA_MUT_RUNTIME_IMG) ::DESKTOP.DB > $(BUILD)/flair_data_magic_regen.bin
@cmp $(DESKTOP_DB_GOLDEN) $(BUILD)/flair_data_magic_regen.bin \
	|| { printf '!!! test-flair-data-volume-mutant FAIL: REGEN did not restore golden header\n'; exit 1; }
@printf '>>> test-flair-data-volume-mutant: DB_MAGIC_FLIP -> REGEN green\n'
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_DATA_MUT_RUNTIME_IMG)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_DATA_MUT_RUNTIME_IMG),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_DB_SKIP_IMG)" --disk2 "$(FLAIR_DATA_MUT_RUNTIME_IMG)" \
	--expect FLAIR-LIVE-OK --screendump --screendump-after FLAIR-LIVE-OK \
	--name flair_data_write_skip --out "$(BUILD)" --timeout-ms 15000
@grep -qxF 'FLAIR-FAT-MOUNT-OK' $(BUILD)/flair_data_write_skip.serial
@if mtype -i $(FLAIR_DATA_MUT_RUNTIME_IMG) ::DESKTOP.DB > $(BUILD)/flair_data_skip.bin 2>/dev/null && \
	cmp -s $(DESKTOP_DB_GOLDEN) $(BUILD)/flair_data_skip.bin; then \
	printf '!!! test-flair-data-volume-mutant FAIL: DB_WRITE_SKIP survived mtools diff\n'; exit 1; \
else \
	printf '>>> test-flair-data-volume-mutant: DB_WRITE_SKIP correctly RED under mtools diff\n'; \
fi
```

## test-flair-dc4v

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 19948):
Prerequisites: `build/qemu_harness build/flair_live.img build/ppm_flair_dc4v_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-dc4v : modal + bars SURVIVE a drag across them\n'
@printf '  Drag FILE COPY left across window 1: modal stays intact at new rect;\n'
@printf '  old footprint restores window content; menu bars remain intact.\n'
@printf '  initech-dc4v/-pipa/-zn61 reverse-survival re-key. Law 2/4.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (the WaitNextEvent pump, show_modal=1)\n' "$(FLAIRLIVE_IMG)"
@printf 'Expecting : FLAIR-MODAL-DRAG (140,200)->(0,200) + window restore + bars intact\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_IMG)" --name "$(FLAIR_DC4V_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_DC4V_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-MODAL-DRAG" --timeout-ms 15000 \
	2> "$(FLAIR_DC4V_REPORT)" || true
@cat "$(FLAIR_DC4V_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(FLAIR_DC4V_REPORT)"; then \
	printf '!!! test-flair-dc4v FAIL: TRIPLE FAULT in the pump/drag boot\n'; exit 1; \
fi
@printf '>>> test-flair-dc4v [1/4]: no triple-fault\n'
@grep -q '^FLAIR-LIVE-READY$$' "$(FLAIR_DC4V_SERIAL)" \
	|| { printf '!!! test-flair-dc4v FAIL: FLAIR-LIVE-READY missing -- the pump never armed\n'; exit 1; }
@printf '>>> test-flair-dc4v [2/4]: FLAIR-LIVE-READY (the WaitNextEvent pump is armed)\n'
@grep -q '^FLAIR-MODAL-DRAG (140,200)->(0,200)$$' "$(FLAIR_DC4V_SERIAL)" \
	|| { printf '!!! test-flair-dc4v FAIL: exact modal drag marker missing / wrong delta\n'; grep '^FLAIR-' "$(FLAIR_DC4V_SERIAL)" || true; exit 1; }
@printf '>>> test-flair-dc4v [3/4]: modal moved (140,200)->(0,200) across window 1\n'
@if [ ! -s "$(FLAIR_DC4V_PPM)" ]; then printf '!!! test-flair-dc4v FAIL: no screendump captured at %s\n' "$(FLAIR_DC4V_PPM)"; exit 1; fi
@$(PPM_FLAIR_DC4V_CHECK_BIN) "$(FLAIR_DC4V_PPM)" \
	|| { printf '!!! test-flair-dc4v FAIL: reverse modal/window/menu survival grader RED\n'; exit 1; }
@printf '>>> test-flair-dc4v [4/4]: modal intact + vacated window restored + bars intact\n'
@printf 'VERDICT   : PASS -- reverse modal survival and restoration contract holds\n'
@printf '======================================================================\n'
```

## test-flair-dc4v-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 19984):
Prerequisites: `build/qemu_harness build/flair_live_mut_modal_restore.img build/ppm_flair_dc4v_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-dc4v-mutant : Rule 6 (the gate BITES)\n'
@printf '  Mutant: -DFLAIR_LIVE_MUTATE_MODAL_NO_RESTORE. Modal moves/new rect draws,\n'
@printf '  but old footprint stays stale -> vacated window-content LEG V RED.\n'
@printf '======================================================================\n'
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_MUT_MODAL_RESTORE_IMG)" --name flair_dc4v_mut --out "$(BUILD)" \
	--mouse "$(FLAIR_DC4V_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-MODAL-DRAG" --timeout-ms 15000 >/dev/null 2>&1 || true
@grep -q '^FLAIR-LIVE-READY$$' "$(BUILD)/flair_dc4v_mut.serial" \
	|| { printf '!!! test-flair-dc4v-mutant: mutant did not reach FLAIR-LIVE-READY (not comparable)\n'; exit 1; }
@grep -q '^FLAIR-MODAL-DRAG (140,200)->(0,200)$$' "$(BUILD)/flair_dc4v_mut.serial" \
	|| { printf '!!! test-flair-dc4v-mutant: exact modal marker missing (not comparable)\n'; exit 1; }
@if [ ! -s "$(BUILD)/flair_dc4v_mut.ppm" ]; then printf '!!! test-flair-dc4v-mutant: no screendump captured (cannot judge the mutant)\n'; exit 1; fi
@if $(PPM_FLAIR_DC4V_CHECK_BIN) "$(BUILD)/flair_dc4v_mut.ppm" >/dev/null 2>&1; then \
	printf '!!! test-flair-dc4v-mutant FAIL: MODAL_NO_RESTORE passed reverse survival -- oracle decoration\n'; exit 1; \
fi
@printf '>>> test-flair-dc4v-mutant: RED as required -- vacated window band retained stale modal pixels\n'
@printf '======================================================================\n'
```

## test-flair-desktop

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 19485):
Prerequisites: `test-color-canon-gen test-color-canon build/qemu_harness build/flair_desktop.img build/ppm_flair_check build/ppm_flair_check_her02 build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-desktop : FLAIR LIVE DESKTOP oracle\n'
@printf '  Ref: ADR-0004 (the desktop shell); PRD Sec 1 / Appendix A (the frame).\n'
@printf '  beads initech-re30.3 LANE 2. CLAUDE.md Law 2/4, Rule 2/6/11.\n'
@printf '  TRI-EMULATOR: QEMU here; Bochs 8bpp leg = make test-flair-desktop-bochs.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (live FLAIR chimera desktop presented to the LFB)\n' "$(FLAIRSHELL_IMG)"
@printf 'Expecting : FLAIR-DESKTOP serial marker + screendump == Office Space frame\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(HARNESS_BIN) --disk "$(FLAIRSHELL_IMG)" --screendump --screendump-after FLAIR-DESKTOP \
	--name "$(FLAIR_DESKTOP_NAME)" --out "$(BUILD)" --timeout-ms 8000 \
	2> "$(FLAIR_DESKTOP_REPORT)" || true
@cat "$(FLAIR_DESKTOP_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@# ---- 1. No triple-fault. ----
@if grep -q 'triple_fault=1' "$(FLAIR_DESKTOP_REPORT)"; then \
	printf '!!! test-flair-desktop FAIL: TRIPLE FAULT in the live desktop boot\n'; \
	exit 1; \
fi
@printf '>>> test-flair-desktop [1/3]: no triple-fault\n'
@# ---- 2. FLAIR-DESKTOP serial marker (the desktop composed + presented). ----
@if [ ! -s "$(FLAIR_DESKTOP_SERIAL)" ]; then \
	printf '!!! test-flair-desktop FAIL: no serial captured at %s\n' "$(FLAIR_DESKTOP_SERIAL)"; \
	exit 1; \
fi
@grep -q '^FLAIR-DESKTOP$$' "$(FLAIR_DESKTOP_SERIAL)" \
	|| { printf '!!! test-flair-desktop FAIL: FLAIR-DESKTOP marker missing -- the desktop never composed/presented\n'; exit 1; }
@printf '>>> test-flair-desktop [2/3]: FLAIR-DESKTOP marker present on serial\n'
@# ---- 3. Screendump structure: the Office Space frame. ----
@if [ ! -s "$(FLAIR_DESKTOP_PPM)" ]; then \
	printf '!!! test-flair-desktop FAIL: no screendump captured at %s (live guest required)\n' "$(FLAIR_DESKTOP_PPM)"; \
	exit 1; \
fi
@$(PPM_FLAIR_CHECK_BIN) "$(FLAIR_DESKTOP_PPM)" \
	|| { printf '!!! test-flair-desktop FAIL: the live desktop screendump is NOT the Office Space frame structure\n'; exit 1; }
@printf '>>> test-flair-desktop [3/3]: screendump == teal desktop + TWO menu bars + window chrome + FILE COPY modal\n'
@# ---- [+] HER-02 boundary (ADR-0010): ppm_flair_check STRUCTURE is value-BLIND.
@#         Run the demo build on the SAME teal dump; a teal->seafoam value
@#         perturbation must leave the structural relations GREEN (else a probe
@#         secretly reads the absolute desktop RGB). The value authority is
@#         test-color-canon (run as the precondition above), not this file. ----
@$(PPM_FLAIR_HER02_BIN) "$(FLAIR_DESKTOP_PPM)" >/dev/null \
	|| { printf '!!! test-flair-desktop FAIL: HER-02 demo -- a STRUCTURE probe secretly depends on the absolute desktop RGB (value/structure boundary broken)\n'; exit 1; }
@printf '>>> test-flair-desktop [+]: HER-02 boundary -- structure value-blind (color VALUES graded by test-color-canon, not by construction)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- the LIVE FLAIR chimera desktop is the Office Space frame\n'
@printf '            (QEMU only; Bochs 8bpp leg = make test-flair-desktop-bochs)\n'
@printf '======================================================================\n'
```

## test-flair-desktop-8bpp

Declared tier: EMU; #  recipe to execute (from 'harness/emu/vesa8.mk', line 87):
Prerequisites: `test-flair-dac test-boot-vesa-images build/qemu_harness build/vesa8/scene.img build/vesa8/flagship.img build/flair_data.img build/ppm_flair_check build/ppm_flair_appswitch_check build/ppm_flair_depth_check build/.makefile.stamp`
```make
$(call vesa8-capture,scene,$(FLAIRSHELL_IMG),scene32)
$(call vesa8-capture,scene,$(VESA8_DIR)/scene.img,scene8)
@$(BUILD)/ppm_flair_depth_check $(VESA8_DIR)/vesa8-scene32.ppm $(VESA8_DIR)/vesa8-scene8.ppm
$(call vesa8-capture,pre,$(FLAIRTENANTS_IMG),pre32)
$(call vesa8-capture,pre,$(VESA8_DIR)/flagship.img,pre8)
$(call vesa8-capture,post,$(FLAIRTENANTS_IMG),post32)
$(call vesa8-capture,post,$(VESA8_DIR)/flagship.img,post8)
@$(PPM_FLAIR_APPSWITCH_CHECK_BIN) $(VESA8_DIR)/vesa8-pre8.ppm $(VESA8_DIR)/vesa8-post8.ppm
@$(BUILD)/ppm_flair_depth_check $(VESA8_DIR)/vesa8-pre32.ppm $(VESA8_DIR)/vesa8-pre8.ppm
@$(BUILD)/ppm_flair_depth_check $(VESA8_DIR)/vesa8-post32.ppm $(VESA8_DIR)/vesa8-post8.ppm
@echo 'GREEN test-flair-desktop-8bpp: canonical checker + flagship locked app switch + exact RGB equality'
```

## test-flair-desktop-8bpp-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 147):
Prerequisites: `build/bochs_harness build/vesa8/flagship.img build/flair_data.img test-flair-desktop-8bpp-bochs-pixel build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-desktop-8bpp-bochs : BOCHS boot leg of %s (%s video path)\n' "build/vesa8/flagship.img" "vbe101pad"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-desktop-8bpp-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-desktop-8bpp_bochs_data.img
@$(BOCHS_BIN) --disk "build/vesa8/flagship.img" --disk2 "$(BUILD)/flair-desktop-8bpp_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-desktop-8bpp_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-desktop-8bpp_bochs.report.txt" || true
@cat "$(BUILD)/flair-desktop-8bpp_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-desktop-8bpp-bochs "$(BUILD)/flair-desktop-8bpp_bochs.report.txt" "$(BUILD)/flair-desktop-8bpp_bochs.serial" vbe101pad FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-desktop-8bpp-bochs-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 157):
Prerequisites: `test-flair-desktop-8bpp-bochs-pixel build/vesa8/mut_omit119.img build/vesa8/mut_omit165.img build/vesa8/mut_rowcopy.img build/vesa8/mut_dac6.img build/vesa8/mut_chan.img build/.makefile.stamp`
```make
@set -eu; for m in $(VESA8_MUTANTS); do \
 if $(call bochs-pixel-leg,bochs_mut_$$m,$(VESA8_DIR)/mut_$$m.img,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),$(BOCHS_PIXEL_TENANTS_LINE)) > $(BUILD)/bochs-pixel/mut_$$m.red 2>&1; then echo "FAIL $$m survived under Bochs"; cat $(BUILD)/bochs-pixel/mut_$$m.red; exit 1; fi; \
 grep -q "^RED bochs-pixel bochs_mut_$$m:" $(BUILD)/bochs-pixel/mut_$$m.red || { cat $(BUILD)/bochs-pixel/mut_$$m.red; echo "FAIL $$m: capture broken, not a pixel RED"; exit 1; }; \
 printf '%s: ' "$$m"; grep -m1 '^RED bochs-exact RGB ' $(BUILD)/bochs-pixel/mut_$$m.red; done
@echo 'GREEN test-flair-desktop-8bpp-bochs-mutant: omissions 119/165, row copy, DAC precision and channel order each RED under Bochs'
```

## test-flair-desktop-8bpp-bochs-pixel

Declared tier: outside active vector; #  recipe to execute (from 'harness/bochs_legs.mk', line 154):
Prerequisites: `build/bochs_harness build/qemu_harness build/ppm_bochs_check build/vesa8/flagship.img build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@$(call bochs-pixel-leg,flair_desktop_8bpp,$(VESA8_DIR)/flagship.img,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),$(BOCHS_PIXEL_TENANTS_LINE))
@sh harness/emu/bochs_leg_check.sh test-flair-desktop-8bpp-bochs-pixel $(BUILD)/bochs-pixel/flair_desktop_8bpp-bochs.report $(BUILD)/bochs-pixel/flair_desktop_8bpp-bochs.serial vbe101pad FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP '$(BOCHS_PIXEL_TENANTS_LINE)' !PANIC
```

## test-flair-desktop-8bpp-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/vesa8.mk', line 110):
Prerequisites: `test-flair-desktop-8bpp build/vesa8/mut_omit119.img build/vesa8/mut_omit165.img build/vesa8/mut_rowcopy.img build/vesa8/mut_dac6.img build/vesa8/mut_chan.img build/.makefile.stamp`
```make
@set -eu; for m in $(VESA8_MUTANTS); do \
 sh harness/emu/vesa8.sh $(BUILD) pre $(VESA8_DIR)/mut_$$m.img mut_$$m $(FLAIR_DATA_IMG); \
 if $(BUILD)/ppm_flair_depth_check $(VESA8_DIR)/vesa8-pre32.ppm $(VESA8_DIR)/vesa8-mut_$$m.ppm > $(VESA8_DIR)/$$m.red 2>&1; then echo "FAIL $$m survived"; exit 1; fi; \
 printf '%s: ' "$$m"; grep '^RED flair-depth RGB ' $(VESA8_DIR)/$$m.red; done
@echo 'GREEN test-flair-desktop-8bpp-mutant: omissions 119/165, row copy, DAC precision and channel order each RED'
```

## test-flair-desktop-bochs

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 19628):
Prerequisites: `build/bochs_harness build/qemu_harness build/ppm_bochs_check build/flair_desktop.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-desktop-bochs : BOCHS leg (FLAIR desktop)\n'
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator). beads initech-re30.3 LANE 2; WL-0170.\n'
@printf '  Bochs 2.7: legacy BIOS + LGPL vgabios; VESA 0101h 640x480x8 LFB + 8-bit DAC.\n'
@printf '  The leg proves the shared boot/heap milestones AND grades the presented\n'
@printf '  desktop against the QEMU 32bpp capture of the same image: EXACT RGB.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-desktop-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2). Set SKIP_BOCHS=1 to explicitly (and loudly) opt out.\n'; exit 1; }
@printf 'Booting   : %s under QEMU (32bpp golden) then Bochs (0101h; RFB headless)\n' "$(FLAIRSHELL_IMG)"
@printf 'Expecting : VBE-MODE=0101 + shared kernel markers + FLAIR-DESKTOP, then\n'
@printf '            Bochs == QEMU on all 307200 pixels (snapshot exact + RFB BGR233)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call bochs-pixel-leg,$(FLAIR_BOCHS_NAME),$(FLAIRSHELL_IMG),$(FLAIRSHELL_IMG),-,FLAIR-DESKTOP)
@cat "$(FLAIR_BOCHS_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@sh harness/emu/bochs_leg_check.sh test-flair-desktop-bochs "$(FLAIR_BOCHS_REPORT)" "$(FLAIR_BOCHS_SERIAL)" vbe101 \
	FLAIR-HEAP-OK FLAIR-DESKTOP !PANIC
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- the flair_desktop kernel booted under Bochs through the\n'
@printf '            shared kernel + FLAIR-heap milestones (== QEMU) on the VESA 0101h\n'
@printf '            path and PRESENTED the desktop: Bochs == QEMU pixel for pixel.\n'
@printf '            (The mode-13h guard is graded by test-flair-desktop-vga13-bochs.)\n'
@printf '======================================================================\n'
```

## test-flair-desktop-icons

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 20524):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_desktop_icons_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-desktop-icons : THE booted DesktopMgr (R3.2)\n'
@printf '  volume + Trash icons on the desktop; select / deselect / marquee / open / drag+persist.\n'
@printf '  Ref: bead initech-tdnl.9; os/flair/finder_desktop.h; spec/flair_desktop_icons_traces.mk.\n'
@printf '  Grader probes are hand-read off spec/assets/desk_icons.h ASCII maps (Law 2).\n'
@printf '======================================================================\n'
@command -v mdir    >/dev/null 2>&1 || { printf '!!! test-flair-desktop-icons FAIL: mdir missing (mtools)\n'; exit 1; }
@command -v mattrib >/dev/null 2>&1 || { printf '!!! test-flair-desktop-icons FAIL: mattrib missing (mtools)\n'; exit 1; }
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@# ---- leg 1: PRE, the boot scene, no mouse. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_DI_PRE_NAME)" --out "$(BUILD)" \
	--screendump --screendump-after "FLAIR-TENANTS-READY" --timeout-ms 15000 \
	2> "$(BUILD)/$(FLAIR_DI_PRE_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_DI_PRE_NAME).report"; then printf '!!! test-flair-desktop-icons FAIL: TRIPLE FAULT in the PRE boot\n'; exit 1; fi
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/$(FLAIR_DI_PRE_NAME).serial" \
	|| { printf '!!! test-flair-desktop-icons FAIL: FLAIR-FAT-MOUNT-OK missing in the PRE boot\n'; exit 1; }
@grep -q '^FINDER-DESKTOP-ICONS n=2$$' "$(BUILD)/$(FLAIR_DI_PRE_NAME).serial" \
	|| { printf '!!! test-flair-desktop-icons FAIL: the Finder shell tenant did not seed 2 desktop icons\n'; grep '^FINDER-' "$(BUILD)/$(FLAIR_DI_PRE_NAME).serial" || true; exit 1; }
@if [ ! -s "$(BUILD)/$(FLAIR_DI_PRE_NAME).ppm" ]; then printf '!!! test-flair-desktop-icons FAIL: PRE screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_DESKICONS_CHECK_BIN) default "$(BUILD)/$(FLAIR_DI_PRE_NAME).ppm" \
	|| { printf '!!! test-flair-desktop-icons FAIL: the boot desktop does not carry both icon strikes\n'; exit 1; }
@printf '>>> test-flair-desktop-icons [1/7]: boot scene -- FINDER-DESKTOP-ICONS n=2 + grader leg DEFAULT\n'
@# ---- leg 2: click the volume icon -> SELECTED (label band inverted). ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_DI_SEL_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_ICON_SELECT_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FINDER-ICON-SELECT name=INITECH count=1" \
	--timeout-ms 15000 2> "$(BUILD)/$(FLAIR_DI_SEL_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_DI_SEL_NAME).report"; then printf '!!! test-flair-desktop-icons FAIL: TRIPLE FAULT in the SELECT boot\n'; exit 1; fi
@grep -q '^FINDER-ICON-SELECT name=INITECH count=1$$' "$(BUILD)/$(FLAIR_DI_SEL_NAME).serial" \
	|| { printf '!!! test-flair-desktop-icons FAIL: the click on the volume sprite did not select it\n'; grep '^FINDER-' "$(BUILD)/$(FLAIR_DI_SEL_NAME).serial" || true; exit 1; }
@if [ ! -s "$(BUILD)/$(FLAIR_DI_SEL_NAME).ppm" ]; then printf '!!! test-flair-desktop-icons FAIL: SELECT screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_DESKICONS_CHECK_BIN) selected "$(BUILD)/$(FLAIR_DI_SEL_NAME).ppm" \
	|| { printf '!!! test-flair-desktop-icons FAIL: the selected volume label band is not inverted\n'; exit 1; }
@printf '>>> test-flair-desktop-icons [2/7]: FINDER-ICON-SELECT name=INITECH count=1 + grader leg SELECTED\n'
@# ---- leg 3: then click bare desktop -> DESELECT + the band restored. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_DI_DESEL_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_ICON_DESELECT_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FINDER-ICON-DESELECT-ALL" \
	--timeout-ms 15000 2> "$(BUILD)/$(FLAIR_DI_DESEL_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_DI_DESEL_NAME).report"; then printf '!!! test-flair-desktop-icons FAIL: TRIPLE FAULT in the DESELECT boot\n'; exit 1; fi
@grep -q '^FINDER-ICON-SELECT name=INITECH count=1$$' "$(BUILD)/$(FLAIR_DI_DESEL_NAME).serial" \
	&& grep -qxF 'FINDER-ICON-DESELECT-ALL' "$(BUILD)/$(FLAIR_DI_DESEL_NAME).serial" \
	|| { printf '!!! test-flair-desktop-icons FAIL: select-then-bare-desktop-click did not clear the selection\n'; grep '^FINDER-' "$(BUILD)/$(FLAIR_DI_DESEL_NAME).serial" || true; exit 1; }
@if [ ! -s "$(BUILD)/$(FLAIR_DI_DESEL_NAME).ppm" ]; then printf '!!! test-flair-desktop-icons FAIL: DESELECT screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_DESKICONS_CHECK_BIN) default "$(BUILD)/$(FLAIR_DI_DESEL_NAME).ppm" \
	|| { printf '!!! test-flair-desktop-icons FAIL: the deselect did not repaint the label band back\n'; exit 1; }
@printf '>>> test-flair-desktop-icons [3/7]: FINDER-ICON-DESELECT-ALL + grader leg DEFAULT (band restored)\n'
@# ---- leg 4: rubber band over BOTH icon cells. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_DI_BAND_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_RUBBER_BAND_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--quit-after "FINDER-MARQUEE n=2" \
	--timeout-ms 15000 2> "$(BUILD)/$(FLAIR_DI_BAND_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_DI_BAND_NAME).report"; then printf '!!! test-flair-desktop-icons FAIL: TRIPLE FAULT in the MARQUEE boot\n'; exit 1; fi
@grep -q '^FINDER-MARQUEE n=2$$' "$(BUILD)/$(FLAIR_DI_BAND_NAME).serial" \
	|| { printf '!!! test-flair-desktop-icons FAIL: the rubber band did not select BOTH icons\n'; grep '^FINDER-' "$(BUILD)/$(FLAIR_DI_BAND_NAME).serial" || true; exit 1; }
@printf '>>> test-flair-desktop-icons [4/7]: FINDER-MARQUEE n=2 (the band swept both icon columns)\n'
@# ---- leg 5: double-click the volume icon. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_DI_OPEN_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_ICON_OPEN_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--quit-after "FINDER-OPEN-VOLUME win=0 n=4" \
	--timeout-ms 15000 2> "$(BUILD)/$(FLAIR_DI_OPEN_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_DI_OPEN_NAME).report"; then printf '!!! test-flair-desktop-icons FAIL: TRIPLE FAULT in the OPEN boot\n'; exit 1; fi
@grep -qxF 'FINDER-OPEN-VOLUME win=0 n=4' "$(BUILD)/$(FLAIR_DI_OPEN_NAME).serial" \
	|| { printf '!!! test-flair-desktop-icons FAIL: the double-click did not open the root disk window with the expected 4-icon listing. Two failure axes, and the serial below says which: (a) NO FINDER-OPEN-VOLUME line at all = the double-click synthesiser did not fire (the measured delta was 8 ticks vs FINDER_DBLCLICK_TICKS=20 when this gate landed -- if the harness pacing has slowed, fix the harness or the trace, NEVER the constant); (b) a FINDER-OPEN-VOLUME line with a different win=/n= = the window opened but the enumeration changed (a file added to the data volume, or the VOLLABEL skip broke)\n'; grep '^FINDER-' "$(BUILD)/$(FLAIR_DI_OPEN_NAME).serial" || true; exit 1; }
@! grep -q '^FINDER-WIN-OPEN-FAIL' "$(BUILD)/$(FLAIR_DI_OPEN_NAME).serial" \
	|| { printf '!!! test-flair-desktop-icons FAIL: the disk window reported an open failure\n'; grep '^FINDER-' "$(BUILD)/$(FLAIR_DI_OPEN_NAME).serial" || true; exit 1; }
@printf '>>> test-flair-desktop-icons [5/7]: FINDER-OPEN-VOLUME win=0 n=4 (the double-click opened the REAL root disk window)\n'
@# ---- leg 6: drag the volume icon to (400,400) and persist. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_DI_DRAG_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_ICON_DRAGDROP_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "DESKTOP-DB-SAVE n=2" \
	--timeout-ms 20000 2> "$(BUILD)/$(FLAIR_DI_DRAG_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_DI_DRAG_NAME).report"; then printf '!!! test-flair-desktop-icons FAIL: TRIPLE FAULT in the DRAG boot\n'; exit 1; fi
@grep -qxF 'FINDER-ICON-DRAG name=INITECH x=400 y=400' "$(BUILD)/$(FLAIR_DI_DRAG_NAME).serial" \
	|| { printf '!!! test-flair-desktop-icons FAIL: the icon drag did not commit the locked drop origin (400,400)\n'; grep '^FINDER-' "$(BUILD)/$(FLAIR_DI_DRAG_NAME).serial" || true; exit 1; }
@grep -qxF 'DESKTOP-DB-SAVE n=2' "$(BUILD)/$(FLAIR_DI_DRAG_NAME).serial" \
	|| { printf '!!! test-flair-desktop-icons FAIL: the drop did not persist both records to DESKTOP.DB\n'; exit 1; }
@! grep -q '^DESKTOP-DB-WRITE-FAIL' "$(BUILD)/$(FLAIR_DI_DRAG_NAME).serial" \
	|| { printf '!!! test-flair-desktop-icons FAIL: DESKTOP-DB-WRITE-FAIL during the drop\n'; exit 1; }
@if [ ! -s "$(BUILD)/$(FLAIR_DI_DRAG_NAME).ppm" ]; then printf '!!! test-flair-desktop-icons FAIL: DRAG screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_DESKICONS_CHECK_BIN) moved "$(BUILD)/$(FLAIR_DI_DRAG_NAME).ppm" \
	|| { printf '!!! test-flair-desktop-icons FAIL: after the drop the icon is not at the new origin with its old cell restored\n'; exit 1; }
@printf '>>> test-flair-desktop-icons [6/7]: FINDER-ICON-DRAG x=400 y=400 + DESKTOP-DB-SAVE n=2 + grader leg MOVED\n'
@# ---- leg 7: PERSISTENCE. Reboot the SAME, now-written data volume. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-LIVE-OK --name "$(FLAIR_DI_PERS_NAME)" --out "$(BUILD)" \
	--screendump --screendump-after "FLAIR-LIVE-OK" \
	--timeout-ms 15000 2> "$(BUILD)/$(FLAIR_DI_PERS_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_DI_PERS_NAME).report"; then printf '!!! test-flair-desktop-icons FAIL: TRIPLE FAULT in the PERSIST boot\n'; exit 1; fi
@grep -qxF 'DESKTOP-DB-OK' "$(BUILD)/$(FLAIR_DI_PERS_NAME).serial" \
	|| { printf '!!! test-flair-desktop-icons FAIL: the reboot did not read back a valid DESKTOP.DB\n'; grep '^DESKTOP-DB' "$(BUILD)/$(FLAIR_DI_PERS_NAME).serial" || true; exit 1; }
@if grep -q '^DESKTOP-DB-CREATE' "$(BUILD)/$(FLAIR_DI_PERS_NAME).serial" || \
    grep -q '^DESKTOP-DB-REGEN' "$(BUILD)/$(FLAIR_DI_PERS_NAME).serial" || \
    grep -q '^DESKTOP-DB-POS-REGEN' "$(BUILD)/$(FLAIR_DI_PERS_NAME).serial" || \
    grep -q '^DESKTOP-DB-POS-SKIP' "$(BUILD)/$(FLAIR_DI_PERS_NAME).serial"; then \
	printf '!!! test-flair-desktop-icons FAIL: the reboot re-created/regenerated DESKTOP.DB instead of reading the saved positions\n'; \
	grep '^DESKTOP-DB' "$(BUILD)/$(FLAIR_DI_PERS_NAME).serial" || true; exit 1; \
fi
@if [ ! -s "$(BUILD)/$(FLAIR_DI_PERS_NAME).ppm" ]; then printf '!!! test-flair-desktop-icons FAIL: PERSIST screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_DESKICONS_CHECK_BIN) moved "$(BUILD)/$(FLAIR_DI_PERS_NAME).ppm" \
	|| { printf '!!! test-flair-desktop-icons FAIL: the dragged position did NOT survive the reboot\n'; exit 1; }
@# mtools sanity on the written volume: 8-byte header + 2 x 24-byte records,
@# still hidden. The BYTES are graded by the host codec oracle
@# (test-finder-desktop) against hand-authored expected records; this is the
@# on-disk shape/attribute check only (Rule 8 / the tdnl.8 mtools idiom).
@mdir -a -i $(FLAIR_GATE_DATA) :: > "$(BUILD)/$(FLAIR_DI_PERS_NAME).mdir"
@grep -Eq '^DESKTOP +DB +56 ' "$(BUILD)/$(FLAIR_DI_PERS_NAME).mdir" \
	|| { printf '!!! test-flair-desktop-icons FAIL: mdir does not show a 56-byte DESKTOP.DB (8 + 24*2)\n'; cat "$(BUILD)/$(FLAIR_DI_PERS_NAME).mdir"; exit 1; }
@mattrib -i $(FLAIR_GATE_DATA) ::DESKTOP.DB 2>/dev/null | grep -Eq '^ +H ' \
	|| { printf '!!! test-flair-desktop-icons FAIL: DESKTOP.DB lost its hidden attribute across the write\n'; exit 1; }
@printf '>>> test-flair-desktop-icons [7/7]: DESKTOP-DB-OK + grader leg MOVED after reboot + mdir 56-byte hidden DESKTOP.DB\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- the booted Finder draws, selects, marquees, opens, drags and\n'
@printf '            PERSISTS its desktop icons; the position survives a power cycle\n'
@printf '            (QEMU; Bochs boot + pixel leg = make test-flair-desktop-icons-bochs)\n'
@printf '======================================================================\n'
```

## test-flair-desktop-icons-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 179):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img test-flair-desktop-icons-bochs-pixel build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-desktop-icons-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-desktop-icons-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-desktop-icons_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-desktop-icons_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-desktop-icons_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-desktop-icons_bochs.report.txt" || true
@cat "$(BUILD)/flair-desktop-icons_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-desktop-icons-bochs "$(BUILD)/flair-desktop-icons_bochs.report.txt" "$(BUILD)/flair-desktop-icons_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK DESKTOP-DB-CREATE TRASH-CREATE FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-desktop-icons-bochs-pixel

Declared tier: outside active vector; #  recipe to execute (from 'harness/bochs_legs.mk', line 187):
Prerequisites: `build/bochs_harness build/qemu_harness build/ppm_bochs_check build/ppm_flair_desktop_icons_check build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(call bochs-pixel-leg,flair_desktop_icons,$(FLAIRTENANTS_IMG),$(FLAIRTENANTS_IMG),$(FLAIR_GATE_DATA),$(BOCHS_PIXEL_TENANTS_LINE))
@sh harness/emu/bochs_leg_check.sh test-flair-desktop-icons-bochs-pixel $(BUILD)/bochs-pixel/flair_desktop_icons-bochs.report $(BUILD)/bochs-pixel/flair_desktop_icons-bochs.serial vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK DESKTOP-DB-CREATE TRASH-CREATE FLAIR-DESKTOP 'FINDER-DESKTOP-ICONS n=2' '$(BOCHS_PIXEL_TENANTS_LINE)' !PANIC
@$(PPM_FLAIR_DESKICONS_CHECK_BIN) default $(FLAIR_DI_BOCHS_SNAP) \
	|| { printf '!!! test-flair-desktop-icons-bochs-pixel FAIL: the Bochs desktop does not carry both icon strikes\n'; exit 1; }
```

## test-flair-desktop-icons-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 20678):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_desktop_icons_check build/flair_tenants_mut_desk_no_underlay.img build/flair_tenants_mut_desk_dbltick_off.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-desktop-icons-mutant : Rule 6 (the gate BITES)\n'
@printf '  2 FLAIRTENANTS finder_desktop.c mutants; each MUST drive its leg RED.\n'
@printf '======================================================================\n'
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@# ---- baseline: the CLEAN image must grade GREEN on both mutated legs. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name flair_deskicons_mut_base --out "$(BUILD)" \
	--screendump --screendump-after "FLAIR-TENANTS-READY" --timeout-ms 15000 >/dev/null 2>&1 || true
@if [ ! -s "$(BUILD)/flair_deskicons_mut_base.ppm" ]; then printf '!!! test-flair-desktop-icons-mutant FAIL: clean baseline screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_DESKICONS_CHECK_BIN) default "$(BUILD)/flair_deskicons_mut_base.ppm" >/dev/null 2>&1 \
	|| { printf '!!! test-flair-desktop-icons-mutant FAIL: the CLEAN image did not grade GREEN on leg DEFAULT -- the baseline is broken (not a mutant)\n'; exit 1; }
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name flair_deskicons_mut_base_open --out "$(BUILD)" \
	--mouse "$(FLAIR_ICON_OPEN_SPEC)" --keys-after "FLAIR-LIVE-READY" --timeout-ms 15000 \
	--quit-after "FINDER-OPEN-VOLUME win=0 n=4" >/dev/null 2>&1 || true
@grep -qxF 'FINDER-OPEN-VOLUME win=0 n=4' "$(BUILD)/flair_deskicons_mut_base_open.serial" \
	|| { printf '!!! test-flair-desktop-icons-mutant FAIL: the CLEAN image did not emit FINDER-OPEN-VOLUME win=0 n=4 -- the baseline is broken\n'; exit 1; }
@printf '>>> baseline: the clean FLAIRTENANTS image grades GREEN on leg DEFAULT and opens on a double-click\n'
@printf '%s\n' '----------------------------------------------------------------------'
@# ---- mutant 1: NO_UNDERLAY -- the DEFAULT-leg grader MUST go RED. ----
@$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_desk_no_underlay.img" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name flair_deskicons_mut_no_underlay --out "$(BUILD)" \
	--screendump --screendump-after "FLAIR-TENANTS-READY" --timeout-ms 15000 \
	2> "$(BUILD)/flair_deskicons_mut_no_underlay.report" || true
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_deskicons_mut_no_underlay.serial" \
	|| { printf '!!! test-flair-desktop-icons-mutant FAIL: no_underlay mutant missing mount marker (not comparable)\n'; exit 1; }
@if grep -q 'triple_fault=1' "$(BUILD)/flair_deskicons_mut_no_underlay.report"; then printf '!!! test-flair-desktop-icons-mutant FAIL: no_underlay TRIPLE-FAULTED (cannot judge the oracle)\n'; exit 1; fi
@if [ ! -s "$(BUILD)/flair_deskicons_mut_no_underlay.ppm" ]; then printf '!!! test-flair-desktop-icons-mutant FAIL: no_underlay produced no screendump\n'; exit 1; fi
@if $(PPM_FLAIR_DESKICONS_CHECK_BIN) default "$(BUILD)/flair_deskicons_mut_no_underlay.ppm" > "$(BUILD)/flair_deskicons_mut_no_underlay.chk" 2>&1; then \
	printf '!!! test-flair-desktop-icons-mutant FAIL: the DEFAULT-leg oracle is DECORATION -- NO_UNDERLAY PASSED it\n'; exit 1; \
else \
	printf '>>> mutant desk_no_underlay correctly RED (the icons never reach the frame):\n'; \
	grep -m2 'FAIL leg' "$(BUILD)/flair_deskicons_mut_no_underlay.chk" | sed 's/^/      /'; \
fi
@# ---- mutant 2: DBLTICK_OFF -- the OPEN marker MUST be absent. ----
@# initech-qed1: --legacy-quit -- the dispatch (or its absence) comes from the
@# FINAL click of the same already-injected mouse burst that produces the
@# EARLIER FINDER-ICON-SELECT marker; quitting right after SELECT would not
@# reliably give the broken double-click threshold time to conclusively NOT
@# fire, so no honest quit-after marker exists for this absence-final leg.
@$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_desk_dbltick_off.img" --disk2 "$(FLAIR_GATE_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name flair_deskicons_mut_dbltick_off --out "$(BUILD)" \
	--mouse "$(FLAIR_ICON_OPEN_SPEC)" --keys-after "FLAIR-LIVE-READY" --timeout-ms 15000 --legacy-quit \
	2> "$(BUILD)/flair_deskicons_mut_dbltick_off.report" || true
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_deskicons_mut_dbltick_off.serial" \
	|| { printf '!!! test-flair-desktop-icons-mutant FAIL: dbltick_off mutant missing mount marker (not comparable)\n'; exit 1; }
@if grep -q 'triple_fault=1' "$(BUILD)/flair_deskicons_mut_dbltick_off.report"; then printf '!!! test-flair-desktop-icons-mutant FAIL: dbltick_off TRIPLE-FAULTED (cannot judge the oracle)\n'; exit 1; fi
@grep -q '^FINDER-ICON-SELECT name=INITECH count=1$$' "$(BUILD)/flair_deskicons_mut_dbltick_off.serial" \
	|| { printf '!!! test-flair-desktop-icons-mutant FAIL: dbltick_off did not even deliver the clicks -- wrong failure axis\n'; exit 1; }
@if grep -q '^FINDER-OPEN-VOLUME ' "$(BUILD)/flair_deskicons_mut_dbltick_off.serial"; then \
	printf '!!! test-flair-desktop-icons-mutant FAIL: the leg-5 marker tooth is DECORATION -- DBLTICK_OFF still opened the volume\n'; exit 1; \
else \
	printf '>>> mutant desk_dbltick_off correctly RED: FINDER-OPEN-VOLUME is ABSENT; the double degraded into %s single clicks\n' \
		"$$(grep -c '^FINDER-ICON-SELECT ' "$(BUILD)/flair_deskicons_mut_dbltick_off.serial")"; \
fi
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- both R3.2 mutants drive their legs RED (the gate bites; Rule 6)\n'
@printf '======================================================================\n'
```

## test-flair-desktop-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 19548):
Prerequisites: `build/qemu_harness build/ppm_flair_check build/flair_shell.o build/flair_desktop_mut_onebar.img build/flair_desktop_mut_nomodal.img build/flair_desktop_mut_modalbehind.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-desktop-mutant : Rule 6 (the gate bites)\n'
@printf '  3 mutants of os/flair/shell.c, each MUST drive ppm_flair_check RED.\n'
@printf '======================================================================\n'
@rc=0; \
for spec in "onebar:SHELL_MUTATE_ONE_MENUBAR:dropped the 2nd (Photoshop) menu bar -- the two-bar chimera" \
            "nomodal:SHELL_MUTATE_NO_MODAL:skipped the FILE COPY modal -- the modal box + canon text" \
            "modalbehind:SHELL_MUTATE_MODAL_BEHIND:drew the modal BEHIND the windows -- the z-order"; do \
	tag=$${spec%%:*}; rest=$${spec#*:}; macro=$${rest%%:*}; desc=$${rest#*:}; \
	img="$(BUILD)/flair_desktop_mut_$$tag.img"; \
	printf '%s\n' '----------------------------------------------------------------------'; \
	printf '>>> mutant %s (-D%s): %s\n' "$$tag" "$$macro" "$$desc"; \
	$(HARNESS_BIN) --disk "$$img" --screendump --screendump-after FLAIR-DESKTOP \
		--name "flair_mut_$$tag" --out "$(BUILD)" --timeout-ms 8000 \
		2> "$(BUILD)/flair_mut_$$tag.report" || true; \
	if grep -q 'triple_fault=1' "$(BUILD)/flair_mut_$$tag.report"; then \
		printf '!!! test-flair-desktop-mutant FAIL: mutant %s TRIPLE-FAULTED (cannot judge the oracle)\n' "$$tag"; rc=1; continue; \
	fi; \
	if [ ! -s "$(BUILD)/flair_mut_$$tag.ppm" ]; then \
		printf '!!! test-flair-desktop-mutant FAIL: mutant %s produced no screendump\n' "$$tag"; rc=1; continue; \
	fi; \
	if $(PPM_FLAIR_CHECK_BIN) "$(BUILD)/flair_mut_$$tag.ppm" > "$(BUILD)/flair_mut_$$tag.chk" 2>&1; then \
		printf '!!! test-flair-desktop-mutant FAIL: the %s oracle is DECORATION -- mutant %s PASSED ppm_flair_check\n' "$$macro" "$$tag"; \
		rc=1; \
	else \
		printf '>>> mutant %s correctly RED:\n' "$$tag"; \
		grep -m3 'FAIL ' "$(BUILD)/flair_mut_$$tag.chk" | sed 's/^/      /'; \
	fi; \
done; \
if [ "$$rc" != "0" ]; then exit 1; fi
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- all 3 mutants drive ppm_flair_check RED (the oracle bites; Rule 6)\n'
@printf '======================================================================\n'
```

## test-flair-desktop-novbe-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 204):
Prerequisites: `build/bochs_harness build/flair_desktop.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-desktop-novbe-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_desktop.img" "e00"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-desktop-novbe-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
@true
@$(BOCHS_BIN) --disk "build/flair_desktop.img"  --vga-ext none --expect HALTED --quit-after HALTED --name "flair-desktop-novbe_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-desktop-novbe_bochs.report.txt" || true
@cat "$(BUILD)/flair-desktop-novbe_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-desktop-novbe-bochs "$(BUILD)/flair-desktop-novbe_bochs.report.txt" "$(BUILD)/flair-desktop-novbe_bochs.serial" e00 FLAIR-HEAP-OK 'PANIC flair-desktop: LFB smaller than 640x480 w=320 h=200' HALTED
```

## test-flair-desktop-vga13-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 202):
Prerequisites: `build/bochs_harness build/flair_desktop.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-desktop-vga13-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_desktop.img" "vga13"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-desktop-vga13-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@test -r /usr/share/vgabios/vgabios.cirrus.bin || { printf '!!! test-flair-desktop-vga13-bochs FAIL: ISA Cirrus VGA BIOS %s missing (apt install vgabios; bochs Depends: vgabios | seabios). A skipped oracle is worse than a red one (Law 2).\n' "/usr/share/vgabios/vgabios.cirrus.bin"; exit 1; }
@true
@$(BOCHS_BIN) --disk "build/flair_desktop.img"  --vga-ext cirrus --vgabios /usr/share/vgabios/vgabios.cirrus.bin --expect HALTED --quit-after HALTED --name "flair-desktop-vga13_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-desktop-vga13_bochs.report.txt" || true
@cat "$(BUILD)/flair-desktop-vga13_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-desktop-vga13-bochs "$(BUILD)/flair-desktop-vga13_bochs.report.txt" "$(BUILD)/flair-desktop-vga13_bochs.serial" vga13 FLAIR-HEAP-OK 'PANIC flair-desktop: LFB smaller than 640x480 w=320 h=200' HALTED
```

## test-flair-dialog-pressed-golden

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 333):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_hold.img build/flair_data.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
@for c in $(FINDER_PRESSED_CAPTURE) $(FINDER_PRESSED_CAPTURE_TL); do test -f $$c || { printf '!!! test-flair-dialog-pressed-golden RED: missing Mac OS 8.1 native held-OK capture: %s (re/mint-results-010.md)\n' "$$c"; exit 1; }; done
@$(MAKE) --no-print-directory $(addprefix -o ,$(FINDER_PRESSED_PROBE_DEPS) $(MAKEFILE_STAMP)) finder-dialog-pressed-probe
```

## test-flair-dialog-pressed-golden-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 347):
Prerequisites: `test-flair-dialog-pressed-golden build/flair_tenants_mut_press_accent.img build/.makefile.stamp`
```make
$(call appl-boot,mut_press_accent,$(BUILD)/flair_tenants_mut_press_accent.img,$(FLAIR_DATA_IMG),FLAIR_DIALOG_HELD_SPEC,--screendump --screendump-after "FINDER-TRASH-ALERT")
@$(call APPL_ALIVE,$(BUILD)/mut_press_accent.serial) || { printf '!!! mut_press_accent did not finish alive\n'; exit 1; }
@if $(PPM_FLAIR_TRASH_CHECK_BIN) pressed $(BUILD)/mut_press_accent.ppm $(FINDER_PRESSED_CAPTURE) $(FINDER_PRESSED_CAPTURE_TL) > $(BUILD)/mut_press_accent.pixels 2>&1; then printf '!!! test-flair-dialog-pressed-golden-mutant: CTRL_MUT_PRESS_ACCENT survived\n'; exit 1; fi
@grep 'FAIL PRESSED-CAPTURE' $(BUILD)/mut_press_accent.pixels
@printf '>>> test-flair-dialog-pressed-golden-mutant: CTRL_MUT_PRESS_ACCENT RED (accent press is not the native pressed button)\n'
```

## test-flair-dialog-tracking

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 78):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_hold.img build/flair_data.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
$(call appl-boot,dialog_tracking,$(FLAIRTENANTS_HOLD_IMG),$(FLAIR_DATA_IMG),FLAIR_DIALOG_TRACK_SPEC,--screendump --screendump-after "FINDER-TRASH-ALERT")
@$(call APPL_ALIVE,$(BUILD)/dialog_tracking.serial) || { printf '!!! dialog_tracking did not finish alive\n'; exit 1; }
@$(PPM_FLAIR_TRASH_CHECK_BIN) chrome $(BUILD)/dialog_tracking.ppm
@! grep -q '^FINDER-TRASH-EMPTIED' $(BUILD)/dialog_tracking.serial
@printf '>>> test-flair-dialog-tracking: green\n'
```

## test-flair-dialog-tracking-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 256):
Prerequisites: `test-flair-dialog-tracking build/flair_tenants_mut_finder_press_hold.img build/.makefile.stamp`
```make
$(call appl-boot,finder_press_mut,$(BUILD)/flair_tenants_mut_finder_press_hold.img,$(FLAIR_DATA_IMG),FLAIR_DIALOG_TRACK_SPEC,--screendump)
@$(call APPL_ALIVE,$(BUILD)/finder_press_mut.serial) || { printf '!!! finder_press_mut did not finish alive\n'; exit 1; }
@if $(PPM_FLAIR_TRASH_CHECK_BIN) chrome $(BUILD)/finder_press_mut.ppm > $(BUILD)/finder_press_mut.pixels 2>&1; then exit 1; fi
@grep 'FAIL ALERT-BANDS' $(BUILD)/finder_press_mut.pixels
@printf '>>> test-flair-dialog-tracking-mutant: DIALOG_MUT_ON_PRESS RED (release-outside lost alert)\n'
```

## test-flair-disk-windows

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 20935):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_disk_windows_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-disk-windows : THE booted DISK WINDOWS (R3.3)\n'
@printf '  open the volume; navigate a folder; the spatial singleton; the remembered\n'
@printf '  window origin across a power cycle; Ctrl-N makes a REAL FAT12 directory.\n'
@printf '  Ref: bead initech-tdnl.10; os/flair/finder_windows.h;\n'
@printf '       spec/flair_disk_windows_traces.mk (the locked traces + the arithmetic).\n'
@printf '  Grader probes are hand-read off spec/assets/finder_icons.h ASCII maps (Law 2).\n'
@printf '======================================================================\n'
@command -v mdir >/dev/null 2>&1 || { printf '!!! test-flair-disk-windows FAIL: mdir missing (mtools)\n'; exit 1; }
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_DW_DATA_A)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_DW_DATA_A),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_DW_DATA_B)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_DW_DATA_B),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_DW_DATA_C)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_DW_DATA_C),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_DW_DATA_D)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_DW_DATA_D),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@# ---- leg 1: open the volume; PIXEL-grade the opened root window. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_DW_DATA_A)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_DW_ROOT_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_ICON_OPEN_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FINDER-OPEN-VOLUME win=0 n=4" \
	--timeout-ms 20000 2> "$(BUILD)/$(FLAIR_DW_ROOT_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_DW_ROOT_NAME).report"; then printf '!!! test-flair-disk-windows FAIL: TRIPLE FAULT in the ROOTWIN boot\n'; exit 1; fi
@grep -qxF 'FINDER-OPEN-VOLUME win=0 n=4' "$(BUILD)/$(FLAIR_DW_ROOT_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: the double-click did not open the root disk window with the expected 4-icon listing\n'; grep '^FINDER-' "$(BUILD)/$(FLAIR_DW_ROOT_NAME).serial" || true; exit 1; }
@! grep -q '^FINDER-WIN-ICONS-FULL' "$(BUILD)/$(FLAIR_DW_ROOT_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: the per-window icon cap bit on a 4-entry root\n'; exit 1; }
@if [ ! -s "$(BUILD)/$(FLAIR_DW_ROOT_NAME).ppm" ]; then printf '!!! test-flair-disk-windows FAIL: ROOTWIN screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) rootwin "$(BUILD)/$(FLAIR_DW_ROOT_NAME).ppm" \
	|| { printf '!!! test-flair-disk-windows FAIL: the opened root window does not carry the derived chrome + the four grid icons\n'; exit 1; }
@printf '>>> test-flair-disk-windows [1/8]: FINDER-OPEN-VOLUME win=0 n=4 + grader leg ROOTWIN\n'
@# ---- leg 1b (bead initech-tdnl.12): the SAME dump, graded for BAND 2. ----
@# Opening a disk window makes the Finder the FOREGROUND TENANT, so band 2
@# must have swapped from HELLO's Photoshop bar to the Finder's own bar.
@# No sixth boot: the rootwin dump already shows it.
@grep -qxF 'FLAIR-DISPATCH app=FINDER' "$(BUILD)/$(FLAIR_DW_ROOT_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: opening the root disk window did NOT promote the Finder to the foreground tenant -- no FLAIR-DISPATCH app=FINDER. Band 2 would still be showing the previous tenant menu bar (bead initech-tdnl.12; flair_live_finder_foreground)\n'; grep -E '^FLAIR-DISPATCH|^FINDER-' "$(BUILD)/$(FLAIR_DW_ROOT_NAME).serial" || true; exit 1; }
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) finderbar "$(BUILD)/$(FLAIR_DW_FBAR_NAME).ppm" \
	|| { printf '!!! test-flair-disk-windows FAIL: band 2 is not the FINDER menu bar (F4.2 File/Edit/View/Special/Help at the hand-derived columns)\n'; exit 1; }
@printf '>>> test-flair-disk-windows [2/8]: FLAIR-DISPATCH app=FINDER + grader leg FINDERBAR (band 2 IS the Finder bar)\n'
@# ---- leg 2: navigate into APPS, then prove the SPATIAL SINGLETON. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_DW_DATA_A)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_DW_NAV_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_FOLDER_NAV_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--quit-after "FINDER-OPEN-FOLDER name=APPS win=1 singleton=1" \
	--timeout-ms 30000 2> "$(BUILD)/$(FLAIR_DW_NAV_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_DW_NAV_NAME).report"; then printf '!!! test-flair-disk-windows FAIL: TRIPLE FAULT in the NAV boot\n'; exit 1; fi
@grep -qxF 'FINDER-OPEN-VOLUME win=0 n=4' "$(BUILD)/$(FLAIR_DW_NAV_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: NAV leg never opened the root window\n'; grep '^FINDER-' "$(BUILD)/$(FLAIR_DW_NAV_NAME).serial" || true; exit 1; }
@grep -qxF 'FINDER-OPEN-FOLDER name=APPS win=1 singleton=0' "$(BUILD)/$(FLAIR_DW_NAV_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: the double-click on the APPS folder icon at the derived cell (107,86)+(16,16) did not open a NEW folder window in slot 1\n'; grep -E '^FINDER-|^FLAIR-DRAG' "$(BUILD)/$(FLAIR_DW_NAV_NAME).serial" || true; exit 1; }
@grep -qxF 'FLAIR-DRAG win 0 (40,80)->(250,250)' "$(BUILD)/$(FLAIR_DW_NAV_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: the folder window did not move by the locked delta (+210,+170) from its cascaded frame (40,80)\n'; grep -E '^FINDER-|^FLAIR-DRAG' "$(BUILD)/$(FLAIR_DW_NAV_NAME).serial" || true; exit 1; }
@grep -qxF 'FINDER-OPEN-FOLDER name=APPS win=1 singleton=1' "$(BUILD)/$(FLAIR_DW_NAV_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: re-opening APPS did NOT raise the existing window -- the spatial singleton is broken\n'; grep -E '^FINDER-|^FLAIR-DRAG' "$(BUILD)/$(FLAIR_DW_NAV_NAME).serial" || true; exit 1; }
@if [ "$$(grep -c '^FINDER-OPEN-FOLDER ' "$(BUILD)/$(FLAIR_DW_NAV_NAME).serial")" != "2" ]; then \
	printf '!!! test-flair-disk-windows FAIL: expected exactly 2 FINDER-OPEN-FOLDER lines, got %s\n' "$$(grep -c '^FINDER-OPEN-FOLDER ' "$(BUILD)/$(FLAIR_DW_NAV_NAME).serial")"; \
	grep '^FINDER-OPEN-FOLDER ' "$(BUILD)/$(FLAIR_DW_NAV_NAME).serial" || true; exit 1; fi
@! grep -q '^FINDER-WIN-FULL' "$(BUILD)/$(FLAIR_DW_NAV_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: the window table filled -- windows are being duplicated\n'; exit 1; }
@printf '>>> test-flair-disk-windows [3/8]: OPEN-FOLDER singleton=0 -> drag -> OPEN-FOLDER singleton=1 (the SAME slot raised)\n'
@# ---- leg 3: drag the root window, close it, and save the view record. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_DW_DATA_A)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_DW_PERSW_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_WINDOW_DRAG_PERSIST_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--quit-after "DESKTOP-DB-SAVE n=3" \
	--timeout-ms 30000 2> "$(BUILD)/$(FLAIR_DW_PERSW_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_DW_PERSW_NAME).report"; then printf '!!! test-flair-disk-windows FAIL: TRIPLE FAULT in the PERSIST-WRITE boot\n'; exit 1; fi
@grep -qxF 'FLAIR-DRAG win 0 (20,60)->(120,180)' "$(BUILD)/$(FLAIR_DW_PERSW_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: the root window did not move by the locked delta (+100,+120) from its default frame (20,60)\n'; grep -E '^FINDER-|^FLAIR-DRAG|^DESKTOP-DB' "$(BUILD)/$(FLAIR_DW_PERSW_NAME).serial" || true; exit 1; }
@grep -qxF 'FINDER-CLOSE-WINDOW win=0' "$(BUILD)/$(FLAIR_DW_PERSW_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: the click on the moved go-away box (130,190) did not close the Finder-owned window\n'; grep -E '^FINDER-|^FLAIR-' "$(BUILD)/$(FLAIR_DW_PERSW_NAME).serial" || true; exit 1; }
@! grep -q '^FLAIR-TENANT-EXIT name=FINDER' "$(BUILD)/$(FLAIR_DW_PERSW_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: closing a disk window TERMINATED the always-resident Finder (design F2-3, the anti-8fhu rule)\n'; exit 1; }
@grep -qxF 'DESKTOP-DB-SAVE n=3' "$(BUILD)/$(FLAIR_DW_PERSW_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: the close did not commit 3 records (2 desktop icons + 1 kind=4 view) to DESKTOP.DB\n'; grep '^DESKTOP-DB' "$(BUILD)/$(FLAIR_DW_PERSW_NAME).serial" || true; exit 1; }
@! grep -q '^DESKTOP-DB-WRITE-FAIL' "$(BUILD)/$(FLAIR_DW_PERSW_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: DESKTOP-DB-WRITE-FAIL during the view save\n'; exit 1; }
@printf '>>> test-flair-disk-windows [4/8]: FLAIR-DRAG (20,60)->(120,180) + FINDER-CLOSE-WINDOW win=0 + DESKTOP-DB-SAVE n=3\n'
@# ---- leg 4: REBOOT the same, now-written volume; the window comes back. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_DW_DATA_A)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_DW_PERSR_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_ICON_OPEN_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FINDER-OPEN-VOLUME win=0 n=4" \
	--timeout-ms 20000 2> "$(BUILD)/$(FLAIR_DW_PERSR_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_DW_PERSR_NAME).report"; then printf '!!! test-flair-disk-windows FAIL: TRIPLE FAULT in the PERSIST-REBOOT boot\n'; exit 1; fi
@grep -qxF 'DESKTOP-DB-OK' "$(BUILD)/$(FLAIR_DW_PERSR_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: the reboot did not read back a valid DESKTOP.DB\n'; grep '^DESKTOP-DB' "$(BUILD)/$(FLAIR_DW_PERSR_NAME).serial" || true; exit 1; }
@if grep -q '^DESKTOP-DB-CREATE' "$(BUILD)/$(FLAIR_DW_PERSR_NAME).serial" || \
    grep -q '^DESKTOP-DB-REGEN' "$(BUILD)/$(FLAIR_DW_PERSR_NAME).serial" || \
    grep -q '^DESKTOP-DB-POS-REGEN' "$(BUILD)/$(FLAIR_DW_PERSR_NAME).serial" || \
    grep -q '^DESKTOP-DB-POS-SKIP' "$(BUILD)/$(FLAIR_DW_PERSR_NAME).serial"; then \
	printf '!!! test-flair-disk-windows FAIL: the reboot re-created/regenerated DESKTOP.DB instead of reading the saved view\n'; \
	grep '^DESKTOP-DB' "$(BUILD)/$(FLAIR_DW_PERSR_NAME).serial" || true; exit 1; \
fi
@grep -qxF 'DESKTOP-DB-VIEWS n=1' "$(BUILD)/$(FLAIR_DW_PERSR_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: the reboot did not take back exactly ONE kind=4 view record\n'; grep '^DESKTOP-DB' "$(BUILD)/$(FLAIR_DW_PERSR_NAME).serial" || true; exit 1; }
@grep -qxF 'FINDER-OPEN-VOLUME win=0 n=4' "$(BUILD)/$(FLAIR_DW_PERSR_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: the reboot could not re-open the root window\n'; grep '^FINDER-' "$(BUILD)/$(FLAIR_DW_PERSR_NAME).serial" || true; exit 1; }
@if [ ! -s "$(BUILD)/$(FLAIR_DW_PERSR_NAME).ppm" ]; then printf '!!! test-flair-disk-windows FAIL: PERSIST-REBOOT screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) movedwin "$(BUILD)/$(FLAIR_DW_PERSR_NAME).ppm" \
	|| { printf '!!! test-flair-disk-windows FAIL: the window did NOT reopen at the saved origin (120,180) -- or the cascaded default rect is still covered\n'; exit 1; }
@printf '>>> test-flair-disk-windows [5/8]: DESKTOP-DB-VIEWS n=1 + grader leg MOVEDWIN (the window remembered where you left it)\n'
@# ---- leg 5: Ctrl-N makes a REAL directory; mtools is the judge. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_DW_DATA_B)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_DW_NEWF_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_NEW_FOLDER_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FINDER-NEW-FOLDER name=NEWFOLD parent=0" \
	--timeout-ms 20000 2> "$(BUILD)/$(FLAIR_DW_NEWF_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_DW_NEWF_NAME).report"; then printf '!!! test-flair-disk-windows FAIL: TRIPLE FAULT in the NEWFOLDER boot\n'; exit 1; fi
@grep -qxF 'FINDER-CMD id=2 name=NEW_FOLDER src=key sel=0' "$(BUILD)/$(FLAIR_DW_NEWF_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: the Ctrl-N chord never reached the ONE command spine. Two axes: (a) no FINDER-CMD line at all = the chord was not decoded (check that harness qmp_send_key_chord emitted [ctrl,n] and that os/flair/event.c still cooks FLAIR_EVT_MOD_CONTROL_KEY off PS/2 make 0x1D); (b) a FINDER-CMD-DISABLED line = the predicate refused\n'; grep -E '^FINDER-' "$(BUILD)/$(FLAIR_DW_NEWF_NAME).serial" || true; exit 1; }
@grep -qxF 'FINDER-NEW-FOLDER name=NEWFOLD parent=0' "$(BUILD)/$(FLAIR_DW_NEWF_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: New Folder did not create the locked first ladder name NEWFOLD in the root\n'; grep -E '^FINDER-' "$(BUILD)/$(FLAIR_DW_NEWF_NAME).serial" || true; exit 1; }
@if [ ! -s "$(BUILD)/$(FLAIR_DW_NEWF_NAME).ppm" ]; then printf '!!! test-flair-disk-windows FAIL: NEWFOLDER screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) newfolder "$(BUILD)/$(FLAIR_DW_NEWF_NAME).ppm" \
	|| { printf '!!! test-flair-disk-windows FAIL: the new directory did not appear as a FOLDER icon at the derived cell 2 (175,86)\n'; exit 1; }
@# The mtools DIFFERENTIAL: an INDEPENDENT reader, never our own FAT code.
@mdir -a -i $(FLAIR_DW_DATA_B) :: > "$(BUILD)/$(FLAIR_DW_NEWF_NAME).mdir"
@grep -Eq '^NEWFOLD +<DIR>' "$(BUILD)/$(FLAIR_DW_NEWF_NAME).mdir" \
	|| { printf '!!! test-flair-disk-windows FAIL: mdir does not see a NEWFOLD directory in the root -- the serial line claimed a directory that is not on the disk\n'; cat "$(BUILD)/$(FLAIR_DW_NEWF_NAME).mdir"; exit 1; }
@mdir -a -i $(FLAIR_DATA_IMG) :: > "$(BUILD)/$(FLAIR_DW_NEWF_NAME).pristine.mdir"
@! grep -Eq '^NEWFOLD ' "$(BUILD)/$(FLAIR_DW_NEWF_NAME).pristine.mdir" \
	|| { printf '!!! test-flair-disk-windows FAIL: the PRISTINE $(FLAIR_DATA_IMG) was dirtied -- the gate must only ever write its own copy\n'; exit 1; }
@# The NEGATIVE MenuKey leg (bead initech-tdnl.12): the Cmd-I that PRECEDES
@# the Cmd-N fires with an EMPTY selection, so File > Get Info is grayed and
@# menu.h's MenuKey hands out nothing at all. The tdnl.10 table scan would
@# have dispatched it and printed FINDER-CMD-DISABLED -- so the ABSENCE of
@# any GET_INFO line is what proves the keyboard path really moved to the
@# live menu resource. See spec/flair_disk_windows_traces.mk step B1.
@! grep -q 'GET_INFO' "$(BUILD)/$(FLAIR_DW_NEWF_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: Cmd-I with NOTHING selected reached the command spine. MenuKey must refuse a GRAYED item outright (os/flair/menu.c: the FIRST ENABLED non-divider match) -- a FINDER-CMD-DISABLED line here means the chord is still being resolved off the command table, not off the live bar\n'; grep -E '^FINDER-' "$(BUILD)/$(FLAIR_DW_NEWF_NAME).serial" || true; exit 1; }
@printf '>>> test-flair-disk-windows [6/8]: Ctrl-I (grayed, refused by MenuKey) then Ctrl-N -> FINDER-NEW-FOLDER NEWFOLD + grader leg NEWFOLDER + mdir sees a REAL <DIR>\n'
@# ---- leg 7 (bead initech-tdnl.12): the FINDER MENU BAR, BY MOUSE. ----
@# Drag an icon off its cell, then Special > Clean Up from the real band-2
@# bar. moved=1 is the whole point: on a freshly populated window Clean Up
@# would report moved=0, which cannot be told from a command that never ran.
@# Then select the snapped-back icon and Cmd-I -- the POSITIVE half of the
@# MenuKey enable-byte proof whose negative half is leg 6.
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_DW_DATA_C)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_DW_MENU_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_FINDER_MENU_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--quit-after "FINDER-CLOSE-WINDOW win=0" \
	--timeout-ms 30000 2> "$(BUILD)/$(FLAIR_DW_MENU_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_DW_MENU_NAME).report"; then printf '!!! test-flair-disk-windows FAIL: TRIPLE FAULT in the FINDER-MENU boot\n'; exit 1; fi
@# RE-KEYED 2026-10-16 (bead initech-tdnl.51, stated): README.TXT's cell is
@# (44,86) since the content starts at L+6, so the (+150,+80) drag lands at
@# (194,166) (was 189); the trace's press point (55,102) is still on the sprite.
@grep -qxF 'FINDER-WIN-DRAG win=0 name=README.TXT x=194 y=166' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: the setup drag did not move README.TXT off its grid cell to the derived (194,166)\n'; grep -E '^FINDER-|^FLAIR-MENU' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" || true; exit 1; }
@grep -qxF 'FLAIR-MENU-DROP menu=515' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: clicking the Special title at the hand-derived band-2 column (190,30) dropped no menu -- menu id 515 expected (F4-1)\n'; grep -E '^FLAIR-MENU' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" || true; exit 1; }
@grep -qxF 'FLAIR-MENU menu=515 item=1 (sel=0x02030001)' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: the release at (190,48) did not select Special item 1 (Clean Up)\n'; grep -E '^FLAIR-MENU' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" || true; exit 1; }
@grep -qxF 'FINDER-CMD id=9 name=CLEANUP src=mouse sel=0' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: the MOUSE selection never reached the ONE command spine with src=mouse (F4-4: both input paths converge on finder_dispatch)\n'; grep -E '^FINDER-CMD|^FLAIR-MENU' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" || true; exit 1; }
@grep -qxF 'FINDER-CLEANUP win=0 moved=1' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: Clean Up dispatched but did not SNAP the dragged icon back (moved=1 expected -- exactly one icon was off-grid)\n'; grep -E '^FINDER-' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" || true; exit 1; }
@grep -qxF 'FINDER-WIN-SELECT win=0 name=README.TXT count=1' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: README.TXT was not at its restored cell (44,86) after Clean Up -- the click at (55,102), on its sprite, selected nothing\n'; grep -E '^FINDER-' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" || true; exit 1; }
@# RE-KEY (bead initech-tdnl.36; spec/flair_disk_windows_traces.mk trace 4):
@# Get Info is unimplemented, so Cmd-I WITH a selection must route nothing;
@# the positive MenuKey half is Cmd-W (Close Window, lit by the front disk
@# window), which must dispatch and really close it.
@! grep -q 'GET_INFO' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: Cmd-I reached the command spine although File > Get Info has no implementation and must be grayed (tdnl.36)\n'; grep -E '^FINDER-' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" || true; exit 1; }
@grep -qxF 'FINDER-CMD id=4 name=CLOSE_WINDOW src=key sel=1' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: Cmd-W with a disk window front did not dispatch. The enable byte for File > Close Window must be recomputed to 1 before MenuKey scans (design F4.4)\n'; grep -E '^FINDER-' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" || true; exit 1; }
@grep -qxF 'FINDER-CLOSE-WINDOW win=0' "$(BUILD)/$(FLAIR_DW_MENU_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: Cmd-W dispatched but the window did not close\n'; exit 1; }
@printf '>>> test-flair-disk-windows [7/8]: Special > Clean Up BY MOUSE (FINDER-CMD src=mouse + moved=1) + Cmd-I refused (unimplemented) + Cmd-W dispatched (src=key)\n'
@# ---- leg 8 (bead initech-tdnl.12): the band-2 CANCEL restores the frame. ----
@# Pull the FILE menu down over the disk window and the desktop, drag off the
@# panel and release on bare desktop. Nothing dispatches, and the WHOLE frame
@# must come back byte-for-byte identical to the boot that only opened the
@# volume (leg 1's dump) -- the solid-leg-D property applied to band 2.
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_DW_DATA_D)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_DW_CANCEL_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_FINDER_MENU_CANCEL_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-MENU menu=512 item=0 (sel=0x00000000)" \
	--timeout-ms 30000 2> "$(BUILD)/$(FLAIR_DW_CANCEL_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_DW_CANCEL_NAME).report"; then printf '!!! test-flair-disk-windows FAIL: TRIPLE FAULT in the MENU-CANCEL boot\n'; exit 1; fi
@grep -qxF 'FLAIR-MENU-DROP menu=512' "$(BUILD)/$(FLAIR_DW_CANCEL_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: clicking the File title at (40,30) dropped no menu -- menu id 512 expected\n'; grep -E '^FLAIR-MENU' "$(BUILD)/$(FLAIR_DW_CANCEL_NAME).serial" || true; exit 1; }
@grep -qxF 'FLAIR-MENU menu=512 item=0 (sel=0x00000000)' "$(BUILD)/$(FLAIR_DW_CANCEL_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: releasing on bare desktop at (400,400) did not CANCEL the pull-down\n'; grep -E '^FLAIR-MENU' "$(BUILD)/$(FLAIR_DW_CANCEL_NAME).serial" || true; exit 1; }
@! grep -q '^FINDER-CMD' "$(BUILD)/$(FLAIR_DW_CANCEL_NAME).serial" \
	|| { printf '!!! test-flair-disk-windows FAIL: a CANCELLED pull-down dispatched a command. IM result word 0 is "nothing chosen" and finder_dispatch must return before tracing\n'; grep -E '^FINDER-CMD' "$(BUILD)/$(FLAIR_DW_CANCEL_NAME).serial" || true; exit 1; }
@if [ ! -s "$(BUILD)/$(FLAIR_DW_CANCEL_NAME).ppm" ]; then printf '!!! test-flair-disk-windows FAIL: MENU-CANCEL screendump missing\n'; exit 1; fi
@cmp -s "$(BUILD)/$(FLAIR_DW_ROOT_NAME).ppm" "$(BUILD)/$(FLAIR_DW_CANCEL_NAME).ppm" \
	|| { printf '!!! test-flair-disk-windows FAIL: PRE != POST. The File pull-down left ink behind: every pixel its panel x[20,176) y[39,245) covered -- disk-window title bar, go-away box, icon row AND bare desktop -- must be restored byte-for-byte by the track-end erase (see os/milton/kmain.c flair_live_erase_menu_panel; mutant KMAIN_MUT_MENU_NO_RESTORE)\n'; exit 1; }
@printf '>>> test-flair-disk-windows [8/8]: band-2 CANCEL -- no dispatch, and the whole frame is BYTE-IDENTICAL to the pre-menu frame\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- the booted Finder opens the REAL volume in a REAL window,\n'
@printf '            navigates folders as spatial singletons, remembers a window origin\n'
@printf '            across a power cycle, and creates a REAL FAT12 directory from a\n'
@printf '            Ctrl-N chord (QEMU; Bochs boot leg = make test-flair-disk-windows-bochs)\n'
@printf '======================================================================\n'
```

## test-flair-disk-windows-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 228):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-disk-windows-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-disk-windows-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-disk-windows_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-disk-windows_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-disk-windows_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-disk-windows_bochs.report.txt" || true
@cat "$(BUILD)/flair-disk-windows_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-disk-windows-bochs "$(BUILD)/flair-disk-windows_bochs.report.txt" "$(BUILD)/flair-disk-windows_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK DESKTOP-DB-CREATE TRASH-CREATE FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-disk-windows-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 21164):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/flair_tenants_mut_win_vollabel_shown.img build/flair_tenants_mut_win_singleton_dup.img build/flair_tenants_mut_menu_dead_item.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-disk-windows-mutant : Rule 6 (the gate BITES)\n'
@printf '  2 FLAIRTENANTS finder_windows.c mutants + 1 finder_menu.c mutant;\n'
@printf '  each MUST drive its leg RED.\n'
@printf '======================================================================\n'
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_DW_DATA_A)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_DW_DATA_A),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@# ---- baseline: the CLEAN image must be GREEN on BOTH mutated legs. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_DW_DATA_A)" \
	--expect FLAIR-FAT-MOUNT-OK --name flair_diskwin_mut_base_open --out "$(BUILD)" \
	--mouse "$(FLAIR_ICON_OPEN_SPEC)" --keys-after "FLAIR-LIVE-READY" --timeout-ms 20000 \
	--quit-after "FINDER-OPEN-VOLUME win=0 n=4" >/dev/null 2>&1 || true
@grep -qxF 'FINDER-OPEN-VOLUME win=0 n=4' "$(BUILD)/flair_diskwin_mut_base_open.serial" \
	|| { printf '!!! test-flair-disk-windows-mutant FAIL: the CLEAN image did not emit FINDER-OPEN-VOLUME win=0 n=4 -- the baseline is broken (not a mutant)\n'; exit 1; }
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_DW_DATA_A)" \
	--expect FLAIR-FAT-MOUNT-OK --name flair_diskwin_mut_base_nav --out "$(BUILD)" \
	--mouse "$(FLAIR_FOLDER_NAV_SPEC)" --keys-after "FLAIR-LIVE-READY" --timeout-ms 30000 \
	--quit-after "FINDER-OPEN-FOLDER name=APPS win=1 singleton=1" >/dev/null 2>&1 || true
@grep -qxF 'FINDER-OPEN-FOLDER name=APPS win=1 singleton=1' "$(BUILD)/flair_diskwin_mut_base_nav.serial" \
	|| { printf '!!! test-flair-disk-windows-mutant FAIL: the CLEAN image did not raise the existing APPS window -- the baseline is broken\n'; grep '^FINDER-OPEN-FOLDER' "$(BUILD)/flair_diskwin_mut_base_nav.serial" || true; exit 1; }
@printf '>>> baseline: the clean FLAIRTENANTS image enumerates 4 icons and raises the singleton\n'
@printf '%s\n' '----------------------------------------------------------------------'
@# ---- mutant 1: VOLLABEL_SHOWN -- the enumerated count MUST become 5. ----
@$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_win_vollabel_shown.img" --disk2 "$(FLAIR_DW_DATA_A)" \
	--expect FLAIR-FAT-MOUNT-OK --name flair_diskwin_mut_vollabel --out "$(BUILD)" \
	--mouse "$(FLAIR_ICON_OPEN_SPEC)" --keys-after "FLAIR-LIVE-READY" --timeout-ms 20000 \
	--quit-after "FINDER-OPEN-VOLUME win=0 n=5" \
	2> "$(BUILD)/flair_diskwin_mut_vollabel.report" || true
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_diskwin_mut_vollabel.serial" \
	|| { printf '!!! test-flair-disk-windows-mutant FAIL: vollabel mutant missing mount marker (not comparable)\n'; exit 1; }
@if grep -q 'triple_fault=1' "$(BUILD)/flair_diskwin_mut_vollabel.report"; then printf '!!! test-flair-disk-windows-mutant FAIL: vollabel TRIPLE-FAULTED (cannot judge the oracle)\n'; exit 1; fi
@if grep -qxF 'FINDER-OPEN-VOLUME win=0 n=4' "$(BUILD)/flair_diskwin_mut_vollabel.serial"; then \
	printf '!!! test-flair-disk-windows-mutant FAIL: the leg-1 count tooth is DECORATION -- VOLLABEL_SHOWN still reported n=2\n'; exit 1; \
fi
@grep -qxF 'FINDER-OPEN-VOLUME win=0 n=5' "$(BUILD)/flair_diskwin_mut_vollabel.serial" \
	|| { printf '!!! test-flair-disk-windows-mutant FAIL: vollabel mutant did not report the EXPECTED n=3 -- wrong failure axis, the mutant is not doing what it claims\n'; grep '^FINDER-' "$(BUILD)/flair_diskwin_mut_vollabel.serial" || true; exit 1; }
@printf '>>> mutant win_vollabel_shown correctly RED: the volume LABEL leaked into the listing -- FINDER-OPEN-VOLUME win=0 n=5 (expected n=4)\n'
@# ---- mutant 2: SINGLETON_DUP -- the second open MUST build a 2nd window. ----
@$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_win_singleton_dup.img" --disk2 "$(FLAIR_DW_DATA_A)" \
	--expect FLAIR-FAT-MOUNT-OK --name flair_diskwin_mut_singleton --out "$(BUILD)" \
	--mouse "$(FLAIR_FOLDER_NAV_SPEC)" --keys-after "FLAIR-LIVE-READY" --timeout-ms 30000 \
	--quit-after "FINDER-OPEN-FOLDER name=APPS win=2 singleton=0" \
	2> "$(BUILD)/flair_diskwin_mut_singleton.report" || true
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_diskwin_mut_singleton.serial" \
	|| { printf '!!! test-flair-disk-windows-mutant FAIL: singleton mutant missing mount marker (not comparable)\n'; exit 1; }
@if grep -q 'triple_fault=1' "$(BUILD)/flair_diskwin_mut_singleton.report"; then printf '!!! test-flair-disk-windows-mutant FAIL: singleton TRIPLE-FAULTED (cannot judge the oracle)\n'; exit 1; fi
@grep -qxF 'FINDER-OPEN-FOLDER name=APPS win=1 singleton=0' "$(BUILD)/flair_diskwin_mut_singleton.serial" \
	|| { printf '!!! test-flair-disk-windows-mutant FAIL: singleton mutant did not even open APPS the FIRST time -- wrong failure axis\n'; grep '^FINDER-' "$(BUILD)/flair_diskwin_mut_singleton.serial" || true; exit 1; }
@if grep -qxF 'FINDER-OPEN-FOLDER name=APPS win=1 singleton=1' "$(BUILD)/flair_diskwin_mut_singleton.serial"; then \
	printf '!!! test-flair-disk-windows-mutant FAIL: the leg-2 singleton tooth is DECORATION -- SINGLETON_DUP still raised the existing window\n'; exit 1; \
fi
@grep -qxF 'FINDER-OPEN-FOLDER name=APPS win=2 singleton=0' "$(BUILD)/flair_diskwin_mut_singleton.serial" \
	|| { printf '!!! test-flair-disk-windows-mutant FAIL: singleton mutant did not build the EXPECTED second window in slot 2 -- wrong failure axis\n'; grep '^FINDER-OPEN-FOLDER' "$(BUILD)/flair_diskwin_mut_singleton.serial" || true; exit 1; }
@printf '>>> mutant win_singleton_dup correctly RED: re-opening APPS built a SECOND window -- FINDER-OPEN-FOLDER name=APPS win=2 singleton=0 (expected win=1 singleton=1)\n'
@# ---- mutant 3 (bead initech-tdnl.12): DEAD_ITEM -- Special > Clean Up ----
@# ships with its enable byte stuck at 0. The bar still draws, the pull-down
@# still DROPS, and the item is still there -- but MenuSelect cannot return a
@# disabled row, so the whole command evaporates. Baseline first: the CLEAN
@# image dispatches it.
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
@cp -f $(FLAIR_DATA_IMG) $(BUILD)/flair_diskwin_mut_menu_base_data.img
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(BUILD)/flair_diskwin_mut_menu_base_data.img,$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(BUILD)/flair_diskwin_mut_menu_base_data.img" \
	--expect FLAIR-FAT-MOUNT-OK --name flair_diskwin_mut_base_menu --out "$(BUILD)" \
	--mouse "$(FLAIR_FINDER_MENU_SPEC)" --keys-after "FLAIR-LIVE-READY" --timeout-ms 30000 \
	--quit-after "FINDER-CMD id=9 name=CLEANUP src=mouse sel=0" >/dev/null 2>&1 || true
@grep -qxF 'FINDER-CMD id=9 name=CLEANUP src=mouse sel=0' "$(BUILD)/flair_diskwin_mut_base_menu.serial" \
	|| { printf '!!! test-flair-disk-windows-mutant FAIL: the CLEAN image did not dispatch Special > Clean Up by mouse -- the baseline is broken (not a mutant)\n'; grep -E '^FINDER-|^FLAIR-MENU' "$(BUILD)/flair_diskwin_mut_base_menu.serial" || true; exit 1; }
@printf '>>> baseline: the clean FLAIRTENANTS image dispatches Special > Clean Up from the real band-2 bar\n'
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
@cp -f $(FLAIR_DATA_IMG) $(BUILD)/flair_diskwin_mut_menu_dead_data.img
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(BUILD)/flair_diskwin_mut_menu_dead_data.img,$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@# initech-qed1: --legacy-quit -- the dispatch-or-not comes from the FINAL
@# click of this same already-injected burst, downstream of the earlier
@# guaranteed FLAIR-MENU-DROP marker; no honest quit-after marker exists
@# for an absence-final leg like this one (see the dbltick_off leg above).
@$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_menu_dead_item.img" --disk2 "$(BUILD)/flair_diskwin_mut_menu_dead_data.img" \
	--expect FLAIR-FAT-MOUNT-OK --name flair_diskwin_mut_dead_item --out "$(BUILD)" \
	--mouse "$(FLAIR_FINDER_MENU_SPEC)" --keys-after "FLAIR-LIVE-READY" --timeout-ms 30000 --legacy-quit \
	2> "$(BUILD)/flair_diskwin_mut_dead_item.report" || true
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_diskwin_mut_dead_item.serial" \
	|| { printf '!!! test-flair-disk-windows-mutant FAIL: dead-item mutant missing mount marker (not comparable)\n'; exit 1; }
@if grep -q 'triple_fault=1' "$(BUILD)/flair_diskwin_mut_dead_item.report"; then printf '!!! test-flair-disk-windows-mutant FAIL: dead-item TRIPLE-FAULTED (cannot judge the oracle)\n'; exit 1; fi
@grep -qxF 'FLAIR-MENU-DROP menu=515' "$(BUILD)/flair_diskwin_mut_dead_item.serial" \
	|| { printf '!!! test-flair-disk-windows-mutant FAIL: the dead-item mutant did not even DROP the Special menu -- wrong failure axis (the bar itself is broken, not the item)\n'; grep -E '^FLAIR-MENU' "$(BUILD)/flair_diskwin_mut_dead_item.serial" || true; exit 1; }
@if grep -q '^FINDER-CMD id=9 ' "$(BUILD)/flair_diskwin_mut_dead_item.serial"; then \
	printf '!!! test-flair-disk-windows-mutant FAIL: the leg-7 dispatch tooth is DECORATION -- DEAD_ITEM still dispatched Clean Up\n'; exit 1; \
fi
@if grep -q '^FINDER-CLEANUP ' "$(BUILD)/flair_diskwin_mut_dead_item.serial"; then \
	printf '!!! test-flair-disk-windows-mutant FAIL: DEAD_ITEM still ran Clean Up -- the enable byte is not gating selection\n'; exit 1; \
fi
@grep -qxF 'FLAIR-MENU menu=515 item=0 (sel=0x00000000)' "$(BUILD)/flair_diskwin_mut_dead_item.serial" \
	|| { printf '!!! test-flair-disk-windows-mutant FAIL: dead-item mutant did not produce the EXPECTED sel=0 (a disabled row is never selectable, menu.c MenuInfo_item_selectable) -- wrong failure axis\n'; grep -E '^FLAIR-MENU' "$(BUILD)/flair_diskwin_mut_dead_item.serial" || true; exit 1; }
@printf '>>> mutant menu_dead_item correctly RED: the Special menu still DROPPED but Clean Up was unselectable -- FLAIR-MENU menu=515 item=0 and no FINDER-CMD id=9 (a menu item that draws and does nothing)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- all three mutants drive their legs RED (the gate bites; Rule 6)\n'
@printf '======================================================================\n'
```

## test-flair-drag

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 19864):
Prerequisites: `build/qemu_harness build/flair_live.img build/ppm_flair_drag_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-drag : THE live draggable desktop (FO-9)\n'
@printf '  Inject the locked trace -> true-modal dispatch -> movableDBoxProc inDrag\n'
@printf '  -> MoveDialog + old-footprint damage restore + present. Modal anatomy moves\n'
@printf '  to the new rect and the windows below reappear. initech-zn61/zvo6/a90f.\n'
@printf '  ADR-0006 E-D5 (Tier-A damage law + Tier-B shifted geometry). Law 2/4.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (the WaitNextEvent pump)\n' "$(FLAIRLIVE_IMG)"
@printf 'Expecting : FLAIR-MODAL-DRAG (140,200)->(180,230) + modal@new + windows@old\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_IMG)" --name "$(FLAIR_DRAG_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_DRAG_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-MODAL-DRAG" --timeout-ms 15000 \
	2> "$(FLAIR_DRAG_REPORT)" || true
@cat "$(FLAIR_DRAG_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(FLAIR_DRAG_REPORT)"; then \
	printf '!!! test-flair-drag FAIL: TRIPLE FAULT in the pump/drag boot\n'; exit 1; \
fi
@printf '>>> test-flair-drag [1/4]: no triple-fault\n'
@grep -q '^FLAIR-LIVE-READY$$' "$(FLAIR_DRAG_SERIAL)" \
	|| { printf '!!! test-flair-drag FAIL: FLAIR-LIVE-READY missing -- the pump never armed\n'; exit 1; }
@printf '>>> test-flair-drag [2/4]: FLAIR-LIVE-READY (the WaitNextEvent pump is armed)\n'
@grep -q '^FLAIR-MODAL-DRAG (140,200)->(180,230)$$' "$(FLAIR_DRAG_SERIAL)" \
	|| { printf '!!! test-flair-drag FAIL: exact FLAIR-MODAL-DRAG marker missing -- modal inDrag did not run / wrong delta\n'; grep '^FLAIR-' "$(FLAIR_DRAG_SERIAL)" || true; exit 1; }
@printf '>>> test-flair-drag [3/4]: FLAIR-MODAL-DRAG (140,200)->(180,230)\n'
@if [ ! -s "$(FLAIR_DRAG_PPM)" ]; then printf '!!! test-flair-drag FAIL: no screendump captured at %s\n' "$(FLAIR_DRAG_PPM)"; exit 1; fi
@$(PPM_FLAIR_DRAG_CHECK_BIN) modal "$(FLAIR_DRAG_PPM)" \
	|| { printf '!!! test-flair-drag FAIL: moved-modal anatomy/vacated-window restoration oracle RED\n'; exit 1; }
@printf '>>> test-flair-drag [4/4]: modal at shifted rect + old frame gone + windows restored\n'
@printf 'VERDICT   : PASS -- the booted FLAIR desktop is LIVE and DRAGGABLE (Law 4)\n'
@printf '======================================================================\n'
```

## test-flair-drag-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 19903):
Prerequisites: `build/qemu_harness build/flair_live_mut_drag.img build/ppm_flair_drag_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-drag-mutant : Rule 6 (the gate BITES)\n'
@printf '  Mutant: -DFLAIR_LIVE_MUTATE_DRAG_NOOP (document + modal commits no-op).\n'
@printf '  Expect: exact modal marker, but pixels unmoved -> modal grader RED.\n'
@printf '======================================================================\n'
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_MUT_DRAG_IMG)" --name flair_drag_mut --out "$(BUILD)" \
	--mouse "$(FLAIR_DRAG_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-MODAL-DRAG" --timeout-ms 15000 >/dev/null 2>&1 || true
@grep -q '^FLAIR-LIVE-READY$$' "$(BUILD)/flair_drag_mut.serial" \
	|| { printf '!!! test-flair-drag-mutant: mutant did not reach FLAIR-LIVE-READY (not comparable)\n'; exit 1; }
@grep -q '^FLAIR-MODAL-DRAG (140,200)->(180,230)$$' "$(BUILD)/flair_drag_mut.serial" \
	|| { printf '!!! test-flair-drag-mutant: exact proposed modal marker missing (not comparable)\n'; exit 1; }
@if [ ! -s "$(BUILD)/flair_drag_mut.ppm" ]; then printf '!!! test-flair-drag-mutant: no screendump captured (cannot judge the mutant)\n'; exit 1; fi
@if $(PPM_FLAIR_DRAG_CHECK_BIN) modal "$(BUILD)/flair_drag_mut.ppm" >/dev/null 2>&1; then \
	printf '!!! test-flair-drag-mutant FAIL: the drag-noop screendump PASSED ppm_flair_drag_check -- the gate is decoration (a static frame passed as interactive)\n'; exit 1; \
fi
@printf '>>> test-flair-drag-mutant: RED as required -- exact marker fired but modal pixels stayed at old rect\n'
@printf '======================================================================\n'
```

## test-flair-egg-jtc

Declared tier: outside active vector; #  recipe to execute (from 'harness/proptest/egg_jtc.mk', line 83):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_jtc_gate.img build/flair_data.img build/ppm_flair_egg_jtc_check build/.makefile.stamp`
```make
$(call jtc-boot,egg_jtc_mat,$(JTC_IMAGE),FLAIR_EGG_JTC_OPEN_SPEC,EGG-JTC-OPEN click=3)
@grep -qx 'EGG-JTC-OPEN click=3' $(BUILD)/egg_jtc_mat.serial || { printf 'FAIL JTC trigger: expected third click\n'; exit 1; }
@$(JTC_CHECK) $(BUILD)/egg_jtc_mat.ppm fixtures/easter/jtc_mat.png
$(call jtc-boot,egg_jtc_alert,$(JTC_IMAGE),FLAIR_EGG_JTC_ALERT_SPEC,EGG-JTC-ALERT up)
@grep -qx 'EGG-JTC-JUMP pad=8 x=218 y=372' $(BUILD)/egg_jtc_alert.serial
@grep -qx 'FLAIR-CURSOR x=218 y=372' $(BUILD)/egg_jtc_alert.serial
@grep -qx 'EGG-JTC-ALERT closed' $(BUILD)/egg_jtc_alert.serial
@$(JTC_CHECK) $(BUILD)/egg_jtc_alert.ppm fixtures/easter/jtc_alert.png
$(call jtc-boot,egg_jtc_return,$(JTC_IMAGE),FLAIR_EGG_JTC_SPEC,EGG-JTC-ALERT up)
@grep -qx 'EGG-JTC-ALERT closed' $(BUILD)/egg_jtc_return.serial && ! grep -q 'FLAIR-TRACK-EXPIRED' $(BUILD)/egg_jtc_return.serial
$(call jtc-boot,egg_jtc_short,$(JTC_IMAGE),FLAIR_EGG_JTC_SHORT_SPEC,FLAIR-LIVE-OK)
@$(call jtc-short-check,$(BUILD)/egg_jtc_short.serial)
@printf '>>> test-flair-egg-jtc: green (third-click trigger, modeless mat, cursor jump, alert, exact authored stills)\n'
```

## test-flair-egg-jtc-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/proptest/egg_jtc.mk', line 100):
Prerequisites: `test-flair-egg-jtc build/flair_tenants_mut_jtc_trigger.img build/.makefile.stamp`
```make
$(call jtc-boot,egg_jtc_mutant,$(BUILD)/flair_tenants_mut_jtc_trigger.img,FLAIR_EGG_JTC_SHORT_SPEC,FLAIR-LIVE-OK)
@if ( $(call jtc-short-check,$(BUILD)/egg_jtc_mutant.serial) ) > $(BUILD)/egg_jtc_trigger_mutant.log 2>&1; then printf 'FAIL JTC mutant: oracle accepted two clicks\n'; exit 1; fi
@grep -qx 'EGG-JTC-OPEN click=2' $(BUILD)/egg_jtc_mutant.serial || { printf 'FAIL JTC mutant: did not fail on early trigger\n'; exit 1; }
@grep -x 'FAIL JTC trigger: two clicks opened mat' $(BUILD)/egg_jtc_trigger_mutant.log
@printf '>>> test-flair-egg-jtc-mutant: EGG_MUT_TRIGGER_OFF_BY_ONE RED (two clicks opened mat)\n'
```

## test-flair-empty-trash

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 21735):
Prerequisites: `build/flair_empty_reference.img build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_trash_check build/ppm_flair_disk_windows_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-empty-trash : Special > Empty Trash (bead initech-6k12)\n'
@printf '  Ref: os/flair/finder_windows.h Sec 12; spec/flair_trash_traces.mk Sec 3.\n'
@printf '  File effects graded by mtools + fsck.fat on the SAME image (Law 2).\n'
@printf '======================================================================\n'
@command -v mdir >/dev/null 2>&1 && command -v fsck.fat >/dev/null 2>&1 || { printf '!!! test-flair-empty-trash FAIL: mtools / fsck.fat (dosfstools) missing\n'; exit 1; }
@# ---- leg 1: the ALERT, left up ----
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_ET_ALERT_NAME),$(BUILD)/$(FLAIR_ET_ALERT_NAME)_data.img,$(FLAIR_EMPTY_ALERT_SPEC),FINDER-TRASH-ALERT n=,1)
$(call fo-has,$(FLAIR_ET_ALERT_NAME),DESK-TRASH-ICON full,staging README.TXT did not turn the Trash FULL)
$(call fo-has,$(FLAIR_ET_ALERT_NAME),FINDER-CMD id=11 name=EMPTY_TRASH src=mouse sel=0,Special > Empty Trash was not chosen (still disabled with a full Trash?))
@k=$$(( ( $$(wc -c < $(FLAIR_DATA_README)) + 1023 ) / 1024 )); grep -qxF "FINDER-TRASH-ALERT n=1 k=$$k" "$(BUILD)/$(FLAIR_ET_ALERT_NAME).serial" || { printf '!!! test-flair-empty-trash FAIL: want FINDER-TRASH-ALERT n=1 k=%s\n' "$$k"; grep -E '^(FINDER|DESK)' "$(BUILD)/$(FLAIR_ET_ALERT_NAME).serial"; exit 1; }
@! grep -q '^FINDER-TRASH-EMPTIED' "$(BUILD)/$(FLAIR_ET_ALERT_NAME).serial" && mdir -i $(BUILD)/$(FLAIR_ET_ALERT_NAME)_data.img ::/TRASH | grep -q '^README   TXT ' || { printf '!!! test-flair-empty-trash FAIL: something was purged while the alert was still up\n'; exit 1; }
@[ -s "$(BUILD)/$(FLAIR_ET_ALERT_NAME).ppm" ] || { printf '!!! test-flair-empty-trash FAIL: ALERT screendump missing\n'; exit 1; }
@$(PPM_FLAIR_TRASH_CHECK_BIN) alert "$(BUILD)/$(FLAIR_ET_ALERT_NAME).ppm" || { printf '!!! test-flair-empty-trash FAIL: the alert / the FULL Trash is not drawn as graded\n'; exit 1; }
@printf '>>> test-flair-empty-trash [1/3]: ALERT -- Empty Trash lit by a full Trash, the confirm alert up (pixel-graded), nothing purged yet\n'
@# ---- leg 2: CANCEL (Escape) ----
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_ET_CANCEL_NAME),$(BUILD)/$(FLAIR_ET_CANCEL_NAME)_data.img,$(FLAIR_EMPTY_CANCEL_SPEC),FINDER-TRASH-EMPTY-CANCEL,0)
$(call fo-has,$(FLAIR_ET_CANCEL_NAME),FINDER-TRASH-EMPTY-CANCEL,Escape did not cancel the alert)
@! grep -q '^FINDER-TRASH-EMPTIED' "$(BUILD)/$(FLAIR_ET_CANCEL_NAME).serial" || { printf '!!! test-flair-empty-trash FAIL: FINDER-TRASH-EMPTIED after a Cancel\n'; exit 1; }
@( $(call et-cancel-mtools,$(BUILD)/$(FLAIR_ET_CANCEL_NAME)_data.img) ) || { printf '!!! test-flair-empty-trash FAIL: the mtools differential of the CANCEL leg (above)\n'; exit 1; }
@printf '>>> test-flair-empty-trash [2/3]: CANCEL -- Escape dismissed the alert; mtools: README.TXT still in ::/TRASH byte-identical, DB 80 B\n'
@# ---- leg 3: OK -- the recursive purge ----
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_ET_OK_NAME),$(BUILD)/$(FLAIR_ET_OK_NAME)_data.img,$(FLAIR_EMPTY_OK_SPEC),FINDER-TRASH-EMPTIED,1)
@a=$$(mcopy -n -i $(FLAIR_DATA_IMG) ::/README.TXT - | wc -c); b=0; \
for f in $$(mdir -b -i $(FLAIR_DATA_IMG) ::/APPS); do b=$$(( b + $$(mcopy -n -i $(FLAIR_DATA_IMG) "$$f" - | wc -c) )); done; \
n=$$(( 2 + $$(mdir -b -i $(FLAIR_DATA_IMG) ::/APPS | wc -l) )); k=$$(( (a + b + 1023) / 1024 )); \
printf '%s\n' 'DESK-TRASH-ICON full' 'FINDER-TRASH name=README.TXT origin=0' 'DESKTOP-DB-SAVE n=3' \
	'FINDER-TRASH name=APPS origin=0' 'DESKTOP-DB-SAVE n=4' 'FINDER-OPEN-TRASH win=1 n=2 singleton=0' \
	'FINDER-CMD id=11 name=EMPTY_TRASH src=mouse sel=0' "FINDER-TRASH-ALERT n=$$n k=$$k" 'DESK-TRASH-ICON empty' \
	"FINDER-TRASH-EMPTIED purged=$$n refused=0" 'DESKTOP-DB-SAVE n=2' > $(FLAIR_ET_CHAIN).want; \
grep -E '^(FINDER-TRASH|FINDER-OPEN-TRASH|FINDER-CMD|DESK-TRASH-ICON|DESKTOP-DB-SAVE)' "$(BUILD)/$(FLAIR_ET_OK_NAME).serial" > $(FLAIR_ET_CHAIN).got || true; \
cmp -s $(FLAIR_ET_CHAIN).want $(FLAIR_ET_CHAIN).got || { printf '!!! test-flair-empty-trash FAIL: the OK serial chain differs from the locked one (n=%s k=%s from mtools)\n' "$$n" "$$k"; diff $(FLAIR_ET_CHAIN).want $(FLAIR_ET_CHAIN).got; exit 1; }
@[ "$$($(call tw-trash-entries,$(BUILD)/$(FLAIR_ET_OK_NAME)_data.img))" = 0 ] || { printf '!!! test-flair-empty-trash FAIL: ::/TRASH is not empty after OK\n'; mdir -i $(BUILD)/$(FLAIR_ET_OK_NAME)_data.img ::/TRASH; exit 1; }
@[ "$$(mdir -a -b -i $(BUILD)/$(FLAIR_ET_OK_NAME)_data.img :: | sort | tr '\n' ' ')" = '::/AUTOEXEC.BAT ::/DESKTOP.DB ::/DOS/ ::/TRASH/ ' ] || { printf '!!! test-flair-empty-trash FAIL: the root differs from AUTOEXEC + DOS + DESKTOP.DB + TRASH after the purge\n'; mdir -a -i $(BUILD)/$(FLAIR_ET_OK_NAME)_data.img ::; exit 1; }
@# The two surviving public root entries retain all DOS chains. Compare the
@# independent fsck used-cluster delta against mtools' pre-purge allocation.
@set -eu; img=$(BUILD)/$(FLAIR_ET_OK_NAME)_data.img; \
 fsck.fat -n $$img > $(BUILD)/flair_empty_fsck.log; \
 fsck.fat -n $(BUILD)/flair_empty_reference.img > $(BUILD)/flair_empty_reference.fsck; \
 a=$$(tail -1 $(BUILD)/flair_empty_fsck.log | sed 's/.*files, //'); \
 b=$$(tail -1 $(BUILD)/flair_empty_reference.fsck | sed 's/.*files, //'); \
 [ "$$a" = "$$b" ] || { echo '!!! empty-trash: surviving allocation differs from mtools'; exit 1; }
@[ "$$(mcopy -n -i $(BUILD)/$(FLAIR_ET_OK_NAME)_data.img ::/DESKTOP.DB - | wc -c)" = 56 ] || { printf '!!! test-flair-empty-trash FAIL: DESKTOP.DB is not 56 bytes (the origins were not dropped)\n'; exit 1; }
@[ -s "$(BUILD)/$(FLAIR_ET_OK_NAME).ppm" ] || { printf '!!! test-flair-empty-trash FAIL: OK screendump missing\n'; exit 1; }
@$(PPM_FLAIR_TRASH_CHECK_BIN) emptied "$(BUILD)/$(FLAIR_ET_OK_NAME).ppm" || { printf '!!! test-flair-empty-trash FAIL: the alert is not gone / the Trash is not EMPTY again\n'; exit 1; }
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) trashempty "$(BUILD)/$(FLAIR_ET_OK_NAME).ppm" || { printf '!!! test-flair-empty-trash FAIL: the open Trash window did not empty\n'; exit 1; }
@printf '>>> test-flair-empty-trash [3/3]: OK -- purged=n (README + APPS + its files, from mtools) depth-first; mtools: ::/TRASH empty, root = AUTOEXEC + DOS + DESKTOP.DB + TRASH; fsck.fat allocation == mtools; Trash window + icon EMPTY (pixel-graded)\n'
@printf '>>> test-flair-empty-trash: green\n'
```

## test-flair-empty-trash-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 21788):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_win_empty_noconfirm.img build/flair_data.img build/.makefile.stamp`
```make
$(call fo-boot,$(BUILD)/flair_tenants_mut_win_empty_noconfirm.img,flair_empty_mut_noconfirm,$(BUILD)/flair_empty_mut_noconfirm_data.img,$(FLAIR_EMPTY_CANCEL_SPEC),FINDER-TRASH-EMPTY-CANCEL,0)
@grep -qxF 'FINDER-TRASH-EMPTY-CANCEL' $(BUILD)/flair_empty_mut_noconfirm.serial || { printf '!!! test-flair-empty-trash-mutant: the mutant never reached the Cancel (not comparable)\n'; grep -E '^(FINDER|DESK)' $(BUILD)/flair_empty_mut_noconfirm.serial; exit 1; }
@out=$$( $(call et-cancel-mtools,$(BUILD)/flair_empty_mut_noconfirm_data.img) ); rc=$$?; \
if [ $$rc -eq 0 ]; then printf '!!! test-flair-empty-trash-mutant FAIL: EMPTY_NO_CONFIRM PASSED the Cancel differential -- decoration\n'; exit 1; fi; \
printf '%s\n' "$$out" | grep -q 'purged without confirmation' || { printf '!!! test-flair-empty-trash-mutant FAIL: RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
printf '>>> test-flair-empty-trash-mutant: EMPTY_NO_CONFIRM correctly RED (%s)\n' "$$out"
```

## test-flair-exit-to-dos

Declared tier: EMU; #  recipe to execute (from 'harness/emu/exit_dos.mk', line 139):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_trash_check build/ppm_text_check spec/dos_banner.txt build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-exit-to-dos : Special > Exit to InitechDOS (initech-3p1a)\n'
@printf '  the alert, the cancel, and the hand-off of the console to COMMAND.COM\n'
@printf '======================================================================\n'
@# ---- leg 1: the ALERT, left up ----
$(call exitdos-boot,$(FLAIRTENANTS_IMG),$(EXITDOS_ALERT_NAME),$(FLAIR_DATA_IMG),$(FLAIR_EXIT_DOS_ALERT_SPEC),FINDER-EXIT-DOS-ALERT,1,40000)
@s=$(BUILD)/$(EXITDOS_ALERT_NAME).serial; \
grep -qxF 'FLAIR-MENU menu=515 item=10 (sel=0x0203000A)' $$s || { printf '!!! test-flair-exit-to-dos FAIL [1]: the release at (170,163) did not select Special item 10: %s\n' "$$(grep '^FLAIR-MENU menu=515' $$s | tr '\n' ' ')"; exit 1; }; \
grep -qxF 'FINDER-CMD id=16 name=EXIT_DOS src=mouse sel=0' $$s || { printf '!!! test-flair-exit-to-dos FAIL [1]: no FINDER-CMD EXIT_DOS\n'; exit 1; }; \
grep -qxF 'FINDER-EXIT-DOS-ALERT' $$s || { printf '!!! test-flair-exit-to-dos FAIL [1]: the confirmation never came up\n'; exit 1; }; \
! grep -qE '^(FINDER-POWER|FLAIR-EXIT-TO-DOS|SHELL-READY|PANIC)' $$s || { printf '!!! test-flair-exit-to-dos FAIL [1]: something happened while the alert was still up: %s\n' "$$(grep -E '^(FINDER-POWER|FLAIR-EXIT-TO-DOS|SHELL-READY|PANIC)' $$s | head -1)"; exit 1; }
@[ -s "$(BUILD)/$(EXITDOS_ALERT_NAME).ppm" ] || { printf '!!! test-flair-exit-to-dos FAIL [1]: no screendump\n'; exit 1; }
@$(PPM_FLAIR_TRASH_CHECK_BIN) exitdos "$(BUILD)/$(EXITDOS_ALERT_NAME).ppm" || { printf '!!! test-flair-exit-to-dos FAIL [1]: the alert is not drawn as graded\n'; exit 1; }
@printf '>>> test-flair-exit-to-dos [1/3]: ALERT -- Special item 10 -> EXIT_DOS -> the confirmation up, graded against s8_alert_modal.png; nothing put away\n'
@# ---- leg 2: CANCEL (Escape) ----
$(call exitdos-boot,$(FLAIRTENANTS_IMG),$(EXITDOS_CANCEL_NAME),$(FLAIR_DATA_IMG),$(FLAIR_EXIT_DOS_CANCEL_SPEC),name=NEW_FOLDER,0,40000)
@s=$(BUILD)/$(EXITDOS_CANCEL_NAME).serial; \
grep -qxF 'FINDER-EXIT-DOS-CANCEL' $$s || { printf '!!! test-flair-exit-to-dos FAIL [2]: Escape did not cancel\n'; exit 1; }; \
! grep -qE '^(FINDER-POWER|FLAIR-EXIT-TO-DOS|SHELL-READY|PANIC)' $$s || { printf '!!! test-flair-exit-to-dos FAIL [2]: the cancel still exited / put away: %s\n' "$$(grep -E '^(FINDER-POWER|FLAIR-EXIT-TO-DOS|SHELL-READY|PANIC)' $$s | head -1)"; exit 1; }; \
sed -n '/^FINDER-EXIT-DOS-CANCEL$$/,$$p' $$s | grep -qE '^FINDER-CMD id=2 name=NEW_FOLDER ' || { printf '!!! test-flair-exit-to-dos FAIL [2]: the desktop did not answer after the cancel\n'; exit 1; }
@printf '>>> test-flair-exit-to-dos [2/3]: CANCEL -- Escape dismissed it, nothing put away, the Finder still answers (Ctrl-N)\n'
@# ---- leg 3: EXIT ----
$(call exitdos-boot,$(FLAIRTENANTS_IMG),$(EXITDOS_EXIT_NAME),$(FLAIR_DATA_IMG),$(FLAIR_EXIT_DOS_SPEC),$(EXITDOS_DONE),1,40000)
@( $(call exitdos-exit-check,$(EXITDOS_EXIT_NAME)) ) || { printf '!!! test-flair-exit-to-dos FAIL [3]: (above)\n'; grep -E '^(FINDER-|FLAIR-EXIT|COMMAND-|SHELL-|DESKTOP-DB|PANIC)' $(BUILD)/$(EXITDOS_EXIT_NAME).serial; exit 1; }
@mtype -i $(BUILD)/$(EXITDOS_EXIT_NAME)_data.img ::DESKTOP.DB > /dev/null 2>&1 || { printf '!!! test-flair-exit-to-dos FAIL [3]: DESKTOP.DB unreadable by mtools after the exit\n'; exit 1; }
@printf '>>> test-flair-exit-to-dos [3/3]: EXIT -- put away + DESKTOP.DB commit -> FLAIR-EXIT-TO-DOS -> transient COMMAND.COM (%s) -> banner == spec -> SHELL-READY -> DIR lists A: (INITECH); the screen is the text console (ppm_text_check)\n' "$$(grep -m1 '^COMMAND-TRANSIENT-OK' $(BUILD)/$(EXITDOS_EXIT_NAME).serial)"
@printf '>>> test-flair-exit-to-dos: green (QEMU; Bochs leg owed)\n'
```

## test-flair-exit-to-dos-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 115):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-exit-to-dos-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-exit-to-dos-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-exit-to-dos_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-exit-to-dos_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-exit-to-dos_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-exit-to-dos_bochs.report.txt" || true
@cat "$(BUILD)/flair-exit-to-dos_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-exit-to-dos-bochs "$(BUILD)/flair-exit-to-dos_bochs.report.txt" "$(BUILD)/flair-exit-to-dos_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-exit-to-dos-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/exit_dos.mk', line 168):
Prerequisites: `build/qemu_harness build/flair_exitdos_mut_no_teardown.img build/flair_exitdos_mut_no_transient.img build/flair_data.img build/ppm_text_check build/.makefile.stamp`
```make
$(call exitdos-boot,$(EXITDOS_MUT_NOTEARDOWN_IMG),flair_exitdos_mut_noteardown,$(FLAIR_DATA_IMG),$(FLAIR_EXIT_DOS_SPEC),$(EXITDOS_DONE),1,40000)
@out=$$( ( $(call exitdos-exit-check,flair_exitdos_mut_noteardown) ) 2>&1 ); rc=$$?; \
if [ $$rc -eq 0 ]; then printf '!!! test-flair-exit-to-dos-mutant FAIL: NO_TEARDOWN PASSED -- decoration\n'; exit 1; fi; \
printf '%s\n' "$$out" | grep -qF 'the desktop is still painted' || { printf '!!! test-flair-exit-to-dos-mutant FAIL: NO_TEARDOWN RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
printf '>>> test-flair-exit-to-dos-mutant: NO_TEARDOWN correctly RED (%s)\n' "$$out"
$(call exitdos-boot,$(EXITDOS_MUT_NOTRANSIENT_IMG),flair_exitdos_mut_notransient,$(FLAIR_DATA_IMG),$(FLAIR_EXIT_DOS_SPEC),HALTED,0,40000)
@out=$$( ( $(call exitdos-exit-check,flair_exitdos_mut_notransient) ) 2>&1 ); rc=$$?; \
if [ $$rc -eq 0 ]; then printf '!!! test-flair-exit-to-dos-mutant FAIL: NO_TRANSIENT PASSED -- decoration\n'; exit 1; fi; \
printf '%s\n' "$$out" | grep -qF 'did not run alert -> put away' || { printf '!!! test-flair-exit-to-dos-mutant FAIL: NO_TRANSIENT RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
grep -qxF 'Bad or missing Command Interpreter' $(BUILD)/flair_exitdos_mut_notransient.sx && grep -q '^PANIC exit-to-dos: COMMAND.COM not found' $(BUILD)/flair_exitdos_mut_notransient.serial \
	|| { printf '!!! test-flair-exit-to-dos-mutant FAIL: NO_TRANSIENT did not say MSG-DOS-0014 + PANIC\n'; exit 1; }; \
printf '>>> test-flair-exit-to-dos-mutant: NO_TRANSIENT correctly RED (%s; MSG-DOS-0014 + %s)\n' "$$out" "$$(grep -m1 '^PANIC' $(BUILD)/flair_exitdos_mut_notransient.serial)"
@printf '>>> test-flair-exit-to-dos-mutant: green (2/2 RED for the named reason)\n'
```

## test-flair-fg-close

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22336):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_app_launch_check build/ppm_flair_solid_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-fg-close : foreground, band 2 and active window agree (tdnl.40)\n'
@printf '======================================================================\n'
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_FGC_NAME),$(BUILD)/$(FLAIR_FGC_NAME)_data.img,$(FLAIR_FG_CLOSE_SPEC),FLAIR-MENU menu=,1)
@( $(call fgc-close-check,$(BUILD)/$(FLAIR_FGC_NAME).serial) ) || { printf '!!! test-flair-fg-close FAIL: close serial (above)\n'; grep -E '^(FINDER-|FLAIR-DISPATCH|FLAIR-MENU)' $(BUILD)/$(FLAIR_FGC_NAME).serial; exit 1; }
$(call fgc-px,$(BUILD)/$(FLAIR_FGC_NAME).ppm,bar-finder,L,close)
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_FGC_NAME)_click,$(BUILD)/$(FLAIR_FGC_NAME)_click_data.img,$(FLAIR_FG_CLOSE_CLICK_SPEC),FLAIR-DISPATCH app=HELLO,1)
@( $(call fgc-close-click-check,$(BUILD)/$(FLAIR_FGC_NAME)_click.serial) ) || { printf '!!! test-flair-fg-close FAIL: close_click serial (above)\n'; grep -E '^(FINDER-|FLAIR-DISPATCH|FLAIR-MENU)' $(BUILD)/$(FLAIR_FGC_NAME)_click.serial; exit 1; }
$(call fgc-px,$(BUILD)/$(FLAIR_FGC_NAME)_click.ppm,bar-photoshop,K,close_click)
$(call fo-boot,$(FLAIRTENANTS_IMG),flair_fg_desk,$(BUILD)/flair_fg_desk_data.img,$(FLAIR_FG_DESK_SPEC),FLAIR-MENU menu=,1)
@( $(call fgc-desk-check,$(BUILD)/flair_fg_desk.serial) ) || { printf '!!! test-flair-fg-close FAIL: desk serial (above)\n'; grep -E '^(FINDER-|FLAIR-DISPATCH|FLAIR-MENU)' $(BUILD)/flair_fg_desk.serial; exit 1; }
$(call fgc-px,$(BUILD)/flair_fg_desk.ppm,bar-finder,L,desk)
$(call fo-boot,$(FLAIRTENANTS_IMG),flair_fg_desk_click,$(BUILD)/flair_fg_desk_click_data.img,$(FLAIR_FG_DESK_CLICK_SPEC),FLAIR-DISPATCH app=HELLO,1)
@( $(call fgc-desk-click-check,$(BUILD)/flair_fg_desk_click.serial) ) || { printf '!!! test-flair-fg-close FAIL: desk_click serial (above)\n'; grep -E '^(FINDER-|FLAIR-DISPATCH|FLAIR-MENU)' $(BUILD)/flair_fg_desk_click.serial; exit 1; }
$(call fgc-px,$(BUILD)/flair_fg_desk_click.ppm,bar-photoshop,K,desk_click)
@if $(PPM_FLAIR_SOLID_CHECK_BIN) K "$(BUILD)/$(FLAIR_FGC_NAME).ppm" >/dev/null 2>&1; then printf '!!! test-flair-fg-close FAIL: leg K passed on the no-active-window frame -- decoration\n'; exit 1; fi
@if $(PPM_FLAIR_SOLID_CHECK_BIN) L "$(BUILD)/$(FLAIR_FGC_NAME)_click.ppm" >/dev/null 2>&1; then printf '!!! test-flair-fg-close FAIL: leg L passed on the HELLO-active frame -- decoration\n'; exit 1; fi
@if $(PPM_FLAIR_APPL_CHECK_BIN) bar-photoshop "$(BUILD)/$(FLAIR_FGC_NAME).ppm" >/dev/null 2>&1; then printf '!!! test-flair-fg-close FAIL: bar-photoshop passed on the Finder-bar frame -- decoration\n'; exit 1; fi
@if $(PPM_FLAIR_APPL_CHECK_BIN) bar-finder "$(BUILD)/$(FLAIR_FGC_NAME)_click.ppm" >/dev/null 2>&1; then printf '!!! test-flair-fg-close FAIL: bar-finder passed on the Photoshop-bar frame -- decoration\n'; exit 1; fi
@printf '>>> test-flair-fg-close: green (close keeps the Finder foreground, no window active; desktop click activates the Finder; a HELLO click brings HELLO back; graders discriminate)\n'
```

## test-flair-fg-close-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22368):
Prerequisites: `build/qemu_harness build/flair_data.img build/flair_tenants_mut_fg_follows_front.img build/flair_tenants_mut_no_desk_activate.img build/.makefile.stamp`
```make
$(call fgc-mut,fg_follows_front,flair_fg_close_mut,$(FLAIR_FG_CLOSE_SPEC),fgc-close-check,the Finder lost the foreground on the close,FG_FOLLOWS_FRONT/close)
$(call fgc-mut,fg_follows_front,flair_fg_desk_mut,$(FLAIR_FG_DESK_SPEC),fgc-desk-check,the Finder did not keep the foreground after the desktop click,FG_FOLLOWS_FRONT/desk)
$(call fgc-mut,no_desk_activate,flair_fg_desk_mut2,$(FLAIR_FG_DESK_SPEC),fgc-desk-check,the desktop click did not activate the Finder,NO_DESK_ACTIVATE/desk)
```

## test-flair-file-copy

Declared tier: EMU; #  recipe to execute (from 'harness/frameapps.mk', line 193):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_file_copy_check build/.makefile.stamp`
```make
@command -v mtype >/dev/null 2>&1 && command -v fsck.fat >/dev/null 2>&1 || { printf '!!! test-flair-file-copy FAIL: mtools / fsck.fat missing\n'; exit 1; }
$(call appl-boot,flair_file_copy_mid,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_FILE_COPY_SPEC,--screendump --screendump-after TENANT-FILECOPY-MID)
@$(call APPL_ALIVE,$(BUILD)/flair_file_copy_mid.serial) && ( $(call file-copy-serial-ok,$(BUILD)/flair_file_copy_mid.serial,$(BUILD)/flair_file_copy_mid_data.img) ) && grep -q '^TENANT-FILECOPY-MID v=' $(BUILD)/flair_file_copy_mid.serial || { printf '!!! test-flair-file-copy FAIL: the MID leg serial (request / rate / size)\n'; grep -E '^(TENANT-(GATE ax=0x0091|FILECOPY|EVT what=145)|FILECOPY)' $(BUILD)/flair_file_copy_mid.serial; exit 1; }
@$(PPM_FLAIR_FILE_COPY_CHECK_BIN) mid $(BUILD)/flair_file_copy_mid.ppm || { printf '!!! test-flair-file-copy FAIL: the mid-copy box pixels\n'; exit 1; }
@printf '>>> test-flair-file-copy [1/2]: MID -- Save asked the box; the shell read rate=%s from MICHAEL.CFG; the partial Platinum bar matches the minted captures pixel-for-pixel; the PRD caption ink\n' "$$(sed -n 's/^TENANT-FILECOPY s=.* rate=//p' $(BUILD)/flair_file_copy_mid.serial)"
$(call appl-boot,flair_file_copy_done,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_FILE_COPY_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_file_copy_done.serial) && ( $(call file-copy-serial-ok,$(BUILD)/flair_file_copy_done.serial,$(BUILD)/flair_file_copy_done_data.img) ) || { printf '!!! test-flair-file-copy FAIL: the DONE leg serial\n'; exit 1; }
@( $(call file-copy-rate-ok,$(BUILD)/flair_file_copy_done.serial,$(BUILD)/flair_file_copy_done_data.img) ) || { printf '!!! test-flair-file-copy FAIL: RATE -- completion ticks are not the configured PROGRESS_BAR_RATE\n'; grep -E '^TENANT-FILECOPY' $(BUILD)/flair_file_copy_done.serial; exit 1; }
@( $(call file-copy-fat-ok,$(BUILD)/flair_file_copy_done_data.img,flair_file_copy_done) ) || { printf '!!! test-flair-file-copy FAIL: FAT -- APPS\\TABLES.TXT is not a byte-exact copy of the ledger (mtools / fat12_ref.py / fsck.fat)\n'; mdir -i $(BUILD)/flair_file_copy_done_data.img ::APPS; exit 1; }
@$(PPM_FLAIR_FILE_COPY_CHECK_BIN) done $(BUILD)/flair_file_copy_done.ppm || { printf '!!! test-flair-file-copy FAIL: the box is still on screen after completion\n'; exit 1; }
@printf '>>> test-flair-file-copy [2/2]: DONE -- %s; TABLES.TXT == LEDGER.TXT == fixture (mtools + python FAT12 reader), fsck.fat clean; box disposed\n' "$$(grep -m1 '^TENANT-FILECOPY-DONE' $(BUILD)/flair_file_copy_done.serial)"
@printf '>>> test-flair-file-copy: green\n'
```

## test-flair-file-copy-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 117):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-file-copy-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-file-copy-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-file-copy_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-file-copy_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-file-copy_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-file-copy_bochs.report.txt" || true
@cat "$(BUILD)/flair-file-copy_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-file-copy-bochs "$(BUILD)/flair-file-copy_bochs.report.txt" "$(BUILD)/flair-file-copy_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-file-copy-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/frameapps.mk', line 207):
Prerequisites: `test-flair-file-copy build/flair_tenants_mut_filecopy_rate_ignored.img build/flair_tenants_mut_filecopy_no_copy.img build/flair_tenants_mut_progress_flat.img build/flair_data_calc_CALC_MUT_CAPTION.img build/.makefile.stamp`
```make
$(call appl-boot,flair_file_copy_rate,$(BUILD)/flair_tenants_mut_filecopy_rate_ignored.img,$(FLAIR_DATA_IMG),FLAIR_FILE_COPY_SPEC,)
@if ( $(call file-copy-rate-ok,$(BUILD)/flair_file_copy_rate.serial,$(BUILD)/flair_file_copy_rate_data.img) ); then printf '!!! test-flair-file-copy-mutant FAIL: TBX_MUT_FILECOPY_RATE_IGNORED survived\n'; exit 1; fi
@grep -q '^TENANT-FILECOPY-DONE rc=' $(BUILD)/flair_file_copy_rate.serial || { printf '!!! rate mutant did not run the box\n'; exit 1; }
@printf '>>> test-flair-file-copy-mutant: TBX_MUT_FILECOPY_RATE_IGNORED RED: RATE (%s, want ceil(10000/rate))\n' "$$(grep -m1 '^TENANT-FILECOPY-DONE' $(BUILD)/flair_file_copy_rate.serial)"
$(call appl-boot,flair_file_copy_nocopy,$(BUILD)/flair_tenants_mut_filecopy_no_copy.img,$(FLAIR_DATA_IMG),FLAIR_FILE_COPY_SPEC,)
@grep -q '^TENANT-FILECOPY-DONE rc=' $(BUILD)/flair_file_copy_nocopy.serial || { printf '!!! no-copy mutant did not run the box\n'; exit 1; }
@if ( $(call file-copy-fat-ok,$(BUILD)/flair_file_copy_nocopy_data.img,flair_file_copy_nocopy) ); then printf '!!! test-flair-file-copy-mutant FAIL: TBX_MUT_FILECOPY_NO_COPY survived\n'; exit 1; fi
@printf '>>> test-flair-file-copy-mutant: TBX_MUT_FILECOPY_NO_COPY RED: FAT (%s)\n' "$$(mdir -b -i $(BUILD)/flair_file_copy_nocopy_data.img ::APPS/TABLES.TXT 2>&1 | head -1)"
$(call appl-boot,flair_file_copy_caption,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_CALC_CALC_MUT_CAPTION_IMG),FLAIR_FILE_COPY_SPEC,--screendump --screendump-after TENANT-FILECOPY-MID)
@grep -q '^TENANT-FILECOPY-MID' $(BUILD)/flair_file_copy_caption.serial || { printf '!!! caption mutant did not run the box\n'; exit 1; }
@if ( $(call file-copy-serial-ok,$(BUILD)/flair_file_copy_caption.serial,$(BUILD)/flair_file_copy_caption_data.img) ); then printf '!!! test-flair-file-copy-mutant FAIL: CALC_MUT_CAPTION survived the serial leg\n'; exit 1; fi
@if $(PPM_FLAIR_FILE_COPY_CHECK_BIN) mid $(BUILD)/flair_file_copy_caption.ppm > $(BUILD)/flair_file_copy_caption.pixels 2>&1; then printf '!!! test-flair-file-copy-mutant FAIL: CALC_MUT_CAPTION survived the pixel leg\n'; exit 1; fi
@grep -q 'FAIL file-copy CAPTION' $(BUILD)/flair_file_copy_caption.pixels || { cat $(BUILD)/flair_file_copy_caption.pixels; exit 1; }
@printf '>>> test-flair-file-copy-mutant: CALC_MUT_CAPTION RED: %s\n' "$$(grep -m1 'FAIL file-copy CAPTION' $(BUILD)/flair_file_copy_caption.pixels)"
$(call appl-boot,flair_file_copy_flat,$(BUILD)/flair_tenants_mut_progress_flat.img,$(FLAIR_DATA_IMG),FLAIR_FILE_COPY_SPEC,--screendump --screendump-after TENANT-FILECOPY-MID)
@grep -q '^TENANT-FILECOPY-MID' $(BUILD)/flair_file_copy_flat.serial || { printf '!!! flat-bar mutant did not run the box\n'; exit 1; }
@if $(PPM_FLAIR_FILE_COPY_CHECK_BIN) mid $(BUILD)/flair_file_copy_flat.ppm > $(BUILD)/flair_file_copy_flat.pixels 2>&1; then printf '!!! test-flair-file-copy-mutant FAIL: CTRL_MUT_PROGRESS_FLAT survived the pixel leg\n'; exit 1; fi
@grep -q 'FAIL file-copy BAR' $(BUILD)/flair_file_copy_flat.pixels || { cat $(BUILD)/flair_file_copy_flat.pixels; exit 1; }
@printf '>>> test-flair-file-copy-mutant: CTRL_MUT_PROGRESS_FLAT RED: %s\n' "$$(grep -m1 'FAIL file-copy BAR' $(BUILD)/flair_file_copy_flat.pixels)"
@printf '>>> test-flair-file-copy-mutant: green (four named mutants RED)\n'
```

## test-flair-file-ops

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 21355):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/tenantfx.exe build/ppm_flair_drop_target_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-file-ops : Finder DRAG-MOVE + DRAG-TO-TRASH (R3.4a)\n'
@printf '  Ref: bead initech-34dh; os/flair/finder_ops.h; spec/flair_file_ops_traces.mk.\n'
@printf '  File effects graded by mtools on the SAME image (the FAT rail, Law 2).\n'
@printf '======================================================================\n'
@command -v mdir >/dev/null 2>&1 && command -v mtype >/dev/null 2>&1 || { printf '!!! test-flair-file-ops FAIL: mtools (mdir/mtype/mcopy) missing\n'; exit 1; }
@# ---- leg 1: INTO a folder icon ----
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_FO_INTO_NAME),$(BUILD)/$(FLAIR_FO_INTO_NAME)_data.img,$(FLAIR_FILEOPS_INTO_SPEC),FINDER-MOVE name=README.TXT from=0 to=3,0)
$(call fo-has,$(FLAIR_FO_INTO_NAME),FINDER-OPEN-VOLUME win=0 n=4,the volume double-click did not open the root window)
$(call fo-has,$(FLAIR_FO_INTO_NAME),FINDER-DROP-HILITE name=APPS,the APPS folder was not lit as the drop target)
$(call fo-has,$(FLAIR_FO_INTO_NAME),FINDER-MOVE name=README.TXT from=0 to=3,the drop on APPS did not move README.TXT into APPS (cluster 3))
@mdir -i $(BUILD)/$(FLAIR_FO_INTO_NAME)_data.img ::/APPS | grep -q '^README   TXT ' || { printf '!!! test-flair-file-ops FAIL: mtools does not see README.TXT in ::/APPS\n'; exit 1; }
@mtype -i $(BUILD)/$(FLAIR_FO_INTO_NAME)_data.img ::/APPS/README.TXT | cmp -s - $(FLAIR_DATA_README) || { printf '!!! test-flair-file-ops FAIL: ::/APPS/README.TXT bytes differ from the fixture\n'; exit 1; }
@! mdir -i $(BUILD)/$(FLAIR_FO_INTO_NAME)_data.img :: | grep -q '^README   TXT ' || { printf '!!! test-flair-file-ops FAIL: README.TXT is STILL in the root after the move\n'; exit 1; }
@printf '>>> test-flair-file-ops [1/5]: INTO -- FINDER-MOVE README.TXT 0->3; mtools sees it in ::/APPS (bytes intact), not in ::/\n'
@# ---- leg 2: the MID-DRAG frame ----
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_FO_HOVER_NAME),$(BUILD)/$(FLAIR_FO_HOVER_NAME)_data.img,$(FLAIR_FILEOPS_HOVER_SPEC),FINDER-DROP-HILITE name=APPS,1)
@[ -s "$(BUILD)/$(FLAIR_FO_HOVER_NAME).ppm" ] || { printf '!!! test-flair-file-ops FAIL: HOVER screendump missing\n'; exit 1; }
@$(PPM_FLAIR_DROP_CHECK_BIN) "$(BUILD)/$(FLAIR_FO_HOVER_NAME).ppm" || { printf '!!! test-flair-file-ops FAIL: the mid-drag frame does not show the highlighted target + gray outline\n'; exit 1; }
@printf '>>> test-flair-file-ops [2/5]: HOVER -- APPS drawn highlighted under the dragged outline (pixel-graded)\n'
@# ---- leg 3: BETWEEN windows ----
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_FO_BETWEEN_NAME),$(BUILD)/$(FLAIR_FO_BETWEEN_NAME)_data.img,$(FLAIR_FILEOPS_BETWEEN_SPEC),FLAIR-DRAG win 0 (20$(fo_comma)60)->(20$(fo_comma)60),0)
$(call fo-has,$(FLAIR_FO_BETWEEN_NAME),FINDER-OPEN-FOLDER name=APPS win=1 singleton=0,APPS did not open as window 1)
$(call fo-has,$(FLAIR_FO_BETWEEN_NAME),FINDER-MOVE name=TENANTFX.EXE from=3 to=0,TENANTFX.EXE was not moved from the APPS window into the root window)
@mcopy -n -i $(BUILD)/$(FLAIR_FO_BETWEEN_NAME)_data.img ::/TENANTFX.EXE - 2>/dev/null | cmp -s - $(TENANTFX_EXE) || { printf '!!! test-flair-file-ops FAIL: mtools does not read ::/TENANTFX.EXE back byte-identical to the shipped build\n'; exit 1; }
@! mdir -i $(BUILD)/$(FLAIR_FO_BETWEEN_NAME)_data.img ::/APPS | grep -q '^TENANTFX EXE' || { printf '!!! test-flair-file-ops FAIL: TENANTFX.EXE is STILL in ::/APPS\n'; exit 1; }
@printf '>>> test-flair-file-ops [3/5]: BETWEEN -- FINDER-MOVE TENANTFX.EXE 3->0; mtools reads it from ::/ byte-identical, gone from ::/APPS\n'
@# ---- leg 4: two REFUSED drops (re-keyed at tdnl.56: the CYCLE drop is now APPS into its own window) ----
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_FO_REFUSE_NAME),$(BUILD)/$(FLAIR_FO_REFUSE_NAME)_data.img,$(FLAIR_FILEOPS_REFUSE_SPEC),FINDER-MOVE-REFUSED reason=cycle name=APPS,0)
$(call fo-has,$(FLAIR_FO_REFUSE_NAME),FINDER-DROP-HILITE name=INITECH,the volume icon was not lit under the dragged README.TXT)
$(call fo-has,$(FLAIR_FO_REFUSE_NAME),FINDER-MOVE-REFUSED reason=samedir name=README.TXT,README.TXT onto its own volume was not refused SAMEDIR)
$(call fo-has,$(FLAIR_FO_REFUSE_NAME),FINDER-OPEN-FOLDER name=APPS win=1 singleton=0,APPS did not open as window 1)
@grep -Eq '^FLAIR-DRAG win [0-9]+ \(40,80\)->\(190,230\)$$' "$(BUILD)/$(FLAIR_FO_REFUSE_NAME).serial" || { printf '!!! test-flair-file-ops FAIL: the APPS window was not dragged to (190,230), so the root APPS icon was not uncovered\n'; grep -E '^(FLAIR-DRAG|FINDER)' "$(BUILD)/$(FLAIR_FO_REFUSE_NAME).serial"; exit 1; }
$(call fo-has,$(FLAIR_FO_REFUSE_NAME),FINDER-MOVE-REFUSED reason=cycle name=APPS,the APPS folder into its own window was not refused CYCLE)
@! grep -qE '^(FINDER-MOVE |FINDER-TRASH |DESKTOP-DB-SAVE)' "$(BUILD)/$(FLAIR_FO_REFUSE_NAME).serial" || { printf '!!! test-flair-file-ops FAIL: a refused drop still committed something\n'; grep -E '^(FINDER|DESKTOP)' "$(BUILD)/$(FLAIR_FO_REFUSE_NAME).serial"; exit 1; }
@mdir -a -i $(BUILD)/$(FLAIR_FO_REFUSE_NAME)_data.img :: > $(BUILD)/$(FLAIR_FO_REFUSE_NAME).root.lst; \
for n in '^README   TXT ' '^APPS .*<DIR>' '^DESKTOP  DB ' '^TRASH .*<DIR>'; do grep -qE "$$n" $(BUILD)/$(FLAIR_FO_REFUSE_NAME).root.lst || { printf '!!! test-flair-file-ops FAIL: after the refusals the root lost %s\n' "$$n"; cat $(BUILD)/$(FLAIR_FO_REFUSE_NAME).root.lst; exit 1; }; done
@mdir -i $(BUILD)/$(FLAIR_FO_REFUSE_NAME)_data.img ::/APPS | grep -q '^TENANTFX EXE' && ! mdir -i $(BUILD)/$(FLAIR_FO_REFUSE_NAME)_data.img ::/APPS | grep -q '^APPS ' || { printf '!!! test-flair-file-ops FAIL: ::/APPS changed on the refused CYCLE drop\n'; exit 1; }
@[ "$$(mdir -i $(BUILD)/$(FLAIR_FO_REFUSE_NAME)_data.img ::/TRASH | grep -cvE '^( Volume|Directory|\.  |\.\. |$$| +[0-9])')" = 0 ] || { printf '!!! test-flair-file-ops FAIL: ::/TRASH is not empty after two refused drops\n'; mdir -i $(BUILD)/$(FLAIR_FO_REFUSE_NAME)_data.img ::/TRASH; exit 1; }
@[ "$$(mcopy -n -i $(BUILD)/$(FLAIR_FO_REFUSE_NAME)_data.img ::/DESKTOP.DB - | wc -c)" = 8 ] || { printf '!!! test-flair-file-ops FAIL: DESKTOP.DB changed on a refused drop\n'; exit 1; }
@printf '>>> test-flair-file-ops [4/5]: REFUSE -- reason=samedir + reason=cycle (APPS into its own window), nothing committed (mtools: root intact, ::/TRASH empty, DB untouched)\n'
@# ---- leg 5: three drags to the TRASH ----
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_FO_TRASH_NAME),$(BUILD)/$(FLAIR_FO_TRASH_NAME)_data.img,$(FLAIR_FILEOPS_TRASH_SPEC),DESKTOP-DB-SAVE n=6,0)
@printf '%s\n' 'FINDER-TRASH name=README.TXT origin=0' 'DESKTOP-DB-SAVE n=3' \
	'FINDER-NEW-FOLDER name=NEWFOLD parent=0' 'FINDER-TRASH name=NEWFOLD origin=0' 'DESKTOP-DB-SAVE n=4' \
	'FINDER-NEW-FOLDER name=NEWFOLD parent=0' 'TRASH-RENAME from=NEWFOLD to=NEWFO001' \
	'FINDER-TRASH name=NEWFO001 origin=0' 'DESKTOP-DB-SAVE n=6' > $(FLAIR_FO_TRASH_CHAIN).want
@grep -E '^(FINDER-TRASH|FINDER-MOVE|TRASH-RENAME|FINDER-NEW-FOLDER|DESKTOP-DB-SAVE)' "$(BUILD)/$(FLAIR_FO_TRASH_NAME).serial" > $(FLAIR_FO_TRASH_CHAIN).got || true
@cmp -s $(FLAIR_FO_TRASH_CHAIN).want $(FLAIR_FO_TRASH_CHAIN).got || { printf '!!! test-flair-file-ops FAIL: the TRASH serial chain differs from the locked one\n'; diff $(FLAIR_FO_TRASH_CHAIN).want $(FLAIR_FO_TRASH_CHAIN).got; exit 1; }
@( $(call fo-trash-mtools,$(BUILD)/$(FLAIR_FO_TRASH_NAME)_data.img) ) || { printf '!!! test-flair-file-ops FAIL: the mtools differential of the Trash leg (above)\n'; exit 1; }
@# lane/finderalerts RE-KEY (tdnl.57, explicit, Rule 8): the renamed NEWFO001
@# origin is followed by its kind=6 original-name companion (same origin dir,
@# the original 8.3 name, flags/coordinates zero; os/flair/finder_desktop.h
@# IDB1 layout), so the third save is n=6 and the file 8 + 6*24 = 152 B.
@{ for r in 5:0:README.TXT 5:0:NEWFOLD 5:1:NEWFO001 6:0:NEWFOLD; do k=$${r%%:*}; r=$${r#*:}; f=$${r%%:*}; n=$${r#*:}; printf "\\00$$k\\00$$f\\000\\000"; printf '%s' "$$n"; head -c $$((20 - $${#n})) /dev/zero; done; } > $(FLAIR_FO_ORIGINS_GOLD)
@mcopy -n -i $(BUILD)/$(FLAIR_FO_TRASH_NAME)_data.img ::/DESKTOP.DB $(BUILD)/$(FLAIR_FO_TRASH_NAME).db
@[ "$$(wc -c < $(BUILD)/$(FLAIR_FO_TRASH_NAME).db)" = 152 ] || { printf '!!! test-flair-file-ops FAIL: DESKTOP.DB is not 8 + 6*24 = 152 bytes\n'; exit 1; }
@tail -c 96 $(BUILD)/$(FLAIR_FO_TRASH_NAME).db | cmp -s - $(FLAIR_FO_ORIGINS_GOLD) || { printf '!!! test-flair-file-ops FAIL: the kind=5 origin records + the kind=6 companion differ from the hand-authored four\n'; od -A d -t x1 $(BUILD)/$(FLAIR_FO_TRASH_NAME).db | tail -6; exit 1; }
@printf '>>> test-flair-file-ops [5/5]: TRASH -- 3 stagings, TRASH-RENAME NEWFOLD->NEWFO001; mtools: all three in ::/TRASH (README bytes intact), none in ::/; 3 kind=5 origins + the kind=6 NEWFOLD companion on disk\n'
@printf '>>> test-flair-file-ops: green\n'
```

## test-flair-file-ops-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 228):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-file-ops-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-file-ops-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-file-ops_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-file-ops_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-file-ops_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-file-ops_bochs.report.txt" || true
@cat "$(BUILD)/flair-file-ops_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-file-ops-bochs "$(BUILD)/flair-file-ops_bochs.report.txt" "$(BUILD)/flair-file-ops_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK DESKTOP-DB-CREATE TRASH-CREATE FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-file-ops-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 21425):
Prerequisites: `build/qemu_harness build/flair_data.img build/ppm_flair_drop_target_check build/flair_tenants_mut_ops_trash_no_stage.img build/flair_tenants_mut_desk_no_hilite.img build/.makefile.stamp`
```make
$(call fo-boot,$(BUILD)/flair_tenants_mut_ops_trash_no_stage.img,flair_fileops_mut_nostage,$(BUILD)/flair_fileops_mut_nostage_data.img,$(FLAIR_FILEOPS_TRASH_SPEC),FINDER-TRASH name=README.TXT origin=0,0)
@grep -qxF 'FINDER-TRASH name=README.TXT origin=0' $(BUILD)/flair_fileops_mut_nostage.serial || { printf '!!! test-flair-file-ops-mutant FAIL: TRASH_NO_STAGE boot never reached the drop\n'; exit 1; }
@out=$$( $(call fo-trash-mtools,$(BUILD)/flair_fileops_mut_nostage_data.img) ); rc=$$?; \
if [ $$rc -eq 0 ]; then printf '!!! test-flair-file-ops-mutant FAIL: TRASH_NO_STAGE PASSED the mtools differential -- decoration\n'; exit 1; fi; \
printf '%s\n' "$$out" | grep -q 'README.TXT is NOT in ::/TRASH' || { printf '!!! test-flair-file-ops-mutant FAIL: TRASH_NO_STAGE went RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
printf '>>> test-flair-file-ops-mutant: TRASH_NO_STAGE correctly RED (%s)\n' "$$out"
$(call fo-boot,$(BUILD)/flair_tenants_mut_desk_no_hilite.img,flair_fileops_mut_nohilite,$(BUILD)/flair_fileops_mut_nohilite_data.img,$(FLAIR_FILEOPS_HOVER_SPEC),FINDER-DROP-HILITE name=APPS,1)
@if $(PPM_FLAIR_DROP_CHECK_BIN) $(BUILD)/flair_fileops_mut_nohilite.ppm > $(BUILD)/flair_fileops_mut_nohilite.grade 2>&1; then printf '!!! test-flair-file-ops-mutant FAIL: NO_HILITE PASSED the mid-drag grader -- decoration\n'; exit 1; fi
@grep -q 'APPS face pixel is the HIGHLIGHTED #545487' $(BUILD)/flair_fileops_mut_nohilite.grade || { printf '!!! test-flair-file-ops-mutant FAIL: NO_HILITE went RED for the wrong reason\n'; cat $(BUILD)/flair_fileops_mut_nohilite.grade; exit 1; }
@printf '>>> test-flair-file-ops-mutant: NO_HILITE correctly RED (the target body is not darkened)\n'
@printf '>>> test-flair-file-ops-mutant: green (both emu mutants RED for the named reason)\n'
```

## test-flair-files

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 31996):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_file_data.img build/.makefile.stamp`
```make
$(call appl-boot,flair_files,$(FLAIRTENANTS_IMG),$(FLAIR_FILE_DATA),FLAIR_FILE_ROUND,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call FILE_ROUND_OK,$(BUILD)/flair_files.serial) || { printf '!!! test-flair-files FAIL: tenant file roundtrip\n'; grep 'FILE .*FAILED' $(BUILD)/flair_files.serial; exit 1; }
mcopy -i $(BUILD)/flair_files_data.img ::KEEP.DAT $(BUILD)/filekeep.actual
@cmp $(BUILD)/filekeep.golden $(BUILD)/filekeep.actual
@if mdir -i $(BUILD)/flair_files_data.img ::DELE.DAT > /dev/null 2>&1; then printf '!!! test-flair-files FAIL: deleted file exists\n'; exit 1; fi
fsck.fat -n $(BUILD)/flair_files_data.img
@printf '>>> test-flair-files: green (tenant reread + mtools bytes/delete + fsck.fat)\n'
```

## test-flair-files-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32004):
Prerequisites: `test-flair-files build/flair_tenants_mut_file_write.img build/.makefile.stamp`
```make
$(call appl-boot,flair_files_mut,$(BUILD)/flair_tenants_mut_file_write.img,$(FLAIR_FILE_DATA),FLAIR_FILE_ROUND,)
@$(call APPL_ALIVE,$(BUILD)/flair_files_mut.serial) && grep -q 's="FILE READ FAILED"' $(BUILD)/flair_files_mut.serial
mcopy -o -i $(BUILD)/flair_files_mut_data.img ::KEEP.DAT $(BUILD)/filekeep.mutant
@if cmp -s $(BUILD)/filekeep.golden $(BUILD)/filekeep.mutant; then printf '!!! test-flair-files-mutant FAIL: disk bytes survived mutant\n'; exit 1; fi
@printf '>>> test-flair-files-mutant: green (WRITE_NOOP correctly RED: reread and mtools bytes)\n'
```

## test-flair-files-reap

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32010):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_file_data.img build/.makefile.stamp`
```make
$(call appl-boot,flair_files_quit,$(FLAIRTENANTS_IMG),$(FLAIR_FILE_DATA),FLAIR_FILE_QUIT,)
$(call appl-boot,flair_files_close,$(FLAIRTENANTS_IMG),$(FLAIR_FILE_DATA),FLAIR_FILE_CLOSE,)
$(call appl-boot,flair_files_crash,$(FLAIRTENANTS_IMG),$(FLAIR_FILE_DATA),FLAIR_FILE_CRASH,)
@for leg in quit close crash; do $(call FILE_REAP_OK,$(BUILD)/flair_files_$$leg.serial) || { printf '!!! test-flair-files-reap FAIL: %s\n' $$leg; exit 1; }; fsck.fat -n $(BUILD)/flair_files_$${leg}_data.img || exit 1; done
@grep -qx 'TENANT-EXIT rc=0 via=exit' $(BUILD)/flair_files_quit.serial
@grep -qx 'TENANT-EXIT rc=0 via=close' $(BUILD)/flair_files_close.serial
@grep -qx 'TENANT-EXIT rc=-1 via=crash' $(BUILD)/flair_files_crash.serial
@$(call FILE_CRASH_OK,$(BUILD)/flair_files_crash.serial) || { printf '!!! test-flair-files-reap FAIL: crash alerts must appear and be dismissed with Return twice\n'; exit 1; }
@printf '>>> test-flair-files-reap: green (twice fifteen opens: quit, close, crash with damaged PSP)\n'
```

## test-flair-files-reap-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32020):
Prerequisites: `test-flair-files-reap build/flair_tenants_mut_file_leak.img build/.makefile.stamp`
```make
$(call appl-boot,flair_files_leak,$(BUILD)/flair_tenants_mut_file_leak.img,$(FLAIR_FILE_DATA),FLAIR_FILE_QUIT,)
@$(call APPL_ALIVE,$(BUILD)/flair_files_leak.serial) && grep -q 's="FILE OPEN LIMIT FAILED"' $(BUILD)/flair_files_leak.serial && grep -q '^TENANT-GATE ax=0x0081.* -> -260$$' $(BUILD)/flair_files_leak.serial
@printf '>>> test-flair-files-reap-mutant: green (FILE_LEAK correctly RED: second launch exhausts SFT)\n'
```

## test-flair-finder-alerts

Declared tier: outside active vector; #  recipe to execute (from 'harness/finder_alerts.mk', line 319):
Prerequisites: `test-flair-finder-input test-flair-dialog-tracking test-flair-finder-limits test-flair-finder-demo test-flair-finder-refused test-flair-finder-replace test-flair-finder-restore test-flair-finder-diskfull test-flair-finder-launch-alerts test-flair-finder-input-mutant test-flair-dialog-tracking-mutant test-flair-finder-entries-mutant test-flair-finder-windows-mutant test-flair-finder-open-all-mutant test-flair-finder-keys-mutant test-flair-finder-disk-alert-mutant test-flair-finder-launch-alert-mutant test-flair-finder-move-alert-mutant test-flair-finder-restore-mutant test-flair-finder-replace-mutant test-flair-finder-demo-mutant test-flair-dialog-pressed-golden test-flair-dialog-pressed-golden-mutant build/.makefile.stamp`
```make
@printf '>>> test-flair-finder-alerts: green (10 QEMU families incl. the native pressed OK; 13 named mutants RED)\n'
```

## test-flair-finder-cmds

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22407):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_disk_windows_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-finder-cmds : no command is enabled and does nothing (tdnl.36)\n'
@printf '======================================================================\n'
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_FC_FAKES_NAME),$(BUILD)/$(FLAIR_FC_FAKES_NAME)_data.img,$(FLAIR_CMDS_FAKES_SPEC),FLAIR-MENU menu=513,1)
@( $(call fc-fakes-check,$(BUILD)/$(FLAIR_FC_FAKES_NAME).serial) ) || { printf '!!! test-flair-finder-cmds FAIL: FAKES serial (above)\n'; grep -E '^(FINDER-|FLAIR-MENU)' $(BUILD)/$(FLAIR_FC_FAKES_NAME).serial; exit 1; }
@[ -s "$(BUILD)/$(FLAIR_FC_FAKES_NAME).ppm" ] || { printf '!!! test-flair-finder-cmds FAIL: FAKES screendump missing\n'; exit 1; }
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) allsel "$(BUILD)/$(FLAIR_FC_FAKES_NAME).ppm" || { printf '!!! test-flair-finder-cmds FAIL: after Select All the four labels are not all drawn selected\n'; exit 1; }
@printf '>>> test-flair-finder-cmds [1/2]: Get Info / Duplicate / Open(doc) / band-1 About select NOTHING; Ctrl-I/D/O inert; Select All selects 4 (leg allsel)\n'
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_FC_ARRANGE_NAME),$(BUILD)/$(FLAIR_FC_ARRANGE_NAME)_data.img,$(FLAIR_CMDS_ARRANGE_SPEC),FLAIR-MENU menu=514,1)
@# RE-KEYED 2026-10-16 (bead initech-tdnl.51, stated): cell (44,86) since the
@# content starts at L+6, so the set-up drag lands at x=134 (was 129).
$(call fo-has,$(FLAIR_FC_ARRANGE_NAME),FINDER-WIN-DRAG win=0 name=README.TXT x=134 y=186,the set-up drag did not move README.TXT off its cell)
$(call fo-has,$(FLAIR_FC_ARRANGE_NAME),FLAIR-MENU menu=514 item=12 (sel=0x0202000C),the release did not select View > Arrange (by Name))
$(call fo-has,$(FLAIR_FC_ARRANGE_NAME),FINDER-ARRANGE win=0 moved=3,Arrange (by Name) did not move the three out-of-order icons)
@[ -s "$(BUILD)/$(FLAIR_FC_ARRANGE_NAME).ppm" ] || { printf '!!! test-flair-finder-cmds FAIL: ARRANGE screendump missing\n'; exit 1; }
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) arranged "$(BUILD)/$(FLAIR_FC_ARRANGE_NAME).ppm" || { printf '!!! test-flair-finder-cmds FAIL: the icons are not on the grid in NAME order\n'; exit 1; }
@printf '>>> test-flair-finder-cmds [2/2]: View > Arrange (by Name) -- FINDER-ARRANGE moved=3, icons graded in name order (leg arranged)\n'
@printf '>>> test-flair-finder-cmds: green\n'
```

## test-flair-finder-cmds-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22428):
Prerequisites: `build/qemu_harness build/flair_data.img build/flair_tenants_mut_menu_stub_enabled.img build/.makefile.stamp`
```make
$(call fo-boot,$(BUILD)/flair_tenants_mut_menu_stub_enabled.img,flair_cmds_mut_stub,$(BUILD)/flair_cmds_mut_stub_data.img,$(FLAIR_CMDS_FAKES_SPEC),FLAIR-MENU menu=513,0)
@out=$$( $(call fc-fakes-check,$(BUILD)/flair_cmds_mut_stub.serial) ); rc=$$?; \
if [ $$rc -eq 0 ]; then printf '!!! test-flair-finder-cmds-mutant FAIL: STUB_ENABLED PASSED -- decoration\n'; exit 1; fi; \
printf '%s\n' "$$out" | grep -q 'want 3 x menu=512 item=0' || { printf '!!! test-flair-finder-cmds-mutant FAIL: RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
printf '>>> test-flair-finder-cmds-mutant: STUB_ENABLED correctly RED (%s)\n' "$$out"
```

## test-flair-finder-demo

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 104):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_hold.img build/flair_data.img build/ppm_finder_demo_check build/.makefile.stamp`
```make
$(call appl-boot,finder_demo,$(FLAIRTENANTS_HOLD_IMG),$(FLAIR_DATA_IMG),FLAIR_FINDER_DEMO_SPEC,--screendump --screendump-after "FLAIR-MENU-DROP menu=256")
@$(call APPL_ALIVE,$(BUILD)/finder_demo.serial) || { printf '!!! finder_demo did not finish alive\n'; exit 1; }
@$(BUILD)/ppm_finder_demo_check $(BUILD)/finder_demo.ppm ../system7-decomp/goldens/captures/s8_menu_dropped.png
@grep -q '^FLAIR-MENU menu=256 item=0 ' $(BUILD)/finder_demo.serial
@printf '>>> test-flair-finder-demo: green\n'
```

## test-flair-finder-demo-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 304):
Prerequisites: `test-flair-finder-demo build/flair_tenants_mut_finder_demo_hold.img build/.makefile.stamp`
```make
$(call appl-boot,mut_finder_demo,$(BUILD)/flair_tenants_mut_finder_demo_hold.img,$(FLAIR_DATA_IMG),FLAIR_FINDER_DEMO_SPEC,--screendump --screendump-after "FLAIR-MENU-DROP menu=256")
@$(call APPL_ALIVE,$(BUILD)/mut_finder_demo.serial) || { printf '!!! mut_finder_demo did not finish alive\n'; exit 1; }
@if $(BUILD)/ppm_finder_demo_check $(BUILD)/mut_finder_demo.ppm ../system7-decomp/goldens/captures/s8_menu_dropped.png > $(BUILD)/mut_finder_demo.pixels 2>&1; then exit 1;fi
@grep 'FAIL DEMO_GRAY' $(BUILD)/mut_finder_demo.pixels
@printf '>>> test-flair-finder-demo-mutant: FINDER_MUT_DEMO_LIVE RED (native disabled ink missing)\n'
```

## test-flair-finder-disk-alert-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 285):
Prerequisites: `test-flair-finder-diskfull build/flair_tenants_mut_finder_disk_alert_hold.img build/.makefile.stamp`
```make
$(call appl-boot,mut_finder_disk_alert,build/flair_tenants_mut_finder_disk_alert_hold.img,build/finder_diskfull_host.img,FLAIR_FINDER_DISKFULL_SPEC,--screendump )
@$(call APPL_ALIVE,build/mut_finder_disk_alert.serial) || { printf '!!! mut_finder_disk_alert did not finish alive\n'; exit 1; }
@if build/ppm_flair_trash_check notice build/mut_finder_disk_alert.ppm > build/mut_finder_disk_alert.pixels 2>&1; then printf '!!! FINDER_MUT_NO_DISK_ALERT survived\n';exit 1;fi
@grep 'FAIL ALERT-BANDS' build/mut_finder_disk_alert.pixels
@printf '>>> test-flair-finder-disk-alert-mutant: FINDER_MUT_NO_DISK_ALERT RED (notice absent from framebuffer)\n'
```

## test-flair-finder-diskfull

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 157):
Prerequisites: `test-finder-diskfull build/qemu_harness build/flair_tenants_mut_hold.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
$(call appl-boot,finder_diskfull,$(FLAIRTENANTS_HOLD_IMG),$(BUILD)/finder_diskfull_host.img,FLAIR_FINDER_DISKFULL_SPEC,--screendump --screendump-after "FINDER-ALERT reason=disk-full")
@$(call APPL_ALIVE,$(BUILD)/finder_diskfull.serial) || { printf '!!! finder_diskfull did not finish alive\n'; exit 1; }
@$(PPM_FLAIR_TRASH_CHECK_BIN) notice $(BUILD)/finder_diskfull.ppm
@! mdir -a -i $(BUILD)/finder_diskfull_data.img ::DESKTOP.DB >/dev/null 2>&1
@printf '>>> test-flair-finder-diskfull: green (alert, no phantom DB)\n'
```

## test-flair-finder-entries-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 271):
Prerequisites: `test-flair-finder-limits build/flair_tenants_mut_finder_entries_hold.img build/.makefile.stamp`
```make
$(call appl-boot,mut_finder_entries,build/flair_tenants_mut_finder_entries_hold.img,build/finder_limits_fixture.img,FLAIR_FINDER_LIMITS_SPEC,--screendump)
@$(call APPL_ALIVE,build/mut_finder_entries.serial) || { printf '!!! mut_finder_entries did not finish alive\n'; exit 1; }
@if grep -qx 'FINDER-OPEN-VOLUME win=0 n=73' build/mut_finder_entries.serial; then printf '!!! FINDER_MUT_ICON64 survived\n'; exit 1; fi
@printf '>>> test-flair-finder-entries-mutant: FINDER_MUT_ICON64 RED (73-entry listing absent)\n'
```

## test-flair-finder-follow

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22209):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_disk_windows_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-finder-follow : Finder window contents FOLLOW the window (tdnl.34)\n'
@printf '  Ref: audit 2026-10-03 F01; os/flair/finder_windows.h Sec 10b; spec/flair_finder_follow_traces.mk.\n'
@printf '======================================================================\n'
@command -v mdir >/dev/null 2>&1 && command -v mtype >/dev/null 2>&1 || { printf '!!! test-flair-finder-follow FAIL: mtools (mdir/mtype) missing\n'; exit 1; }
@# ---- [1] DRAG ----
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_FF_DRAG_NAME),$(BUILD)/$(FLAIR_FF_DRAG_NAME)_data.img,$(FLAIR_FOLLOW_DRAG_SPEC),FLAIR-DRAG win 0 (20$(fo_comma)60)->(120$(fo_comma)180),1)
$(call fo-has,$(FLAIR_FF_DRAG_NAME),FINDER-OPEN-VOLUME win=0 n=4,the volume double-click did not open the root window)
$(call fo-has,$(FLAIR_FF_DRAG_NAME),FLAIR-DRAG win 0 (20$(fo_comma)60)->(120$(fo_comma)180),the root window did not move by the locked (+100$(fo_comma)+120))
@[ -s "$(BUILD)/$(FLAIR_FF_DRAG_NAME).ppm" ] || { printf '!!! test-flair-finder-follow FAIL: DRAG screendump missing\n'; exit 1; }
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) movedwin "$(BUILD)/$(FLAIR_FF_DRAG_NAME).ppm" \
	|| { printf '!!! test-flair-finder-follow FAIL: after the DRAG the icons are not on the grid of the moved content (audit F01)\n'; exit 1; }
@printf '>>> test-flair-finder-follow [1/4]: DRAG (20,60)->(120,180) -- two icons graded at the moved grid, cells 2-3 bare (leg movedwin)\n'
@# ---- [2] collapse, expand, ZOOM ----
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_FF_ZOOM_NAME),$(BUILD)/$(FLAIR_FF_ZOOM_NAME)_data.img,$(FLAIR_FOLLOW_ZOOM_SPEC),FLAIR-ZOOM win 0 in,1)
$(call fo-has,$(FLAIR_FF_ZOOM_NAME),FLAIR-COLLAPSE win 0 1,the collapse box of the moved window did not collapse it)
$(call fo-has,$(FLAIR_FF_ZOOM_NAME),FLAIR-COLLAPSE win 0 0,the second collapse-box click did not expand it)
$(call fo-has,$(FLAIR_FF_ZOOM_NAME),FLAIR-ZOOM win 0 in,the zoom box of the moved window did not zoom it)
@[ -s "$(BUILD)/$(FLAIR_FF_ZOOM_NAME).ppm" ] || { printf '!!! test-flair-finder-follow FAIL: ZOOM screendump missing\n'; exit 1; }
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) zoomedwin "$(BUILD)/$(FLAIR_FF_ZOOM_NAME).ppm" \
	|| { printf '!!! test-flair-finder-follow FAIL: after collapse/expand/ZOOM the icons are not on the grid of the zoomed content\n'; exit 1; }
@printf '>>> test-flair-finder-follow [2/4]: COLLAPSE 1/0 + ZOOM in -- two icons graded at the zoomed grid, cells 2-3 bare (leg zoomedwin)\n'
@# ---- [3] RESTORE ----
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_FF_RESTORE_NAME),$(BUILD)/$(FLAIR_FF_RESTORE_NAME)_data.img,$(FLAIR_FOLLOW_RESTORE_SPEC),FLAIR-ZOOM win 0 out,1)
$(call fo-has,$(FLAIR_FF_RESTORE_NAME),FLAIR-ZOOM win 0 out,the zoom box of the ZOOMED window did not restore it)
@[ -s "$(BUILD)/$(FLAIR_FF_RESTORE_NAME).ppm" ] || { printf '!!! test-flair-finder-follow FAIL: RESTORE screendump missing\n'; exit 1; }
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) movedwin "$(BUILD)/$(FLAIR_FF_RESTORE_NAME).ppm" \
	|| { printf '!!! test-flair-finder-follow FAIL: after RESTORE the icons are not back on the moved grid\n'; exit 1; }
@printf '>>> test-flair-finder-follow [3/4]: ZOOM out -- the restored window graded again (leg movedwin)\n'
@# ---- [4] SELECT where drawn + MOVE into a folder of the moved window ----
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_FF_MOVE_NAME),$(BUILD)/$(FLAIR_FF_MOVE_NAME)_data.img,$(FLAIR_FOLLOW_MOVE_SPEC),FINDER-MOVE name=README.TXT from=0 to=3,0)
$(call fo-has,$(FLAIR_FF_MOVE_NAME),FINDER-WIN-SELECT win=0 name=APPS count=1,a click on APPS at its NEW centre (223$(fo_comma)222) did not select it)
$(call fo-has,$(FLAIR_FF_MOVE_NAME),FINDER-DROP-HILITE name=APPS,APPS was not lit under README.TXT dragged in the moved window)
$(call fo-has,$(FLAIR_FF_MOVE_NAME),FINDER-MOVE name=README.TXT from=0 to=3,README.TXT dropped on the moved APPS did not move into APPS)
@mdir -i $(BUILD)/$(FLAIR_FF_MOVE_NAME)_data.img ::/APPS | grep -q '^README   TXT ' || { printf '!!! test-flair-finder-follow FAIL: mtools does not see README.TXT in ::/APPS\n'; exit 1; }
@mtype -i $(BUILD)/$(FLAIR_FF_MOVE_NAME)_data.img ::/APPS/README.TXT | cmp -s - $(FLAIR_DATA_README) || { printf '!!! test-flair-finder-follow FAIL: ::/APPS/README.TXT bytes differ from the fixture\n'; exit 1; }
@! mdir -i $(BUILD)/$(FLAIR_FF_MOVE_NAME)_data.img :: | grep -q '^README   TXT ' || { printf '!!! test-flair-finder-follow FAIL: README.TXT is STILL in the root\n'; exit 1; }
@printf '>>> test-flair-finder-follow [4/4]: SELECT APPS where drawn + FINDER-MOVE README.TXT 0->3; mtools sees it in ::/APPS (bytes intact)\n'
@printf '>>> test-flair-finder-follow: green\n'
```

## test-flair-finder-follow-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22254):
Prerequisites: `build/qemu_harness build/flair_data.img build/ppm_flair_disk_windows_check build/flair_tenants_mut_win_abs_coords.img build/.makefile.stamp`
```make
$(call fo-boot,$(BUILD)/flair_tenants_mut_win_abs_coords.img,flair_follow_mut_abs,$(BUILD)/flair_follow_mut_abs_data.img,$(FLAIR_FOLLOW_DRAG_SPEC),FLAIR-DRAG win 0 (20$(fo_comma)60)->(120$(fo_comma)180),1)
@grep -qxF 'FLAIR-DRAG win 0 (20,60)->(120,180)' $(BUILD)/flair_follow_mut_abs.serial || { printf '!!! test-flair-finder-follow-mutant FAIL: ABS_COORDS boot never reached the drag\n'; exit 1; }
@if $(PPM_FLAIR_DISKWIN_CHECK_BIN) movedwin $(BUILD)/flair_follow_mut_abs.ppm > $(BUILD)/flair_follow_mut_abs.grade 2>&1; then printf '!!! test-flair-finder-follow-mutant FAIL: ABS_COORDS PASSED the moved-window grader -- decoration\n'; exit 1; fi
@# RE-KEYED 2026-10-16 (bead initech-tdnl.51, stated): README.TXT's cell is
@# (44,86) since the content starts at L+6, so the moved cell is (144,206).
@grep -q 'README.TXT DOC UNSEL sprite @(144,206)' $(BUILD)/flair_follow_mut_abs.grade || { printf '!!! test-flair-finder-follow-mutant FAIL: ABS_COORDS went RED for the wrong reason\n'; cat $(BUILD)/flair_follow_mut_abs.grade; exit 1; }
@printf '>>> test-flair-finder-follow-mutant: ABS_COORDS correctly RED (README.TXT is not drawn at the moved cell (144,206))\n'
@printf '>>> test-flair-finder-follow-mutant: green\n'
```

## test-flair-finder-icons

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 21885):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_finder_icons_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-finder-icons : every Finder icon kind vs the Mac OS 8.1 captures\n'
@printf '  Ref: beads initech-tdnl.53; ../system7-decomp/re/mint-results-008.md; spec/flair_finder_icons_traces.mk.\n'
@printf '======================================================================\n'
$(call icons-scene,$(FLAIRTENANTS_IMG),flair_finder_icons)
@$(PPM_FLAIR_ICONS_CHECK_BIN) "$(ICON_CAPTURES_DIR)" "$(BUILD)/flair_finder_icons.ppm" $(FLAIR_ICONS_SCENE_CELLS) \
	|| { printf '!!! test-flair-finder-icons FAIL: a booted icon cell differs from the mint-008 capture\n'; exit 1; }
@printf '>>> test-flair-finder-icons: green (7 cells: VOLUME sel, TRASH, DOC sel, FOLDER, 2 x APP, DOC -- 0/1024 differing each)\n'
```

## test-flair-finder-icons-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 223):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-finder-icons-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-finder-icons-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-finder-icons_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-finder-icons_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-finder-icons_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-finder-icons_bochs.report.txt" || true
@cat "$(BUILD)/flair-finder-icons_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-finder-icons-bochs "$(BUILD)/flair-finder-icons_bochs.report.txt" "$(BUILD)/flair-finder-icons_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-finder-icons-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 21895):
Prerequisites: `build/qemu_harness build/flair_data.img build/ppm_flair_finder_icons_check build/flair_tenants_mut_icon_row_off1.img build/flair_tenants_mut_icon_no_select.img build/.makefile.stamp`
```make
$(call icons-scene,$(BUILD)/flair_tenants_mut_icon_row_off1.img,flair_finder_icons_mut_row)
@rc=0; $(PPM_FLAIR_ICONS_CHECK_BIN) "$(ICON_CAPTURES_DIR)" "$(BUILD)/flair_finder_icons_mut_row.ppm" $(FLAIR_ICONS_SCENE_CELLS) > $(BUILD)/flair_finder_icons_mut_row.grade 2>&1 || rc=$$?; \
[ "$$rc" = 1 ] && grep -q 'FAIL: 7 icon cell(s) differ' $(BUILD)/flair_finder_icons_mut_row.grade \
	|| { cat $(BUILD)/flair_finder_icons_mut_row.grade; printf '!!! test-flair-finder-icons-mutant FAIL: DESK_ICON_MUT_ROW_OFF1 did not turn all 7 cells RED\n'; exit 1; }
@printf '>>> DESK_ICON_MUT_ROW_OFF1: RED -- all 7 booted cells miss the captures\n'
$(call icons-scene,$(BUILD)/flair_tenants_mut_icon_no_select.img,flair_finder_icons_mut_nosel)
@rc=0; $(PPM_FLAIR_ICONS_CHECK_BIN) "$(ICON_CAPTURES_DIR)" "$(BUILD)/flair_finder_icons_mut_nosel.ppm" $(FLAIR_ICONS_SCENE_CELLS) > $(BUILD)/flair_finder_icons_mut_nosel.grade 2>&1 || rc=$$?; \
[ "$$rc" = 1 ] && grep -q 'FAIL: 2 icon cell(s) differ' $(BUILD)/flair_finder_icons_mut_nosel.grade && \
[ "$$(grep -c 'SEL at .*: [1-9][0-9]*/1024' $(BUILD)/flair_finder_icons_mut_nosel.grade)" = 2 ] && \
! grep -q 'UNSEL at .*: [1-9][0-9]*/1024' $(BUILD)/flair_finder_icons_mut_nosel.grade \
	|| { cat $(BUILD)/flair_finder_icons_mut_nosel.grade; printf '!!! test-flair-finder-icons-mutant FAIL: DESK_ICON_MUT_NO_SELECT did not turn exactly the 2 SELECTED cells RED\n'; exit 1; }
@printf '>>> DESK_ICON_MUT_NO_SELECT: RED -- exactly the 2 selected cells (VOLUME, README) miss the captures\n'
```

## test-flair-finder-input

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 72):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_hold.img build/flair_data.img build/ppm_flair_app_launch_check build/.makefile.stamp`
```make
$(call appl-boot,finder_activate,$(FLAIRTENANTS_HOLD_IMG),$(FLAIR_DATA_IMG),FLAIR_ACTIVATE_ONLY_SPEC,--screendump --screendump-after "FLAIR-LIVE-OK")
@$(call APPL_ALIVE,$(BUILD)/finder_activate.serial) || { printf '!!! finder_activate did not finish alive\n'; exit 1; }
@! grep -q '^TENANT-EXIT' $(BUILD)/finder_activate.serial || { printf '!!! ACTIVATE_ONLY content click quit the inactive tenant\n'; exit 1; }
@$(PPM_FLAIR_APPL_CHECK_BIN) bar-tenant $(BUILD)/finder_activate.ppm
@printf '>>> test-flair-finder-input: green\n'
```

## test-flair-finder-input-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 251):
Prerequisites: `test-flair-finder-input build/flair_tenants_mut_finder_click_hold.img build/.makefile.stamp`
```make
$(call appl-boot,finder_click_mut,$(BUILD)/flair_tenants_mut_finder_click_hold.img,$(FLAIR_DATA_IMG),FLAIR_ACTIVATE_ONLY_SPEC,--screendump)
@$(call APPL_ALIVE,$(BUILD)/finder_click_mut.serial) || { printf '!!! finder_click_mut did not finish alive\n'; exit 1; }
@grep -q '^TENANT-EXIT' $(BUILD)/finder_click_mut.serial
@printf '>>> test-flair-finder-input-mutant: PROC_MUT_CLICK_THROUGH RED (activating click exits tenant)\n'
```

## test-flair-finder-keys-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 274):
Prerequisites: `test-flair-finder-limits build/flair_tenants_mut_finder_keys_hold.img build/.makefile.stamp`
```make
$(call appl-boot,mut_finder_keys,build/flair_tenants_mut_finder_keys_hold.img,build/finder_limits_fixture.img,FLAIR_FINDER_LIMITS_SPEC,--screendump)
@$(call APPL_ALIVE,build/mut_finder_keys.serial) || { printf '!!! mut_finder_keys did not finish alive\n'; exit 1; }
@if grep -q '^FINDER-OPEN-FOLDER name=F68 ' build/mut_finder_keys.serial; then printf '!!! FINDER_MUT_NO_KEYS survived\n'; exit 1; fi
@printf '>>> test-flair-finder-keys-mutant: FINDER_MUT_NO_KEYS RED (F68 not opened)\n'
```

## test-flair-finder-launch-alert-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 286):
Prerequisites: `test-flair-finder-launch-alerts build/flair_tenants_mut_finder_launch_alert_hold.img build/.makefile.stamp`
```make
$(call appl-boot,mut_finder_launch_alert,build/flair_tenants_mut_finder_launch_alert_hold.img,build/flair_data_tfx_noreg.img,FLAIR_APP_LAUNCH_SPEC,--screendump )
@$(call APPL_ALIVE,build/mut_finder_launch_alert.serial) || { printf '!!! mut_finder_launch_alert did not finish alive\n'; exit 1; }
@if build/ppm_flair_trash_check notice build/mut_finder_launch_alert.ppm > build/mut_finder_launch_alert.pixels 2>&1; then printf '!!! FINDER_MUT_NO_LAUNCH_ALERT survived\n';exit 1;fi
@grep 'FAIL ALERT-BANDS' build/mut_finder_launch_alert.pixels
@printf '>>> test-flair-finder-launch-alert-mutant: FINDER_MUT_NO_LAUNCH_ALERT RED (notice absent from framebuffer)\n'
```

## test-flair-finder-launch-alerts

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 163):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_hold.img build/flair_data_tfx_noreg.img build/flair_data_tfx_crash.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
$(call appl-boot,finder_noreg,$(FLAIRTENANTS_HOLD_IMG),$(FLAIR_DATA_TFX_NOREG_IMG),FLAIR_APP_LAUNCH_SPEC,--screendump --screendump-after "FINDER-ALERT reason=launch")
@$(call APPL_ALIVE,$(BUILD)/finder_noreg.serial) || { printf '!!! finder_noreg did not finish alive\n'; exit 1; }
@$(PPM_FLAIR_TRASH_CHECK_BIN) notice $(BUILD)/finder_noreg.ppm
$(call appl-boot,finder_crash,$(FLAIRTENANTS_HOLD_IMG),$(FLAIR_DATA_TFX_CRASH_IMG),FLAIR_APP_LAUNCH_SPEC,--screendump --screendump-after "FINDER-ALERT reason=crash")
@$(call APPL_ALIVE,$(BUILD)/finder_crash.serial) || { printf '!!! finder_crash did not finish alive\n'; exit 1; }
@$(PPM_FLAIR_TRASH_CHECK_BIN) notice $(BUILD)/finder_crash.ppm
@printf '>>> test-flair-finder-launch-alerts: green (unregistered and crashed alerts)\n'
```

## test-flair-finder-limits

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 91):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_hold.img build/finder_limits_fixture.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
$(call appl-boot,finder_limits,$(FLAIRTENANTS_HOLD_IMG),$(FINDER_LIMITS_DATA),FLAIR_FINDER_LIMITS_SPEC,--screendump --screendump-after "FINDER-ALERT reason=windows")
@$(call APPL_ALIVE,$(BUILD)/finder_limits.serial) || { printf '!!! finder_limits did not finish alive\n'; exit 1; }
@grep -qx 'FINDER-OPEN-VOLUME win=0 n=73' $(BUILD)/finder_limits.serial
@grep -q '^FINDER-OPEN-FOLDER name=APPS ' $(BUILD)/finder_limits.serial
@grep -q '^FINDER-OPEN-FOLDER name=F68 ' $(BUILD)/finder_limits.serial
@grep -q '^FINDER-ALERT reason=windows' $(BUILD)/finder_limits.serial
@awk '/^FINDER-OPEN-COUNT n=/{split($$2,a,"=");if(a[2]>4)ok=1}END{exit !ok}' $(BUILD)/finder_limits.serial
@$(PPM_FLAIR_TRASH_CHECK_BIN) notice $(BUILD)/finder_limits.ppm
@printf '>>> test-flair-finder-limits: green\n'
```

## test-flair-finder-move-alert-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 289):
Prerequisites: `test-flair-finder-refused build/flair_tenants_mut_finder_move_alert_hold.img build/.makefile.stamp`
```make
$(call appl-boot,mut_finder_move_alert,build/flair_tenants_mut_finder_move_alert_hold.img,build/flair_data.img,FLAIR_FINDER_REFUSED_SPEC,--screendump --screendump-after "FINDER-MOVE-REFUSED reason=cycle")
@$(call APPL_ALIVE,build/mut_finder_move_alert.serial) || { printf '!!! mut_finder_move_alert did not finish alive\n'; exit 1; }
@if build/ppm_flair_trash_check notice build/mut_finder_move_alert.ppm > build/mut_finder_move_alert.pixels 2>&1; then printf '!!! FINDER_MUT_NO_MOVE_ALERT survived\n';exit 1;fi
@grep 'FAIL ALERT-BANDS' build/mut_finder_move_alert.pixels
@printf '>>> test-flair-finder-move-alert-mutant: FINDER_MUT_NO_MOVE_ALERT RED (notice absent from framebuffer)\n'
```

## test-flair-finder-open

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23729):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/flair_tenants_mut_hold.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-finder-open : File > Open / Ctrl-O launch a selected application like a double-click\n'
@printf '  bead initech-fbo4; audit 2026-10-23 desktop L004; Users Guide V6.0 PDF p. 38, 45\n'
@printf '======================================================================\n'
$(call appl-boot,flair_fopen_key,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_FINDER_OPEN_KEY_SPEC,)
@f=$(BUILD)/flair_fopen_key.serial; $(call APPL_ALIVE,$$f) && $(call FOPEN_VIA_CMD,$$f,key) && $(call FOPEN_LAUNCHED,$$f) || { printf '!!! test-flair-finder-open [1/4] FAIL: KEY -- Ctrl-O on the selected CTENANT.EXE must run File > Open once (src=key), launch it once (one REGISTER, one DISPATCH), and Ctrl-Q must quit it back to the Finder\n'; $(call FOPEN_GREP,$$f); exit 1; }
@printf '>>> test-flair-finder-open [1/4] KEY: %s -> %s -> one REGISTER, one DISPATCH, Ctrl-Q quit, the Finder back\n' "$$(grep -m1 '^FINDER-CMD id=3' $(BUILD)/flair_fopen_key.serial)" "$$(grep -m1 '^FINDER-OPEN-APP' $(BUILD)/flair_fopen_key.serial)"
$(call appl-boot,flair_fopen_menu,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_FINDER_OPEN_MENU_SPEC,)
@f=$(BUILD)/flair_fopen_menu.serial; $(call APPL_ALIVE,$$f) && $(call FOPEN_VIA_CMD,$$f,mouse) && $(call FOPEN_LAUNCHED,$$f) || { printf '!!! test-flair-finder-open [2/4] FAIL: MENU -- File > Open by the mouse on the selected CTENANT.EXE must run the command once (src=mouse), launch it once, and Ctrl-Q must quit it back to the Finder\n'; $(call FOPEN_GREP,$$f); exit 1; }
@printf '>>> test-flair-finder-open [2/4] MENU: %s -> the SAME launch, quit and return as the chord\n' "$$(grep -m1 '^FINDER-CMD id=3' $(BUILD)/flair_fopen_menu.serial)"
$(call appl-boot,flair_fopen_dbl,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_FINDER_OPEN_DBL_SPEC,)
@f=$(BUILD)/flair_fopen_dbl.serial; $(call APPL_ALIVE,$$f) && $(call FOPEN_NO_CMD,$$f) && $(call FOPEN_LAUNCHED,$$f) || { printf '!!! test-flair-finder-open [3/4] FAIL: DBL -- the double-click route must launch CTENANT.EXE once with NO File > Open command, and Ctrl-Q must quit it back to the Finder\n'; $(call FOPEN_GREP,$$f); exit 1; }
@printf '>>> test-flair-finder-open [3/4] DBL: the double-click launches the same way (no command line), one REGISTER, one DISPATCH, quit, return\n'
$(call cten-hold-boot,flair_fopen_busy,$(FLAIR_DATA_IMG),FLAIR_FINDER_OPEN_BUSY_SPEC)
@f=$(BUILD)/flair_fopen_busy.serial; $(call APPL_ALIVE,$$f) && $(call FOPEN_BUSY,$$f) || { printf '!!! test-flair-finder-open [4/4] FAIL: BUSY -- with CTENANT resident, Ctrl-O on it must be refused out loud (one TENANT-SLOT-BUSY + FINDER-ALERT reason=launch) with the resident tenant undisturbed and still quit cleanly\n'; $(call FOPEN_GREP,$$f); exit 1; }
@printf '>>> test-flair-finder-open [4/4] BUSY: %s, %s; the resident tenant quit cleanly\n' "$$(grep -m1 '^TENANT-SLOT-BUSY' $(BUILD)/flair_fopen_busy.serial)" "$$(grep -m1 '^FINDER-ALERT reason=launch' $(BUILD)/flair_fopen_busy.serial)"
@printf '>>> test-flair-finder-open: green\n'
```

## test-flair-finder-open-all-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 273):
Prerequisites: `test-flair-finder-limits build/flair_tenants_mut_finder_open_one_hold.img build/.makefile.stamp`
```make
$(call appl-boot,mut_finder_open_one,build/flair_tenants_mut_finder_open_one_hold.img,build/finder_limits_fixture.img,FLAIR_FINDER_LIMITS_SPEC,--screendump)
@$(call APPL_ALIVE,build/mut_finder_open_one.serial) || { printf '!!! mut_finder_open_one did not finish alive\n'; exit 1; }
@if grep -q '^FINDER-ALERT reason=windows' build/mut_finder_open_one.serial; then printf '!!! FINDER_MUT_OPEN_ONE survived\n'; exit 1; fi
@printf '>>> test-flair-finder-open-all-mutant: FINDER_MUT_OPEN_ONE RED (only one folder opened)\n'
```

## test-flair-finder-open-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23748):
Prerequisites: `test-flair-finder-open build/qemu_harness build/flair_data.img build/flair_tenants_mut_open_app_dark.img build/flair_tenants_mut_open_no_launch.img build/.makefile.stamp`
```make
$(call appl-boot,flair_fopenmut_dark,$(BUILD)/flair_tenants_mut_open_app_dark.img,$(FLAIR_DATA_IMG),FLAIR_FINDER_OPEN_KEY_SPEC,)
@f=$(BUILD)/flair_fopenmut_dark.serial; if $(call FOPEN_VIA_CMD,$$f,key) && $(call FOPEN_LAUNCHED,$$f); then printf '!!! test-flair-finder-open-mutant FAIL: OPEN_APP_DARK PASSED the KEY leg -- the enablement oracle is decoration\n'; exit 1; fi
@f=$(BUILD)/flair_fopenmut_dark.serial; $(call APPL_ALIVE,$$f) && grep -qx 'FINDER-CMD id=4 name=CLOSE_WINDOW src=key sel=1' $$f && $(call FOPEN_NO_CMD,$$f) && ! grep -q '^TENANT-REGISTER' $$f || { printf '!!! test-flair-finder-open-mutant FAIL: OPEN_APP_DARK went RED for the wrong reason (want: Ctrl-O grayed -> no FINDER-CMD id=3, nothing launched, desktop alive; the Finder still foreground with CTENANT.EXE selected, so Ctrl-W closes APPS with sel=1)\n'; $(call FOPEN_GREP,$$f); exit 1; }
@printf '>>> test-flair-finder-open-mutant: OPEN_APP_DARK RED for the named reason -- CTENANT.EXE selected, Ctrl-O ran no command and launched nothing\n'
$(call appl-boot,flair_fopenmut_nolaunch,$(BUILD)/flair_tenants_mut_open_no_launch.img,$(FLAIR_DATA_IMG),FLAIR_FINDER_OPEN_KEY_SPEC,)
@f=$(BUILD)/flair_fopenmut_nolaunch.serial; if $(call FOPEN_VIA_CMD,$$f,key) && $(call FOPEN_LAUNCHED,$$f); then printf '!!! test-flair-finder-open-mutant FAIL: OPEN_NO_LAUNCH PASSED the KEY leg -- the launch oracle is decoration\n'; exit 1; fi
@f=$(BUILD)/flair_fopenmut_nolaunch.serial; $(call APPL_ALIVE,$$f) && grep -qx 'FINDER-CMD id=3 name=OPEN src=key sel=1' $$f && grep -q '^FINDER-OPEN-APP name=CTENANT.EXE dir=' $$f && ! grep -q '^TENANT-REGISTER' $$f || { printf '!!! test-flair-finder-open-mutant FAIL: OPEN_NO_LAUNCH went RED for the wrong reason (want: the command ran and named the launch, but no TENANT-REGISTER followed)\n'; $(call FOPEN_GREP,$$f); exit 1; }
@printf '>>> test-flair-finder-open-mutant: OPEN_NO_LAUNCH RED for the named reason -- File > Open ran and named CTENANT.EXE, the adapter launched nothing\n'
@printf '>>> test-flair-finder-open-mutant: green (both mutants RED for their named reasons)\n'
```

## test-flair-finder-refused

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 131):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_hold.img build/flair_data.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
$(call appl-boot,finder_refused,$(FLAIRTENANTS_HOLD_IMG),$(FLAIR_DATA_IMG),FLAIR_FINDER_REFUSED_SPEC,--screendump --screendump-after "FINDER-ALERT-LINE A folder cannot be moved inside itself.")
@grep -q '^FINDER-ALERT-LINE The item is already in this folder.' $(BUILD)/finder_refused.serial
@$(call APPL_ALIVE,$(BUILD)/finder_refused.serial) || { printf '!!! finder_refused did not finish alive\n'; exit 1; }
@grep -q '^FINDER-ALERT reason=move' $(BUILD)/finder_refused.serial
@$(PPM_FLAIR_TRASH_CHECK_BIN) notice $(BUILD)/finder_refused.ppm
@printf '>>> test-flair-finder-refused: green\n'
```

## test-flair-finder-replace

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 138):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_hold.img build/finder_replace_fixture.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
$(call appl-boot,finder_replace,$(FLAIRTENANTS_HOLD_IMG),$(FINDER_REPLACE_DATA),FLAIR_FINDER_REPLACE_SPEC,--screendump --screendump-after "FINDER-ALERT reason=replace")
@$(call APPL_ALIVE,$(BUILD)/finder_replace.serial) || { printf '!!! finder_replace did not finish alive\n'; exit 1; }
@$(PPM_FLAIR_TRASH_CHECK_BIN) chrome $(BUILD)/finder_replace.ppm
$(call appl-boot,finder_replace_ok,$(FLAIRTENANTS_HOLD_IMG),$(FINDER_REPLACE_DATA),FLAIR_FINDER_REPLACE_OK_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/finder_replace_ok.serial) || { printf '!!! finder_replace_ok did not finish alive\n'; exit 1; }
@mcopy -o -i $(BUILD)/finder_replace_ok_data.img ::APPS/README.TXT $(BUILD)/finder_replaced.txt
@cmp $(BUILD)/finder_original.txt $(BUILD)/finder_replaced.txt
@! mdir -i $(BUILD)/finder_replace_ok_data.img ::README.TXT >/dev/null 2>&1
@printf '>>> test-flair-finder-replace: green (Replace alert; mtools incoming bytes match)\n'
```

## test-flair-finder-replace-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 298):
Prerequisites: `test-flair-finder-replace build/flair_tenants_mut_finder_replace_hold.img build/.makefile.stamp`
```make
$(call appl-boot,mut_finder_replace,$(BUILD)/flair_tenants_mut_finder_replace_hold.img,$(FINDER_REPLACE_DATA),FLAIR_FINDER_REPLACE_OK_SPEC,--screendump)
@$(call APPL_ALIVE,$(BUILD)/mut_finder_replace.serial) || { printf '!!! mut_finder_replace did not finish alive\n'; exit 1; }
@grep -q '^FINDER-ALERT reason=replace' $(BUILD)/mut_finder_replace.serial
@mdir -i $(BUILD)/mut_finder_replace_data.img ::README.TXT >/dev/null 2>&1
@printf '>>> test-flair-finder-replace-mutant: FINDER_MUT_REPLACE_CANCEL RED (confirmed source never moved)\n'
```

## test-flair-finder-restore

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 148):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_hold.img build/finder_restore_fixture.img build/ppm_flair_disk_windows_check build/.makefile.stamp`
```make
$(call appl-boot,finder_restore,$(FLAIRTENANTS_HOLD_IMG),$(FINDER_RESTORE_DATA),FLAIR_FINDER_RESTORE_SPEC,--screendump --screendump-after "FINDER-PUT-AWAY n=1")
@$(call APPL_ALIVE,$(BUILD)/finder_restore.serial) || { printf '!!! finder_restore did not finish alive\n'; exit 1; }
@grep -qx 'FINDER-PUT-AWAY n=1' $(BUILD)/finder_restore.serial
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) trashwin $(BUILD)/finder_restore.ppm
@mcopy -o -i $(BUILD)/finder_restore_data.img ::README.TXT $(BUILD)/finder_restored.txt
@cmp $(BUILD)/finder_original.txt $(BUILD)/finder_restored.txt
@! mdir -i $(BUILD)/finder_restore_data.img ::TRASH/READM001.TXT >/dev/null 2>&1
@printf '>>> test-flair-finder-restore: green (original name/place; mtools byte-identical)\n'
```

## test-flair-finder-restore-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 291):
Prerequisites: `test-flair-finder-restore build/flair_tenants_mut_finder_origin_hold.img build/.makefile.stamp`
```make
$(call appl-boot,mut_finder_origin,$(BUILD)/flair_tenants_mut_finder_origin_hold.img,$(FINDER_RESTORE_DATA),FLAIR_FINDER_RESTORE_SPEC,--screendump)
@$(call APPL_ALIVE,$(BUILD)/mut_finder_origin.serial) || { printf '!!! mut_finder_origin did not finish alive\n'; exit 1; }
@grep -q '^FINDER-PUT-AWAY n=0' $(BUILD)/mut_finder_origin.serial
@grep -q '^TRASH-RENAME from=README.TXT to=READM001.TXT' $(BUILD)/mut_finder_origin.serial
@! mdir -i $(BUILD)/mut_finder_origin_data.img ::README.TXT >/dev/null 2>&1
@printf '>>> test-flair-finder-restore-mutant: FINDER_MUT_ORIGINAL_LOST RED (original name not restored)\n'
```

## test-flair-finder-service

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 21508):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/flair_damaged_data.img build/ppm_flair_disk_windows_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-finder-service : TRASH + DESKTOP.DB hidden and safe\n'
@printf '  beads initech-tdnl.56/.73/.75; traces FLAIR_SVC_* (spec/flair_file_ops_traces.mk)\n'
@printf '======================================================================\n'
@command -v mdir >/dev/null 2>&1 && command -v mtype >/dev/null 2>&1 || { printf '!!! test-flair-finder-service FAIL: mtools missing\n'; exit 1; }
@# ---- leg 1a: open the volume; PIXEL-grade the root window ----
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(BUILD)/$(FLAIR_SVC_NAME)_open_data.img
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(BUILD)/$(FLAIR_SVC_NAME)_open_data.img,$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(BUILD)/$(FLAIR_SVC_NAME)_open_data.img" --expect FLAIR-FAT-MOUNT-OK \
	--name "$(FLAIR_SVC_NAME)_open" --out "$(BUILD)" --mouse "$(FLAIR_ICON_OPEN_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FINDER-OPEN-VOLUME win=0 n=4" \
	--timeout-ms 20000 2> "$(BUILD)/$(FLAIR_SVC_NAME)_open.report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_SVC_NAME)_open.report"; then printf '!!! test-flair-finder-service FAIL: TRIPLE FAULT\n'; exit 1; fi
@grep -qxF 'FINDER-OPEN-VOLUME win=0 n=4' "$(BUILD)/$(FLAIR_SVC_NAME)_open.serial" || { printf '!!! test-flair-finder-service FAIL: leg 1a -- want FINDER-OPEN-VOLUME win=0 n=4 (README.TXT + APPS + DOS + AUTOEXEC.BAT)\n'; grep -E '^FINDER' "$(BUILD)/$(FLAIR_SVC_NAME)_open.serial"; exit 1; }
@[ -s "$(BUILD)/$(FLAIR_SVC_NAME)_open.ppm" ] || { printf '!!! test-flair-finder-service FAIL: no screendump at FINDER-OPEN-VOLUME win=0 n=4\n'; exit 1; }
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) rootwin "$(BUILD)/$(FLAIR_SVC_NAME)_open.ppm" || { printf '!!! test-flair-finder-service FAIL: the root window is not README/APPS/DOS/AUTOEXEC with service cells 4-5 bare\n'; exit 1; }
@# ---- leg 1b: a file dragged to the Trash is staged; the DB is written ----
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(BUILD)/$(FLAIR_SVC_NAME)_data.img
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(BUILD)/$(FLAIR_SVC_NAME)_data.img,$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(BUILD)/$(FLAIR_SVC_NAME)_data.img" --expect FLAIR-FAT-MOUNT-OK \
	--name "$(FLAIR_SVC_NAME)" --out "$(BUILD)" --mouse "$(FLAIR_SVC_TRASH_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--quit-after "DESKTOP-DB-SAVE n=3" --timeout-ms 40000 2> "$(BUILD)/$(FLAIR_SVC_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_SVC_NAME).report"; then printf '!!! test-flair-finder-service FAIL: TRIPLE FAULT\n'; exit 1; fi
@$(call svc-leg1-check,$(FLAIR_SVC_NAME)) || { printf '!!! test-flair-finder-service FAIL: leg 1b -- want FINDER-OPEN-VOLUME win=0 n=4, FINDER-TRASH name=README.TXT origin=0, DESKTOP-DB-SAVE n=3\n'; grep -E '^(FINDER|DESKTOP|TRASH)' "$(BUILD)/$(FLAIR_SVC_NAME).serial"; exit 1; }
@mdir -a -i $(BUILD)/$(FLAIR_SVC_NAME)_data.img :: > $(BUILD)/$(FLAIR_SVC_NAME).root.lst
@sh harness/verify_fixture_volume.sh harness $(BUILD)/$(FLAIR_SVC_NAME)_data.img TRASH=H DESKTOP.DB=H
@grep -qE '^DESKTOP  DB ' $(BUILD)/$(FLAIR_SVC_NAME).root.lst && grep -qE '^TRASH .*<DIR>' $(BUILD)/$(FLAIR_SVC_NAME).root.lst || { printf '!!! test-flair-finder-service FAIL: mtools no longer lists DESKTOP.DB and TRASH in the root (hidden from the Finder only)\n'; cat $(BUILD)/$(FLAIR_SVC_NAME).root.lst; exit 1; }
@mtype -i $(BUILD)/$(FLAIR_SVC_NAME)_data.img ::/TRASH/README.TXT | cmp -s - $(FLAIR_DATA_README) || { printf '!!! test-flair-finder-service FAIL: README.TXT was not staged byte-identical into ::/TRASH\n'; exit 1; }
@[ "$$(mcopy -n -i $(BUILD)/$(FLAIR_SVC_NAME)_data.img ::/DESKTOP.DB - | wc -c)" = 80 ] || { printf '!!! test-flair-finder-service FAIL: DESKTOP.DB is not 8 + 3*24 = 80 bytes after the staging\n'; exit 1; }
@printf '>>> test-flair-finder-service [1/2]: root shows n=4 (pixel-graded, cells 4-5 bare); README staged, DB written (80 B); mtools still lists DESKTOP.DB + TRASH\n'
@# ---- leg 2: the reviewer's damaged volume ----
@$(call fixture-preflight,$(FLAIR_DAMAGED_DATA_IMG))
cp -f $(FLAIR_DAMAGED_DATA_IMG) $(BUILD)/$(FLAIR_SVC_DMG_NAME)_data.img
@$(if $(call fixture-spec,$(FLAIR_DAMAGED_DATA_IMG)),$(call fixture-check,harness,$(BUILD)/$(FLAIR_SVC_DMG_NAME)_data.img,$(call fixture-spec,$(FLAIR_DAMAGED_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(BUILD)/$(FLAIR_SVC_DMG_NAME)_data.img" --expect FLAIR-FAT-MOUNT-OK \
	--name "$(FLAIR_SVC_DMG_NAME)" --out "$(BUILD)" --mouse "$(FLAIR_SVC_DAMAGED_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--quit-after "FINDER-OPEN-FOLDER name=TRASH win=2" --timeout-ms 40000 2> "$(BUILD)/$(FLAIR_SVC_DMG_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_SVC_DMG_NAME).report"; then printf '!!! test-flair-finder-service FAIL: TRIPLE FAULT on the damaged volume\n'; exit 1; fi
@for l in 'FLAIR-FAT-MOUNT-OK' 'FINDER-OPEN-VOLUME win=0 n=4' 'FINDER-TRASH name=NEWFOLD origin=0' 'FINDER-OPEN-FOLDER name=APPS win=1 singleton=0' 'FINDER-OPEN-FOLDER name=TRASH win=2 singleton=0'; do \
	grep -qxF "$$l" "$(BUILD)/$(FLAIR_SVC_DMG_NAME).serial" || { printf '!!! test-flair-finder-service FAIL: leg 2 (damaged volume) -- missing serial line: %s\n' "$$l"; grep -E '^(FLAIR-FAT|FINDER|DESKTOP|TRASH)' "$(BUILD)/$(FLAIR_SVC_DMG_NAME).serial"; exit 1; }; done
@mdir -i $(BUILD)/$(FLAIR_SVC_DMG_NAME)_data.img ::/TRASH | grep -qE '^NEWFOLD .*<DIR>' || { printf '!!! test-flair-finder-service FAIL: NEWFOLD is not in the ROOT ::/TRASH\n'; exit 1; }
@mtype -i $(BUILD)/$(FLAIR_SVC_DMG_NAME)_data.img ::/APPS/TRASH/README.TXT | cmp -s - $(FLAIR_DATA_README) || { printf '!!! test-flair-finder-service FAIL: the stranded ::/APPS/TRASH/README.TXT was lost or changed (no user data may be deleted)\n'; exit 1; }
@[ "$$(mdir -a -i $(BUILD)/$(FLAIR_SVC_DMG_NAME)_data.img :: | grep -cE '^TRASH .*<DIR>')" = 1 ] || { printf '!!! test-flair-finder-service FAIL: the root does not hold exactly one TRASH\n'; exit 1; }
@printf '>>> test-flair-finder-service [2/2]: damaged volume boots; root n=4; NEWFOLD staged in ::/TRASH; stranded APPS\\TRASH opens as an ordinary folder and keeps README.TXT (mtools)\n'
@printf '>>> test-flair-finder-service: green\n'
```

## test-flair-finder-service-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 21557):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_win_service_shown.img build/flair_data.img build/ppm_flair_disk_windows_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-finder-service-mutant : Rule 6\n'
@printf '  Mutant: finder_windows.o -DFINDER_WIN_MUT_SERVICE_SHOWN (the listing filter removed)\n'
@printf '======================================================================\n'
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(BUILD)/flair_finder_service_mut_data.img
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(BUILD)/flair_finder_service_mut_data.img,$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_win_service_shown.img" --disk2 "$(BUILD)/flair_finder_service_mut_data.img" --expect FLAIR-FAT-MOUNT-OK \
	--name flair_finder_service_mut --out "$(BUILD)" --mouse "$(FLAIR_ICON_OPEN_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FINDER-OPEN-VOLUME win=0 n=6" \
	--timeout-ms 20000 2> "$(BUILD)/flair_finder_service_mut.report" || true
@grep -q '^FINDER-OPEN-VOLUME win=0 ' "$(BUILD)/flair_finder_service_mut.serial" && [ -s "$(BUILD)/flair_finder_service_mut.ppm" ] || { printf '!!! test-flair-finder-service-mutant: the mutant never opened the root / no dump (not comparable)\n'; exit 1; }
@if grep -qxF 'FINDER-OPEN-VOLUME win=0 n=4' "$(BUILD)/flair_finder_service_mut.serial"; then printf '!!! test-flair-finder-service-mutant FAIL: the filter-removed kernel still reported n=4 -- decoration\n'; exit 1; fi
@if $(PPM_FLAIR_DISKWIN_CHECK_BIN) rootwin "$(BUILD)/flair_finder_service_mut.ppm" > $(BUILD)/flair_finder_service_mut.grade 2>&1; then printf '!!! test-flair-finder-service-mutant FAIL: the grader PASSED a root window that draws DESKTOP.DB / TRASH -- decoration\n'; exit 1; fi
@grep -q 'is not bare content' $(BUILD)/flair_finder_service_mut.grade || { printf '!!! test-flair-finder-service-mutant FAIL: the grader went RED for the wrong reason\n'; cat $(BUILD)/flair_finder_service_mut.grade; exit 1; }
@printf '>>> test-flair-finder-service-mutant [1/2]: SERVICE_SHOWN correctly RED -- %s; grader: cells 4-5 not bare\n' "$$(grep -m1 '^FINDER-OPEN-VOLUME' $(BUILD)/flair_finder_service_mut.serial)"
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(BUILD)/flair_finder_service_guard_data.img
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(BUILD)/flair_finder_service_guard_data.img,$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_win_service_shown.img" --disk2 "$(BUILD)/flair_finder_service_guard_data.img" --expect FLAIR-FAT-MOUNT-OK \
	--name flair_finder_service_guard --out "$(BUILD)" --mouse "$(FLAIR_SVC_GUARD_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--quit-after "FINDER-MOVE-REFUSED" --timeout-ms 40000 2> "$(BUILD)/flair_finder_service_guard.report" || true
@grep -qxF 'FINDER-MOVE-REFUSED reason=service name=TRASH' "$(BUILD)/flair_finder_service_guard.serial" || { printf '!!! test-flair-finder-service-mutant FAIL: dragging the shown TRASH folder onto APPS was not refused by identity (reason=service)\n'; grep -E '^(FINDER|DESKTOP)' "$(BUILD)/flair_finder_service_guard.serial"; exit 1; }
@mdir -a -i $(BUILD)/flair_finder_service_guard_data.img :: | grep -qE '^TRASH .*<DIR>' && ! mdir -i $(BUILD)/flair_finder_service_guard_data.img ::/APPS | grep -q '^TRASH ' || { printf '!!! test-flair-finder-service-mutant FAIL: ::/TRASH left the root\n'; exit 1; }
@printf '>>> test-flair-finder-service-mutant [2/2]: with the filter removed, the H03 move is refused by the identity guard (reason=service; mtools: ::/TRASH stays at the root)\n'
```

## test-flair-finder-windows-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/finder_alerts.mk', line 272):
Prerequisites: `test-flair-finder-limits build/flair_tenants_mut_finder_windows_hold.img build/.makefile.stamp`
```make
$(call appl-boot,mut_finder_windows,build/flair_tenants_mut_finder_windows_hold.img,build/finder_limits_fixture.img,FLAIR_FINDER_LIMITS_SPEC,--screendump)
@$(call APPL_ALIVE,build/mut_finder_windows.serial) || { printf '!!! mut_finder_windows did not finish alive\n'; exit 1; }
@if awk '/^FINDER-OPEN-COUNT n=/{split($$2,a,"=");if(a[2]>4)ok=1}END{exit !ok}' build/mut_finder_windows.serial; then printf '!!! FINDER_MUT_WINDOW4 survived\n'; exit 1; fi
@printf '>>> test-flair-finder-windows-mutant: FINDER_MUT_WINDOW4 RED (Open-all stops at four-window ceiling)\n'
```

## test-flair-fullsave

Declared tier: EMU; #  recipe to execute (from 'harness/emu/fullsave.mk', line 111):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_fullsave_data.img build/.makefile.stamp`
```make
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
```

## test-flair-fullsave-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/fullsave.mk', line 142):
Prerequisites: `test-flair-fullsave build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_fullsave_mut_data.img build/flair_fullsave_collide_data.img build/.makefile.stamp`
```make
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
```

## test-flair-grow

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23158):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_window_ops_check build/.makefile.stamp`
```make
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_GROW_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_GROW_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-GROW win 0" --timeout-ms 20000 \
	2> "$(BUILD)/$(FLAIR_GROW_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_GROW_NAME).report"; then printf '!!! test-flair-grow FAIL: TRIPLE FAULT\n'; exit 1; fi
@grep -q '^FLAIR-GROW win 0 (300,200)->(96,88)$$' "$(BUILD)/$(FLAIR_GROW_NAME).serial" \
	|| { printf '!!! test-flair-grow FAIL: exact grow marker missing\n'; grep '^FLAIR-' "$(BUILD)/$(FLAIR_GROW_NAME).serial" || true; exit 1; }
@$(PPM_FLAIR_WINDOW_OPS_CHECK_BIN) grow "$(BUILD)/$(FLAIR_GROW_NAME).ppm"
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/$(FLAIR_GROW_NAME).serial"
@printf '>>> test-flair-grow: green -- outline release committed 96x88\n'
```

## test-flair-grow-box

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34588):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_grow_box_check`
```make
$(call scroll-boot,$(FLAIRTENANTS_IMG),flair_grow_box,$(FLAIR_DATA_IMG),$(FLAIR_GROW_BOX_SPEC),FLAIR-LIVE-OK,1)
@$(call APPL_ALIVE,$(BUILD)/flair_grow_box.serial)
@grep -qx 'FINDER-OPEN-FOLDER name=APPS win=1 singleton=0' $(BUILD)/flair_grow_box.serial
@grep -qx 'FLAIR-GROW win 0 (360,220)->(400,260)' $(BUILD)/flair_grow_box.serial
@! grep -Eq '^(PANIC|FLAIR-DRAG)' $(BUILD)/flair_grow_box.serial
@$(GROW_BOX_CHECK) $(BUILD)/flair_grow_box.ppm "$(GROW_BOX_ACTIVE_PNG)"
@printf '>>> test-flair-grow-box: green (APPS +40,+40; 400x260 serial marker and frame; 324/324 golden pixels)\n'
```

## test-flair-grow-box-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 212):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-grow-box-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-grow-box-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-grow-box_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-grow-box_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-grow-box_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-grow-box_bochs.report.txt" || true
@cat "$(BUILD)/flair-grow-box_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-grow-box-bochs "$(BUILD)/flair-grow-box_bochs.report.txt" "$(BUILD)/flair-grow-box_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-grow-box-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34600):
Prerequisites: `test-flair-grow-box build/flair_tenants_mut_grow_figure.img build/flair_tenants_mut_grow_no_commit.img`
```make
$(call scroll-boot,$(BUILD)/flair_tenants_mut_grow_figure.img,flair_grow_box_mut_figure,$(FLAIR_DATA_IMG),$(FLAIR_GROW_BOX_SPEC),FLAIR-LIVE-OK,1)
@$(call APPL_ALIVE,$(BUILD)/flair_grow_box_mut_figure.serial)
@grep -qx 'FLAIR-GROW win 0 (360,220)->(400,260)' $(BUILD)/flair_grow_box_mut_figure.serial
@if $(GROW_BOX_CHECK) $(BUILD)/flair_grow_box_mut_figure.ppm "$(GROW_BOX_ACTIVE_PNG)" > $(BUILD)/flair_grow_box_mut_figure.pixels 2>&1; then printf 'FAIL grow figure mutant survived\n'; exit 1; fi
@grep -q 'FAIL grow-box active crop' $(BUILD)/flair_grow_box_mut_figure.pixels
@printf '>>> test-flair-grow-box-mutant: CHROME_MUT_GROW_FIGURE_SWAP correctly RED: %s\n' "$$(head -1 $(BUILD)/flair_grow_box_mut_figure.pixels)"
$(call scroll-boot,$(BUILD)/flair_tenants_mut_grow_no_commit.img,flair_grow_box_mut_commit,$(FLAIR_DATA_IMG),$(FLAIR_GROW_BOX_SPEC),FLAIR-LIVE-OK,1)
@$(call APPL_ALIVE,$(BUILD)/flair_grow_box_mut_commit.serial)
@grep -qx 'FLAIR-GROW win 0 (360,220)->(400,260)' $(BUILD)/flair_grow_box_mut_commit.serial
@if $(GROW_BOX_CHECK) $(BUILD)/flair_grow_box_mut_commit.ppm "$(GROW_BOX_ACTIVE_PNG)" > $(BUILD)/flair_grow_box_mut_commit.pixels 2>&1; then printf 'FAIL grow commit mutant survived\n'; exit 1; fi
@grep -q 'FAIL grow-box committed frame' $(BUILD)/flair_grow_box_mut_commit.pixels
@printf '>>> test-flair-grow-box-mutant: WINDOW_MUT_GROW_NO_COMMIT correctly RED: %s\n' "$$(grep 'FAIL grow-box committed frame' $(BUILD)/flair_grow_box_mut_commit.pixels)"
@printf '>>> test-flair-grow-box-mutant: green (colour and resize-commit mutants RED)\n'
```

## test-flair-headers

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15918):
Prerequisites: `build/test_flair_headers build/.makefile.stamp`
```make
@printf ">>> test-flair-headers: FLAIR locked-spec compile contract (grafport/imaging static_asserts) + frozen canon (hourglass + Photoshop menu)\n"
@$(TEST_FLAIR_HEADERS)
@printf ">>> test-flair-headers: green\n"
```

## test-flair-headers-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15923):
Prerequisites: `build/test_flair_headers_mutant build/.makefile.stamp`
```make
@printf ">>> test-flair-headers-mutant: confirming the canon-byte mutant goes RED (Rule 6)\n"
@if $(TEST_FLAIR_HEADERS_MUT) >/dev/null 2>&1; then \
	printf '!!! test-flair-headers-mutant FAIL: mutant PASSED -- the canon oracle is decoration\n'; exit 1; \
else \
	printf '>>> test-flair-headers-mutant: green (canon mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-flair-held-gestures

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30300):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_hold.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-held-gestures : gestures track until mouseUp (G01)\n'
@printf '  bead initech-tdnl.59; traces FLAIR_HELD_DRAG_SPEC / FLAIR_HELD_MENU_SPEC\n'
@printf '======================================================================\n'
$(call held-boot,$(FLAIRTENANTS_HOLD_IMG),flair_held_drag,FLAIR_HELD_DRAG_SPEC,FLAIR-GROW win)
@$(call held-drag-check,"$(BUILD)/flair_held_drag.serial") || { printf '!!! test-flair-held-gestures FAIL: HELD-DRAG -- a 3 s hold cut the drag/grow short (want (60,60)->(190,170) and (300,200)->(180,120), no FLAIR-TRACK-EXPIRED)\n'; grep -E '^FLAIR-(DRAG|GROW|TRACK|EVT what=[12])' "$(BUILD)/flair_held_drag.serial"; exit 1; }
@printf '>>> test-flair-held-gestures [1/2]: HELD-DRAG -- %s ; %s\n' "$$(grep -m1 '^FLAIR-DRAG' $(BUILD)/flair_held_drag.serial)" "$$(grep -m1 '^FLAIR-GROW' $(BUILD)/flair_held_drag.serial)"
$(call held-boot,$(FLAIRTENANTS_HOLD_IMG),flair_held_menu,FLAIR_HELD_MENU_SPEC,FINDER-CLOSE-WINDOW win=0)
@$(call held-menu-check,"$(BUILD)/flair_held_menu.serial") || { printf '!!! test-flair-held-gestures FAIL: HELD-MENU -- a 3 s hold executed the hovered item or ended the drag/menu before release\n'; grep -E '^(FINDER-|FLAIR-MENU|FLAIR-TRACK|FLAIR-EVT what=[12])' "$(BUILD)/flair_held_menu.serial"; exit 1; }
@printf '>>> test-flair-held-gestures [2/2]: HELD-MENU -- icon drag reached APPS; the held File menu chose Close Window on release, never New Folder\n'
@printf '>>> test-flair-held-gestures: green\n'
```

## test-flair-held-gestures-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30313):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_hold_timeout.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-held-gestures-mutant : Rule 6\n'
@printf '  Mutant: -DKMAIN_MUT_TRACK_TIMEOUT (the pre-tdnl.59 150-tick guard that commits)\n'
@printf '======================================================================\n'
$(call held-boot,$(FLAIRTENANTS_HOLD_MUT_IMG),flair_held_drag_mut,FLAIR_HELD_DRAG_SPEC,FLAIR-GROW win)
@grep -q '^FLAIR-DRAG win' "$(BUILD)/flair_held_drag_mut.serial" || { printf '!!! test-flair-held-gestures-mutant: HELD-DRAG mutant made no drag (not comparable)\n'; exit 1; }
@if $(call held-drag-check,"$(BUILD)/flair_held_drag_mut.serial"); then printf '!!! test-flair-held-gestures-mutant FAIL: HELD-DRAG passed on the timeout mutant -- decoration\n'; exit 1; fi
@printf '>>> test-flair-held-gestures-mutant [1/2]: RED as required -- %s\n' "$$(grep -m1 '^FLAIR-DRAG' $(BUILD)/flair_held_drag_mut.serial)"
$(call held-boot,$(FLAIRTENANTS_HOLD_MUT_IMG),flair_held_menu_mut,FLAIR_HELD_MENU_SPEC,FLAIR-LIVE-OK)
@grep -qxF 'FINDER-OPEN-VOLUME win=0 n=4' "$(BUILD)/flair_held_menu_mut.serial" || { printf '!!! test-flair-held-gestures-mutant: HELD-MENU mutant never opened the root (not comparable)\n'; exit 1; }
@if $(call held-menu-check,"$(BUILD)/flair_held_menu_mut.serial"); then printf '!!! test-flair-held-gestures-mutant FAIL: HELD-MENU passed on the timeout mutant -- decoration\n'; exit 1; fi
@printf '>>> test-flair-held-gestures-mutant [2/2]: RED as required -- %s\n' "$$(grep -E -m1 '^FINDER-(CMD|MOVE|WIN-DRAG)' $(BUILD)/flair_held_menu_mut.serial || echo 'no command')"
```

## test-flair-icon-oracles

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 21862):
Prerequisites: `test-finder-icon-oracles test-flair-locked-purge test-flair-finder-icons build/.makefile.stamp`
```make
@printf '>>> test-flair-icon-oracles: green (selected FULL Trash after locked purge; 7 named scene cells)\n'
```

## test-flair-icon-oracles-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 21866):
Prerequisites: `test-flair-icon-oracles build/ppm_flair_trash_check_mut_locked_unselected build/.makefile.stamp`
```make
@rc=0; $(BUILD)/ppm_flair_trash_check_mut_locked_unselected locked-full $(BUILD)/locked_trash_purge.ppm > $(BUILD)/icon_oracles_mut_unselected.grade 2>&1 || rc=$$?; \
[ "$$rc" = 1 ] && grep -q 'FAIL leg locked-full -- FULL Trash (TRASH_FULL): 617/1024' $(BUILD)/icon_oracles_mut_unselected.grade \
|| { cat $(BUILD)/icon_oracles_mut_unselected.grade; exit 1; }; \
printf '>>> TRASH_CHECK_MUT_LOCKED_UNSELECTED: RED -- booted selected FULL can rejected (617/1024)\n'
@rc=0; $(PPM_FLAIR_ICONS_CHECK_BIN) "$(ICON_CAPTURES_DIR)" $(BUILD)/flair_finder_icons.ppm $(ICON_SCENE_MUT_STALE_DOC) > $(BUILD)/icon_oracles_mut_stale_doc.grade 2>&1 || rc=$$?; \
[ "$$rc" = 1 ] && grep -q 'FAIL: 1 icon cell(s) differ' $(BUILD)/icon_oracles_mut_stale_doc.grade && grep -q 'DOC UNSEL at (478,258).*626/1024' $(BUILD)/icon_oracles_mut_stale_doc.grade \
|| { cat $(BUILD)/icon_oracles_mut_stale_doc.grade; exit 1; }; \
printf '>>> ICON_SCENE_MUT_STALE_DOC: RED -- booted PAINT.EXE graded as DOC (626/1024); other 6 cells pass\n'
@printf '>>> test-flair-icon-oracles-mutant: green\n'
```

## test-flair-key

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 19775):
Prerequisites: `build/qemu_harness build/flair_live.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'Booting   : %s (FO-5 kbd raw post -> FO-7 WaitNextEvent cook)\n' "$(FLAIRLIVE_IMG)"
@printf 'Expecting : cooked FLAIR-EVT what=3 where=320,240 msg=00001E61 (keyDown a)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@# initech-qed1: quit-after the exact cooked marker this gate asserts on --
@# the IRQ1->FLAIR-ring->WaitNextEvent cook is fast but not instant, and the
@# legacy ~400ms-after-key quit can outrun it under host load.
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_IMG)" \
	--keys "a" --keys-after "FLAIR-LIVE-READY" \
	--quit-after "FLAIR-EVT what=3 where=320,240 msg=00001E61" \
	--name "$(FLAIR_KEY_NAME)" --out "$(BUILD)" --timeout-ms 15000 \
	2> "$(FLAIR_KEY_REPORT)" || true
@cat "$(FLAIR_KEY_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(FLAIR_KEY_REPORT)"; then \
	printf '!!! test-flair-key FAIL: triple fault\n'; exit 1; \
fi
@if [ ! -s "$(FLAIR_KEY_SERIAL)" ]; then \
	printf '!!! test-flair-key FAIL: no serial captured\n'; exit 1; \
fi
@grep -q '^FLAIR-EVT what=3 where=320,240 msg=00001E61$$' "$(FLAIR_KEY_SERIAL)" \
	|| { printf '!!! test-flair-key FAIL: cooked keyDown a (FLAIR-EVT what=3 msg=00001E61, scancode 0x1E -> ASCII 0x61) not on serial -- WaitNextEvent did not cook the IRQ1 scancode into an EventRecord\n'; grep '^FLAIR-EVT ' "$(FLAIR_KEY_SERIAL)" || true; exit 1; }
@printf '>>> test-flair-key PASS -- kbd IRQ1 -> FLAIR ring -> WaitNextEvent COOK -> FLAIR-EVT keyDown a LIVE (FO-5/7)\n'
@printf '======================================================================\n'
```

## test-flair-key-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 19808):
Prerequisites: `build/qemu_harness build/flair_live_mut_kbd.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-key-mutant : Rule 6 (the gate bites)\n'
@printf '  Mutant: -DFLAIR_LIVE_MUTATE_NO_KBD_HOOK (kbd_set_scancode_hook NEVER called).\n'
@printf '  Expect: cooked keyDown a (FLAIR-EVT msg=00001E61) NEVER appears -> test-flair-key would go RED.\n'
@printf '======================================================================\n'
@# initech-qed1: --legacy-quit -- this leg asserts ABSENCE (the cooked keyDown
@# NEVER appears because the hook was never installed); there is no marker to
@# name for an event that must not occur, and waiting longer cannot
@# manufacture output that will never come, so the legacy quit is honest here.
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_MUT_KBD_IMG)" \
	--keys "a" --keys-after "FLAIR-LIVE-READY" --legacy-quit \
	--name "$(FLAIR_KEY_MUT_NAME)" --out "$(BUILD)" --timeout-ms 15000 \
	2> "$(FLAIR_KEY_MUT_REPORT)" || true
@cat "$(FLAIR_KEY_MUT_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@grep -q '^FLAIR-HOOK-SET$$' "$(FLAIR_KEY_MUT_SERIAL)" \
	|| { printf '!!! test-flair-key-mutant FAIL: mutant did not reach FLAIR-HOOK-SET (not the same boot path)\n'; exit 1; }
@# Match the SPECIFIC cooked 'a' keyDown (msg=00001E61), NOT a bare ^FLAIR-EVT:
@# the gate keys on the injected key's message. (The boot-time 0xFA ACK via
@# IRQ1 used to cook into a phantom keyDown msg=0000FA00; since initech-tdnl.58
@# event.c treats controller bytes as non-keys, but the specific match stays.)
@if grep -q '^FLAIR-EVT what=3 .*msg=00001E61' "$(FLAIR_KEY_MUT_SERIAL)" 2>/dev/null; then \
	printf '!!! test-flair-key-mutant FAIL: cooked keyDown a appeared despite no kbd hook -- the gate is decoration\n'; exit 1; \
fi
@printf '>>> test-flair-key-mutant: RED as required -- no kbd hook => no cooked keyDown a; the gate BITES (Rule 6)\n'
@printf '======================================================================\n'
```

## test-flair-launchkey

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/flair_launchkey.mk', line 26):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_launchkey_gate.img build/launchkey_data.img build/.makefile.stamp`
```make
$(call appl-boot,$(LAUNCHKEY_TAG)_plain,$(LAUNCHKEY_IMAGE),$(LAUNCHKEY_DATA),FLAIR_LAUNCHKEY_PLAIN_SPEC,)
@grep -qx 'TENANT-REGISTER ok app=123' $(BUILD)/$(LAUNCHKEY_TAG)_plain.serial && \
 ! grep -q '^FLAIR-SUSPEND' $(BUILD)/$(LAUNCHKEY_TAG)_plain.serial && \
 $(call LAUNCHKEY_REPEAT,$(BUILD)/$(LAUNCHKEY_TAG)_plain.serial) \
 || { printf '!!! test-flair-launchkey FAIL: plain backslash did not draw repeat label without SAMIR\n'; exit 1; }
@$(call APPL_ALIVE,$(BUILD)/$(LAUNCHKEY_TAG)_plain.serial) || { printf '!!! test-flair-launchkey FAIL: plain desktop not alive\n'; exit 1; }
@printf '>>> test-flair-launchkey: plain backslash-minus draws nine hyphens in A1; no SAMIR launch\n'
$(call appl-boot,$(LAUNCHKEY_TAG)_resume,$(LAUNCHKEY_IMAGE),$(LAUNCHKEY_DATA),FLAIR_LAUNCHKEY_RESUME_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/$(LAUNCHKEY_TAG)_resume.serial) && \
 $(call LAUNCHKEY_RECOVERY,$(BUILD)/$(LAUNCHKEY_TAG)_resume.serial) \
 || { printf '!!! test-flair-launchkey FAIL: chord / QUIT / sheet recovery\n'; exit 1; }
@printf '>>> test-flair-launchkey: green (Ctrl+Alt+backslash launches SAMIR; QUIT resumes sheet; Right reaches B1)\n'
```

## test-flair-launchkey-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/flair_launchkey.mk', line 40):
Prerequisites: `build/qemu_harness build/launchkey_data.img build/flair_tenants_mut_plain_bsl_launch.img build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(MAKE) --no-print-directory -j3 test-flair-launchkey \
 LAUNCHKEY_IMAGE=$(BUILD)/flair_tenants_mut_plain_bsl_launch.img LAUNCHKEY_TAG=launchkey_mut \
 >$(BUILD)/launchkey-mutant.log 2>&1; rc=$$?; set -e; \
 test $$rc -ne 0 && grep -q '^FLAIR-SUSPEND$$' $(BUILD)/launchkey_mut_plain.serial && \
 ! grep -q 'triple_fault=1' $(BUILD)/launchkey_mut_plain.report && \
 grep -F '!!! test-flair-launchkey FAIL: plain backslash did not draw repeat label without SAMIR' $(BUILD)/launchkey-mutant.log \
 || { cat $(BUILD)/launchkey-mutant.log; exit 1; }
@printf '>>> test-flair-launchkey-mutant: green (FLAIR_MUT_PLAIN_BSL_LAUNCH RED: plain backslash launched SAMIR)\n'
```

## test-flair-launchkey-record

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/flair_launchkey.mk', line 60):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_record.img build/flair_tenants_recorddbl.img build/flair_live_interactive.img build/flair_data.img build/launchkey_data.img build/.makefile.stamp`
```make
+@INITECH_QMP_STDIO=1 $(MAKE) --no-print-directory -j3 record-flair SCRIPT=launchkey
@# A recorder stops after its final frame, before the bounded LIVE-OK.
@grep -qx '\[harness\] OK=1' $(RECORD_CLIPS_DIR)/rec_launchkey.report
@$(call LAUNCHKEY_RECOVERY,$(RECORD_CLIPS_DIR)/rec_launchkey.serial) \
 || { printf '!!! test-flair-launchkey-record FAIL: repeating label / SAMIR / QUIT / resumed sheet\n'; exit 1; }
@printf '>>> test-flair-launchkey-record: green (locked trace clip + repeat label + SAMIR exit + resumed B1)\n'
```

## test-flair-list-view

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 20898):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_list_view_check build/.makefile.stamp`
```make
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(BUILD)/list_view_data.img
$(call list-view-boot,list_view_folder,$(FLAIRTENANTS_IMG),$(FLAIR_LIST_ROOT_SPEC),FINDER-VIEW win=0)
@$(LIST_VIEW_CHECK) $(BUILD)/list_view_folder.ppm $(LIST_VIEW_CAPTURE) folder
$(call list-view-boot,list_view_name,$(FLAIRTENANTS_IMG),$(FLAIR_LIST_NAME_SPEC))
@grep -qx 'FINDER-VIEW win=1 mode=list sort=name bits=1' $(BUILD)/list_view_name.serial
@$(LIST_VIEW_CHECK) $(BUILD)/list_view_name.ppm $(LIST_VIEW_CAPTURE) name
$(call list-view-boot,list_view_size,$(FLAIRTENANTS_IMG),$(FLAIR_LIST_SIZE_SPEC))
@grep -qx 'FINDER-VIEW win=1 mode=list sort=size bits=5' $(BUILD)/list_view_size.serial
@$(LIST_VIEW_CHECK) $(BUILD)/list_view_size.ppm $(LIST_VIEW_SIZE_CAPTURE) size
@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(BUILD)/list_view_data.img" --expect FLAIR-FAT-MOUNT-OK --name list_view_restart --out $(BUILD) --mouse "$(FLAIR_LIST_RESTART_SPEC)" --keys-after FLAIR-LIVE-READY --allow-reboot --quit-after FLAIR-LIVE-READY --quit-after-count 2 --timeout-ms 30000 > $(BUILD)/list_view_restart.out 2> $(BUILD)/list_view_restart.report
@grep -qx FLAIR-RESTART $(BUILD)/list_view_restart.serial
@grep -q '^DESKTOP-DB-SAVE ' $(BUILD)/list_view_restart.serial
@test "$$(grep -c '^FLAIR-LIVE-READY' $(BUILD)/list_view_restart.serial)" -eq 2
mcopy -o -i $(BUILD)/list_view_data.img ::DESKTOP.DB $(BUILD)/list_view_desktop.db
@$(LIST_VIEW_CHECK) --db $(BUILD)/list_view_desktop.db
$(call list-view-boot,list_view_reopen,$(FLAIRTENANTS_IMG),$(FLAIR_LIST_REOPEN_SPEC))
@grep -qx 'FINDER-VIEW-RESTORE win=1 bits=5' $(BUILD)/list_view_reopen.serial
@$(LIST_VIEW_CHECK) $(BUILD)/list_view_reopen.ppm $(LIST_VIEW_SIZE_CAPTURE) reopen
@printf '>>> test-flair-list-view: green (Name/Size header cells, sort shading, Restart kind=4)\n'
```

## test-flair-list-view-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 20919):
Prerequisites: `test-flair-list-view build/flair_tenants_mut_list_header_face.img build/flair_tenants_mut_list_small_icon_shift.img build/.makefile.stamp`
```make
cp -f $(FLAIR_DATA_IMG) $(BUILD)/list_view_data.img
$(call list-view-boot,list_view_mutant,$(BUILD)/flair_tenants_mut_list_header_face.img,$(FLAIR_LIST_SIZE_SPEC))
@grep -qx 'FINDER-VIEW win=1 mode=list sort=size bits=5' $(BUILD)/list_view_mutant.serial
@if $(LIST_VIEW_CHECK) $(BUILD)/list_view_mutant.ppm $(LIST_VIEW_SIZE_CAPTURE) size > $(BUILD)/list_view_mutant.log 2>&1; then printf 'FAIL LIST_HEADER_FACE passed\n'; exit 1; fi
@grep -q 'FAIL list-view header pixel' $(BUILD)/list_view_mutant.log
@head -1 $(BUILD)/list_view_mutant.log
cp -f $(FLAIR_DATA_IMG) $(BUILD)/list_view_data.img
$(call list-view-boot,list_view_icon_mutant,$(BUILD)/flair_tenants_mut_list_small_icon_shift.img,$(FLAIR_LIST_NAME_SPEC))
@grep -qx 'FINDER-VIEW win=1 mode=list sort=name bits=1' $(BUILD)/list_view_icon_mutant.serial
@if $(LIST_VIEW_CHECK) $(BUILD)/list_view_icon_mutant.ppm $(LIST_VIEW_CAPTURE) name > $(BUILD)/list_view_icon_mutant.log 2>&1; then printf 'FAIL SMALL_ICON_SHIFT passed\n'; exit 1; fi
@grep -q 'FAIL list-view small application pixel' $(BUILD)/list_view_icon_mutant.log
@head -1 $(BUILD)/list_view_icon_mutant.log
@printf '>>> test-flair-list-view-mutant: green (LIST_HEADER_FACE and SMALL_ICON_SHIFT correctly RED)\n'
```

## test-flair-live

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 19672):
Prerequisites: `build/qemu_harness build/flair_live.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-live : FLAIR PIT-TICK lane (FO-4)\n'
@printf '  Ref: ADR-0006 E-D3a/E-D4/FO-4 (the cooperative WaitNextEvent time base);\n'
@printf '       pit.c:93 (tick hook before PIC EOI); os/flair/event.c:98.\n'
@printf '  beads initech-5l5z FO-4. CLAUDE.md Law 2, Rule 2/5/6/11.\n'
@printf '  TRI-EMULATOR: QEMU here; Bochs leg = make test-flair-live-bochs.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (FLAIR live cooperative tick kernel)\n' "$(FLAIRLIVE_IMG)"
@printf 'Expecting : FLAIR-HOOK-SET + IRQ-LIVE + FLAIR-TICK advancing + FLAIR-LIVE-OK\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_IMG)" \
	--name "$(FLAIR_LIVE_NAME)" --out "$(BUILD)" --timeout-ms 8000 \
	2> "$(FLAIR_LIVE_REPORT)" || true
@cat "$(FLAIR_LIVE_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@# ---- 1. No triple-fault (sti + the PIT IRQ path did not cascade to a reboot). ----
@if grep -q 'triple_fault=1' "$(FLAIR_LIVE_REPORT)"; then \
	printf '!!! test-flair-live FAIL: TRIPLE FAULT in the live tick boot\n'; \
	exit 1; \
fi
@printf '>>> test-flair-live [1/4]: no triple-fault\n'
@# ---- 2. Serial captured + the hook+sti milestones reached. ----
@if [ ! -s "$(FLAIR_LIVE_SERIAL)" ]; then \
	printf '!!! test-flair-live FAIL: no serial captured at %s\n' "$(FLAIR_LIVE_SERIAL)"; \
	exit 1; \
fi
@grep -q '^FLAIR-HOOK-SET$$' "$(FLAIR_LIVE_SERIAL)" \
	|| { printf '!!! test-flair-live FAIL: FLAIR-HOOK-SET missing -- the desktop present/hook-install path never reached\n'; exit 1; }
@grep -q '^IRQ-LIVE$$' "$(FLAIR_LIVE_SERIAL)" \
	|| { printf '!!! test-flair-live FAIL: IRQ-LIVE missing -- sti/PIC-unmask path did not complete\n'; exit 1; }
@printf '>>> test-flair-live [2/4]: FLAIR-HOOK-SET + IRQ-LIVE (hook installed, first sti survived)\n'
@# ---- 3. THE FO-4 PROOF: the FLAIR tick ADVANCES on metal. Count the DISTINCT
@#         "FLAIR-TICK n=<count>" lines; >=2 distinct counts proves the PIT IRQ ->
@#         hook -> flair_tick_advance wiring actually fires (a dead hook -- the
@#         mutant -- emits ZERO). ----
@n=$$(grep -c '^FLAIR-TICK n=' "$(FLAIR_LIVE_SERIAL)"); \
d=$$(grep '^FLAIR-TICK n=' "$(FLAIR_LIVE_SERIAL)" | sort -u | wc -l); \
if [ "$$d" -lt 2 ]; then \
	printf '!!! test-flair-live FAIL: FLAIR-TICK did not advance (%s lines, %s distinct) -- the PIT tick hook never drove flair_tick_advance\n' "$$n" "$$d"; \
	exit 1; \
fi; \
printf '>>> test-flair-live [3/4]: FLAIR-TICK advanced (%s emissions, %s distinct counts) -- PIT IRQ -> hook -> flair_tick_advance LIVE\n' "$$n" "$$d"
@# ---- 4. FLAIR-LIVE-OK terminal marker (the bounded loop completed its goal). ----
@grep -q '^FLAIR-LIVE-OK$$' "$(FLAIR_LIVE_SERIAL)" \
	|| { printf '!!! test-flair-live FAIL: FLAIR-LIVE-OK missing -- the bounded tick loop never reached its goal\n'; exit 1; }
@printf '>>> test-flair-live [4/4]: FLAIR-LIVE-OK (the bounded wake loop converged on >=4 ticks)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- the FLAIR PIT tick lane is LIVE; IRQ0 drives the\n'
@printf '            flair_tick_advance hook into the cooperative time base on metal\n'
@printf '            (QEMU; Bochs leg = make test-flair-live-bochs)\n'
@printf '======================================================================\n'
```

## test-flair-live-bochs

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 25399):
Prerequisites: `build/bochs_harness build/flair_live.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-live-bochs : BOCHS leg (FLAIR live tick)\n'
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator). beads initech-5l5z FO-4.\n'
@printf '  Bochs 2.7: VBE ENOMODE -> mode-0x13 (320x200); the 640x480 desktop\n'
@printf '  cannot fit, so kmain FAILS LOUD (Rule 2) BEFORE the tick loop; the leg\n'
@printf '  proves the shared boot/heap milestones + that the guard fires, no reboot.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s under Bochs (RFB headless; serial via com1=file)\n' "$(FLAIRLIVE_IMG)"
@printf 'Expecting : VGA13 fallback + shared kernel markers + the 640x480 fail-loud\n'
@printf '            guard (PANIC + HALTED), no triple-fault\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(BOCHS_BIN) --disk "$(FLAIRLIVE_IMG)" --expect HALTED \
	--name "$(FLAIR_LIVE_BOCHS_NAME)" --out "$(BUILD)" --timeout-ms 45000 \
	2> "$(FLAIR_LIVE_BOCHS_REPORT)" || true
@cat "$(FLAIR_LIVE_BOCHS_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@grep -q 'rfb_unblocked=1' "$(FLAIR_LIVE_BOCHS_REPORT)" \
	|| { printf '!!! test-flair-live-bochs FAIL: RFB unblock failed -- Bochs did not run the guest\n'; exit 1; }
@if grep -q 'triple_fault=1' "$(FLAIR_LIVE_BOCHS_REPORT)"; then \
	printf '!!! test-flair-live-bochs FAIL: TRIPLE FAULT under Bochs\n'; exit 1; \
fi
@if [ ! -s "$(FLAIR_LIVE_BOCHS_SERIAL)" ]; then \
	printf '!!! test-flair-live-bochs FAIL: no serial captured at %s\n' "$(FLAIR_LIVE_BOCHS_SERIAL)"; exit 1; \
fi
@printf 'Serial markers captured:\n'
@for m in S1 VBE-ENOMODE VGA13 PM KERNEL CONSOLE BANNER FLAIR-HEAP-OK HALTED; do \
	if grep -q "^$$m$$" "$(FLAIR_LIVE_BOCHS_SERIAL)"; then printf '  %-14s : present\n' "$$m"; \
	else printf '  %-14s : MISSING\n' "$$m"; fi; \
done
@# 1. The Bochs 320x200 fallback fired (VBE ENOMODE -> mode 0x13).
@for m in VBE-ENOMODE VGA13; do \
	grep -q "^$$m$$" "$(FLAIR_LIVE_BOCHS_SERIAL)" \
		|| { printf '!!! test-flair-live-bochs FAIL: fallback marker %s missing\n' "$$m"; exit 1; }; \
done
@# 2. The SHARED kernel + FLAIR-heap milestones -- the differential vs QEMU.
@for m in S1 PM KERNEL CONSOLE BANNER FLAIR-HEAP-OK; do \
	grep -q "^$$m$$" "$(FLAIR_LIVE_BOCHS_SERIAL)" \
		|| { printf '!!! test-flair-live-bochs FAIL: shared kernel marker %s missing under Bochs\n' "$$m"; exit 1; }; \
done
@# 3. The 640x480-required fail-loud guard FIRED (PANIC + HALTED), NOT a tick
@#    wedge and NOT a triple-fault. (FLAIR-TICK is QEMU-only -- see header.)
@grep -q 'PANIC flair-desktop: LFB smaller than 640x480' "$(FLAIR_LIVE_BOCHS_SERIAL)" \
	|| { printf '!!! test-flair-live-bochs FAIL: the 640x480-required guard did NOT fire under the 320x200 fallback\n'; exit 1; }
@grep -q '^HALTED$$' "$(FLAIR_LIVE_BOCHS_SERIAL)" \
	|| { printf '!!! test-flair-live-bochs FAIL: HALTED terminal marker missing\n'; exit 1; }
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- the flair_live kernel booted under Bochs through the shared\n'
@printf '            kernel + FLAIR-heap milestones (== QEMU), and the 640x480 fail-loud\n'
@printf '            guard fired correctly under the 320x200 fallback (no triple-fault).\n'
@printf '            NOTE: the FLAIR-TICK proof is QEMU-only (test-flair-live) -- Bochs\n'
@printf '            has no 640x480 LFB so the present halts before the tick loop.\n'
@printf '======================================================================\n'
```

## test-flair-live-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 19738):
Prerequisites: `build/qemu_harness build/flair_live_mut.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-live-mutant : Rule 6 (the gate bites)\n'
@printf '  Mutant: -DFLAIR_LIVE_MUTATE_NO_HOOK (pit_set_tick_hook NEVER called).\n'
@printf '  Expect: FLAIR-TICK NEVER appears -> test-flair-live would go RED.\n'
@printf '======================================================================\n'
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_MUT_IMG)" \
	--name "$(FLAIR_LIVE_MUT_NAME)" --out "$(BUILD)" --timeout-ms 8000 \
	2> "$(FLAIR_LIVE_MUT_REPORT)" || true
@cat "$(FLAIR_LIVE_MUT_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if [ ! -s "$(FLAIR_LIVE_MUT_SERIAL)" ]; then \
	printf '!!! test-flair-live-mutant FAIL: no serial captured (cannot judge the mutant)\n'; \
	exit 1; \
fi
@# The mutant DID reach the hook-install site (so it is the SAME boot path) but
@# SKIPPED the install: FLAIR-HOOK-SET present, FLAIR-TICK absent.
@grep -q '^FLAIR-HOOK-SET$$' "$(FLAIR_LIVE_MUT_SERIAL)" \
	|| { printf '!!! test-flair-live-mutant FAIL: mutant did not reach FLAIR-HOOK-SET (not the same boot path -- mutant is not comparable)\n'; exit 1; }
@if grep -q '^FLAIR-TICK n=' "$(FLAIR_LIVE_MUT_SERIAL)"; then \
	printf '!!! test-flair-live-mutant FAIL: the no-hook mutant STILL emitted FLAIR-TICK -- the gate is decoration (tick advanced without the hook?!)\n'; \
	exit 1; \
fi
@printf '>>> test-flair-live-mutant: RED as required -- no hook => FLAIR-TICK never advanced; the gate BITES (Rule 6)\n'
@printf '======================================================================\n'
```

## test-flair-live-scene

Declared tier: EMU; #  recipe to execute (from 'harness/film_scene.mk', line 13):
Prerequisites: `build/qemu_harness build/flair_live_interactive.img build/flair_data.img build/ppm_flair_calc_check build/.makefile.stamp`
```make
$(call appl-boot,film_scene_busy,$(FLAIRLIVE_INTERACTIVE_IMG),$(FLAIR_DATA_IMG),FLAIR_FILM_BUSY_SPEC,--screendump --screendump-after "FLAIR-CURSOR x=470 y=228" --quit-after "FLAIR-CURSOR x=470 y=228")
@$(BUILD)/ppm_flair_calc_check $(BUILD)/film_scene_busy.ppm $(CALC_GOLDEN) busy
@$(call CALC_DRAW_OK,$(BUILD)/film_scene_busy.serial) && grep -qx 'FLAIR-FILM-SCENE CALC ledger+chart' $(BUILD)/film_scene_busy.serial || { printf 'FAIL film real-tenant content trace\n'; exit 1; }
$(call appl-boot,film_scene_drag,$(FLAIRLIVE_INTERACTIVE_IMG),$(FLAIR_DATA_IMG),FLAIR_FILM_DRAG_SPEC,--screendump --screendump-after "FLAIR-CURSOR x=620 y=440" --quit-after "FLAIR-CURSOR x=620 y=440")
@grep -qx 'FLAIR-DRAG win 0 (300,120)->(360,160)' $(BUILD)/film_scene_drag.serial && grep -qx 'FLAIR-FILM-MODAL dismissed' $(BUILD)/film_scene_drag.serial || { printf 'FAIL film real-window drag trace\n'; exit 1; }
@$(BUILD)/ppm_flair_calc_check $(BUILD)/film_scene_drag.ppm $(CALC_GOLDEN) drag
@printf '>>> test-flair-live-scene: green (real Calc, authored arrangement, Saving hourglass, five labelled slices after drag)\n'
```

## test-flair-live-scene-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 130):
Prerequisites: `build/bochs_harness build/flair_live_interactive.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-live-scene-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_live_interactive.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-live-scene-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-live-scene_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_live_interactive.img" --disk2 "$(BUILD)/flair-live-scene_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-live-scene_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-live-scene_bochs.report.txt" || true
@cat "$(BUILD)/flair-live-scene_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-live-scene-bochs "$(BUILD)/flair-live-scene_bochs.report.txt" "$(BUILD)/flair-live-scene_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-live-scene-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/film_scene.mk', line 59):
Prerequisites: `test-flair-live-scene build/flair_data_calc_CALC_MUT_NO_PIE.img build/flair_data_calc_CALC_MUT_NO_LABELS.img build/flair_data_calc_CALC_MUT_MERGE_SLICES.img build/flair_tenants_mut_film_busy_arrow.img build/.makefile.stamp`
```make
$(call film-pixel-mutant-leg,CALC_MUT_NO_PIE,PIE_DRAW)
$(call film-pixel-mutant-leg,CALC_MUT_NO_LABELS,PIE_LABELS)
$(call film-pixel-mutant-leg,CALC_MUT_MERGE_SLICES,PIE_FIVE)
$(call appl-boot,film_busy_arrow,$(BUILD)/flair_tenants_mut_film_busy_arrow.img,$(FLAIR_DATA_IMG),FLAIR_FILM_BUSY_SPEC,--screendump --screendump-after "FLAIR-CURSOR x=470 y=228" --quit-after "FLAIR-CURSOR x=470 y=228")
@grep -qx 'FLAIR-FILM-SCENE CALC ledger+chart' $(BUILD)/film_busy_arrow.serial || { printf 'FAIL cursor mutant did not launch Calc\n'; exit 1; }
@$(BUILD)/ppm_flair_calc_check $(BUILD)/film_busy_arrow.ppm $(CALC_GOLDEN) busy > $(BUILD)/film_busy_arrow.pixels 2>&1; rc=$$?; \
[ "$$rc" = 1 ] && grep -q 'BUSY_HOURGLASS' $(BUILD)/film_busy_arrow.pixels || { cat $(BUILD)/film_busy_arrow.pixels; exit 1; }
@printf '>>> test-flair-live-scene-mutant: CURSOR_MUT_BUSY_ARROW RED: %s\n' "$$(grep -m1 'BUSY_HOURGLASS' $(BUILD)/film_busy_arrow.pixels)"
@printf '>>> test-flair-live-scene-mutant: green (four named mutants RED on scene pixels)\n'
```

## test-flair-locked-purge

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30586):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/locked_trash.sh $(FLAIRTENANTS_IMG) $(FLAIR_DATA_IMG) '$(FLAIR_TRASHFULL_REGRESSION_SPEC)' _purge purge
```

## test-flair-locked-trash

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30584):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/locked_trash.sh $(FLAIRTENANTS_IMG) $(FLAIR_DATA_IMG) '$(FLAIR_LOCKED_TRASH_SPEC)'
```

## test-flair-locked-trash-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30588):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/flair_tenants_mut_locked_stage.img build/flair_tenants_mut_locked_notice.img build/flair_tenants_mut_locked_purge.img build/flair_data.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
@set -e; for pair in 'locked_stage:locked file left source:stage' 'locked_notice:locked notice pixels missing:stage' 'locked_purge:visible message missing:purge'; do \
  tag=$${pair%%:*}; rest=$${pair#*:}; why=$${rest%:*}; mode=$${rest##*:}; log=$(BUILD)/$$tag.log; \
  trace='$(FLAIR_LOCKED_TRASH_SPEC)'; [ $$mode = stage ] || trace='$(FLAIR_LOCKED_PURGE_SPEC)'; \
  if sh harness/diff/fat_diff/locked_trash.sh $(BUILD)/flair_tenants_mut_$$tag.img $(FLAIR_DATA_IMG) "$$trace" _mut_$$tag $$mode > $$log 2>&1; then echo "FAIL: $$tag mutant passed"; exit 1; fi; \
  grep -F "$$why" $$log || { cat $$log; exit 1; }; \
  printf 'VERDICT: PASS -- test-flair-locked-trash-mutant %s RED (%s)\n' "$$tag" "$$why"; \
done
```

## test-flair-mechanism-colorblind

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16113):
Prerequisites: `build/test_flair_mechanism_colorblind build/.makefile.stamp`
```make
@printf ">>> test-flair-mechanism-colorblind: stub the policy seam -> sentinel; assert EVERY chrome pixel == sentinel (no color in the mechanism)\n"
@$(TEST_COLORBLIND)
@printf ">>> test-flair-mechanism-colorblind: green\n"
```

## test-flair-mechanism-colorblind-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16117):
Prerequisites: `build/test_flair_mechanism_colorblind_mutant build/.makefile.stamp`
```make
@if $(TEST_COLORBLIND_MUT) >/dev/null 2>&1; then printf '!!! test-flair-mechanism-colorblind-mutant FAIL: a hardcoded color in the decoration was NOT caught -- the sentinel render is decoration\n'; exit 1; else printf '>>> test-flair-mechanism-colorblind-mutant: green (hardcoded decoration color correctly RED)\n'; fi
```

## test-flair-menu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 20042):
Prerequisites: `build/qemu_harness build/flair_live_nomodal.img build/ppm_flair_menu_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-menu : THE live WORKING MENUS (FO-8b)\n'
@printf '  Inject the locked menu trace -> WaitNextEvent pump -> inMenuBar band\n'
@printf '  -> flair_draw_menu_panel + present + track + MenuSelect. The System-7\n'
@printf '  "File" pull-down DROPS live; "Quit" (item 2) is chosen. beads initech-5l5z\n'
@printf '  FO-8b. ADR-0004 D-3 / ADR-0006 FO-8. Law 2/4.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (same flair_live scene, show_modal=0)\n' "$(FLAIRLIVE_NOMODAL_IMG)"
@printf 'Expecting : held DROP frame + FLAIR-MENU menu=128 item=2 (sel=0x00800002)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@# --quit-after the asserted final marker (bead initech-tdnl.33; the
@# initech-qed1 rule): the DROP-gated dump alone left the FLAIR-MENU line to
@# the legacy ~400 ms post-input quit, a host-timing race the slightly
@# heavier proportional glyph walk lost deterministically on a loaded host.
@# The assertion is unchanged; only the harness now waits for its marker.
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_NOMODAL_IMG)" --name "$(FLAIR_MENU_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_MENU_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-MENU-DROP" \
	--quit-after "FLAIR-MENU menu=" --timeout-ms 15000 \
	2> "$(FLAIR_MENU_REPORT)" || true
@cat "$(FLAIR_MENU_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(FLAIR_MENU_REPORT)"; then \
	printf '!!! test-flair-menu FAIL: TRIPLE FAULT in the pump/menu boot\n'; exit 1; \
fi
@printf '>>> test-flair-menu [1/4]: no triple-fault\n'
@grep -q '^FLAIR-LIVE-READY$$' "$(FLAIR_MENU_SERIAL)" \
	|| { printf '!!! test-flair-menu FAIL: FLAIR-LIVE-READY missing -- the pump never armed\n'; exit 1; }
@printf '>>> test-flair-menu [2/4]: FLAIR-LIVE-READY (the WaitNextEvent pump is armed)\n'
@grep -q '^FLAIR-MENU-DROP menu=128$$' "$(FLAIR_MENU_SERIAL)" \
	|| { printf '!!! test-flair-menu FAIL: FLAIR-MENU-DROP menu=128 missing -- the inMenuBar dispatch did not drop the panel\n'; grep '^FLAIR-' "$(FLAIR_MENU_SERIAL)" || true; exit 1; }
@grep -q '^FLAIR-MENU menu=128 item=2 (sel=0x00800002)$$' "$(FLAIR_MENU_SERIAL)" \
	|| { printf '!!! test-flair-menu FAIL: FLAIR-MENU menu=128 item=2 (sel=0x00800002) missing -- MenuSelect did not choose item 2 / wrong result word\n'; grep '^FLAIR-' "$(FLAIR_MENU_SERIAL)" || true; exit 1; }
@printf '>>> test-flair-menu [3/4]: FLAIR-MENU-DROP + FLAIR-MENU menu=128 item=2 (sel=0x00800002) (the pump dropped + selected)\n'
@if [ ! -s "$(FLAIR_MENU_PPM)" ]; then printf '!!! test-flair-menu FAIL: no screendump captured at %s\n' "$(FLAIR_MENU_PPM)"; exit 1; fi
@$(PPM_FLAIR_MENU_CHECK_BIN) "$(FLAIR_MENU_PPM)" \
	|| { printf '!!! test-flair-menu FAIL: the DROP screendump does not show the held pull-down (black frame + unhilited BTNFACE body) where bare teal was\n'; exit 1; }
@printf '>>> test-flair-menu [4/4]: DROP screendump == held panel (frame + unhilited BTNFACE rows) over the previously-teal desktop\n'
@printf 'VERDICT   : PASS -- the booted FLAIR desktop has WORKING MENUS (Law 4)\n'
@printf '======================================================================\n'
```

## test-flair-menu-crossdrag

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 20150):
Prerequisites: `build/qemu_harness build/flair_live_nomodal.img build/ppm_flair_menu_crossdrag_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-menu-crossdrag : cross-menu drag live redraw (initech-9op1)\n'
@printf '  Inject the locked cross-menu trace -> WaitNextEvent pump -> inMenuBar band\n'
@printf '  -> click File -> drag sideways onto Edit -> release. The ON-SCREEN drop\n'
@printf '  must follow the drag (File erased, Edit dropped), not just MenuSelect.\n'
@printf '  beads initech-9op1 (found during initech-rl4v). Law 2/4.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (same flair_live scene, show_modal=0)\n' "$(FLAIRLIVE_NOMODAL_IMG)"
@printf 'Expecting : DROP menu=128 -> XDROP menu=129 -> final sel=0 cancel\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_NOMODAL_IMG)" --name "$(FLAIR_MENU_CROSSDRAG_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_MENU_CROSSDRAG_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-MENU-XDROP" --timeout-ms 15000 \
	2> "$(FLAIR_MENU_CROSSDRAG_REPORT)" || true
@cat "$(FLAIR_MENU_CROSSDRAG_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(FLAIR_MENU_CROSSDRAG_REPORT)"; then \
	printf '!!! test-flair-menu-crossdrag FAIL: TRIPLE FAULT in the pump/menu boot\n'; exit 1; \
fi
@printf '>>> test-flair-menu-crossdrag [1/4]: no triple-fault\n'
@grep -q '^FLAIR-LIVE-READY$$' "$(FLAIR_MENU_CROSSDRAG_SERIAL)" \
	|| { printf '!!! test-flair-menu-crossdrag FAIL: FLAIR-LIVE-READY missing -- the pump never armed\n'; exit 1; }
@printf '>>> test-flair-menu-crossdrag [2/4]: FLAIR-LIVE-READY (the WaitNextEvent pump is armed)\n'
@grep -q '^FLAIR-MENU-DROP menu=128$$' "$(FLAIR_MENU_CROSSDRAG_SERIAL)" \
	|| { printf '!!! test-flair-menu-crossdrag FAIL: FLAIR-MENU-DROP menu=128 missing -- the inMenuBar dispatch did not drop File\n'; grep '^FLAIR-' "$(FLAIR_MENU_CROSSDRAG_SERIAL)" || true; exit 1; }
@grep -q '^FLAIR-MENU-XDROP menu=129$$' "$(FLAIR_MENU_CROSSDRAG_SERIAL)" \
	|| { printf '!!! test-flair-menu-crossdrag FAIL: FLAIR-MENU-XDROP menu=129 missing -- the held drop did not redraw on Edit\n'; grep '^FLAIR-' "$(FLAIR_MENU_CROSSDRAG_SERIAL)" || true; exit 1; }
@grep -q '^FLAIR-MENU menu=128 item=0 (sel=0x00000000)$$' "$(FLAIR_MENU_CROSSDRAG_SERIAL)" \
	|| { printf '!!! test-flair-menu-crossdrag FAIL: FLAIR-MENU menu=128 item=0 (sel=0x00000000) missing -- release-on-title did not yield sel=0\n'; grep '^FLAIR-' "$(FLAIR_MENU_CROSSDRAG_SERIAL)" || true; exit 1; }
@printf '>>> test-flair-menu-crossdrag [3/4]: DROP File + XDROP Edit mid-track + final sel=0 on Edit'"'"'s title\n'
@if [ ! -s "$(FLAIR_MENU_CROSSDRAG_PPM)" ]; then printf '!!! test-flair-menu-crossdrag FAIL: no screendump captured at %s\n' "$(FLAIR_MENU_CROSSDRAG_PPM)"; exit 1; fi
@$(PPM_FLAIR_MENU_CROSSDRAG_CHECK_BIN) "$(FLAIR_MENU_CROSSDRAG_PPM)" \
	|| { printf '!!! test-flair-menu-crossdrag FAIL: the screendump does not show File erased + Edit dropped (the live on-screen menu lags the drag; initech-9op1)\n'; exit 1; }
@printf '>>> test-flair-menu-crossdrag [4/4]: screendump == File erased (bare teal) + Edit dropped (frame + BTNFACE body)\n'
@printf 'VERDICT   : PASS -- the live menu drop FOLLOWS a cross-menu drag (Law 4; initech-9op1)\n'
@printf '======================================================================\n'
```

## test-flair-menu-crossdrag-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 20195):
Prerequisites: `build/qemu_harness build/flair_live_nomodal_mut_norehit.img build/ppm_flair_menu_crossdrag_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-menu-crossdrag-mutant : Rule 6 (the gate BITES)\n'
@printf '  Mutant: -DKMAIN_MUT_MENU_NO_REHIT (flair_live_do_menu freezes mi at the click).\n'
@printf '  Expect: File never switches to Edit -> no XDROP marker/dump -> RED.\n'
@printf '  The direct initech-9op1 frozen-mi mutant.\n'
@printf '======================================================================\n'
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_NOMODAL_MUT_NOREHIT_IMG)" --name flair_menu_crossdrag_mut --out "$(BUILD)" \
	--mouse "$(FLAIR_MENU_CROSSDRAG_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-MENU-XDROP" --timeout-ms 15000 >/dev/null 2>&1 || true
@grep -q '^FLAIR-LIVE-READY$$' "$(BUILD)/flair_menu_crossdrag_mut.serial" \
	|| { printf '!!! test-flair-menu-crossdrag-mutant: mutant did not reach FLAIR-LIVE-READY (not comparable)\n'; exit 1; }
@if grep -q '^FLAIR-MENU-XDROP ' "$(BUILD)/flair_menu_crossdrag_mut.serial"; then printf '!!! test-flair-menu-crossdrag-mutant FAIL: frozen-mi unexpectedly emitted XDROP\n'; exit 1; fi
@if [ -s "$(BUILD)/flair_menu_crossdrag_mut.ppm" ]; then printf '!!! test-flair-menu-crossdrag-mutant FAIL: an XDROP-gated dump exists without XDROP -- harness contract broken\n'; exit 1; fi
@printf '>>> test-flair-menu-crossdrag-mutant: RED as required -- XDROP marker and dump are absent; the crossdrag gate fails loud (Rule 6, initech-9op1)\n'
@printf '======================================================================\n'
```

## test-flair-menu-long

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30367):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_hold.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-menu-long : a long held menu gesture runs the visible item (H01)\n'
@printf '  bead initech-tdnl.71; trace FLAIR_MENU_LONG_SPEC (spec/flair_input_traces.mk)\n'
@printf '======================================================================\n'
$(call held-boot,$(FLAIRTENANTS_HOLD_IMG),$(FLAIR_MENU_LONG_NAME),FLAIR_MENU_LONG_SPEC,FINDER-CMD id=)
@$(call menu-long-check,"$(BUILD)/$(FLAIR_MENU_LONG_NAME).serial") || { printf '!!! test-flair-menu-long FAIL: want >= 70 cursor samples before the View switch, then FLAIR-MENU ... (sel=0x02020001) + FINDER-CMD id=14 VIEW_ICONS and no New Folder\n'; grep -E '^(FINDER-|FLAIR-MENU|FLAIR-TRACK)' "$(BUILD)/$(FLAIR_MENU_LONG_NAME).serial"; exit 1; }
@printf '>>> test-flair-menu-long: %s cursor samples held on File; %s ; %s\n' "$$(awk '/^FLAIR-MENU-DROP menu=512$$/{c=0;d=1} d&&/^FLAIR-CURSOR /{c++} /^FLAIR-MENU-XDROP menu=514$$/{if(d)n=c;d=0} END{print n+0}' $(BUILD)/$(FLAIR_MENU_LONG_NAME).serial)" "$$(grep -m1 '^FLAIR-MENU menu=' $(BUILD)/$(FLAIR_MENU_LONG_NAME).serial)" "$$(grep -m1 '^FINDER-CMD ' $(BUILD)/$(FLAIR_MENU_LONG_NAME).serial)"
@printf '>>> test-flair-menu-long: green\n'
```

## test-flair-menu-long-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30377):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_trackcap.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-menu-long-mutant : Rule 6\n'
@printf '  Mutant: menu.o -DMENU_MUT_TRACK_CAP=1 (the pre-tdnl.71 64-point history)\n'
@printf '======================================================================\n'
$(call held-boot,$(BUILD)/flair_tenants_mut_trackcap.img,flair_menu_long_mut,FLAIR_MENU_LONG_SPEC,FINDER-CMD id=)
@grep -qxF 'FLAIR-MENU-DROP menu=512' "$(BUILD)/flair_menu_long_mut.serial" && grep -q '^FLAIR-MENU menu=512 ' "$(BUILD)/flair_menu_long_mut.serial" || { printf '!!! test-flair-menu-long-mutant: the mutant never dropped File and released (not comparable)\n'; exit 1; }
@if $(call menu-long-check,"$(BUILD)/flair_menu_long_mut.serial"); then printf '!!! test-flair-menu-long-mutant FAIL: the 64-point history PASSED -- the gate is decoration\n'; exit 1; fi
@printf '>>> test-flair-menu-long-mutant: RED as required -- %s ; %s\n' "$$(grep -m1 '^FLAIR-MENU menu=' $(BUILD)/flair_menu_long_mut.serial)" "$$(grep -m1 '^FINDER-CMD ' $(BUILD)/flair_menu_long_mut.serial || echo 'no command')"
```

## test-flair-menu-look

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33662):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_menu_look_gate.img build/flair_data.img build/ppm_flair_menu_look_check build/.makefile.stamp`
```make
$(call menu-look-boot,menu_look_file,$(BUILD)/flair_tenants_mut_menu_look_gate.img,FLAIR-MENU-DROP menu=512)
@$(MENU_LOOK_CHECK) file $(BUILD)/menu_look_file.ppm
$(call menu-look-boot,menu_look_view,$(BUILD)/flair_tenants_mut_menu_look_gate.img,FLAIR-MENU-XDROP menu=514)
@$(MENU_LOOK_CHECK) view $(BUILD)/menu_look_view.ppm
@printf '>>> test-flair-menu-look: green (File dark band/white ink/item20/4/shortcut14/whole divider rows; View NFNT check18)\n'
```

## test-flair-menu-look-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33674):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_menu_look_flat.img build/flair_tenants_mut_menu_old_divider_inset.img build/flair_data.img build/ppm_flair_menu_look_check build/.makefile.stamp`
```make
$(call menu-look-boot,menu_look_mut,$(BUILD)/flair_tenants_mut_menu_look_flat.img,FLAIR-MENU-DROP menu=512)
@if $(MENU_LOOK_CHECK) file $(BUILD)/menu_look_mut.ppm > $(BUILD)/menu_look_mut.pixels 2>&1; then printf 'FAIL flat hilite survived\n'; exit 1; fi
@grep -q 'FAIL menu-look ML1 title band' $(BUILD)/menu_look_mut.pixels
@printf '>>> test-flair-menu-look-mutant: MENU_LOOK_MUT_FLAT_HILITE correctly RED (ML1 title band)\n'
$(call menu-look-boot,menu_look_mut_divider,$(BUILD)/flair_tenants_mut_menu_old_divider_inset.img,FLAIR-MENU-DROP menu=512)
@if $(MENU_LOOK_CHECK) file $(BUILD)/menu_look_mut_divider.ppm > $(BUILD)/menu_look_mut_divider.pixels 2>&1; then printf 'FAIL old divider inset survived\n'; exit 1; fi
@grep -q 'FAIL menu-look ML10 whole divider rows' $(BUILD)/menu_look_mut_divider.pixels
@! grep -Eq 'FAIL menu-look (ML1 title|ML4|ML9)' $(BUILD)/menu_look_mut_divider.pixels
@printf '>>> test-flair-menu-look-mutant: MENU_LOOK_MUT_OLD_DIVIDER_INSET correctly RED (ML10 whole divider rows)\n'
```

## test-flair-menu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 20090):
Prerequisites: `build/qemu_harness build/flair_live_nomodal_mut_menu.img build/ppm_flair_menu_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-menu-mutant : Rule 6 (the gate BITES)\n'
@printf '  Mutant: -DFLAIR_LIVE_MUTATE_MENU_NOOP (the inMenuBar dispatch drops no panel).\n'
@printf '  Expect: no DROP marker -> no marker-gated dump -> the real gate fails loud.\n'
@printf '  The direct HER-14 "menus do not work" mutant.\n'
@printf '======================================================================\n'
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_NOMODAL_MUT_MENU_IMG)" --name flair_menu_mut --out "$(BUILD)" \
	--mouse "$(FLAIR_MENU_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-MENU-DROP" --timeout-ms 15000 >/dev/null 2>&1 || true
@grep -q '^FLAIR-LIVE-READY$$' "$(BUILD)/flair_menu_mut.serial" \
	|| { printf '!!! test-flair-menu-mutant: mutant did not reach FLAIR-LIVE-READY (not comparable)\n'; exit 1; }
@if grep -q '^FLAIR-MENU-DROP ' "$(BUILD)/flair_menu_mut.serial"; then printf '!!! test-flair-menu-mutant FAIL: the menu-noop mutant unexpectedly emitted DROP\n'; exit 1; fi
@if [ -s "$(BUILD)/flair_menu_mut.ppm" ]; then printf '!!! test-flair-menu-mutant FAIL: a DROP-gated screendump exists without a DROP marker -- harness contract broken\n'; exit 1; fi
@printf '>>> test-flair-menu-mutant: RED as required -- DROP marker and dump are both absent; test-flair-menu fails loud on its missing DROP/dump checks (Rule 6, HER-14)\n'
@printf '======================================================================\n'
```

## test-flair-menubar-clock

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34357):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_menubar_clock_gate.img build/flair_data.img build/ppm_flair_menubar_clock_check build/.makefile.stamp`
```make
$(call menubar-clock-boot,menubar_clock,$(BUILD)/flair_tenants_mut_menubar_clock_gate.img)
@$(MENUBAR_CLOCK_CHECK) $(BUILD)/menubar_clock.ppm "$(MENUBAR_CLOCK_PNG)" "$(MENUBAR_CLOCK_NFNT)"
@grep -qx 'FLAIR-CLOCK 12:55 am' $(BUILD)/menubar_clock.serial
@grep -qx 'FLAIR-CLOCK 12:56 am' $(BUILD)/menubar_clock.serial
@test "$$(grep -c '^FLAIR-CLOCK ' $(BUILD)/menubar_clock.serial)" -eq 2
@printf '>>> test-flair-menubar-clock: green (pinned RTC crossed a minute, exactly two paints, independent1600-pixel golden)\n'
```

## test-flair-menubar-clock-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 109):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_menubar_clock_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-menubar-clock-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_menubar_clock_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-menubar-clock-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-menubar-clock_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_menubar_clock_gate.img" --disk2 "$(BUILD)/flair-menubar-clock_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-menubar-clock_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-menubar-clock_bochs.report.txt" || true
@cat "$(BUILD)/flair-menubar-clock_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-menubar-clock-bochs "$(BUILD)/flair-menubar-clock_bochs.report.txt" "$(BUILD)/flair-menubar-clock_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-menubar-clock-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34374):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_clock_wrong_hour.img build/flair_tenants_mut_clock_stuck.img build/flair_data.img build/ppm_flair_menubar_clock_check build/.makefile.stamp`
```make
$(call menubar-clock-mutant,wrong_hour,CLOCK_WRONG_HOUR)
$(call menubar-clock-mutant,stuck,CLOCK_STUCK)
@grep -qx 'FLAIR-CLOCK 12:55 am' $(BUILD)/menubar_clock_stuck.serial
@printf '>>> test-flair-menubar-clock-mutant: green (wrong-hour and stale-minute regressions rejected)\n'
```

## test-flair-modifier-release

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30215):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-modifier-release : released modifiers stay released (G11)\n'
@printf '  bead initech-tdnl.58; trace FLAIR_MODREL_SPEC (spec/flair_input_traces.mk)\n'
@printf '======================================================================\n'
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_MODREL_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_MODREL_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_MODREL_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_MODREL_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_MODREL_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--quit-after "FINDER-NEW-FOLDER name=NEWFOLD" \
	--timeout-ms 30000 2> "$(BUILD)/$(FLAIR_MODREL_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_MODREL_NAME).report"; then printf '!!! test-flair-modifier-release FAIL: TRIPLE FAULT\n'; exit 1; fi
@$(call modrel-check,"$(BUILD)/$(FLAIR_MODREL_NAME).serial") || { \
	printf '!!! test-flair-modifier-release FAIL: a released modifier still acted (G11). Want: root open; exactly ONE FINDER-CMD (the final Ctrl-N, sel=1); no FINDER-CLOSE-WINDOW; APPS click count=1; no release byte cooked as keyDown\n'; \
	grep -E '^(FINDER-|FLAIR-EVT what=[34])' "$(BUILD)/$(FLAIR_MODREL_NAME).serial"; exit 1; }
@printf '>>> test-flair-modifier-release: plain n / w after Ctrl / RCtrl release did nothing; Shift release ended extension (count=1); the Ctrl-N chord still dispatched\n'
@printf '>>> test-flair-modifier-release: green\n'
```

## test-flair-modifier-release-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30235):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_set1.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-modifier-release-mutant : Rule 6\n'
@printf '  Mutant: event.o -DEVENT_MUTATE_SET1_BREAK_BLIND (the pre-tdnl.58 decoder)\n'
@printf '======================================================================\n'
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_MODREL_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_MODREL_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_set1.img" --disk2 "$(FLAIR_MODREL_DATA)" \
	--expect FLAIR-FAT-MOUNT-OK --name flair_modrel_mut --out "$(BUILD)" \
	--mouse "$(FLAIR_MODREL_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--quit-after FLAIR-LIVE-OK --timeout-ms 30000 2> "$(BUILD)/flair_modrel_mut.report" || true
@grep -qxF 'FINDER-OPEN-VOLUME win=0 n=4' "$(BUILD)/flair_modrel_mut.serial" \
	|| { printf '!!! test-flair-modifier-release-mutant: the mutant never opened the root window (not comparable)\n'; exit 1; }
@if $(call modrel-check,"$(BUILD)/flair_modrel_mut.serial"); then \
	printf '!!! test-flair-modifier-release-mutant FAIL: the pre-fix decoder PASSED -- the gate is decoration\n'; exit 1; fi
@printf '>>> test-flair-modifier-release-mutant: RED as required -- %s\n' "$$(grep -m1 '^FINDER-CMD ' "$(BUILD)/flair_modrel_mut.serial" || echo 'no FINDER-CMD')"
```

## test-flair-mouse

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 11681):
Prerequisites: `build/qemu_harness build/flair_live.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-mouse : FLAIR MOUSE IRQ12 lane (FO-6)\n'
@printf '  THE minefield: IRQ12 = slave 8259A IR4 (vec 0x34); dual-PIC EOI\n'
@printf '  (slave 0xA0 THEN master 0x20). cursor advance + 2 cooked events = no wedge.\n'
@printf '  beads initech-5l5z FO-6/7; ADR-0006 E-D3b/BC-3. Law 2, Rule 2/5/6/11.\n'
@printf '======================================================================\n'
@# initech-qed1: quit-after the LAST cooked marker this gate requires
@# (mouseUp) -- the dual-PIC-EOI cook chain is IRQ-driven, not instant.
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_IMG)" --name "$(FLAIR_MOUSE_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_MOUSE_SPEC)" --keys-after "FLAIR-LIVE-READY" --timeout-ms 15000 \
	--quit-after "FLAIR-EVT what=2 " \
	2> "$(FLAIR_MOUSE_REPORT)" || true
@cat "$(FLAIR_MOUSE_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(FLAIR_MOUSE_REPORT)"; then \
	printf '!!! test-flair-mouse FAIL: TRIPLE FAULT in the mouse IRQ12 boot\n'; exit 1; \
fi
@printf '>>> test-flair-mouse [1/3]: no triple-fault\n'
@if [ ! -s "$(FLAIR_MOUSE_SERIAL)" ]; then printf '!!! test-flair-mouse FAIL: no serial captured\n'; exit 1; fi
@grep -q '^FLAIR-HOOK-SET$$' "$(FLAIR_MOUSE_SERIAL)" \
	|| { printf '!!! test-flair-mouse FAIL: FLAIR-HOOK-SET missing -- mouse_init/IRQ12-gate path not reached\n'; exit 1; }
@# THE no-wedge + cook proof: the spec moves the cursor (+20 right, +20 DOWN
@# in screen coords -> 340,260 from the 320,240 center; harness m-dy is
@# screen-down-positive, the producer flips PS/2 wire dy per the LOCKED
@# event_model.h Sec 5 contract -- rebaselined by initech-rgt8: the old
@# 340,220 golden had BAKED IN the producer inversion) THEN clicks.
@# WaitNextEvent cooks
@# the moves into the cursor (mouse moves => nullEvent, the cursor still
@# advances) and delivers the mouseDown at the ADVANCED cursor. That can only
@# happen if MULTIPLE IRQ12 packets fired (each move = 3 bytes) -- i.e. the
@# dual-PIC EOI re-armed the slave 8259A after every byte (no wedge).
@grep -q '^FLAIR-EVT what=1 where=340,260 ' "$(FLAIR_MOUSE_SERIAL)" \
	|| { printf '!!! test-flair-mouse FAIL: cooked mouseDown at the ADVANCED cursor (FLAIR-EVT what=1 where=340,260) not on serial -- the injected moves did not cook into the cursor AND/OR the dual-PIC EOI WEDGED (no IRQ12 re-arm)\n'; grep '^FLAIR-EVT ' "$(FLAIR_MOUSE_SERIAL)" || true; exit 1; }
@printf '>>> test-flair-mouse [2/3]: cooked mouseDown where=340,260 -- moves cooked into the cursor; multiple IRQ12 packets fired (dual-PIC EOI did NOT wedge)\n'
@grep -q '^FLAIR-EVT what=2 ' "$(FLAIR_MOUSE_SERIAL)" \
	|| { printf '!!! test-flair-mouse FAIL: cooked mouseUp (FLAIR-EVT what=2) not on serial -- the button-up IRQ12 packet did not fire\n'; grep '^FLAIR-EVT ' "$(FLAIR_MOUSE_SERIAL)" || true; exit 1; }
@printf '>>> test-flair-mouse [3/3]: cooked mouseUp (a further IRQ12 packet) -- WaitNextEvent cook path LIVE\n'
@printf 'VERDICT   : PASS -- mouse IRQ12 -> packet -> dual-PIC EOI -> WaitNextEvent cook -> FLAIR-EVT LIVE (FO-6/7)\n'
@printf '======================================================================\n'
```

## test-flair-mouse-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 11870):
Prerequisites: `build/qemu_harness build/flair_live_mut_mouse.img build/flair_live_mut_eoi.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-mouse-mutant : Rule 6 (the gate BITES)\n'
@printf '======================================================================\n'
@# Mutant 1: no mouse hook (-DFLAIR_LIVE_MUTATE_NO_MOUSE_HOOK) -> no cooked mouse event.
@# initech-qed1: --legacy-quit -- this leg asserts ABSENCE (no hook means the
@# cooked event can never appear); no marker exists to wait on for an event
@# that must not occur, and a longer wait cannot manufacture it.
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_MUT_MOUSE_IMG)" --name flair_mouse_mut_nohook --out "$(BUILD)" \
	--mouse "$(FLAIR_MOUSE_SPEC)" --keys-after "FLAIR-LIVE-READY" --timeout-ms 15000 --legacy-quit >/dev/null 2>&1 || true
@grep -q '^FLAIR-LIVE-READY$$' "$(BUILD)/flair_mouse_mut_nohook.serial" \
	|| { printf '!!! mutant1 did not reach FLAIR-LIVE-READY (not comparable)\n'; exit 1; }
@if grep -q '^FLAIR-EVT what=1 ' "$(BUILD)/flair_mouse_mut_nohook.serial"; then \
	printf '!!! test-flair-mouse-mutant FAIL: no-mouse-hook mutant STILL cooked a mouseDown (FLAIR-EVT what=1) -- the gate is decoration\n'; exit 1; \
fi
@printf '>>> mutant1 (NO_MOUSE_HOOK): RED as required -- no cooked mouseDown (the gate bites)\n'
@# Mutant 2: master-only EOI (-DFLAIR_LIVE_MUTATE_MASTER_ONLY_EOI) -> slave wedges
@# after the FIRST IRQ12, so the move packets never complete -> the cursor never
@# advances to 340,260 -> no cooked mouseDown at the advanced position.
@# initech-qed1: --legacy-quit -- same absence-check reasoning as mutant 1
@# (the slave-wedge means the cursor never reaches the advanced position).
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_MUT_EOI_IMG)" --name flair_mouse_mut_eoi --out "$(BUILD)" \
	--mouse "$(FLAIR_MOUSE_SPEC)" --keys-after "FLAIR-LIVE-READY" --timeout-ms 15000 --legacy-quit >/dev/null 2>&1 || true
@grep -q '^FLAIR-LIVE-READY$$' "$(BUILD)/flair_mouse_mut_eoi.serial" \
	|| { printf '!!! mutant2 did not reach FLAIR-LIVE-READY (not comparable)\n'; exit 1; }
@if grep -q '^FLAIR-EVT what=1 where=340,260 ' "$(BUILD)/flair_mouse_mut_eoi.serial"; then \
	printf '!!! test-flair-mouse-mutant FAIL: master-only-EOI mutant reached the ADVANCED cursor (FLAIR-EVT what=1 where=340,260) -- the no-wedge gate is decoration (the slave did NOT wedge)\n'; exit 1; \
fi
@printf '>>> mutant2 (MASTER_ONLY_EOI): RED as required -- cursor never advanced to 340,260 (slave 8259A wedged after the first IRQ12)\n'
@printf 'VERDICT   : PASS -- both FO-6 mutants BITE (Rule 6)\n'
@printf '======================================================================\n'
```

## test-flair-part-map

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r5.mk', line 64):
Prerequisites: `build/.makefile.stamp | build/ksize5`
```make
$(CC) $(CFLAGS) $(PART_MAP_INC) $(PART_MAP_LINK) -o $(KERNEL_R5_DIR)/part-map
@$(KERNEL_R5_DIR)/part-map >$(KERNEL_R5_DIR)/part-map.txt
@diff -u harness/proptest/fixtures/flair_part_map.golden $(KERNEL_R5_DIR)/part-map.txt >$(KERNEL_R5_DIR)/part-map.diff || { \
	head -20 $(KERNEL_R5_DIR)/part-map.diff; printf '!!! flair-part-map FAIL: PART resolution differs from the pre-change table\n'; exit 1; }
@printf '>>> test-flair-part-map: green (%s PART x depth x skin resolutions == the pre-change table)\n' "$$(wc -l < $(KERNEL_R5_DIR)/part-map.txt)"
```

## test-flair-part-map-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r5.mk', line 70):
Prerequisites: `test-flair-part-map build/.makefile.stamp`
```make
$(CC) $(CFLAGS) -DFLAIR_PART_MAP_MUT_DERIVED $(PART_MAP_INC) $(PART_MAP_LINK) -o $(KERNEL_R5_DIR)/part-map-mut
@$(KERNEL_R5_DIR)/part-map-mut >$(KERNEL_R5_DIR)/part-map-mut.txt
@if cmp -s harness/proptest/fixtures/flair_part_map.golden $(KERNEL_R5_DIR)/part-map-mut.txt; then \
	printf '!!! test-flair-part-map-mutant FAIL: part_map_derived survived\n'; exit 1; fi
@printf '!!! flair-part-map FAIL: part_map_derived changes the resolution table\n'
@printf '>>> test-flair-part-map-mutant: green (part_map_derived RED)\n'
```

## test-flair-power-notice

Declared tier: EMU; #  recipe to execute (from 'harness/emu/calcda_power.mk', line 87):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_calcda_gate.img build/flair_data.img build/ppm_flair_power_notice_check build/.makefile.stamp`
```make
$(call calc-text-cancel-boot,power_text_notice,$(FLAIR_CALCDA_GATE_IMG),$(CALC_POWER_TEXT_HELD),FINDER-POWER-NOTICE,FINDER-POWER-NOTICE)
@grep -qxF 'FINDER-POWER-CANCEL app=TEXT.EXE declined' $(BUILD)/power_text_notice.serial
@$(PPM_POWER_NOTICE) $(BUILD)/power_text_notice.ppm
$(call calc-text-cancel-boot,power_text_cancel,$(FLAIR_CALCDA_GATE_IMG),$(CALC_POWER_TEXT_SD),FINDER-NEW-FOLDER name=)
@sh harness/emu/power_notice_check.sh $(BUILD)/power_text_cancel.serial
@fsck.fat -n $(BUILD)/power_text_cancel_data.img > $(BUILD)/power_text_cancel.fsck
```

## test-flair-power-notice-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/calcda_power.mk', line 97):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_power_silent_cancel.img build/flair_data.img build/ppm_flair_power_notice_check build/.makefile.stamp`
```make
$(call calc-text-cancel-boot,power_text_silent,$(BUILD)/flair_tenants_mut_power_silent_cancel.img,$(CALC_POWER_TEXT_SD),FINDER-NEW-FOLDER name=,FINDER-NEW-FOLDER name=)
@if sh harness/emu/power_notice_check.sh $(BUILD)/power_text_silent.serial > $(BUILD)/power_text_silent.check; then echo 'FAIL silent cancellation survived emulator'; exit 1; fi
@grep -qxF 'FAIL power-notice: no visible explanation' $(BUILD)/power_text_silent.check
@if $(PPM_POWER_NOTICE) $(BUILD)/power_text_silent.ppm > $(BUILD)/power_text_silent.pixels 2>&1; then echo 'FAIL absent notice survived pixels'; exit 1; fi
@grep -qxF 'FAIL power-notice pixels' $(BUILD)/power_text_silent.pixels
@echo 'GREEN power-notice: serial-only cancellation mutant RED'
```

## test-flair-restart

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22611):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-restart : Special > Restart and the shut-down screen Restart button reboot the machine with the desktop saved (tdnl.78)\n'
@printf '======================================================================\n'
$(call pw-reboot,$(FLAIRTENANTS_IMG),$(FLAIR_RS_NAME),$(BUILD)/$(FLAIR_RS_NAME)_data.img,$(FLAIR_RESTART_SPEC))
@( $(call rs-check,$(BUILD)/$(FLAIR_RS_NAME).serial,FINDER-CMD id=12 name=RESTART src=mouse sel=0,Special > Restart) ) || { printf '!!! test-flair-restart FAIL: [1] serial (above)\n'; grep -E '^(FINDER-|FLAIR-MENU|FLAIR-RESTART|FLAIR-LIVE-READY|DESKTOP-DB|PANIC)' $(BUILD)/$(FLAIR_RS_NAME).serial; exit 1; }
@grep -qxF 'FLAIR-MENU menu=515 item=6 (sel=0x02030006)' $(BUILD)/$(FLAIR_RS_NAME).serial || { printf '!!! test-flair-restart FAIL: [1] the release did not select Special > Restart\n'; exit 1; }
@! grep -q '^FLAIR-SHUTDOWN' $(BUILD)/$(FLAIR_RS_NAME).serial || { printf '!!! test-flair-restart FAIL: [1] Restart went through the shut-down screen\n'; exit 1; }
@( $(call pw-disk-check,$(BUILD)/$(FLAIR_RS_NAME)_data.img) ) || { printf '!!! test-flair-restart FAIL: [1] the disk (above)\n'; exit 1; }
@printf '>>> test-flair-restart [1/2]: Special > Restart -- DESKTOP-DB-SAVE n=3 -> FLAIR-RESTART -> reboot -> DESKTOP-DB-OK + DESKTOP-DB-VIEWS n=1 -> FLAIR-LIVE-READY again\n'
$(call pw-reboot,$(FLAIRTENANTS_IMG),$(FLAIR_SDR_NAME),$(BUILD)/$(FLAIR_SDR_NAME)_data.img,$(FLAIR_SHUTDOWN_RESTART_SPEC))
@( $(call rs-check,$(BUILD)/$(FLAIR_SDR_NAME).serial,FINDER-CMD id=13 name=SHUTDOWN src=mouse sel=0,Shut Down) ) || { printf '!!! test-flair-restart FAIL: [2] serial (above)\n'; grep -E '^(FINDER-|FLAIR-MENU|FLAIR-SHUTDOWN|FLAIR-RESTART|FLAIR-LIVE-READY|DESKTOP-DB|PANIC)' $(BUILD)/$(FLAIR_SDR_NAME).serial; exit 1; }
@sed -n '/^FLAIR-SHUTDOWN-SAFE$$/,/^FLAIR-RESTART$$/p' $(BUILD)/$(FLAIR_SDR_NAME).serial | grep -qxF 'FLAIR-RESTART' || { printf '!!! test-flair-restart FAIL: [2] the Restart did not come from the shut-down screen (no FLAIR-SHUTDOWN-SAFE before FLAIR-RESTART)\n'; exit 1; }
@printf '>>> test-flair-restart [2/2]: Shut Down -> the safe screen -> its Restart button -> reboot with the view saved\n'
@printf '>>> test-flair-restart: green\n'
```

## test-flair-restart-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 132):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img test-bochs-reboot build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-restart-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-restart-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-restart_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-restart_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-restart_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-restart_bochs.report.txt" || true
@cat "$(BUILD)/flair-restart_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-restart-bochs "$(BUILD)/flair-restart_bochs.report.txt" "$(BUILD)/flair-restart_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-restart-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22627):
Prerequisites: `build/qemu_harness build/flair_data.img build/flair_tenants_mut_no_pulse.img build/.makefile.stamp`
```make
$(call pw-reboot,$(BUILD)/flair_tenants_mut_no_pulse.img,flair_restart_mut_nopulse,$(BUILD)/flair_restart_mut_nopulse_data.img,$(FLAIR_RESTART_SPEC))
@out=$$( ( $(call rs-check,$(BUILD)/flair_restart_mut_nopulse.serial,FINDER-CMD id=12 name=RESTART src=mouse sel=0,Special > Restart) ) ); rc=$$?; \
$(call pw-mut-verdict,NO_PULSE,$$out,$$rc,PANIC kernel_reset: the 8042 pulse did not reset,test-flair-restart-mutant)
```

## test-flair-routing

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15087):
Prerequisites: `build/test_flair_routing build/.makefile.stamp`
```make
@printf ">>> test-flair-routing: the R3.7 host routing suite over disk_tenant_event + the clipped title run + the INT 81h dispatcher (initech-2svz)\n"
@$(TEST_FLAIR_ROUTING)
@printf ">>> test-flair-routing: green\n"
```

## test-flair-routing-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15094):
Prerequisites: `build/test_flair_routing_mutant_PROC_MUT_ADMIT_IS_FOREGROUND build/test_flair_routing_mutant_CHROME_MUT_TITLE_UNCLIPPED build/test_flair_routing_mutant_TBX_MUT_AX_FULL_EAX build/test_flair_routing_mutant_TBX_MUT_ARG_OFFSET build/test_flair_routing_mutant_FLAIR_MUT_PLAIN_BSL_LAUNCH build/.makefile.stamp`
```make
@for pair in 'PROC_MUT_ADMIT_IS_FOREGROUND:leg(A1): a tenant that REGISTERs without a window does NOT become the foreground' \
             'CHROME_MUT_TITLE_UNCLIPPED:leg(T1): the covered title run writes NO pixel inside the front window' \
             'TBX_MUT_AX_FULL_EAX:leg(X2): AX is the LOW WORD of EAX' \
             'TBX_MUT_ARG_OFFSET:leg(X3): with the frame and the stack above the args POISONED' \
             'FLAIR_MUT_PLAIN_BSL_LAUNCH:leg(L1): plain backslash and non-launch modifiers reach tenant'; do \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_flair_routing_mutant_$$m; \
	if $$bin > $$bin.log 2>&1; then printf '!!! test-flair-routing-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
	line=$$(grep -F "$$why" $$bin.log | grep FAIL) || { printf '!!! test-flair-routing-mutant FAIL: %s went RED for the wrong reason\n' "$$m"; cat $$bin.log; exit 1; }; \
	printf '>>> test-flair-routing-mutant: %s correctly RED:\n    %s\n' "$$m" "$$line"; \
done
@printf '>>> test-flair-routing-mutant: green (all five mutants RED for the named reason)\n'
```

## test-flair-scroll

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22108):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/flair_scroll_data.img build/ppm_flair_disk_windows_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-scroll : the scroll bars work; a bar click never moves the window (tdnl.35)\n'
@printf '  Ref: audit 2026-10-03 F02; os/flair/winscroll.h; spec/flair_scroll_traces.mk.\n'
@printf '======================================================================\n'
@# ---- [1] ARROW ----
$(call scroll-boot,$(FLAIRTENANTS_IMG),flair_scroll_arrow,$(FLAIR_SCROLL_DATA_IMG),$(FLAIR_SCROLL_ARROW_SPEC),FLAIR-SCROLL win 0 v part=21 value=48 max=138,1)
$(call fo-has,flair_scroll_arrow,FINDER-OPEN-VOLUME win=0 n=22,the overflow volume did not open with 22 entries)
$(call fo-has,flair_scroll_arrow,FLAIR-SCROLL win 0 v part=21 value=16 max=138,the first down-arrow click did not scroll one line)
$(call fo-has,flair_scroll_arrow,FLAIR-SCROLL win 0 v part=21 value=32 max=138,the second down-arrow click did not scroll one line)
$(call fo-has,flair_scroll_arrow,FLAIR-SCROLL win 0 v part=21 value=48 max=138,the third down-arrow click did not scroll one line)
$(call scroll-nodrag,flair_scroll_arrow)
@[ -s "$(BUILD)/flair_scroll_arrow.ppm" ] || { printf '!!! test-flair-scroll FAIL: ARROW screendump missing\n'; exit 1; }
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) scrollarrow "$(BUILD)/flair_scroll_arrow.ppm" \
	|| { printf '!!! test-flair-scroll FAIL: after 3 down-arrow clicks the view is not scrolled 48 px\n'; exit 1; }
@printf '>>> test-flair-scroll [1/6]: 3 down-arrow clicks -> value 48; rows graded scrolled; frame unmoved; no FLAIR-DRAG\n'
@# ---- [2] SELECT at the scrolled position ----
$(call scroll-boot,$(FLAIRTENANTS_IMG),flair_scroll_select,$(FLAIR_SCROLL_DATA_IMG),$(FLAIR_SCROLL_SELECT_SPEC),FINDER-WIN-SELECT,0)
$(call fo-has,flair_scroll_select,FINDER-WIN-SELECT win=0 name=F06.TXT count=1,a click on F06.TXT where it is DRAWN (55$(fo_comma)158) did not select it)
$(call scroll-nodrag,flair_scroll_select)
@printf '>>> test-flair-scroll [2/6]: a click on F06.TXT at its scrolled position selects F06.TXT\n'
@# ---- [3] PAGE ----
$(call scroll-boot,$(FLAIRTENANTS_IMG),flair_scroll_page,$(FLAIR_SCROLL_DATA_IMG),$(FLAIR_SCROLL_PAGE_SPEC),FLAIR-SCROLL win 0 v part=23 value=138 max=138,1)
$(call scroll-nodrag,flair_scroll_page)
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) scrollpage "$(BUILD)/flair_scroll_page.ppm" \
	|| { printf '!!! test-flair-scroll FAIL: after a page-down click the view is not at its end (138)\n'; exit 1; }
@printf '>>> test-flair-scroll [3/6]: page down -> 138 (clamped); last rows graded; frame unmoved\n'
@# ---- [4] THUMB ----
$(call scroll-boot,$(FLAIRTENANTS_IMG),flair_scroll_thumb,$(FLAIR_SCROLL_DATA_IMG),$(FLAIR_SCROLL_THUMB_SPEC),FLAIR-SCROLL win 0 v part=129 value=69 max=138,1)
$(call scroll-nodrag,flair_scroll_thumb)
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) scrollthumb "$(BUILD)/flair_scroll_thumb.ppm" \
	|| { printf '!!! test-flair-scroll FAIL: after the thumb drag the view is not at 69\n'; exit 1; }
@printf '>>> test-flair-scroll [4/6]: thumb dragged +66 px -> value 69 on release; rows graded; frame unmoved\n'
@# ---- [5] HOLD (auto-repeat) ----
$(call scroll-boot,$(FLAIRTENANTS_IMG),flair_scroll_hold,$(FLAIR_SCROLL_DATA_IMG),$(FLAIR_SCROLL_HOLD_SPEC),FLAIR-SCROLL win 0 v part=21 value=138 max=138,1)
@n=$$(grep -c '^FLAIR-SCROLL win 0 v part=21 ' "$(BUILD)/flair_scroll_hold.serial"); [ "$$n" -eq 9 ] \
	|| { printf '!!! test-flair-scroll FAIL: a held down arrow made %s steps, want exactly 9 (16..128 then 138)\n' "$$n"; exit 1; }
$(call scroll-nodrag,flair_scroll_hold)
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) scrollpage "$(BUILD)/flair_scroll_hold.ppm" \
	|| { printf '!!! test-flair-scroll FAIL: the held arrow did not run the view to its end\n'; exit 1; }
@printf '>>> test-flair-scroll [5/6]: down arrow HELD -> 9 repeated steps to 138; graded at the end\n'
@# ---- [6] FIT: nothing to scroll ----
$(call scroll-boot,$(FLAIRTENANTS_IMG),flair_scroll_fit,$(FLAIR_DATA_IMG),$(FLAIR_SCROLL_FIT_SPEC),FLAIR-SCROLL-IGNORED win 0 h,1)
$(call fo-has,flair_scroll_fit,FINDER-OPEN-VOLUME win=0 n=4,the flagship volume did not open)
$(call fo-has,flair_scroll_fit,FLAIR-SCROLL-IGNORED win 0 v,the DISABLED vertical bar click was not reported as ignored)
$(call fo-has,flair_scroll_fit,FLAIR-SCROLL-IGNORED win 0 h,the DISABLED horizontal bar click was not reported as ignored)
@! grep -q '^FLAIR-SCROLL win' "$(BUILD)/flair_scroll_fit.serial" || { printf '!!! test-flair-scroll FAIL: a DISABLED bar scrolled\n'; exit 1; }
$(call scroll-nodrag,flair_scroll_fit)
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) scrollfit "$(BUILD)/flair_scroll_fit.ppm" \
	|| { printf '!!! test-flair-scroll FAIL: the fitting window is not drawn with DISABLED bars and unmoved icons\n'; exit 1; }
@printf '>>> test-flair-scroll [6/6]: content fits -> DISABLED bars (Sec 3), both clicks ignored, nothing moved\n'
@printf '>>> test-flair-scroll: green\n'
```

## test-flair-scroll-look

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32914):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_scroll_look_check build/.makefile.stamp`
```make
$(call scroll-look-boot,$(FLAIRTENANTS_IMG),scroll_look_shrink,$(FLAIR_SCROLL_LOOK_SHRINK_SPEC))
@$(SCROLL_LOOK_CHECK) shrink $(BUILD)/scroll_look_shrink.ppm
$(call scroll-look-boot,$(FLAIRTENANTS_IMG),scroll_look_click,$(FLAIR_SCROLL_LOOK_CLICK_SPEC))
@grep -qx 'FLAIR-SCROLL win 0 h part=21 value=16 max=171' $(BUILD)/scroll_look_click.serial
@$(SCROLL_LOOK_CHECK) click $(BUILD)/scroll_look_click.ppm
$(call scroll-look-boot,$(FLAIRTENANTS_IMG),scroll_look_restore,$(FLAIR_SCROLL_LOOK_RESTORE_SPEC))
@grep -qx 'FLAIR-GROW win 0 (128,120)->(360,220)' $(BUILD)/scroll_look_restore.serial
@$(SCROLL_LOOK_CHECK) restore $(BUILD)/scroll_look_restore.ppm
$(call scroll-look-boot,$(FLAIRTENANTS_IMG),scroll_look_min,$(FLAIR_SCROLL_LOOK_SPEC))
@grep -qx 'FLAIR-GROW win 0 (360,220)->(96,88)' $(BUILD)/scroll_look_min.serial
@grep -qx 'FLAIR-SCROLL win 0 v part=21 value=10 max=10' $(BUILD)/scroll_look_min.serial
@$(SCROLL_LOOK_CHECK) min $(BUILD)/scroll_look_min.ppm
@printf '>>> test-flair-scroll-look: green (shrink enabled; click scrolls16; restore disabled; minimum96x88 draws all parts; no SCROLL-IGNORED)\n'
```

## test-flair-scroll-look-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32933):
Prerequisites: `test-flair-scroll-look build/qemu_harness build/flair_data.img build/ppm_flair_scroll_look_check build/flair_tenants_mut_scroll_stale_resize.img build/flair_tenants_mut_scroll_no_min.img build/flair_tenants_mut_scroll_ramp_flat.img build/.makefile.stamp`
```make
$(call scroll-look-boot,$(BUILD)/flair_tenants_mut_scroll_stale_resize.img,scroll_look_mut_stale,$(FLAIR_SCROLL_LOOK_SHRINK_SPEC))
@if $(SCROLL_LOOK_CHECK) shrink $(BUILD)/scroll_look_mut_stale.ppm > $(BUILD)/scroll_look_mut_stale.pixels 2>&1; then printf 'FAIL stale resize survived\n'; exit 1; fi
@grep -q 'FAIL scroll-look crop thumb-h' $(BUILD)/scroll_look_mut_stale.pixels
@printf '>>> test-flair-scroll-look-mutant: WINDOW_MUT_SCROLL_STALE_RESIZE correctly RED (enabled thumb missing after shrink)\n'
$(call scroll-look-boot,$(BUILD)/flair_tenants_mut_scroll_ramp_flat.img,scroll_look_mut_ramp,$(FLAIR_SCROLL_LOOK_SHRINK_SPEC))
@if $(SCROLL_LOOK_CHECK) shrink $(BUILD)/scroll_look_mut_ramp.ppm > $(BUILD)/scroll_look_mut_ramp.pixels 2>&1; then printf 'FAIL flat ramp survived\n'; exit 1; fi
@grep -q 'FAIL scroll-look crop thumb-h' $(BUILD)/scroll_look_mut_ramp.pixels
@printf '>>> test-flair-scroll-look-mutant: SB_LOOK_MUT_RAMP_FLAT correctly RED (independent thumb crop)\n'
$(call scroll-boot,$(BUILD)/flair_tenants_mut_scroll_no_min.img,scroll_look_mut_min,$(FLAIR_DATA_IMG),$(FLAIR_SCROLL_LOOK_SPEC),FLAIR-LIVE-OK,1)
@grep -qx 'FLAIR-GROW win 0 (360,220)->(60,40)' $(BUILD)/scroll_look_mut_min.serial
@if $(SCROLL_LOOK_CHECK) min $(BUILD)/scroll_look_mut_min.ppm > $(BUILD)/scroll_look_mut_min.pixels 2>&1; then printf 'FAIL no minimum survived\n'; exit 1; fi
@grep -q 'FAIL scroll-look crop' $(BUILD)/scroll_look_mut_min.pixels
@printf '>>> test-flair-scroll-look-mutant: WINDOW_MUTATE_GROW_NO_MIN correctly RED (undrawn bar and wrong frame)\n'
```

## test-flair-scroll-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22163):
Prerequisites: `build/qemu_harness build/flair_scroll_data.img build/ppm_flair_disk_windows_check build/flair_tenants_mut_scroll_drag.img build/flair_tenants_mut_scroll_paint_only.img build/.makefile.stamp`
```make
@# SCROLL_DRAG: the ARROW trace's first click is a WINDOW DRAG again (the
@# audit's zero-distance FLAIR-DRAG) and nothing scrolls -> the real gate's
@# leg [1] goes RED on its no-drag + value-16 assertions.
$(call scroll-boot,$(BUILD)/flair_tenants_mut_scroll_drag.img,flair_scroll_mut_drag,$(FLAIR_SCROLL_DATA_IMG),$(FLAIR_SCROLL_ARROW_SPEC),FLAIR-DRAG win 0,0)
@grep -q '^FLAIR-DRAG win 0 (20,60)->(20,60)' "$(BUILD)/flair_scroll_mut_drag.serial" || { printf '!!! test-flair-scroll-mutant FAIL: SCROLL_DRAG did not drag -- RED for the wrong reason (or not at all)\n'; exit 1; }
@! grep -q '^FLAIR-SCROLL win 0 v part=21 value=16' "$(BUILD)/flair_scroll_mut_drag.serial" || { printf '!!! test-flair-scroll-mutant FAIL: SCROLL_DRAG still scrolled -- the leg is decoration\n'; exit 1; }
@printf '>>> test-flair-scroll-mutant: SCROLL_DRAG correctly RED (the arrow click is a FLAIR-DRAG win 0 (20,60)->(20,60); nothing scrolls)\n'
@# PAINT_ONLY: the view LOOKS scrolled, but the click on the drawn F06.TXT
@# hits the unscrolled model -> leg [2] goes RED (another icon selects).
$(call scroll-boot,$(BUILD)/flair_tenants_mut_scroll_paint_only.img,flair_scroll_mut_paint,$(FLAIR_SCROLL_DATA_IMG),$(FLAIR_SCROLL_SELECT_SPEC),FINDER-WIN-SELECT,0)
@grep -q '^FINDER-WIN-SELECT win=0 name=' "$(BUILD)/flair_scroll_mut_paint.serial" || { printf '!!! test-flair-scroll-mutant FAIL: PAINT_ONLY never selected anything\n'; exit 1; }
@! grep -qx 'FINDER-WIN-SELECT win=0 name=F06.TXT count=1' "$(BUILD)/flair_scroll_mut_paint.serial" || { printf '!!! test-flair-scroll-mutant FAIL: PAINT_ONLY still selected F06.TXT -- the select leg is decoration\n'; exit 1; }
@printf '>>> test-flair-scroll-mutant: PAINT_ONLY correctly RED (%s instead of F06.TXT)\n' "$$(grep -m1 '^FINDER-WIN-SELECT' $(BUILD)/flair_scroll_mut_paint.serial)"
@printf '>>> test-flair-scroll-mutant: green (both mutants RED for the named reason)\n'
```

## test-flair-shutdown

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22570):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_shutdown_gate.img build/flair_data.img build/ppm_flair_shutdown_check test-flair-shutdown-cancel build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-shutdown : Special > Shut Down puts everything away, shows the safe screen, and the session is over (tdnl.78)\n'
@printf '======================================================================\n'
$(call fo-boot,$(FLAIR_SHUTDOWN_GATE_IMG),$(FLAIR_SD_NAME),$(BUILD)/$(FLAIR_SD_NAME)_data.img,$(FLAIR_SHUTDOWN_SPEC),$(FLAIR_SD_DONE),1)
@( $(call sd-check,$(BUILD)/$(FLAIR_SD_NAME).serial) ) || { printf '!!! test-flair-shutdown FAIL: serial (above)\n'; grep -E '^(FINDER-|FLAIR-MENU|FLAIR-SHUTDOWN|FLAIR-RESTART|DESKTOP-DB|PANIC)' $(BUILD)/$(FLAIR_SD_NAME).serial; exit 1; }
@( $(call pw-disk-check,$(BUILD)/$(FLAIR_SD_NAME)_data.img) ) || { printf '!!! test-flair-shutdown FAIL: the disk (above)\n'; exit 1; }
@[ -s "$(BUILD)/$(FLAIR_SD_NAME).ppm" ] || { printf '!!! test-flair-shutdown FAIL: screendump missing\n'; exit 1; }
@$(PPM_FLAIR_SHUTDOWN_CHECK_BIN) safe "$(BUILD)/$(FLAIR_SD_NAME).ppm" || { printf '!!! test-flair-shutdown FAIL: the shut-down screen is not drawn as graded\n'; exit 1; }
@printf '>>> test-flair-shutdown [1/2]: FINDER-CMD SHUTDOWN -> DESKTOP-DB-SAVE n=3 -> FLAIR-SHUTDOWN-SAFE; the view is on the disk; the safe screen graded; the probe click + Ctrl-N answered by nothing\n'
$(call fo-boot,$(FLAIR_SHUTDOWN_GATE_IMG),$(FLAIR_SDA_NAME),$(BUILD)/$(FLAIR_SDA_NAME)_data.img,$(FLAIR_SHUTDOWN_APP_SPEC),$(FLAIR_SD_DONE),1)
@( $(call sda-check,$(BUILD)/$(FLAIR_SDA_NAME).serial) ) || { printf '!!! test-flair-shutdown FAIL: [2] serial (above)\n'; grep -E '^(FINDER-|TENANT-(MENU|EXIT)|FLAIR-DISPATCH|FLAIR-MENU |FLAIR-SHUTDOWN|DESKTOP-DB|PANIC)' $(BUILD)/$(FLAIR_SDA_NAME).serial; exit 1; }
@$(PPM_FLAIR_SHUTDOWN_CHECK_BIN) safe "$(BUILD)/$(FLAIR_SDA_NAME).ppm" || { printf '!!! test-flair-shutdown FAIL: [2] the shut-down screen is not drawn as graded\n'; exit 1; }
@printf '>>> test-flair-shutdown [2/2]: CTENANT running -> asked to quit by its own File > Quit -> TENANT-EXIT -> DESKTOP-DB-SAVE n=4 -> FLAIR-SHUTDOWN-SAFE\n'
@printf '>>> test-flair-shutdown: green\n'
```

## test-flair-shutdown-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 131):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data_power_cancel.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-shutdown-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-shutdown-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data_power_cancel.img $(BUILD)/flair-shutdown_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-shutdown_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-shutdown_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-shutdown_bochs.report.txt" || true
@cat "$(BUILD)/flair-shutdown_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-shutdown-bochs "$(BUILD)/flair-shutdown_bochs.report.txt" "$(BUILD)/flair-shutdown_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-shutdown-cancel

Declared tier: EMU; #  recipe to execute (from 'harness/emu/power_cancel.mk', line 45):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_shutdown_gate.img build/flair_data_power_cancel.img build/.makefile.stamp`
```make
$(call power-cancel-boot,$(FLAIR_SHUTDOWN_GATE_IMG),flair_shutdown_cancel,$(FLAIR_SDC_DONE))
@sh harness/emu/shutdown_cancel_check.sh $(BUILD)/flair_shutdown_cancel.serial $(BUILD)/flair_shutdown_cancel_data.img
@mcopy -o -i $(BUILD)/flair_shutdown_cancel_data.img ::APPS/CTENANT.EXE $(BUILD)/flair_shutdown_cancel_tenant.exe
@cmp $(BUILD)/tenant/ctenant_decline.exe $(BUILD)/flair_shutdown_cancel_tenant.exe
@fsck.fat -n $(BUILD)/flair_shutdown_cancel_data.img > $(BUILD)/flair_shutdown_cancel.fsck
@printf '>>> test-flair-shutdown-cancel: green (declining fixture retained byte-exact by mtools; Ctrl-N folder complete on the volume; fsck.fat clean)\n'
```

## test-flair-shutdown-cancel-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/power_cancel.mk', line 59):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_shutdown_gate_no_ask.img build/flair_tenants_mut_shutdown_gate_mkdir_cut.img build/flair_data_power_cancel.img build/.makefile.stamp`
```make
$(call power-cancel-boot,$(BUILD)/flair_tenants_mut_shutdown_gate_no_ask.img,flair_shutdown_cancel_mut_noask,$(FLAIR_SD_DONE))
@if sh harness/emu/shutdown_cancel_check.sh $(BUILD)/flair_shutdown_cancel_mut_noask.serial > $(BUILD)/flair_shutdown_cancel_mut_noask.check 2>&1; then printf 'FAIL POWER_NO_ASK survived the cancel leg\n'; exit 1; fi
@grep -qxF 'SDC: the application did not cancel Shut Down' $(BUILD)/flair_shutdown_cancel_mut_noask.check
@printf '>>> test-flair-shutdown-cancel-mutant: POWER_NO_ASK RED: %s\n' "$$(cat $(BUILD)/flair_shutdown_cancel_mut_noask.check)"
$(call power-cancel-boot,$(BUILD)/flair_tenants_mut_shutdown_gate_mkdir_cut.img,flair_shutdown_cancel_mut_mkdircut,$(FLAIR_SDC_DONE))
@if sh harness/emu/shutdown_cancel_check.sh $(BUILD)/flair_shutdown_cancel_mut_mkdircut.serial $(BUILD)/flair_shutdown_cancel_mut_mkdircut_data.img > $(BUILD)/flair_shutdown_cancel_mut_mkdircut.check 2>&1; then printf 'FAIL MKDIR_CUT survived the cancel leg\n'; exit 1; fi
@grep -qxF 'SDC: folder NEWFOLD (parent 3) reported created but absent from the volume' $(BUILD)/flair_shutdown_cancel_mut_mkdircut.check
@if fsck.fat -n $(BUILD)/flair_shutdown_cancel_mut_mkdircut_data.img > $(BUILD)/flair_shutdown_cancel_mut_mkdircut.fsck 2>&1; then printf 'FAIL MKDIR_CUT orphan passed fsck.fat\n'; exit 1; fi
@grep -qF 'Reclaimed 1 unused cluster' $(BUILD)/flair_shutdown_cancel_mut_mkdircut.fsck
@printf '>>> test-flair-shutdown-cancel-mutant: MKDIR_CUT RED: %s; fsck.fat: %s\n' "$$(cat $(BUILD)/flair_shutdown_cancel_mut_mkdircut.check)" "$$(grep -F 'Reclaimed' $(BUILD)/flair_shutdown_cancel_mut_mkdircut.fsck)"
```

## test-flair-shutdown-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22598):
Prerequisites: `build/qemu_harness build/flair_data.img build/flair_tenants_mut_shutdown_gate_no_flush.img build/flair_tenants_mut_shutdown_gate_returns.img build/flair_tenants_mut_shutdown_gate_no_ask.img test-flair-shutdown-cancel-mutant build/.makefile.stamp`
```make
$(call fo-boot,$(BUILD)/flair_tenants_mut_shutdown_gate_no_flush.img,flair_shutdown_mut_noflush,$(BUILD)/flair_shutdown_mut_noflush_data.img,$(FLAIR_SHUTDOWN_SPEC),$(FLAIR_SD_DONE),0)
@out=$$( ( $(call sd-check,$(BUILD)/flair_shutdown_mut_noflush.serial) ) && ( $(call pw-disk-check,$(BUILD)/flair_shutdown_mut_noflush_data.img) ) ); rc=$$?; \
$(call pw-mut-verdict,POWER_NO_FLUSH,$$out,$$rc,did not commit DESKTOP.DB,test-flair-shutdown-mutant)
@out=$$( ( $(call pw-disk-check,$(BUILD)/flair_shutdown_mut_noflush_data.img) ) ); rc=$$?; \
$(call pw-mut-verdict,POWER_NO_FLUSH/disk,$$out,$$rc,is not on the disk,test-flair-shutdown-mutant)
$(call fo-boot,$(BUILD)/flair_tenants_mut_shutdown_gate_returns.img,flair_shutdown_mut_returns,$(BUILD)/flair_shutdown_mut_returns_data.img,$(FLAIR_SHUTDOWN_SPEC),$(FLAIR_SD_DONE),0)
@out=$$( ( $(call sd-check,$(BUILD)/flair_shutdown_mut_returns.serial) ) ); rc=$$?; \
$(call pw-mut-verdict,SHUTDOWN_RETURNS,$$out,$$rc,the desktop answered after Shut Down,test-flair-shutdown-mutant)
$(call fo-boot,$(BUILD)/flair_tenants_mut_shutdown_gate_no_ask.img,flair_shutdown_mut_noask,$(BUILD)/flair_shutdown_mut_noask_data.img,$(FLAIR_SHUTDOWN_APP_SPEC),$(FLAIR_SD_DONE),0)
@out=$$( ( $(call sda-check,$(BUILD)/flair_shutdown_mut_noask.serial) ) ); rc=$$?; \
$(call pw-mut-verdict,POWER_NO_ASK,$$out,$$rc,CTENANT was not asked to quit,test-flair-shutdown-mutant)
```

## test-flair-solid

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22689):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_solid_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-solid : the live-desktop SOLIDITY contract\n'
@printf '  A close / B drag / C activate / D menu-cancel restore / E held band-2 menu / G clamp / H raise.\n'
@printf '  Ref: epic initech-av7s (DQ1-DQ3, DQ5-DQ6, DQ9); initech-b3hl/-j0vt; locked solid traces.\n'
@printf '======================================================================\n'
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@# ---- leg A: close HELLO -> the exposed NOTES overlap is repainted content ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_A_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_CLOSE_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-CLOSE win" --timeout-ms 15000 \
	2> "$(BUILD)/$(FLAIR_SOLID_A_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_SOLID_A_NAME).report"; then printf '!!! test-flair-solid FAIL: leg A TRIPLE FAULT\n'; exit 1; fi
@grep -q '^FLAIR-CLOSE win 0$$' "$(BUILD)/$(FLAIR_SOLID_A_NAME).serial" \
	|| { printf '!!! test-flair-solid FAIL: leg A exact FLAIR-CLOSE win 0 marker missing (tenant identity/dispatch failed)\n'; grep '^FLAIR-' "$(BUILD)/$(FLAIR_SOLID_A_NAME).serial" || true; exit 1; }
@grep -q '^FLAIR-TENANT-EXIT name=HELLO$$' "$(BUILD)/$(FLAIR_SOLID_A_NAME).serial" \
	|| { printf '!!! test-flair-solid FAIL: leg A terminate-on-close marker missing (HELLO was hidden instead of terminated)\n'; grep '^FLAIR-' "$(BUILD)/$(FLAIR_SOLID_A_NAME).serial" || true; exit 1; }
@if [ ! -s "$(BUILD)/$(FLAIR_SOLID_A_NAME).ppm" ]; then printf '!!! test-flair-solid FAIL: leg A screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_SOLID_CHECK_BIN) A "$(BUILD)/$(FLAIR_SOLID_A_NAME).ppm" \
	|| { printf '!!! test-flair-solid FAIL: leg A -- the close-exposed NOTES overlap is NOT repainted content (the white-hole family; initech-gofc)\n'; exit 1; }
@printf '>>> test-flair-solid [A]: close-expose content repainted (no white hole)\n'
@# ---- leg B: activate NOTES then drag it -- content survives the move ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_B_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_DRAG_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-DRAG win" --timeout-ms 20000 \
	2> "$(BUILD)/$(FLAIR_SOLID_B_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_SOLID_B_NAME).report"; then printf '!!! test-flair-solid FAIL: leg B TRIPLE FAULT\n'; exit 1; fi
@grep -q '^FLAIR-DISPATCH app=NOTES$$' "$(BUILD)/$(FLAIR_SOLID_B_NAME).serial" \
	|| { printf '!!! test-flair-solid FAIL: leg B precondition -- the activating switch never dispatched\n'; grep '^FLAIR-' "$(BUILD)/$(FLAIR_SOLID_B_NAME).serial" || true; exit 1; }
@grep -q '^FLAIR-DRAG win 0 (260,120)->(200,180)$$' "$(BUILD)/$(FLAIR_SOLID_B_NAME).serial" \
	|| { printf '!!! test-flair-solid FAIL: leg B exact FLAIR-DRAG win 0 marker missing (tenant identity/title drag failed)\n'; grep '^FLAIR-' "$(BUILD)/$(FLAIR_SOLID_B_NAME).serial" || true; exit 1; }
@if [ ! -s "$(BUILD)/$(FLAIR_SOLID_B_NAME).ppm" ]; then printf '!!! test-flair-solid FAIL: leg B screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_SOLID_CHECK_BIN) B "$(BUILD)/$(FLAIR_SOLID_B_NAME).ppm" \
	|| { printf '!!! test-flair-solid FAIL: leg B -- dragged NOTES content wiped to WDEF white (initech-gofc)\n'; exit 1; }
@printf '>>> test-flair-solid [B]: drag preserves tenant content at the new position\n'
@# ---- leg C: the O-5 switch -- activation chrome full-width, no stale band ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_C_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_SWITCH_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-DISPATCH app=NOTES" --timeout-ms 15000 \
	2> "$(BUILD)/$(FLAIR_SOLID_C_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_SOLID_C_NAME).report"; then printf '!!! test-flair-solid FAIL: leg C TRIPLE FAULT\n'; exit 1; fi
@grep -q '^FLAIR-DISPATCH app=NOTES$$' "$(BUILD)/$(FLAIR_SOLID_C_NAME).serial" \
	|| { printf '!!! test-flair-solid FAIL: leg C FLAIR-DISPATCH app=NOTES missing\n'; grep '^FLAIR-' "$(BUILD)/$(FLAIR_SOLID_C_NAME).serial" || true; exit 1; }
@if [ ! -s "$(BUILD)/$(FLAIR_SOLID_C_NAME).ppm" ]; then printf '!!! test-flair-solid FAIL: leg C screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_SOLID_CHECK_BIN) C "$(BUILD)/$(FLAIR_SOLID_C_NAME).ppm" \
	|| { printf '!!! test-flair-solid FAIL: leg C -- activation chrome wrong (flat/half-active title or stale band; initech-rqz5)\n'; exit 1; }
@printf '>>> test-flair-solid [C]: activation chrome full-width active + flat inactive + no stale band\n'
@# ---- leg D: held File -> Edit -> release in bar; PRE == restored POST ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_D_PRE_NAME)" --out "$(BUILD)" \
	--screendump --screendump-after "FLAIR-LIVE-OK" --timeout-ms 15000 \
	2> "$(BUILD)/$(FLAIR_SOLID_D_PRE_NAME).report" || true
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_D_POST_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_MENUCANCEL_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-LIVE-OK" --timeout-ms 15000 \
	2> "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_SOLID_D_PRE_NAME).report" "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).report"; then printf '!!! test-flair-solid FAIL: leg D TRIPLE FAULT in PRE or POST\n'; exit 1; fi
@grep -q '^FLAIR-LIVE-OK$$' "$(BUILD)/$(FLAIR_SOLID_D_PRE_NAME).serial" \
	|| { printf '!!! test-flair-solid FAIL: leg D PRE FLAIR-LIVE-OK missing\n'; exit 1; }
@grep -q '^FLAIR-MENU-DROP menu=256$$' "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).serial" \
	|| { printf '!!! test-flair-solid FAIL: leg D FLAIR-MENU-DROP menu=256 missing -- band-2 File did not drop\n'; grep '^FLAIR-' "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).serial" || true; exit 1; }
@grep -q '^FLAIR-MENU-XDROP menu=257$$' "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).serial" \
	|| { printf '!!! test-flair-solid FAIL: leg D FLAIR-MENU-XDROP menu=257 missing -- held File did not cross-redraw to Edit\n'; grep '^FLAIR-' "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).serial" || true; exit 1; }
@grep -q '^FLAIR-MENU menu=256 item=0 (sel=0x00000000)$$' "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).serial" \
	|| { printf '!!! test-flair-solid FAIL: leg D final sel=0 cancel marker missing -- release was not in the bar\n'; grep '^FLAIR-' "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).serial" || true; exit 1; }
@if [ ! -s "$(BUILD)/$(FLAIR_SOLID_D_PRE_NAME).ppm" ] || [ ! -s "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).ppm" ]; then printf '!!! test-flair-solid FAIL: leg D PRE or POST screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_SOLID_CHECK_BIN) D "$(BUILD)/$(FLAIR_SOLID_D_PRE_NAME).ppm" "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).ppm" \
	|| { printf '!!! test-flair-solid FAIL: leg D PRE != POST -- menu cancel left a panel or shell-render wiped tenant content (initech-b3hl/-j0vt)\n'; exit 1; }
@printf '>>> test-flair-solid [D]: cross-title cancel restored the whole frame byte-identical (TOL=0)\n'
@# ---- leg E: HELLO active -> Photoshop File drops from band 2 at y=40 ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_E_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_MENU2_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-MENU-DROP menu=256" --timeout-ms 15000 \
	2> "$(BUILD)/$(FLAIR_SOLID_E_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_SOLID_E_NAME).report"; then printf '!!! test-flair-solid FAIL: leg E TRIPLE FAULT\n'; exit 1; fi
@grep -q '^FLAIR-MENU-DROP menu=256$$' "$(BUILD)/$(FLAIR_SOLID_E_NAME).serial" \
	|| { printf '!!! test-flair-solid FAIL: leg E FLAIR-MENU-DROP menu=256 missing -- band 2 did not route to active HELLO/Photoshop\n'; grep '^FLAIR-' "$(BUILD)/$(FLAIR_SOLID_E_NAME).serial" || true; exit 1; }
@# Rule-8 re-key 2026-10-20 (initech-tdnl.79, merged with the Finder alerts bundle): the
@# HELLO/NOTES demo menu items are authored DISABLED (os/apps/ref_demo_menu.h), so the
@# release over Photoshop File item 2 must report NO selection (item=0, sel=0) -- a dead
@# item never dispatches (MTE Menu Manager: disabled items cannot be chosen). The leg
@# keeps its purpose (band 2 drops the active tenant's menu, graded below on pixels).
@grep -q '^FLAIR-MENU menu=256 item=0 (sel=0x00000000)$$' "$(BUILD)/$(FLAIR_SOLID_E_NAME).serial" \
	|| { printf '!!! test-flair-solid FAIL: leg E Photoshop File release over the disabled item 2 must select nothing (tdnl.79)\n'; grep '^FLAIR-' "$(BUILD)/$(FLAIR_SOLID_E_NAME).serial" || true; exit 1; }
@if [ ! -s "$(BUILD)/$(FLAIR_SOLID_E_NAME).ppm" ]; then printf '!!! test-flair-solid FAIL: leg E screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_SOLID_CHECK_BIN) E "$(BUILD)/$(FLAIR_SOLID_E_NAME).ppm" \
	|| { printf '!!! test-flair-solid FAIL: leg E -- active tenant File panel did not drop below band 2 (initech-t1rv)\n'; exit 1; }
@printf '>>> test-flair-solid [E]: band 2 drops HELLO/Photoshop menuID 256 below y=40\n'
@# ---- leg G: drag toward (5,5) -- title band stays reachable below band 2 ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_G_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_CLAMP_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-DRAG win" --timeout-ms 20000 \
	2> "$(BUILD)/$(FLAIR_SOLID_G_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_SOLID_G_NAME).report"; then printf '!!! test-flair-solid FAIL: leg G TRIPLE FAULT\n'; exit 1; fi
@grep -q '^FLAIR-DISPATCH app=NOTES$$' "$(BUILD)/$(FLAIR_SOLID_G_NAME).serial" \
	|| { printf '!!! test-flair-solid FAIL: leg G precondition -- the activating switch never dispatched\n'; grep '^FLAIR-' "$(BUILD)/$(FLAIR_SOLID_G_NAME).serial" || true; exit 1; }
@# Grab (450,130)->release (5,5) = (-445,-125); NOTES (260,120) proposes
@# (-185,-5), then DQ6 clamps top to 2*FLAIR_CHROME_MENUBAR_H = 40.
@grep -q '^FLAIR-DRAG win 0 (260,120)->(-185,40)$$' "$(BUILD)/$(FLAIR_SOLID_G_NAME).serial" \
	|| { printf '!!! test-flair-solid FAIL: leg G expected clamped FLAIR-DRAG win 0 (-185,40) marker missing\n'; grep '^FLAIR-' "$(BUILD)/$(FLAIR_SOLID_G_NAME).serial" || true; exit 1; }
@if [ ! -s "$(BUILD)/$(FLAIR_SOLID_G_NAME).ppm" ]; then printf '!!! test-flair-solid FAIL: leg G screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_SOLID_CHECK_BIN) G "$(BUILD)/$(FLAIR_SOLID_G_NAME).ppm" \
	|| { printf '!!! test-flair-solid FAIL: leg G -- dragged NOTES title band is not reachable below menu band 2 (initech-r8r7)\n'; exit 1; }
@printf '>>> test-flair-solid [G]: drag clamp keeps the full title band reachable at (-185,40)\n'
@# ---- leg H: background title press -- switch first, then drag at the front ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_H_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_RAISE_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-DRAG win" --timeout-ms 20000 \
	2> "$(BUILD)/$(FLAIR_SOLID_H_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_SOLID_H_NAME).report"; then printf '!!! test-flair-solid FAIL: leg H TRIPLE FAULT\n'; exit 1; fi
@grep -q '^FLAIR-DRAG win 0 (260,120)->(200,180)$$' "$(BUILD)/$(FLAIR_SOLID_H_NAME).serial" \
	|| { printf '!!! test-flair-solid FAIL: leg H exact FLAIR-DRAG win 0 (200,180) marker missing\n'; grep '^FLAIR-' "$(BUILD)/$(FLAIR_SOLID_H_NAME).serial" || true; exit 1; }
@awk '/^FLAIR-DISPATCH app=NOTES$$/ && !d { d=NR } /^FLAIR-DRAG win 0 \(260,120\)->\(200,180\)$$/ && !g { g=NR } END { exit !(d && g && d < g) }' "$(BUILD)/$(FLAIR_SOLID_H_NAME).serial" \
	|| { printf '!!! test-flair-solid FAIL: leg H requires FLAIR-DISPATCH app=NOTES BEFORE the exact FLAIR-DRAG marker\n'; grep '^FLAIR-' "$(BUILD)/$(FLAIR_SOLID_H_NAME).serial" || true; exit 1; }
@if [ ! -s "$(BUILD)/$(FLAIR_SOLID_H_NAME).ppm" ]; then printf '!!! test-flair-solid FAIL: leg H screendump missing\n'; exit 1; fi
@$(PPM_FLAIR_SOLID_CHECK_BIN) H "$(BUILD)/$(FLAIR_SOLID_H_NAME).ppm" \
	|| { printf '!!! test-flair-solid FAIL: leg H -- NOTES did not raise/activate before its title drag (initech-haaq)\n'; exit 1; }
@printf '>>> test-flair-solid [H]: background title click switches to NOTES, then drags it in front to (200,180)\n'
@for n in $(FLAIR_SOLID_A_NAME) $(FLAIR_SOLID_B_NAME) $(FLAIR_SOLID_C_NAME) \
	$(FLAIR_SOLID_D_PRE_NAME) $(FLAIR_SOLID_D_POST_NAME) $(FLAIR_SOLID_E_NAME) \
	$(FLAIR_SOLID_G_NAME) $(FLAIR_SOLID_H_NAME); do \
	grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/$$n.serial" \
		|| { printf '!!! test-flair-solid FAIL: %s missing FLAIR-FAT-MOUNT-OK\n' "$$n"; exit 1; }; \
done
@printf 'VERDICT   : PASS -- the live desktop honours the ONE repaint contract under\n'
@printf '            close/drag/switch/menu; DQ5 raises on title and DQ6 keeps titles reachable.\n'
@printf '======================================================================\n'
```

## test-flair-solid-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22919):
Prerequisites: `build/qemu_harness build/ppm_flair_solid_check build/flair_tenants.img build/flair_data.img build/flair_tenants_mut_no_route_on_chrome.img build/flair_tenants_mut_no_activate_inval.img build/flair_tenants_mut_menu_no_restore.img build/flair_tenants_mut_menu_restore_shell.img build/flair_tenants_mut_menu2_dead.img build/flair_tenants_mut_menu2_bar_sys.img build/flair_tenants_mut_no_drag_clamp.img build/flair_tenants_mut_no_raise_on_title.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-solid-mutant : Rule 6 (the gate BITES)\n'
@printf '  no_route_on_chrome breaks A+B; no_activate_inval C; menu restore mutants D; menu2 mutants E; clamp G; raise H.\n'
@printf '======================================================================\n'
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@# ---- baseline: the CLEAN image grades GREEN on all seven legs. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_A_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_CLOSE_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-CLOSE win" --timeout-ms 15000 >/dev/null 2>&1 || true
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_B_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_DRAG_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-DRAG win" --timeout-ms 20000 >/dev/null 2>&1 || true
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_C_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_SWITCH_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-DISPATCH app=NOTES" --timeout-ms 15000 >/dev/null 2>&1 || true
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_D_PRE_NAME)" --out "$(BUILD)" \
	--screendump --screendump-after "FLAIR-LIVE-OK" --timeout-ms 15000 >/dev/null 2>&1 || true
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_D_POST_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_MENUCANCEL_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-LIVE-OK" --timeout-ms 15000 >/dev/null 2>&1 || true
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_E_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_MENU2_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-MENU-DROP menu=256" --timeout-ms 15000 >/dev/null 2>&1 || true
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_G_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_CLAMP_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-DRAG win" --timeout-ms 20000 >/dev/null 2>&1 || true
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_SOLID_H_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_RAISE_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-DRAG win" --timeout-ms 20000 >/dev/null 2>&1 || true
@for leg in A:$(FLAIR_SOLID_A_NAME) B:$(FLAIR_SOLID_B_NAME) C:$(FLAIR_SOLID_C_NAME) E:$(FLAIR_SOLID_E_NAME) G:$(FLAIR_SOLID_G_NAME) H:$(FLAIR_SOLID_H_NAME); do \
	l=$${leg%%:*}; n=$${leg#*:}; \
	grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/$$n.serial" \
		|| { printf '!!! test-flair-solid-mutant FAIL: clean leg %s missing mount marker\n' "$$l"; exit 1; }; \
	if [ ! -s "$(BUILD)/$$n.ppm" ]; then printf '!!! test-flair-solid-mutant FAIL: clean leg %s produced no screendump (no baseline)\n' "$$l"; exit 1; fi; \
	$(PPM_FLAIR_SOLID_CHECK_BIN) $$l "$(BUILD)/$$n.ppm" >/dev/null 2>&1 \
		|| { printf '!!! test-flair-solid-mutant FAIL: the CLEAN image did not grade GREEN on leg %s -- the baseline is broken\n' "$$l"; exit 1; }; \
done
@if [ ! -s "$(BUILD)/$(FLAIR_SOLID_D_PRE_NAME).ppm" ] || [ ! -s "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).ppm" ]; then printf '!!! test-flair-solid-mutant FAIL: clean leg D PRE/POST dump missing\n'; exit 1; fi
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/$(FLAIR_SOLID_D_PRE_NAME).serial" \
	&& grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).serial" \
	|| { printf '!!! test-flair-solid-mutant FAIL: clean leg D mount marker missing\n'; exit 1; }
@$(PPM_FLAIR_SOLID_CHECK_BIN) D "$(BUILD)/$(FLAIR_SOLID_D_PRE_NAME).ppm" "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).ppm" >/dev/null 2>&1 \
	|| { printf '!!! test-flair-solid-mutant FAIL: the CLEAN image did not grade GREEN on leg D -- baseline restore is broken\n'; exit 1; }
@grep -q '^FLAIR-MENU-DROP menu=256$$' "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).serial" \
	&& grep -q '^FLAIR-MENU-XDROP menu=257$$' "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).serial" \
	&& grep -q '^FLAIR-MENU menu=256 item=0 (sel=0x00000000)$$' "$(BUILD)/$(FLAIR_SOLID_D_POST_NAME).serial" \
	|| { printf '!!! test-flair-solid-mutant FAIL: clean leg D lacks the DROP/XDROP/cancel serial baseline\n'; exit 1; }
@grep -q '^FLAIR-MENU-DROP menu=256$$' "$(BUILD)/$(FLAIR_SOLID_E_NAME).serial" \
	|| { printf '!!! test-flair-solid-mutant FAIL: clean leg E lacks the menu=256 routing baseline\n'; exit 1; }
@printf '>>> baseline: the clean FLAIRTENANTS image grades GREEN on legs A+B+C+D+E+G+H\n'
@# ---- no_route_on_chrome: legs A and B MUST go RED. ('|'-delimited fields:
@# the mouse specs themselves contain ':'.) ----
@rc=0; \
for leg in "A|$(FLAIR_SOLID_CLOSE_SPEC)|FLAIR-CLOSE win|15000" "B|$(FLAIR_SOLID_DRAG_SPEC)|FLAIR-DRAG win|20000"; do \
	l=$${leg%%|*}; rest=$${leg#*|}; spec=$${rest%%|*}; rest=$${rest#*|}; marker=$${rest%%|*}; tmo=$${rest#*|}; \
	$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_no_route_on_chrome.img" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK \
		--name "flair_solid_mut_route_$$l" --out "$(BUILD)" \
		--mouse "$$spec" --keys-after "FLAIR-LIVE-READY" \
		--screendump --screendump-after "$$marker" --timeout-ms "$$tmo" \
		2> "$(BUILD)/flair_solid_mut_route_$$l.report" || true; \
	if ! grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_solid_mut_route_$$l.serial"; then \
		printf '!!! test-flair-solid-mutant FAIL: no_route leg %s missing mount marker\n' "$$l"; rc=1; continue; \
	fi; \
	if grep -q 'triple_fault=1' "$(BUILD)/flair_solid_mut_route_$$l.report"; then \
		printf '!!! test-flair-solid-mutant FAIL: no_route_on_chrome leg %s TRIPLE-FAULTED (cannot judge)\n' "$$l"; rc=1; continue; \
	fi; \
	if [ ! -s "$(BUILD)/flair_solid_mut_route_$$l.ppm" ]; then \
		printf '!!! test-flair-solid-mutant FAIL: no_route_on_chrome leg %s produced no screendump\n' "$$l"; rc=1; continue; \
	fi; \
	if $(PPM_FLAIR_SOLID_CHECK_BIN) $$l "$(BUILD)/flair_solid_mut_route_$$l.ppm" > "$(BUILD)/flair_solid_mut_route_$$l.chk" 2>&1; then \
		printf '!!! test-flair-solid-mutant FAIL: leg %s oracle is DECORATION -- no_route_on_chrome PASSED it\n' "$$l"; rc=1; \
	else \
		printf '>>> no_route_on_chrome leg %s correctly RED:\n' "$$l"; \
		grep -m2 'FAIL ' "$(BUILD)/flair_solid_mut_route_$$l.chk" | sed 's/^/      /'; \
	fi; \
done; \
if [ "$$rc" != "0" ]; then exit 1; fi
@# ---- no_activate_inval: leg C MUST go RED. ----
@$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_no_activate_inval.img" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK \
	--name flair_solid_mut_actinval_C --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_SWITCH_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-DISPATCH app=NOTES" --timeout-ms 15000 \
	2> "$(BUILD)/flair_solid_mut_actinval_C.report" || true
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_solid_mut_actinval_C.serial"
@if grep -q 'triple_fault=1' "$(BUILD)/flair_solid_mut_actinval_C.report"; then printf '!!! test-flair-solid-mutant FAIL: no_activate_inval TRIPLE-FAULTED (cannot judge)\n'; exit 1; fi
@if [ ! -s "$(BUILD)/flair_solid_mut_actinval_C.ppm" ]; then printf '!!! test-flair-solid-mutant FAIL: no_activate_inval produced no screendump\n'; exit 1; fi
@if $(PPM_FLAIR_SOLID_CHECK_BIN) C "$(BUILD)/flair_solid_mut_actinval_C.ppm" > "$(BUILD)/flair_solid_mut_actinval_C.chk" 2>&1; then \
	printf '!!! test-flair-solid-mutant FAIL: the leg C oracle is DECORATION -- no_activate_inval PASSED it\n'; exit 1; \
else \
	printf '>>> no_activate_inval leg C correctly RED:\n'; \
	grep -m2 'FAIL ' "$(BUILD)/flair_solid_mut_actinval_C.chk" | sed 's/^/      /'; \
fi
@# ---- menu restore mutants: leg D PRE == POST MUST go RED. ----
@rc=0; \
for spec in "menu_no_restore:track-end restore skipped -- Edit panel persists" \
            "menu_restore_shell:shell_render erase ran without tenant content route -- WDEF white wipe"; do \
	tag=$${spec%%:*}; desc=$${spec#*:}; \
	$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_$$tag.img" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK \
		--name "flair_solid_mut_$${tag}_D" --out "$(BUILD)" \
		--mouse "$(FLAIR_SOLID_MENUCANCEL_SPEC)" --keys-after "FLAIR-LIVE-READY" \
		--screendump --screendump-after "FLAIR-LIVE-OK" --timeout-ms 15000 \
		2> "$(BUILD)/flair_solid_mut_$${tag}_D.report" || true; \
	if ! grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_solid_mut_$${tag}_D.serial"; then \
		printf '!!! test-flair-solid-mutant FAIL: %s missing mount marker\n' "$$tag"; rc=1; continue; \
	fi; \
	if grep -q 'triple_fault=1' "$(BUILD)/flair_solid_mut_$${tag}_D.report"; then \
		printf '!!! test-flair-solid-mutant FAIL: %s leg D TRIPLE-FAULTED (cannot judge)\n' "$$tag"; rc=1; continue; \
	fi; \
	if ! grep -q '^FLAIR-MENU-DROP menu=256$$' "$(BUILD)/flair_solid_mut_$${tag}_D.serial" || \
	   ! grep -q '^FLAIR-MENU-XDROP menu=257$$' "$(BUILD)/flair_solid_mut_$${tag}_D.serial" || \
	   ! grep -q '^FLAIR-MENU menu=256 item=0 (sel=0x00000000)$$' "$(BUILD)/flair_solid_mut_$${tag}_D.serial"; then \
		printf '!!! test-flair-solid-mutant FAIL: %s did not complete the comparable DROP/XDROP/cancel trace\n' "$$tag"; rc=1; continue; \
	fi; \
	if [ ! -s "$(BUILD)/flair_solid_mut_$${tag}_D.ppm" ]; then \
		printf '!!! test-flair-solid-mutant FAIL: %s leg D produced no POST dump\n' "$$tag"; rc=1; continue; \
	fi; \
	if $(PPM_FLAIR_SOLID_CHECK_BIN) D "$(BUILD)/$(FLAIR_SOLID_D_PRE_NAME).ppm" \
	   "$(BUILD)/flair_solid_mut_$${tag}_D.ppm" > "$(BUILD)/flair_solid_mut_$${tag}_D.chk" 2>&1; then \
		printf '!!! test-flair-solid-mutant FAIL: leg D oracle is DECORATION -- %s PASSED (%s)\n' "$$tag" "$$desc"; rc=1; \
	else \
		printf '>>> %s leg D correctly RED -- %s:\n' "$$tag" "$$desc"; \
		grep -m1 'FAIL leg D' "$(BUILD)/flair_solid_mut_$${tag}_D.chk" | sed 's/^/      /'; \
	fi; \
done; \
if [ "$$rc" != "0" ]; then exit 1; fi
@# ---- menu2_dead: leg E pixels + marker MUST go RED. Capture at LIVE-OK
@# because the faithful dead-band mutant emits no FLAIR-MENU marker. ----
@$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_menu2_dead.img" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK \
	--name flair_solid_mut_menu2_dead_E --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_MENU2_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-LIVE-OK" --timeout-ms 15000 \
	2> "$(BUILD)/flair_solid_mut_menu2_dead_E.report" || true
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_solid_mut_menu2_dead_E.serial"
@if grep -q 'triple_fault=1' "$(BUILD)/flair_solid_mut_menu2_dead_E.report"; then printf '!!! test-flair-solid-mutant FAIL: menu2_dead TRIPLE-FAULTED (cannot judge)\n'; exit 1; fi
@if grep -q '^FLAIR-MENU-DROP ' "$(BUILD)/flair_solid_mut_menu2_dead_E.serial"; then printf '!!! test-flair-solid-mutant FAIL: menu2_dead is not faithful -- it unexpectedly dropped a menu\n'; exit 1; fi
@if [ ! -s "$(BUILD)/flair_solid_mut_menu2_dead_E.ppm" ]; then printf '!!! test-flair-solid-mutant FAIL: menu2_dead produced no LIVE-OK screendump\n'; exit 1; fi
@if $(PPM_FLAIR_SOLID_CHECK_BIN) E "$(BUILD)/flair_solid_mut_menu2_dead_E.ppm" > "$(BUILD)/flair_solid_mut_menu2_dead_E.chk" 2>&1; then \
	printf '!!! test-flair-solid-mutant FAIL: the leg E pixel oracle is DECORATION -- menu2_dead PASSED it\n'; exit 1; \
else \
	printf '>>> menu2_dead leg E correctly RED (no drop marker; teal remains):\n'; \
	grep -m2 'FAIL ' "$(BUILD)/flair_solid_mut_menu2_dead_E.chk" | sed 's/^/      /'; \
fi
@# ---- menu2_bar_sys: panel pixels stay GREEN, but menu=256 tooth MUST RED. ----
@$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_menu2_bar_sys.img" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK \
	--name flair_solid_mut_menu2_bar_sys_E --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_MENU2_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-MENU-DROP" --timeout-ms 15000 \
	2> "$(BUILD)/flair_solid_mut_menu2_bar_sys_E.report" || true
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_solid_mut_menu2_bar_sys_E.serial"
@if grep -q 'triple_fault=1' "$(BUILD)/flair_solid_mut_menu2_bar_sys_E.report"; then printf '!!! test-flair-solid-mutant FAIL: menu2_bar_sys TRIPLE-FAULTED (cannot judge)\n'; exit 1; fi
@if [ ! -s "$(BUILD)/flair_solid_mut_menu2_bar_sys_E.ppm" ]; then printf '!!! test-flair-solid-mutant FAIL: menu2_bar_sys produced no screendump\n'; exit 1; fi
@$(PPM_FLAIR_SOLID_CHECK_BIN) E "$(BUILD)/flair_solid_mut_menu2_bar_sys_E.ppm" >/dev/null 2>&1 \
	|| { printf '!!! test-flair-solid-mutant FAIL: menu2_bar_sys did not drop a panel at y=40 -- wrong failure axis\n'; exit 1; }
@if grep -q '^FLAIR-MENU-DROP menu=256$$' "$(BUILD)/flair_solid_mut_menu2_bar_sys_E.serial"; then \
	printf '!!! test-flair-solid-mutant FAIL: the leg E routing tooth is DECORATION -- menu2_bar_sys still emitted menu=256\n'; exit 1; \
elif ! grep -q '^FLAIR-MENU-DROP menu=128$$' "$(BUILD)/flair_solid_mut_menu2_bar_sys_E.serial"; then \
	printf '!!! test-flair-solid-mutant FAIL: menu2_bar_sys lacks the expected menu=128 proof -- wrong failure axis\n'; grep '^FLAIR-' "$(BUILD)/flair_solid_mut_menu2_bar_sys_E.serial" || true; exit 1; \
elif ! grep -q '^FLAIR-MENU menu=128 item=2 (sel=0x00800002)$$' "$(BUILD)/flair_solid_mut_menu2_bar_sys_E.serial"; then \
	printf '!!! test-flair-solid-mutant FAIL: menu2_bar_sys did not complete File item 2 selection\n'; grep '^FLAIR-' "$(BUILD)/flair_solid_mut_menu2_bar_sys_E.serial" || true; exit 1; \
else \
	printf '>>> menu2_bar_sys leg E routing tooth correctly RED: panel dropped, but serial says menu=128 (expected 256)\n'; \
fi
@# ---- no_drag_clamp: leg G MUST go RED. ----
@$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_no_drag_clamp.img" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK \
	--name flair_solid_mut_dragclamp_G --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_CLAMP_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-DRAG win" --timeout-ms 20000 \
	2> "$(BUILD)/flair_solid_mut_dragclamp_G.report" || true
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_solid_mut_dragclamp_G.serial"
@if grep -q 'triple_fault=1' "$(BUILD)/flair_solid_mut_dragclamp_G.report"; then printf '!!! test-flair-solid-mutant FAIL: no_drag_clamp TRIPLE-FAULTED (cannot judge)\n'; exit 1; fi
@if [ ! -s "$(BUILD)/flair_solid_mut_dragclamp_G.ppm" ]; then printf '!!! test-flair-solid-mutant FAIL: no_drag_clamp produced no screendump\n'; exit 1; fi
@if $(PPM_FLAIR_SOLID_CHECK_BIN) G "$(BUILD)/flair_solid_mut_dragclamp_G.ppm" > "$(BUILD)/flair_solid_mut_dragclamp_G.chk" 2>&1; then \
	printf '!!! test-flair-solid-mutant FAIL: the leg G oracle is DECORATION -- no_drag_clamp PASSED it\n'; exit 1; \
else \
	printf '>>> no_drag_clamp leg G correctly RED:\n'; \
	grep -m2 'FAIL ' "$(BUILD)/flair_solid_mut_dragclamp_G.chk" | sed 's/^/      /'; \
fi
@# ---- no_raise_on_title: leg H MUST go RED. ----
@$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_no_raise_on_title.img" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK \
	--name flair_solid_mut_title_raise_H --out "$(BUILD)" \
	--mouse "$(FLAIR_SOLID_RAISE_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-DRAG win" --timeout-ms 20000 \
	2> "$(BUILD)/flair_solid_mut_title_raise_H.report" || true
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_solid_mut_title_raise_H.serial"
@if grep -q 'triple_fault=1' "$(BUILD)/flair_solid_mut_title_raise_H.report"; then printf '!!! test-flair-solid-mutant FAIL: no_raise_on_title TRIPLE-FAULTED (cannot judge)\n'; exit 1; fi
@if [ ! -s "$(BUILD)/flair_solid_mut_title_raise_H.ppm" ]; then printf '!!! test-flair-solid-mutant FAIL: no_raise_on_title produced no screendump\n'; exit 1; fi
@if $(PPM_FLAIR_SOLID_CHECK_BIN) H "$(BUILD)/flair_solid_mut_title_raise_H.ppm" > "$(BUILD)/flair_solid_mut_title_raise_H.chk" 2>&1; then \
	printf '!!! test-flair-solid-mutant FAIL: the leg H oracle is DECORATION -- no_raise_on_title PASSED it\n'; exit 1; \
else \
	printf '>>> no_raise_on_title leg H correctly RED:\n'; \
	grep -m2 'FAIL ' "$(BUILD)/flair_solid_mut_title_raise_H.chk" | sed 's/^/      /'; \
fi
@printf 'VERDICT   : PASS -- all eight solidity mutants drive their legs RED (the gate bites; Rule 6)\n'
@printf '======================================================================\n'
```

## test-flair-tbxhardening-host

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 64):
Prerequisites: `test-control test-control-mutant test-list test-list-mutant test-resource test-resource-mutant test-scrap test-scrap-mutant test-flair-heap test-flair-heap-mutant test-process test-process-mutant test-process-teardown test-process-teardown-mutant test-process-budget test-process-budget-mutant test-process-activate test-process-activate-mutant test-process-update test-process-update-mutant test-flair-shell test-flair-shell-mutant test-blitter test-blitter-mutant test-fbagree test-event test-event-mutant test-region test-region-mutant test-region-gdi test-region-gdi-mutant test-mouse-producer test-mouse-producer-mutant test-kbd-unit test-kbd-unit-mutant test-stdfile test-stdfile-mutant test-text test-flair-tbxhardening-mutant build/.makefile.stamp`
```make
@printf '>>> test-flair-tbxhardening-host: green\n'
```

## test-flair-tbxhardening-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 54):
Prerequisites: `test-tbx-mutant-LIST_MUT_NULL_CLICK_MISS test-tbx-mutant-LIST_MUT_ROW_WRAP test-tbx-mutant-LIST_MUT_NO_OVERFLOW_CHECK test-tbx-mutant-LIST_MUT_COLUMN_HEADROOM test-tbx-mutant-RES_MUT_MAP_PREAMBLE test-tbx-mutant-RES_MUT_TYPE_WRAP test-tbx-mutant-RES_MUT_DATA_WRAP test-tbx-mutant-RES_MUT_DATA_ESCAPE test-tbx-mutant-SCRAP_MUT_FULL_FLAVOR test-tbx-mutant-SCRAP_MUT_NULL_DATA test-tbx-mutant-FLAIR_HEAP_MUTATE_ALIGN_OVERFLOW test-tbx-mutant-FLAIR_HEAP_MUTATE_SPAN_OVERFLOW test-tbx-mutant-FLAIR_HEAP_MUTATE_WALK_REUSE test-tbx-mutant-PROC_MUT_REGISTERED_FREE test-tbx-mutant-PROC_MUT_STALE_MAGIC test-tbx-mutant-CTRL_MUT_NULL_SILENT test-tbx-mutant-CTRL_MUT_DISABLED_HIT test-tbx-mutant-CTRL_MUT_SHORT_BUTTON test-tbx-mutant-CTRL_MUT_SHORT_LIVE test-tbx-mutant-SHELL_MUT_MENU_COUNT test-tbx-mutant-SHELL_MUT_SHORT_SURFACE test-tbx-mutant-SURFACE_MUT_BGR_SWAP test-tbx-mutant-BLITTER_MUT_COPY32_ZERO test-tbx-mutant-SURFACE_MUT_INDEX_ZERO test-tbx-mutant-SURFACE_MUT_CLEAR_FLAT test-tbx-mutant-EVENT_MUT_TICK_WRAP test-tbx-mutant-EVENT_MUT_SILENT_DROP test-tbx-mutant-KBD_MUT_SILENT_DROP test-tbx-mutant-MOUSE_MUT_SIGN_BYTE test-tbx-mutant-MOUSE_MUT_EARLY_BUTTON test-tbx-mutant-SF_MUT_SWALLOW_NAV_ERROR test-tbx-mutant-RGN_MUT_ALIAS_RESET test-tbx-mutant-RGN_MUT_REPEATED_UNION build/.makefile.stamp`
```make
@printf '>>> test-flair-tbxhardening-mutant: green (%s named mutants RED)\n' '$(words $(TBX_HARDENING_MUTANTS))'
```

## test-flair-tenant-menu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23465):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_app_launch_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-tenant-menu : a disk tenant hears its own menu choices\n'
@printf '  bead initech-tdnl.31; spec/toolbox_gate.h Sec 6a (TBX_EVT_MENU) + Sec 9 V3 (DRAWMENUBAR)\n'
@printf '======================================================================\n'
$(call appl-boot,flair_tmenu_pre,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_APP_LAUNCH_PRE_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_tmenu_pre.serial) && ! grep -q '^TENANT-' $(BUILD)/flair_tmenu_pre.serial || { printf '!!! test-flair-tenant-menu [4/4] FAIL: the never-launched baseline did not come up clean\n'; exit 1; }
$(call appl-boot,flair_tmenu_qmouse,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_TMENU_QUIT_MOUSE_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_tmenu_qmouse.serial) && $(call TMENU_QMOUSE,$(BUILD)/flair_tmenu_qmouse.serial) || { printf '!!! test-flair-tenant-menu [1/4] FAIL: QUIT-MOUSE -- File > Quit picked in the tenant bar must reach the tenant (TENANT-MENU src=mouse, what=81) and quit it\n'; grep -E '^(TENANT-|FLAIR-MENU|FLAIR-DISPATCH|PANIC)' $(BUILD)/flair_tmenu_qmouse.serial; exit 1; }
@cmp -s $(BUILD)/flair_tmenu_pre.ppm $(BUILD)/flair_tmenu_qmouse.ppm || { printf '!!! test-flair-tenant-menu [1/4] FAIL: QUIT-MOUSE -- the post-quit frame differs from the never-launched baseline (%s bytes)\n' "$$(cmp -l $(BUILD)/flair_tmenu_pre.ppm $(BUILD)/flair_tmenu_qmouse.ppm | wc -l)"; exit 1; }
@$(PPM_FLAIR_APPL_CHECK_BIN) bar-finder $(BUILD)/flair_tmenu_qmouse.ppm >/dev/null || { printf '!!! test-flair-tenant-menu [1/4] FAIL: QUIT-MOUSE -- band 2 is not the Finder bar after the quit\n'; exit 1; }
@printf '>>> test-flair-tenant-menu [1/4] QUIT-MOUSE: File > Quit -> TENANT-MENU 129/1 src=mouse -> what=81 -> EXIT(0); the frame is byte-identical to the never-launched baseline, band 2 the Finder bar\n'
$(call appl-boot,flair_tmenu_qkey,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_TMENU_QUIT_KEY_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_tmenu_qkey.serial) && $(call TMENU_QKEY,$(BUILD)/flair_tmenu_qkey.serial) || { printf '!!! test-flair-tenant-menu [2/4] FAIL: QUIT-KEY -- Ctrl-Q must go the route the mouse takes (TENANT-MENU src=key, what=81, EXIT) and never reach the tenant as a plain keyDown\n'; grep -E '^(TENANT-|FLAIR-EVT what=3|FLAIR-DISPATCH|PANIC)' $(BUILD)/flair_tmenu_qkey.serial; exit 1; }
@sed 's/ src=mouse$$/ src=key/' $(BUILD)/flair_tmenu_qmouse.serial.tail | grep -v '^FLAIR-MENU ' | cmp -s - $(BUILD)/flair_tmenu_qkey.serial.tail || { printf '!!! test-flair-tenant-menu [2/4] FAIL: QUIT-KEY -- the key route is not the mouse route (tails differ beyond src=)\n'; exit 1; }
@cmp -s $(BUILD)/flair_tmenu_pre.ppm $(BUILD)/flair_tmenu_qkey.ppm || { printf '!!! test-flair-tenant-menu [2/4] FAIL: QUIT-KEY -- the post-quit frame differs from the never-launched baseline (%s bytes)\n' "$$(cmp -l $(BUILD)/flair_tmenu_pre.ppm $(BUILD)/flair_tmenu_qkey.ppm | wc -l)"; exit 1; }
@printf '>>> test-flair-tenant-menu [2/4] QUIT-KEY: Ctrl-Q -> the SAME TENANT-MENU 129/1 -> what=81 -> EXIT(0) tail as the mouse (src=key), no plain keyDown delivered; frame byte-identical to the baseline\n'
$(call appl-boot,flair_tmenu_about,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_TMENU_ABOUT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_tmenu_about.serial) && $(call TMENU_ABOUT,$(BUILD)/flair_tmenu_about.serial) && ! grep -q '^TENANT-EXIT' $(BUILD)/flair_tmenu_about.serial || { printf '!!! test-flair-tenant-menu [3/4] FAIL: ABOUT -- Fixture > About must reach the tenant, which draws its About run, SETMBARs mbar2 after its window and calls DRAWMENUBAR\n'; grep -E '^(TENANT-|FLAIR-MENU|FLAIR-DISPATCH|PANIC)' $(BUILD)/flair_tmenu_about.serial; exit 1; }
@$(PPM_FLAIR_APPL_CHECK_BIN) about $(BUILD)/flair_tmenu_about.ppm || { printf '!!! test-flair-tenant-menu [3/4] FAIL: ABOUT -- the About run is not on screen exactly\n'; exit 1; }
@$(PPM_FLAIR_APPL_CHECK_BIN) bar-tenant2 $(BUILD)/flair_tmenu_about.ppm || { printf '!!! test-flair-tenant-menu [3/4] FAIL: ABOUT -- band 2 is not the bar the tenant installed after its window (DRAWMENUBAR)\n'; exit 1; }
@printf '>>> test-flair-tenant-menu [3/4] ABOUT: Fixture > About -> the About run drawn exactly; SETMBAR after the window + DRAWMENUBAR -> band 2 is exactly File Edit Fixture Info\n'
@printf '>>> test-flair-tenant-menu [4/4] PRE: the never-launched baseline both quit legs were compared against\n'
@printf '>>> test-flair-tenant-menu: green\n'
```

## test-flair-tenant-menu-bochs

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23516):
Prerequisites: `build/bochs_harness build/qemu_harness build/flair_tenants_mut_tbx_smoke.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-tenant-menu-bochs : Rule 5 menu-route CPU-path differential\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-tenant-menu-bochs FAIL: bochs not found (a skipped oracle is worse than a red one; SKIP_BOCHS=1 to opt out loudly)\n'; exit 1; }
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(BUILD)/flair_tmenu_smoke_qemu_data.img
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(BUILD)/flair_tmenu_smoke_qemu_data.img,$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(BUILD)/flair_tmenu_smoke_bochs_data.img
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(BUILD)/flair_tmenu_smoke_bochs_data.img,$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIR_APPL_SMOKE_IMG)" --disk2 "$(BUILD)/flair_tmenu_smoke_qemu_data.img" --expect HALTED \
	--name flair_tmenu_smoke_qemu --out "$(BUILD)" --timeout-ms 20000 2> $(BUILD)/flair_tmenu_smoke_qemu.report || true
@$(BOCHS_BIN) --disk "$(FLAIR_APPL_SMOKE_IMG)" --disk2 "$(BUILD)/flair_tmenu_smoke_bochs_data.img" --expect HALTED \
	--name flair_tmenu_smoke_bochs --out "$(BUILD)" --timeout-ms 60000 2> $(BUILD)/flair_tmenu_smoke_bochs.report || true
@if grep -q 'triple_fault=1' $(BUILD)/flair_tmenu_smoke_qemu.report $(BUILD)/flair_tmenu_smoke_bochs.report; then printf '!!! test-flair-tenant-menu-bochs FAIL: TRIPLE FAULT\n'; exit 1; fi
@for e in qemu bochs; do \
	f=$(BUILD)/flair_tmenu_smoke_$$e.serial; \
	grep -qx 'TBX-SMOKE-OK list-empty' $$f && ! grep -q '^PANIC' $$f && ! grep -q '^TBX-SMOKE-FAIL' $$f || { printf '!!! test-flair-tenant-menu-bochs FAIL: %s did not complete the smoke cleanly\n' $$e; grep -E '^(TBX-|TENANT-|PANIC|HALTED)' $$f; exit 1; }; \
	grep -E '^(TENANT-MENU|TENANT-EVT what=81$$|TENANT-GATE ax=0x005[02] |TENANT-GATE ax=0x0070 |TENANT-EXIT|TBX-SMOKE-MBAR-REDRAW)' $$f | sed -n '/^TENANT-MENU menu=131/,$$p' > $$f.menu; \
	printf '$(TMENU_SMOKE_WANT)' | cmp -s - $$f.menu || { printf '!!! test-flair-tenant-menu-bochs FAIL: %s menu-route lines are not the expected sequence\n' $$e; cat $$f.menu; exit 1; }; \
	grep -E '^(TENANT-|TBX-SMOKE-)' $$f > $$f.trace; \
done
@cmp -s $(BUILD)/flair_tmenu_smoke_qemu.serial.trace $(BUILD)/flair_tmenu_smoke_bochs.serial.trace || { printf '!!! test-flair-tenant-menu-bochs FAIL: QEMU and Bochs disagree on the menu route (stop condition: investigate, do not pin to QEMU)\n'; diff -u $(BUILD)/flair_tmenu_smoke_qemu.serial.trace $(BUILD)/flair_tmenu_smoke_bochs.serial.trace; exit 1; }
@printf '>>> test-flair-tenant-menu-bochs: the menu route (About by mouse -> SETMBAR after the window + DRAWMENUBAR; Ctrl-Q by key -> EXIT) and all %s TENANT-*/TBX-SMOKE-* lines byte-identical on QEMU and Bochs\n' "$$(wc -l < $(BUILD)/flair_tmenu_smoke_qemu.serial.trace)"
@printf '>>> test-flair-tenant-menu-bochs: green\n'
```

## test-flair-tenant-menu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23492):
Prerequisites: `test-flair-tenant-menu build/qemu_harness build/flair_data.img build/flair_tenants_mut_tbx_menu_key_plain.img build/flair_tenants_mut_tbx_menu_not_delivered.img build/flair_tenants_mut_tbx_drawmbar_noop.img build/.makefile.stamp`
```make
$(call appl-boot,flair_tmenumut_key,$(BUILD)/flair_tenants_mut_tbx_menu_key_plain.img,$(FLAIR_DATA_IMG),FLAIR_TMENU_QUIT_KEY_SPEC,)
@if $(call TMENU_QKEY,$(BUILD)/flair_tmenumut_key.serial); then printf '!!! test-flair-tenant-menu-mutant FAIL: MENU_KEY_PLAIN PASSED the QUIT-KEY leg -- the key-route oracle is decoration\n'; exit 1; fi
@$(call APPL_ALIVE,$(BUILD)/flair_tmenumut_key.serial) && ! grep -q '^TENANT-MENU' $(BUILD)/flair_tmenumut_key.serial && ! grep -q '^TENANT-EXIT' $(BUILD)/flair_tmenumut_key.serial && awk '/^FLAIR-DISPATCH app=TENANTFX$$/{a=1} a&&/^TENANT-EVT what=3$$/{b=1} END{exit !b}' $(BUILD)/flair_tmenumut_key.serial || { printf '!!! test-flair-tenant-menu-mutant FAIL: MENU_KEY_PLAIN went RED for the wrong reason (want: Ctrl-Q delivered as a plain keyDown, no TENANT-MENU, the tenant still resident)\n'; grep -E '^(TENANT-|FLAIR-DISPATCH|PANIC)' $(BUILD)/flair_tmenumut_key.serial; exit 1; }
@printf '>>> test-flair-tenant-menu-mutant: MENU_KEY_PLAIN RED for the named reason -- Ctrl-Q reached the tenant as a plain keyDown (TENANT-EVT what=3), no TENANT-MENU, no quit (the audit G03 symptom)\n'
$(call appl-boot,flair_tmenumut_mouse,$(BUILD)/flair_tenants_mut_tbx_menu_not_delivered.img,$(FLAIR_DATA_IMG),FLAIR_TMENU_QUIT_MOUSE_SPEC,)
@if $(call TMENU_QMOUSE,$(BUILD)/flair_tmenumut_mouse.serial); then printf '!!! test-flair-tenant-menu-mutant FAIL: MENU_NOT_DELIVERED PASSED the QUIT-MOUSE leg -- the mouse-route oracle is decoration\n'; exit 1; fi
@$(call APPL_ALIVE,$(BUILD)/flair_tmenumut_mouse.serial) && grep -qx 'FLAIR-MENU menu=129 item=1 (sel=0x00810001)' $(BUILD)/flair_tmenumut_mouse.serial && ! grep -q '^TENANT-MENU' $(BUILD)/flair_tmenumut_mouse.serial && ! grep -q '^TENANT-EXIT' $(BUILD)/flair_tmenumut_mouse.serial || { printf '!!! test-flair-tenant-menu-mutant FAIL: MENU_NOT_DELIVERED went RED for the wrong reason (want: the shell logged File > Quit, the tenant never heard it, no quit)\n'; grep -E '^(TENANT-|FLAIR-MENU|FLAIR-DISPATCH|PANIC)' $(BUILD)/flair_tmenumut_mouse.serial; exit 1; }
@printf '>>> test-flair-tenant-menu-mutant: MENU_NOT_DELIVERED RED for the named reason -- FLAIR-MENU logged 129/1 but the tenant never heard it and did not quit (the pre-tdnl.31 behaviour)\n'
$(call appl-boot,flair_tmenumut_draw,$(BUILD)/flair_tenants_mut_tbx_drawmbar_noop.img,$(FLAIR_DATA_IMG),FLAIR_TMENU_ABOUT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@if $(PPM_FLAIR_APPL_CHECK_BIN) bar-tenant2 $(BUILD)/flair_tmenumut_draw.ppm >/dev/null 2>&1; then printf '!!! test-flair-tenant-menu-mutant FAIL: DRAWMBAR_NOOP PASSED the band-2 grade -- the DRAWMENUBAR oracle is decoration\n'; exit 1; fi
@$(call APPL_ALIVE,$(BUILD)/flair_tmenumut_draw.serial) && $(call TMENU_ABOUT,$(BUILD)/flair_tmenumut_draw.serial) && $(PPM_FLAIR_APPL_CHECK_BIN) about $(BUILD)/flair_tmenumut_draw.ppm >/dev/null && $(PPM_FLAIR_APPL_CHECK_BIN) bar-tenant $(BUILD)/flair_tmenumut_draw.ppm >/dev/null || { printf '!!! test-flair-tenant-menu-mutant FAIL: DRAWMBAR_NOOP went RED for the wrong reason (want: the About run drawn, DRAWMENUBAR said 0, band 2 still the OLD tenant bar)\n'; grep -E '^(TENANT-|FLAIR-MENU|PANIC)' $(BUILD)/flair_tmenumut_draw.serial; exit 1; }
@printf '>>> test-flair-tenant-menu-mutant: DRAWMBAR_NOOP RED for the named reason -- About answered and DRAWMENUBAR returned 0, but band 2 is still the old File Edit Fixture bar\n'
@printf '>>> test-flair-tenant-menu-mutant: green (all three mutants RED for their named reasons)\n'
```

## test-flair-trash-edges

Declared tier: EMU; #  recipe to execute (from 'harness/finder_trash_edges.mk', line 114):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_hold.img build/finder_full_fixture.img build/flair_data.img build/ppm_flair_trash_check build/ppm_flair_finder_icons_check build/.makefile.stamp`
```make
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
```

## test-flair-trash-edges-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/finder_trash_edges.mk', line 151):
Prerequisites: `test-flair-trash-edges build/flair_tenants_mut_te_no_delete_now.img build/flair_tenants_mut_te_notrash.img build/flair_tenants_mut_te_putaway.img build/flair_tenants_mut_te_newfolder.img build/.makefile.stamp`
```make
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
```

## test-flair-trash-full

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 21909):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_finder_icons_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-trash-full : the FULL Trash + the caution alert icon\n'
@printf '  Ref: beads initech-tdnl.74 / tdnl.87; ../system7-decomp/re/mint-results-008.md.\n'
@printf '======================================================================\n'
$(call fo-boot,$(FLAIRTENANTS_IMG),flair_trash_full,$(BUILD)/flair_trash_full_data.img,$(FLAIR_TRASH_FULL_SPEC),DESKTOP-DB-SAVE n=3,1)
$(call fo-has,flair_trash_full,DESK-TRASH-ICON full,staging README.TXT did not turn the Trash FULL)
$(call fo-has,flair_trash_full,FINDER-TRASH name=README.TXT origin=0,README.TXT was not staged)
@mdir -i $(BUILD)/flair_trash_full_data.img ::/TRASH | grep -q '^README   TXT ' || { printf '!!! test-flair-trash-full FAIL: mtools: README.TXT is not in ::/TRASH\n'; exit 1; }
@$(PPM_FLAIR_ICONS_CHECK_BIN) "$(ICON_CAPTURES_DIR)" "$(BUILD)/flair_trash_full.ppm" $(FLAIR_TRASH_FULL_CELL) \
	|| { printf '!!! test-flair-trash-full FAIL: the staged Trash is not the captured FULL can\n'; exit 1; }
@printf '>>> test-flair-trash-full [1/2]: README.TXT staged (mtools) -> the desktop Trash == the captured FULL can, 0/1024\n'
$(call fo-boot,$(FLAIRTENANTS_IMG),flair_trash_full_alert,$(BUILD)/flair_trash_full_alert_data.img,$(FLAIR_EMPTY_ALERT_SPEC),FINDER-TRASH-ALERT n=,1)
@grep -q '^FINDER-TRASH-ALERT n=1 ' "$(BUILD)/flair_trash_full_alert.serial" || { printf '!!! test-flair-trash-full FAIL: the Empty Trash confirm did not come up\n'; exit 1; }
@$(PPM_FLAIR_ICONS_CHECK_BIN) "$(ICON_CAPTURES_DIR)" "$(BUILD)/flair_trash_full_alert.ppm" $(FLAIR_CAUTION_CELL) $(FLAIR_TRASH_FULL_CELL) \
	|| { printf '!!! test-flair-trash-full FAIL: the confirm alert does not carry the captured CAUTION icon (or the Trash is not FULL)\n'; exit 1; }
@printf '>>> test-flair-trash-full [2/2]: Empty Trash confirm up -> the CAUTION icon at (156,100) == the capture, 0/1024; the Trash still FULL\n'
@printf '>>> test-flair-trash-full: green\n'
```

## test-flair-trash-full-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 223):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-trash-full-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-trash-full-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-trash-full_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-trash-full_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-trash-full_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-trash-full_bochs.report.txt" || true
@cat "$(BUILD)/flair-trash-full_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-trash-full-bochs "$(BUILD)/flair-trash-full_bochs.report.txt" "$(BUILD)/flair-trash-full_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-trash-full-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 21928):
Prerequisites: `build/qemu_harness build/flair_data.img build/ppm_flair_finder_icons_check build/flair_tenants_mut_trash_never_full.img build/flair_tenants_mut_dialog_no_icon.img build/.makefile.stamp`
```make
$(call fo-boot,$(BUILD)/flair_tenants_mut_trash_never_full.img,flair_trash_full_mut_never,$(BUILD)/flair_trash_full_mut_never_data.img,$(FLAIR_TRASH_FULL_SPEC),DESKTOP-DB-SAVE n=3,1)
@grep -qxF 'DESK-TRASH-ICON full' $(BUILD)/flair_trash_full_mut_never.serial || { printf '!!! test-flair-trash-full-mutant: the mutant never staged (not comparable)\n'; exit 1; }
@rc=0; $(PPM_FLAIR_ICONS_CHECK_BIN) "$(ICON_CAPTURES_DIR)" "$(BUILD)/flair_trash_full_mut_never.ppm" $(FLAIR_TRASH_FULL_CELL) > $(BUILD)/flair_trash_full_mut_never.grade 2>&1 || rc=$$?; \
[ "$$rc" = 1 ] && grep -q 'TRASH_FULL UNSEL at (584,404) on #8DDCDC: [1-9][0-9]*/1024' $(BUILD)/flair_trash_full_mut_never.grade \
	|| { cat $(BUILD)/flair_trash_full_mut_never.grade; printf '!!! test-flair-trash-full-mutant FAIL: TRASH_NEVER_FULL passed the FULL cell\n'; exit 1; }
@printf '>>> FINDER_DESK_MUT_TRASH_NEVER_FULL: RED -- %s\n' "$$(grep 'TRASH_FULL UNSEL at' $(BUILD)/flair_trash_full_mut_never.grade | sed 's/^ *//')"
$(call fo-boot,$(BUILD)/flair_tenants_mut_dialog_no_icon.img,flair_trash_full_mut_noicon,$(BUILD)/flair_trash_full_mut_noicon_data.img,$(FLAIR_EMPTY_ALERT_SPEC),FINDER-TRASH-ALERT n=,1)
@grep -q '^FINDER-TRASH-ALERT n=1 ' $(BUILD)/flair_trash_full_mut_noicon.serial || { printf '!!! test-flair-trash-full-mutant: the mutant alert never came up (not comparable)\n'; exit 1; }
@rc=0; $(PPM_FLAIR_ICONS_CHECK_BIN) "$(ICON_CAPTURES_DIR)" "$(BUILD)/flair_trash_full_mut_noicon.ppm" $(FLAIR_CAUTION_CELL) $(FLAIR_TRASH_FULL_CELL) > $(BUILD)/flair_trash_full_mut_noicon.grade 2>&1 || rc=$$?; \
[ "$$rc" = 1 ] && grep -q 'CAUTION UNSEL at (156,100) on #E7E7E7: 572/1024' $(BUILD)/flair_trash_full_mut_noicon.grade && grep -q 'TRASH_FULL UNSEL at (584,404) on #8DDCDC: 0/1024' $(BUILD)/flair_trash_full_mut_noicon.grade \
	|| { cat $(BUILD)/flair_trash_full_mut_noicon.grade; printf '!!! test-flair-trash-full-mutant FAIL: DIALOG_MUT_NO_ICON did not turn exactly the CAUTION cell RED\n'; exit 1; }
@printf '>>> DIALOG_MUT_NO_ICON: RED -- the CAUTION cell 572/1024 (every opaque cell missing), the FULL Trash still 0/1024\n'
```

## test-flair-trash-purge-views

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22032):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_purge_views_data.img build/ppm_flair_disk_windows_check build/.makefile.stamp`
```make
@printf 'InitechOS (STAPLER) -- make test-flair-trash-purge-views : Empty Trash retires every view of a removed folder (bead initech-uios, audit L002)\n'
@command -v mdir >/dev/null 2>&1 && command -v mshowfat >/dev/null 2>&1 && command -v minfo >/dev/null 2>&1 && command -v fsck.fat >/dev/null 2>&1 || { printf '!!! test-flair-trash-purge-views FAIL: mtools / fsck.fat (dosfstools) missing\n'; exit 1; }
@sh $(FLAIR_PURGE_VIEWS_SH) $(FLAIRTENANTS_IMG) $(FLAIR_PURGE_VIEWS_DATA) '$(FLAIR_TRASH_PURGE_VIEWS_SPEC)' purge
@sh $(FLAIR_PURGE_VIEWS_SH) $(FLAIRTENANTS_IMG) $(FLAIR_PURGE_VIEWS_DATA) '$(FLAIR_TRASH_PURGE_CTRLN_SPEC)' ctrln
@printf '>>> test-flair-trash-purge-views: green\n'
```

## test-flair-trash-purge-views-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22039):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_purge_keeps_views.img build/flair_tenants_mut_l002.img build/flair_purge_views_data.img build/ppm_flair_disk_windows_check build/.makefile.stamp`
```make
@if sh $(FLAIR_PURGE_VIEWS_SH) $(BUILD)/flair_tenants_mut_purge_keeps_views.img $(FLAIR_PURGE_VIEWS_DATA) '$(FLAIR_TRASH_PURGE_VIEWS_SPEC)' purge _mut_keep > $(BUILD)/flair_purge_views_mut_keep.log 2>&1; then printf '!!! test-flair-trash-purge-views-mutant FAIL: PURGE_KEEPS_VIEWS PASSED -- decoration\n'; exit 1; fi
@grep -q "FAIL trash-purge-views purge: a purged folder's window is still drawn" $(BUILD)/flair_purge_views_mut_keep.log || { printf '!!! test-flair-trash-purge-views-mutant FAIL: PURGE_KEEPS_VIEWS went RED for the wrong reason\n'; cat $(BUILD)/flair_purge_views_mut_keep.log; exit 1; }
@printf ">>> test-flair-trash-purge-views-mutant: PURGE_KEEPS_VIEWS correctly RED (the purged folders' windows are still drawn)\n"
@if sh $(FLAIR_PURGE_VIEWS_SH) $(BUILD)/flair_tenants_mut_l002.img $(FLAIR_PURGE_VIEWS_DATA) '$(FLAIR_TRASH_PURGE_CTRLN_SPEC)' ctrln _mut_l002 FINDER-NEW-FOLDER > $(BUILD)/flair_purge_views_mut_l002.log 2>&1; then printf '!!! test-flair-trash-purge-views-mutant FAIL: the restored L002 defect PASSED -- decoration\n'; exit 1; fi
@grep -q 'FAIL trash-purge-views ctrln: fsck.fat -n: an allocated cluster is unreachable (orphan)' $(BUILD)/flair_purge_views_mut_l002.log || { printf '!!! test-flair-trash-purge-views-mutant FAIL: the restored L002 defect went RED for the wrong reason\n'; cat $(BUILD)/flair_purge_views_mut_l002.log; exit 1; }
@grep -m1 '^FINDER-NEW-FOLDER' $(BUILD)/flair_purge_views_ctrln_mut_l002.serial | sed 's/^/    mutant serial: /' || true
@printf ">>> test-flair-trash-purge-views-mutant: L002 RESTORED correctly RED (Ctrl-N wrote into the purged folder's freed cluster; fsck.fat finds the orphan)\n"
```

## test-flair-trash-window

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 21624):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_disk_windows_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-trash-window : the Trash OPENS (bead initech-tdnl.39)\n'
@printf '  Ref: os/flair/finder_windows.h finder_win_open_trash; spec/flair_trash_traces.mk.\n'
@printf '  File effects graded by mtools on the SAME image (the FAT rail, Law 2).\n'
@printf '======================================================================\n'
@command -v mdir >/dev/null 2>&1 && command -v mshowfat >/dev/null 2>&1 || { printf '!!! test-flair-trash-window FAIL: mtools (mdir/mtype/mcopy/mshowfat) missing\n'; exit 1; }
@# ---- leg 1: OPEN the Trash ----
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_TW_OPEN_NAME),$(BUILD)/$(FLAIR_TW_OPEN_NAME)_data.img,$(FLAIR_TRASHWIN_OPEN_SPEC),FINDER-OPEN-TRASH win=,1)
$(call fo-has,$(FLAIR_TW_OPEN_NAME),FINDER-OPEN-VOLUME win=0 n=4,the root window did not open showing README.TXT + APPS only (TRASH must stay hidden))
$(call fo-has,$(FLAIR_TW_OPEN_NAME),FINDER-TRASH name=README.TXT origin=0,README.TXT was not staged)
$(call fo-has,$(FLAIR_TW_OPEN_NAME),FINDER-OPEN-TRASH win=1 n=1 singleton=0,the Trash double-click did not open a NEW Trash window in slot 1 listing the one staged item)
@! grep -q 'NYI' "$(BUILD)/$(FLAIR_TW_OPEN_NAME).serial" || { printf '!!! test-flair-trash-window FAIL: an NYI line on serial\n'; grep NYI "$(BUILD)/$(FLAIR_TW_OPEN_NAME).serial"; exit 1; }
@n=$$($(call tw-trash-entries,$(BUILD)/$(FLAIR_TW_OPEN_NAME)_data.img)); [ "$$n" = 1 ] && mdir -i $(BUILD)/$(FLAIR_TW_OPEN_NAME)_data.img ::/TRASH | grep -q '^README   TXT ' || { printf '!!! test-flair-trash-window FAIL: mtools lists %s entries in ::/TRASH (want exactly README.TXT, = the window n=1)\n' "$$n"; exit 1; }
@[ -s "$(BUILD)/$(FLAIR_TW_OPEN_NAME).ppm" ] || { printf '!!! test-flair-trash-window FAIL: OPEN screendump missing\n'; exit 1; }
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) trashwin "$(BUILD)/$(FLAIR_TW_OPEN_NAME).ppm" || { printf '!!! test-flair-trash-window FAIL: the Trash window is not drawn as graded (chrome at (40,80), "Trash" title, README.TXT in cell 0, cells 1-3 bare)\n'; exit 1; }
@printf '>>> test-flair-trash-window [1/2]: OPEN -- FINDER-OPEN-TRASH win=1 n=1 (mtools: ::/TRASH holds exactly README.TXT); pixel-graded window\n'
@# ---- leg 2: PUT BACK (drag out onto the volume) ----
$(call fo-boot,$(FLAIRTENANTS_IMG),$(FLAIR_TW_PUTBACK_NAME),$(BUILD)/$(FLAIR_TW_PUTBACK_NAME)_data.img,$(FLAIR_TRASHWIN_PUTBACK_SPEC),DESKTOP-DB-SAVE n=2,0)
@t=$$($(call tw-trash-cluster,$(BUILD)/$(FLAIR_TW_PUTBACK_NAME)_data.img)); [ -n "$$t" ] || { printf '!!! test-flair-trash-window FAIL: mshowfat found no ::/TRASH\n'; exit 1; }; \
printf '%s\n' 'FINDER-TRASH name=README.TXT origin=0' 'DESKTOP-DB-SAVE n=3' 'FINDER-OPEN-TRASH win=1 n=1 singleton=0' \
	"FINDER-MOVE name=README.TXT from=$$t to=0" 'DESKTOP-DB-SAVE n=2' > $(FLAIR_TW_CHAIN).want; \
grep -E '^(FINDER-TRASH|FINDER-OPEN-TRASH|FINDER-MOVE|DESKTOP-DB-SAVE)' "$(BUILD)/$(FLAIR_TW_PUTBACK_NAME).serial" > $(FLAIR_TW_CHAIN).got || true; \
cmp -s $(FLAIR_TW_CHAIN).want $(FLAIR_TW_CHAIN).got || { printf '!!! test-flair-trash-window FAIL: the PUT BACK serial chain differs from the locked one (T=%s from mshowfat)\n' "$$t"; diff $(FLAIR_TW_CHAIN).want $(FLAIR_TW_CHAIN).got; exit 1; }
$(call fo-has,$(FLAIR_TW_PUTBACK_NAME),FINDER-DROP-HILITE name=INITECH,the volume icon was not lit under the item dragged out of the Trash window)
@mtype -i $(BUILD)/$(FLAIR_TW_PUTBACK_NAME)_data.img ::/README.TXT | cmp -s - $(FLAIR_DATA_README) || { printf '!!! test-flair-trash-window FAIL: ::/README.TXT is not back byte-identical to the fixture\n'; exit 1; }
@n=$$($(call tw-trash-entries,$(BUILD)/$(FLAIR_TW_PUTBACK_NAME)_data.img)); [ "$$n" = 0 ] || { printf '!!! test-flair-trash-window FAIL: ::/TRASH still holds %s entries after the put back\n' "$$n"; exit 1; }
@[ "$$(mcopy -n -i $(BUILD)/$(FLAIR_TW_PUTBACK_NAME)_data.img ::/DESKTOP.DB - | wc -c)" = 56 ] || { printf '!!! test-flair-trash-window FAIL: DESKTOP.DB is not 8 + 2*24 = 56 bytes (the origin record was not dropped)\n'; exit 1; }
@printf '>>> test-flair-trash-window [2/2]: PUT BACK -- FINDER-MOVE README.TXT from \\TRASH (cluster per mshowfat) to 0; mtools: back in ::/ byte-identical, ::/TRASH empty, DB 56 B\n'
@printf '>>> test-flair-trash-window: green\n'
```

## test-flair-trash-window-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 228):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-trash-window-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-trash-window-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-trash-window_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-trash-window_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-trash-window_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-trash-window_bochs.report.txt" || true
@cat "$(BUILD)/flair-trash-window_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-trash-window-bochs "$(BUILD)/flair-trash-window_bochs.report.txt" "$(BUILD)/flair-trash-window_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK DESKTOP-DB-CREATE TRASH-CREATE FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-trash-window-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 21659):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_win_trash_root.img build/flair_data.img build/ppm_flair_disk_windows_check build/.makefile.stamp`
```make
$(call fo-boot,$(BUILD)/flair_tenants_mut_win_trash_root.img,flair_trashwin_mut_root,$(BUILD)/flair_trashwin_mut_root_data.img,$(FLAIR_TRASHWIN_OPEN_SPEC),FINDER-OPEN-TRASH win=,1)
@grep -q '^FINDER-OPEN-TRASH win=' $(BUILD)/flair_trashwin_mut_root.serial && [ -s $(BUILD)/flair_trashwin_mut_root.ppm ] || { printf '!!! test-flair-trash-window-mutant: the mutant never reached the Trash open / no dump (not comparable)\n'; exit 1; }
@if grep -qxF 'FINDER-OPEN-TRASH win=1 n=1 singleton=0' $(BUILD)/flair_trashwin_mut_root.serial; then printf '!!! test-flair-trash-window-mutant FAIL: TRASH_ROOT still reported the Trash window -- decoration\n'; exit 1; fi
@if $(PPM_FLAIR_DISKWIN_CHECK_BIN) trashwin $(BUILD)/flair_trashwin_mut_root.ppm > $(BUILD)/flair_trashwin_mut_root.grade 2>&1; then printf '!!! test-flair-trash-window-mutant FAIL: TRASH_ROOT PASSED the trashwin grader -- decoration\n'; exit 1; fi
@grep -q 'window frame top-left corner' $(BUILD)/flair_trashwin_mut_root.grade || { printf '!!! test-flair-trash-window-mutant FAIL: the grader went RED for the wrong reason\n'; cat $(BUILD)/flair_trashwin_mut_root.grade; exit 1; }
@printf '>>> test-flair-trash-window-mutant: TRASH_ROOT correctly RED -- serial "%s"; grader: no Trash window at (40,80)\n' "$$(grep -m1 '^FINDER-OPEN-TRASH' $(BUILD)/flair_trashwin_mut_root.serial)"
```

## test-flair-window-ops

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 23188):
Prerequisites: `test-flair-zoom-toggle test-flair-grow test-flair-collapse build/.makefile.stamp`
```make
@printf '>>> test-flair-window-ops: all three R1.2 emulator legs green\n'
```

## test-flair-zoom-toggle

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23141):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_window_ops_check build/.makefile.stamp`
```make
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name "$(FLAIR_ZOOM_TOGGLE_NAME)" --out "$(BUILD)" \
	--mouse "$(FLAIR_ZOOM_TOGGLE_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-ZOOM win 0 out" --timeout-ms 20000 \
	2> "$(BUILD)/$(FLAIR_ZOOM_TOGGLE_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(FLAIR_ZOOM_TOGGLE_NAME).report"; then printf '!!! test-flair-zoom-toggle FAIL: TRIPLE FAULT\n'; exit 1; fi
@grep -q '^FLAIR-ZOOM win 0 in$$' "$(BUILD)/$(FLAIR_ZOOM_TOGGLE_NAME).serial" \
	|| { printf '!!! test-flair-zoom-toggle FAIL: zoom-in marker missing\n'; exit 1; }
@grep -q '^FLAIR-ZOOM win 0 out$$' "$(BUILD)/$(FLAIR_ZOOM_TOGGLE_NAME).serial" \
	|| { printf '!!! test-flair-zoom-toggle FAIL: zoom-out marker missing\n'; exit 1; }
@$(PPM_FLAIR_WINDOW_OPS_CHECK_BIN) zoom_toggle "$(BUILD)/$(FLAIR_ZOOM_TOGGLE_NAME).ppm"
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/$(FLAIR_ZOOM_TOGGLE_NAME).serial"
@printf '>>> test-flair-zoom-toggle: green -- in/out markers + restored user frame\n'
```

## test-ovl-absence-finder-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/overlay.mk', line 129):
Prerequisites: `test-ovl-absence build/ovl-gates/finder_ops_kept.o build/.makefile.stamp`
```make
$(KERNEL_LINK) -u finder_ops_drop_kept -o $(OVL_GATE_DIR)/finder_kept.elf $(KERNEL_FLAIRTENANTS_OBJS) $(OVL_GATE_DIR)/finder_ops_kept.o @$(TRANSIENT_IMPORTS_LD)
@if ( $(call ovl-absence-check,$(OVL_GATE_DIR)/finder_kept.elf) ) > $(OVL_GATE_DIR)/finder_kept.log 2>&1; then printf '!!! Finder resident body survived\n'; exit 1; fi
@grep -E '!!! ovl-absence FAIL: (export finder_ops_drop resident [12] times|moved symbol fo_)' $(OVL_GATE_DIR)/finder_kept.log
@printf '>>> test-ovl-absence-finder-mutant: green (resident Finder body RED)\n'
```
