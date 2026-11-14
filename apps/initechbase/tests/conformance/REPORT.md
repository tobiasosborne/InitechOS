# InitechBase unusual-file conformance review

Branch: `t3/initechbase-file-format-conformance`. Product studied:
`09d52a32173911debc13035096135143fefb4b2c`; fixtures, captures and readers:
`f3222e17`. No product code, existing expected values, registration, STATUS.md,
shared factory code or other component was changed. Nothing was pushed, per
BRIEF-COMMON; the orchestrator owns landing.

36 cases executed on the unmodified host interpreter and on real dBASE III
PLUS 1.1: **15 match; 21 differ**, grouped into **16 issues: one P0, fourteen
P1, one P2**. All host runs completed within the five-second per-case bound;
all real captures completed. No case expectation remains pending. The new
opt-in differential is deliberately red; these are test materials for the
product owner, not fixes or promotion into the accepted green selection.

## Evidence and method

Everything needed for replay is in `apps/initechbase/tests/conformance/`:

- `cases/Cnn/`: small DBF/DBT/NDX/program files and exact `MINT.PRG` commands.
  `cases.tsv` spells out input geometry and command sequences. `fixtures.c`
  regenerates the exact inputs using independent format geometry, without
  product headers or codecs. A fresh regeneration compared byte for byte.
- `reference/Cnn/`: actual III+ alternate output, original/copy DBF readback,
  memo/index readback where present, driver exit status, VGA dump and text.
  `reference/SHA256SUMS` pins all 227 capture files; `INPUTS.sha256` pins inputs.
- `observed/Cnn/`: current InitechBase output and disk bytes, never expected
  results. `evidence/differential.log` records every red/green observation.
- `PROVENANCE.md` and `reference-runtime.sha256`: exact runtime/media/driver
  hashes, source documents and environment. Prior accepted captures were read,
  particularly `tests/xbase_prog_diff/do-file-mint/depth/GOLD.OUT`, but the 36
  case expectations were freshly captured rather than inferred from them.

DOSBox-X was absent. The unchanged independent `goldens/dataloss/drive.c`
drove genuine DOS 3.30 plus the real III+ runtime on QEMU 8.2.2, 486/8 MiB,
fixed RTC, fresh disk per case. It waited for DOS's completion marker before
mtools readback. No InitechOS kernel was used as an oracle. The 2 MiB host
harness uses the existing DOS-aware `w2_host.c` adapter and actual interpreter,
commands and codecs. This does not certify target-memory capacity or yielding.

Each session installs `ON ERROR ? 'ERR',ERROR(),MESSAGE()`, runs USE and the
case's navigation/read/edit commands, closes the work area and copies the
resulting DBF to SAVED.DBF. Writes happen to BASE in place; SAVED is a byte copy,
not a database export or an independently relinked memo family. After a failed
USE, `IF LEN(DBF())>0` skips data commands to avoid III+'s interactive file-name
prompt. Errors inside a successfully open work area still resume normally.

The comparator requires exact alternate bytes and compares DBF version, count,
field names/types/width/decimals and raw logical record bytes using an independent
Python reader. Date/reserved/RAM-address bytes, +1/+2 header padding and optional
physical EOF are normalized. Partial reads stay explicit; they never become
blank records in the reader. DBT comparison ignores reserved header bytes but
preserves next-free and content. Changed NDX files must independently decode to
the same key/record pairs as real III+. An unchanged rejected malformed index
can match by exact bytes. No oracle is derived from InitechBase.

## Case table

`Cnn` identifies both the input directory and the real/host observation directory.
The exact commands are `cases/Cnn/MINT.PRG`; see `cases.tsv` for compact recipes.
All rows have completed independent captures. “Same” includes saved DBF/memo/
index comparisons, not just matching text. Issue details follow the table.

