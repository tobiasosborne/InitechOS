# SAMIR DML exact acceptance evidence

The unit and emulator aggregates return nonzero solely because RUN remains RED.
The fifteen implemented baseline cases and their fifteen named engine mutants
pass their acceptance recipes. No baseline failure is counted as mutation proof.
All emulator gates use INITECH_QMP_STDIO=1. No aggregate make test or Bochs.

## Exact lane targets

Every row has all four targets in TEST_UNIT_GATES / TEST_EMU_GATES. Vectors are
expanded by the existing test-unit/test-emu recipes after all additions.

| Verb | Host | Host mutant | QEMU | QEMU mutant | Printed Using III+ pages |
|---|---|---|---|---|---|
| clear | `test-samir-dml-clear` | `test-samir-dml-clear-mutant` | `test-samir-dml-clear-emu` | `test-samir-dml-clear-emu-mutant` | U5-51 |
| macro | `test-samir-dml-macro` | `test-samir-dml-macro-mutant` | `test-samir-dml-macro-emu` | `test-samir-dml-macro-emu-mutant` | U6-7/8 |
| close | `test-samir-dml-close` | `test-samir-dml-close-mutant` | `test-samir-dml-close-emu` | `test-samir-dml-close-emu-mutant` | U5-53 |
| index | `test-samir-dml-index` | `test-samir-dml-index-mutant` | `test-samir-dml-index-emu` | `test-samir-dml-index-emu-mutant` | U5-131/132 |
| reindex | `test-samir-dml-reindex` | `test-samir-dml-reindex-mutant` | `test-samir-dml-reindex-emu` | `test-samir-dml-reindex-emu-mutant` | U5-172 |
| sort | `test-samir-dml-sort` | `test-samir-dml-sort-mutant` | `test-samir-dml-sort-emu` | `test-samir-dml-sort-emu-mutant` | U5-269/270 |
| total | `test-samir-dml-total` | `test-samir-dml-total-mutant` | `test-samir-dml-total-emu` | `test-samir-dml-total-emu-mutant` | U5-276 |
| append | `test-samir-dml-append` | `test-samir-dml-append-mutant` | `test-samir-dml-append-emu` | `test-samir-dml-append-emu-mutant` | U5-28 |
| save | `test-samir-dml-save` | `test-samir-dml-save-mutant` | `test-samir-dml-save-emu` | `test-samir-dml-save-emu-mutant` | U5-187 |
| restore | `test-samir-dml-restore` | `test-samir-dml-restore-mutant` | `test-samir-dml-restore-emu` | `test-samir-dml-restore-emu-mutant` | U5-180/181 |
| copyfile | `test-samir-dml-copyfile` | `test-samir-dml-copyfile-mutant` | `test-samir-dml-copyfile-emu` | `test-samir-dml-copyfile-emu-mutant` | U5-59 |
| type | `test-samir-dml-type` | `test-samir-dml-type-mutant` | `test-samir-dml-type-emu` | `test-samir-dml-type-emu-mutant` | U5-278 |
| rename | `test-samir-dml-rename` | `test-samir-dml-rename-mutant` | `test-samir-dml-rename-emu` | `test-samir-dml-rename-emu-mutant` | U5-175 |
| erase | `test-samir-dml-erase` | `test-samir-dml-erase-mutant` | `test-samir-dml-erase-emu` | `test-samir-dml-erase-emu-mutant` | U5-121 |
| label | `test-samir-dml-label` | `test-samir-dml-label-mutant` | `test-samir-dml-label-emu` | `test-samir-dml-label-emu-mutant` | U5-143 |
| run | `test-samir-dml-run` | `test-samir-dml-run-mutant` | `test-samir-dml-run-emu` | `test-samir-dml-run-emu-mutant` | U5-185/186 |

Aggregate targets: mint-samir-dml, samir-dml-goldens, test-samir-dml,
test-samir-dml-mutants, test-samir-dml-emu, test-samir-dml-emu-mutants.
State targets retained: test-samir-dml-clear-state,
test-samir-dml-macro-state, test-samir-dml-close-state and each -mutant.
Printer additions: test-samir-print, test-samir-print-mutant,
test-samir-print-emu, test-samir-print-emu-mutant (all in the vectors).
Mint/integrity additions: mint-samir-prog, mint-samir-dml-extra,
samir-prog-goldens (integrity gate in TEST_UNIT_GATES and prerequisites of the
three existing program differential targets).

## Exact GREEN tails

Each summary below was immediately followed by
`>>> test-samir-dml-<verb>[-emu]: green` for the exact target in the table above.
Logs: build/samirdml/final-refactor-host.log and acceptance-emu.log.

