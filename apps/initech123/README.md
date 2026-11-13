# Initech 123

The worksheet store, formula compiler/evaluator, recalculation, range operations,
Lotus WK1 codec, slash-menu model and native Toolbox face. Present feature depth
is preserved; this import adds no spreadsheet behaviour.

Public headers are `include/i123.h` (the core/codec API), `include/menu.h`
(the menu API) and `include/i123/wk1_{format,formula}.h` (format contracts).
Consumers use `-Iapps/initech123/include`. InitechCalc uses i123.h and wk1_format.h,
linking sheet.c, fmt.c, wk1.c, formula.c and range.c, as verified against the
legacy tenant/calc.exe recipe. There are no core/codec dependencies on the OS.
The face depends on sdk/tbx.h and sdk/tbx_save.h; all OS calls go through it.

Run `make` to compile nine freestanding i386 objects under build/apps/initech123.
It uses the shared TARGET_CC/TARGET_CFLAGS with no size/ABI tuning. The semantic
I123_MENU_DISK define preserves the external APPS/123.MNU loading path. Linking
123.EXE awaits sdk tenant rules. `make test` builds and runs 18 host entries,
using mk/config.mk and mk/host-test.mk. Logs and isolated fresh scratch directories
are under build/apps/initech123/host. No emulator runs. Set LOTUS123_DECOMP to the local
lotus123-decomp corpus; absence is an error. Python 3 runs the corpus's independent
reader and the two unchanged legacy adapters claimed into tests/oracles; they do
not share format constants with the product. The factory additions are make and
POSIX shell. All legacy cases run by default; no reduced/THOROUGH subset is needed.

Oracles are real R2.2 WK1 files and cached formula results, mint step files,
123.RI strings/offsets, captured screenshots and manual examples, plus independent
property models. See tests/IMPORT.md for the inventory and legacy comparison.
The checked-in menu_data.h is authored source; test_i123_menu_tree --resource is
the existing C generator for 123.MNU. Its output is compared with real 123.RI and
the captured tree, including embedded and loaded-resource child links.

Local build extensions: the host-test runner macro wraps tests/run.sh to preserve
multi-command oracle checks, clean scratch, resource comparisons and the expected
53-bit x87 failure. Normal and x87 core objects are shared between executables to
keep clean builds fast; per-suite CFLAGS apply to each test translation unit.
_DEFAULT_SOURCE exposes MAP_ANONYMOUS for the existing fork-based face suites.

## Borrowed from attic

ATTIC_SPEC in the Makefile is temporary: cpanels_record.h, toolbox_gate.h,
cp_dialog.h and event_model.h belong to other importers. event_model.h pulls in
private graphics headers through grafport.h, whose imaging.h still names the old
os/flair/surface.h. tests/borrow-events.sh generates a header-only view under
build/apps/initech123/borrowed: the original flair_point_t declaration extracted from
grafport.h followed by event_model.h with only its grafport include removed.
No event definitions are hand-copied, and all original EventRecord assertions run.
This temporary SDK bridge must disappear when sdk exports self-contained events.
Only the face sees the borrowed headers; core and fs compile with -Iinclude alone.
No attic implementation or legacy make fragment is built.

## Limits and deferred work

The 256 by 8192 address space has 2048 populated cells, a 32 KiB cell pool per
bank and 4096 bytes of peripheral/name records. File links retain cached values;
three named mint exceptions are recorded by the formula oracle. Unsupported
menu leaves still visibly refuse; the master release's dead-command sweep and
full Lotus parity are not implemented here. tests/oracles/menu/era_shortfalls.md
records the historical feature inventory, including retired size/process notes.

The recording fake is in tests/fake_tbx/recording.h. A shared sdk/host replacement
needs cell text and background recording, window origin/content sizing, scroll
and beep counters, alert contents/buttons/results, exit recording, a controllable
guest clock and DOS-name, case-insensitive file verbs (create/create-new, open,
read/write/seek/close/delete/rename), with injectable create/write failures.
The current face suites also inspect app-static state and use fork/shared counters
for pristine launches; their driver must remain component-local when the gate fake
moves. Tests for save/quit and currency independently read the saved bytes back.

Mutation hooks remain unchanged pending patch conversion (tests/MUTANTS-TODO.md).
Tier 1 is not ported; `make smoke` fails with a pointer to tests/SMOKES.md.