| Case | File condition | Real dBASE III+ | InitechBase today | Result / issue |
|---|---|---|---|---|
| C01 | Intact C8/N6, 3 rows | Lists, bottom=3, updates row 3 | Same | GREEN |
| C02 | Optional DBF EOF removed | Reads and saves normally | Same | GREEN |
| C03 | Final row missing last 5 bytes | Reads partial row; bottom=3, numeric=0; writes replacement | USE refuses #15 | RED / initech-silaf |
| C04 | Header says 3; only 2 full rows exist | Opens; GO BOTTOM #4; recno=3; session saves | USE refuses #15 | RED / initech-silaf |
| C05 | Header cut inside first descriptor | USE #15; source copied unchanged | Same | GREEN |
| C06 | Header count=0; 3 physical rows remain | LIST blank; GO BOTTOM exposes first physical row A/1 | Heading printed; blank key/0 | RED / initech-24whh |
| C07 | Header count=2; 3 physical rows remain | Lists 2; bottom=2; updates row 2 | Same | GREEN |
| C08 | Last row deleted; DELETED ON | Lists 2; bottom=2; updates row 2 | Same | GREEN |
| C09 | 0D 00 descriptor terminator | Reads true offset and saves valid table | Same meaningful data | GREEN |
| C10 | Memo pointer=1, missing sibling | USE #41 `.DBT file cannot be opened.` | USE #29 `File is not accessible.` | RED / initech-16kuo |
| C11 | Memo pointer=0, missing sibling | Still USE #41 | USE #29 | RED / initech-16kuo |
| C12 | Empty DBT | Opens; memo blank; edits intact KEY | USE #15 | RED / initech-2m9do |
| C13 | Header-only DBT; pointer=1 | Empty memo; LEN(memo) #34 | Empty memo; LEN returns 0 | RED / initech-mdn3i |
| C14 | 12 memo bytes, no terminator | Displays `memommmmmmmm`; LEN #34 | Memo blank; LEN=0 | RED / initech-qmkp5, initech-mdn3i |
| C15 | Single trailing 1A | Displays full short memo; LEN #34 | Same memo display; LEN=12 | RED / initech-mdn3i |
| C16 | 511-byte memo; terminator crosses block | All text, 550 padded content columns; LEN #34 | Only 492 content columns; LEN=511 | RED / initech-7vdl8, initech-mdn3i |
| C17 | 2,300-byte memo | All 2,300 content columns; LEN #34 | Only 492 displayed; resolved length=2,048 | RED / initech-7vdl8, initech-mdn3i |
| C18 | Memo pointer beyond next-free | Empty memo; LEN #34 | Empty memo; LEN=0 | RED / initech-mdn3i |
| C19 | Indexed key externally changed A->Z | Old SEEK lands at row 1/Z; REINDEX makes Z last | Same, including rebuilt keys | GREEN |
| C20 | Appended row absent from NDX | Old bottom=2; REINDEX restores bottom=3 | Same | GREEN |
| C21 | DBF shortened to 2; NDX still has recno 3 | EOF #4 on old bottom; REINDEX repairs; edits row 2 | USE/REINDEX #114; wrong SEEK; later edits row 1 | RED / initech-v6zx7 (P0) |
| C22 | NDX leaf cut to 600 bytes; used entries present | Reads all 3 entries; REINDEX repairs | USE #15; repair not reached | RED / initech-litwz |
| C23 | 128 C1 fields | Last field readable/editable; last LIST cell unpadded | Same table bytes, three extra trailing display spaces | RED / initech-xg1dd |
| C24 | 129 fields | USE #15 | Same | GREEN |
| C25 | Exactly 4,000-byte record | Reads and edits last field | Same | GREEN |
| C26 | 4,001-byte record | Also reads and edits this supplied file | Same | GREEN |
| C27 | C254 field | Width=254, full LIST and save | Same | GREEN |
| C28 | Nonstandard D10 raw `1992030200` | Displays date; REPLACE keeps extra `00` | Same display; REPLACE changes tail to spaces | RED / initech-s4ph1 (P2) |
| C29 | Correct D8, 2000-02-29, CENTURY ON | Ten-character date display; valid saved D8 | Same | GREEN |
| C30 | 254-byte program line | LEN prints 245; next line executes | Same | GREEN |
| C31 | 255-byte program line | LEN prints 246; next line executes | #18; entire child program skipped | RED / initech-uf3f9 |
| C32 | 24 nested IF blocks | INNER executes; caller resumes/saves | Same | GREEN |
| C33 | Recursive disk DO with DBF/alternate open | D=12, #6 too many files; unwinds/saves | D=19, #103 nesting; unwinds/saves | RED / initech-b3lus |
| C34 | 513 active statements, short file | Executes LINE513, resumes/saves | #43; no child statement executes | RED / initech-8o3rs |
| C35 | Exactly 8,192 bytes of short lines | START and PROBE-END, resumes/saves | #18 on file size; no child execution | RED / initech-8ux6b |
| C36 | Semicolon-continued expression | LEN prints 245; marker executes | #36 and #16 on split physical lines | RED / initech-affo8 |