| Verb | Host summary | QEMU summary |
|---|---|---|
| clear | `test-samir-dml: 4 checks, 0 failures` | `test-samir-dml: 11 checks, 0 failures` |
| macro | `test-samir-dml: 4 checks, 0 failures` | `test-samir-dml: 11 checks, 0 failures` |
| close | `test-samir-dml: 4 checks, 0 failures` | `test-samir-dml: 11 checks, 0 failures` |
| index | `test-samir-dml: 6 checks, 0 failures` | `test-samir-dml: 14 checks, 0 failures` |
| reindex | `test-samir-dml: 7 checks, 0 failures` | `test-samir-dml: 15 checks, 0 failures` |
| sort | `test-samir-dml: 5 checks, 0 failures` | `test-samir-dml: 13 checks, 0 failures` |
| total | `test-samir-dml: 5 checks, 0 failures` | `test-samir-dml: 13 checks, 0 failures` |
| append | `test-samir-dml: 5 checks, 0 failures` | `test-samir-dml: 12 checks, 0 failures` |
| save | `test-samir-dml: 5 checks, 0 failures` | `test-samir-dml: 13 checks, 0 failures` |
| restore | `test-samir-dml: 5 checks, 0 failures` | `test-samir-dml: 12 checks, 0 failures` |
| copyfile | `test-samir-dml: 5 checks, 0 failures` | `test-samir-dml: 13 checks, 0 failures` |
| type | `test-samir-dml: 4 checks, 0 failures` | `test-samir-dml: 11 checks, 0 failures` |
| rename | `test-samir-dml: 6 checks, 0 failures` | `test-samir-dml: 14 checks, 0 failures` |
| erase | `test-samir-dml: 5 checks, 0 failures` | `test-samir-dml: 12 checks, 0 failures` |
| label | `test-samir-dml: 6 checks, 0 failures` | `test-samir-dml: 14 checks, 0 failures` |
| run | `RED` | `RED` |

RUN's current host and QEMU first difference is:
`DML transcript differs at byte 11 (got 0x31, real 0x0d; lengths 48/47)`.
Both reach the unrecognized-command diagnostic, omit DML-END and fail to create
VERSION.TXT. The gate explicitly prints the missing PAL/nested-EXEC seam.
The RUN mutant recipes do not run because their baseline prerequisite is RED.

## Named mutants and actual RED evidence

Host diagnostic offsets are relative to DML-BEGIN. All fifteen also fail their
QEMU file/transcript comparison while the locked guest trace returns normally.
The mutant recipes themselves are GREEN when this expected failure is observed.

| Engine mutant | Actual host RED detail | Actual guest failed assertion |
|---|---|---|
| SAMIR_DML_MUTANT_clear | `DML transcript differs at byte 13 (got 0x4e, real 0x55; lengths 52/52)` | `FAIL: DML transcript byte-identical to real III+` |
| SAMIR_DML_MUTANT_macro | `DML transcript differs at byte 11 (got 0x31, real 0x0d; lengths 48/70)` | `FAIL: DML transcript byte-identical to real III+` |
| SAMIR_DML_MUTANT_close | `DML transcript differs at byte 22 (got 0x30, real 0x32; lengths 145/145)` | `FAIL: DML transcript byte-identical to real III+` |
| SAMIR_DML_MUTANT_index | `DML transcript differs at byte 22 (got 0x34, real 0x33; lengths 64/74)` | `FAIL: DML transcript byte-identical to real III+` |
| SAMIR_DML_MUTANT_reindex | `DML transcript differs at byte 22 (got 0x34, real 0x33; lengths 37/47)` | `FAIL: DML transcript byte-identical to real III+` |
| SAMIR_DML_MUTANT_sort | `DML transcript differs at byte 88 (got 0x57, real 0x4c; lengths 260/260)` | `FAIL: DML transcript byte-identical to real III+` |
| SAMIR_DML_MUTANT_total | `DML transcript differs at byte 90 (got 0x20, real 0x37; lengths 151/151)` | `FAIL: DML transcript byte-identical to real III+` |
| SAMIR_DML_MUTANT_append | `DML transcript differs at byte 22 (got 0x35, real 0x36; lengths 77/77)` | `FAIL: DML transcript byte-identical to real III+` |
| SAMIR_DML_MUTANT_save | `FAIL: DML VALUES.MEM bytes match real III+` | `FAIL: DML VALUES.MEM bytes match real III+` |
| SAMIR_DML_MUTANT_restore | `DML transcript differs at byte 13 (got 0x43, real 0x55; lengths 70/70)` | `FAIL: DML transcript byte-identical to real III+` |
| SAMIR_DML_MUTANT_copyfile | `FAIL: DML COPIED.TXT bytes match real III+` | `FAIL: DML COPIED.TXT bytes match real III+` |
| SAMIR_DML_MUTANT_type | `DML transcript differs at byte 13 (got 0x0d, real 0x41; lengths 45/71)` | `FAIL: DML transcript byte-identical to real III+` |
| SAMIR_DML_MUTANT_rename | `DML transcript differs at byte 14 (got 0x54, real 0x46; lengths 51/51)` | `FAIL: DML transcript byte-identical to real III+` |
| SAMIR_DML_MUTANT_erase | `DML transcript differs at byte 14 (got 0x54, real 0x46; lengths 47/47)` | `FAIL: DML transcript byte-identical to real III+` |
| SAMIR_DML_MUTANT_label | `DML transcript differs at byte 15 (got 0x0d, real 0x43; lengths 241/3745)` | `FAIL: DML transcript byte-identical to real III+` |
| SAMIR_PRINT_MUTATE_LF | `DML transcript differs at byte 11 (got 0x0a, real 0x0d; lengths 50/52)` | `FAIL: DML transcript byte-identical to real III+` |

