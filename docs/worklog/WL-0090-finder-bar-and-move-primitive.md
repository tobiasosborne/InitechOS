<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0090 -- THE FINDER GETS ITS MENUS, THE ENTRY MOVES, AND THE WALL IS MEASURED

**Date:** 2026-08-25 (session close; same orchestrated sitting as WL-0088/0089)
**Beads:** initech-tdnl.12 STAGE 1 landed (bead stays in_progress -- stage 2 is
OPERATOR-BLOCKED, see below) + initech-tpzf CLOSED (the raise gap) +
initech-tdnl.26 CLOSED (fat12_move_dirent). Filed: initech-f0hh (same-dir
folder rename, now trivial), initech-8z9j (kernel size policy -- THE next
work item), initech-34dh/6k12/p6st (the tdnl.11 reslice chain).
**Commits:** `2dbbf3a` (tdnl.12 stage 1 + tpzf) -> `22903a9` (tdnl.26). ALL
PUSHED; tree clean at `22903a9` at close.
**Closing certificate:** ALL GREEN **353 host + 105 emu** (first-person,
clean build, after 22903a9). Every landing in this sitting was bracketed by
its own first-person clean certificate (351+105 after 2dbbf3a).

## What changed

1. **tdnl.12 stage 1 (`2dbbf3a`): the real Finder menu bar exists.**
   os/flair/finder_menu.{h,c}: the full F4.2 resource (Apple standalone data
   id 0 -- MenuBar_hit's glyph-slot constraint documented; Calculator/Note
   Pad OMITTED until tdnl.14 per F4-3 omit-if-not-installed; About grayed
   until R4.1; mark char is a documented ASCII substitution -- Chicago has
   no check glyph); finder_menu_refresh_enables truth-tabled; REAL MenuKey
   replaces the interim finder_cmd_key_lookup (removed). Proven on metal:
   open a disk window -> the Finder tenant is promoted (FLAIR-DISPATCH
   app=FINDER) -> band 2 shows Apple/File/Edit/View/Special/Help (pixel
   differential); mouse menu dispatch, the Cmd-I positive/negative MenuKey
   pair, Clean Up's mouse leg UNLOCKED, byte-identical cancel restore.
   704-check host oracle + ENABLE_STUCK/CMDCHAR_DUP/DEAD_ITEM mutants.
   **tpzf fixed at root in the same landing:** SelectWindow + group-head
   repoint in flair_app_dispatch after switch_foreground (routing layer,
   BC-2 honest, IM-cited) -- closes BOTH the same-tenant no-raise gap and
   the cross-tenant clicked-window-not-frontmost gap. Rule-6 catch: the
   raise STRUCTURALLY HEALED the FLAIR_LIVE_MUTATE_NO_RAISE_ON_TITLE
   mutant (leg H went green under it); the knob's scope was extended and
   leg H bites again -- only the FULL emu vector caught it.