The D10 row is deliberately nonstandard. III+ stores dates in eight YYYYMMDD
bytes; the requested ten-digit boundary is correctly represented by the
CENTURY display case, C29. Likewise the 4,001-byte and 255-byte results establish
what USE/DO actually accept here; they do not claim CREATE/editor limits or a
new maximum. The disk-recursion result depends on this open-file environment,
not a general claim that III+ has a twelve-level DO cap.

## Filed defects and responsible code

All IDs were created from the main checkout after searching the existing tracker,
with `component:apps-initechbase` and `review-2026-11-14`. Full reproducer,
independent source, both results and code locations are in every issue.
Paths below are relative to `apps/initechbase/`, at the studied commit.

| ID | Priority | Responsible code | Concrete defect and evidence |
|---|---|---|---|
| initech-v6zx7 | P0 | `cmd/nav.c:192`, `cmd/nav.c:339`, `cmd/mutate.c:1056` | C21 materializes old index entries into only `nrec` slots; physical-key reads during REINDEX consult that invalid old sequence. Repair fails. After the reported errors the installed ON ERROR handler continues, and REPLACE overwrites intact row 1 (VALUE 1->9), whereas real III+ repairs and changes row 2 (2->9). Independent DBF/NDX readback proves the wrong-row write. This P0 is scoped to resumed execution after errors, not a claim of silent or structural corruption. |
| initech-silaf | P1 | `fs/dbf.c:442` | Global minimum-record-area check refuses C03/C04 before reading recoverable records. Real III+ opens them, exposes available contents and distinguishes partial final data from end-of-file. |
| initech-24whh | P1 | `cmd/nav.c:303`, `cmd/workarea.c:861` | C06 zero-count shortcuts synthesize blank fields rather than the captured first physical record after GO BOTTOM. LIST also emits an extra heading. No fabricated count-repair expectation. |
| initech-16kuo | P1 | `cmd/workarea.c:491`, `ibase_main.c:367` | Missing DBT codes cross the DBF error-number mapper and become #29 instead of real #41 (C10/C11). |
| initech-2m9do | P1 | `fs/dbt.c:158` | Requiring a full physical header block prevents C12 from opening intact nonmemo data; real III+ opens and edits it. |
| initech-mdn3i | P1 | `core/fn_builtins.c:105`, `core/fn_builtins.c:2495`, `core/fn_builtins.c:236` | Generic character signature accepts M for LEN, returning a length where real III+ raises #34 (C13-C18). No extrapolation to other functions. |
| initech-qmkp5 | P1 | `fs/dbt.c:591`, `cmd/workarea.c:898` | Unterminated C14 text is rejected at physical EOF and converted into an empty memo. Real III+ displays its twelve bytes. Original DBT survives; visible loss of access, not overwritten memo data. |
| initech-7vdl8 | P1 | `cmd/query.c:903`, `cmd/query.c:200`, `cmd/workarea.c:903` | C16/C17 ? output silently saturates one 512-byte buffer, leaving 492 content columns. C17 also hits the separate 2,048-byte resolve cap; both caps need attention. DBT bytes remain intact. |
| initech-litwz | P1 | `fs/ndx.c:334`, `fs/ndx.c:388` | Physical multiple-of-page and page-count checks reject C22, although all used node entries exist and real III+ reads/rebuilds it. |
| initech-xg1dd | P1 | `cmd/query.c:592`, `cmd/query.c:607`, `cmd/query.c:698` | C23 pads the final C1 value to its four-character heading width; real omits the final three spaces. Saved table contents match. |
| initech-s4ph1 | P2 | `fs/dbf.c:1640` | C28's extra nonstandard date-slot bytes are space-filled; real preserves `00`. Same visible date, malformed-width input, no valid date lost. |
| initech-uf3f9 | P1 | `cmd/flow.c:64`, `cmd/flow.c:2037` | Preclassification 254-byte copy bound refuses C31, which real III+ executes. |
| initech-b3lus | P1 | `cmd/proc.c:589`, `cmd/proc.c:138` | Eager closing of disk program handles avoids real C33 file exhaustion and reaches a different error/depth. Existing independent library-recursion control still reaches 19/#103 in III+. |
| initech-8o3rs | P1 | `cmd/flow.c:56`, `cmd/flow.c:581`, `cmd/flow.c:1984` | Fixed logical-line cap refuses C34 as #43 before execution; real runs it. Current README documents this clone limit; the owner must decide its compatibility treatment. |
| initech-8ux6b | P1 | `cmd/proc.c:115`, `cmd/proc.c:560` | C35 is rejected by total byte count with a line-length message, although every line is short. Current README documents the 8191-byte clone limit. |
| initech-affo8 | P1 | `cmd/flow.c:554` | Physical-line splitting never joins C36's trailing semicolon, generating two errors for one valid expression. |

