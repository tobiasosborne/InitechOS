# ATKINSON

The FLAIR region engine: caller-backed scanline inversion lists, normal form,
Boolean set operations, constructors and queries. Pure C; dependencies: none
apart from the C implementation's standard headers. It allocates no memory.

Public headers are `include/region_algebra.h` (types, invariants, half-open
coordinates, capacity limits, Boolean operations, QuickDraw and GDI facades)
and `include/region.h` (working-set storage and initialization). Other
components use this include directory. The engine compiles separately with
only `-I. -Iinclude`; factory include paths apply only to test sources.

Run `make` to build host artifacts under `build/flair/atkinson/`, `make test`
for tier 0, and `make mutants` for the mutation sweep. Both suites retain their
full legacy iteration counts (4000 pairs per Boolean property, 150 high-count
stress pairs, 3000 pairs per GDI leg); no reduced default is necessary. The
clean build and tests take about one second here. No emulator is involved.
The reboot has no bootable image or smoke target yet.

The behavioral authority is the existing region algebra contract, carried
without changes from legacy `spec/region_algebra.h`: PRD section 6.2, Inside
Macintosh I's QuickDraw API and Win 3.1 wingdi.h's public modes and names.
The internal byte format remains our own. No behavior is changed by this
import; hosted and freestanding preprocessed engine output matches legacy.

The primary oracle independently paints pixel sets, checks all four Boolean
operations plus framed complement, checks normal form, and checks identities.
The second oracle compiles the unchanged Wine server region implementation
in a separate, namespaced host translation unit and compares pixel sets,
complexity, bounding boxes, point membership and rectangle overlap. Its shim,
opaque interface and header stubs are private to `tests/`.
`WIN31_DECOMP` defaults through `mk/config.mk` to `~/Projects/win31-decomp`.
Wine source must be present for acceptance; a missing reference fails loudly
unless the legacy explicit `SKIP_WIN31_DECOMP=1` opt-out is set.

## Legacy test inventory

| Class | Imported | Coverage / legacy entry |
|---|---|---|
| ORACLE-STRONG | 2 programs | `test-region`: 67 checks; `test-region-gdi`: 7 behavioral checks |
| UNIT | embedded | Truth tables, constructors, degenerate inputs, capacity/guard probes and alias regressions remain in the 67-check suite; no separate unit program |
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
eight distinct faults. The new sweep runs all eight, using both default suites
except the ASan scratch-overrun selection.

Client integration suites (blitter, text clipping/styles, window, interaction,
drag, process and desktop) remain in attic for their owning components; they
link other product code and are not standalone region-engine gates.
`attic/tools/ppm_region_count.c` counts image pixels, not ATKINSON regions.

## Patch mutants

Each `mutants/*.patch` has component-relative `a/` and `b/` paths; apply with
`patch -p1` from this directory. There are no mutation conditionals in product
source. The eight patches retain the exact legacy wrong-code branches.

`mk/mutants.mk` needs only TOP, COMPONENT, config.mk and host-test.mk. It
defaults MUTANT_PATCHES to `mutants/*.patch`; override that list to select a
subset. A `# test: name ...` line selects registered host test programs via
TESTS. `no-xmerge-cap.patch` selects the optional `region-asan` program;
`make test TESTS=region-asan` runs its unmodified baseline directly. ASan uses
non-PIE host executables to avoid randomized shadow-memory collisions.

The runner copies the component under `build/flair/atkinson/mutants/run.*`,
using the original TOP for shared make fragments and test helpers. Each exact
test selection must pass unmodified, its patch must apply, and its mutated
programs must build before a runtime failure counts as a kill. A survivor,
patch error, baseline error or build error fails the sweep. Scratch sources
and baseline/build/test logs remain under build for inspection; `make clean`
removes them. The runner never edits the working source.
