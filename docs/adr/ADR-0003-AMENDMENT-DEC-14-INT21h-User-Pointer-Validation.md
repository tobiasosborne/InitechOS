<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0003 Amendment DEC-14 — INT 21h User-Pointer Validation (Fail-Loud Buffer Guards)

**Issuing Body:** Initech Systems Corporation — Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record Amendment (ADR-A)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0003-A2 |
| Title | ADR-0003 Amendment DEC-14: INT 21h User-Pointer Validation |
| Version | 1.0 |
| Status | **Ratified** |
| Classification | Internal Use Only |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | STAPLER Programme (kernel-hardening initiative) |
| Effective Date | 2026-06-13 |
| Supersedes | (none; extends DEC-04a's INT 21h ABI) |
| Related Documents | ADR-0003 (OEA-ADR-0003); ADR-0003 Amendment DEC-04a (OEA-ADR-0003-A1) |
| Related Issues | beads initech-tzq (decision + implementation); initech-bcg (epic) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |

---

## 1. Context

The wave-B INT 21h error-path suite (beads initech-xrd) confirmed that three
INT 21h handlers dereference the caller's `EDX` pointer with no validation:
`do_read` (0x3F) and `do_write` (0x40) hand `EDX` to the file backend / `con_putc`
loop, and `do_setdta` (0x1A) arms a later FINDFIRST/FINDNEXT to write a 43-byte
`find_data_t` through whatever DTA was set. An uninitialized caller pointer
(`EDX == 0`) or a byte count that overflows the 32-bit address space therefore
faults or silently corrupts memory rather than failing loud.

Real MS-DOS does not validate INT 21h pointers — it runs at CPL=0 and trusts the
caller. InitechOS is the **foundation** the Toolbox, the bundled apps, and Turbo
Initech are built on; a corrupt framebuffer or scribbled kernel structure from
one bad pointer is exactly the class of defect CLAUDE.md Rule 2 ("fail fast, fail
loud") exists to prevent. This is a deliberate, documented divergence from DOS
authenticity in favour of a bulletproof base (Law 4 fidelity is preserved for
*observable* behaviour; a program that passes a valid pointer sees identical
results).

## 2. Decision

INT 21h buffer-taking calls **validate the user buffer before any access** and
fail loud on a bad pointer:

- A call whose buffer `[EDX, EDX+CX)` is **NULL** (`EDX == 0`, `CX > 0`) or whose
  range **wraps the 32-bit address space** (`EDX + CX < EDX`) returns **CF=1,
  AX=0x0009** (INVALID MEMORY BLOCK ADDRESS — the standard MS-DOS code, newly
  added to the error enum) and performs **no** read/write through the pointer.
- A **zero count** (`CX == 0`) never touches memory and always succeeds (CF clear,
  EAX=0), preserving the DOS contract.
- Applies to `do_read`, `do_write`, and the FINDFIRST/FINDNEXT DTA write
  (`emit_find_data`, validated against `sizeof(find_data_t)`). `do_setdta` itself
  keeps the DOS "no error path" contract (1Ah always succeeds); a wild DTA is
  caught at the write site, and the existing NULL-DTA → PSP:0x80 fallback is
  preserved.

**Scope of the guard (DEC-14.1):** the validated properties are NULL and 32-bit
wrap — both meaningful on the flat target *and* exercisable by the host unit
tests (whose buffers legitimately live at host addresses above any conventional-
RAM ceiling, so a fixed arena-ceiling check is intentionally **not** part of this
decision; it would reject valid host-test buffers and is a target-only concern
deferred unless an emulator oracle is built for it).

## 3. Consequences

- **C-1:** A new error code `INT21_ERR_INVALID_MEMORY = 0x0009` enters the INT 21h
  error vocabulary (gap between 0x0008 and 0x000B; matches MS-DOS 0x09).
- **C-2:** Behavioural change vs DEC-04a only on the error path — a previously
  faulting/corrupting bad-pointer call now returns CF=1/AX=0x0009. Valid calls are
  bit-identical. No emulator gate regresses (verified).
- **C-3:** The guard is mutation-proven (test-int21-edge): a build with the guard
  removed SIGSEGVs on a NULL read of a non-empty file — the very fault the guard
  prevents — which the mutant oracle reads as RED.

## 4. Oracle

`make test-int21-edge` + `test-int21-edge-mutant`: a NULL/`CX`-overflow buffer on
a *valid* handle returns CF=1, AX=0x0009; a valid buffer is unaffected; a wild
DTA makes FINDFIRST return the error; the no-guard mutant goes RED.