No hang was reproduced. No new case proves structural destruction of a good
DBF; C21's P0 is a wrong-record overwrite after handled errors. Missing/short
memo and damaged-file refusals were not inflated into data-loss claims. The
ordinary cases absent from this table remain outside this study's coverage.

## Verification

Exact commands and retained logs:

1. `sh apps/initechbase/tests/conformance/build.sh`: C11 fixture generator,
   existing host object set, new observation runner built successfully.
2. `sh apps/initechbase/tests/conformance/run-host.sh`: 36 process exits=0,
   no five-second timeout or crash, complete alternate markers.
3. `sh apps/initechbase/tests/conformance/capture.sh ABSOLUTE_EMPTY_DIR`:
   completed all 36 real captures. Corrected C01-C04/C35 inputs were recaptured;
   only matching final inputs passed retain.sh. No pending case promoted.
4. `sh apps/initechbase/tests/conformance/check.sh`: expected exit 1;
   15 GREEN cases, 21 distinct RED cases. `evidence/differential.log` includes
   original/saved DBF and rebuilt index mismatches, not just text differences.
5. `sh apps/initechbase/tests/conformance/qualify.sh`: exit 0. Unchanged C01
   passed; changed date/reserved/RAM-address bytes still passed; changing a
   SAVED.DBF character was rejected as `RED C01 meaningful SAVED.DBF bytes`;
   hash-pinned but incomplete reference was rejected as `PENDING C01`.
   Logs are in `evidence/{baseline,normalized,corrupted-record,pending}.log`.
   No deliberately wrong code was put in product sources.
6. Fresh fixture regeneration + recursive diff, shell syntax checks and
   staged whitespace check: passed. Raw DOS programs/transcripts/VGA captures
   are explicitly binary Git inputs so intentional CRLF/padding is preserved.
7. `nice make -j2 -C apps/initechbase test`: all 75 assertions suites passed,
   but elapsed 51s exceeded the unchanged 30s budget; command failed.
   With `TEST_TIMEOUT_S=60 TEST_BUDGET_S=120`, all 75 suites passed in 43s.
   Complete compressed logs are `evidence/component-{default,relaxed}.log.gz`.
