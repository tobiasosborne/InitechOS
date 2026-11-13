# Toolbox import and verification

Baseline: legacy-final, c5d18538049986bbbb27018c351405acf814ebbc.
All listed legacy targets were run in /home/tobiasosborne/Projects/InitechOS-legacy
with `. tools/lane-env.sh`, followed by `make -j16 <gate>`. Counts are observed
outputs, not the draft inventory's inferred counts. No assertion count was reduced.

52 invocations from 51 source programs: 25 ORACLE-STRONG, 27 UNIT/REGRESSION.
451,944 numbered checks in each build, plus separately counted storage/table
comparisons below. The 438,499-check TextEdit property model dominates the total.

| Program (test_ prefix) | Class | Legacy | Imported |
|---|---|---:|---:|
| asset_lz | UNIT/REGRESSION | 4 blocks / 6320 bytes | 4 blocks / 6320 bytes |
| blitter | ORACLE-STRONG | 18 | 18 |
| boot_logo | ORACLE-STRONG | 7 | 7 |
| canon | UNIT/REGRESSION | 67 | 67 |
| chicago_metrics | ORACLE-STRONG | 934 | 934 |
| chicago_pack | UNIT/REGRESSION | 1568 rows | 1568 rows |
| chrome | UNIT/REGRESSION | 55 | 55 |
| chrome_fidelity | ORACLE-STRONG | 35 | 35 |
| clut | ORACLE-STRONG | 1047 | 1047 |
| color_canon | ORACLE-STRONG | 39 | 39 |
| control | ORACLE-STRONG | 244 | 244 |
| control_record | UNIT/REGRESSION | 77 | 77 |
| cursor | ORACLE-STRONG | 512 | 512 |
| dialog | ORACLE-STRONG | 81 | 81 |
| dialog_record | UNIT/REGRESSION | 87 | 87 |
| dialog_tracking | UNIT/REGRESSION | 6 | 6 |
| drawing_ops | UNIT/REGRESSION | 128 | 128 |
| event | ORACLE-STRONG | 328 | 328 |
| flair_dac | UNIT/REGRESSION | 256 entries + shape | 256 entries + shape |
| flair_headers | UNIT/REGRESSION | 14 | 14 |
| flair_heap | ORACLE-STRONG | 43 | 43 |
| flair_mechanism_colorblind | UNIT/REGRESSION | 2 | 2 |
| flair_part_map | UNIT/REGRESSION | 405 transcript rows | 405 transcript rows |
| flair_routing | UNIT/REGRESSION | 138 | 138 |
| font_styles | ORACLE-STRONG | 4304 | 4304 |
| geneva12_metrics | ORACLE-STRONG | 503 | 503 |
| geneva12_pack | UNIT/REGRESSION | 1536 rows | 1536 rows |
| interact | ORACLE-STRONG | 21 | 21 |
| list | UNIT/REGRESSION | 57 | 57 |
| menu | ORACLE-STRONG | 83 | 83 |
| menu_look | ORACLE-STRONG | 21 | 21 |
| menu_record | UNIT/REGRESSION | 101 | 101 |
| process | UNIT/REGRESSION | 58 | 58 |
| process_activate | ORACLE-STRONG | 21 | 21 |
| process_budget | UNIT/REGRESSION | 19 | 19 |
| process_teardown | ORACLE-STRONG | 54 | 54 |
| process_update | UNIT/REGRESSION | 18 | 18 |
| resource | UNIT/REGRESSION | 41 | 41 |
| scrap | UNIT/REGRESSION | 57 | 57 |
| scrollbar_look | ORACLE-STRONG | 10 | 10 |
| skin_era_frozen | UNIT/REGRESSION | 5 | 5 |
| skin_teal | UNIT/REGRESSION | 13 | 13 |
| stdfile | UNIT/REGRESSION | 32 | 32 |
| tbx_file | UNIT/REGRESSION | 611 | 611 |
| tbx_menu | UNIT/REGRESSION | 23 | 23 |
| te_surface | ORACLE-STRONG | 438499 | 438499 |
| text | ORACLE-STRONG | 30 | 30 |
| text_clip | ORACLE-STRONG | 3002 | 3002 |
| textedit | UNIT/REGRESSION | 68 | 68 |
| window | ORACLE-STRONG | 68 | 68 |
| winscroll | ORACLE-STRONG | 34 | 34 |
| event_record | ORACLE-STRONG | 329 | 329 |

