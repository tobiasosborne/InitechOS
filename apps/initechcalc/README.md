# InitechCalc

CALC.EXE is the frame's ledger and its separate pie-chart window.
Dependencies: SDK and Initech 123 public worksheet headers/core. Plain target
flags, no semantic defines. The target links Initech 123's public libi123.a;
host tests compile its seven worksheet sources (sheet, fmt, formula, range,
eval, recalc and fs/wk1) for the host architecture.

## The ledger document

Calc opens `APPS\TABLES.TXT` when the user has saved one, otherwise the
shipped `APPS\LEDGER.TXT`. If TABLES.TXT is present but unreadable, Calc says
so and opens LEDGER.TXT; if neither reads, it says so and opens empty. The
reader is all or nothing: a document with any fault is refused whole.

A document is ASCII text, CR LF lines, tab-separated:

    INITECH LEDGER
    WINDOW  l t r b         the ledger window's bounds
    CHART   l t r b         the chart window's bounds
    SERIES  v1 ... vn       the chart's values, 1 to 8, each above zero
    NOTE    text            the source note under the grid (optional)
    ROWS    n               rows in the ledger, 1 to 26
    row  c0 c1 ... c9       a non-empty row, ascending, columns 0 to 9

A cell is empty, a label with its Lotus prefix (`'text`, or `\c` to repeat
`c` across the cell), or a format and a number: `(G) 570-`, `(F2) 98`,
`(F1) 89302`, `(C2) 112276.87` (general, fixed and currency with their
decimals). Numbers are written as Calc prints them, sign at the tail. Saved
bounds are fitted back on screen, keeping their size.

The shipped LEDGER.TXT is the transcription of the surviving still
(`goldens/frame/transcription.tsv`): the certain and probable cells as data,
everything the still does not show empty (operator ruling 2026-11-14,
treatment A), and the note "Blank entries were not recovered from the
surviving material." Document row = fragment row + 9: nine ruled rows stand
for the part of the ledger window hidden under Drive A Files and the chart
(goldens/frame/README.md question 7). The total is a stored number, not a
sum of cells nobody can read. The window bounds are the still's arrangement
mapped to 640 x 480 (geometry.tsv LEDWIN, CHARTWIN). The frame's arrangement
is therefore ordinary saved state: launching Calc reproduces it, and moving
the windows and saving keeps the new places.

## The ledger

Ten columns (label 100 px, eight amounts of 46, one narrow of 10), rows 12 px,
Geneva 9, grey rules, the note below the grid. All cells are left-aligned, as
in the still. A label runs over empty cells to its right and stops at a full
one. Arrows and Tab move the cell pointer; typing enters a value (a number,
with a leading or trailing minus and for currency `$` and commas) or a label;
`\` starts a repeating label; Return commits, Escape abandons, Del clears.
A new number keeps the cell's format; a new cell is general.

## The chart

The series as an elliptical, exploded, extruded pie on a black field, slices
clockwise from 12 o'clock, each value/sum of a turn so the pie closes; each
label is the value with a percent sign. The frame's series is 40 35 18 14 9,
so the labels really sum to 116%. The Toolbox has one blue (navy): faces are
navy, sides a navy and black scanline dither, edges black.

## Numbers

Calc's one format adaptation: Initech 123's display of the absolute value,
then the sign at the tail, in every format: `570-`, `570.00-`, `$1,234.50-`.

## Saving

Every Save writes the model, with both windows' current bounds (the content
rect less CalcDocContentRect's insets), to a stage it owns and asks FILE COPY
to copy the stage to TABLES.TXT under "Saving tables to disk...". An unedited
ledger at its saved places saves byte for byte as it was read.

Save safety follows the operator's failed-save ruling (GM3/GM4): after any
reported failure the previous file must still open under its own name.
Calc reserves stages with CREATE_NEW, advances only on DOS file-exists,
closes every acquired handle even after a failed or short write, and removes
only its own stage. An error probing TABLES.TXT refuses the save rather than
bypassing replace confirmation. A rejected FILE COPY request cleans its
stage immediately; accepted requests keep it until the completion event.
Failure/cancel retains unsaved work and cancels a deferred Quit. A model too
large for the stage refuses the save rather than writing a shortened
document. Destination preservation during FILE COPY belongs to FLAIR's
stage-and-commit transaction. This guarantees reported failures, not crashes.

## Checks

`make test`: four host programs over sdk/host.

- `test_calc_frame` + `tests/frame-content.sh`: the frame's content. The
  script reads goldens/frame/transcription.tsv and geometry.tsv and grades,
  with the same rules, the shipped document (read with awk, no product code)
  and what the real tenant drew (cell texts at cell origins, the note, the
  chart labels, and a ring of chart pixels: a slice face at each value/sum
  middle, a gap within 4 degrees of each boundary). Every certain or
  probable item must show exactly; every unreadable or blank item, and every
  cell outside the fragment, must be empty; the labels must be the still's
  and sum to the canon 116%; the windows must open within 4 px of the
  still's arrangement.
- `test_calc_tenant`: launch, which document opens, the reader's refusals,
  formats (typed Lotus R2 expectations with the sign moved), editing, the
  quit guard.
- `test_calc_file_copy`: Save routing, the byte-for-byte round trip of the
  shipped ledger, saved positions after a drag, reopening the saved tables,
  replace confirmation, stage ownership. Expected bytes are the shipped
  fixture with literal substitutions.
- `test_calc_save_safety` + `tests/save-safety.sh`: 14 failure scenarios;
  the shell compares exported disk files with pre-save snapshots or the
  fixture with the edited row inserted by awk.

The JSON generator check (`include/calc_ledger.json` -> `calc_ledger.h`)
is a serialization regression for the locked literals, not a content oracle.
`make mutants` runs the 18 patches in `mutants/`; each must fail a host
check for its own reason. `make smoke` boots QEMU and Bochs, types 73 into
the first cell, saves through Quit, and checks every saved byte with mtools
against the shipped ledger with that row inserted, the untouched original,
no stage left and fsck.fat. Screendump: build/smoke-desktop/calc.ppm.

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: desktop
Interface use-gate: sdk/include/tbx.h
Interface use-pure: apps/initech123/include/i123.h
Interface use-pure: apps/initech123/include/i123/wk1_format.h
<!-- end-interface-declarations -->