8. `nice make -j2 test`: all 19 registered components ran; 773s, six components
   red on this heavily shared machine. Kernel, 123, Base and DOS had passing
   suites but exceeded their budgets; Word's codes test hit 20s and its suite
   budget; Turbo self-protocols/self-generation failed with empty entry logs
   under the 20s wrapper, and its suite exceeded budget. No unrelated source
   was changed. Full log: `evidence/top-default.log.gz`.

The relaxed top-level rerun and its final result are recorded in the addendum.
No smoke was required or run: no kernel, Toolbox, interface, product source or
product-image smoke driver was changed. Reference QEMU runs are independent
capture operations, not InitechOS smoke results.

## Captures still needed and owner handoff

**None for these 36 expected cases.** Every case has its original bytes,
commands, real III+ output and disk readback, including beyond-limit probes.
The product owner can select an issue's cases, see them red now and implement
a fix independently. Cases with documented clone limits are disclosed as such;
this review does not change GOLDEN-MASTER scope or the omission inventory.

Two additional qualifications were not performed and are not claimed:

- **Real reopening of host-written families.** To capture it exactly, take a
  new copy of `cases/Cnn`, replace BASE.DBF with the corresponding
  `observed/Cnn/BASE.DBF` (also BASE.DBT and OLD.NDX when present), and replace
  MINT.PRG with: SET TALK OFF; SET SAFETY OFF; SET ALTERNATE TO REOPEN.OUT;
  SET ALTERNATE ON; ON ERROR ? ERROR(),MESSAGE(); USE BASE [INDEX OLD for
  C19-C22]; IF LEN(DBF())>0; LIST OFF; GO BOTTOM; ? RECNO(); ENDIF; USE;
  SET ALTERNATE TO; QUIT (CRLF commands, fresh media/runtime/config as in
  capture.sh). Capture REOPEN.OUT plus table/index readback. Run C01,C09,
  C17,C21,C23,C25,C28,C29 first. This adds a direct consumer proof to the
  independent byte/reader comparison, not a missing expected value here.
- **Product image/target memory/yielding.** Replay each MINT.PRG as DO MINT
  on the owner's standard InitechBase TEST disk, collect RESULT.OUT and
  BASE/SAVED/DBT/NDX using mtools, compare to the same real references, with
  an external watchdog. Start with C03,C12,C17,C21,C25,C33-C36. The host
  heap does not stand in for the shipped heap or a cooperative-scheduling
  guarantee. Component/factory owners must supply any additional image
  orchestration; this test-only branch does not edit shared places.

STATUS.md remains for the orchestrator's rewrite. Tracker issues are open;
none was closed by the reviewer. Main and origin/main were unchanged from
the studied base when the fixture commit was made, so no rebase was needed.

## Final verification addendum

`nice make -j2 test TEST_TIMEOUT_S=120 TEST_BUDGET_S=240` finished successfully:
**registered 19, run 19; elapsed 308s; ALL GREEN**. Kernel ran its 91 suites in
163s; Base its 75 in 34s; Word its 13 in 63s; DOS its 32 in 51s; Turbo/compiler
its 40 in 87s, including both previously timed-out self entries. Full retained
log: `evidence/top-relaxed.log.gz`. Only command-line limits were relaxed;
no timeout/budget configuration or unrelated product source was changed. This
is an override-qualified green, not a claim that default quiet-machine timing
has been requalified. The orchestrator still owes its ordinary quiet gate.

Final comparator replay reproduced the exact committed differential log:
15 matching cases, 21 differing cases, expected exit 1. The independent
reader/completion controls passed again. Both capture and host observation
entry points now reject an empty/unknown CASES selection (exit 2), and capture
returns failure if any selected driver run fails; the retained oracle bytes
are unchanged. `evidence/no-{host,capture}-cases.log` records those controls.
Fixture regeneration, shell syntax and Git whitespace checks passed. Main and
origin/main remained at `09d52a32`; no rebase was necessary. Working report is
also written to the requested P2 `review-initechbase.md` path.
