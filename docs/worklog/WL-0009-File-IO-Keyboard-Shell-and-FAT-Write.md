<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0009 — Programme Engineering Work Record (PEWR)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Engineering Work Record (Worklog)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | PE-WL-0009 |
| Title | File-Handle I/O, the Keyboard/PIT Interrupt Path, CON Input, FAT-Sourced Program Load, the COMMAND.COM Shell, and FAT Write |
| Version | 1.0 |
| Status | Issued |
| Classification | Internal Use Only |
| Period Covered | 2026-06-08 → 2026-06-09 |
| Recording Function | Build Orchestration (supervised multi-agent; one bead per delegated subagent, orchestrator-verified) |
| Related | WL-0008; ADR-0003 (DEC-01/04/06/07/08/11/13); beads initech-509.5, -3rs, -43b, -n62, -saw, -7pc, -509.11 (all closed), -adf (closed) |

---

## 1. Purpose

This Record memorializes a sustained **orchestration session**: the operator
directed "take a bead, delegate a subagent, monitor, raise beads, repeat — until
the DOS is done with all features." The orchestrator (main loop) sequenced the
critical path to an **interactive InitechDOS**, delegated each bead to a focused
subagent, and **independently re-ran every oracle** (Law 2 / Rule 4 — the green
oracle is authoritative, the subagent's report is not) before closing anything.

The headline outcome: **InitechDOS now boots to an `A:\>` prompt and runs
`DIR`, `TYPE`, a program, and `EXIT` interactively**, and can **create and write
files** to a FAT12 volume that real `mtools` reads back byte-for-byte.

## 2. What Landed (in dependency order)

1. **INT 21h file-handle functions (`initech-509.5` read-side).** 3Dh OPEN /
   3Fh READ / 3Eh CLOSE / 42h LSEEK / 4Eh-4Fh FINDFIRST-NEXT / 1Ah-2Fh
   SETDTA-GETDTA, resolved through the JFT→SFT via a new `int21_file_backend_t`
   vtable (kernel binds `fileio_fat.c`; host oracle binds a mock — keeping
   int21.c host-testable). Whole-file read at OPEN into a reserved static buffer
   (`spec/memory_map.h`); 43-byte find-record locked as `spec/find_data.h`.
2. **PS/2 keyboard (IRQ1) + PIT (IRQ0) (`initech-3rs`) + harness keystroke
   injection (`initech-43b`).** `kbd.{h,c}` (ring + scancode-set-1→ASCII with
   Shift/Caps), `pit.{h,c}` (100 Hz tick), IRQ stubs, 0x8E interrupt gates,
   `pic_unmask_irq0_irq1`, and **the first `sti`** — placed after the boot demos,
   no regression. Harness gained `--keys`/`--keys-after` (QMP `send-key`).
3. **INT 21h CON input (`initech-n62`).** AH=01h/06h/07h/08h/0Ah/0Bh/0Ch via a
   new `int21_set_conin(get,poll)` seam (kernel binds a `hlt`-spin over
   `kbd_getchar` + a non-blocking poll; host binds a queued mock). AH=0Ah
   buffered line input (BACKSPACE edit, max-length clamp) is what the shell reads.
4. **FAT-sourced program load (`initech-saw`) + EXEC (`509.5` tail).**
   `load_program_from_fat` reads a `.COM` BY NAME from the mounted volume and
   runs it; AH=4Bh EXEC + AH=4Dh return-code from kernel/shell context; a
   single-level reentrancy guard rejects nested EXEC. `GREET.COM` minted onto a
   FAT image.
5. **COMMAND.COM-alike shell (`initech-7pc`) — the M2 capstone.** A
   kernel-resident REPL (`command.{h,c}`) that **dogfoods the INT 21h API** via
   inline `int $0x21`: `A:\>` prompt, DIR (FINDFIRST/NEXT), TYPE (OPEN/READ/
   WRITE/CLOSE), CD/CLS/VER/ECHO/EXIT, and external `.COM` via EXEC. Built behind
   a separate `BOOT_SHELL` image so the demo-based gates' normal image is
   untouched.
6. **FAT12 write (`initech-509.11`).** ATA WRITE SECTORS + CACHE FLUSH;
   `fat12_create`/`write_file`/`unlink` (lowest-free cluster alloc, both-FAT-copy
   sync, 12-bit packed-entry write); INT 21h 3Ch CREAT / 40h WRITE-to-file
   (buffer-then-flush-at-close) / 3Eh CLOSE-flush / 41h UNLINK. **A volume
   written entirely by InitechDOS reads back byte-for-byte in `mtools`.**

`initech-adf` (ATA + FAT read) was closed (functionally complete, oracle-green).

## 3. Oracles of Record (Law 2 — all re-run by the orchestrator, all green)

Host unit + mutation (Rule 6, every new oracle has a mutant proven to go RED):
`test-fileio` (50), `test-kbd-unit` (286), `test-conin-unit` (45), `test-exec-unit`
(34), `test-command` (41), `test-fat12-write`/`test-fat-write` (97). Plus the
pre-existing `test-int21` (48), `test-sft` (47), `test-psp` (84), `test-loader`
(21), `test-fat`, `test-fat12-{bpb,chain,dir}`.

In-emulator keystones (QEMU; `triple_fault=0`):
`test-type` (a program TYPEs a real file), `test-dir` (a program enumerates the
root), `test-kbd` (QMP-injected `d,i,r` → guest echoes `dir`), `test-conin` (a
program reads a keyboard line via AH=0Ah), `test-exec` (`GREET.COM` loaded FROM
FAT, ran, rc=7; AH=4Bh/4Dh), **`test-shell`** (boot → `A:\>` → dir/type/run/exit),
`test-fatwrite` (CREAT+WRITE+CLOSE then OPEN+READ back; post-run `mcopy` confirms
on disk), plus `test-program`/`test-fs`/`test-boot`/`test-tracer-boot`/`test-panic`.

**22-gate regression sweep is green** (run after the `KERNEL_SECTORS 64→80`
change, §4). All new sources ASCII-clean (Rule 12).

## 4. Decisions / Frictions of Record

- **First `sti` (interrupt enablement).** Placed after the boot demos; IRQ
  handlers use 0x8E gates (IF cleared, no nesting). **Reentrancy invariant
  (initech-xk2):** kbd/pit ISRs share ZERO state with the INT 21h dispatcher
  (verified by grep). INT 21h uses 0x8F trap gates (IF stays set), so an IRQ can
  now interrupt a syscall — safe ONLY by that zero-shared-state property. Deeper
  audit still owed (xk2).
- **Bochs (Rule 5) is owed.** All in-emulator gates are **QEMU-only**: Bochs
  cannot boot InitechOS — stage2's VBE 640×480 mode-set fails on the available
  Bochs VGA BIOS (`ERR-VBE`), before any C. The `sti`/IRQ path and the ATA-WRITE
  path are therefore not yet tri-emulator-verified. Tracked + evidenced in
  **initech-x0i**.
- **Kernel outgrew its window.** FAT write pushed the `BOOT_SHELL` image past the
  64-sector kernel region; root-caused (Rule 3) and fixed by bumping
  `KERNEL_SECTORS` 64→80 (40 KiB) in **both** `Makefile` and `stage2.asm`. Kernel
  ends ~0x1A000, below `PROGRAM_BASE` 0x20000. All boot gates re-verified.
- **Two beads force-closed over `initech-509.2`.** 509.5 (and earlier 509.3)
  depend on 509.2 (the IO.SYS/INITDOS.SYS two-file split + a distinct SYSINIT
  phase), which is **not yet built** — `sft_init` + the conin/file/exec backend
  binds run in `kernel_main` (the de-facto SYSINIT). 509.2 remains open; when it
  lands, relocate that init into the real SYSINIT.
- **DTA layout divergence (initech-dww).** `spec/find_data.h` uses the
  ground-truth brief's simplified offsets (self-consistent with our DIR.COM) —
  NOT byte-compatible with the real-DOS FINDFIRST DTA. Decide before any
  real-DOS differential.
- **Enter encoding (initech-62m).** kbd.c decodes Enter→LF(0x0A); AH=0Ah
  normalizes to CR. Root fix (kbd→CR, or an INT 16h layer) deferred.

## 5. Phase Disposition — M2 IS 4/11; NEXT AGENT START HERE

Closed M2 children: 509.3, 509.4, **509.5**, **509.11**. The interactive shell
keystone works. Remaining M2 work, in a sensible dependency/priority order:

1. **`initech-509.2` (P1) — IO.SYS + INITDOS.SYS two-file kernel + SYSINIT.**
   Structural; unblocks 509.7 and 509.10, and lets two force-closes be tidied.
   SYSINIT parses CONFIG.SYS (FILES=/BUFFERS=/DEVICE=/SHELL=). Relocate the
   `kernel_main` init (sft_init, backend binds) into SYSINIT here.
2. **`initech-509.7` (P2, dep 509.2) — resident device drivers** CON/PRN/AUX/
   CLOCK\$/NUL + the INT 2Fh redirector (DEC-09). CLOCK\$ gives a clock source
   that unblocks date/time (yv9) and FILETIME (uvf).
3. **`initech-509.6` (P2) — MCB memory arena** 48h/49h/4Ah (vestigial; dep 509.3
   done). New `mcb.{h,c}` + int21 dispatch.
4. **`initech-509.8` (P2, dep 509.1) — INT 22/23/24 handlers + SETVECT/GETVECT
   25h/35h + PSP vector save** (DEC-10). Enables Ctrl-C/critical-error (initech-4tw).
5. **`initech-yv9` (P2) — remaining resident INT 21h queries** (0Eh/19h/2Ah-2Dh/
   36h/47h/59h/62h). Date/time wants 509.7's CLOCK\$.
6. **`initech-509.9` (P3) — FCB legacy file API** 0Fh-24h (vestigial; dep 509.5
   done + adf done).
7. **`initech-509.1` (P0) — diagnostic-message catalogue enforcement.** Best done
   as a FINAL sweep once all diagnostic-emitting features (above) have landed:
   route every user-facing console diagnostic through `spec/dos_messages.json`
   (MSG-DOS-0001..0016) and add the string-extraction oracle. (Serial debug
   markers are exempt — they are not the DOS personality.)
8. **`initech-509.10` (P2, dep 509.1+509.2) — CONFIG.SYS + AUTOEXEC.BAT baseline**
   (DEC-11/12). Pairs with the shell's batch interpreter (initech-xw1).
9. **`initech-k6x` (P1) — M2 FINALE: make COMMAND.COM the default boot** + migrate
   the demo-based gates (test-program/type/dir/fs) to drive through the shell.
   This is what makes "the DOS boots to a shell" the real, default experience.
10. **`initech-dtw` (P1, rescoped) — MZ .EXE loader** (flat .COM loader is done;
    MZ deferred per DEC-08; also unlocks real-DOS .EXE differential oracles).

Cross-cutting follow-ups raised this session: file — lq2/0qh/ti8/80k/dww/snk/uvf;
shell — xw1(.BAT)/atf(PATH/COMSPEC)/456(EXEC argv); input — 4tw(Ctrl-C)/62m(Enter);
platform — x0i(Bochs/tri-emulator, **growing in importance**)/xk2(reentrancy audit).

**Orchestration note for the next agent:** most remaining beads touch `int21.c`
(the dispatcher) — keep delegations SERIAL to avoid merge conflicts there, and
re-run the full ~22-gate sweep after anything that touches the kernel image
(boot layout is sensitive; see §4 KERNEL_SECTORS). Verify every subagent's
"green" claim yourself (Rule 4).

## 6. Verification of Record

At close of period the full per-subsystem gate vector passes on QEMU with
`triple_fault=0`; FAT write round-trips through real `mtools`; the interactive
shell keystone is green and bite-proven. Bochs/86Box agreement is the principal
outstanding quality debt (initech-x0i). The aggregate `make test` target is still
the pre-existing M0 stub.

---

*— End of Record —*

<!-- Tedium certified compliant with NFR-7. -->
