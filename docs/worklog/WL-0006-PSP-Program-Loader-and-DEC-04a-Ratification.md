<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0006 — Programme Engineering Work Record (PEWR)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Engineering Work Record (Worklog)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | PE-WL-0006 |
| Title | PSP Construction, the Flat Program Loader, and the Ratification of DEC-04a |
| Version | 1.0 |
| Status | Issued |
| Classification | Internal Use Only |
| Period Covered | 2026-06-08 |
| Recording Function | Build Orchestration (supervised multi-agent) |
| Related | WL-0005; ADR-0003 (DEC-05, App B.2); ADR-0003 Amendment DEC-04a; beads initech-509.4, initech-509.5, initech-1f9, initech-we2, initech-xk2, initech-saw |

---

## 1. Purpose

This Record memorializes two concurrent threads: (a) the ratification, by a
delegated Architecture Review Board committee, of ADR-0003 amendment **DEC-04a**
(the 32-bit-flat INT 21h calling convention and IDT/PIC vector map); and (b) the
implementation of Program Segment Prefix construction and a flat program loader —
the increment at which **InitechDOS executes a program for the first time**.

## 2. The Ratification Committee (DEC-04a)

At operator direction, ratification was delegated to a committee of subagents. Three
reviewers examined the provisional convention independently, each through a distinct
lens, and a chair synthesised the ratified record:

- **Technical / Correctness (Concur-with-comment):** certified the IDT gate-type
  policy, the PIC remap arithmetic, the register-frame layout, and the
  carry-flag-via-saved-EFLAGS mechanism — and **found a real defect**: `do_getver`
  (AH=30h) cleared the wrong byte for the BH/OEM field (masking bits 31:24 rather
  than 15:8). The defect was fixed and a host-test assertion added that bites it
  before ratification proceeded.
- **Period Authenticity (Concur-with-comment):** certified fidelity to DOS 3.3 —
  the literal `int 0x21`, the AH function numbers, the `$` string terminator, the
  carry-flag error idiom, and the version-byte order — with no changes required.
- **Governance / Compliance (Concur-with-comment):** certified controlled scope and
  specs-as-data conformance, and enumerated the controlled-record contents the
  amendment had to carry.

The committee's value was demonstrated, not assumed: an independent review surfaced
a correctness bug that the existing oracle had not caught, and closing that oracle
gap (Rule 6) was a precondition of sign-off.

The ratified amendment is `docs/adr/ADR-0003-AMENDMENT-DEC-04a-INT21h-Flat-Calling-Convention.md`
(full house style, with the ARB sign-off matrix). `spec/int21h_calling_convention.json`
was advanced from provisional to ratified. Two forward obligations the record
captures were filed as beads: the DPL=0→DPL=3 gate upgrade for future ring-3 callers
(initech-we2) and INT 21h reentrancy hardening once IRQs are unmasked (initech-xk2).

## 3. PSP Construction (initech-509.4, closed)

`os/milton/psp.{h,c}` implement `psp_build()` — a pure, freestanding, host-testable
constructor that writes all 256 bytes of the locked `psp_t` per Appendix B.2: the
`INT 20h` instruction at offset 0, the `INT 21h; RETF` entry at 0x50, the Job File
Table (standard handles + 0xFF unused), the command-tail/DTA region, and the
vestigial segment fields rendered in flat mode as caller-supplied
flat-address-in-paragraph-units (the design stance: vestigial structures implemented
*in full*, not elided). The deferred fields (saved INT 22/23/24 vectors; the two
FCBs) are zero-filled with their successor beads noted (509.8, 509.9). Oracle:
`make test-psp` (78 checks, RED→GREEN, mutation-proven).

## 4. The Flat Program Loader (advances initech-509.5 / the f8v.4 keystone)

`os/milton/loader.{h,c}` lay out a program at the locked memory map
(`spec/memory_map.h`: PSP at 0x20000, image at 0x20100 preserving the authentic
`.COM` offset, environment at 0x20200, program stack top at 0x6FFFC), build the PSP,
save the loader context, and transfer control by `JMP`. The program's termination
(`INT 21h` AH=4Ch, or the newly-installed `INT 20h` trap gate at the now-free vector
0x20) routes through an exit hook that performs a **non-returning** assembly stack
restore back into `load_program()`, which returns the exit code — the return-to-loader
mechanism. A real register-collision defect in the restore sequence was caught and
root-caused during bring-up (Rule 3) and the context-field offsets are now pinned by
`_Static_assert`.

A baked flat test program (`os/milton/test_program.asm`, assembled `org 0x20100` and
embedded deterministically via `tools/bin2c.c`) prints a string through `INT 21h`
AH=09h and exits AH=4Ch. Sourcing programs from the FAT12 volume over ATA is
deferred (initech-saw) and will, when it lands, also validate `ata.c` on the
emulator for the first time.

## 5. Oracle Disposition (Law 2) — and the visible result

- **Host:** `make test-loader` (21 checks; the 0x100 image-offset is mutation-proven)
  and `make test-psp` (above).
- **In-emulator:** `make test-program` asserts, on the real boot chain, that the
  loaded program prints its line through `INT 21h` **and** that control returns to
  the loader (`PROGRAM-EXIT rc=0`) with no triple-fault. The return-to-loader path
  was mutation-proven (breaking the saved return point makes the gate go red).
- **Visible (Law 4):** the boot screendump now shows three lines — the two banner
  lines and `Hello from InitechOS program.`, the third printed by a loaded program
  calling the operating system.

## 6. Verification of Record

At close of period the full gate vector passes without exception: `test-spec`,
`test-psp`(+mutant), `test-idt`(+mutant), `test-int21`(+mutant), `test-loader`(+mutant),
`test-console`, `test-fat`, `test-harness`, `test-tracer-boot`, `test-boot`,
`test-panic`, and `test-program`. No triple-fault occurs. New sources are ASCII-clean
and compile both freestanding and hosted; binaries are reproducible.

## 7. Phase Disposition

InitechDOS now boots, presents its banner, exposes a working `int 0x21` system-call
surface, and **loads and runs a program that calls back into the OS and returns
cleanly**. Per the internals roadmap, the remaining internals are the SFT/JFT and the
file-handle INT 21h functions + find-first/next (initech-509.3, completing 509.5),
and SYSINIT + the IO.SYS/INITDOS.SYS two-file partition (initech-509.2). The shell
(initech-7pc) follows, gated on CON input / the keyboard (initech-n62, initech-3rs).
FAT-sourced program loading (initech-saw) is the bridge to the f8v.4 keystone
(DIR/TYPE over a FAT volume).

## 8. Follow-On Items

- **initech-509.5** remains open for file-handle + CON-input functions and EXEC from
  a real file.
- **initech-we2 / initech-xk2** — the two DEC-04a forward obligations (ring-3 DPL
  upgrade; reentrancy hardening).
- **initech-saw** (new) — FAT-sourced program loading (connects loader ↔ fat12 ↔
  ata; validates ata.c on the emulator).

---

*— End of Record —*

<!-- Tedium certified compliant with NFR-7. -->
