# WL-0118 -- FILE COPY lands on a freshly minted Platinum progress bar; InitechPaint carries the canon bar

Beads: initech-bf2 (FILE COPY dialog), initech-tdnl.20 (bar-carrier half:
InitechPaint). Lane: lane/frameapps (sibling clone), on top of main 26156dc.
Corpus: ../system7-decomp branch mint/s8-progress-bar (worktree
system7-decomp-wt-progress), commit b168640.

## Context

The previous pass (same WL number, then "file-copy-reference-block") stopped
correctly: the Mac OS 8.1 Appearance determinate progress bar (CDEF 5) had no
rendered golden (controls.md Sec 8, INDEX-sys8 gap 9), so a Platinum bar could
not be graded (Law 1/2). It left a deliberately RED routing test and a RED
reference precondition.

## What changed

1. **Golden minted (system7-decomp b168640).** mint/s8_progress_mint.sh boots a
   scratch copy of boot81.img in an isolated Xvfb (:171) with an extfs host
   folder as a second volume; dragging a 120 MB file onto the boot disk opens
   the Finder's modeless `Copy to "Initech81"` window; 80 burst shots (~82 ms)
   caught the bar at start (1-px fill), ~45 % and full; a 200 MB repro run's
   start state is pixel-identical over the whole copy window. Captures
   goldens/captures/s8_progress_determinate{_start,,_full,_start_run2}.png
   (gitignored, md5s in re/mint-results-s8-progress.md with the full anatomy:
   etched frame, black frame, 184x10 interior, per-row lavender ramp, #000077
   cap, recessed track). INDEX-sys8 gap 9 closed for the determinate bar;
   value-0 and the indeterminate bar stay open (not observable / not minted).
2. **Control Manager** (os/flair/control.c): draw_progress_bar is the CDEF 5
   anatomy transcribed into LOCKED spec/progress_platinum.h; lavender rungs
   take the scroll thumb's ratified H/P/S/R teal roles (DEC-10 OQ-2/OQ-3) and
   the cap the menu title's dark edge -- no new colour row. Fill count
   = (width-4)*value/max incl. the black edge column. FileCopyDialog's bar is
   14 rows (the measured height).
3. **Toolbox Gate Sec 12** (spec/toolbox_gate.h, deliberate Rule-8 act):
   TBX_FILECOPY 0x0091 (caption, src, dst) asked/answered like TBX_ALERT;
   TBX_EVT_FILECOPY delivers bytes copied or the error. tbxgate.c reads
   PROGRESS_BAR_RATE from APPS\MICHAEL.CFG (8.3 alias of michael_bolton.conf;
   APPS because the root listing is locked) and copies src -> dst through INT
   21h 3Dh/3Ch/3Fh/40h/42h/3Eh on the tenant's kernel-owned JFT; kmain runs
   the modal FILE COPY box (Dialog Manager, no buttons, events absorbed),
   v = min(100, rate*ticks/100) %, the copy never ahead of the bar.
4. **InitechCalc** File > Save (Cmd-S) is enabled and asks the box with the
   locked caption; the shipped APPS\LEDGER.TXT is copied to APPS\TABLES.TXT.
5. **InitechPaint** (os/apps/paint/paint.c, PAINT.EXE): the frozen canon bar
   (menu_canon.h titles, no Apple slot), every item period-grayed except
   File > Quit, a blank white "Untitled-1" canvas. Item words are authored.
6. **Volume growth**: APPS gains PAINT.EXE (cell 11), LEDGER.TXT (12),
   MICHAEL.CFG (13). Re-keys: FLAIR_CALC_SPEC = main's uncommitted PM re-key
   (cell 10) carried verbatim, unchanged by this lane; stranded TRASH cell 11
   -> 14, FLAIR_SVC_DAMAGED_SPEC m100:100,m56:24 -> m88:100,m0:76 and park
   m100:32,m36:0 -> m100:-10,m100:-10,m4:0 (absolute (415,258) kept);
   locked_trash.sh purged=11 -> 14 (+3 leaves); fixture_volumes.mk
   FLAIR_FIXTURES +3; RECORD_MARKER_empty_trash -> purged=16 (it was stale).
   fat12_ref.py --cat accepts a backslash path (find_path).

