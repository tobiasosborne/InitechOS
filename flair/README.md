# FLAIR Toolbox

The reusable graphics and application managers. The Finder and desktop shell
are built independently in desktop/ and linked by system/. No application or desktop policy is linked
into this library. Dependencies: flair/region (region/include/), sdk/include
(interface records), and, for the host-only real-DOS tests, kernel/include.
There is no target dependency on kernel headers.

Managers:

- Surface: bounded pixel writes and depth packing.
- Blitter: region-clipped fills and copies.
- Text/fonts: Chicago and Geneva strikes, metrics, styled/clipped text runs.
- TextEdit: editable text, layout, selection, caret and scrap operations.
- Chrome: Platinum window and widget drawing.
- Window: geometry, z-order, visible regions, exposure and update damage.
- Menu: proportional bars, pull-down layout, tracking and keyboard equivalents.
- Control: buttons, checkboxes, radios, scroll bars and progress bars.
- Dialog: item routing, modal handling and standard alert/file-copy furniture.
- Event: deterministic raw ring, cooked events, mouse tracking and ticks.
- List: cell layout and selection.
- Scrap: typed desktop scrap storage.
- Resource: resource-map reader.
- Heap: caller-backed, classed arena allocation and reclamation.
- Cursor: save-under, masking and shielded cursor drawing.
- Look: part/skin/palette resolution, including authored canon colors.
- Process: cooperative app lifecycle, activation, dispatch and teardown.
- Standard File: directory list and get/put file selection model.
- Toolbox gate: validated INT 81h verbs and resident disk-tenant lifecycle.

Public headers are include/*.h, with shared font/icon/color inputs in
include/assets/. The matching manager header is its API; window_record.h,
control_record.h, dialog_record.h, menu_record.h, imaging.h, grafport.h and
drawing_ops.h carry the shared types and constants. flair_platform.h is the
host binding, tbxgate.h the OS-side pump interface. tbxfile.h, tbxmenu.h,
cp_service.h, cp_dialog_items.h, tbx_layout.h, rdoc.h and ring_report.h remain
private implementation helpers.
The asset decoders are public because asset headers use them. Clients add the
public include directories and build/flair/generated for packed storage.

make builds 20 freestanding ELF32 i386 objects and build/flair/libflair.a with
plain TARGET_CC/TARGET_CFLAGS from mk/config.mk. It also invokes region/
make target to build build/flair/region/target/region.o; link that object
alongside libflair.a. The archive does not duplicate the region implementation. Generated fonts,
palette/canon, DAC and packed assets come from component-local inputs using
host C tools under build/flair/tools. No LTO, per-file size flags or executable
overlay packing remains. make test runs all host programs with one result line
each, logs under build/flair/host and isolated scratch under build/flair/runs.
Use -j32 for parallel compilation; the serial clean run is under a minute.
All seeded legacy sweeps run by default; THOROUGH=1 does not shorten any suite.
make test TESTS='test_window test_menu' selects registered entries for debugging.

The host tests use independent pixel/owner grids, text/layout models and
reference-derived Mac resources and screen crops. SYSTEM7_DECOMP and
WIN31_DECOMP default through mk/config.mk. Missing, changed or malformed corpus
inputs fail loudly. Twenty runtime reference inputs are SHA-256 pinned in
tests/REFERENCE-INPUTS.sha256; the five captured/transcribed golden headers cite
their sources. tests/IMPORT.md gives every program, counts and the honest
reference/self-derived split. In particular, DAC/skin consistency, compressed
storage and the pre-compaction part-map transcript are UNIT/REGRESSION, not
proof of historical fidelity. Authored logo art and teal substitutions remain
authored policy. No pixel or user-visible behavior is intentionally changed.

The platform binding supplies private file sessions, DOS-form file/clock calls,
DTA exchange, memory accounting, tenant loading and transfer, desktop pattern,
speaker, serial diagnostics and panic. Clock ticks and the master arena already
have explicit Event Manager / tbx_bind inputs. Console-to-GUI hand-off belongs
to the desktop; the Toolbox does not seize the console. NOTES-FOR-KERNEL.md maps
every binding to the legacy routine and states ordering/lifetime requirements.
Routing uses real tbxgate/process/window/event with fail-loud platform stubs for
services outside its scope; file integration uses tests/platform_dos.c over the
real existing InitechDOS dispatcher. No fake success replaces those DOS checks.

The SDK owns all app-facing gate, event, geometry and Control Panels records.
GrafPort includes geometry.h rather than declaring Point again. The former
cp_dialog_items adapter is private to FLAIR (unchanged from legacy-final's
spec/cp_dialog.h). tbx_layout.h pins registration and event wire offsets and
compares every meaningful menu-manager field with the SDK records in i386
builds. Host routing uses fixed i386 wire offsets, never native host pointers.
The DOS find buffer is opaque storage sized from the SDK's DTA payload offset
and length; only the platform binding interprets its DOS resume state.

Host-only integration debt: tests/platform_dos.c and test_tbx_file.c use
only kernel public headers, and the
file suite compiles six unchanged kernel translation units directly. There
are no borrowed headers, source views or app GrafPort compatibility paths.
The target never sees kernel include paths. Move the host adapter into the
kernel's host binding when that binding is published. The diagnostic catalog
in tests/platform is a frozen legacy fixture, not product code or an oracle.

The target platform binding and INT 81h assembly are integrated in desktop/.
Disabled and pressed CDEF capture gaps remain explicit. tests/SMOKES.md indexes
250 legacy emulator gates and their traces/checkers; those individual checks
remain with the factory and desktop owners. The product smoke exercises the
current booted pump, rather than certifying every historical scenario.

make mutants runs 192 patches through the shared strict runner: passing baseline,
zero-fuzz application, successful compilation, and the same runtime failure twice.
Each patch selects a killing host program. tools/run-mutants.sh groups exact
selections and runs up to MUTANT_JOBS=16 groups concurrently with private scratch;
the measured sweep is 85 seconds on this machine. The shared runner itself is
serial; native parallel scheduling and baseline sharing across patch workers
would remove this local wrapper and the serial tail of large selection groups.
Sixteen patches under mutants/pending/ need their named legacy emulator gates.
None was deleted as equivalent. tests/MUTATION-RESULTS.tsv records all 207 macros,
their original 248 guard entries, test selection, disposition and recipe evidence.
This includes five hooks omitted from the original 202-macro/243-entry inventory:
the two clock faults, modal passthrough, close hiding and update visibility.
Shared scrollbar patches are counted once under control; surface/asset decoding
under blitter, look resolution under chrome, font storage under text, and the
menu gate helpers under menu. No product behavior changes.

Shared build conventions: command stamps rebuild on tool/flag changes; C
translation units use -MMD dependencies. TEST_BUDGET_S is 30 seconds for the
complete tier-0 selection; measured execution fits the default, excluding builds.
Standalone tests verify their consumed files in goldens/CORPORA.lock; the
top-level test run verifies the full lock once before parallel execution.
The graph-based import audit now checks all-reachable sources and headers.
The remaining DAC generator omission probe is an external patch, killed by
test_flair_dac (index 255 differs from the independently locked white entry).
make test builds and audits libflair.a before its consumers run. libflair.a uses the
shared target rules; region-target asks region for its single library/object build.
