# SAMIR DML mint evidence and bounded acceptance

Real dBASE III PLUS 1.1, dosbox-x 2024.03.01, isolated drive under `build/samirdml/golden/`. Reference installation was read only. No dBASE binaries or copied fixtures are proposed for commit. Mint recipe: `harness/diff/dbf_diff/dml_mint.sh`; corpus: `harness/diff/dbf_diff/xbase_prog_diff/corpus/dml/`.

Ledger rows DML-01 through DML-16 pin independently obtained evidence. Every output has a completed `[DML-BEGIN]` / `[DML-END]` transcript and no MINT-ERROR. dosbox-x retains its headless window after QUIT on this host, so the recipe bounds it with timeout after the alternate file is flushed. `dml-goldens.sha256` pins every input program, output transcript and data file; regeneration does not update that checked-in manifest.

| Row | Verb | Real output and generated artifacts | Printed manual pages |
|---|---|---|---|
| DML-01 | clear | `build/samirdml/golden/clear/GOLD.OUT`; `build/samirdml/golden/clear/BASE.DBF`, `build/samirdml/golden/clear/INPUT.TXT` | U5-51 |
| DML-02 | macro | `build/samirdml/golden/macro/GOLD.OUT`; `build/samirdml/golden/macro/BASE.DBF`, `build/samirdml/golden/macro/INPUT.TXT` | U6-7/8 |
| DML-03 | close | `build/samirdml/golden/close/GOLD.OUT`; `build/samirdml/golden/close/BASE.DBF`, `build/samirdml/golden/close/INPUT.TXT` | U5-53 |
| DML-04 | index | `build/samirdml/golden/index/GOLD.OUT`; `build/samirdml/golden/index/BASE.DBF`, `build/samirdml/golden/index/INPUT.TXT`, `build/samirdml/golden/index/NAMES.NDX` | U5-131/132 |
| DML-05 | reindex | `build/samirdml/golden/reindex/GOLD.OUT`; `build/samirdml/golden/reindex/BASE.DBF`, `build/samirdml/golden/reindex/INPUT.TXT`, `build/samirdml/golden/reindex/NAMES.NDX` | U5-172 |
| DML-06 | sort | `build/samirdml/golden/sort/GOLD.OUT`; `build/samirdml/golden/sort/BASE.DBF`, `build/samirdml/golden/sort/INPUT.TXT`, `build/samirdml/golden/sort/SORTED.DBF` | U5-269/270 |
| DML-07 | total | `build/samirdml/golden/total/GOLD.OUT`; `build/samirdml/golden/total/BASE.DBF`, `build/samirdml/golden/total/CITIES.NDX`, `build/samirdml/golden/total/INPUT.TXT`, `build/samirdml/golden/total/TOTALS.DBF` | U5-276 |
| DML-08 | append | `build/samirdml/golden/append/GOLD.OUT`; `build/samirdml/golden/append/BASE.DBF`, `build/samirdml/golden/append/IMPORT.DBF`, `build/samirdml/golden/append/INPUT.TXT` | U5-28 |
| DML-09 | save | `build/samirdml/golden/save/GOLD.OUT`; `build/samirdml/golden/save/BASE.DBF`, `build/samirdml/golden/save/INPUT.TXT`, `build/samirdml/golden/save/VALUES.MEM` | U5-187 |
| DML-10 | restore | `build/samirdml/golden/restore/GOLD.OUT`; `build/samirdml/golden/restore/BASE.DBF`, `build/samirdml/golden/restore/INPUT.TXT`, `build/samirdml/golden/restore/VALUES.MEM` | U5-180/181 |
| DML-11 | copyfile | `build/samirdml/golden/copyfile/GOLD.OUT`; `build/samirdml/golden/copyfile/BASE.DBF`, `build/samirdml/golden/copyfile/COPIED.TXT`, `build/samirdml/golden/copyfile/INPUT.TXT` | U5-59 |
| DML-12 | type | `build/samirdml/golden/type/GOLD.OUT`; `build/samirdml/golden/type/BASE.DBF`, `build/samirdml/golden/type/INPUT.TXT` | U5-278 |
| DML-13 | rename | `build/samirdml/golden/rename/GOLD.OUT`; `build/samirdml/golden/rename/BASE.DBF`, `build/samirdml/golden/rename/RENAMED.TXT` | U5-175 |
| DML-14 | erase | `build/samirdml/golden/erase/GOLD.OUT`; `build/samirdml/golden/erase/BASE.DBF` | U5-121 |
| DML-15 | label | `build/samirdml/golden/label/GOLD.OUT`; `build/samirdml/golden/label/ADDRESS.LBL`, `build/samirdml/golden/label/BASE.DBF`, `build/samirdml/golden/label/CLIENTS.DBF`, `build/samirdml/golden/label/INPUT.TXT`, `build/samirdml/golden/label/LABELS.TXT` | U5-143 |
| DML-16 | run | `build/samirdml/golden/run/GOLD.OUT`; `build/samirdml/golden/run/BASE.DBF`, `build/samirdml/golden/run/INPUT.TXT`, `build/samirdml/golden/run/VERSION.TXT` | U5-185/186 |

