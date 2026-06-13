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

- **e5357b1 — INT 21h user-pointer guards (ADR-0003 DEC-14, initech-tzq).**
  Acting on the edge-suite finding: `do_read`/`do_write` (and the FINDFIRST DTA
  write) now validate `[EDX,EDX+CX)` via `user_buf_ok` -- NULL or a 32-bit-
  wrapping range -> CF=1, AX=0x0009 (new "invalid memory block" code) instead of
  faulting/scribbling. Documented divergence from DOS authenticity (ADR DEC-14).
  Mutation-proven (test-int21-edge 82->94 checks; the no-guard mutant SIGSEGVs on
  a NULL read = RED).

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

## Open follow-ups

- **initech-tzq — DONE** (e5357b1): user chose fail-loud error return; ADR-0003
  DEC-14 ratified + implemented + mutation-proven.
- **initech-6pj / 564 / 44m — Bochs tri-emulator, OPEN (dedicated effort).** User
  approved investigating a Bochs-native BIOS; `bochsbios` + `bochs-term` were
  installed. Result: `BIOS-bochs-latest` does NOT boot clean either -- the guest
  faults very early (interrupt with `IDT.limit=0x0`, never reaching stage2),
  likely the i440FX/ACPI/USB environment or the tiny 2/2/32 disk geometry vs what
  the native BIOS expects. So **both** paths now have a known failure (SeaBIOS:
  reaches stage2, no VBE; native BIOS: crashes pre-stage2). Next options, all
  non-trivial: standard disk geometry + `bochs-term` headless for the native BIOS;
  or a stage2 non-LFB VGA fallback + a console renderer (the most robust, biggest
  effort). QEMU remains the binding boot/visual gate; the headless Bochs mechanism
  (RFB-unblock client) is proven and reusable.

## Epic status (initech-bcg)

Robustness, edge-case, fuzzing, reproducibility, and user-pointer-guard work is
**complete and green**. The only remaining leg is the Bochs/86Box tri-emulator
gate, which is a dedicated effort blocked as above.

## Pointers

`os/milton/test_int21_edge.c`, `harness/diff/fat_diff/fat12_corrupt_fuzz.c`,
`os/milton/test_config_sys_fuzz.c`, `os/milton/test_cmdline_fuzz.c`,
`os/milton/config_sys.c` (cfg_parse_uint), `os/boot/stage2.asm` (vbe_setup),
Makefile `test-kernel-repro` + `TEST_UNIT_GATES`. `bd show initech-bcg`.

---

## HANDOFF — next agent

**Branch `kernel-hardening`, 8 commits since 7edee16 (fac5577 through the
handoff commit inclusive), `make test` GREEN = 55 host + 19 emu** (independently
re-verified). Local-only repo (no remote; do NOT try to push). Start by
re-reading CLAUDE.md + this shard, then `make test` to confirm the baseline.
(Line numbers below are accurate as of the handoff commit but drift with edits --
grep the cited symbol if a number is stale.)

### State of the epic (initech-bcg)

DONE + green: waves A/B robustness fixes, reproducibility gate, edge/error-path
suite, FAT-corruption + CONFIG.SYS + cmdline fuzzers, the config_sys overflow
fix, and the DEC-14 user-pointer guards. The ONLY open leg is the Bochs/86Box
tri-emulator gate (initech-6pj, 564, 44m).

### Verified non-bugs (do NOT "re-fix" -- Rule 4)

The recon audit over-reported. These are guarded/nonexistent, confirmed in code
(lines re-verified at the handoff commit): `do_close` ref_count underflow
(int21.c:~1035 `if (e->ref_count > 0u)` + sft.c:195 + `jft[handle]=JFT_CLOSED`);
FAT mount div-by-zero/underflow (fat12.c: sectors_per_cluster==0 ~:92,
total_sectors==0 ~:105, first_data_sector bound ~:118); the "kbd 8042 drain loop"
(does not exist); loader exit-hook struct-packing asserts (loader.c:163-170,
already present).

### DEC-14 scope NOT covered (candidate follow-ups, low priority)

- `user_buf_ok` validates NULL + 32-bit-wrap only. A target arena-ceiling check
  (reject ptr >= conventional-RAM top) is intentionally OUT (would reject the
  host unit tests' buffers, which live at host addresses). If wanted, it needs an
  EMULATOR oracle (a .COM passing an out-of-arena ptr), not a host test.
- The `emit_find_data` DTA guard is wired but NOT directly tested (this suite's
  mock has no `dir_entry` hook). A FINDFIRST-wild-DTA test belongs in
  `test_fileio` (which has a working dir_entry mock + FINDFIRST).

### Bochs continuation guide (the real remaining work)

Headless Bochs is SOLVED for diagnosis: `harness/emu/rfb_unblock.py` (the env's
Bochs has only the RFB plugin, which blocks 30s for a client). `bochs-term` and
`bochsbios` are now installed -- **try `display_library: term` first** (simpler
than RFB; may remove the client+timing entirely).

Two known, distinct failure modes (both reproduced empirically, headless):
1. **SeaBIOS bios.bin + any vgabios -> `S1 JMP2 S2 VBE-E00 ERR-VBE`.** VBE
   function 0x4F00 (get-controller-info) itself fails; the SeaBIOS vgabios
   variants are QEMU-targeted and don't drive Bochs's VBE/DISPI via INT 10h. The
   new stage2 markers (VBE-E00/ESIG/ENOMODE/E02, os/boot/stage2.asm vbe_setup)
   localize it. NOT a stage2 logic bug.
2. **BIOS-bochs-latest (native) -> faults pre-stage2** (`interrupt(): IDT.limit
   =0x0`, no S1). Likely the i440FX/ACPI/USB machine or the unusual CHS=2/2/32
   geometry vs what the native BIOS expects.

Decision tree for the next agent (in increasing effort):
- (a) Native BIOS + STANDARD disk geometry: rebuild the tracer image with a
  larger/standard CHS (or LBA) and retry `BIOS-bochs-latest` -- the IDT.limit=0
  fault may be geometry/boot-sector related. Cheapest experiment.
- (b) `bochs-term` headless + either BIOS, to remove RFB timing as a variable.
- (c) If VBE genuinely cannot be made to work on Bochs: add a **stage2 non-LFB
  VGA fallback** (INT 10h mode 0x12/0x13) + a console renderer for that mode.
  Biggest effort; touches the display model; warrants a user decision (the
  current agent deferred it on those grounds).
Once a path boots clean: write `harness/emu/bochs.c` (model on qemu.c -- see the
shape captured in the recon; structured BochsResult, generated bochsrc, serial
capture, marker scan, triple-fault detect) and wire `test-boot` to a QEMU==Bochs
differential (initech-564 / initech-bea). 86Box (initech-44m) is installed via
flatpak but its Qt-offscreen headless automation is unbuilt -- lowest priority.
