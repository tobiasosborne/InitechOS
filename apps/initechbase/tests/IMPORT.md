# Import inventory

Reference: legacy-final, c5d18538049986bbbb27018c351405acf814ebbc.
The draft manifest was checked against the frozen Makefile, its included fragments,
the expanded UNIT/EMU vectors, actual dry-run compile commands, and executed gates.
The original names below identify historical targets; they are not product names.

## Suites and measured legacy comparison

| Suite | Class | Legacy / imported counted checks | Cases / notes |
|---|---|---:|---|
| dbf_coerce_fuzz | ORACLE-STRONG | 8079 / 8079 | full legacy source suite |
| test_canon_salami | ORACLE-STRONG | 3 / 3 | full legacy source suite |
| test_canon_y2k | ORACLE-STRONG | 4 / 4 | full legacy source suite |
| test_dbase_roundtrip | ORACLE-STRONG | 69 / 69 | full legacy source suite |
| test_dbf_fields | ORACLE-STRONG | 185 / 185 | full legacy source suite |
| test_dbf_header | ORACLE-STRONG | 61 / 61 | full legacy source suite |
| test_dbf_mutate | UNIT | 89 / 89 | full legacy source suite |
| test_dbf_read | ORACLE-STRONG | 122 / 122 | full legacy source suite |
| test_dbf_roundtrip | ORACLE-STRONG | 59 / 59 | full legacy source suite |
| test_dbt_read | ORACLE-STRONG | 30 / 30 | full legacy source suite |
| test_dbt_roundtrip | ORACLE-STRONG | 57 / 57 | full legacy source suite |
| test_interp_decimals | UNIT | 23 / 23 | full legacy source suite |
| test_interp_flow | UNIT | 79 / 79 | full legacy source suite |
| test_interp_list | ORACLE-STRONG | 108 / 108 | full legacy source suite |
| test_interp_nav | ORACLE-STRONG | 261 / 261 | full legacy source suite |
| test_interp_proc | UNIT | 44 / 44 | full legacy source suite |
| test_interp_replace | UNIT | 67 / 67 | full legacy source suite |
| test_interp_set | ORACLE-STRONG | 65 / 65 | full legacy source suite |
| test_interp_setfmt | UNIT | 48 / 48 | full legacy source suite |
| test_interp_use | ORACLE-STRONG | 73 / 73 | full legacy source suite |
| test_ndx_build | ORACLE-STRONG | 33 / 33 | full legacy source suite |
| test_ndx_incremental | ORACLE-STRONG | 1731 / 1731 | full legacy source suite |
| test_ndx_keys | ORACLE-STRONG | 63 / 63 | full legacy source suite |
| test_ndx_maintain | ORACLE-STRONG | 64 / 64 | full legacy source suite |
| test_ndx_multilevel | UNIT | 22 / 22 | full legacy source suite |
| test_ndx_pack | UNIT | 35 / 35 | full legacy source suite |
| test_ndx_parse | ORACLE-STRONG | 139 / 139 | full legacy source suite |
| test_ndx_seek | ORACLE-STRONG | 93 / 93 | full legacy source suite |
| test_ibase_use | ORACLE-STRONG | 213 / 213 | full legacy source suite |
| test_ibase_core | ORACLE-STRONG | 286 / 297 | 11 additional short-read assertions |
| test_ibase_ctl | ORACLE-STRONG | 82 / 82 | full legacy source suite |
| test_ibase_deferred_errors | UNIT | 100 / 100 | full legacy source suite |
| test_ibase_input | ORACLE-STRONG | 104 / 104 | full legacy source suite |
| test_ibase_memocells | UNIT | 5 / 5 | full legacy source suite |
| test_ibase_messages | ORACLE-STRONG | 0 / 0 | 151 real message ordinals |
| test_ibase_ndx_names | UNIT | 59 / 59 | full legacy source suite |
| test_ibase_ovl_lifetime | UNIT | 636 / 636 | full legacy source suite |
| test_ibase_pal_contract | UNIT | 34 / 34 | full legacy source suite |
| test_ibase_pal_host | UNIT | 28 / 28 | full legacy source suite |
| test_ibase_query | UNIT | 49 / 49 | full legacy source suite |
| test_ibase_repl | UNIT | 59 / 59 | full legacy source suite |
| test_ibase_report | ORACLE-STRONG | 1138 / 1138 | full legacy source suite |
| test_ibase_rt | ORACLE-STRONG | 93 / 93 | full legacy source suite |
| test_ibase_say | UNIT | 70 / 70 | full legacy source suite |
| test_ibase_set_color | UNIT | 141 / 141 | full legacy source suite |
| test_ibase_softfp | ORACLE-STRONG | 23858 / 23858 | full legacy source suite |
| test_ibase_spec | UNIT | 111 / 111 | full legacy source suite |
| test_ibase_status | UNIT | 1809 / 1809 | full legacy source suite |
| test_ibase_value | UNIT | 59 / 59 | full legacy source suite |
| test_use_rw | ORACLE-STRONG | 59 / 59 | full legacy source suite |
| test_w2_host | UNIT | 48 / 49 | 1 additional read assertion |
| test_xbase_eval | UNIT | 123 / 123 | full legacy source suite |
| test_xbase_fn_a | UNIT | 93 / 93 | full legacy source suite |
| test_xbase_fn_b | UNIT | 82 / 82 | full legacy source suite |
| test_xbase_fn_c | UNIT | 69 / 69 | full legacy source suite |
| test_xbase_fn_d | ORACLE-STRONG | 59 / 59 | full legacy source suite |
| test_xbase_lex | UNIT | 167 / 167 | full legacy source suite |
| test_xbase_parse | UNIT | 158 / 158 | full legacy source suite |
| test_xbase_transform | UNIT | 48 / 48 | full legacy source suite |
| prog_diff | ORACLE-STRONG | 8 / 8 | full legacy source suite |
| test_ibase_view | ORACLE-STRONG | 159 / 159 | full legacy source suite |
| test_ibase_dml | ORACLE-STRONG | 136 / 161 | 25 invocations, including 3 state and abbreviation control; 25 new getcwd assertions |
| test_ibase_fns | ORACLE-STRONG | 502 / 502 | 49 cases (46 original + 3 unregistered passing cases) |
| test_ibase_w2 | ORACLE-STRONG | 68 / 68 | 13 SET/verb cases (9 original + 4 unregistered passing cases) |
| test_ibase_dbsafe | ORACLE-STRONG | 1475 / 1475 | full legacy source suite |
| test_ibase_datatype | UNIT | 286 / 286 | 9 named cases |
| test_interp_do_file | ORACLE-STRONG | 0 / 0 | 9 real disk-program transcripts |
| ibase_numeric_format | ORACLE-STRONG | 0 / 0 | 8957 independent descriptor cells |
| test_ibase_ovl | UNIT | 50 / 24 | 24 pure checks plus 50 built-pair checks in package-check |
| readers | ORACLE-STRONG | 263 / 263 | independent DBF/DBT and NDX selftests |
| captured-hashes | ORACLE-STRONG | support / 0 | external mint, owned program/input/output manifests |
| w4-files | ORACLE-STRONG | support / 0 | 9 format kinds, meaningful/padding/truncation controls |
| retained-screens | ORACLE-STRONG | 0 / 0 | 4 captures, 5 dumps; see legacy failure below |
| mzlink-io | UNIT | support / 0 | relocation stream short/read/close failures |
| messages-generated | ORACLE-STRONG | support / 0 | deterministic catalogue regeneration |

