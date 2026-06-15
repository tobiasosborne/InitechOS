# WL-0026 -- WL-0025 follow-up tranche: CREATNEW oracle/robustness, IOCTL minors, absdisk spec/emu, fault-injection, oracle hardening

Branch: `command-com-default`. Discharges the follow-up debt filed by WL-0025
(the INT 21h/25h/26h parity tranche). Orchestrated from the main thread as TWO
background workflows + one focused fix subagent, with the orchestrator owning
all re-verification (Law 2) and commits.

## Orchestration shape (what was delegated vs owned)

- **Workflow 1 (coding tranche):** 7 logical work-items landed by one Opus
  subagent each, **strictly serial** (shared kernel files -- Rule 3 / WL-0023-25),
  each followed by **3 parallel adversarial verifiers** (correctness vs ground
  truth / does-the-oracle-bite / scope-fidelity+vtable-landmine). 28 agents total.
- **Workflow 2 (forward grounding):** 7 READ-ONLY briefs for the forward
  capstone-path features (FAT16 z01, wildcard 80k, break 4tw/er3h, CON-read x8fs,
  INT24-wire mvg, streaming dao, MBR-partitions kzfs) -- pre-grounding the next
  tranche; ran in parallel with the coding tranche.
- **Fix subagent:** the three oracle-quality findings below.
- **Orchestrator-owned (NOT trusted to agent self-reports):** every `make clean &&
  make test` re-run, the non-ASCII scan, the verifier-mutation cleanup audit, the
  cnvp manual mutation re-check, and all commits/bead-closes.

## What changed (each landed green, mutation-proven, adversarially verified)

- **`initech-nmpo` + `initech-glsw` (CREATNEW oracle + robustness).** Strengthened
  the AH=5Bh host oracle (FS-effect assertions: no-truncate-on-collision +
  no-SFT/JFT-churn + materialize-on-fresh) and added an end-to-end FAT12
  differential (`test-nmpo`: real fat_create + dir-scoped probe vs mtools+python).
  glsw added direct AH=5Bh assertions for the null/no-backend/exhaustion arms and
  made `open()==NULL` a fail-loud hard error at the do_creatnew existence guard
  (was an `if (open != 0)` silent-skip -- the OPPOSITE of the CREATNEW contract).
- **`initech-4nbn` (IOCTL AH=44h minors).** Corrected the device-info bit-NAME
  labels in int21.h to the PRM char-device layout (DEAD macros, no behavior
  change) and implemented AL=01/06/07/08 as thin SFT-derived answers; locked the
  contract in int21h_calling_convention.json. ~16 new oracle cases + 4 mutants.
- **`initech-lpf3` (fault-injection infra).** Added a fail-the-Nth-write seam to
  the blockdev_file harness backend + `test-fat-fault-rollback` (35 checks, 3
  mutants) driving the fat12 create/write/mkdir rollback paths RED -- the infra
  the m0bp adversarial review flagged as missing.
- **`initech-8403` (absdisk asm-path emu self-test).** `test-absdisk-emu`: a baked
  guest issues real `int $0x26` WRITE then `int $0x25` READ on a SAFE scratch LBA
  (total-1) through the live IDT trap gates, closing the int25_entry/int26_entry
  asm-round-trip coverage gap (the host oracle drives the dispatch C-funcs only).
- **`initech-cnvp` (locked-spec completeness).** Added the DEC-15 bad-buffer
  (0x0F/0x0C) error row to `spec/absdisk_int2526.json` and strengthened test-spec
  with an error_codes set-equality completeness assertion.
- **`initech-isil` (RENAME coverage).** Added a non-root same-dir AH=56h success
  leg (\SUB\OLD -> \SUB\NEW.BAK) to test-gnrc + a 4th rename mutant.
- **`initech-5o6o` (deviation doc).** Recorded the CHMOD SET dir/vol-label reject
  as an intentional, operator-approved Law-4 fail-loud deviation in ADR-0003
  DEC-14.2 + a comment at the do_chmod reject site.

## Oracle hardening (orchestrator-driven, post adversarial review)

The verifiers earned their keep -- three Rule-6 "decoration" findings on a GREEN
suite, all fixed by strengthening (never weakening) the oracle:

- **cnvp:** the new error_codes completeness assertion's "missing-row" branch was
  DEAD -- the AL/AH value loop ran first and `KeyError`'d on a deleted row before
  the assertion fired. Reordered set-equality BEFORE the loop + made the loop
  `.get`-tolerant. Re-verified by hand: deleting `bad_buffer` now yields
  `error_codes drift -- missing ['bad_buffer']` (not KeyError).
