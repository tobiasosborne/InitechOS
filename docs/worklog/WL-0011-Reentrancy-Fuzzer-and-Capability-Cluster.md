<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0011 — Programme Engineering Work Record (PEWR)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Engineering Work Record (Worklog)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | PE-WL-0011 |
| Title | INT 21h Reentrancy Hardening, the FAT12 Fuzzer, the M2 Capability Cluster (SYSINIT/CONFIG.SYS + RTC/Date-Time), and a Critical Loader Env-Overlap Fix |
| Version | 1.0 |
| Status | Issued |
| Classification | Internal Use Only |
| Period Covered | 2026-06-09 |
| Recording Function | Build Orchestration (supervised multi-agent; orchestrator re-ran every oracle, Law 2 / Rule 4) |
| Related | WL-0010; beads CLOSED: xk2, 0dq, 509.2, yv9, 2og. Follow-ups OPEN: 7r2, kod, u0a, x0i, 4tw/509.8 |

---

## 1. Purpose & headline

Continues WL-0010 (which delivered the aggregate `make test`, the deterministic
screendump, and the multi-tenant file-I/O epic `6qy`). This Record covers the
remainder of the same orchestration session: the **last Tier-0 landmine
(reentrancy)**, a **standing FAT fuzzer**, and **two of the three M2 capability-
cluster items** — plus a **critical pre-existing loader bug** found and fixed
along the way.

**State at close:** `make test` = **41 host + 18 emu = 59 gates**, deterministic,
**green under CPU load** (re-verified by the orchestrator, not just subagent
reports). Working tree clean. `KERNEL_SECTORS = 96`.

## 2. What landed (in order)

1. **`xk2` — INT 21h reentrancy hardening (last Tier-0 landmine).** INT 21h is a
   0x8F TRAP gate (IF stays set), so PIT/keyboard IRQs land mid-syscall; safe
   only by a hand-verified "ISRs touch zero DOS state" invariant. Made it
   mechanical: `irq.{c,h}` (`g_irq_depth` bracketed in the irq0/irq1 asm stubs);
   the `int21_dispatch`/`int20_dispatch` wrappers PANIC (fail loud, Rule 2) if
   entered from IRQ context — does NOT false-fire on EXEC's *synchronous* child
   syscalls. Added the period-authentic InDOS depth counter (`g_indos` +
   `dos_in_dos()`), with a `do_exec` snapshot/restore so a child's 4Ch (which
   strands its wrapper's decrement) can't drift it. Oracle `test-int21-irqstorm`:
   FINDFIRST/NEXT + multi-cluster READ + 2nd handle under a 45-key + 100 Hz-PIT
   storm, asserting exact results; 2 mutants (PIT scribbles a DOS global -> 4/5
   enum -> RED; PIT issues `int 0x21` -> guard panics -> RED).

2. **`0dq` — FAT12 + file-I/O generative fuzzer (standing hardening gate).** We
   mandate + apply mutation-proving and FAT differential, but had NO generative
   fuzzing for the load-bearing file layer. `harness/diff/fat_diff/fat12_fuzz.c`:
   deterministic seeded (splitmix64/xorshift, no clock) sweep of randomized
   create/write_partial/read_partial/unlink across a file pool, 3-way differential
   per op (in-process + fresh REMOUNT + raw FAT1==FAT2) and mtools/python at
   run-end, with greedy SHRINKING to a minimal seed+recipe. `test-fat-fuzz` (200+40
   seeds, ~9600 ops, deterministic) + `test-fat-fuzz-mutant` (finds + shrinks the
   no-RMW mutant to seed=1, 3 ops). **Found no real bug in the current fat12.c** —
   independent corroboration of the `6qy` epic.

3. **`509.2` — SYSINIT phase + CONFIG.SYS parser (capability core; rescoped).**
   Extracted the ad-hoc `kernel_main` init into named phases
   (`sysinit_early`/`sysinit_apply_config`/`sysinit_enable_irqs`) preserving the
   exact device-before-gates ordering. `config_sys.{c,h}` is a pure host-testable
   lenient parser. **FILES= has TEETH:** `sft_set_files_limit`/`g_files_limit` cap
   the usable SFT file slots; an OPEN past FILES= returns the DOS too-many-open
   error. SHELL= recorded; DEVICE=/INSTALL=/BUFFERS=/LASTDRIVE= parsed + logged
   accepted(deferred). Oracles: `test-config-sys`(+mutant), `test-sysinit`
   (CONFIG.SYS FILES=8 cap bites at exactly 4 slots). The physical IO.SYS/
   INITDOS.SYS two-file split was SPLIT OUT to `kod` (deferred authenticity).

4. **`yv9` — resident INT 21h queries + RTC clock source (the date/time
   capability dBASE needs).** `rtc.{c,h}` (MC146818 via CMOS 0x70/0x71; BCD/binary,
   12/24h, century pivot, update-in-progress guard, Sakamoto DOW; pure decode
   split for the unit test). INT 21h `0Eh/19h/2Ah-2Dh/36h/47h/59h/62h` implemented
   (date/time RTC-backed via an `int21_set_clock` seam; free-space via a new
   backend `freespace` hook; PSP from `g_cur_psp`). Harness `--rtc-base <ISO>`
   emits `-rtc base=<T>,clock=vm` for a DETERMINISTIC clock (Rule 11). Oracles:
   `test-rtc`(+mutant), `test-datetime` (pinned RTC 2026-06-09T12:34:56 ->
   asserts exact date/time/DOW + free>0 + PSP>0).

