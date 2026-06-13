# WL-0017 -- Kernel audit + hardening sweep (initech-bcg.1..11)

Epic: **initech-bcg** ("Kernel hardening: robustness, fuzzing, edge-cases,
dual/tri-emulator"). This shard covers a grounded audit of the whole InitechDOS
kernel and the first 11 fixes it produced. Branch: `command-com-default`.

## Context

The operator's directive: get the DOS kernel "rock solid, correct, robust, and
tested feature complete ... bulletproof otherwise everything we build on it will
be flaky." Before landing more features, a multi-agent **read-only audit** swept
nine kernel subsystems (int21, fat12, ata/blockdev, loader/psp, fileio/SFT,
shell, interrupts, console, sysinit/boot) plus three queue-recon questions, with
an adversarial refute pass on every finding (Law 2).

## Audit outcome

- **0 P0 / 0 P1 escaped** -- the core is sound -- but **20 P2 + 20 P3** real
  defects confirmed; 26 candidate findings rejected by the skeptic pass.
- Queue recon: `initech-my3` closed (stale dup of `509.5`; dependents `4iq`/`dtw`
  repointed); `initech-saw` closed (FAT-sourced `.COM` load already done+green);
  the four open `509.x` children confirmed genuinely remaining.
- The 40 confirmed defects were filed as `initech-bcg.1`..`bcg.14` (+ `bcg.15`
  split out), bugs-first by severity.

## What changed (11 fixes, each its own commit)

Each fix: RED -> GREEN -> mutation-proven -> full `make test` -> committed.

- **bcg.1 (P1)** `int21.c` do_write: AL=2 (RDWR) handles were denied WRITE
  (would break InitechBase `.dbf` in-place updates). Permit WRITE||RDWR; keep
  AL=0 denied. `test_fileio` AL=2 round-trip + AL=0 negative.
- **bcg.2 (P1)** `int21.c`: AH=59h GET EXTENDED ERROR returned stale data after
  most failures. Capture the error once at the dispatch choke point (CF=1 ->
  note AX); covers every current+future handler.
- **bcg.3 (P1)** `fat12.c`: no upper-cluster-bound -> an out-of-range in-chain
  cluster mapped to a wrong/out-of-volume LBA (silent wrong data). Added
  `fat12_cluster_in_range`; hardened all 5 walk sites (read + write paths).
- **bcg.4 (P1)** `fat12.c`: FAT16 volumes mounted OK but were decoded 12-bit
  (garbage). Reject at mount with new `FAT12_ERR_UNSUPPORTED`.
- **bcg.5 (P2)** `int21.c` do_puts (AH=09h): NULL/unbounded EDX walk. NULL guard
  + `INT21_PUTS_SCAN_MAX` (64KiB) bound.
- **bcg.6 (P2)** `panic.c`: spurious/unhandled vectors halted forever. Resume
  (clean iret) for vector>31; halt only for CPU exceptions 0-31. New
  `test-spurious` emu gate (fires `int 0x70`).
- **bcg.7 (P2)** `isr.asm`/`idt.c`/`panic.c`: 8259A spurious-IRQ EOI discipline.
  Dedicated 0x2F/0x37 stubs; slave spurious -> master-only EOI.
- **bcg.8 (P2)** `irq.c`: `rserial_putc` unbounded spin could hang the fail-loud
  path. Bound it (`RSERIAL_SPIN_MAX`) mirroring `panic.c`.
- **bcg.9 (P2)** `sysinit.c`: an oversize CONFIG.SYS was silently discarded for
  the baseline. Per operator choice, honor the first 1KB via `fat12_read_partial`
  (trim to last line) + loud `source=CONFIG.SYS(truncated@1024)` marker. New
  `test-sysinit-oversize` gate.
- **bcg.11 (P2)** `console.c`: degenerate geometry accepted (pitch==0 aliased
  scanlines, pitch<width*bpp overran). `CONSOLE_ERR_GEOMETRY` guard + padded-
  pitch coverage test.
- **bcg.10 (P2)** `loader.c`: the entry asm did `jmp *entry_eip` (memory operand)
  AFTER switching ESP -- correct only at -O0 with a frame pointer; at -O2 /
  -fomit-frame-pointer the local goes ESP-relative and the jump would read the
  switched PROGRAM stack. Load the target into EAX BEFORE the switch and jmp via
  register. Demonstrated RED->GREEN by building loader.c at -O2 -fomit-frame-
  pointer (pre-fix: GREET.COM never runs; fixed: passes); no -O0 change. When the
  build moves to -O2/i686-elf (initech-6pm, CDR-0001) test-exec becomes the
  standing -O2 gate and now stays green.

## Why this shape

Audit + recon fan out cleanly (read-only); the fixes do NOT -- they share
`int21.c`/`fat12.c` and a single oracle, so implementation was serial and
oracle-gated. Parallelism lived in the audit, not the edits.

## Frictions

- `perl` restores choke on `/* */` mutant delimiters -- restore those via Edit.
- Adding asm externs (`isr_irq7/15_spurious`) to `idt.c` broke the host
  `test_idt` link until stubs were added -- the full `make test` (not just the
  subsystem gate) caught it. Law 2 earns its keep.

## Acceptance

- `make test` = **55 host + 21 emu gates GREEN** (was 55+19; +test-spurious,
  +test-sysinit-oversize). `test-boot-bochs` PASS (re-run after the interrupt
  changes per the CLAUDE.md interrupt-minefield rule).
- New gates mutation-proven; new error codes `FAT12_ERR_UNSUPPORTED`,
  `CONSOLE_ERR_GEOMETRY`.

## Pointers / remaining (initech-bcg children still open: 12, 13, 14, 15)

- **bcg.12** ata.c fail-loud/error/write branches have no oracle; +BSY/DRDY
  before command; error codes collapsed. Needs boot-without-disk + corrupt-image
  emu infra.
- **bcg.13** shell message-catalogue conformance scanner (ADR-0003 7.4); CD/DIR
  diagnostics off-spec.
- **bcg.14** P3 robustness + test-gap sweep (lseek high half, getver garbage,
  console rows==0, ata niEN, SFT creat-truncate/close-leak/open-mode, fileio
  rc-mapping, loader nested-exec guard oracle).
- **bcg.15** 8bpp DAC index->seafoam end-to-end oracle (mode-0x13 screendump).