## Why

PRD 6.3 puts the FILE COPY box in the Dialog Manager and the bar in the
Control Manager; 6.5/App B make PROGRESS_BAR_RATE the comedic constant; the
App Contract forbids tenant modal loops (Sec 6a/11), hence the asked/answered
verb. The bar is graded by an oracle that reads the PNGs, never the artifact's
table (Law 2, HER-02).

## Acceptance (all run on this lane, SYSTEM7_DECOMP absolute)

Host: test-control (170 checks; PROPERTY 5b: three captured states at 8 and
32 bpp + every fill count at the dialog width) + test-control-mutant
(CTRL_MUT_PROGRESS_FLAT RED, and a hand mutation of one cap pixel went RED
then was restored); test-dialog(+mutant), test-flair-shell(+mutant);
test-calc-tenant(+mutant) 415 checks; test-calc-file-copy-routing (11 checks,
RED 6/11 against the pre-change Calc, build/file_copy_host_red.log)
(+ CALC_MUT_CAPTION RED); test-paint-tenant 69 checks (+ BAR_TEXT / GRAYED RED).
Emu (QEMU): test-flair-file-copy (MID: bar pixel-exact vs the captures at
f=173/328, caption ink 475 px; DONE: ticks=500 == ceil(10000/20), TABLES.TXT
== LEDGER.TXT == fixture by mtools AND fat12_ref.py, fsck.fat clean, box gone)
+ mutant (RATE_IGNORED ticks=102, NO_COPY TABLES.TXT absent, CALC_MUT_CAPTION
463 vs 475 ink px, PROGRESS_FLAT bar RED); test-flair-paint (band 2 pixel-exact
canon bar, blank canvas, grayed Edit panel, grayed release delivers nothing,
Quit exits) + mutant (BAR_TEXT 721 px, ITEMS_ENABLED 476 black + an extra
delivery); make image; test-kernel-headroom (flagship 16,936 B free, binding
15,816 B; was 20,824/19,704); test-flair-calc(+mutant); test-flair-locked-purge;
test-flair-finder-service; test-flair-empty-trash; test-flair-app-launch;
test-flair-desktop(+mutant); test-flair-disk-windows; test-flair-scroll;
test-flair-scroll-look; test-flair-data-volume; test-fixture-volumes;
test-makefile-vars. Not run: make test, Bochs, record-flair (PM records the
file_copy and paint_bar clips on main; blocks are defined before the target).

## Frictions

- A FLAIR-MENU-DROP capture waits 750 ms after every press, which breaks the
  launch double-clicks; the Paint menu leg captures at FLAIR-MENU-XDROP (File
  pressed, dragged onto Edit), which waits only on held moves.
- The emulator runs 8bpp: the teal ramp collapses S/R to #3F3F3F and the cap
  to black (the scroll thumb's ratified indexed fallback).
- The other two lanes minting in system7-decomp share goldens/captures; this
  mint used its own scratch disk copy and display.

## Open / follow-ups (not filed: lane does not touch beads)

- ADR-0004-AMENDMENT-DEC-10 Sec 3.3.1 says the navy accent "drives ... the
  progress/selection accents", while its ratified OQ-2 maps every clut-208
  lavender accent to teal; the measured bar IS clut-208, so teal was used.
  Operator ruling wanted; the switch is one table (control.c CTRL_PROG_*).
- The frame's hourglass cursor is not shown during the copy (busy cursor
  during FILE COPY is a natural follow-up).
- Value-0 bar rendering and the indeterminate bar remain golden-resolves.

## Pointers

spec/progress_platinum.h; harness/render/progress_golden.h;
tools/ppm_flair_file_copy_check.c; tools/ppm_flair_paint_check.c;
harness/frameapps.mk; spec/toolbox_gate.h Sec 12; os/milton/kmain.c
flair_live_tenant_filecopy; os/flair/tbxgate.c (FILE COPY engine);
../system7-decomp/re/mint-results-s8-progress.md.
