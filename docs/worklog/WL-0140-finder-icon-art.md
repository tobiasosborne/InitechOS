# WL-0140 -- Finder icon art: Platinum strikes from a fresh Mac OS 8.1 mint

Beads: initech-tdnl.53 (Finder icon art), initech-tdnl.74 (the FULL Trash),
initech-tdnl.87 (the alert caution icon). Lane `lane/icons`.

## Context

The v1 strikes (spec/assets/desk_icons.h, finder_icons.h) were hand-drawn
outline shapes in three tones (black, white, #C0C0C0): the audit (F16)
judged them sparse next to the Mac OS 8.1 Finder; the Empty Trash confirm
drew no caution icon because iconItem was a no-op and no strike existed
(L022); and audit H04 (an image from before initech-6k12) saw the Trash
unchanged after staging.

## What changed

**Mint (system7-decomp, mint-008, commits 391740e + fca1a8e).** A Basilisk II
Mac OS 8.1 session on a scratch copy of boot81.img with a 1.44 MB HFS floppy
holding a generic folder, document (creator `zzzz`) and application (APPL,
no resources) -- `mint/make_icon_floppy.py`, `mint/s8_icons_session.sh`. 24
captures `goldens/captures/s8_icons_*.png`: the floppy, the hard disk, the
Trash empty and full, the folder (closed + open), document and application,
each unselected and selected, over a window's white AND the 128x128 desktop
picture, plus the Empty Wastebasket caution alert. Ledger
`re/mint-results-008.md`: 32x32 cell origins, opacity recovered as "differs
from the background in any capture of any state" (the two backgrounds agree
on every opaque cell), the 30 colours, and the finding that SELECTION is one
colour function for every icon (white -> #969696, #DADAFF -> #545487, black
unchanged, ...). Traps: the `floppy` pref does not mount a raw HFS image in
this Basilisk build (the same image on a `disk` slot is drawn with the
Finder's floppy icon); the hard-disk icon is the Basilisk driver's, not
Apple's. Addendum (coordinator request, initech-tdnl.44): the Finder list
view, name sort and Size sort, `s8_finder_list_view*.png`, Sec 6.

**Strikes (Rule 8, deliberate).** FLAIRDeskIcon v2: 4 bits a pixel plus a
16-entry tone table naming DESK_IC_* colour ordinals; DESK_IC_SELECTED is the
measured selection function. VOLUME (floppy), HARDDISK, TRASH, TRASH_FULL
(desk_icons.h), FOLDER, DOC, APP (finder_icons.h), CAUTION (new
alert_icons.h). HONESTY: the strikes are TRANSCRIBED from the capture cells
-- each ASCII map was printed from the crop by a one-shot scratch script and
is reproduced verbatim; they are not freehand and not from any resource or
ROM. The brief said "hand-authored"; that word would be false here, so the
headers say transcribed. The HARDDISK strike is the Basilisk driver icon;
no InitechOS volume is a hard disk, so nothing paints it yet.

**Colour (Rule 8, deliberate).** color_canon.json: slots 11..25 now carry
the fifteen non-gray icon colours (they were unnamed #0B..#19 ramp grays
that no row or PART drew -- the L023 slot 9/10 precedent), plus three named
ramp grays (#272727, #545454, #666666). The generator is count-driven
(CANON_N 26, RAMP_N 16). flair_look gains FLAIR_PART_ICON_CLUT (one PART
per colour ordinal), so finder_icon.c still names no colour (C-8).

**Renderer.** finder_icon.c decodes v2; finder_icon_draw_hilite applies the
selection function and now serves BOTH the selected and the drop-target
state -- a selected icon darkens, as captured (it used to change only the
label). dialog.c draws an iconItem (its IM ICON ID in `_pad[0]`,
DIALOG_ITEM_ICON_ID; only Caution has a strike) through the same decode
without linking the Finder; kmain's Empty Trash confirm (not the locked
notice, which has no capture) adds the caution iconItem at the capture's
cell (156,100). tdnl.74: the FULL state already followed the staged count
(6k12 landed after the H04 image); it is now pixel-graded on the booted
image.

## Gates

Host: `test-finder-icon-art` -- all 8 strikes x captured states, 32bpp +
8bpp, 0/1024 differing cells; `-mutant` puts each strike one row off (RED
on that strike only, all 8) and DESK_ICON_MUT_NO_SELECT (7 selected grades
RED). `test-desk-icons` re-keyed: blitter mechanics graded against the
captures; MASK_IGNORED / ROW_OFF1 / FULL_ROW (21/3072) RED.
`harness/render/icon_capture.h` is the shared factory golden (no artifact
header; missing capture FAILS). test-finder-desktop / test-finder-ops
probes re-read from the v2 maps.

Emulator: `test-flair-finder-icons` (FLAIR_FINDER_ICONS_SPEC; 7 cells:
VOLUME selected, TRASH, DOC selected, FOLDER, 2 x APP, DOC) and
`test-flair-trash-full` (FULL can after staging, mtools-confirmed; the
caution icon on the confirm), both 0/1024 per cell; mutants ROW_OFF1 (7/7),
NO_SELECT (exactly the 2 selected cells), FINDER_DESK_MUT_TRASH_NEVER_FULL
(421/1024), DIALOG_MUT_NO_ICON (572/1024). The four existing icon graders
(desktop-icons, disk-windows, drop-target, trash) grade their sprite cells
against the captures instead of v1 map probes; the trash alert leg now
requires the caution icon where it required an empty area.

Clip: RECORD block `finder_icons` added; `empty_trash` should be
re-recorded after merge (PM).

## Frictions

- No `bd` (the embedded dolt lock is held by another process): beads were
  read from .beads/issues.jsonl and not updated.
- Two lanes already own mint-007 numbers; this mint is 008 as briefed.

## Not done (follow-ups)

- The OPEN state (a volume/folder whose window is open) is captured and
  measured (mint-008 Sec 3) but not drawn: no record carries "window open".
- Apple's own generic hard-disk art (needs a System resource read).
- The drag outline is still a rectangle, not icon-shaped (tdnl.53 note).
- Stop / Note alert icons (no capture).