The legacy gate names are test-<program name with underscores changed to hyphens>.
Exception: event_record is the second test-event invocation, compiled with
FINDER_DBLCLICK_TICKS=3000u (329 checks); normal event has 328. Both are retained.
The generated-header equality gates test-chicago-art, test-geneva12-art,
test-color-canon-gen run inside the pack/color program wrappers. Asset LZ retains
its committed-header synchronization check and decoded-byte check. The DAC
retains the 256-entry comparison; it is a consistency check, never a value oracle.
The frame-dependent test-assets tool is claimed in tools/ but not run without its
local copyrighted preview.webp; it is not silently counted as a passing test.

## Classes and provenance

ORACLE-STRONG means that the program has independent reference or property legs;
unit assertions remain in the same programs. Properties use separate pixel grids,
set/owner models, text-layout models or allocator invariants. They prove the
specified algorithm, not that every authored pixel matches period software.
Boot-logo geometry is authored product art checked by a separately implemented
per-pixel rasterizer, not a capture of a historical operating system.

Five checked-in reference-derived golden headers: chicago12_nfnt_golden.h,
chrome_fidelity_golden.h, menu_look_golden.h, scrollbar_look_golden.h (tests/goldens),
and font_styles_golden.h. Their opening comments identify the exact resource,
capture coordinates, decomp specification and/or mint provenance. Runtime
reference inputs are enumerated and SHA-256 pinned in REFERENCE-INPUTS.sha256:
10 PNG captures, 6 binary resources (including the minted Geneva 12 NFNT),
4 reference texts (3 Mac specifications and the Windows color cross-check).
These are 20 external files, not 20 additional test programs. The screenshot
transcriptions also cite historical captures not reread at runtime.

Two checked-in self-derived regression fixtures: fixtures/flair_part_map.golden
(405 lines, captured from our pre-compaction renderer, commit 4d907b1f) and
goldens/asset_lz_data.h (packed from our authored strikes). They are NOT fidelity
oracles. The pack suites compare all raw strike rows against our authored font
inputs; the art/header equality checks prove transcription consistency. No one
may count those as independent visual evidence. The raw asset headers are
product inputs, not additional golden captures. tests/platform/dos_messages.h
is a frozen host integration input from the legacy generated diagnostic catalog,
not an oracle; it is never linked into the product archive.

The independent color suite (39 graded rows, zero missing rows) reads real wctb,
Windows reference values and Mac specification measurements. The DAC consistency
suite and skin cross-checks are classified UNIT. This preserves the distinction
that the legacy palette-grading heresy erased: matching two consumers of the
same color table does not validate the colors. Authored teal/bevel substitutions
and canon mismatches are retained as stated authored policy, not historical color
fidelity. No expected value is newly generated from an artifact framebuffer.

## Moves and excluded tests

All original source, public contracts, assets, render helpers and test files were
claimed with git mv, in a move-only commit before the include-path changes.
Public headers are in include/ and include/assets/. Manager source remains at
flair/; internal gate adapters stay private. region/ is a separate dependency.
Source assertions are unchanged except the TextEdit probe's explicit memcpy bound
(fixes an optimized-host compiler diagnostic, with the same seeded cases), and
the gate suite's platform API adaptation. Routing retains all 138 checks.
The file suite retains all 611 checks, including real DOS caller PSP/CWD restoration,
SFT exhaustion, device precedence, cleanup and independent saved bytes.

Desktop tests parked, unbuilt, in desktop/tests/: all test_finder_*, test_drag,
test_menu_handlers, test_apple_menu, test_desk_icons, test_film_scene,
test_egg_jtc, test_power_notice, test_shell and test_menubar_clock.
The icon ART suite needs the desktop finder_icon renderer; it is parked despite
its strong reference pixels. test_finder_icon_oracles is META (checker-state
routing), not a product fidelity test. The COLD halves are not built or rejoined.

Left in attic (not built):

- test_mech_policy.c: META/source scanner of retired os/ paths and a historical
  mechanism/policy allowlist. test_flair_mechanism_colorblind.c is different:
  an executable sentinel-resolver test; it IS imported.
