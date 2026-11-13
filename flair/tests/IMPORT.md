# Toolbox import and verification

Current Toolbox test provenance. Header ownership, target linking and platform
binding are described in ../README.md and ../NOTES-FOR-KERNEL.md.

Baseline: legacy-final, c5d18538049986bbbb27018c351405acf814ebbc.
All listed legacy targets were run in /home/tobiasosborne/Projects/InitechOS-legacy
with `. tools/lane-env.sh`, followed by `make -j16 <gate>`. Counts are observed
outputs, not the draft inventory's inferred counts. No assertion count was reduced.

52 invocations from 51 source programs: 25 ORACLE-STRONG, 27 UNIT/REGRESSION.
451,944 baseline checks; 451,957 after restoring missing host coverage, plus separately counted storage/table
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
| cursor | ORACLE-STRONG | 512 | 515 |
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
| menu | ORACLE-STRONG | 83 | 92 |
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
| window | ORACLE-STRONG | 68 | 69 |
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

## Restored host coverage and remaining ownership

The legacy host gates for Apple-slot hit testing, clock formatting/refresh,
busy-cursor selection and overlay occlusion were not represented by their
manager suites. Their independent assertions now run in test_menu, test_cursor
and test_window, retaining all original checks and adding thirteen. Chicago's
advance patch changes both the header and its generator because generated
headers precede checked-in asset headers; the generator no longer emits hooks.
The default-ring patch selects test_dialog; the wrong-era patch selects
test_chrome_fidelity, matching their legacy host gates.

MUTATION-RESULTS.tsv is the complete conversion disposition and recipe index.
The sixteen pending emulator checks remain with the factory/desktop owners;
SMOKES.md and SMOKES.tsv retain their broader legacy scenario inventory. Kernel
owns the host-DOS binding debt and its mouse-producer/boot-renderer probes.
Factory owns the retired checker/source-policy probes; desktop owns parked
Finder/film/icon/clock integration tests. None is built from attic/ here.

The shared import audit still scans across the nested component boundaries.
README.md states the exact scoping change required from its owner. No imported
status is claimed until that audit can grade the Toolbox's product sources.
