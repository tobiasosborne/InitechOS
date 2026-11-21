# InitechCalc

CALC.EXE is the frame's ledger and its separate pie-chart window.
Dependencies: SDK and Initech 123 public worksheet headers/core. Plain target
flags, no semantic defines. The target links Initech 123's public libi123.a;
host tests compile its seven worksheet sources (sheet, fmt, formula, range,
eval, recalc and fs/wk1) for the host architecture. Host geometry checks also
use FLAIR's public chrome metrics.

## Menus

Frozen inventory: one menu, File, with Save (Ctrl-S) and Quit (Ctrl-Q). The
bar shows the front program's menus (operator, 2026-11-13), so while Calc
saves the bar is the Initech mark, File and the clock: that is Calc's whole
set, not a cut-down one. The still's Photoshop and worksheet menus belong
to programs Calc is not (goldens/frame/README.md question 13); a menu with
no working command is not added (GOLDEN-MASTER item 4).

## The ledger document

Calc opens `APPS\TABLES.TXT` when the user has saved one, otherwise the
shipped `APPS\LEDGER.TXT`. If TABLES.TXT is present but unreadable, Calc says
so and opens LEDGER.TXT; if neither reads, it says so and opens an empty
ledger of 26 rows with an empty chart, an ordinary document whose entries
save and reopen (initech-gs1ww). The reader is all or nothing: a document
with any fault is refused whole.

A document is ASCII text, CR LF lines, tab-separated:

    INITECH LEDGER
    WINDOW  l t r b         the ledger window's bounds
    CHART   l t r b         the chart window's bounds
    SERIES  v1 ... vn       the chart's values, 0 to 8, each above zero
    NOTE    text            the source note below the total (optional)
    ROWS    n               rows in the ledger, 1 to 26
    row  c0 c1 ... c9       a non-empty row, ascending, columns 0 to 9

A cell is empty, a label with its Lotus prefix (`'text`, or `\c` to repeat
`c` across the cell), or a format and a number: `(G) 570-`, `(F2) 98`,
`(F1) 89302`, `(C2) 112276.87` (general, fixed and currency with their
decimals). Numbers are written as Calc prints them, sign at the tail, in
Initech 123's shortest form, which is exponent form for very small and very
large values (`(G) 1E-7`, `(G) 1.5E21-`); the reader and the typed-entry
parser accept that form (initech-3yye3). The number written is the stored
double itself, never the 15-digit control-panel text, so a typed
1234567890123456 saves as itself (initech-qdscg). A number that would not
read back as itself refuses the save. Saved bounds are fitted back on screen, keeping
their size.