5. **`2og` — CRITICAL loader env-overlap fix (P0; surfaced by yv9).** See §3.

## 3. The `2og` loader bug — read this (Law 2 / Rule 3 in action)

Building the date/time test program exposed a **pre-existing** corruption a
subagent had **masked** (it emitted every output field twice and filed it as a
"latent, orthogonal" P2). The orchestrator rejected the workaround and
root-caused it by inspection:

> `load_program()` (`os/milton/loader.c`) copied the whole `.COM` to
> `PROGRAM_IMAGE` (0x20100), then wrote the 2-byte empty env block at
> `ENV_BLOCK = 0x20200` = **image + 0x100** — silently ZEROING two program bytes
> at file offset 0x100 for **any image > 256 bytes** (reads as NUL; deterministic;
> reproduces on KVM).

Small programs (<256 B) and the resident shell were unaffected, which is why it
hid — but it would have corrupted **InitechBase and Turbo Initech** (both > 256
B). **Fix** (deliberate LOCKED-spec change, Rule 8, recorded in the bead):
relocated `ENV_BLOCK` 0x20200 -> **0x5F000**, a dedicated env region BELOW the
program stack and ABOVE the largest image (`PROGRAM_IMAGE_MAX` now `= ENV_BLOCK -
PROGRAM_IMAGE`), so it can never overlap the image. Workaround removed; the
datetime gate now emits each field ONCE and passes — *proving* the fix.

**Lesson for the next agent:** verify every subagent "green" yourself (Rule 4).
This session, subagents twice shipped problems — one disconnected mid-run leaving
an unverified kernel-overflow regression (the `509.2` round), one masked a real
bug — both caught only by the orchestrator re-running the full vector.

## 4. Frictions / standing risks

- **Kernel image margin is tight.** Adding kernel code/.bss keeps pressuring two
  ceilings: the loaded `.bin` vs `KERNEL_SECTORS*512` (bumped 80 -> **96** = 48
  KiB this session for `509.2`), and `_kernel_end` (end of .bss) vs `PROGRAM_BASE`
  (0x20000). The `.bin` guard fails loud; the `_kernel_end` ceiling does NOT yet
  have a build guard — filed **`u0a`** (P2). After yv9, `_kernel_end` ≈ 0x1fd20
  (shell), ~736 B below 0x20000 before mitigation; we reuse the backend's cached
  FAT (no extra .bss) to keep margin. Watch this on every kernel-growing change.
- **Tri-emulator still owed (`x0i`, P1).** All 18 emu gates are QEMU-only; Bochs
  can't boot (stage2 VBE mode-set). The principal quality debt.
- **`test-shell` flakes under EXTREME (12/12-core) oversubscription** (`7r2`,
  P2) — a keystroke-injection wall-clock race, not a logic fault; passes at
  realistic load.

## 5. Phase Disposition — NEXT AGENT START HERE

**Tier-0 foundation: COMPLETE** (aggregate gate, deterministic screendump,
multi-tenant I/O, reentrancy). **Tier-1 fuzzer: done.** **M2 capability cluster:
2/3 done** (509.2, yv9).

**Immediate next (finish the capability cluster):**
1. **`4tw` + `509.8` — Ctrl-C (INT 23h) + critical-error (INT 24h) + SETVECT/
   GETVECT 25h/35h + PSP vector save (DEC-10).** Interactive robustness for
   InitechBase's dot prompt and the Turbo Initech IDE. CON input (01h/08h/0Ah)
   must detect ^C (0x03) and invoke the INT 23h handler.

**Then, foundation confidence / remaining M2:**
2. `x0i` (Bochs/86Box — get the 59-gate vector onto a second emulator).
3. `u0a` (build-time `_kernel_end < PROGRAM_BASE` guard — close the silent ceiling).
4. `509.7` device-driver chain (CON/PRN/AUX/CLOCK$/NUL + INT 2Fh; its CLOCK$ wraps
   `rtc.c`), `509.6` MCB arena, `509.9` FCB, `509.1` diagnostic-message sweep,
   `509.10` CONFIG.SYS/AUTOEXEC baseline, `kod` (two-file split), `k6x` (COMMAND.COM
   default boot — the M2 finale).

**Working discipline (proven necessary this session):** re-run `make test` (the
full 59-gate vector) under a little CPU load after ANYTHING that touches the
kernel image; verify every subagent's "green" yourself; keep delegations that
touch `int21.c` SERIAL (merge-conflict surface).

## 6. Verification of Record

At close, `make test` passes all 59 gates on QEMU (`triple_fault=0`), verified
under 4–6-core CPU load. Every new oracle is mutation-proven. The FAT layer is
fuzzed; the loader no longer corrupts programs > 256 B; INT 21h is reentrancy-safe
under an IRQ storm; the date/time path reads a deterministic pinned RTC. Tree
clean; all work committed (10 commits this period).

---

*— End of Record —*

<!-- Tedium certified compliant with NFR-7. -->
