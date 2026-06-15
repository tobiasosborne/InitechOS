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
| Related Issues | beads initech-tzq (decision + implementation); initech-bcg (epic); initech-5o6o (DEC-14.2 CHMOD SET deviation record); initech-b53d (CHMOD impl) |
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

**Companion fail-loud deviation — CHMOD AH=43h SET attribute-type bits
(DEC-14.2):** Recorded here as a sibling to DEC-14 because it is the same class
of deliberate, documented divergence from DOS authenticity in favour of a
fail-loud base (Rule 2). Real DOS 3.3 (RBIL INT 21h/AX=4301h; DOS 3.3 PRM
Function 43h) **masks/ignores** a `CX` whose Directory (0x10) or Volume-Label
(0x08) bit is set — it silently drops the type bits and **succeeds** — and a
SET targeting a directory entry (type bit cleared in `CX`) **succeeds**,
changing only the other attribute bits. InitechOS instead **rejects** both
cases with **CF=1, AX=0x0005** (ACCESS DENIED): a SET whose `CX` sets the
Directory/VolLabel bit, and a SET against a directory or volume-label entry.
Re-typing a directory entry by way of `CHMOD` is exactly the silent dirent
corruption Rule 2 exists to prevent, so the kernel fails loud at the dispatch
edge (`do_chmod`) *and* again in the fat12 primitive (defense in depth).
Operator-approved 2026-06-15 (Q4 DOS-semantics fork). Registered in beads
initech-5o6o; implemented under initech-b53d (commits 76c4c8f + c7bffac).
Revisit only if a real DOS consumer needs DOS's mask-and-succeed behaviour.

*This deviation is SET-only.* A **GET** (AL=00) on a directory or volume-label
entry **succeeds** and returns the faithful attribute byte (`CX=0x10` for a
directory, `CX=0x08` for a volume label) — GET is a pure read and RBIL
AX=4300h has no directory exclusion. (An earlier b53d defect over-rejected
GET-on-directory with 0x0005; that was a separate, *unsanctioned* over-reject
corrected in the b53d follow-up commit c7bffac and is **not** part of this
intentional deviation — do not conflate the two.)

## 3. Consequences

- **C-1:** A new error code `INT21_ERR_INVALID_MEMORY = 0x0009` enters the INT 21h
  error vocabulary (gap between 0x0008 and 0x000B; matches MS-DOS 0x09).
- **C-2:** Behavioural change vs DEC-04a only on the error path — a previously
  faulting/corrupting bad-pointer call now returns CF=1/AX=0x0009. Valid calls are
  bit-identical. No emulator gate regresses (verified).
- **C-3:** The guard is mutation-proven (test-int21-edge): a build with the guard
  removed SIGSEGVs on a NULL read of a non-empty file — the very fault the guard
  prevents — which the mutant oracle reads as RED.
- **C-4 (DEC-14.2):** The CHMOD SET attribute-type reject (the dispatch-edge
  "defense in depth" guard in `do_chmod`) is mutation-proven under
  `make test-b53d-mutant` (MUTANT 8, `INT21_MUTATE_CHMOD_NO_CX_REJECT`, removes
  the `CX` type-bit reject branch; the host `test_fileio` oracle then goes RED).
  The mutant bites BOTH dispatch-edge legs INDEPENDENTLY -- a SET whose `CX`
  sets the Directory bit (`0x10`, on a file) AND a SET whose `CX` sets the
  VolLabel bit (`0x08`, on a *separate* plain file so the backend TARGET reject
  cannot mask the dropped dispatch guard). The accompanying host fileio suite
  also asserts the dir/vol-label SET TARGET reject and the GET-on-directory
  success (`CX=0x10`). No emulator gate regresses; valid attribute SETs are
  bit-identical to DOS.

## 4. Oracle

`make test-int21-edge` + `test-int21-edge-mutant`: a NULL/`CX`-overflow buffer on
a *valid* handle returns CF=1, AX=0x0009; a valid buffer is unaffected; a wild
DTA makes FINDFIRST return the error; the no-guard mutant goes RED.

For the companion CHMOD SET deviation (DEC-14.2): `make test-b53d` — the
attribute SET round-trips through `python --attr` + `mtools mattrib`, the
dir/vol-label SET reject and the GET-on-directory success (`CX=0x10`) are
asserted. The dispatch-edge `CX` re-typing reject is mutation-proven by
`make test-b53d-mutant` (MUTANT 8): the `INT21_MUTATE_CHMOD_NO_CX_REJECT` mutant
drops the `CX` type-bit reject and the host oracle goes RED on BOTH the
Directory-bit and VolLabel-bit dispatch-edge legs.