Manual: Ashton-Tate *Using dBASE III PLUS*, local `initech-manuals-ref/dbase3plus/dBASE III Plus 1.1 (1986) Manuals/Using dBase III Plus.pdf`. Printed page U5-n is PDF page n+112; U6-7/8 are PDF 403/404. Rendered inspected pages for implemented behavior: U5-51 (PDF 163), U5-53 (PDF 165), U6-7/8 (PDF 403/404). The second launch also inspected every verb page in the ledger, U5-15 and U6-86/87.

## Second launch result

The shared ?/?? printer now preserves CR LF and numeric width/precision through
literal, arithmetic and memory-variable values. TYPE evaluates its Character
argument and returns U for an absent or invalid inner expression. LIST numeric
headings and dotted Logical fields are graded against independently re-minted
PROG-query, alongside the unchanged real mint-005 transcripts.

Fifteen bounded DML corpus cases pass host and QEMU comparisons of their exact
transcripts and generated files. Each has an engine mutant that is RED on host
and QEMU. The LF printer mutant differs at byte 11 (0x0a versus real 0x0d).

RUN remains RED. Its exact first difference is byte 11, got 0x31 versus real
0x0d. os/milton/loader.c:1193 returns LOADER_ERR_BUSY while g_load_active is set;
INT 21h EXEC is single-level in this clone, and samir_pal_t has no EXEC slot.
The requested test-zs24-exec passes, but proves two sequential subdirectory
launches. No unsafe nested loader handoff or synthetic VER output was added.

SAMIR.COM fits at 103551 bytes with zero x87 instructions. The target uses the
manual's standard 6000-byte memvar pool (Using U5-180), with reclaimed space
moved into the BSS owned-heap tail. Scratch and total owned-heap capacities are
unchanged. Both disjoint-region link assertions remain enforced. LTO preserves
soft-float helper entry points explicitly; the build uses the existing i123
size-optimization pattern. No fs/ source was changed.

## New real outputs (all paths are local)

The original DML-01..16 outputs and their manifest remain intact. No expected
output was edited by hand. The seven historical authored program .out files
were replaced by copies of the real process's output, including DOS EOF;
the established program-diff normalizer recognizes that EOF. PUBS-01's terminal
projection removes only the ALTERNATE file's final DOS EOF byte.

