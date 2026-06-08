<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0005 — Programme Engineering Work Record (PEWR)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Engineering Work Record (Worklog)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | PE-WL-0005 |
| Title | Interrupt Foundation (IDT/PIC) and the INT 21h Dispatcher Spine |
| Version | 1.0 |
| Status | Issued |
| Classification | Internal Use Only |
| Period Covered | 2026-06-08 |
| Recording Function | Build Orchestration (supervised multi-agent) |
| Related | WL-0004; ADR-0003 (DEC-04, DEC-13, App A); beads initech-a5a, initech-509.5, initech-1f9, initech-n62 |

---

## 1. Purpose

This Record memorializes the first increment of the InitechDOS resident-kernel
*internals*: the interrupt foundation (an Interrupt Descriptor Table, CPU exception
handlers with a fail-loud panic, and an 8259 PIC remap/mask) and the INT 21h
system-call dispatcher spine with its console-output function subset. The increment
was executed as a supervised, serial multi-agent orchestration (one research pass,
two implementation passes, an orchestrator verification pass), each verified
independently against the locked spec and a mechanical oracle before the next.

## 2. Ground Truth Established (Law 1)

A research pass produced `docs/research/internals-int21h-ground-truth.md`: the 32-bit
IDT gate descriptor format and gate-type policy (interrupt gate 0x8E for
exceptions/IRQs, trap gate 0x8F for the syscall); the canonical
error-code-pushing exception vector list (8, 10, 11, 12, 13, 14, 17, 21); the 8259
ICW remap sequence; and — the load-bearing decision — a calling convention for
`INT 21h` in a 32-bit flat kernel, where the 16-bit segmented DOS convention does
not apply.

## 3. Load-Bearing Architectural Decision (Ratification Pending)

The research surfaced an architectural choice not derivable from the ratified ADRs,
and accordingly raised for operator ratification (beads initech-1f9, proposed as
ADR-0003 amendment **DEC-04a**):

1. **Literal `int 0x21`** via a 32-bit trap gate at vector 0x21.
2. **PIC remap to master 0x28 / slave 0x30** (not the conventional 0x20/0x28), so
   vector 0x21 remains free for the syscall — averting a permanent INT 21h ↔ IRQ1
   (keyboard) collision that the conventional mapping would create.
3. **Flat calling convention:** function in AH (EAX[15:8]); flat pointer in EDX;
   count in ECX; handle in EBX; return in EAX; error via the carry flag in the
   caller's saved EFLAGS (the DOS `int 0x21 / jc` idiom preserved).

These are IMPLEMENTED now under this documented (reversible) decision and locked as
spec-data in `spec/int21h_calling_convention.json` (marked provisional pending
DEC-04a). The implementation is contained; should the operator revise the
convention, the affected surface is the dispatcher and the gate map.

## 4. Artifacts Delivered

- **Interrupt foundation (initech-a5a, closed):** `os/milton/idt.{h,c}` (256-entry
  IDT, gate encoder, `lidt`, an `idt_install_trap` seam); `os/milton/isr.asm` (32
  exception stubs with uniform-frame discipline + a common dispatch stub);
  `os/milton/panic.c` (fail-loud exception handler: serial register dump + console
  panic line + halt, never a silent triple-fault); `os/milton/pic.{h,c}` (remap to
  0x28/0x30 + mask-all). A documented `int_frame_t` matching the `pushad` order.
- **INT 21h spine (initech-509.5, console subset; issue remains open):**
  `spec/int21h_calling_convention.json` (new locked convention); `os/milton/int21.{h,c}`
  (the AH dispatcher, controlled scope); the `int21_entry` trap stub. Functions:
  02h PUTCHAR, 09h PUTS ($-terminated), 40h WRITE (handles 1/2 → console), 30h
  GETVER (3.30), 4Ch/00h TERMINATE. Unlisted AH → diagnostic + carry; listed-but-
  deferred AH → a distinct "not-yet-implemented" diagnostic — never a silent no-op
  (Rule 2 / ADR-0003 DEC-13 controlled scope).

## 5. Oracle Disposition (Law 2)

- **Host oracles (RED→GREEN, mutation-proven):** `make test-idt` (28 checks — gate
  byte-encoding against hand-computed values + `int_frame_t` field offsets) and
  `make test-int21` (35 checks — per-function dispatch, the $-terminator, handle
  validation, carry propagation, controlled scope). Both were independently
  re-mutated by the orchestrator (offset-hi shift; the $-terminator → NUL) and
  observed to bite, then restored bit-identical.
- **In-emulator oracle:** `make test-panic` boots a fault-self-test kernel that
  deliberately raises #DE and asserts `PANIC vec=00` on serial **with
  triple_fault=0** — proving the fail-loud path catches a real fault instead of
  silently rebooting (the documented minefield).
- **Live integration:** the InitechDOS banner now prints **through `int 0x21`
  AH=09h** on the real boot chain; `make test-boot` still verifies the serial banner
  equals `spec/dos_banner.txt` byte-for-byte and the screendump renders it.
- `make test-spec` was extended to parse the new convention file and assert every AH
  it documents exists in the controlled register (consistency). A spec-drift
  reference (an "ADR-0005" mislabel) was corrected to DEC-04a during reconciliation.

## 6. Verification of Record

At close of period the following pass without exception: `test-idt`,
`test-idt-mutant`, `test-int21`, `test-int21-mutant`, `test-panic`, `test-boot`,
`test-tracer-boot`, `test-spec`, `test-console`, `test-fat`, and the FAT12 unit
oracles. No triple-fault occurs on any emulator run. All new sources are ASCII-clean
(Rule 12) and compile both freestanding (kernel) and hosted (tests). Kernel binaries
are reproducible (Rule 11).

## 7. Phase Disposition

InitechDOS now has an interrupt foundation and a working `int 0x21` system-call
surface, exercised end-to-end (the banner prints through it). Per the internals
roadmap, the next increments are: the PSP + flat `.COM` program loader (over the
proven FAT12 read path); the SFT/JFT (initech-509.3) and the file-handle INT 21h
functions + find-first/next (completing initech-509.5); and SYSINIT + the
IO.SYS/INITDOS.SYS two-file partition (initech-509.2). The interactive shell follows
once the keyboard/CON-input path (initech-n62, initech-3rs) lands.

## 8. Follow-On Items

- **initech-509.5** remains open for the file-handle and CON-input functions.
- **initech-1f9** tracks operator ratification of DEC-04a (the convention + vector map).
- **initech-n62** (new) tracks CON input (01h/06h/0Ah) + the keyboard path; it now
  blocks initech-509.5.

---

*— End of Record —*

<!-- Tedium certified compliant with NFR-7. -->
