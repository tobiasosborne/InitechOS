# Initech 123

The worksheet store, formula compiler/evaluator, recalculation, range operations,
Lotus WK1 codec, slash-menu model and native Toolbox face. Present feature depth
is preserved; this import adds no spreadsheet behaviour.

Public headers are `include/i123.h` (the core/codec API), `include/menu.h`
(the menu API) and `include/i123/wk1_{format,formula}.h` (format contracts).
Consumers use `-Iapps/initech123/include`. InitechCalc uses i123.h and wk1_format.h,
linking sheet.c, fmt.c, wk1.c, formula.c and range.c, as verified against the
legacy tenant/calc.exe recipe. There are no core/codec dependencies on the OS.
The face depends on sdk/include/tbx.h and sdk/include/tbx_save.h; all OS calls go through it.

Run `make` to build the real 123.EXE and its 123.MNU resource under
build/apps/initech123 through mk/tenant.mk. Product sources use the shared
TARGET_CC/TARGET_CFLAGS without size or calling-convention tuning; the semantic
I123_MENU_DISK define retains external APPS/123.MNU loading. The resource comes
from the existing C test_i123_menu_tree --resource generator and is checked
against real 123.RI and the captured tree by the host suite. Two target links
derive relocations; an independent factory/mz reader validates the EXE and BSS.
The vertical slice launches this plain-flags EXE through the Finder and verifies
a saved worksheet on QEMU and Bochs; the loader's former file cap is removed.

`make test` runs the same 18 host entries through mk/config.mk and
mk/host-test.mk, with the single sdk/host library for all Toolbox calls. Logs
and fresh scratch directories are under build/apps/initech123/host. No emulator
runs. Set LOTUS123_DECOMP to the local lotus123-decomp corpus; absence is an
error. Python 3 runs that corpus's independent reader and the two unchanged
legacy adapters claimed into tests/oracles. The factory additions are C, make
and POSIX shell. All legacy cases run by default. See tests/IMPORT.md for the
unchanged counts, oracles and legacy comparison.

Local build extensions: the host-test runner macro wraps tests/run.sh to preserve
multi-command oracle checks, clean scratch, resource comparisons and the expected
53-bit x87 failure. Normal and x87 core objects are shared between executables to
keep clean builds fast; per-suite CFLAGS apply to each test translation unit.
_DEFAULT_SOURCE exposes MAP_ANONYMOUS for the existing fork-based face suites.

## SDK boundary

The face compiles with its own include directory and sdk/include alone. There
are no attic or private graphics includes. The event-header bridge and private
fake have been deleted. tests/face_driver.h keeps only app-state access, fork
launching and aliases to sdk/host recording fields. Each child resets the fake,
imports its scratch fixtures, drives the app, then exports files for the same
independent saved-byte checks. Toolbox file operations themselves use memory.
Core and fs still compile with only -Iinclude.

## Limits and deferred work

The 256 by 8192 address space has 2048 populated cells, a 32 KiB cell pool per
bank and 4096 bytes of peripheral/name records. File links retain cached values;
three named mint exceptions are recorded by the formula oracle. Unsupported
menu leaves still visibly refuse; the master release's dead-command sweep and
full Lotus parity are not implemented here. tests/oracles/menu/era_shortfalls.md
records the historical feature inventory, including retired size/process notes.

The SDK fake records cell/background/style/color drawing, window geometry,
alerts, exit, scrolling, beeps and an injectable guest clock. Its in-memory
DOS-like volume supports the save failure cases and explicit fixture export.
This component retains all app-specific test driving and assertions.

Mutation hooks remain unchanged pending patch conversion (tests/MUTANTS-TODO.md).
Tier 1 is not ported; `make smoke` fails with a pointer to tests/SMOKES.md.

Shared build conventions: command stamps rebuild on tool/flag changes; C
translation units use -MMD dependencies. TEST_BUDGET_S is 30 seconds for the
complete tier-0 selection; measured execution fits the default, excluding builds.
Standalone tests verify their consumed files in goldens/CORPORA.lock; the
top-level test run verifies the full lock once before parallel execution.
The canonical factory assertion header replaces the private copy. Tenant
linking retains 123.EXE and 123.MNU. Product mutation hooks block import audit.
