# WL-0014 — Kernel hardening: bug fixes, edge/fuzz coverage, reproducibility, Bochs VBE diagnosis

Epic: **initech-bcg** — "Kernel hardening: robustness, fuzzing, edge-cases,
dual/tri-emulator". Branch: `kernel-hardening`.

## Context

Objective from the user: make the InitechDOS kernel **rock solid, correct,
robust, and tested** before the Toolbox/apps/compiler are built on it — focus on
rigorous testing, fuzzing, edge-case coverage, and triple-emulator testing.
Operating principle: fast feedback, no human-in-the-loop. Driven by a grounded
5-facet recon of the kernel surface, test/oracle infra, beads map, tri-emulator
status, and spec ground-truth. Baseline at start: 45 host + 19 QEMU gates green.

## What changed (5 commits, all green)

- **fac5577 — wave A robustness fixes.** Three *verified* latent bugs:
  buffered-input CR-store guard wrapped `uint8_t` for `max>=254` (dropped the
  terminator on a full-size buffer) → `count < max`; `path_has_subdir_or_drive`
  unbounded ASCIIZ scan → bounded runaway guard (`INT21_PATH_SCAN_MAX`);
  `serial_putc`/`pserial_putc` unbounded UART spin (worst on the panic path) →
  bounded spin. Added `kbd.h` power-of-two `_Static_assert`. Two new mutation
  proofs.
- **ad79795 — reproducibility gate.** `test-kernel-repro` clean-rebuilds
  kernel.bin and diffs the sha256 (Rule 11 / PRD §7); `+mutant` proves the
  comparison bites. kernel.bin confirmed reproducible.
- **b381fb3 — stage2 VBE diagnostics.** Split the single `ERR-VBE` marker into
  `VBE-E00/ESIG/ENOMODE/E02`; failure-path only (QEMU stays green).
- **b1fac57 — wave B edge/fuzz coverage.** `test_int21_edge.c` (82
  characterization checks: bad/closed handles, double-close no-underflow, CX=0,
  short-read, 0Eh/19h/47h/59h/62h); `fat12_corrupt_fuzz.c` (200 seeds × 8000
  corruptions — loop-chains, bad EOC, truncated/oob clusters, malformed BPB; all
  fail-loud + bounded); `test_config_sys_fuzz.c` + `test_cmdline_fuzz.c` (4000
  seeds each). All mutation-proven; inert `#ifdef` hooks only in the artifact.
- **0bb8eee — config_sys overflow fix (initech-zfo).** The CONFIG.SYS fuzzer
  found a real off-by-one: `cfg_parse_uint`'s `v > 429496729` guard let the
  2^32-family wrap instead of saturate (clamped to FILES_MIN not MAX). Fixed to
  guard the exact overflow `v > (UINT32_MAX-digit)/10`; fuzzer reference updated
  in lockstep; off-by-one mutant added and caught.

Gate now: **55 host + 19 emu, all green.**

## Why

Every fix is red→green with a mutation-proven oracle (Rule 6). Skepticism (Rule
4) paid off repeatedly: of the audit's flagged "bugs", **do_close ref_count
underflow, FAT mount geometry div-by-zero/underflow, the kbd 8042 drain loop,
and the loader exit-hook struct-packing assert were all already guarded or
nonexistent** — verified and left untouched rather than papered over with
redundant "fixes."

## Frictions

- **Bochs `ERR-VBE` is an environmental wall, not a stage2 bug.** Empirically
  (headless via a custom RFB-unblock client — see below) the tracer boots
  `S1→JMP2→S2→VBE-E00→ERR-VBE`: VBE function `0x4F00` itself fails on Bochs. All
  7 SeaBIOS vgabios variants fail identically; the LGPL vgabios
  (`/usr/share/vgabios/vgabios.bin`, installed this session) is a PCI option ROM
  SeaBIOS won't run under Bochs; no `BIOS-bochs-latest` present. QEMU works
  because it loads a matched vgabios for its std-vga DISPI. **Fixes are
  non-trivial** (Bochs-native BIOS, or a stage2 non-LFB VGA fallback + a console
  renderer for that mode) — a decision, not a quick patch. (initech-6pj)
- **Headless Bochs solved with no install/HITL.** Only the RFB display plugin is
  built; it blocks 30s for a VNC client and never self-proceeds. A minimal RFB
  3.3 handshake client (`/tmp/rfb_unblock.py`, security type None) unblocks it.
  Recorded via `bd remember`.
- **86Box installed** (flatpak `--user`, no sudo) but its headless-automation
  story (Qt offscreen) is unbuilt (initech-44m).

## Acceptance

`make test` = 55 host + 19 emu gates GREEN (was 45+19). 8 new host gates, each
with a mutation proof. kernel.bin reproducible-gated. No emu regression (the
artifact changes are inert `#ifdef` mutant hooks).

## Open follow-ups (decisions needed)

- **initech-tzq** — `do_read`/`do_write`/`do_setdta` accept unvalidated `EDX`
  (the edge suite confirmed the sites). Adding fail-loud guards is a behavioral
  change vs DOS authenticity → needs an ADR + a guard-vs-authentic decision.
- **initech-6pj / 564 / 44m** — the Bochs/86Box tri-emulator path; blocked on the
  VBE-on-Bochs decision above. The headless harness mechanism is proven.

## Pointers

`os/milton/test_int21_edge.c`, `harness/diff/fat_diff/fat12_corrupt_fuzz.c`,
`os/milton/test_config_sys_fuzz.c`, `os/milton/test_cmdline_fuzz.c`,
`os/milton/config_sys.c` (cfg_parse_uint), `os/boot/stage2.asm` (vbe_setup),
Makefile `test-kernel-repro` + `TEST_UNIT_GATES`. `bd show initech-bcg`.