- **5o6o:** ADR DEC-14.2 cited a mutant (`INT21_MUTATE_CHMOD_NO_CX_REJECT`) that
  NO Makefile target built -- a false oracle citation in a Ratified document.
  Wired the real MUTANT 8 into test-b53d-mutant (now 8 mutants); it bites BOTH the
  Directory- and VolLabel-bit dispatch-edge legs independently (the VolLabel leg
  was retargeted to a separate plain file so the backend TARGET reject can't mask
  the dropped dispatch guard -- the adversary was right about the masking).
- **nmpo:** the new headline gate `test-nmpo` had no standing mutation target.
  Added `test-nmpo-mutant` (reuses the inert `INT21_MUTATE_CREATNEW_NO_GUARD`
  hook): a CREATNEW-degenerates-to-CREAT mutant truncates the materialized file
  and the mtools/python differential goes RED.

## Acceptance

`make clean && make test` GREEN: **90 host + 27 emu gates** (was 85+26 at WL-0025
close; +5 host = test-nmpo, test-fat-fault-rollback(+mutant), test-4nbn-mutant,
test-nmpo-mutant; +1 emu = test-absdisk-emu). Re-verified twice from a clean tree
on the MAIN thread (once before, once after the oracle-hardening fixes). All
mutants confirmed RED-for-the-right-reason (incl. b53d MUTANT 8 both legs, the
nmpo NO_GUARD mutant, the lpf3 rollback mutants). Non-ASCII scan clean (code);
spec byte-intact after the manual cnvp re-check.

## Frictions / lessons

- **Adversarial verification caught real Rule-6 decoration on a green suite,
  three times** (cnvp dead branch, 5o6o phantom-mutant ADR citation, nmpo
  unmutated headline gate). The WL-0025 lesson holds: a green, "mutation-proven"
  suite can still hide an oracle that never bites; the riskiest path must be
  exercised AND the proof independently re-run.
- **Verifier agents mutated the shared working tree** (revert/restore + perturb
  cycles), leaving it transiently inconsistent (cnvp's verifier saw the spec
  oscillating between intended/deleted/0x99 states; an 8403 verifier observed its
  own mid-revert kmain.c as "absent" and reported a P0 that was a race artifact).
  LESSON: in a shared-tree workflow, treat ALL agent self-reports as untrusted;
  the orchestrator's clean re-run is the only truth (Law 2). For future tranches,
  give mutation-running verifiers an isolated worktree or forbid tree mutation.
- **`git checkout <file>` destroys uncommitted tranche state.** The fix subagent
  reverted the spec mid-mutation and lost the (uncommitted) bad_buffer row, then
  reconstructed it. The orchestrator's manual re-check used a `/tmp` backup to
  restore, never `git checkout`. Commit-per-landing would have avoided this.
- **8403's emu gate is flaky under CONCURRENT QEMU relaunch** (serial_len=0 /
  cpu_reset at EIP=0) but deterministic-green in the serial `make test`. Filed
  `initech-fgdz` to harden the serial-capture race (mirror test-vect). Not
  blocking -- it is green the way the gate actually runs.

## Pointers / next

Epic `initech-bsy` advanced; the Appendix-A INT 21h surface debt from WL-0025 is
discharged. Follow-up beads filed (all P3, non-blocking): `initech-fgdz`
(8403 serial-capture race), `initech-jplu` (4nbn AL=08 BL=drive fidelity + AL=01
invented DX output), `initech-aaan` (lpf3 mkdir-EOC-fail leg + scenario-[A]
on-disk assertions), `initech-815i` (nmpo host leg-(e) 0x0003 mock artifact).

FORWARD TRANCHE (pre-grounded this session, dependency-ordered, another SERIAL
kernel lane): `dao` (streaming walk; unblocks z01) -> `z01` (FAT16 read-only;
initrd deferred) -> `80k` (wildcard) -> `x8fs` (cooked CON read; before 4tw) ->
`4tw`+`er3h` (break) -> `mvg` (INT 24h wiring; consumes lpf3). TWO operator gates
remain (ADR-by-committee draft -> ratify, per the DEC-15 pattern): **AH=33h** is
not in the locked int21h_register/Appendix-A (needed by 4tw/er3h), and **DEC-07 /
`t1on`** must rule on the kzfs MBR-partition forks (start-LBA authority,
application layer, detection heuristic) before `kzfs` can be built.
