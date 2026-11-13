# Import inventory and verification

Source baseline: legacy-final, compared with the read-only legacy checkout at
/home/tobiasosborne/Projects/InitechOS-legacy. No behavioural product edits.
All paths here are relative to apps/i123 unless otherwise stated.

## Claimed with git mv

- Eight proptest/test_i123_*.c suites -> tests/.
- Six diff/wk1_diff/test_i123_*.c suites -> tests/diff/.
- Two independent-reader adapters -> tests/oracles/.
- spec/i123/wk1_format.h and wk1_formula.h -> include/i123/.
- Remaining spec/i123 files -> tests/oracles/menu/ (offsets, captured tree,
  level visits and historical feature inventory).
- fixtures/i123-{copy,numeric,keys,width} -> tests/fixtures/ (all 111 files,
  including capture provenance, steps, screenshots and WK1/ref files).
- core/i123.h and menu.h -> include/; menu_data.h stays in core/.

The test_assert.h macros are a local copy of tps/seed/test_assert.h; the seed
owns that file, so it was not moved. The recording fake/driver was extracted
unchanged from the moved quit suite into tests/fake_tbx/recording.h.
No menu-data generator was found beyond the existing C --resource mode, which
writes 123.MNU and is retained in the menu host suite.

## Host suites and legacy parity

ORACLE-STRONG: six source suites / ten invocations, including property models
and independent Lotus differentials. UNIT: eight face source suites / eight
invocations; these grade routing/state against authored requirements and manual
or captured expectations using the recording fake. The face quit/entry suites
also have independent saved-file post-checks, retained from the legacy recipes.
The 53-bit x87 invocation is an expected negative control, not a product mutant.

All corresponding legacy targets were actually run, not merely dry-run:

```
. tools/lane-env.sh
make test-i123-wk1-roundtrip test-i123-formula test-i123-recalc-order \
 test-i123-quit test-i123-menu-tree test-i123-copy-move test-i123-range-menu \
 test-i123-entry test-i123-clock test-i123-keys test-i123-hit \
 test-i123-resize test-i123-cells
```

Counts below are legacy / imported. Test names are `test_i123_<suffix>`.

| Entry | Class | Legacy / imported checks or cases |
|---|---|---|
| wk1_roundtrip | ORACLE-STRONG | 454 / 454 assertions; 55 / 55 real files; 44 / 44 display rows |
| formula | ORACLE-STRONG | 152 / 152 assertions; 1090 / 1090 formulas; 4000 / 4000 random formulas |
| formula_x87_037F | ORACLE-STRONG | 152 / 152 assertions; same full formula suite |
| formula_x87_027F | ORACLE-STRONG | 152 / 152 assertions; 2 / 2 expected assertion failures, 3 / 3 A/B diagnostics |
| recalc_order | ORACLE-STRONG | 187 / 187 assertions |
| copy_move | ORACLE-STRONG | 1020041 / 1020041 assertions; 4000 / 4000 random sheets |
| copy_corpus | ORACLE-STRONG | 8 / 8 real before/after pairs, 5 Copy + 3 Move, 3 step files |
| copy_tile | ORACLE-STRONG | 4 / 4 real SINGLE/ROW/OVER/BLOCK fixture comparisons |
| transfer_baseline | ORACLE-STRONG | 12 / 12 real-sheet self-transfers and FORMULA-body comparisons |
| menu_tree | ORACLE-STRONG | 82 / 82 levels, 381 / 381 items; all 3 full tree comparisons retained |
| quit | UNIT | 60 / 60 assertions; Q/PE/X independent file checks; failed-save absence checks |
| range_menu | UNIT | 2092 / 2092 assertions |
| entry | UNIT | 73 / 73 assertions; CURRENCY independent file checks |
| clock | UNIT | 14 / 14 assertions |
| keys | UNIT | 62 / 62 assertions |
| hit | UNIT | 38 / 38 assertions |
| resize | UNIT | 55 / 55 assertions |
| cells | UNIT | 28 / 28 assertions |

Total: 14 source suites, 18 invocations; 1,023,560 counted assertions in both
builds (including the 152-check expected-negative precision run), plus the
separately counted 8 command pairs, 4 tiling cases, 12 self-transfers and complete
82-level/381-item menu comparisons. No cases were reduced or silently skipped.
The formula suite retains 37 typed examples, 986 independently reproduced
caches (840 numeric, 100 string, 46 ERR/NA), 101 ungraded file links and three
named exceptions, 34 poisoned/recalculated whole files (980 caches), and 97
manual examples, on both positive arithmetic builds.

## Corpus inputs