- test_palette_seafoam.c: retired UNIT/self-equality checking the superseded
  palette/boot fill; no fidelity claim. test_color_canon replaces its value role.
- test_fbagree.c: kernel console-to-surface integration, owned by kernel.
- test_mouse_producer.c: real mouse IRQ producer into the event ring, owned by
  kernel; the complete Event Manager suite is imported here.
- test_ssim.c and check_win95isms.c: META/factory checker and source-policy tests.
- test_kmain_cold.c, finder_overlay.mk, kernel_size*.mk, overlay.mk,
  power_overlay.mk, kernel_cold_*.c, kernel_gc_*.c, ovl_reader.c and
  kernel transfer/ABI scripts: retired SIZE/LAYOUT or META. Functional storage
  tests were extracted from those make fragments; they were not discarded.
- test_boot_screen.c: kernel boot renderer integration, not a Toolbox manager.
- kernel/test_flair_heap_ram.c stays with kernel: RAM profile/budget integration.
- region and region-gdi: already owned and tested by region/ (not double-counted).
- Shared hardening, serial/capture and emulator checkers and trace fragments:
  remain in attic, listed in SMOKES.md and its complete TSV index.

LEGACY-HOST-GATES.tsv names every host-attributed gate in the draft manifest,
including mutants and retired factory gates, with disposition. Its rule names
were checked against the actual legacy Makefile/make fragments. Program rows
above, not alias/mutant gate names, are the check-count denominator.

## Build and boundary decisions

No mk/*.mk edits. config.mk supplies plain target flags. Target: 19 C modules,
19 ELF32 i386 objects, deterministic libflair.a; region code is linked separately.
The host fragment supplies compilation and status/summary. Local extensions:
per-program scratch/capture wrapper, corpus preflight, header generation,
whitelisted borrowed header views and one real-DOS host integration view.
For default all, TESTS is empty to suppress host-test.mk's all prerequisites;
make test still runs the entire suite. THOROUGH=1 does not reduce or replace any
checks: the full legacy sweeps already fit comfortably in the default run.

Dropped legacy size tuning: -Oz, -flto, per-file -Os overrides,
-ffunction-sections, -fdata-sections, -fomit-frame-pointer, -fno-inline,
-fno-jump-tables, -mregparm=3, -mrtd, -freg-struct-return,
-mpreferred-stack-boundary=2, -mincoming-stack-boundary=2, -mtune=generic,
-malign-data=abi, and all legacy --param/per-optimization-pass -fno-* lists.
Only TARGET_CFLAGS from config.mk are used; no LTO link or overlay packing.
There were ZERO COLD section annotations or kernel overlay imports/stubs in the
Toolbox modules themselves. They lived in kernel/ and the now-parked desktop.
Removed the asset decoder's size-driven noinline attribute. Retained packed
storage and its tested decoders; window always-on-top overlay is GUI geometry,
not executable overlay machinery. The explicit cdecl INT 81h annotation remains
because it describes the trap frame's calling convention, not size tuning.

The target includes only Toolbox, region public, SDK and the five named borrowed
SDK contracts. The host-only DOS adapter has a deliberately visible kernel debt:
it includes int21.h/sft.h and builds the existing kernel code against four
borrowed kernel contracts. See README and NOTES-FOR-KERNEL.md. No kernel source,
SDK source or app source is edited. The generated DOS view only rewrites two
obsolete ../../spec includes; it does not replace DOS behavior with a fake.

Import exposed the earlier spreadsheet's hardcoded attic/spec/grafport.h path.
The top-level loops pass ATTIC_SPEC pointing at the Toolbox's compatibility view;
that view provides the moved public GrafPort contract and the five borrowed SDK
contracts. No app is edited. Remove this bridge with the SDK event-header import.

## Acceptance evidence

See ACCEPTANCE.txt for measured clean build times, full tier-0 results, the three
red controls, missing-corpus failure, target members and include audit. All
negative controls use separate output directories; no wrong product source or
mutated archive is committed. make smoke deliberately fails with SMOKES.md:
this checkout has no booted product image to drive yet. This is an import, not
an emulator certification or a claim of complete reference fidelity.