2. **tdnl.26 (`22903a9`): the MILTON move primitive.** fat12_move_dirent:
   same-volume cross-directory 32-byte transplant, dst_name83 compose (one
   atomic-shaped move+rename), DIRECTORY moves WITH the '..' fixup
   (fat12_set_dotdot; reverse-unwind rollback incl. fat12_shrink_dir_tail),
   cycle guard (FAT12_ERR_CYCLE -18), SAME_DIR (-17), error-number ledger
   (the -15 collision was already fixed at -16 in tdnl.8 -- verified).
   fat12_trash_suffix: the F1.4 RULE (trunc5+%03u; the REP001 example
   contradicts its own rule -- documented, rule wins). 149-check host
   oracle + mtools/python differential where the '..' assertion uses an
   INDEPENDENT byte reader (fat12_ref.py --dotdot) because mdir resolves
   '..' TEXTUALLY -- the grep would have been decoration (proven against
   the mutant image). SIX mutants (not the briefed two): MOVE_FREES_CHAIN
   added because without it the FAT-bytes-unchanged assertions were
   unproven decoration; NO_DOTDOT_FIX / NO_GROW_ROLLBACK /
   SUFFIX_NO_COLLIDE_CHECK cover the added rules. INT-21h fence holds
   (cross-dir rename still 0x0011; ycb3's call).

3. **tdnl.11 STOPPED PRE-CODE at the headroom gate -- the correct stop.**
   The lane measured instead of estimating: the REAL ceiling is the
   LARGEST kernel, kernel_flairtenants_mut_no_raise_on_title at 194,884B =
   **1,724B headroom** (the placement guard reports only the flagship --
   understates the binding constraint). The R3.4 slice floor is 12-20KiB.
   KERNEL_SECTORS=384 is AT the ratified bounce ceiling (K2) and cannot
   move. **THE FIND: selective -Os is a measured ~40.9KiB reclaim with no
   boot-geometry change** (int21 -14.7K, fat12 -8.3K -- NB fat12 GROWS at
   -O1, per-object levels only; control/chrome/dialog/menu/region -2.5..
   -4.8K each; all compile clean under -Werror; kmain must stay -O1; the
   march=i386 pin stands). Filed as initech-8z9j with the full table;
   tdnl.11 resliced into 34dh (binding+drag-move+Trash staging) -> 6k12
   (Empty Trash+FULL-trash strike) -> p6st (Duplicate+Get Info+rename),
   each with lane-recorded design rulings on the tdnl.11 bead (no
   TextEdit for rename; movableDBoxProc Get Info; folder Duplicate
   refused; purge-protection no-op until tdnl.14's g_disk_tenant_slot).
   The 8z9j lane was dispatched and then INTERRUPTED by the operator
   wind-up BEFORE any edit (tree verified clean); it is the next agent's
   FIRST work item.

## THE OPERATOR ESCALATION (stage 2 of tdnl.12 is blocked on this)

Measured by the stage-1 lane: with the Finder bar in band 2, **band 1 and
band 2 differ by ZERO pixels in the exact region the appswitch
DISTINCT-CHIMERA leg grades** (whole-bar difference is only the Help
title, 105 px). Flipping the boot foreground to the Finder (design F2.1
step 3, the band-2-at-rest re-key) would therefore collapse the visible
two-stacked-bars chimera the frame is built on and drive DISTINCT-CHIMERA
RED. This is a Law-4 ruling, not a quiet re-key. Options recorded on
initech-tdnl.12: (a) keep HELLO boot-foreground (frame-correct,
Finder-at-rest period-INcorrect); (b) flip + retire/re-key
DISTINCT-CHIMERA (the chimera collapses at rest); (c) visually
differentiate the Finder bar (ties into the pending D3-3 Charcoal-vs-
Chicago ruling). The full evidence is in the tdnl.12 bead notes.

## Frictions

- A finished lane left stale poll loops that kept firing completion
  notifications; a single kill instruction via SendMessage drained them.
  Lesson: lane briefs should forbid self-polling background shells at exit.
- pkill from the orchestrator shell exits 144 (kills its own process
  group's pipeline) -- verify with pgrep in a SEPARATE call.

## Acceptance

- ALL GREEN 353 host + 105 emu, clean build, first-person, at the pushed
  HEAD `22903a9`. Working tree clean; no stashes; dolt pushed.
- Every mutant added this sitting proven RED-for-named-reason; the one
  structurally-healed mutant re-armed.

## Pointers -- NEXT AGENT START HERE

1. **initech-8z9j** (kernel size policy) -- everything needed is in the
   bead description; it unblocks 34dh -> 6k12 -> p6st (the R3.4 chain).
2. The operator rulings queue: the stage-2 chimera question (above), the
   desktop PATTERN canon, rainbow-vs-mono Apple + Charcoal-vs-Chicago
   (D3-3), 81ft.4 alert pinks, ADR ratification of the four design drafts,
   and the standing Law-4 eyeball of build/clips/ (9 clips, all
   repro-proven: icon_select, rubber_band, icon_dragdrop, folder_nav,
   window_drag_persist, new_folder + the R1/R2 set).
3. tdnl.13 (menu-bar clock) is small and independent -- good filler if the
   size lane blocks.
