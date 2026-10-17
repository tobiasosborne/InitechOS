# WL-0144 -- DOS folder deployment

## Context

PM decision for initech-zc68 and initech-2kgh: move DOS externals off the
overflowing APPS grid, ship MORE/FIND/SORT, and set PATH from the flagship
volume's root AUTOEXEC.BAT. This lane leaves its changes uncommitted for PM
review. Kernel and program sources are unchanged.

## What changed

`Makefile` has one deployment list, `FLAIR_DOS_EXTERNALS`, for CHKDSK,
ATTRIB, LABEL, XCOPY, TREE, EDLIN, MORE, FIND and SORT. They ship in DOS.
The root batch file contains exactly `@ECHO OFF` and `PATH=A:\DOS`, with
CRLF line endings. Root directory order is README.TXT, APPS, DOS,
AUTOEXEC.BAT; the Finder still hides its own DESKTOP.DB and TRASH.

APPS directory order is locked independently in `spec/dos_folder_apps.txt`:

| Entry | File | Click point |
|---|---|---|
| 0 | TENANTFX.EXE | 75,122 |
| 1 | CTENANT.EXE | 143,122 |
| 2 | 123.EXE | 211,122 |
| 3 | WORD.EXE | 279,122 |
| 4 | 123.MNU | 75,174 |
| 5 | CALC.EXE | 143,174 |
| 6 | TEXT.EXE | 211,174 |
| 7 | PAINT.EXE | 279,174 |
| 8 | CALCULAT.EXE | 75,226 |
| 9 | TPSREP.COM | 143,226 |
| 10 | TPSPARM.DAT | 211,226 |
| 11 | TPS.DBF | 279,226 |
| 12 | LEDGER.TXT | 75,278 |
| 13 | MICHAEL.CFG | 143,278 |

The trace arithmetic is `75+68*(n%4),122+52*(n/4)` (WL-0131). Park points
retain their absolute destinations. The damaged fixture retains stranded
TRASH at APPS entry 0. Locked purge removes README and thirteen writable
APPS leaves (14), refusing TENANTFX and APPS (2); DOS and AUTOEXEC survive.
Arrange moves three icons: APPS 1->0, AUTOEXEC 3->1, dragged README ->3;
DOS remains at cell 2. Select All selects four root icons.

Root-listing serial contracts, pixel rosters, service-mutant cells, Standard
File's root count, launch traces and fixture-volume contracts are re-keyed.
The scroll fixture uses eighteen filler files rather than twenty, keeping
its independent 22-entry/six-row/max138 contract; F06 moves to column 2.
The small-window scrollbar fixture also follows the four-cell root:
document width272, view101, horizontal max171, value16 thumb x46
(41 + floor(16*56/171)); minimum-width max203. Vertical extent stays55.

Exit/TPS traces and the two books' practice replay keys omit startup answers
when AUTOEXEC exists. The separate practice diskette retains APPS. Only its
explicit noautoexec examples answer DATE/TIME. MORE/FIND/SORT have Reference
entries, and the obsolete not-shipped DRAFT notice is removed. The Reference
command contents can continue onto another page, like IBM's original.

## Ground truth

- PRD 6.1 and 6.5; ADR-0003 DEC-11/12, Appendix A (3Dh, 3Fh, 40h,
  4Bh, 4Eh/4Fh), B.1 and Appendix D. The enterprise baseline in
  `spec/dos_autoexec_bat_baseline.txt` remains unchanged; the one-volume
  flagship uses the PM-directed A: adaptation.
- Local `dos33/MS-DOS_3.3_Users_Guide_198707.pdf`, viewed rendered scans:
  printed p.72/PDF157 (FIND), p.94/PDF179 (MORE), p.97/PDF182 (PATH),
  p.116/PDF201 (SORT), p.135/PDF220 (AUTOEXEC replaces DATE/TIME).
- Local `dos33/6138519_DOS_3.10_Reference_Feb85.pdf`, printed 7-1/7-2,
  PDF106/107: command contents continue across pages. The generated
  Reference opener and new filter entries were rendered and inspected.
- `os/milton/kmain.c:flair_tenant_mount_volume` binds the primary slave
  with `fileio_fat_bind`; `flair_exit_to_dos` preserves that binding and
  resets CWD to A:'s root. `command.c:command_repl` probes A:\AUTOEXEC.BAT
  and asks DATE/TIME only when it is absent. No kernel edit is needed.
