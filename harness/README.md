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
