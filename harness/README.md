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

The audit rules are mechanical:

- Every concrete test-* target in `make -pq` belongs to exactly one family.
  Pattern rules and unexpanded define bodies are not concrete targets.
  Mutant, emulator and Bochs twins share their base gate's family. Duplicate
  declarations, including overlapping globs inside one family, fail.
- Every tracked path under os/, harness/, spec/, seed/, tools/ and pubs/,
  plus Makefile and every .mk file, matches at least one family. A changed
  path in that scope must match even if it was deleted or is not tracked yet.
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
make database with Bochs definitions exposed, after all gate-vector
assignments. `test-gate-map` audits that full inventory and `git ls-files`.
The Rule-6 gate uses `GATE_MAP_MUT=drop-family` and
`GATE_MAP_MUT=duplicate-gate` to create JSON copies under build/, then
requires the real audit to fail for the named reason. It never edits spec/.
The include must stay after the TEST_*_GATES assignments and before
`include harness/gate_pairs.mk` so both new gates reach the final vectors.