The retained three state mutants additionally fail: hidden PRIVATE N removed,
macro assignment executed, and DBF/cursor preserved by CLOSE INDEX.

## Existing regression gates (GREEN)

- test-samir: PAL contract 23, runtime 52, host PAL 28, spec 111, value 59,
  dbf_ref 147 and ndx_ref 116 checks; zero failures.
- test-dbase-roundtrip: `test-dbase-roundtrip: 69 checks, 0 failures`.
- test-dbase-diff: `xbase-prog-diff: 8 checks, 0 failures`.
- test-dbase-diff-mutant: `green (=-direction swap correctly RED)`.
- test-dbase-diff-mutants: `green (ALL SIX corpus goldens mutation-proven)`.
- test-dbase: `M6 InitechBase differential GREEN -- round-trip + program diff (100%)`.
- test-pubs-examples: `green (1 SAMIR programs replayed byte for byte; screen examples use test-pubs-screens)`.
- test-samir-use: `151 checks, 0 failures`, including unchanged mint-005 replay.
- test-xbase-fn-a: `86 checks, 0 failures`; both existing mutants correctly RED.
- test-interp-flow: `74 checks, 0 failures`; existing runs-all mutant correctly RED.
- test-samir-report: `1056 checks, 0 failures`; `111 REPORT FORM files byte-identical to real dBASE III PLUS 1.1`; subtotal/column/page mutants correctly RED.
- image: `SAMIR.COM: 103551 bytes (flat .COM @0x40100, soft-float, x87=0)`; `image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS + DOS + AUTOEXEC.BAT)`.
- test-samir-boot: `[5/5]: screendump shows SAMIR's dot prompt + LIST table on the seafoam desktop`.
- test-samir-write: `[5/5]: PERSISTED -- independent reader confirms 4 records, rec0 BAL=9999.99, rec3 BAL=5555.55 ON DISK`.
- test-samir-canon-y2k: `[4/4]: screendump shows the AR aging report on the seafoam desktop`.
- test-flair-locked-purge: `VERDICT: PASS -- test-flair-locked-purge (locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat)`.
- test-flair-finder-service: `green` (both healthy and damaged volume legs).
- test-zs24-exec: `[5/5]: both subdir EXECs returned to the REPL; EXIT halted cleanly (SHELL-EXIT + SHELL-DONE)`; this is sequential EXEC, not nested EXEC.

Current regression logs: final-shared-printer.log, final-host-recheck.log,
affected-unit-green.log and dbf-projection-green.log under build/samirdml/.
The older authored mutate.dbf.out was regenerated through dbf_ref.py from real
PROG-07 PARTS.DBF; its bytes were already identical. PROG-07b:
build/samirdml/prog-mint/mutate/PARTS.records, copied to
harness/diff/dbf_diff/xbase_prog_diff/golden/mutate.dbf.out.

## Files changed across the lane

- Makefile.
- os/samir/samir_main.c; cmd/flow.c, proc.c, workarea.c, query.c, mutate.c, report.c.
- os/samir/core/value.c, eval.c, lex.c, parse.c, fn_builtins.c.
- os/samir/include/samir/value.h, eval.h, workarea.h.
- os/samir/pal/pal_milton.c; boot/softfp.c, samir.ld.
- harness/diff/dbf_diff/test_samir_dml.c, test_samir_audit.c,
  test_samir_screen_emu.c, test_xbase_fn_a.c.
- harness/diff/dbf_diff/dml_mint.sh, prog_mint.sh, dml_probe.c, dml_extra_mint.sh.
- harness/diff/dbf_diff/xbase_prog_diff/prog_diff.c.
- harness/diff/dbf_diff/xbase_prog_diff/corpus/dml/{clear,macro,close,index,reindex,sort,total,append,save,restore,copyfile,type,rename,erase,label,run}.prg (retained).
- harness/diff/dbf_diff/xbase_prog_diff/dml-goldens.sha256 (retained), prog-real.sha256.
- harness/diff/dbf_diff/xbase_prog_diff/golden/{expr,funcs,exact,flow,proc,query,mutate}.out; mutate.dbf.out independently re-proven unchanged.
- pubs/examples/samir/salary.txt.
- docs/worklog/samirdml-mint.md and this report.

No fs/, kernel loader, git metadata, CLAUDE.md, HANDOFF.md or beads edits.
The bounded-feature limitations and all mint ledger rows are in
samirdml-mint.md. The local manual is
/home/tobias/Projects/initech-manuals-ref/dbase3plus/dBASE III Plus 1.1 (1986) Manuals/Using dBase III Plus.pdf.
? / ?? is U5-15; TYPE is U6-86/87; all implemented verb pages in the first table
were rendered at 110 dpi and inspected. PRD 6.6 and ADR-0008/0009 govern the
PAL/freestanding architecture. No broader language or failure-path parity is
claimed beyond the independently graded corpus.
