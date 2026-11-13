# InitechCalc

CALC.EXE is the frame's small worksheet and separate pie-chart window.
Dependencies: SDK and Initech 123 public worksheet headers/core. Plain target
flags, no semantic defines. The target links Initech 123's public libi123.a;
host tests compile its seven worksheet sources (sheet, fmt, formula, range,
eval, recalc and fs/wk1) for the host architecture.

Frozen legacy-final inventory: a two-column, three-row ledger; A1 entry using
the Initech 123 cell engine; five grayscale pie sectors whose labels total
116%; trailing-minus 570- specimen; total $112,276.87; File Save through
FILE COPY, Quit, unsaved-work Save/Cancel/Don't Save and replace confirmation.
Stages avoid occupied temporary names. There are zero visible dead commands.
This imports the existing scene content; it does not recover missing content.

GOLDEN-MASTER item 3 known gap, preserved in include/calc_ledger.json:
TODO_FRAME row count, column count, column headings, all three amounts,
remaining rows, trailing-minus cell address, and all five pie labels. The
unlocated specimen and total remain separate from those unknown cells.
LEDGER.TXT and MICHAEL.CFG are byte copies of the already imported kernel
fixtures, originally legacy-final's fat_diff flagship fixtures; no contents
were invented. MICHAEL.CFG is the existing 8.3 alias for michael_bolton.conf.

make test: two unit/regression suites, 424 drawing/guard checks and 30
FILE COPY/stage/replace checks, <1 second. Tests use sdk/host, with explicit
completion events and fixture copying; recorded rectangles are rasterized
only in the checker. Expectations are independent PRD literals. Formatting
and .WK1 behavior come from Initech 123, whose own Lotus differential is the
oracle for that core. The JSON generator is a serialization regression,
not an independent content oracle. make mutants converts all 12 hooks to
patches and checks each twice. Hook removal preserved the pre-integration
CALC.EXE (61,008 bytes); linking the public worksheet archive at integration
produces 55,408 bytes because unused archive members are not pulled in.
make smoke runs QEMU and Bochs, enters 73, saves, and checks every saved byte
with mtools/cmp, untouched original ledger, deleted stage and fsck.fat.
Screendump: build/smoke-desktop/calc.ppm.
Remaining legacy scene/pixel and FILE COPY failure gates belong to factory.