59 distinct real WK1 inputs: 38 shipped and 17 minted in LOTUS123_DECOMP, plus
4 claimed local Copy fixtures. Formula uses the 38 shipped + first 6 mints;
codec uses all 55. Copy/move replays 3 corpus mint/*.steps files; baseline uses
CIRC01 and MINT01..05 twice each. Menu reads real installed 123.RI plus the
claimed offset table and golden. Numeric/key/width captures are evidence for
hard-coded expectations; their ancillary WK1 files are preserved, not claimed
as additional exercised differential inputs.

Shipped: AWSAMPLE CAMBRIDG LONDON MONTREAL NEWYORK QSSOURCE QSTARGET QSTART1X
QSTART2X QSTART4X QSTART5X SAMP1X SAMP2X SAMP3X SAMP4X SAMP6X SAMP7X SAMPBAL
SAMPCASH SAMPINC SAMPMACS TUTOR10X TUTOR11X TUTOR12X TUTOR13X TUTOR14X TUTOR15X
TUTOR17X TUTOR18X TUTOR19X TUTOR20X TUTOR2X TUTOR4X TUTOR5X TUTOR6X TUTOR7X
TUTOR8X TUTOR9X.
Minted: CIRC01 MINT01 MINT02 MINT03 MINT04 MINT05 COPY01A COPY01B COPY01C
COPY01D MOVE01A MOVE01B MOVE01C MOVE01D WRAP01A WRAP01B WRAP01C.
Local: SINGLE ROW OVER BLOCK.

## Left in attic

No standalone i123 host source suite was excluded as META or SIZE/LAYOUT.
The following are intentionally not imported as host tests:

- check_i123_menu_levels.c and check_i123_numeric.c: emulator-output oracle
  helpers; strong checks, but require serial captures or RTC-generated guest
  files. Their gates are listed in SMOKES.md, not silently replaced by host tests.
- mint_i123_numeric.c and mint_i123_copy_steps.c: fixture minting utilities,
  not tests (META/support); the captured fixtures themselves were claimed.
- i123_numeric.mk, flair_i123*_traces.mk and i123_copy_smoke_keys.h: retired
  emulator/capture wiring. Kept pending the tier-1 factory import.
- harness/test_gate_honesty.sh, fixture_volumes.mk and emu/test_bochs_legs.c:
  META tests of retired factory accounting/skip policy/scratch volumes/backends;
  their mention of i123 does not make them spreadsheet behaviour tests.
- The 64 KiB `tenant-exe-rules` size guard (legacy Makefile:10641) and
  entry/relocation/link layout checks belong to SIZE/LAYOUT, not tier 0 here.
- test_calc_tenant.c, test_calc_file_copy.c, test_finder_icon_oracles.c and
  test_iword_face.c mention WK1/123 but exercise other components and stay there.
- Shared spec headers remain owned by their future importers; see README.

## Flags and boundaries

Dropped from i123's legacy compile/link tuning:
-Oz, -flto, -ffunction-sections, -fdata-sections, -fno-jump-tables,
-fno-inline, -fno-ipa-cp-clone, -mpreferred-stack-boundary=2,
-mincoming-stack-boundary=2, -fomit-frame-pointer, -fno-tree-loop-im,
-fno-tree-sra, -fno-tree-dominator-opts, -fno-tree-scev-cprop,
-fno-move-loop-invariants, -fno-shrink-wrap,
--param=max-completely-peel-times=0, -mregparm=3, -mrtd, -freg-struct-return,
-fno-caller-saves, -mtune=generic, -fno-if-conversion, -fno-split-wide-types,
-fno-tree-switch-conversion, -malign-data=abi, -fno-tree-cselim,
-fno-tree-coalesce-vars, -fno-ipa-strict-aliasing, -fno-tree-phiprop.
Legacy common flags absent from TARGET_CFLAGS were not added either:
-fno-pie, -fno-tree-loop-distribute-patterns, -fno-unwind-tables, -fno-common.
There is no LTO linker or --gc-sections in this objects-only build.

No additional ABI convention is required: sdk's tbx_trap is explicitly cdecl
regparm(0), kernel-facing runtime entry points take no arguments, and the future runtime must
use the same default C ABI as these objects. I123_MENU_DISK is retained because
it selects external resource loading, not a code-size optimisation. Native m32
arithmetic is x87; the 0x037F runtime control word remains an SDK/kernel contract.
Host positive/negative x87 builds retain -mfpmath=387 / I123_TEST_X87_CW.

All eight core/fs translation units compile with only -Iinclude, for host and
target. Face adds sdk and temporary shared headers. The only discovered boundary
problem was sdk -> event_model -> grafport -> imaging -> old private surface.h;
the generated header view described in README removes that unused graphics
coupling without taking shared files or copying behaviour. No files in mk/ are
edited; only COMPONENTS changes at the top level.

mk/host-test.mk needs a per-test command-wrapper hook for fresh scratch and
multi-command independent checks; host_test_run is locally overridden with the
same status/summary protocol. It recompiles all product C sources per executable,
so component-local normal/x87 object rules avoid duplicate compilation while
using the fragment for linking/running every entry. Explicit dependencies cover
the included face/quit driver and borrowed SDK headers. These are the only local
extensions beyond the component's target build rules and corpus preflight.

## Acceptance evidence

From a clean component output directory:

```
make -C apps/i123 clean
/usr/bin/time -f 'wall %e seconds' make -C apps/i123 test
apps/i123: 18 tests, 0 failures
wall 15.41 seconds
make -C apps/i123
file build/apps/i123/core/*.o build/apps/i123/fs/*.o build/apps/i123/app/*.o
# all nine: ELF 32-bit LSB relocatable, Intel 80386
make test
apps/i123: 18 tests, 0 failures
ALL GREEN
```

Red once: changed only eval.c's @ROUND offset from `+ 0.5` to `+ 0.4`, then
ran the full `make -C apps/i123 test`. Exit 2, 18 tests / 2 failed entries;
formula native and x87-037F both reported cached values/recalculated file mismatch
and `@ROUND(134.578,0) shows "134", the manual prints "135" (PDF 341)`.
The original evaluator was restored byte-for-byte and the clean/full runs above
were performed after restoring it. No broken code or red objects are committed.

`make -C apps/i123 test LOTUS123_DECOMP=/no/such/lotus123-corpus` exited 2:
`FAIL: Lotus corpus missing; set LOTUS123_DECOMP to the lotus123-decomp directory`.

`git diff --check` and `sh -n tests/run.sh tests/borrow-events.sh` passed.
Product C/public headers contain no non-ASCII bytes. The core/fs compiler commands
have only the component's include directory; app additionally has sdk and the
explicit temporary shared-header borrowing. No attic C or make fragment is built.
