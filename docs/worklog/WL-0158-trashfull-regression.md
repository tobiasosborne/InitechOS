# WL-0158 -- FULL Trash regression hunt

Lane: `lane/trashfull`, based on main `aa169fc4`. Changes left for PM review;
no commits, git mutations, beads edits, full test vector or Bochs run.

## Root causes

Both reported failures reproduce on the unmodified clone. Neither is a
strike decoder, palette, blitter, optimization or Hidden-directory failure.

1. `lane/icons`, commit `9b5d999d` (merged as `014de0ac`), changed the Trash
   oracle to the independent mint-008 captures but always chose UNSELECTED.
   `147f4c2f` corrected the emptied leg's selection and missed locked-full.
   The locked-purge trace double-clicks Trash before choosing Empty Trash.
   It leaves the desktop Trash selected, including after the locked notice.
   The booted can differs from the unselected FULL capture on 617/1024 cells
   and equals the selected FULL capture on every cell (0/1024). The reported
   #3F3F3F -> #272727 is exactly the measured selection function, not a wrong
   tone lookup. The host renderer also matches all eight strikes/states at
   8bpp and 32bpp. The factory now chooses SELECTED for locked-full and keeps
   UNSELECTED for the ordinary alert.
2. `lane/dosfolder`, commit `157b7d6c` (merged as `d85c63fb`), moved DOS
   commands out of APPS. The icon lane's base still had LABEL.COM at APPS
   cell 7, the DOC probe at (478,258). Main has PAINT.EXE there: the booted
   cell equals APP on all 1024 cells and differs from DOC on 626 cells.
   The probe now names TPSPARM.DAT, cell 10 in the independent locked
   `spec/dos_folder_apps.txt`, at (410,310). All seven probes remain, with
   zero tolerance. 123.MNU at cell 4 is covered by the raised root window
   in this scene and is not a suitable replacement probe.

## Evidence and changes

The stopped mixed-purge image has Hidden TRASH, retains the protected
TENANTFX.EXE bytes and attributes, and passes fsck.fat. Serial records prove
README.TXT and APPS were staged, Trash was selected/opened, the alert counted
19 nodes, and the purge removed 17 and refused 2. No runtime source or locked
asset was changed; no shipped data-volume growth.

`test-finder-icon-oracles` uses independent captured cells as inputs to test
the factory grader's state routing and the locked APPS inventory to check its
named scene probes. Before the fix it reports exactly two failures: selected
FULL routing (617 cells) and the stale DOC coordinate. This is a test of the
oracle's routing, not a substitute for grading rendered pixels. The reused
emulator gates still grade the real booted pixels against the captures.

New targets, wired into the neighbouring host/emulator vectors:

- `test-finder-icon-oracles`, `test-finder-icon-oracles-mutant`
- `test-flair-icon-oracles`, `test-flair-icon-oracles-mutant`

The emulator wrapper reuses test-flair-locked-purge and
test-flair-finder-icons, including their injected traces, serial/mtools/fsck
checks and screenshot grading. Mutants restore each actual factory defect:
`TRASH_CHECK_MUT_LOCKED_UNSELECTED` rejects the selected FULL can (617/1024),
and `ICON_SCENE_MUT_STALE_DOC` grades PAINT.EXE as DOC (626/1024); the other
six booted cells stay green. Both are proven RED on host and emulator inputs.

## Acceptance sources and artifacts

Ground truth: PRD Sec 3, 6.2-6.5, Appendix A/B; spec/chrome_metrics.json;
spec/assets; system7-decomp INDEX.md/GAPS.md and re/mint-results-008.md.
Pixel goldens: `../system7-decomp/goldens/captures/s8_icons_*.png`, especially
`s8_icons_trash_full_sel.png` and `s8_icons_trash_full.png`, plus
`s8_alert_modal.png`. No expected pixel is generated from a shipped strike.

RED-first logs and per-target acceptance logs are in `build/trashfull/`.
Every QEMU run uses INITECH_QMP_STDIO=1 and the supplied corpus environment.
RAM/disk were checked before heavy runs. The requested kernel-headroom gate
measures all kernel variants; image construction uses the normal image target.

Rule-14 block `trashfull_regression` is before record-flair. Its trace name
FLAIR_TRASHFULL_REGRESSION_SPEC aliases the existing locked mixed-purge trace,
so no duplicate gesture stream is maintained. It uses the existing read-only
TENANTFX fixture and long double-click record image. The PM records it on main
after merge, as requested. No clip was recorded in this lane.

Beads were read-only: bd prime ran; bd show could not resolve the historical
IDs in this clone. The checked-in issue JSONL and WL-0140/WL-0152 supplied the
historical context. No beads state was initialized or changed.

Final acceptance: all four new targets GREEN, including both named defects
restored and caught on host and booted screenshots. Requested gates GREEN:
test-flair-locked-purge, test-flair-finder-icons, test-flair-trash-full,
test-flair-empty-trash, test-flair-finder-service, test-finder-icon-art,
test-desk-icons, test-flair-desktop-icons, test-kernel-headroom, and image.
The existing test-flair-finder-icons-mutant and test-flair-trash-full-mutant
also pass: ROW_OFF1 all 7 RED, NO_SELECT exactly 2 RED, TRASH_NEVER_FULL
421/1024 RED, DIALOG_NO_ICON 572/1024 RED with FULL Trash still 0/1024.
test-makefile-vars passes and the record-block expansion resolves correctly.
Headroom: 235 kernels measured; flagship 177172 B loaded / 19436 B free;
largest kernel_flairtenants_db_write_skip.elf 178420 B / 18188 B free.
The image target builds build/initech.img and build/flair_data.img.
