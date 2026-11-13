# Removed desktop mutation hooks

Disabled wrong-code branches were removed from built desktop sources, not
ported. These existing tests still name their old knobs; convert them to
patches before claiming the corresponding tier 2 check. Delete each row when
acted on. The two new vertical-slice patches live in factory/mutants and are
killed by factory/red-desktop.sh. No new mutant lives in product source.

| Original source | Removed knob | Existing test or harness |
|---|---|---|
| `kmain_legacy.c` | `FLAIR_DAC_MUT_` | `attic/harness/bochs_legs.mk` |
| `kmain_legacy.c` | `FLAIR_DAC_MUT_6BIT` | `attic/harness/emu/vesa8.mk` |
| `kmain_legacy.c` | `FLAIR_DAC_MUT_CHANNEL` | `attic/harness/emu/vesa8.mk` |
| `kmain_legacy.c` | `FLAIR_DAC_MUT_OMIT119` | `attic/harness/emu/vesa8.mk` |
| `kmain_legacy.c` | `FLAIR_DAC_MUT_OMIT165` | `attic/harness/emu/vesa8.mk` |
| `kmain_legacy.c` | `FLAIR_DAC_MUT_ROWCOPY` | `attic/harness/emu/vesa8.mk` |
| `kmain_legacy.c` | `FLAIR_MUT_PLAIN_BSL_LAUNCH` | `attic/harness/emu/flair_launchkey.mk` |
| `kmain_legacy.c` | `KMAIN_MUT_COMMAND_VOLUME` | `attic/harness/proptest/power_overlay.mk` |
| `kmain_legacy.c` | `KMAIN_MUT_CP_NO_SYSTEM_BAR` | `attic/harness/emu/cpanels_bars.mk` |
| `kmain_legacy.c` | `KMAIN_MUT_EXEC_NO_COMSPEC` | `attic/harness/emu/execguard.mk` |
| `kmain_legacy.c` | `KMAIN_MUT_MKDIR_CUT` | `attic/harness/emu/power_cancel.mk` |
| `kmain_legacy.c` | `KMAIN_MUT_NO_INPUT_REPLAY` | `attic/harness/emu/copybreak.mk` |
| `kmain_legacy.c` | `KMAIN_MUT_OVL_UNBOUND` | `attic/harness/exec_overlay.mk` |
| `kmain_legacy.c` | `OVL_MUT_NO_FAULT_NAME` | `attic/harness/proptest/overlay.mk` |
| `desktop.c` | `DESKTOP_MUTATE_NO_PAINTALL_CLEAR` | `flair/desktop/tests/test_drag.c` |
| `desktop.c` | `DRAG_MUTATE_NO_CLIP` | `flair/desktop/tests/test_drag.c` |
| `desktop.c` | `DRAG_MUTATE_SKIP_EXPOSED` | `flair/desktop/tests/test_drag.c` |
| `desktop_db.c` | `DB_MUT_PHANTOM` | `attic/harness/finder_alerts.mk` |
| `finder_actions.c` | `KMAIN_MUT_MKDIR_CUT` | `attic/harness/emu/power_cancel.mk` |
| `finder_actions.c` | `KMAIN_MUT_NO_DELETE_NOW` | `attic/harness/finder_trash_edges.mk` |
| `finder_cmd.c` | `FINDER_CMD_MUT_EXEC_SWALLOWS` | `flair/desktop/tests/test_menu_handlers.c` |
| `finder_cmd.c` | `FINDER_CMD_MUT_NEWFOLDER_IN_TRASH` | `attic/harness/finder_trash_edges.mk` |
| `finder_cmd.c` | `FINDER_CMD_MUT_PRED_STUCK_ENABLED` | `flair/desktop/tests/test_finder_cmd.c` |
| `finder_cmd.c` | `FINDER_CMD_MUT_SILENT_UNKNOWN` | `flair/desktop/tests/test_finder_cmd.c` |
| `finder_cmd.c` | `FINDER_CMD_MUT_TABLE_BYPASS` | `flair/desktop/tests/test_finder_cmd.c` |
| `finder_desktop.c` | `FINDER_DESK_MUT_DBLTICK_OFF` | `flair/desktop/tests/test_finder_desktop.c` |
| `finder_desktop.c` | `FINDER_DESK_MUT_MARQUEE_OFFBYONE` | `flair/desktop/tests/test_finder_desktop.c` |
| `finder_desktop.c` | `FINDER_DESK_MUT_NO_HILITE` | `flair/desktop/tests/test_finder_ops.c` |
| `finder_desktop.c` | `FINDER_DESK_MUT_NO_UNDERLAY` | `flair/desktop/tests/test_finder_desktop.c` |
| `finder_icon.c` | `DESK_ICON_MUT_MASK_IGNORED` | `flair/desktop/tests/test_desk_icons.c` |
| `finder_icon.c` | `DESK_ICON_MUT_NO_SELECT` | `flair/desktop/tests/test_finder_icon_art.c` |
| `finder_icon.c` | `DESK_ICON_MUT_ROW_OFF1` | `flair/desktop/tests/test_desk_icons.c` |
| `finder_ops_cold.c` | `FINDER_MUT_ORIGINAL_LOST` | `attic/harness/finder_alerts.mk` |
| `finder_ops_cold.c` | `FINDER_MUT_PUTAWAY_REPOPULATE` | `attic/harness/finder_trash_edges.mk` |
| `finder_ops_cold.c` | `FINDER_MUT_REPLACE_CANCEL` | `attic/harness/finder_alerts.mk` |
| `finder_ops_cold.c` | `FINDER_OPS_MUT_COUNT_CELL` | `flair/desktop/tests/test_finder_ops.c` |
| `finder_ops_cold.c` | `FINDER_OPS_MUT_NO_CYCLE` | `flair/desktop/tests/test_finder_ops.c` |
| `finder_ops_cold.c` | `FINDER_OPS_MUT_NO_ORIGIN` | `flair/desktop/tests/test_finder_ops.c` |
| `finder_ops_cold.c` | `FINDER_OPS_MUT_NO_SERVICE_GUARD` | `flair/desktop/tests/test_finder_ops.c` |
| `finder_ops_cold.c` | `FINDER_OPS_MUT_REMOVE_NO_META` | `flair/desktop/tests/test_finder_ops.c` |
| `finder_ops_cold.c` | `FINDER_OPS_MUT_TRASH_NO_STAGE` | `flair/desktop/tests/test_finder_ops.c` |
| `finder_windows.c` | `FINDER_MUT_ICON64` | `attic/harness/finder_alerts.mk` |
| `finder_windows.c` | `FINDER_MUT_NO_KEYS` | `attic/harness/finder_alerts.mk` |
| `finder_windows.c` | `FINDER_MUT_OPEN_ONE` | `attic/harness/finder_alerts.mk` |
| `finder_windows.c` | `FINDER_MUT_WINDOW4` | `attic/harness/finder_alerts.mk` |
| `finder_windows.c` | `FINDER_WIN_MUT_ABS_COORDS` | `flair/desktop/tests/test_finder_ops.c` |
| `finder_windows.c` | `FINDER_WIN_MUT_ARRANGE_NOOP` | `flair/desktop/tests/test_finder_ops.c` |
| `finder_windows.c` | `FINDER_WIN_MUT_CLEANUP_UNSORTED` | `flair/desktop/tests/test_finder_windows.c` |
| `finder_windows.c` | `FINDER_WIN_MUT_EMPTY_NO_CONFIRM` | `flair/desktop/tests/test_finder_ops.c` |
| `finder_windows.c` | `FINDER_WIN_MUT_EXIT_DOS_UNWIRED` | `flair/desktop/tests/test_menu_handlers.c` |
| `finder_windows.c` | `FINDER_WIN_MUT_POWER_NO_SAVE` | `flair/desktop/tests/test_finder_ops.c` |
| `finder_windows.c` | `FINDER_WIN_MUT_PURGE_KEEPS_VIEWS` | `flair/desktop/tests/test_finder_windows.c` |
| `finder_windows.c` | `FINDER_WIN_MUT_SCROLL_PAINT_ONLY` | `flair/desktop/tests/test_finder_ops.c` |
| `finder_windows.c` | `FINDER_WIN_MUT_SERVICE_SHOWN` | `flair/desktop/tests/test_finder_windows.c` |
| `finder_windows.c` | `FINDER_WIN_MUT_SINGLETON_DUP` | `flair/desktop/tests/test_finder_windows.c` |
| `finder_windows.c` | `FINDER_WIN_MUT_TRASH_ROOT` | `flair/desktop/tests/test_finder_ops.c` |
| `finder_windows.c` | `FINDER_WIN_MUT_VIEW_UNCHECKED` | `flair/desktop/tests/test_finder_windows.c` |
| `finder_windows.c` | `FINDER_WIN_MUT_VOLLABEL_SHOWN` | `flair/desktop/tests/test_finder_windows.c` |
| `finder_windows_cold.c` | `FINDER_MUT_NOTRASH_GENERIC` | `attic/harness/finder_trash_edges.mk` |
| `finder_windows_cold.c` | `FINDER_MUT_NO_DISK_ALERT` | `attic/harness/finder_alerts.mk` |
| `finder_windows_cold.c` | `FINDER_MUT_NO_LAUNCH_ALERT` | `attic/harness/finder_alerts.mk` |
| `finder_windows_cold.c` | `FINDER_MUT_NO_MOVE_ALERT` | `attic/harness/finder_alerts.mk` |
| `finder_windows_cold.c` | `FINDER_WIN_MUT_PURGE_KEEPS_VIEWS` | `flair/desktop/tests/test_finder_windows.c` |
| `apple_menu.c` | `APPLE_MENU_MUT_LIST_ALL` | `flair/desktop/tests/test_apple_menu.c` |
| `shell.c` | `SHELL_MUTATE_MODAL_BEHIND` | `flair/desktop/tests/test_shell.c` |
| `shell.c` | `SHELL_MUTATE_NO_MODAL` | `flair/desktop/tests/test_shell.c` |
| `shell.c` | `SHELL_MUT_SHORT_SURFACE` | `attic/harness/toolbox_hardening.mk` |
| `power_cold.c` | `KMAIN_MUT_COMMAND_MAGIC` | `attic/harness/proptest/power_overlay.mk` |
| `power_cold.c` | `KMAIN_MUT_COMMAND_MESSAGE` | `attic/harness/diff/fat_diff/command_file.mk` |
| `power_cold.c` | `KMAIN_MUT_COMMAND_VOLUME` | `attic/harness/proptest/power_overlay.mk` |
| `power_cold.c` | `KMAIN_MUT_EXIT_NO_TEARDOWN` | `attic/harness/proptest/power_overlay.mk` |
| `power_cold.c` | `KMAIN_MUT_POWER_NO_ASK` | `attic/harness/proptest/power_overlay.mk` |
| `power_cold.c` | `KMAIN_MUT_POWER_NO_FLUSH` | `attic/harness/proptest/power_overlay.mk` |
| `power_cold.c` | `KMAIN_MUT_SHUTDOWN_RETURNS` | `attic/harness/proptest/power_overlay.mk` |
