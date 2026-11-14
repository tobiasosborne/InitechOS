# The region engine

The FLAIR region engine: caller-backed scanline inversion lists, normal form,
Boolean set operations, constructors and queries. Pure C; dependencies: none
apart from the C implementation's standard headers. It allocates no memory.

Public headers are `include/region_algebra.h` (types, invariants, half-open
coordinates, capacity limits, Boolean operations, QuickDraw and GDI facades)
and `include/region.h` (working-set storage and initialization). Other
components use this include directory. The engine compiles separately with
only `-I. -Iinclude`; factory include paths apply only to test sources.

Run `make` to build the freestanding 32-bit libregion.a under `build/flair/region/`, `make test`
for tier 0, and `make mutants` for the mutation sweep. Both suites retain their
full legacy iteration counts (4000 pairs per Boolean property, 150 high-count
stress pairs, 3000 pairs per GDI leg); no reduced default is necessary. The
clean build and tests take about one second here. No emulator is involved.
The product smoke is run from the top level; this engine has no separate smoke.

The behavioral authority is the existing region algebra contract, carried
without changes from legacy `spec/region_algebra.h`: PRD section 6.2, Inside
Macintosh I's QuickDraw API and Win 3.1 wingdi.h's public modes and names.
The internal byte format remains our own. Boolean output collapses unchanged
and leading empty bands before consuming caller storage (initech-itce5), as
the normal-form capacity contract requires. Real vertical gap terminators and
genuine exhaustion checks are retained.

The primary oracle independently paints pixel sets, checks all four Boolean
operations plus framed complement, checks normal form, and checks identities.
The second oracle compiles the unchanged Wine server region implementation
in a separate, namespaced host translation unit and compares pixel sets,
complexity, bounding boxes, point membership and rectangle overlap. Its shim,
opaque interface and header stubs are private to `tests/`.
Wine region.c, request.h, user.h and COPYING.LIB are vendored unchanged in
`tests/wine/`; NOTICE records origin and licence. There is no sibling-corpus
dependency and no missing-oracle opt-out. `goldens/CORPORA.lock` pins the files.

## Legacy test inventory

| Class | Imported | Coverage / legacy entry |
|---|---|---|
| ORACLE-STRONG | 2 programs | `test-region`: 86 checks; `test-region-gdi`: 7 behavioral checks |
| UNIT | embedded | Truth tables, constructors, degenerate inputs, exact-capacity/guard probes and alias regressions remain in the 86-check suite; no separate unit program |
| META | 0 | Two L3 source-grep checks extracted to `attic/harness/proptest/test_region_meta.c`; tied to the retired `os/` tree |
| SIZE/LAYOUT | 0 | No region-specific legacy host gate in this class |

Legacy `test-region-gdi` reports 9 checks; the difference is exactly those two
META checks. Property generators, shrinker and all behavioral assertions are
retained. The alias probe also runs 24 child checks and the accumulation probe
one child check; their child-local counters are not included in either legacy
or new parent summaries. `--benchmark` is a timing mode, not a check gate.
The retired mutation recipes in `attic/harness/toolbox_hardening.mk` remain
unbuilt. Their two region mutants are carried as patches, together with the
six unique mutants from `test-region-mutant` and `test-region-gdi-mutant`.
Those four legacy targets plus `test-tbx-mutant-RGN_MUT_ALIAS_RESET` and
`test-tbx-mutant-RGN_MUT_REPEATED_UNION` run 10 mutation invocations covering
eight distinct faults. The new sweep adds redundant merge bands and runs nine, using both default suites
except the ASan scratch-overrun selection.

Client integration suites (blitter, text clipping/styles, window, interaction,
drag, process and desktop) remain in attic for their owning components; they
link other product code and are not standalone region-engine gates.
`attic/tools/ppm_region_count.c` counts image pixels, not engine regions.

## Patch mutants

Each `mutants/*.patch` has component-relative `a/` and `b/` paths; apply with
`patch -p1` from this directory. There are no mutation conditionals in product
source. Eight patches retain the legacy wrong-code branches; the ninth restores
premature capacity exhaustion. Exact-capacity union, intersection, difference
and XOR tests independently limit rows, x storage and both together; empty
results fit zero storage, while genuinely undersized results still abort.

`mk/mutants.mk` needs only TOP, COMPONENT, config.mk and host-test.mk. It
defaults MUTANT_PATCHES to `mutants/*.patch`; override that list to select a
subset. A `# test: name ...` line selects registered host test programs via
TESTS. `no-xmerge-cap.patch` selects the optional `region-asan` program;
`make test TESTS=region-asan` runs its unmodified baseline directly. ASan uses
non-PIE host executables to avoid randomized shadow-memory collisions.

The runner copies the component under `build/flair/region/mutants/run.*`,
using the original TOP for shared make fragments and test helpers. Each exact
test selection must pass unmodified, its patch must apply, and its mutated
programs must build before a runtime failure counts as a kill. A survivor,
patch error, baseline error or build error fails the sweep. Scratch sources
and logs are removed on success and retained on failure; `make clean` removes
them. Each kill must reproduce the same failing tests on a second run.
Baselines run once per distinct selection. The runner never edits working source.

The shared target, host-test, import-check and mutant fragments are used here.
`make test` also builds the real target library and checks build isolation,
product mutation hooks and escaping includes. The shared assertion header
provides the machine-readable positive count; no custom count pattern is needed.

Shared build conventions: command stamps rebuild on tool/flag changes; C
translation units use -MMD dependencies. TEST_BUDGET_S is 30 seconds for the
complete tier-0 selection; measured execution fits the default, excluding builds.
Standalone tests verify their consumed files in goldens/CORPORA.lock; the
top-level test run verifies the full lock once before parallel execution.
The target library contains target/region.o; target is an alias for all.

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: pure
Interface export-pure: flair/region/include/region.h region_engine_bind_ws
Interface export-pure: flair/region/include/region_algebra.h CombineRgn DiffRgn EmptyRgn EqualRgn GetRgnBox PtInRegion PtInRgn RectInRegion RectInRgn SectRgn SetEmptyRgn SetRectRgn UnionRgn XorRgn region_assert_normal region_complement region_contains_point region_engine_reset region_equal region_from_rects region_get_bbox region_intersects region_is_empty region_normalize region_op region_rect_fully_in region_rect_overlaps region_set_empty region_set_rect
<!-- end-interface-declarations -->
