# Parallel certificate -- sharded `make test` across worktrees

Written 2026-10-24 on the new WSL box (64 cores, 62 GB), bead initech-9bgg6
(implementation lane: Codex gpt-6.1-sol, lane/parcert). Supersedes the
"one full make test per host" line of bd memory host-discipline-2026-09-26
ONCE the acceptance below has been met; until then the serial certificate
stands.

## 1. The problem

The certificate is 1478 gates (792 host, 687 emulator at 69b855b8) run
strictly one after another: `make test` = `make test-unit` then
`make test-emu`, each a serial loop that stops at the first red. Measured on
this box (sweep of 2026-10-24, per-gate make, serial): about 5 h wall clock,
of which the emulator half is roughly 4 h at 20-60 s per QEMU boot and more
per Bochs leg. One gate uses one core. The consequence is cadence, not
coverage: the laptop PM re-ran families after each merge and never the whole
vector, and the first full run on this box found 30 reds, 26 of them main
regressions that had been sitting there for one to four days (HANDOFF
CURRENT STATE 2026-10-24; beads zxzi, u01i, buze, hk3py, kqr8, hztg).

## 2. Why it is serial, and which reasons still hold

| Reason | Holds on this box? |
|---|---|
| Two makes in one `build/` collide (shared intermediates, `build/dbsafe`, `build/pubs`, scratch images) | YES -- so parallelism must be across worktrees, never inside one |
| Concurrent QEMUs flaked injected traces on the 12-core / 30 GB laptop | Unproven here: the wave-2 harness waits on progress markers and key acks (`--keys-ack`, progress-keyed waits), which were built to remove load sensitivity; 64 cores make 8 QEMUs a light load |
| QEMU fixed ports | NO -- the gdb stub (`-s`, tcp::1234) is pushed only for `enable_gdb` (make run), never by a gate; QMP is `stdio` under `INITECH_QMP_STDIO=1` (otherwise a per-`--out` unix socket) |
| Bochs fixed RFB port 5900 | Serialised by design: `harness/emu/bochs.c` takes the host-wide `flock` on `/tmp/initech-bochs.lock` (bd memory d9tt; Makefile:11898 does the same for its own Bochs recipes). 118 of the 687 emu gates are `*-bochs`; they self-serialise and cost about 1 h of the wall clock in any layout |
| `/tmp` scratch collisions | Only the Bochs lock lives in /tmp; everything else is under `$(BUILD)` |
| Disk | `build/` after a full run is ~310 MB; 16 worktrees are ~5 GB, trivial against 200 GB free |
| RAM | a QEMU gate is 32-64 MB guest + host; 16 abreast is under 2 GB |

## 3. Design

`tools/cert-sharded.sh [-j N] [-r REF] [--out DIR] [--only unit|emu] [--resume]`

1. **Gate list** from the ONE source of truth: `make -pn` (no target: `make
   -pn test` recurses and prints the database once per sub-make) ->
   `TEST_UNIT_GATES` then `TEST_EMU_GATES`, deduplicated in order. The count is
   printed and must equal `$(words ...)` the Makefile prints in its banner.
2. **Workers = worktrees.** `git worktree add --detach <out>/w<k> <REF>` for
   k = 1..N (default N = 8; REF defaults to HEAD of the invoking checkout).
   Each worker is `make clean`-fresh (a certificate is a clean build), then
   warmed with `make factory image` in parallel so the kernels/tools are
   built once per worker rather than lazily inside the first gate.
3. **Work stealing, not static shards.** One queue file; each worker pops the
   next gate under `flock` on the queue, runs `INITECH_QMP_STDIO=1 make
   --no-print-directory <gate> > <out>/logs/<gate>.log 2>&1`, appends
   `PASS|FAIL <kind> <gate> <seconds> w<k>` under flock to `results.txt`.
   Durations are recorded so later runs can sort the queue longest-first
   (LPT) and shave the tail; the first run uses Makefile order, host gates
   first.
4. **Bochs gates** go to the queue last and are capped at one in flight by
   the existing host lock; a worker that would block on the lock takes the
   next non-Bochs gate instead (`flock -n` probe). So the Bochs hour
   overlaps the QEMU hours instead of adding to them.
5. **Fail handling, no silent retries.** When the queue is empty, every FAIL
   is re-run ONCE, alone, by the last worker standing, with everything else
   idle, and recorded as `FAIL-CONFIRMED` or `FLAKY` with both logs kept.
   FLAKY is a red result for the certificate (Law 2) AND a bead: a gate
   that passes alone and fails under load is a timing bug in the harness or
   the OS, never noise to be averaged away.
6. **Verdict.** `ALL GREEN -- H host + E emu gates, N workers, T min` only
   when every gate is PASS; otherwise the list of FAIL-CONFIRMED and FLAKY
   with their log paths, exit 1. A `--diff OTHER/results.txt` mode prints
   gates whose verdict differs between two runs (the serial-vs-sharded
   check of Sec 4, and any two HEADs).
7. **Cleanup.** `git worktree remove --force` for every worker unless
   `--keep`; the out dir keeps results and logs.
8. **No oracle changes.** No gate, Makefile recipe, or timeout is touched.
   The runner is a factory tool in thin shell (CLAUDE.md Build & test: "C +
   make + thin shell"); Law 3.

## 4. Acceptance (what makes this the certificate)

- Same HEAD, same box: the sharded run's verdict set equals the serial
  sweep's for every gate (the 2026-10-24 serial sweep at 2808f8b5 plus the
  serial family re-runs on main are the control; a second serial sweep at
  69b855b8 was started and is kept running until the first sharded run can
  be diffed against its overlap).
- Zero FLAKY at N = 8 across two consecutive full runs. If a gate is FLAKY,
  it gets a bead and the runner is not yet the certificate for that gate's
  family; the rest still counts.
- Wall clock under 1 h at N = 8 on this box (expected ~40 min: 4 h of QEMU
  work / 8 plus the Bochs hour overlapped).
- `tools/bootstrap/doctor.sh` unchanged; BOOTSTRAP Sec 4 and HANDOFF Sec 6
  name the new runner as the certificate command; bd memory
  host-discipline updated by the PM.

## 5. What it does NOT do

- It does not parallelise inside a worktree (`make -jN test` is unsafe: the
  recipes share scratch paths under `build/`).
- It does not change the one-Bochs-at-a-time rule.
- It does not replace the family re-runs after a merge; it makes the full
  vector cheap enough to run after every wave and before every push of a
  kernel-growing lane.