| Ledger row | Real output | Copied expectation |
|---|---|---|
| PROG-01 | `build/samirdml/prog-mint/expr/REAL.OUT` | `harness/diff/dbf_diff/xbase_prog_diff/golden/expr.out` |
| PROG-02 | `build/samirdml/prog-mint/funcs/REAL.OUT` | `harness/diff/dbf_diff/xbase_prog_diff/golden/funcs.out` |
| PROG-03 | `build/samirdml/prog-mint/exact/REAL.OUT` | `harness/diff/dbf_diff/xbase_prog_diff/golden/exact.out` |
| PROG-04 | `build/samirdml/prog-mint/flow/REAL.OUT` | `harness/diff/dbf_diff/xbase_prog_diff/golden/flow.out` |
| PROG-05 | `build/samirdml/prog-mint/proc/REAL.OUT` | `harness/diff/dbf_diff/xbase_prog_diff/golden/proc.out` |
| PROG-06 | `build/samirdml/prog-mint/query/REAL.OUT` | `harness/diff/dbf_diff/xbase_prog_diff/golden/query.out` |
| PROG-07 | `build/samirdml/prog-mint/mutate/REAL.OUT` | `harness/diff/dbf_diff/xbase_prog_diff/golden/mutate.out` |
| EXTRA-01 | `build/samirdml/extra-mint/LO0.TXT` | Label comma probe 0; original ADDRESS.LBL is DML-15 |
| EXTRA-02 | `build/samirdml/extra-mint/LO1.TXT` | Label comma probe 1; original ADDRESS.LBL is DML-15 |
| EXTRA-03 | `build/samirdml/extra-mint/LO2.TXT` | Label comma probe 2; original ADDRESS.LBL is DML-15 |
| EXTRA-04 | `build/samirdml/extra-mint/LO3.TXT` | Label comma probe 3; original ADDRESS.LBL is DML-15 |
| EXTRA-05 | `build/samirdml/extra-mint/LO4.TXT` | Label comma probe 4; original ADDRESS.LBL is DML-15 |
| EXTRA-06 | `build/samirdml/extra-mint/OUT0.BIN` | COPY FILE input length 0; outputs 0/512/512/512/1024/1024 |
| EXTRA-07 | `build/samirdml/extra-mint/OUT1.BIN` | COPY FILE input length 1; outputs 0/512/512/512/1024/1024 |
| EXTRA-08 | `build/samirdml/extra-mint/OUT2.BIN` | COPY FILE input length 511; outputs 0/512/512/512/1024/1024 |
| EXTRA-09 | `build/samirdml/extra-mint/OUT3.BIN` | COPY FILE input length 512; outputs 0/512/512/512/1024/1024 |
| EXTRA-10 | `build/samirdml/extra-mint/OUT4.BIN` | COPY FILE input length 513; outputs 0/512/512/512/1024/1024 |
| EXTRA-11 | `build/samirdml/extra-mint/OUT5.BIN` | COPY FILE input length 1000; outputs 0/512/512/512/1024/1024 |
| EXTRA-12 | `build/samirdml/extra-mint/SCOPE0.MEM` | PUBLIC zero / private declaring level metadata |
| EXTRA-13 | `build/samirdml/extra-mint/SCOPE1.MEM` | PUBLIC zero / private declaring level metadata |
| PUBS-01 | `build/samirdml/extra-mint/SALARY.OUT` | `pubs/examples/samir/salary.txt` |

Reproduction: make mint-samir-prog, make mint-samir-dml-extra. Input programs,
captures and expected output copies are pinned in
harness/diff/dbf_diff/xbase_prog_diff/prog-real.sha256. Per-run sha256.txt files
also live in the mint directories. The extra mint requires the original DML-15
CLIENTS fixture and CONFIG.DB. No writes to the sister repository were made.

## Bounded coverage and remaining work

GREEN means the named, independently minted corpus, not the entire language.
INDEX covers non-UNIQUE C/N/D expressions through the existing NDX builder;
SORT covers one ascending field; TOTAL covers the documented consecutive-group
case; APPEND FROM covers DBF name matching, different field order, and per-row
persistence. Its adapter never rolls back earlier appended rows. A new real
partial-I/O/failure transcript was not minted. Memo import, import FOR/SDF/
DELIMITED and broader export scopes/options remain ungraded.

SAVE/RESTORE cover scalar N/C/D/L, replacement and ADDITIVE. LIKE/EXCEPT,
additional scope combinations and malformed-file/error-prompt parity remain
ungraded. New file commands reject protected existing output under SAFETY;
the exact interactive overwrite prompt is not implemented. LABEL FORM covers
an existing independently minted LBL with all records and TO FILE; SAMPLE,
printer and scoped variants remain ungraded. Macro-generated control blocks,
recursive/error macro parity and external procedure resource lifetime remain
ungraded. CLOSE PROCEDURE/FORMAT/ALTERNATE are harmless for resources that this
engine does not open. Full u78k and the three P3 siblings are not closed.

No git metadata, commits, pushes, CLAUDE.md, HANDOFF.md or beads were edited.
All QEMU runs use INITECH_QMP_STDIO=1. No make test or Bochs run was invoked.

## Acceptance evidence

Detailed current target/byte/mutant results are in samirdml-acceptance.md.
Primary logs: build/samirdml/final-refactor-host.log (only RUN is RED),
acceptance-emu.log (only RUN is RED), final-shared-printer.log (image, printer,
boot/write/Y2K, locked purge and Finder), final-host-recheck.log, and
 affected-unit-green.log (TYPE/flow/report regressions and pubs replay).

PROG-07b: `build/samirdml/prog-mint/mutate/PARTS.records` is the independent dbf_ref.py projection of real `PARTS.DBF`; copied to `harness/diff/dbf_diff/xbase_prog_diff/golden/mutate.dbf.out`. Its bytes were unchanged.