- `harness/diff/fat_diff/dos_folder.sh` compares the actual artifact FAT
  reader with mtools on identical images and requires an independent file
  roster and exact AUTOEXEC bytes; dosfstools checks allocation integrity.

## Oracle and mutation evidence

Before implementation the host gate failed `RED dos-folder external-set`
and QEMU failed `RED dos-folder startup-prompts`. New gates and their
mutants are in TEST_UNIT_GATES / TEST_EMU_GATES:
`test-dos-folder-host`, `test-dos-folder-host-mutant`, `test-dos-folder`,
`test-dos-folder-mutant`. The emulator checks serial DIR, bare CHKDSK and
MORE through PATH, exact MORE output, no startup prompts, and all nine DOS
folder icons against independent pixel probes.

Named mutants: EXTERNAL_IN_APPS and EXTRA_DOS_FILE -> external-set /
serial-external-set (the latter adds an unexpected TXT leaf to DOS);
PATH_UNSET -> autoexec / `PATH: Bad command or file name`;
NO_AUTOEXEC -> autoexec / startup-prompts;
DOS_NOT_DIRECTORY -> finder-folder. Logs and full gate tails are retained
under `build/lane-dosfolder/` for this working session.

The zoomed root's 77-pixel AUTOEXEC.BAT label contributes two black pixels
to the next empty cell's margin (Geneva9 T, row1, at 284/285,102). The
grader requires these exact pixels while keeping every other pixel white.
LABEL_TAIL renames the file to AUTOEXEC.BAI and must fail that same check.

## Fixture details

CHKDSK's utility fixture independently counts the extra DOS directory and
reads DOS/CHKDSK.COM. All four external utility emulator families now use
bare names through PATH. EDLIN's original raw Ctrl-C input is paced after
its redirected prompt write; BREAK=ON and all expected bytes remain intact.
This avoids injecting its insert-mode character into the separate DOS
disk-call break checkpoint (Reference p.38/PDF123; int21.c dispatch).

`make pubs-screens` changed exactly three committed captures: Reference
chkdsk/chkfile and User's Guide chkdsk. Each changes only total/free memory
118448 -> 118320 bytes for the current utility binary. The remaining 63
captures are unchanged. The diff is retained in
`build/lane-dosfolder/pubs-screens.diff`.

## PM recording handoff

Record the new `dos_folder` script. Re-record `tps_journey`, `calc_ledger`,
`file_copy`, `paint_bar`, `itext_edit`, `itext_savefile`, `itext_scroll`,
`empty_trash`, `locked_purge`, `select_all`, `arrange`, and `scroll_arrow`,
`scroll_page`, `scroll_thumb`. The APPS launch clips and Finder clips that
show the root/APPS background also need refreshed backgrounds: `folder_nav`,
`window_drag_persist`, `new_folder`, `app_launch`, `app_menubar`,
`tenant_menu`, `ctenant_keys`, `i123_typing`, `i123_files`, `i123_menu`,
`i123_formula`, `i123_quit_guard`, `iword_typing`, `iword_codes`,
`iword_block`, `iword_format`, `fonts_styles`, `drag_move`, `trash_drag`,
`drag_refused`, `finder_service`, `finder_service_damaged`, `trash_window`,
`finder_follow`, `modifier_release`, `held_menu`, `fg_close`,
`menu_long`, `menu_look`, `scroll_look`, `locked_trash`, `shutdown`.

PM records after merge, per lane instructions. No Bochs or full `make test`
run belongs to this lane. Beads, CLAUDE.md, HANDOFF.md and git metadata are
not changed. Existing m0dc limitations (FIND filename operands, SORT /+n,
interactive MORE coverage) remain outside this deployment change.

## Acceptance

All 75 named targets below completed successfully (normal app gates also
run as prerequisites of their mutant gates). Every QEMU invocation used
INITECH_QMP_STDIO=1. The complete `make test` vector and Bochs were not run,
as instructed. Two fresh data images match each other and the final
flagship data image byte for byte. Added source lines are ASCII;
`git diff --check` is clean. No commit or push was made.

Exact acceptance lines, with separator-only tails omitted:

