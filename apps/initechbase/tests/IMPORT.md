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
| test_ibase_ovl | UNIT | 50 / 24 | 24 pure checks imported; 26 built-pair checks await package |
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

## Legacy host gate disposition

Every component host target, including aliases, pending cases and mutation gates,
is enumerated below. Template instances are expanded. Emulator targets instead
appear in SMOKES.md. Mutation rows mean conversion is deferred, not passed here.
SIZE/LAYOUT includes resident/overlay packing and output-rebuild tests, which
cannot run against the objects-only import. Hardware JSON and scratch-lock gates
are META for this component (hardware owner / factory locking); they remain attic.

| Legacy gate | Class / disposition |
|---|---|
| `samir-ctl-goldens` | ORACLE-STRONG provenance; captured-hashes (size-only rows excluded) |
| `samir-dml-goldens` | ORACLE-STRONG provenance; captured-hashes (size-only rows excluded) |
| `samir-fns-goldens` | ORACLE-STRONG provenance; captured-hashes (size-only rows excluded) |
| `samir-list-off-goldens` | ORACLE-STRONG provenance; captured-hashes (size-only rows excluded) |
| `samir-prog-goldens` | ORACLE-STRONG provenance; captured-hashes (size-only rows excluded) |
| `samir-report-goldens` | ORACLE-STRONG provenance; captured-hashes (size-only rows excluded) |
| `samir-size-goldens` | SIZE/LAYOUT; left in attic; packaging blocked |
| `samir-use-goldens` | ORACLE-STRONG provenance; captured-hashes (size-only rows excluded) |
| `samir-w2-goldens` | ORACLE-STRONG provenance; captured-hashes (size-only rows excluded) |
| `samir-w2-unique-goldens` | ORACLE-STRONG provenance; captured-hashes (size-only rows excluded) |
| `test-canon-salami` | ORACLE-STRONG; test_canon_salami |
| `test-canon-salami-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-canon-y2k` | ORACLE-STRONG; test_canon_y2k |
| `test-canon-y2k-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-dbase` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-dbase-corpus-pristine` | META; corpus immutability tooling left in attic |
| `test-dbase-corpus-pristine-mutant` | META; corpus immutability tooling left in attic |
| `test-dbase-diff` | ORACLE-STRONG; prog_diff |
| `test-dbase-diff-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-dbase-diff-mutants` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-dbase-roundtrip` | ORACLE-STRONG; test_dbase_roundtrip |
| `test-dbase-roundtrip-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-dbf-fields` | ORACLE-STRONG; test_dbf_fields |
| `test-dbf-fields-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-dbf-header` | ORACLE-STRONG; test_dbf_header |
| `test-dbf-header-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-dbf-mutate` | UNIT; test_dbf_mutate |
| `test-dbf-mutate-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-dbf-read` | ORACLE-STRONG; test_dbf_read |
| `test-dbf-read-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-dbf-read-openrw-hdrlen-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-dbf-roundtrip` | ORACLE-STRONG; test_dbf_roundtrip |
| `test-dbf-roundtrip-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-dbt-read` | ORACLE-STRONG; test_dbt_read |
| `test-dbt-read-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-dbt-roundtrip` | ORACLE-STRONG; test_dbt_roundtrip |
| `test-dbt-roundtrip-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-flair-samir-high-return` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-flair-samir-suspend` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-flair-samir-suspend-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-interp-decimals` | UNIT; test_interp_decimals |
| `test-interp-decimals-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-interp-do-file` | ORACLE-STRONG; test_interp_do_file |
| `test-interp-do-file-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-interp-flow` | UNIT; test_interp_flow |
| `test-interp-flow-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-interp-list` | ORACLE-STRONG; test_interp_list |
| `test-interp-list-heading-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-interp-list-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-interp-nav` | ORACLE-STRONG; test_interp_nav |
| `test-interp-nav-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-interp-proc` | UNIT; test_interp_proc |
| `test-interp-proc-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-interp-replace` | UNIT; test_interp_replace |
| `test-interp-replace-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-interp-set` | ORACLE-STRONG; test_interp_set |
| `test-interp-set-console-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-interp-set-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-interp-setfmt` | UNIT; test_interp_setfmt |
| `test-interp-setfmt-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-interp-setfmt-mutant-blankdtoc` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-interp-use` | ORACLE-STRONG; test_interp_use |
| `test-interp-use-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-merge-samir` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-ndx-build` | ORACLE-STRONG; test_ndx_build |
| `test-ndx-build-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-ndx-incremental` | ORACLE-STRONG; test_ndx_incremental |
| `test-ndx-incremental-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-ndx-keys` | ORACLE-STRONG; test_ndx_keys |
| `test-ndx-keys-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-ndx-maintain` | ORACLE-STRONG; test_ndx_maintain |
| `test-ndx-maintain-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-ndx-maintain-span-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-ndx-multilevel` | UNIT; test_ndx_multilevel |
| `test-ndx-multilevel-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-ndx-pack` | UNIT; test_ndx_pack |
| `test-ndx-pack-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-ndx-parse` | ORACLE-STRONG; test_ndx_parse |
| `test-ndx-parse-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-ndx-seek` | ORACLE-STRONG; test_ndx_seek |
| `test-ndx-seek-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-pubs-samir-disk` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-pubs-samir-programs` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-samir` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-samir-abbrev` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-samir-abbrev-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-append-safe` | ORACLE-STRONG; test_ibase_dbsafe |
| `test-samir-append-safe-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-core` | ORACLE-STRONG; test_ibase_core |
| `test-samir-core-diff` | ORACLE-STRONG; core-live, THOROUGH=1 (live DOSBox) |
| `test-samir-core-diff-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-core-mutants` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-ctl` | ORACLE-STRONG; test_ibase_ctl |
| `test-samir-ctl-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-datatype` | UNIT; test_ibase_datatype |
| `test-samir-datatype-mutants` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dbf-ref` | ORACLE-STRONG; readers |
| `test-samir-def-deps` | META; left in attic (retired factory) |
| `test-samir-deferred-errors` | UNIT; test_ibase_deferred_errors |
| `test-samir-dml` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-samir-dml-abbrev` | ORACLE-STRONG; test_ibase_dml / abbrev |
| `test-samir-dml-abbrev-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-abbrscope` | ORACLE-STRONG; test_ibase_dml / abbrscope |
| `test-samir-dml-abbrscope-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-append` | ORACLE-STRONG; test_ibase_dml / append |
| `test-samir-dml-append-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-clear` | ORACLE-STRONG; test_ibase_dml / clear |
| `test-samir-dml-clear-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-clear-state` | ORACLE-STRONG; test_ibase_dml / clear-state |
| `test-samir-dml-clear-state-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-close` | ORACLE-STRONG; test_ibase_dml / close |
| `test-samir-dml-close-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-close-state` | ORACLE-STRONG; test_ibase_dml / close-state |
| `test-samir-dml-close-state-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-copyfile` | ORACLE-STRONG; test_ibase_dml / copyfile |
| `test-samir-dml-copyfile-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-erase` | ORACLE-STRONG; test_ibase_dml / erase |
| `test-samir-dml-erase-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-index` | ORACLE-STRONG; test_ibase_dml / index |
| `test-samir-dml-index-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-label` | ORACLE-STRONG; test_ibase_dml / label |
| `test-samir-dml-label-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-macro` | ORACLE-STRONG; test_ibase_dml / macro |
| `test-samir-dml-macro-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-macro-state` | ORACLE-STRONG; test_ibase_dml / macro-state |
| `test-samir-dml-macro-state-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-memory` | ORACLE-STRONG; test_ibase_dml / memory |
| `test-samir-dml-memory-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-mutants` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-reindex` | ORACLE-STRONG; test_ibase_dml / reindex |
| `test-samir-dml-reindex-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-rename` | ORACLE-STRONG; test_ibase_dml / rename |
| `test-samir-dml-rename-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-restore` | ORACLE-STRONG; test_ibase_dml / restore |
| `test-samir-dml-restore-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-run` | ORACLE-STRONG; test_ibase_dml / run |
| `test-samir-dml-run-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-save` | ORACLE-STRONG; test_ibase_dml / save |
| `test-samir-dml-save-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-sort` | ORACLE-STRONG; test_ibase_dml / sort |
| `test-samir-dml-sort-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-status` | ORACLE-STRONG; test_ibase_dml / status |
| `test-samir-dml-status-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-statusinto` | ORACLE-STRONG; test_ibase_dml / statusinto |
| `test-samir-dml-statusinto-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-total` | ORACLE-STRONG; test_ibase_dml / total |
| `test-samir-dml-total-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-dml-type` | ORACLE-STRONG; test_ibase_dml / type |
| `test-samir-dml-type-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-do-file` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-samir-do-file-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-samir-fns-alias` | ORACLE-STRONG; test_ibase_fns / alias (accepted) |
| `test-samir-fns-alias-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-alternate` | ORACLE-STRONG; test_ibase_fns / alternate (accepted) |
| `test-samir-fns-alternate-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-argorder` | ORACLE-STRONG; test_ibase_fns / argorder (accepted) |
| `test-samir-fns-argorder-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-argpicture` | ORACLE-STRONG; test_ibase_fns / argpicture (accepted) |
| `test-samir-fns-argpicture-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-century` | ORACLE-STRONG; test_ibase_fns / century (unregistered but passing; imported by default) |
| `test-samir-fns-col` | ORACLE-STRONG; test_ibase_fns / col (accepted) |
| `test-samir-fns-col-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-console` | ORACLE-STRONG; test_ibase_fns / console (accepted) |
| `test-samir-fns-console-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-default` | ORACLE-STRONG; test_ibase_fns / default; pending CASE=default (legacy known-red evidence, outside default vector) |
| `test-samir-fns-diskspace` | ORACLE-STRONG; test_ibase_fns / diskspace (accepted) |
| `test-samir-fns-diskspace-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-division` | ORACLE-STRONG; test_ibase_fns / division (accepted) |
| `test-samir-fns-division-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-errors` | ORACLE-STRONG; test_ibase_fns / errors (accepted) |
| `test-samir-fns-errors-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-exp` | ORACLE-STRONG; test_ibase_fns / exp (accepted) |
| `test-samir-fns-exp-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-exppicture` | ORACLE-STRONG; test_ibase_fns / exppicture (accepted) |
| `test-samir-fns-exppicture-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-exprcol` | ORACLE-STRONG; test_ibase_fns / exprcol (accepted) |
| `test-samir-fns-exprcol-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-fklabel` | ORACLE-STRONG; test_ibase_fns / fklabel (accepted) |
| `test-samir-fns-fklabel-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-getenv` | ORACLE-STRONG; test_ibase_fns / getenv (accepted) |
| `test-samir-fns-getenv-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-index` | ORACLE-STRONG; test_ibase_fns / index (accepted) |
| `test-samir-fns-index-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-inkey` | ORACLE-STRONG; test_ibase_fns / inkey (accepted) |
| `test-samir-fns-inkey-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-iscolor` | ORACLE-STRONG; test_ibase_fns / iscolor (accepted) |
| `test-samir-fns-iscolor-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-lastkey` | ORACLE-STRONG; test_ibase_fns / lastkey (unregistered but passing; imported by default) |
| `test-samir-fns-limitpic` | ORACLE-STRONG; test_ibase_fns / limitpic (accepted) |
| `test-samir-fns-limitpic-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-listoff` | ORACLE-STRONG; test_ibase_fns / listoff (accepted) |
| `test-samir-fns-listoff-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-log` | ORACLE-STRONG; test_ibase_fns / log (accepted) |
| `test-samir-fns-log-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-logdoarg` | ORACLE-STRONG; test_ibase_fns / logdoarg (accepted) |
| `test-samir-fns-logdoarg-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-logguards` | ORACLE-STRONG; test_ibase_fns / logguards (accepted) |
| `test-samir-fns-logguards-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-lupdate` | ORACLE-STRONG; test_ibase_fns / lupdate (accepted) |
| `test-samir-fns-lupdate-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-matrixb` | ORACLE-STRONG; test_ibase_fns / matrixb (accepted) |
| `test-samir-fns-matrixb-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-mutants` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-ndx` | ORACLE-STRONG; test_ibase_fns / ndx (accepted) |
| `test-samir-fns-ndx-deferred-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-ndx-handler-depth-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-ndx-handler-name-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-ndx-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-ndxdisplay` | ORACLE-STRONG; test_ibase_fns / ndxdisplay (accepted) |
| `test-samir-fns-ndxdisplay-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-ndxdoarg` | ORACLE-STRONG; test_ibase_fns / ndxdoarg (accepted) |
| `test-samir-fns-ndxdoarg-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-numconvert` | ORACLE-STRONG; test_ibase_fns / numconvert (accepted) |
| `test-samir-fns-numconvert-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-numdisplay` | ORACLE-STRONG; test_ibase_fns / numdisplay; pending CASE=numdisplay (legacy known-red evidence, outside default vector) |
| `test-samir-fns-numfmt` | ORACLE-STRONG; test_ibase_fns / numfmt; pending CASE=numfmt (legacy known-red evidence, outside default vector) |
| `test-samir-fns-numfmtwide` | ORACLE-STRONG; test_ibase_fns / numfmtwide; pending CASE=numfmtwide (legacy known-red evidence, outside default vector) |
| `test-samir-fns-numlist` | ORACLE-STRONG; test_ibase_fns / numlist (accepted) |
| `test-samir-fns-numlist-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-numround` | ORACLE-STRONG; test_ibase_fns / numround; pending CASE=numround (legacy known-red evidence, outside default vector) |
| `test-samir-fns-os` | ORACLE-STRONG; test_ibase_fns / os (accepted) |
| `test-samir-fns-os-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-overflow` | ORACLE-STRONG; test_ibase_fns / overflow; pending CASE=overflow (legacy known-red evidence, outside default vector) |
| `test-samir-fns-path` | ORACLE-STRONG; test_ibase_fns / path; pending CASE=path (legacy known-red evidence, outside default vector) |
| `test-samir-fns-pcol` | ORACLE-STRONG; test_ibase_fns / pcol (accepted) |
| `test-samir-fns-pcol-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-pending` | ORACLE-STRONG; test_ibase_fns / pending; pending CASE=pending (legacy known-red evidence, outside default vector) |
| `test-samir-fns-procedure` | ORACLE-STRONG; test_ibase_fns / procedure (unregistered but passing; imported by default) |
| `test-samir-fns-prow` | ORACLE-STRONG; test_ibase_fns / prow (accepted) |
| `test-samir-fns-prow-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-readkey` | ORACLE-STRONG; test_ibase_fns / readkey (accepted) |
| `test-samir-fns-readkey-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-recsize` | SIZE/LAYOUT; left in attic; packaging blocked |
| `test-samir-fns-recsize-mutant` | SIZE/LAYOUT; left in attic; packaging blocked |
| `test-samir-fns-relation` | ORACLE-STRONG; test_ibase_fns / relation; pending CASE=relation (legacy known-red evidence, outside default vector) |
| `test-samir-fns-retryconfirm` | ORACLE-STRONG; test_ibase_fns / retryconfirm (accepted) |
| `test-samir-fns-retryconfirm-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-returnarg` | ORACLE-STRONG; test_ibase_fns / returnarg (accepted) |
| `test-samir-fns-returnarg-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-row` | ORACLE-STRONG; test_ibase_fns / row (accepted) |
| `test-samir-fns-row-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-sqrt` | ORACLE-STRONG; test_ibase_fns / sqrt (accepted) |
| `test-samir-fns-sqrt-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-sqrtdoarg` | ORACLE-STRONG; test_ibase_fns / sqrtdoarg (accepted) |
| `test-samir-fns-sqrtdoarg-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-sqrtpass` | ORACLE-STRONG; test_ibase_fns / sqrtpass (accepted) |
| `test-samir-fns-sqrtpass-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-struct` | ORACLE-STRONG; test_ibase_fns / struct (accepted) |
| `test-samir-fns-struct-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-talk` | ORACLE-STRONG; test_ibase_fns / talk; pending CASE=talk (legacy known-red evidence, outside default vector) |
| `test-samir-fns-time` | ORACLE-STRONG; test_ibase_fns / time (accepted) |
| `test-samir-fns-time-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-type` | ORACLE-STRONG; test_ibase_fns / type (accepted) |
| `test-samir-fns-type-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-unknown` | ORACLE-STRONG; test_ibase_fns / unknown (accepted) |
| `test-samir-fns-unknown-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-version` | ORACLE-STRONG; test_ibase_fns / version (accepted) |
| `test-samir-fns-version-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-fns-with` | ORACLE-STRONG; test_ibase_fns / with; pending CASE=with (legacy known-red evidence, outside default vector) |
| `test-samir-format-mutants` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-high-stack` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-samir-high-stack-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-input` | ORACLE-STRONG; test_ibase_input |
| `test-samir-input-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-lifetime` | ORACLE-STRONG; test_ibase_dbsafe |
| `test-samir-lifetime-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-link-reserve-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-list-layout` | ORACLE-STRONG; test_ibase_view |
| `test-samir-list-layout-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-memocells` | UNIT; test_ibase_memocells |
| `test-samir-memwidth` | ORACLE-STRONG; test_ibase_fns / memwidth |
| `test-samir-memwidth-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-messages` | ORACLE-STRONG; test_ibase_messages |
| `test-samir-messages-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-mzlink-io` | UNIT; mzlink-io |
| `test-samir-ndx-names` | UNIT; test_ibase_ndx_names |
| `test-samir-ndx-ref` | ORACLE-STRONG; readers |
| `test-samir-numeric-format` | ORACLE-STRONG; ibase_numeric_format |
| `test-samir-numeric-format-fraction-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-numeric-format-left-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-ovl` | UNIT; test_ibase_ovl (24 pure); B5 26 pair checks deferred to package |
| `test-samir-ovl-lifetime` | UNIT; test_ibase_ovl_lifetime |
| `test-samir-ovl-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-ovl-output` | SIZE/LAYOUT; left in attic; packaging blocked |
| `test-samir-ovl-static` | SIZE/LAYOUT; left in attic; packaging blocked |
| `test-samir-ovl-static-mutant` | SIZE/LAYOUT; left in attic; packaging blocked |
| `test-samir-pal` | UNIT; test_ibase_pal_contract |
| `test-samir-pal-host` | UNIT; test_ibase_pal_host |
| `test-samir-print` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-samir-print-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-query` | UNIT; test_ibase_query |
| `test-samir-registries` | META; left in attic (retired factory) |
| `test-samir-registries-mutant` | META; left in attic (retired factory) |
| `test-samir-repl` | UNIT; test_ibase_repl |
| `test-samir-repl-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-report` | ORACLE-STRONG; test_ibase_report |
| `test-samir-report-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-rt` | ORACLE-STRONG; test_ibase_rt |
| `test-samir-safety` | ORACLE-STRONG; test_ibase_dbsafe |
| `test-samir-safety-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-say` | UNIT; test_ibase_say |
| `test-samir-scratch-lock` | META; left in attic (retired factory) |
| `test-samir-set-color` | UNIT; test_ibase_set_color |
| `test-samir-set-color-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-set-headings` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-samir-set-headings-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-size` | SIZE/LAYOUT; left in attic; packaging blocked |
| `test-samir-size-mutant` | SIZE/LAYOUT; left in attic; packaging blocked |
| `test-samir-size-repro` | SIZE/LAYOUT; left in attic; packaging blocked |
| `test-samir-softfp` | ORACLE-STRONG; test_ibase_softfp |
| `test-samir-softfp-implmutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-softfp-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-spec` | UNIT; test_ibase_spec |
| `test-samir-status` | UNIT; test_ibase_status |
| `test-samir-status-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-trace` | META; left in attic (retired factory) |
| `test-samir-undefined` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-samir-undefined-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-use` | ORACLE-STRONG; test_ibase_view |
| `test-samir-use-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-value` | UNIT; test_ibase_value |
| `test-samir-view` | ORACLE-STRONG; test_ibase_view |
| `test-samir-view-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-w2-history` | ORACLE-STRONG; test_ibase_w2 / history; pending CASE=history (legacy aggregate/unfinished case) |
| `test-samir-w2-history-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-w2-host-adapter` | ORACLE-STRONG; test_ibase_w2 / host-adapter; pending CASE=host-adapter (legacy aggregate/unfinished case) |
| `test-samir-w2-pending` | ORACLE-STRONG; test_ibase_w2 / pending; pending CASE=pending (legacy aggregate/unfinished case) |
| `test-samir-w2-set-catalog` | ORACLE-STRONG; test_ibase_w2 / set-catalog; pending CASE=set-catalog (legacy aggregate/unfinished case) |
| `test-samir-w2-set-default` | ORACLE-STRONG; test_ibase_w2 / set-default; pending CASE=set-default (legacy aggregate/unfinished case) |
| `test-samir-w2-set-dohistory` | ORACLE-STRONG; test_ibase_w2 / set-dohistory (accepted) |
| `test-samir-w2-set-fields` | ORACLE-STRONG; test_ibase_w2 / set-fields; pending CASE=set-fields (legacy aggregate/unfinished case) |
| `test-samir-w2-set-fixed` | ORACLE-STRONG; test_ibase_w2 / set-fixed (accepted) |
| `test-samir-w2-set-fixed-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-w2-set-headings` | ORACLE-STRONG; test_ibase_w2 / set-headings (unregistered but passing; imported by default) |
| `test-samir-w2-set-history` | ORACLE-STRONG; test_ibase_w2 / set-history (accepted) |
| `test-samir-w2-set-historygrow` | ORACLE-STRONG; test_ibase_w2 / set-historygrow; pending CASE=set-historygrow (legacy aggregate/unfinished case) |
| `test-samir-w2-set-historyresize` | SIZE/LAYOUT; left in attic; packaging blocked |
| `test-samir-w2-set-historysame` | ORACLE-STRONG; test_ibase_w2 / set-historysame; pending CASE=set-historysame (legacy aggregate/unfinished case) |
| `test-samir-w2-set-historyslots` | ORACLE-STRONG; test_ibase_w2 / set-historyslots (accepted) |
| `test-samir-w2-set-margin` | ORACLE-STRONG; test_ibase_w2 / set-margin (accepted) |
| `test-samir-w2-set-margin-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-w2-set-memowidth` | ORACLE-STRONG; test_ibase_w2 / set-memowidth (accepted) |
| `test-samir-w2-set-memowidth-mutant-ignore` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-w2-set-memowidth-mutant-nowrap256` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-w2-set-memowidth-mutants` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-w2-set-path` | ORACLE-STRONG; test_ibase_w2 / set-path; pending CASE=set-path (legacy aggregate/unfinished case) |
| `test-samir-w2-set-print` | ORACLE-STRONG; test_ibase_w2 / set-print (unregistered but passing; imported by default) |
| `test-samir-w2-set-procedure` | ORACLE-STRONG; test_ibase_w2 / set-procedure (unregistered but passing; imported by default) |
| `test-samir-w2-set-unique` | ORACLE-STRONG; test_ibase_w2 / set-unique (accepted) |
| `test-samir-w2-set-uniquerebuild` | ORACLE-STRONG; test_ibase_w2 / set-uniquerebuild (accepted) |
| `test-samir-w2-set-view` | ORACLE-STRONG; test_ibase_w2 / set-view; pending CASE=set-view (legacy aggregate/unfinished case) |
| `test-samir-w2-unique` | ORACLE-STRONG; test_ibase_w2 / unique; pending CASE=unique (legacy aggregate/unfinished case) |
| `test-samir-w2-unique-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-samir-w2-verb-average` | ORACLE-STRONG; test_ibase_w2 / verb-average; pending CASE=verb-average (legacy aggregate/unfinished case) |
| `test-samir-w2-verb-count` | ORACLE-STRONG; test_ibase_w2 / verb-count; pending CASE=verb-count (legacy aggregate/unfinished case) |
| `test-samir-w2-verb-dir` | ORACLE-STRONG; test_ibase_w2 / verb-dir; pending CASE=verb-dir (legacy aggregate/unfinished case) |
| `test-samir-w2-verb-eject` | ORACLE-STRONG; test_ibase_w2 / verb-eject (unregistered but passing; imported by default) |
| `test-samir-w2-verb-insert` | ORACLE-STRONG; test_ibase_w2 / verb-insert; pending CASE=verb-insert (legacy aggregate/unfinished case) |
| `test-samir-w2-verb-join` | ORACLE-STRONG; test_ibase_w2 / verb-join; pending CASE=verb-join (legacy aggregate/unfinished case) |
| `test-samir-w2-verb-sum` | ORACLE-STRONG; test_ibase_w2 / verb-sum; pending CASE=verb-sum (legacy aggregate/unfinished case) |
| `test-samir-w2-verb-text` | ORACLE-STRONG; test_ibase_w2 / verb-text; pending CASE=verb-text (legacy aggregate/unfinished case) |
| `test-samir-w2-verb-update` | ORACLE-STRONG; test_ibase_w2 / verb-update; pending CASE=verb-update (legacy aggregate/unfinished case) |
| `test-screen-mint-samir` | Aggregate/alias; constituent suites above (not an additional case) |
| `test-use-rw` | ORACLE-STRONG; test_use_rw |
| `test-use-rw-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-w4-files` | ORACLE-STRONG; w4-files |
| `test-w4-inventory` | META; left in attic (retired factory) |
| `test-w4-inventory-mutant` | META; left in attic (retired factory) |
| `test-w4-retained` | ORACLE-STRONG; retained-screens (producer identity META excluded) |
| `test-w4-structure-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-w4-trace-smoke` | META; left in attic (retired factory) |
| `test-xbase-coercion` | ORACLE-STRONG; dbf_coerce_fuzz |
| `test-xbase-coercion-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-xbase-coercion-mutant-blankdate` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-xbase-eval` | UNIT; test_xbase_eval |
| `test-xbase-eval-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-xbase-eval-mutant-blankdate` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-xbase-fn-a` | UNIT; test_xbase_fn_a |
| `test-xbase-fn-a-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-xbase-fn-a-mutant-ctod` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-xbase-fn-b` | UNIT; test_xbase_fn_b |
| `test-xbase-fn-b-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-xbase-fn-b-mutant-round` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-xbase-fn-c` | UNIT; test_xbase_fn_c |
| `test-xbase-fn-c-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-xbase-fn-d` | ORACLE-STRONG; test_xbase_fn_d |
| `test-xbase-fn-d-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-xbase-lex` | UNIT; test_xbase_lex |
| `test-xbase-lex-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-xbase-parse` | UNIT; test_xbase_parse |
| `test-xbase-parse-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-xbase-transform` | UNIT; test_xbase_transform |
| `test-xbase-transform-dotwidth-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-xbase-transform-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |
| `test-xbase-transform-rounding-mutant` | Mutation; original guard retained, patch conversion in MUTANTS-TODO.md |

## Left behind and limits

- test_hardware_spec.c: kernel hardware spec, not database behaviour.
- test_scratch_lock.c/.sh, registries.mk/check_registries.c, gate maps,
  trace/certificate/merge-train, pending-inventory and binary-provenance audits: META.
- test_ibase_size.c, samir_size.mk and overlay static/output packing gates: SIZE/LAYOUT.
- Emulator drivers, mint/capture tools and trace makefiles: deferred tier 1; SMOKES.md.
- No component C sources or retired make fragments are built from attic.
- Large captured directories retain unused pending fixtures; these are evidence,
  not additional claims of implemented features. See make pending CASE=... .

## Target flags and packaging

Dropped legacy size flags: -Oz, -flto, -flto-partition=one, -fomit-frame-pointer,
-fcrossjumping, -fno-inline-functions-called-once, -fno-partial-inlining,
-fno-jump-tables/-fjump-tables, -mregparm=3, -mrtd, -fno-ipa-sra, -mtune=generic,
-fno-forward-propagate, -fno-shrink-wrap, -fno-defer-pop,
-fno-stack-clash-protection, -fno-tree-dominator-opts, -fno-tree-tail-merge,
-fno-tree-sra, -fno-tree-loop-im, -fno-tree-scev-cprop,
-fno-guess-branch-probability, -fno-code-hoisting, -fno-move-loop-invariants,
-fno-caller-saves, --param=max-inline-insns-single=200/175, -fno-early-inlining,
-mpreferred-stack-boundary=2, -mincoming-stack-boundary=2; also no extra -fno-pie.
The default shared -Os supplies normal optimization. Soft-float, section isolation
and runtime arena/registry/overlay configuration remain semantic requirements;
the default ABI is used coherently. --gc-sections and retained relocation records
remain on the optional package link.

Without LTO the required reserve is 65616 bytes (slot 13968, BSS/alignment and
PAL tail 51648), 80 over the loader declaration. Objects build, package fails
on that precise assertion. No other owner implementation is missing. Borrowed
memory_map.h and initechmz_extended.h remain explicitly temporary dependencies.
Overlay removal is deferred; README describes the required dispatch/lifetime work.

## Red controls and acceptance

Changed one evaluator line from numeric addition to subtraction: test_xbase_eval
reported 123 checks / 2 failures (A1 N+N and A9 precedence), make exit 2. Restored
the evaluator byte-for-byte before clean acceptance.
DBASE3_DECOMP=/no/such/dbase-corpus: exit 2,
FAIL: dBASE corpus missing; set DBASE3_DECOMP to dbase3-decomp.
Pre-rename clean acceptance: make clean; /usr/bin/time make test:
75 suites, 0 failures, 37.29 seconds. Default target build: 23 ELF32 i386 objects
(22 C plus startup), green. The package assertion is a documented limitation,
not a successful executable build. The earlier THOROUGH run passed all 75 entries
before the final MZ I/O entry was added; final THOROUGH verification follows.
Top-level make test: region, Initech 123 and InitechBase all green (ALL GREEN).
The pre-rename evaluator and corpus red logs were captured before the clean run.

Counted legacy summaries total 44493; imported summaries total 44504.
The differences are the split overlay pair checks (-26), explicit short-read
checks (+12), and 25 getcwd assertions (+25). Retained-screen producer-identity
META failure is reported above rather than counted as a passed legacy check.

## Rename evidence

The import baseline commit is de41c480. The subsequent commit renames the
component's identifiers/macros to ibase_/IBASE_, headers to include/ibase,
startup/linker/main/tools/tests, the DOS PAL binding, and INITBASE.EXE/INITBASE.OVL.
The optional MZ emitter's component profile option is now --ibase.
1969 captured/provenance payload files were hash-compared before/after and are
byte-identical. Remaining whole-word samir/milton matches are historical target
names in this inventory/SMOKES.md, preserved capture provenance, the real
SEEK "MILTON" customer-name literal in two captured PRGs, and a historical
salami-program comment. None is a product identifier or serial marker.

Post-rename clean acceptance: make clean; /usr/bin/time make test:
75 suites, 0 failures, 35.24 seconds. Target object build: 4.08 seconds,
23 ELF32 i386 objects. THOROUGH=1: 76 suites, 0 failures, 27.84 seconds warm,
including live real dBASE and its 104 independent file postchecks.
The renamed package target still fails on the unchanged reserve assertion;
this is the same 80-byte overrun as before the rename.
All product/test/factory C, headers, assembly, linker and shell sources are ASCII.
git diff --check and shell syntax checks pass. No core/cmd/fs foreign include
path is present. STATUS.md was intentionally not edited under the ownership
restriction; the component README and this inventory describe its actual state.

Full overlay B5 validation is retained as make pair-test EXE=/absolute/file.EXE.
It was run after renaming against the frozen legacy build/SAMIR.EXE and its
matching overlay: 50 checks, 0 failures. Default --pure still runs 24 checks;
the remaining 26 validate an executable pair and cannot grade our unlinked target.

The full pair check caught an accidental rename of the eight-byte overlay
signature. Restored the exact legacy format bytes, expressed as hex escapes;
only the C macro name changes. Both the pure literal test and the independent
frozen EXE/OVL pair now grade those compatibility bytes. The original failed
pair check was 50 checks / 1 failure at B5 signature and version.

Final unregistered-case qualification ran all 32 additional FNS/W2 captures:
7 pass unchanged in BOTH legacy and new binaries and now run by default;
25 remain explicit pending failures. No ACCEPTED/PENDING provenance markers
were changed. FNS has 49 cases, W2 13, DML 25, datatype and DO-file 9 each.
The report's independent 124-output grep and NDX built-in undefined-symbol
boundary check are retained in the run wrapper, in addition to C assertions.
A final clean run after restoring the fixed overlay signature passed 75 suites
in 34.06 seconds. The final vector also includes the seven additional cases;
its warm THOROUGH timing and top-level result follow below.

Final warm THOROUGH vector (including all seven extra cases and postchecks):
76 suites, 0 failures, 46.88 seconds while a separate top-level build ran.
Top-level make test: all three imported components green, ALL GREEN.
The final target make also passed after the wrapper/case-list updates.

Final clean acceptance with the complete 75-suite case vector: 33.55 seconds,
0 failures. The final dependency audit adds explicit REPL prerequisites for the
two drivers that include the REPL C source through w2_host.c. This changes only
rebuild tracking. All target objects and the final top-level tests are verified
after that dependency correction.

Final post-dependency top-level make -j32 test: ALL GREEN. Full pair-test: 50 checks, 0 failures.