Default: 75 suites: 43 ORACLE-STRONG, 32 UNIT.
THOROUGH=1 adds core-live (104 real-dBASE postchecks plus the core producer).
The table counts TEST_SUMMARY checks, including all matrix invocations; descriptor
cells, ordinal comparisons, hash entries, file/screen comparisons and shrinker
iterations are separately described, not inflated into this assertion count.

All standalone and matrix binaries were actually run in the legacy tree; the
registered accepted gate vector was also run with make -j32 in 12.55 seconds.
Unregistered foundation/CTL gates were additionally built and executed.
The retained-screen legacy gate goes RED before comparisons: its pinned historical
screen_tool producer binary hash differs from the locally rebuilt executable.
That binary-identity check is retired-factory META. The imported runner retains
reference data/input/event/real-dBASE hashes, literal screen assertions and all
cell/attribute/cursor negative controls; only producer binary/tool-version hashes
are omitted. No captured expected screen bytes were changed.

### Captures and corpus

The measured default run opened 769 distinct files under DBASE3_DECOMP for reading,
including corpus manifests and reference Python inputs (temporary LD_PRELOAD file
open audit; not an estimate from the implementation). Fixture staging also opens
owned captures not all graded by a given case. Real input/output data were moved
as complete capture directories, retaining logs and source/hash provenance.
Two numlist files were copied from docs/research/mints/numlist-v1-2026-10-23;
their pinned hashes are unchanged. The entire real capture payload is preserved
across the separate identifier/file rename.

## Current target and migration limits

The plain-flags target now packages INITBASE.EXE/INITBASE.OVL, validated by the
independent factory/mz/mzcheck and all 50 pair checks in make test. The former
80-byte reserve overrun is resolved by the app-owned extended-memory contract;
README.md records the layout, headroom, enforcement points and frozen inventory.
Soft-float, section isolation and runtime arena/profile flags remain; no LTO,
register/callee-pop ABI or size-squeezing compiler options were restored.

The default host vector remains 75 suites. The full pair check additionally
runs 50 assertions rather than silently treating --pure as pair qualification.
Real-capture expectations and file-format readers are unchanged. Twenty-five
previously failing captures remain explicit make pending cases; the seven
previously unregistered passing cases are included in the normal family lists.
The retained-screen producer-identity check is historical factory metadata,
not an assertion that the target rendered those screens in the new image.

The product session runs interactive USE/LIST/REPLACE/APPEND/DELETE, a captured
INDEX/SEEK disk program, and the canon Y2K program, then QUIT on QEMU and Bochs.
Independent mtools extraction and the existing DBF/NDX readers grade disk
results. A dropped-REPLACE-write patch was observed red at disk readback, with
a normal return and no CPU fault; restoration recovered both product hashes.
The additional legacy emulator journeys remain migration work (SMOKES.md).

Mutation activation macros are removed from product sources. Each concrete
fault is a zero-fuzz patch; shared factory grading requires a clean baseline,
successful compilation and the same failing suite twice. Pending patches record
their missing emulator/target oracle individually, without claiming a kill.

Legacy leftovers remain owned by InitechBase: emulator journey fixtures and
size/link-layout scenarios in attic/harness/diff/dbf_diff and their manifest
records. The generic emulator drivers and MZ tools belong to factory; obsolete
inventory/certificate/merge machinery is not being imported. No build reads
attic. The orchestrator owns the corresponding STATUS.md update.