```text
test-dos-folder-host | GREEN test-dos-folder-host: exact DOS/APPS order, AUTOEXEC bytes, artifact FAT reader == mtools, fsck.fat
test-dos-folder-host-mutant | GREEN test-dos-folder-host-mutant NO_AUTOEXEC rejected
test-dos-folder | ppm_flair_disk_windows_check: leg dosfolder PASS (9 icon strikes graded off the finder_icons.h ASCII maps, 9 label-band relations, 0 empty cells, 11 chrome assertions, 2 bare-teal controls)
test-dos-folder-mutant | GREEN test-dos-folder-mutant LABEL_TAIL rejected
test-makefile-vars | >>> test-makefile-vars: green
test-fixture-volumes | >>> test-fixture-volumes: green (277 scratch names vs 125 fixture names; 68 inline image writers)
test-fixture-volumes-mutant | >>> test-fixture-volumes-mutant: green (QUIT_DATA_COLLISION, HANDLIST_DRIFT, UNDECLARED_VARIANT, UNSEEDED_MASK, BROKEN_CONTRACT correctly RED; failed image deleted and rebuilt; real Makefile restored)
test-boot-system | GREEN test-boot-system: mdir order + mattrib HSR + mcopy/cmp both files + fsck.fat clean
test-boot-system-repro | GREEN test-boot-system-repro: two fresh FAT builds == each other == shipped boot image, byte for byte
image | >>> image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS + DOS + AUTOEXEC.BAT)
test-flair-app-launch | >>> test-flair-app-launch: green
test-flair-app-launch-mutant | >>> test-flair-app-launch-mutant: green (all six mutants RED for their named reasons)
test-flair-locked-trash | VERDICT: PASS -- test-flair-locked-stage (locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat)
test-flair-locked-purge | VERDICT: PASS -- test-flair-locked-purge (locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat)
test-flair-locked-trash-mutant | VERDICT: PASS -- test-flair-locked-trash-mutant locked_purge RED (visible message missing)
test-flair-finder-service | >>> test-flair-finder-service: green
test-flair-finder-service-mutant | >>> test-flair-finder-service-mutant [1/2]: SERVICE_SHOWN correctly RED -- FINDER-OPEN-VOLUME win=0 n=6; grader: cells 4-5 not bare
test-flair-empty-trash | >>> test-flair-empty-trash: green
test-flair-empty-trash-mutant | >>> test-flair-empty-trash-mutant: EMPTY_NO_CONFIRM correctly RED (ET-CANCEL: README.TXT is gone from ::/TRASH after Cancel -- purged without confirmation)
test-flair-desktop | VERDICT   : PASS -- the LIVE FLAIR chimera desktop is the Office Space frame
test-flair-appswitch | VERDICT   : PASS -- clicking the background tenant RAISES + ACTIVATES it and swaps
test-flair-exit-to-dos | >>> test-flair-exit-to-dos: green (QEMU; Bochs leg owed)
test-flair-exit-to-dos-mutant | >>> test-flair-exit-to-dos-mutant: green (2/2 RED for the named reason)
test-flair-tps-journey | >>> test-flair-tps-journey: green (QEMU; Bochs leg owed)
test-flair-tps-journey-mutant | >>> test-flair-tps-journey-mutant: green (3/3 RED for the named reason)
test-flair-calc-mutant | >>> test-flair-calc-mutant: green (five named mutants RED)
test-flair-calcda-mutant | >>> test-flair-calcda-mutant: green (two mutants RED for the named reason)
test-flair-apple-menu-mutant | >>> test-flair-apple-menu-mutant: MENU_MUT_APPLE_DEAD correctly RED (FLAIR-MENU menu=0 item=0: the F13 dead glyph)
test-flair-itext-mutant | >>> test-flair-itext-mutant: green (all three mutants RED for the named reason)
test-flair-itext-scroll-mutant | >>> test-flair-itext-scroll-mutant: ITEXT_MUT_SCROLL_NEVER_ENABLED correctly RED (range0; independent thumb-v crop)
test-flair-paint-mutant | >>> test-flair-paint-mutant: green (both named mutants RED)
test-flair-file-copy-mutant | >>> test-flair-file-copy-mutant: green (four named mutants RED)
test-flair-disk-windows | VERDICT   : PASS -- the booted Finder opens the REAL volume in a REAL window,
test-flair-disk-windows-mutant | VERDICT   : PASS -- all three mutants drive their legs RED (the gate bites; Rule 6)
test-flair-finder-cmds | >>> test-flair-finder-cmds: green
test-flair-finder-cmds-mutant | >>> test-flair-finder-cmds-mutant: STUB_ENABLED correctly RED (FC: File > Get Info / Duplicate / Open did not all release on NOTHING (want 3 x menu=512 item=0): FLAIR-MENU menu=512 item=6 (sel=0x02000006) FLAIR-MENU menu=512 item=0 (sel=0x00000000) FLAIR-MENU menu=512 item=0 (sel=0x00000000) )
test-flair-scroll | >>> test-flair-scroll: green
test-flair-scroll-mutant | >>> test-flair-scroll-mutant: green (both mutants RED for the named reason)
test-flair-file-ops | >>> test-flair-file-ops: green
test-flair-trash-window | >>> test-flair-trash-window: green
test-flair-desktop-icons | VERDICT   : PASS -- the booted Finder draws, selects, marquees, opens, drags and
test-flair-finder-follow | >>> test-flair-finder-follow: green
test-flair-scroll-look-mutant | >>> test-flair-scroll-look-mutant: WINDOW_MUTATE_GROW_NO_MIN correctly RED (undrawn bar and wrong frame)
test-flair-menu-look-mutant | >>> test-flair-menu-look-mutant: MENU_LOOK_MUT_OLD_DIVIDER_INSET correctly RED (ML10 whole divider rows)
test-chkdsk | VERDICT: PASS CHKDSK host
test-chkdsk-emu | VERDICT: PASS CHKDSK emu
test-chkdsk-emu-mutant | GREEN CHKDSK emu mutant CACHE rejected
test-xcopy-tree | XCOPY/TREE GREEN: mcopy FAT differential; /S /E /A /M /D /P /V /W F/D; TREE mdir/manual rendering; fsck.fat
test-xcopy-tree-emu | XCOPY/TREE EMU GREEN: external flat EXEC; injected /W /P keys; FAT differential; TREE byte-exact; SHELL-EXIT
test-xcopy-tree-emu-cancel | XCOPY/TREE CANCEL GREEN: injected Ctrl-C; exit=2; destination unchanged; SHELL-EXIT
test-xcopy-tree-emu-mutant | XCOPY/TREE EMU MUTANTS GREEN: S TREE_LAYOUT CTRL_C booted and caught
test-attrib | PASS: attrib host differential (mattrib/mlabel/mdir/fsck.fat; identical source images)
test-label-vol | PASS: label host differential (mattrib/mlabel/mdir/fsck.fat; identical source images)
test-dosutil-emu | PASS: ATTRIB/LABEL/VOL QEMU differential, dialogue, disk and clean return
test-dosutil-emu-mutant | PASS: cache QEMU mutant caught RED
test-edlin-host | GREEN test-edlin-host (all command scripts; independent manual model)
test-edlin-emu | GREEN test-edlin-emu (all command scripts; independent manual model)
test-edlin-emu-mutant | GREEN test-edlin-emu-mutant QUIT rejected
test-more-filter | VERDICT   : PASS -- MORE detects a file stdout and passes input through on the 386 (QEMU only)
test-more-filter-mutant | >>> test-more-filter-mutant: green (lossy-passthrough mutant correctly RED -- round-trip != input, the full byte forward is load-bearing)
test-find-filter | VERDICT   : PASS -- FIND really filters piped stdin by a case-sensitive literal on the 386 (QEMU only)
test-find-filter-mutant | >>> test-find-filter-mutant: green (inverted-match mutant correctly RED -- matched set != golden, the match sense is load-bearing)
test-sort-filter | VERDICT   : PASS -- SORT really sorts stdin (case-insensitive + /R) on the 386 (QEMU only)
test-sort-filter-mutant | >>> test-sort-filter-mutant: green (reversed-comparator mutant correctly RED -- ascending block != golden, the collation is load-bearing)
test-pubs | >>> test-pubs: green (command coverage, example replay, source lints, build cap)
test-flair-calc | >>> test-flair-calc: green (disk launch, two windows, PRD text, historical chrome pixels, edit, quit)
test-flair-calcda | >>> test-flair-calcda: green
test-flair-apple-menu | >>> test-flair-apple-menu: green
test-flair-itext | >>> test-flair-itext: green
test-flair-itext-scroll | >>> test-flair-itext-scroll: green (40 Returns; range/value70; whole enabled bar; caret in view)
test-flair-paint | >>> test-flair-paint: green
test-flair-file-copy | >>> test-flair-file-copy: green
test-flair-scroll-look | >>> test-flair-scroll-look: green (shrink enabled; click scrolls16; restore disabled; minimum96x88 draws all parts; no SCROLL-IGNORED)
test-flair-menu-look | >>> test-flair-menu-look: green (File dark band/white ink/item20/4/shortcut14/whole divider rows; View NFNT check18)
test-pubs-screens | >>> test-pubs-screens: green (66 screens in the manuals == live OS output)
```