The shipped LEDGER.TXT is the transcription of the surviving still
(`goldens/frame/transcription.tsv`): the certain and probable cells as data,
everything the still does not show empty (operator ruling 2026-11-14,
treatment A), and the note "Blank entries were not recovered from the
surviving material." Document row = fragment row + 9: nine ruled rows stand
for the part of the ledger window hidden under Drive A Files and the chart
(goldens/frame/README.md question 7). The total is a stored number, not a
sum of cells nobody can read. The chart window is the still's CHARTWIN
mapped to 640 x 480, (349,72)-(598,288): a no-grow document window (NEWWINDOW
WDEF byte 4, FLAIR's noGrowDocProc, 3eda89aa) whose field runs to its frame
as in the still, with no scroll-bar gutter to cut the pie, and no title
(CHT.TITLE). The ledger window
opens at LEDWIN's origin (116,104) but is larger than LEDWIN, to (622,474)
against (568,449), because its grid is: amount columns of 46 pixels against
about 41 and rows of 12 against about 11 (LEDGRID). That is a stated
deviation carried from the import, to be settled against the full frame
reference. The frame's arrangement
is therefore ordinary saved state: launching Calc reproduces it, and moving
the windows and saving keeps the new places.

## The ledger

Ten columns (label 100 px, eight amounts of 46, one narrow of 10), rows 12 px,
Geneva 9 and grey rules. Treatment A puts the note in a white strip a row
deep at the foot of the ledger, below the last row's rule and so below the
total and its double rule, in black Geneva 9, 4 pixels in, with no rules
(operator, 2026-11-15: goldens/frame/README.md question 9; it was above the
total until then). The strip is layout, not a worksheet row; it stays below
the upper chart and copy dialog. The shipped ledger's 26 rows end at 312 and
the note's line at 324, inside the window's 327 rows of content; the window
does not scroll, so a ledger shrunk below that clips the foot first
(initech-j89c). All cells are left-aligned, as
in the still. A label runs over empty cells to its right and stops at a full
one. Arrows and Tab move the cell pointer; typing enters a value (a number,
with a leading or trailing minus and for currency `$` and commas) or a label;
`\` starts a repeating label; Return commits, Escape abandons, Del clears.
A new number keeps the cell's format; a new cell is general.

## The chart

The series as an elliptical, exploded, extruded pie on a black field,
covering on screen the box the still shows (geometry.tsv PIE and PIEDEPTH:
378,107 to 581,265 with the chart at its saved place), slices
clockwise from 12 o'clock, each value/sum of a turn so the pie closes; each
label is the value with a percent sign. The frame's series is 40 35 18 14 9,
so the labels really sum to 116%. Faces are the still's slice blue and sides
its darker blue (goldens/frame/colours.tsv; the Toolbox's TBX_COLOR_SLICE,
#4282FF, and TBX_COLOR_SLICE_SIDE, #334A87), edges black.

Exactly, in the chart's content pixels: the disk's centre is (124,70) and its
top face the pixel centres with (x/96)^2 + (y/55)^2 <= 1. Each slice is
pushed out along its middle by 6 pixels across and 6*55/96 down, each cut to
whole pixels toward zero. Its side is its face swept 42 rows down, in the
side's blue; the row below each face pixel that is not face is a black edge.
A slice of more than half a turn is drawn as its
two halves. Slices are drawn from the back (middle nearest 12 o'clock) to the
front, ties in series order. A label, white on the face's blue, has its top-left corner
5 rows above and 3 pixels per character left of the point 59 x 34 pixels
(62% of the axes) out along the middle from the pushed-out centre. Angles
are kept to 1/65536 of a turn, and the sine is the series to x^11 in fixed
point (within about 1e-7), so an edge lies where value/sum puts it; the
short table it replaced missed by up to about 0.2 pixels at the rim.

## Painting

Every FILLRECT and TEXTDRAW is an INT 81h gate call that FLAIR clips and
traces, so Calc paints with as few as its picture allows (initech-kzras;
since ee66540c the pie and ledger cost about 10,000 calls a session and the
Bochs journey ran out of time). The pie is drawn into Calc's own memory and
sent as rectangles over the black field, each running down as far as its
colour covers it: the sides first, which may run under the faces, then the
faces over them. The ledger is painted a band of rows at a time, a column at
a time, so a column's rule and stops are one call per run of rows; a
keystroke repaints only the band between the row it left and the row it is
on. The pixels are those of the cell-by-cell painting they replaced
(test_calc_picture, below). An update repaints the window its message names
by NEWWINDOW token (sdk tbx.h, cd590563; IM I-59); any other message
repaints both, which is what happens until FLAIR's gate sends the token in
place of its resident WindowPtr (initech-3zcsf).

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
Calc reserves stages with CREATE_NEW (`TABLES.TMP`, then `TABLE~1.TMP` to
`TABLE~9.TMP`, `TABL~10.TMP` and so on to `T~99999.TMP`: 100,000 names, more
than a FAT directory can hold, so leftovers never stop a Save before the
directory is full; initech-40kpb), advances only on DOS file-exists, closes every acquired
handle even after a failed or short write, and removes only the stage the
current Save created, while that Save is still going on (the FILE COPY box
is modal). Once a Save is over its stage is no longer provably Calc's: the
user can move or replace it in the Finder, and a later session has no record
of it. So a stage whose DELETE DOS refuses twice is named to the user
("Temporary file remains:" and the path, after "The worksheet could not be
saved." when it could not), left exactly as it is, and never deleted later
(initech-zev40); the next Save takes the next free name (initech-40kpb). A
Quit waiting on a landed Save quits once that notice is dismissed. An error probing
TABLES.TXT refuses the save rather than
bypassing replace confirmation. A rejected FILE COPY request cleans its
stage immediately; accepted requests keep it until the completion event.
Failure/cancel retains unsaved work and cancels a deferred Quit. A model too
large for the stage refuses the save rather than writing a shortened
document. Destination preservation during FILE COPY belongs to FLAIR's
stage-and-commit transaction. This guarantees reported failures, not crashes.

## Checks

`make test`: six host programs over sdk/host.

- `test_calc_frame` + `tests/frame-content.sh`: the frame's content. The
  script reads goldens/frame/transcription.tsv and geometry.tsv and grades,
  with the same rules, the shipped and saved documents (read with awk, no product code)
  and what the real tenant drew (cell texts at cell origins, the note and
  its place, below the total's row and every other row (the total is the row
  above the transcription's double rules; red when the note stood above
  it), the chart labels, the chart window's title against CHT.TITLE (empty), and
  the chart replayed on the screen at the window's
  observed content origin and clipped to its observed size: every slice
  pixel's bounds within 4 px of geometry.tsv PIE/PIEDEPTH, and a ring at 80%
  of geometry.tsv's PIE ellipse with a slice face at each value/sum middle
  and a gap within 4 degrees of each boundary). The pie's expected place
  comes from the reference package, never from the renderer
  (initech-42lrp). Every certain or
  probable item must show exactly; every unreadable or blank item, and every
  cell outside the fragment, must be empty; the labels must be the still's
  and sum to the canon 116%; the windows must open within 4 px of the
  still's arrangement.
- `test_calc_tenant`: launch, which document opens, the reader's refusals,
  formats (typed Lotus R2 expectations with the sign moved), editing, the
  quit guard, typed missing cells, white ruled gaps, and note order/visibility.
  The source, shipped ledger and drawing are also checked for retired
  placeholders. Existing transcription data is preserved; this treatment
  introduces no recovered labels or amounts.
- `test_calc_file_copy`: Save routing, the byte-for-byte round trip of the
  shipped ledger, saved positions after a drag, reopening the saved tables,
  replace confirmation, stage ownership, exponent-form numbers typed, saved
  and reopened as the typed values, numbers with more digits than the
  control panel shows saved as the stored double (the worksheet's numeric
  accessor before Save; the saved TABLES.TXT read back with the C library's
  strtod), the empty ledger's entry saved and reopened, and a stage left by a
  refused DELETE named in the notice, kept byte for byte and passed over by
  the next Save.
  Expected bytes are the shipped fixture with literal
  substitutions, or a document written by hand from the format above.
- `test_calc_save_safety` + `tests/save-safety.sh`: 14 failure scenarios
  and two cleanup sequences; the shell compares exported disk files with
  pre-save snapshots or the fixture with the edited row inserted by awk.
  `cleanup-replaced`: a cancelled copy's undeletable stage, then replaced by
  the user with an unrelated file, which must survive the next Save.
  `restart`: eleven sessions over one disk, each Quit > Save with its
  stage's DELETE refused, then a twelfth session without the fault that must
  save (red on the ten-name reserve). `all-collisions`: ten unrelated files
  hold TABLES.TMP to TABLE~9.TMP; the Save takes TABL~10.TMP and leaves them.

- `test_calc_paint`: the gate-call budget and the repaint property. One
  update of the shipped ledger at most 700 drawing calls, one keystroke at
  most 80 (a budget for the Bochs journey's 90 s limit, not a measurement;
  the code before it made 1,472 and 380; 600 until the slices took two
  colours, 635 since). After every keystroke of a
  sequence (typing, Return, arrows, Tab into the narrow column, a label run
  over its neighbours and cleared, Escape, Del, down to the total's rows
  and back) what the windows show equals a fresh full repaint, replayed
  pixel by pixel by tests/replay.h with FLAIR's Geneva 9 strike. An update
  naming token 2 draws only in the chart, token 1 only in the ledger, and a
  message that is no token in both; each leaves a full repaint's picture
  (red before the change: 183 + 340 calls for either token).
- `test_calc_picture`: the full repaint against a picture built without
  Calc's drawing code (initech-dbdp3), every pixel of both windows' content.
  The ledger is painted cell by cell, left to right, from transcription.tsv's
  certain and probable readings and the layout above, in FLAIR's Geneva 9.
  The pie is a per-pixel test in floating point with the C library's sine
  from the specification under "The chart", and the model's own reach must
  be geometry.tsv's PIE/PIEDEPTH box exactly. Pixels whose centre is within
  the 1/65536-turn rounding of a slice's edge may be either of the two
  colours the two roundings give (12 for
  the frame, capped at 16); a cut to whole pixels that rounding could tip
  (one for the frame, at most 4) is tried both ways. Charts of one slice,
  eight equal slices, 1 and 999, and none are graded too. Seen red on a pie
  that drops its lone single pixels (`calc_mut_pie_drops_pixels`, which
  test_calc_paint and test_calc_frame pass), on the 17-point sine table it
  replaced (47 pixels), and on a ledger without its bottom rule or with text
  a pixel to the right.

The JSON generator check (`include/calc_ledger.json` -> `calc_ledger.h`)
is a serialization regression for the locked literals, not a content oracle.
`make mutants` runs the 37 patches in `mutants/`; each must fail a host
check for its own reason. `make smoke` boots QEMU and Bochs, types 73 into
the first cell, saves through Quit, and checks every saved byte with mtools
against the shipped ledger with that row inserted, the untouched original,
no stage left and fsck.fat. On QEMU it also grades screendumps with
`tests/frame-pixels.sh`, every expectation from goldens/frame, colours to
3/255 (DAC quantisation). `calc.ppm`, before Save: the chart window's outline
exactly at geometry.tsv CHARTWIN, the pie's slice pixels within 4 px of
PIE/PIEDEPTH, a slice face at each slice middle on the PIE ellipse, black
at colours.tsv's chart-field samples mapped through SCREEN, and at each of
its slice samples (three faces, two sides) the per-channel median of the
box within 5/255 of its role's colour, the area-weighted median of the
role's samples (#4282FF, #334A87). Seen red on main before the slice blues:
every sample's box read navy or black. `calc-copy.ppm`,
while FILE COPY runs: the chart's and FILE COPY's outlines exactly at
CHARTWIN and COPY where nothing in front covers them (COPY's foot 11 rows
lower for Cancel, FLAIR's stated departure), the caption's text starting at
COPYCAPTION and the bar's outline at COPYBAR within 4 px, and no slice colour
showing through FILE COPY. Seen red on the captures from before FLAIR placed
FILE COPY at the still's box (1218 outline failures, navy through the box)
and on edited copies of a green capture (caption moved 10 px right, bar 8 px
down, navy painted in the box, box 5 px left). Before Save the journey moves
the pointer to the centre of geometry.tsv COPYHOURGLASS, as a user would
leave it beside the caption, so the canon hourglass is graded where the still
has it: in `calc-copy.ppm` or one of six more frames 60 ms apart
(`calc-glass-N.ppm`), black pixels bounded within 4 px of COPYHOURGLASS, both
ends at least 3/4 of its width and the waist at most 1/4 (never a watch).
FLAIR shields the cursor while it repaints the bar (flair/tests/
smoke-filecopy.sh), so a single frame can show none; one frame of seven must.
Seen red with the pointer left at the trace's anchor (no hourglass in the
box) and on edited copies of a green frame (waist filled out; moved 6 px
right). Bochs has no capture stream in the desktop journey framework, so it
makes no pixel claim.

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
