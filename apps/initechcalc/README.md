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

Save safety follows the operator's failed-save ruling (GM3/GM4): after any
reported failure the previous file must still open under its own name.
Calc reserves stages with CREATE_NEW, advances only on DOS file-exists,
closes every acquired handle even after a failed or short write, and removes
only its own stage. An error probing TABLES.TXT refuses the save rather than
bypassing replace confirmation. A rejected FILE COPY request cleans its
stage immediately; accepted requests keep it until the completion event.
Failure/cancel retains unsaved work and cancels a deferred Quit. Destination
preservation during FILE COPY belongs to FLAIR's stage-and-commit transaction;
Calc needs no new Toolbox verb. This guarantees reported failures, not crashes.

There is no Open, Save As, or ledger parser in this frozen inventory. Launch
initializes the sheet from the compiled content; an unedited Save copies
LEDGER.TXT verbatim through FILE COPY. Source open/read errors, including a
file truncated during copying, belong to FLAIR's transfer implementation.
Calc never parses a partial ledger into its live worksheet.

make test: three unit/regression suites, 424 drawing/guard checks, 30
FILE COPY/stage/replace checks, and 14 save-safety scenarios. Tests use sdk/host, with explicit
completion events and fixture copying; recorded rectangles are rasterized
only in the checker. Expectations are independent PRD literals. Formatting
and .WK1 behavior come from Initech 123, whose own Lotus differential is the
oracle for that core. The JSON generator is a serialization regression,
not an independent content oracle. The safety suite injects OPEN, CREATE_NEW,
WRITE (including zero/short disk-full writes), CLOSE and copy-request faults;
it checks handles and retry/quit behavior. Its independent shell reader
compares exported disk files against pre-save snapshots (or literal successful
save bytes), including every occupied stage name. FILE COPY completion here
is a fixture of FLAIR's contract, not proof of that transfer engine.
make mutants checks all 14 patches twice. Hook removal preserved the pre-integration
CALC.EXE (61,008 bytes); linking the public worksheet archive at integration
produces 55,408 bytes because unused archive members are not pulled in.
make smoke runs QEMU and Bochs, enters 73, saves, and checks every saved byte
with mtools/cmp, untouched original ledger, deleted stage and fsck.fat.
Screendump: build/smoke-desktop/calc.ppm.
Remaining legacy scene/pixel and FILE COPY failure gates belong to factory.

Initial windows meet the single menu bar (operator ruling, CLAUDE.md
"Hazards that are still true"). Host routing checks their structure top
against the public FLAIR_CHROME_MENUBAR_H metric; content sizes are preserved.

Host geometry tests also depend on flair/include/chrome_metrics.h.

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
