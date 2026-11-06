# harness/ — the oracle + emulator factory (C)

Everything that builds and *judges* the artifact, written in **C only**
(CLAUDE.md Law 3, PRD §14). The emulator is the fitness signal: nothing
ships on "looks right" — every subsystem is gated by a mechanical oracle
here (PRD §8, §9).

Layout:

- `emu/` — emulator drivers: `qemu.c`, `bochs.c`, `box86.c`. Wire the gdb
  stub, QMP `screendump`, serial markers, and the tri-emulator differential
  (CLAUDE.md Rule 5, PRD §8).
- `ssim.c` — per-window SSIM fidelity *guide* against the frame fixtures.
  A guide that points agents toward fidelity, **not** a hard numeric gate
  (CLAUDE.md Law 4, PRD §3).
- `diff/` — the differential suites: `fat_diff` (vs mtools/python),
  `dbf_diff` (vs real dBASE), `compiler_diff` (vs Free Pascal).
- `proptest/` — the ATKINSON region property suite + shrinker: the
  homomorphism, normal-form, and algebra-identity oracle (PRD §6.2).

`harness/factory_smoke.c` is the current placeholder proving the C factory
toolchain compiles and runs (beads `initech-tse`); it will be superseded by
the real harness components above.

Governed by: **PRD §8, §9, §10, §14.**

## QEMU diagnostic log storage (initech-tfn6r)

The QEMU driver writes the full `-d int,guest_errors,cpu_reset -D` log
throughout each run. It parses that intact log for the existing fault
signals before computing its verdict. The CLI removes only the `.qemu.log`
after an `OK=1` report; serial transcripts, screenshots and report cards
remain. An `OK=0` run (including a cap failure) keeps its complete log.
The library does not remove logs: callers doing their own grading own them.

`QEMU_LOG_CAP_BYTES` defaults to 67108864 (64 MiB). Make exports it as
`INITECH_QEMU_LOG_CAP_BYTES`; direct harness invocations can set that
environment variable too. Only a positive decimal byte count is accepted;
zero is an error, not an opt-out. An independent monitor checks every
20 ms, including while the parent waits for QMP/serial markers. If the log
exceeds the limit it kills QEMU and reports `RED QEMU-LOG-CAP`, retaining
every byte written. The threshold can be exceeded by writes during one
poll interval and host scheduling delay. A final check also catches a
short run that finishes between polls. No serial or disk file is capped.

Target-scoped private assignments in `emu/qemu_log.mk` give
`test-samir-forms-emu` and `test-samir-forms-emu-mutant` 536870912 bytes
(512 MiB): historical GREEN forms logs were 270-350 MB each. The
`test-samir-dml-%-emu` and `test-samir-dml-%-emu-mutant` patterns, plus
the direct DML callers `test-samir-abbrev-emu` and
`test-samir-print-emu-mutant`, get 2147483648 bytes (2 GiB): historical
GREEN DML logs reached 1.3 GB. Other targets retain the 64 MiB default,
including `ovl-lazy-leg-about` under `test-ovl-lazy-mutant`, whose 2.7 GB
`ovl_lazy_nosave_about.qemu.log` was a reset loop the cap must stop.

The default was based on a read-only survey of 1639 remaining build logs:
1220 had matching `OK=1` reports, and the largest of those was
`pubs/screens-check/tps-report-generator/tpsbak.qemu.log`, 9033431 bytes.
The screen capture budgets are 60 s (DOS) and 120 s (FLAIR); compiler
replays allow up to 240 s. The default gives over seven times the largest
surveyed GREEN log; the historical large logs above had already been
deleted and were absent from this survey. The largest surveyed log overall,
18576809 bytes, had `OK=0`
and was excluded from the GREEN measurement. No surveyed file was changed.

`make test-harness-qemu-log test-harness-qemu-log-mutant` exercises real
QEMU good/fault fixtures, a reboot loop during a QMP marker wait, a
short-lived writer, invalid overrides, and both Rule-6 glue mutants
(keep GREEN logs, disable the cap). It preserves the RED fixture artifacts
under a private `build/qlog-*` directory for inspection.

## Gate map

`spec/gate_map.json` is locked spec data for initech-fw3x0 (PRD Sec 8/9/14;
CLAUDE.md Rules 1, 6 and 8). It maps changed repository paths to mandatory
families of gates. Lanes and the PM use the same map and resolver:

```sh
make gates-for DIFF=main..HEAD
make gates-for FILES='os/samir/cmd/query.c spec/samir/error_codes.h'
make test-for DIFF=main..HEAD
make test-gate-map test-gate-map-mutant
```

`gates-for` accepts exactly one of DIFF (a git diff range) or FILES
(space-separated canonical repository paths). It prints only gate names,
one per line, sorted and unique. Git diffs disable rename detection, so both
old and new paths trigger their families. Deleted paths still participate.
An empty diff selects the `always` family. Paths outside the audited scope
select `always` too. Paths must be printable ASCII; FILES cannot encode a
path containing spaces (DIFF and the C tool's newline input can).

`test-for` resolves once, then runs the selected list in one `make -k -j16`,
retaining the existing gate/mutant ordering. Successful completion targets
count green gates; failed prerequisites count as red, even if no gate recipe
ran. It prints `>>> test-for: N gates, M red` and fails if any gate is red.
The selected list and completion records remain under a private
`build/gate-map-run.*` directory. Reproducibility gates always run at -j16.

The map derives its families from TEST_UNIT_GATES / TEST_EMU_GATES,
`harness/merge_samir.mk`, `harness/gate_pairs.mk`, and the subsystem recipes.
SAMIR includes the dbf/dbt/ndx/interp/xbase gates and its FLAIR legs; TPS
includes seed and compiler gates; i123 and iword retain their own host and
FLAIR legs. Opaque issue-named FAT/INT21 gates belong to MILTON according to
the C sources their recipes compile. Shared oracle, emulator, code generator
and spec inputs can trigger several families. Broad subsystem paths are
conservative, so some changes run additional gates. Aggregate entry points
(`test-unit`, `test-emu`, `test-par`, `test-merge-samir`, `test-for`) have
ownership but no automatic path triggers: concrete gates are selected from
their subsystem families, avoiding recursive selection of `test-for`.

Review fix for initech-fw3x0: FLAIR also covers `os/milton/**`. Any MILTON
kernel edit therefore selects the whole FLAIR family, including kernel DF
and native-boundary gates. This extra work is intentional: those gates link
the MILTON kernel. TPS explicitly covers `os/milton/tpsrep_program.asm`;
the harness family covers both `harness/proptest/serial_trace_fixture.c`
and `harness/proptest/tbx_trace_fixture.c`. Shared boot images and make's
global invalidation stamp expose further cross-family consumers. Their
tracked inputs are listed in the map, so changes to shared kernel, font,
definition-table and fixture inputs can select several subsystem families.
The rebased inventory also registers the SAMIR DO-file, TSR-exec, lane
certificate and merge-revert gates. The latter two belong to the harness
family, including their directly consumed proof/revert scripts.

The audit rules are mechanical:

- Every concrete test-* target in `make -pq` belongs to exactly one family.
  Pattern rules and unexpanded define bodies are not concrete targets.
  Mutant, emulator and Bochs twins share their base gate's family. Duplicate
  declarations, including overlapping globs inside one family, fail.
- Every tracked path under os/, harness/, spec/, seed/, tools/ and pubs/,
  plus Makefile and every .mk file, matches at least one family. A changed
  path in that scope must match even if it was deleted or is not tracked yet.
- Independently, each derived source/gate pair must select the consuming
  gate's own family (the always family is selected unconditionally). Merely
  mapping that source to another family does not satisfy this assertion.
  This stricter check does not rely on another family's incidental calls.
- Every family matches at least one real gate; every gate name/glob matches
  the inventory. New or removed test targets require a deliberate map edit.
- `always` owns test-makefile-vars, test-gate-honesty and test-gate-map,
  together with their existing mutants, and runs for every diff.
- `build-system` owns test-kernel-repro, test-kernel-flair-repro and
  test-ovl-repro and their twins. Makefile, all .mk files, tools/**,
  os/milton/** and os/flair/** trigger this family. The audit checks this
  requirement for each tracked path; resolution checks changed paths too.
- Malformed JSON, unknown schema members, duplicate keys, unknown gates,
  missing coverage and empty results fail loudly. There is no fallback
  family that silently accepts an unmapped source or gate.

Gate globs use POSIX fnmatch syntax. Path globs use the same component
syntax, with `*` confined to one component and `**` crossing directories;
`**/` also matches zero directories. The always family and aggregate entry
points have empty path lists. Keep map edits ASCII and cite their bead.

`build/gate_inventory.txt` is regenerated on every request from a query-only
make database (`make -p -q`) with Bochs definitions exposed, after all
gate-vector assignments. `test-gate-map` runs both the structural audit
(`--audit`) and the consumer audit (`--audit-consumers`) against that same
database and a fresh `git ls-files` snapshot. Source evidence is derived
without consulting the JSON paths: walk explicit normal and order-only
prerequisites transitively, including generated outputs and the global
makefile stamp, and collect tracked files under os/, harness/, spec/, seed/,
tools/ and pubs/. Also inspect recipe paths and follow recipe references to
file targets back to their inputs. Cycles terminate by visiting each rule
once per gate; parsed recipe evidence is cached across gates.

The C reader resolves named make variables, nested `$(call ...)` bodies
with numbered arguments, `foreach` bindings, addprefix/addsuffix, filter /
filter-out, patsubst/subst, basename/dir/notdir, firstword/words and strip.
Sort retains all words; it need not order them for set membership. Other
functions conservatively retain their expanded arguments (including both
conditional branches), and recipe path globs match tracked files. No
recipe or shell/eval/file function is executed by the reader. The green
line reports the checked pair count and conservative/unresolved expansion
count; it is not a claim that every make expression was evaluated exactly.

Derivation limits (changes outside these observations still need review):

- Only explicit rules in this make configuration's database are visible.
  Other conditional configurations, implicit/pattern-rule instantiation,
  target-specific variable overrides and make substitution references are
  not fully evaluated. Expansion nesting is bounded at 64.
- Shell-computed paths, dynamically assembled filenames and shell loops
  that concatenate path fragments cannot generally be recovered. Unknown
  make functions retain argument paths but may construct unseen outputs.
  Shell/eval/file results and unbound variables are not evaluated.
- File names containing whitespace or shell escapes are not tokenized as
  complete paths. External corpora, untracked/generated file contents,
  undeclared compiler includes and files opened inside scripts or programs
  are not inspected. Known generated file targets are followed through
  their declared prerequisites and visible recipe paths.
- Recursive make recipes are not executed or interpreted as gate calls;
  gate names in diagnostics must not create dependency edges. The five
  aggregate dispatchers (test-unit, test-emu, test-par, test-merge-samir,
  test-for) have structural ownership but no source-selection assertions;
  their concrete test gates are checked individually. They retain empty
  path lists to prevent full-suite selection and test-for recursion.
- Literal paths in diagnostics and unused conditional branches can produce
  extra obligations. This is deliberately conservative, not precise
  command tracing or proof of complete compiler dependency information.

The Rule-6 gate creates JSON copies under build/ with
`GATE_MAP_MUT=drop-family`, `duplicate-gate` and `drop-consumer`, then
requires the real gate to fail for the named reason. The third mutant
removes the harness serial fixture path: the old structural audit must
still pass, while the consumer audit must name the missing consuming gate.
It never edits spec/ and registers no additional test gate.
The include must stay after the TEST_*_GATES assignments and before
`include harness/gate_pairs.mk` so both new gates reach the final vectors.

## W4 acceptance inventory (initech-9nzpr, DEC-03a.10)

The ratified DEC-03a.10 table is locked in
`spec/samir/screen_regrade.json`: ten W4 groups, all `not-started`.
W3 GET/READ/@/generated-.FMT forms keep their existing byte-exact screens.
An inventory GREEN certifies the independent C minimum contract for every
app's L1 kind/tool pairs, L2 assertion/mutant pairs, retained names/dump paths,
and existing tool/reference paths. Evidence can grow; deleting a requirement
fails even when the resulting JSON has a valid shape. It does not certify an
unimplemented application. The JSON uses a
`_comment` provenance field so it remains plain, valid JSON.

Run `. tools/lane-env.sh`, set `DBASE3_DECOMP` to the sister repository, then
`make -j8 test-w4-inventory test-w4-inventory-mutant test-w4-files
test-w4-retained test-w4-trace-smoke test-w4-structure-mutant`. The file gate
canonicalises existing paths relative to the repository root, including the
default `../dbase3-decomp`. Its path regression runs that default with an
existing sibling in a private repo-shaped root, plus an absolute path and a
missing-reference control. Missing references fail with the explicit
`SKIP_DBASE3_DECOMP=1` shouting opt-out. These gates join the host/emulator
vectors; no per-app `test-w4-<app>` target is instantiated yet. Missing real
samples fail; no replacement samples or silent skips are manufactured.

The new C comparators are built as `build/{frm,lbl,qry,scr,fmt,config,prg,vue,
exact}_diff REFERENCE ACTUAL`. They return 0 for meaningful-byte equality,
1 for a difference or invalid supported format, 2 for usage/I/O failure.
They share factory-only `w4_file_diff.c`, and include no SAMIR code or tables.
All unknown durable fields remain significant. Normalization is exhaustive:

| Tool | Independent sister specification | Bytes ignored |
| --- | --- | --- |
| frm_diff | specs/file-formats/frm.md Sec 2-6 | `[0xE0+heap_bytes,0x68C)` and `[0x68C+12*ncols,0x7B6)` only: scratch gaps/unused column area. The uncertain USED-column byte +4 stays exact. Header, length/pointer tables, heap, used columns, footer stay exact. |
| lbl_diff | specs/file-formats/lbl.md Sec 2-4 | Inactive slots `[0x49+60*height,0x409)` only. Active expressions, their spaces, remarks and dimensions stay exact. |
| scr_diff | specs/file-formats/scr-and-fmt.md Sec 3.2-3.4 | Field-record alias/name bytes after their first NUL; descriptor address `[+0x18,+0x1C)`; runtime pointers `[+0x2C,+0x34)`; box trailer `[+0x0B,+0x13)`. Reserved bytes, flags, page, coordinates, masks and complete image remain exact. |
| qry_diff | specs/file-formats/qry.md Sec 2 | None. `pad` and `pad2` are HIGH bytes of u16 length/operator words, not disposable padding. |
| fmt_diff | specs/file-formats/scr-and-fmt.md Sec 2 | None. CRLF, whitespace, source case, trailing spaces and Ctrl-Z stay exact. |
| config_diff | specs/environment/config-db.md | None. Parses text framing, preserves every directive byte; does not infer unresolved directive semantics. |
| prg_diff | specs/file-formats/prg-and-aux-files.md Sec 1 | None. Source-text framing, OEM bytes preserved; linked/encrypted programs with NUL bytes are unsupported. |
| vue_diff | specs/file-formats/vue.md Sec 3-8 | None, including reserved/unknown fields. Only the observed single-format, single-relation dialect has a proven parse. |
| exact_diff | the caller's cited file/transcript specification | None; opaque bytes, also for unchanged re-save identity and backups. |

These are additional W4 comparisons, not changes to existing DBF/DBT/NDX
oracles or `dbf_normalization.json`. FRM/SCR reader normalizations follow the
cited file specs; their inferred pointer/scratch semantics are explicitly
limited to those specs. In particular, scr-and-fmt.md Sec 5 (Open questions)
labels the SCR field pointers (+0x2C/+0x30) and box trailer (+0x0B) as
inferred stale memory. It is NOT proven that real dBASE ignores arbitrary
values in them on reload. A future real-runtime mutant mint/load or OVL
load-code analysis must close that question. Retained bytes immediately
before these ranges (CLIENTS.SCR 0x2B and 0x22D) are proven RED controls.
A designer's unchanged real-file re-save must ALSO
pass `exact_diff` on the full file, including opaque bytes. Cross-runtime
REPORT/LABEL output still needs real III+ transcript equality. A .FMT generated
by a W4 painter still needs its W3 byte-exact screen. The file-tool tests use
pristine III+ FLT_INFO/TRAVELER/TRIP.FRM, ADDRESS.LBL, NOTPAID.QRY,
CLIENTS/TOURS/TRAVEL.SCR, CLIENTS/TOURS/TRAVEL/RESERVE.FMT, RESERVE.VUE,
ADD.PRG and shipped CONFIG.DB, plus the corrupt UNIVERS.FRM control.
Each tool first sees a meaningful-byte mutant, then the real sample self-pair.
Padding/stale-byte positives and neighboring meaningful-byte negatives pin
the normalization boundaries. Formats with no ignored bytes have no padding
GREEN: QRY's misleadingly named pad bytes are specifically proven RED.
These tests prove comparators; no SAMIR designer output is claimed yet.

`sh harness/diff/dbf_diff/w4_trace.sh <app> [SAMIR.EXE]` uses the existing
QEMU key runner, screen_capture's native console decoder, fresh data volume,
before/after recursive extraction, required named captures, and mandatory
per-app `CHECK`. Trace inputs live at
`harness/diff/dbf_diff/xbase_prog_diff/w4/<app>/{KEYS,CHECK,SETUP/}`.
Before KEYS, a private driver runs `DO SETUP`, as the real minter does, then
copies a 512-byte token to a fresh completion marker. Missing/invalid markers
fail before CHECK. Reserved driver/marker names cannot come from SETUP.
The marker is saved as `setup.ok` beside captures, outside extracted app files.
CHECK runs in `build/w4/<app>` with `W4_ROOT`, `W4_CASE`, `W4_OUT` exported.
It must grade L1 real-file/transcript effects and L2 independent assertions;
it must not return success on a missing golden, dump, tool or expected effect.
`KEYS` uses WAIT n (BIOS ticks at 55 ms), TEXT, KEY and DUMP; unsupported
text/keys fail explicitly. TEXT preserves case using Shift chords for uppercase
letters. It supports letters, digits, spaces, periods and double quotes;
extend and prove the mapping before a future trace needs other punctuation.
No directives or keys are silently ignored. DUMP yields both `.ppm` and
`.ppm.screen`; runner errors, faults, missing captures and CHECK failures fail.
An app lock refuses concurrent writes to the same evidence directory.

`w4_screen_check SCREEN ASSERTIONS` reads the existing screen format and
checks independent zero-based `TEXT row col literal`, `CURSOR row col`,
`ATTR row col length hex`, and `BLANK row col length` statements. Ordered
menu pads use several TEXT assertions at distinct columns. Field position
uses a TEXT/CURSOR pair; highlight/status changes use ATTR/TEXT pairs.
`--mutants SCREEN ASSERTIONS OUT` writes and re-reads a changed captured
screen for EVERY assertion, including each assertion class. CHECK should
also use `ppm_text_check --cells` (or a stronger existing raster checker)
to verify visible presentation. A future navigation lane must additionally
prove actual broken navigation/status/menu behavior, not merely tool mutants.

The `smoke` case exercises existing CLEAR + USE CLIENTS + LIST, checks all
file names/contents unchanged, the three independent fixture names, prompt
row 6/column 2, normal attribute and blank row, plus PPM ink/blank cells.
`test-w4-structure-mutant` proves text/cursor/attribute/blank assertions,
a changed DBF record, and a full QEMU replay with LIST removed turn RED for
the named reason. This is a harness smoke, not W4 app or Rule-14 acceptance.
The `mixed-case` smoke types `Ab` into a C(10) field and checks those exact
extracted DBF bytes. The `setup-prelude` smoke inherits CLIENTS opened by
SETUP and LISTs it without a USE in KEYS. Lowercasing TEXT and skipping the
setup prelude are each proven RED by a complete QEMU replay.

Future app lanes instantiate `$(eval $(call w4-acceptance,browse))` in a
make fragment included AFTER `w4.mk`. This defines `test-w4-browse` only when
the app lane exists. Supply the inventory's KEYS, CHECK, and every matching
`mutants/<assertion>.sh`; each mutant script returns success only after it
has demonstrated its named failing behavior. The macro fails closed on
missing files, runs CHECK and all declared mutant proofs, compares retained
screen dumps at matching DUMP basenames, then runs real
`record-flair SCRIPT=browse` and `record-flair-repro SCRIPT=browse` and requires
both nonempty clips. Register the corresponding record-flair script/image/
trace before record-flair's parse-time prerequisite use, following its existing
pattern. No substitute or placeholder clip is created. CONFIG.DB has no
editor: its KEYS/CHECK covers startup settings/transcripts/effects/return.
W5 must review complete evidence before changing any row's status; the macro
does not silently mark an app complete.

Retained reference minting uses the existing SCREEN.COM/screen_tool route:
`sh harness/diff/dbf_diff/w4_retained_mint.sh retained build/w4-real-help`,
or `create-retained` / `structure-retained` / `generated-form` with separate destinations.
The real runtime/help files and pristine generated form are read from
DBASE3_DECOMP; no sister files are changed. The startup license gets a delayed
DOSBox AUTOTYPE Enter before SCREEN.COM is installed. No -silent mode is
used. Fresh save markers, every declared dump, input/runtime hashes and two
independent byte-identical captures are required before publishing references.
`test-w4-retained` rechecks input/runtime provenance, capture identity and exact
screen character/attribute/cursor mutants. Golden UTF-8/CP437 `.screen` files
use the existing reversible capture format; the new factory sources are ASCII.
HELP captures are III+ navigation/layout references, clearly NOT authored
InitechBase text. The HELP implementation lane owes separately labelled
InitechBase-authored exact content goldens before app certification.
