<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0003 Amendment DEC-04a — INT 21h Flat 32-Bit Calling Convention and IDT/PIC Vector Map

**Issuing Body:** Initech Systems Corporation — Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record Amendment (ADR-A)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0003-A1 |
| Title | ADR-0003 Amendment DEC-04a: INT 21h Flat 32-Bit Calling Convention and IDT/PIC Vector Map |
| Version | 1.0 |
| Status | **Ratified** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Architecture Review Board, STAPLER Programme |
| Effective Date | 2026-06-08 |
| Next Scheduled Review | 2026-12-08 (semi-annual, per RECORDS-POL-002) |
| Supersedes | (partial; see §6 — refines DEC-04 for the 32-bit-flat implementation) |
| Superseded By | (none at time of ratification) |
| Related Documents | ADR-0003 (OEA-ADR-0003); CDR-0001 (OEA-CDR-0001) |
| Related Issues | beads initech-1f9 (ratification tracking); beads initech-509.5 (implementation) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | 2026-06-08 | P. Gibbons (Software Engineering II) | Initial draft for ARB committee review. Incorporates ground-truth brief (docs/research/internals-int21h-ground-truth.md) and provisional spec-data (spec/int21h_calling_convention.json). | — |
| 1.0 | 2026-06-08 | ARB Chair (synthesis) | Ratified following committee review. Bug fix (do_getver BH masking error, found in review) confirmed resolved prior to ratification. Status updated to Ratified. | ARB (full committee) |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter | P. Gibbons (Software Engineering II) | Submitted | 2026-06-08 |
| ARB Reviewer — Technical Correctness | M. Bolton (Senior Engineer, Platform) | Concur-with-comment | 2026-06-08 |
| ARB Reviewer — Period Authenticity | S. Nagheenanajar (Engineering, Heritage Conformance) | Concur-with-comment | 2026-06-08 |
| ARB Reviewer — Governance & Compliance | T. Smykowski (QA / Change Advisory) | Concur-with-comment | 2026-06-08 |
| ARB Chair (Synthesis) | Slydell & Porter Consulting (delegated per beads initech-1f9) | Ratified | 2026-06-08 |
| Operator Delegation | T. Osborne (Operator) | Delegated to ARB per beads initech-1f9 | 2026-06-08 |
| Records Management | M. Waddams (Archive Annex B) | Filed | 2026-06-08 |

*Note on committee composition: The Technical/Correctness, Period-Authenticity, and Governance/Compliance reviewers correspond to the in-programme engineering functions designated in ADR-0003 §1.3. All three submitted independent findings; the Chair synthesized concurrences and recorded the resulting ratified amendment. The operator delegated ratification authority to the committee per beads initech-1f9; no separate operator sign-off is required for this amendment.*

---

## 1. Purpose and Scope

### 1.1 Purpose

The purpose of this Amendment (the "Amendment" or "DEC-04a") is to supplement and refine Sub-Decision DEC-04 of ADR-0003 (OEA-ADR-0003, ratified 2026-06-08) with respect to the concrete 32-bit-flat implementation of the `INT 21h` system-call interface. DEC-04 (ADR-0003 §5.4) established the principle that the system-call surface shall be presented via interrupt `21h` with the function selected by register `AH`. That Sub-Decision did not, and was not intended to, specify the 32-bit-flat register-level calling convention, the interrupt-gate type and privilege level, or the Programmable Interrupt Controller (PIC) vector remapping required to keep vector `0x21` available for the software interrupt.

This Amendment ratifies those three categories of architectural choice as binding decisions upon the implementation, locks the associated spec-data, and records the forward obligations arising therefrom.

### 1.2 Scope

This Amendment governs:

- The IDT gate type, descriptor privilege level (DPL), and selector for the INT 21h software-interrupt handler (Sub-Decision DEC-04a.1).
- The 8259A PIC initialization command word (ICW) vector bases for the master and slave controllers, and the motivation therefor (Sub-Decision DEC-04a.2).
- The flat 32-bit register-level calling convention for INT 21h: function selector register, pointer-argument register, count register, handle register, return-value register, and carry-flag error-reporting mechanism (Sub-Decision DEC-04a.3).

### 1.3 Out of Scope

The following are expressly out of scope of this Amendment:

- The complete INT 21h function register (governed by ADR-0003 Appendix A, which is unchanged).
- The program-loading mechanism, memory arena, PSP construction, and all other architectural elements addressed under ADR-0003 §5.
- The ring-3 privilege transition for Turbo Initech user programs (ADR-0007, pending); see Consequence C-4 below.
- The keyboard device driver and IRQ1 handler implementation; those are deferred to the applicable device-driver milestone.

### 1.4 Additional Defined Terms

The following terms, when used herein, supplement the definitions in ADR-0003 §1.4.

| Term | Definition |
|---|---|
| CF | Carry Flag. Bit 0 of EFLAGS. Used as the error indicator in the DOS INT 21h convention. |
| CPL | Current Privilege Level. The privilege level at which executing code runs (ring 0 to ring 3). |
| DPL | Descriptor Privilege Level. The privilege level encoded in an IDT gate descriptor controlling which CPL may invoke it via a software INT instruction. |
| ICW | Initialization Command Word. One of four sequential control words used to initialize an Intel 8259A PIC. |
| IDT | Interrupt Descriptor Table. The protected-mode table mapping vector numbers to gate descriptors. |
| IDTR | IDT Register. The CPU register holding the IDT base and limit, loaded via the LIDT instruction. |
| int_frame_t | The C structure describing the register state saved on the kernel stack by the INT 21h entry stub, as specified in Appendix E of this Amendment. |
| IRQ | Interrupt Request. A hardware interrupt signal delivered through the 8259A PIC. |
| PIC | Programmable Interrupt Controller. The Intel 8259A (or compatible) device that arbitrates and delivers hardware interrupt requests to the CPU. |
| Trap Gate | A 32-bit IDT gate descriptor of type 0xF (type byte 0x8F at DPL=0). Unlike an interrupt gate, a trap gate does not clear the Interrupt Flag (IF) on entry. |

---

## 2. Context

### 2.1 What DEC-04 Established and What It Left Open

ADR-0003 DEC-04 ratified the principle of the INT 21h system-call surface: interrupt vector `21h`; function selector in `AH`; the complete set of functions catalogued in ADR-0003 Appendix A. DEC-04 was deliberately platform-agnostic at the register-convention level because, at the time of ratification, the implementation of the kernel interrupt infrastructure had not commenced.

Subsequent implementation work, documented in the ground-truth research brief (`docs/research/internals-int21h-ground-truth.md`), identified three categories of decision that are not derivable from existing ratified documents and that have material, non-reversible consequences for the entire INT 21h implementation surface:

1. **IDT gate type and DPL for vector 0x21.** The choice between an interrupt gate and a trap gate determines Interrupt Flag behavior on syscall entry; the DPL determines which privilege level may issue the software interrupt without a General Protection Fault (#GP).
2. **PIC ICW vector-base selection.** The conventional master-PIC base of `0x20` (in use by many protected-mode systems) causes IRQ1 (keyboard) to be delivered at vector `0x21`, directly colliding with the INT 21h software-interrupt gate. This is not a latent or theoretical conflict: when the keyboard IRQ is unmasked, any keystroke invokes the INT 21h dispatcher with arbitrary register state, producing incorrect system-call dispatch, EAX/EBX/EDX corruption, and an erroneous `iretd` return. The resolution — remapping the master PIC base to `0x28` — is a non-obvious departure from common practice that must be explicitly ratified.
3. **Flat 32-bit register-level calling convention.** DOS 3.3 INT 21h uses 16-bit registers and segment:offset pointer pairs. InitechDOS targets flat 32-bit native programs (ADR-0003 DEC-02, DEC-08); there are no usable segments in the flat model. The 32-bit adaptation (EDX as flat linear pointer, ECX as count, EBX as handle, EAX for return value, CF-via-saved-EFLAGS for error signaling) constitutes a new convention not specified in any prior ratified document.

The ground-truth brief recommended that these decisions be ratified as DEC-04a (an amendment to ADR-0003) or as a new ADR-0005. The Architecture Review Board elected the amendment path, as DEC-04a refines an existing sub-decision in ADR-0003 without disturbing any other sub-decision.

### 2.2 Relationship to the Locked Spec-Data

The provisional spec-data file `spec/int21h_calling_convention.json` was created under beads initech-1f9 and beads initech-509.5 as the implementation vehicle for the convention described in the ground-truth brief. That file carried a `"status"` field of `"provisional -- pending ADR-0003 amendment DEC-04a ratification (beads initech-1f9)"`. Upon ratification of this Amendment, that status is updated to reflect ratification (see §7). The file is the locked authoritative contract per CLAUDE.md Rule 8; this Amendment is the governance record that gives it that status.

---

## 3. The Decision (DEC-04a)

It is hereby recorded that the following three Sub-Decisions collectively constitute Amendment DEC-04a to ADR-0003. Each is binding upon the implementation unless and until amended through the change-control process. For the avoidance of doubt, these Sub-Decisions refine but do not supersede DEC-04; ADR-0003 Appendix A (the INT 21h function register) is unchanged.

### 3.1 Sub-Decision DEC-04a.1 — IDT Gate Type, DPL, and Selector for INT 21h

The INT 21h handler shall be installed in the Interrupt Descriptor Table at vector `0x21` as a **32-bit trap gate** with the following descriptor attributes:

| Attribute | Value | Rationale |
|---|---|---|
| Gate type | `0x8F` (32-bit trap gate, type=1111b) | Leaves IF unchanged on entry; permits future timer-tick delivery during long console I/O without requiring explicit STI inside the dispatcher. Preferred over interrupt gate (0x8E) for the syscall vector. |
| DPL | 0 (ring-0) | InitechOS applications run at CPL=0 in the current release (ADR-0003 DEC-02; no ring-3 transition yet). DPL must equal the caller's CPL; DPL=0 is therefore correct for the current release. |
| Selector | `0x08` (CODE_SEL) | The flat ring-0 code segment established in ADR-0001 and confirmed in `os/boot/stage2.asm` GDT at the ratified base. |

CPU exception handlers (vectors 0-31) and hardware IRQ handlers (vectors 0x28-0x2F for master IRQs, 0x30-0x37 for slave IRQs — see DEC-04a.2) shall be installed as **32-bit interrupt gates** (`0x8E`, DPL=0), which automatically clear IF on entry to prevent nested interrupt delivery during exception and IRQ dispatch.

### 3.2 Sub-Decision DEC-04a.2 — PIC Remapping: Master Base 0x28, Slave Base 0x30

The Intel 8259A master PIC shall be remapped to deliver IRQ0-7 at vectors **`0x28`-`0x2F`**. The slave PIC shall be remapped to deliver IRQ8-15 at vectors **`0x30`-`0x37`**. The 8259A initialization sequence shall use ICW2 values of `0x28` (master) and `0x30` (slave), with ICW1=`0x11` (cascade, ICW4 needed, edge-triggered), ICW3=`0x04`/`0x02` (master/slave cascade identity), and ICW4=`0x01` (8086 mode, no auto-EOI).

This remap choice is the direct consequence of the INT 21h/IRQ1 vector collision described in §2.1: with the conventional master base of `0x20`, IRQ1 (keyboard) would be delivered at vector `0x21`, sharing the INT 21h gate. The non-conventional master base `0x28` displaces all master IRQs upward by eight vectors, leaving `0x21` exclusively available for the software-interrupt gate. The complete post-remap vector map is provided in Appendix E, §E.4.

The vector range `0x28`-`0x37` is reserved for hardware IRQ delivery and shall not be used for any software-interrupt gate. All IRQs shall be fully masked (OCW1=`0xFF` to both PICs) during IDT initialization; individual IRQs are unmasked only when the corresponding device driver installs its handler.

### 3.3 Sub-Decision DEC-04a.3 — Flat 32-Bit Register-Level Calling Convention

The INT 21h calling convention for InitechOS flat 32-bit programs shall be as follows. The complete ABI table is reproduced in Appendix E, §E.1.

| Role | Register | Notes |
|---|---|---|
| Function selector | AH (bits 15:8 of EAX) | Preserves DOS mnemonic compatibility with the locked register (ADR-0003 Appendix A; spec/int21h_register.json). AL carries sub-function or input byte as in DOS 3.3. |
| Primary pointer argument | EDX (flat 32-bit linear address) | DOS used DS:DX; in flat mode DS is always base-0 (DATA_SEL), so EDX is the flat pointer. No segment:offset arithmetic. |
| Byte count / secondary argument | ECX | DOS used CX for count (e.g. INT 21h AH=40h: CX=count). Extended to 32 bits. |
| Handle / sub-selector | EBX | DOS used BX for file handles. |
| Return value | EAX (32-bit) | AL for single-byte returns; AX for 16-bit legacy results; EAX for 32-bit quantities. |
| Error flag | CF in saved EFLAGS | CF=0 on success; CF=1 on error with AX=error code. The dispatcher writes bit 0 of the saved EFLAGS field in int_frame_t before IRETD. POPAD does not restore EFLAGS; IRETD restores the modified saved EFLAGS, propagating CF to the caller. |

**Pointer arguments are flat 32-bit linear addresses throughout.** No far-pointer (segment:offset) arithmetic occurs. This is a binding consequence of ADR-0003 DEC-02 (personality, not binary compatibility) and DEC-08 (flat executables).

**Controlled scope for unlisted and deferred functions:**

- An AH value that is **listed in the INT 21h register** (`spec/int21h_register.json`) but **not yet implemented** shall return CF=1, AX=`0x0001` (invalid function), with serial diagnostic `INT21 not-yet-impl AH=NN`. This is a distinct, recognized response — not "unknown function."
- An AH value that is **not listed in the INT 21h register** shall return CF=1, AX=`0x0001`, with serial diagnostic `INT21 unknown AH=NN` and (where the console is bound) MSG-DOS-0002 (`Bad command or file name`). Per ADR-0003 DEC-13 (controlled vocabulary) and CLAUDE.md Rule 2 (fail loud), a silent no-op or hang is forbidden in either case.

---

## 4. Rationale

### 4.1 Trap Gate vs. Interrupt Gate for INT 21h (DEC-04a.1)

The distinction between the two gate types is IF behavior on entry: an interrupt gate clears IF (disabling further hardware interrupts); a trap gate leaves IF unchanged. For CPU exceptions and hardware IRQ handlers, an interrupt gate (0x8E) is appropriate and required — exception dispatch must not be interrupted, and IRQ handlers issue EOI before returning. For the INT 21h software-interrupt gate, a trap gate (0x8F) is the more correct choice: in the cooperative, non-preemptive scheduling model of InitechOS (CLAUDE.md hallucination callout; no preemption is a design invariant), there is no interrupt-storm risk from leaving IF enabled during a syscall, and the trap-gate choice avoids the need for explicit STI/CLI pairs inside the dispatcher as future implementations add timer-dependent console I/O.

### 4.2 DPL=0 for the Current Release (DEC-04a.1 Forward Obligation)

The current release of InitechOS operates in a ring-0-only model (ADR-0003 DEC-02; no ring-3 privilege transition). Applications and the kernel share CPL=0. A software INT instruction requires that the CPL be less than or equal to the gate's DPL; with both caller and gate at ring-0, DPL=0 is therefore exactly correct. A DPL=3 gate would not fail on a ring-0 caller, but it is unnecessary and would permit ring-3 code — which does not yet exist — to invoke the gate, which would be a latent privilege issue. DPL=0 is conservative and correct for the current release.

When ring-3 support is introduced for Turbo Initech user programs (ADR-0007, pending), the DPL of the INT 21h gate **must** be upgraded to DPL=3 before any ring-3 program issues `INT 0x21`. A ring-3 program calling `INT 0x21` into a DPL=0 gate generates a #GP (General Protection Fault), which would surface as a cascading exception and a `PC LOAD LETTER` panic — a correct but unwanted outcome. This upgrade is a forward obligation recorded in §5 (Consequence C-4).

### 4.3 PIC Remap Base Selection — The IRQ1/INT 21h Collision (DEC-04a.2)

The BIOS initializes the master 8259A to deliver IRQ0-7 at vectors `0x08`-`0x0F`, directly overlapping CPU exception vectors 8-15. Protected-mode initialization requires remapping the PIC before STI (CLAUDE.md hallucination callout on triple-fault behavior in QEMU). The most common remap in contemporary practice (e.g. Linux x86) maps master IRQs to `0x20`-`0x27` and slave IRQs to `0x28`-`0x2F`.

This conventional remap, however, maps IRQ1 (keyboard) to vector `0x21`. An INT 21h gate installed at vector `0x21` would then be invoked both by software `INT 0x21` instructions and by hardware keyboard interrupts. These two invocation paths are not distinguishable by the gate handler; both deliver the CPU to the same handler entry point with the caller's register state. Hardware keyboard delivery with arbitrary EAX state would produce incorrect AH dispatch, corrupting EBX/EDX/ECX and returning via IRETD to a keyboard interrupt context that expected a hardware-IRQ return protocol (including EOI), not a syscall return. This is documented as Risk 3 in the ground-truth brief (§8) and confirmed by the Technical/Correctness reviewer.

The resolution is to remap the master PIC base to `0x28` (shifting master IRQs to `0x28`-`0x2F`) and the slave to `0x30` (slave IRQs to `0x30`-`0x37`). Vector `0x21` is then exclusively a software-interrupt vector, free from any hardware IRQ delivery. This remap is non-standard relative to common practice and is therefore ratified explicitly here rather than left as an implementation detail.

### 4.4 CF-via-Saved-EFLAGS (DEC-04a.3)

The DOS 3.3 `INT 0x21 / JC error` idiom, used uniformly across the function register, depends on CF being set in the caller's EFLAGS upon return from the syscall. The IRETD instruction restores the saved EFLAGS; POPAD does not restore EFLAGS. The dispatcher must therefore write CF into the saved EFLAGS field of `int_frame_t` before POPAD/IRETD. This is the only correct mechanism for CF propagation in a 32-bit protected-mode INT handler; any alternative (e.g. returning CF in a general-purpose register) would break the DOS idiom and require a non-standard wrapper in every caller, including future Turbo Initech-compiled programs (ADR-0007).

### 4.5 Period Authenticity (DEC-04a.3)

The Period-Authenticity reviewer confirmed, without required change, that the literal `INT 0x21` software-interrupt vector, the AH function-selector values (02h/09h/30h/40h/4Ch), the `$` (0x24) string terminator, the carry-flag error idiom, and the GETVER AL=major/AH=minor ordering all match the DOS 3.3 Programmer's Reference Manual. The 32-bit-flat adaptation (EDX as flat pointer, ECX as count, EBX as handle) preserves the register-mnemonic alignment while substituting the flat linear address for the segment:offset pair — the only adaptation mandated by the flat memory model (ADR-0003 DEC-02, DEC-08). No anachronism is introduced (NFR-1).

---

## 5. Consequences

### 5.1 Binding Constraints

**C-1 — PIC base is architecturally binding.** The master-PIC base `0x28` and slave-PIC base `0x30` are established by this Amendment as binding constants. Any future IRQ handler must be installed at the correct offset within these remapped ranges (IRQ0 at `0x28`, IRQ1 at `0x29`, etc.). No implementation may remap the PIC to a different base without a further amendment and a consequent review of all installed IDT gates.

**C-2 — INT 21h gate at 0x21 with type 0x8F, DPL=0, selector 0x08.** These gate attributes are binding for the current release. The gate must be installed after PIC remap and IDT population, before STI.

**C-3 — int_frame_t layout is locked spec-data.** The field layout of `int_frame_t` (Appendix E, §E.3) is locked as spec-data in `spec/int21h_calling_convention.json` (CLAUDE.md Rule 8). Any modification to the asm entry stub's push sequence must be accompanied by a corresponding update to the struct layout, a further amendment to this record, and a re-run of the test-spec oracle and the mutation oracle for `test_int21`. Changing the struct layout silently breaks the register-argument extraction in the C dispatcher.

### 5.2 Forward Obligations

**C-4 — DPL=0 to DPL=3 upgrade required before ring-3 programs issue INT 21h.** When ring-3 privilege-level support is introduced for Turbo Initech user programs (ADR-0007, pending), the INT 21h IDT gate DPL **must** be upgraded to DPL=3. A ring-3 program calling `INT 0x21` into a DPL=0 gate generates a #GP. This upgrade shall be tracked as a dependency of the ring-3 milestone and must be ratified by a further amendment. It is not optional. The Technical/Correctness reviewer flagged this obligation explicitly.

**C-5 — Reentrancy discipline when IRQs are unmasked.** The current implementation operates with all IRQs masked. When IRQ0 (PIT timer) or IRQ1 (keyboard) are later unmasked for device-driver use, the INT 21h dispatcher must be audited for reentrancy: either CLI/STI pairs at entry/exit of the dispatcher to prevent hardware IRQ delivery during syscall processing, or a semaphore-based in-dispatcher lock. The trap-gate choice (DEC-04a.1) leaves IF enabled on entry; this is correct and intentional, but it means the dispatcher is immediately exposed to hardware IRQ delivery once any IRQ is unmasked. The Technical/Correctness reviewer flagged this obligation explicitly. Resolution shall be selected and ratified at the time of the first IRQ-unmasking milestone.

### 5.3 Neutral Consequences

- ADR-0003 Appendix A (the INT 21h function register) is unchanged. DEC-04a adds implementation detail beneath DEC-04; it does not alter the function set.
- The vestigial structures (MCB arena, PSP, FCB model) and all other Sub-Decisions of ADR-0003 are unaffected.
- No change is anticipated to the procurement of foam packaging inserts.

---

## 6. Relationship to DEC-04 and to ADR-0003 Appendix A

**DEC-04a refines DEC-04; it does not supersede it.** DEC-04 (ADR-0003 §5.4) established the INT 21h system-call surface, the AH function-selector principle, the dual file API, and the reference to Appendix A. DEC-04a adds the concrete 32-bit-flat implementation details — gate type, PIC base, register convention — that DEC-04 deliberately left open. Both sub-decisions coexist; DEC-04 governs what the INT 21h surface provides; DEC-04a governs how it is invoked and dispatched at the machine level in the 32-bit-flat context.

**ADR-0003 Appendix A is unchanged and remains platform-agnostic.** Appendix A lists the sanctioned INT 21h function set (AH values, mnemonics, descriptions, conformance classes). It does not specify register conventions, gate types, or PIC configuration; those were always implementation details for the applicable platform ADR or amendment. Appendix A shall not be modified by this Amendment; any change to the function set requires an amendment to ADR-0003 §5.4 and Appendix A through the standard change-control process.

---

## 7. Verification

### 7.1 Spec-Data Consistency Gate (test-spec)

The `make test-spec` oracle (ADR-0003 §7, Makefile target) includes a gate (test-spec step 2/5) that validates `spec/int21h_calling_convention.json`. Specifically, it asserts that every `ah` value documented in `spec/int21h_calling_convention.json` also exists in `spec/int21h_register.json` (the locked function register). This enforces CLAUDE.md Rule 8 (locked spec is the contract) and ADR-0003 DEC-13 (controlled vocabulary): the calling-convention spec may not document a function that is not in the sanctioned register, and the register is not orphaned from the convention spec.

`make test-spec` shall be green at all times. Any change to either spec file that causes `make test-spec` to fail is a non-conformance requiring remediation before merge.

### 7.2 INT 21h Functional Oracle (test_int21)

The `test_int21` host-test suite comprises 36 assertion checks covering the implemented function subset (AH=00h, 02h, 09h, 30h, 40h, 4Ch). The suite is run as part of `make test`. Key oracle properties verified by this suite:

- **PUTCHAR (AH=02h):** character is written to console; AL=DL on return; CF=0.
- **PUTS (AH=09h):** string walk terminates at `$` (0x24); `$` is not emitted; AL=0x24 on return; CF=0.
- **GETVER (AH=30h):** AL=3 (major); AH=0x1E=30 (minor); BH=0x00 (OEM); CF=0. See §7.3 for the BH masking bug found by committee review.
- **WRITE (AH=40h):** handles 1 and 2 succeed; any other handle returns CF=1, AX=0x0006; EAX=bytes written on success.
- **EXIT (AH=4Ch):** does not return to caller; serial EXIT marker emitted.
- **Unlisted AH:** CF=1, AX=0x0001 returned; serial diagnostic emitted; no hang or silent no-op.
- **CF propagation:** bit 0 of saved EFLAGS in `int_frame_t` is modified by the dispatcher and recovered via IRETD; CF is correctly visible to the caller after return.

### 7.3 Evidence of Oracle Effectiveness — BH Masking Bug Found and Fixed by Review

During ARB committee review (Technical/Correctness reviewer, 2026-06-08), a defect was identified in the `do_getver` implementation: the register mask used to clear the BH byte cleared bits 31:24 of EBX (the BH position in a little-endian 32-bit register was treated as the high byte) rather than bits 15:8 (the correct BH position in the 32-bit EBX layout). Specifically, the incorrect mask `ebx &= 0x00FFFFFF` was replaced with the correct mask `ebx &= 0xFFFF00FF`. A corresponding host-test assertion — `GETVER zeroes BH` — was added to `test_int21` and confirmed to fail against the defective implementation and pass against the corrected one. This constitutes the mutation-oracle evidence required by CLAUDE.md Rule 6: the golden has caught a real regression. The fix was applied prior to ratification of this Amendment; `test_int21` is 36 checks green.

### 7.4 Locked Spec Authority

`spec/int21h_calling_convention.json` is the **locked authoritative contract** for the INT 21h flat 32-bit calling convention, per CLAUDE.md Rule 8. Its ratified status (updated upon ratification of this Amendment) signals that:

- No implementation may deviate from the ABI, gate type, DPL, selector, or frame layout recorded therein without a further amendment to this record.
- Any proposed change to `spec/int21h_calling_convention.json` requires: (a) filing a beads issue; (b) authoring a further amendment; (c) ARB review; (d) a green `make test-spec` and a green mutation oracle before ratification.
- The spec file shall not be silently edited to make a test pass. Locked specs are strengthened, not relaxed (CLAUDE.md Stop Conditions).

---

## 8. Related Decisions and References

- ADR-0003 (OEA-ADR-0003) — InitechDOS Base Operating System Personality and Resident Kernel Architecture. *(Accepted; this Amendment refines DEC-04 thereof.)*
- CDR-0001 (OEA-CDR-0001) — Interim Implementation Toolchain Deviation. *(Accepted; unaffected by this Amendment.)*
- ADR-0007 — Turbo Initech Self-Hosting Compiler. *(Pending; ring-3 DPL obligation, Consequence C-4, is a dependency.)*
- beads initech-1f9 — Ratification tracking issue for DEC-04a.
- beads initech-509.5 — Implementation milestone for the INT 21h dispatcher and console subset.
- `spec/int21h_calling_convention.json` — Locked calling-convention spec-data ratified herein.
- `spec/int21h_register.json` — Locked function register (Appendix A companion spec-data).
- `docs/research/internals-int21h-ground-truth.md` — Technical ground-truth brief; primary source for §4.
- Intel 64 and IA-32 SDM Vol. 3A §6.10-6.12 (IDT, gate descriptors, exception handling).
- Intel 8259A Programmable Interrupt Controller datasheet (PIC ICW initialization).
- DOS 3.3 Programmer's Reference Manual (AH function mnemonics and semantics).
- INITECH-STD-0042 — Corporate Standard for the Authoring of Architecture Decision Records.
- RECORDS-POL-002 — Document Review and Retention Policy.

---

## Appendix E — INT 21h Flat 32-Bit Calling Convention (Normative)

*This Appendix is normative. It reproduces, in structured form, the binding specification established by DEC-04a.3, DEC-04a.1, and DEC-04a.2. In the event of any conflict between this Appendix and the prose of §3, §3 governs. In the event of any conflict between this Appendix and `spec/int21h_calling_convention.json`, the spec file is authoritative (§7.4).*

### E.1 ABI Register-Role Table

| Role | Register | Width | Notes |
|---|---|---|---|
| Function selector | AH | 8-bit (bits 15:8 of EAX) | Matches DOS 3.3 AH mnemonic convention. AL carries sub-function or input byte. |
| Primary pointer | EDX | 32-bit flat linear address | Replaces DS:DX of 16-bit DOS; DS is always base-0 (DATA_SEL) in flat mode. |
| Byte count | ECX | 32-bit | Extends DOS CX count to 32 bits. |
| Handle | EBX | 32-bit (low 16 bits meaningful for handle) | Extends DOS BX handle to 32 bits. |
| Return value | EAX | 32-bit | AL for single-byte; AX for 16-bit legacy results; EAX for 32-bit values. |
| Error flag | CF (bit 0 of EFLAGS) | 1-bit | CF=0 success; CF=1 error with AX=error code. Written to saved EFLAGS before IRETD. |

*Caller-saved registers:* EAX (return value), EFLAGS (CF). All other general-purpose registers (EBX, ECX, EDX, ESI, EDI, EBP) are preserved by the dispatcher across the INT 21h call unless they carry a return value for the specific function.

### E.2 IDT Gate-Type Policy

| Vector Range | Usage | Gate Type | Type Byte | DPL | Notes |
|---|---|---|---|---|---|
| 0x00-0x1F | CPU exceptions | 32-bit interrupt gate | 0x8E | 0 | Clears IF on entry. |
| 0x21 | INT 21h syscall | 32-bit trap gate | 0x8F | 0 | Leaves IF unchanged. DPL must be upgraded to 3 when ring-3 is introduced (C-4). |
| 0x28-0x2F | Master IRQs 0-7 | 32-bit interrupt gate | 0x8E | 0 | After PIC remap. Clears IF; handler must issue EOI before IRET. |
| 0x30-0x37 | Slave IRQs 8-15 | 32-bit interrupt gate | 0x8E | 0 | After PIC remap. Slave + master EOI required. |
| 0x20, 0x22-0x27, 0x29-0x2F, 0x38-0xFF | Unused / spurious | Spurious stub | 0x8E | 0 | Must point to a fail-loud stub (CLAUDE.md Rule 2); not a null entry. |

*Note: Vector 0x20 (IRQ0, PIT timer) is listed under the unused range above because it is masked and its handler is not yet implemented at the time of this Amendment. It shall be populated when the timer milestone lands.*

### E.3 int_frame_t Field/Offset Layout

The INT 21h entry stub pushes registers in the following order, producing the layout below as seen from ESP upward (field at lowest address = top of stack):

```
; Entry stub push sequence (NASM, 32-bit flat):
;   CPU has pushed (high to low): EFLAGS, CS, EIP
;   (No SS:ESP -- ring-0 to ring-0; no CPL change)
; Then stub executes:
;   pushad              ; pushes: EAX ECX EDX EBX ESP_pre EBP ESI EDI
;                       ; (Intel SDM Vol 2A PUSHAD order; EDI at lowest addr)
;   push ds
;   push es
;   push fs
;   push gs
;   push 0              ; dummy err_code (INT has no hardware error code)
;   push 0x21           ; vector sentinel
;   ...then push esp and call int21_dispatch(int_frame_t *)

Offset  Field         Bytes  Notes
------  -----------  -----  ---------------------------------------------------
 +0     vector        4     Stub-pushed sentinel (0x21); keeps layout uniform
                            with exception path which pushes the vector number.
 +4     err_code      4     Dummy 0 (INT 21h carries no hardware error code).
 +8     eip           4     CPU-pushed return address.
+12     cs            4     CPU-pushed (padded to 32 bits).
+16     eflags        4     CPU-pushed; dispatcher writes CF (bit 0) here before
                            POPAD/IRETD to propagate error status to caller.
+20     gs            4     Stub-pushed (push gs executed last in seg sequence).
+24     fs            4     Stub-pushed.
+28     es            4     Stub-pushed.
+32     ds            4     Stub-pushed.
+36     edi           4     PUSHAD (EDI at lowest address in PUSHAD sequence).
+40     esi           4     PUSHAD.
+44     ebp           4     PUSHAD.
+48     esp_saved     4     PUSHAD (pre-push ESP value; not used for return).
+52     ebx           4     PUSHAD. Handle argument; preserved across dispatch.
+56     edx           4     PUSHAD. Pointer argument; preserved across dispatch.
+60     ecx           4     PUSHAD. Count argument; preserved across dispatch.
+64     eax           4     PUSHAD (EAX at highest address in PUSHAD sequence).
                            Dispatcher writes return value here.
```

*Total frame size on stack: 68 bytes (17 x 4-byte fields).*

*The C struct `int_frame_t` in `os/milton/idt.h` mirrors this layout field-for-field. The `cf_bit` is bit 0 of the `eflags` field. Dispatcher sets CF: `r->eflags |= 0x1u`. Dispatcher clears CF: `r->eflags &= ~0x1u`. AH extraction: `uint8_t func = (uint8_t)(r->eax >> 8)`.*

*Risk note (ground-truth brief §8, Risk 2): A single field transposition between the asm push sequence and the C struct produces a silently-wrong dispatcher that reads the wrong register value. The mutation oracle in `test_int21` (§7.2) guards against this.*

### E.4 PIC Vector Map (Post-Remap)

```
Vector   Source            Handler requirement
------   ----------------  ---------------------------------------------------
0x00     #DE Divide Error  Exception -- interrupt gate, fail-loud, cli;hlt
0x01     #DB Debug         Exception -- interrupt gate, fail-loud, cli;hlt
0x02     NMI               Exception -- interrupt gate, fail-loud, cli;hlt
0x03     #BP Breakpoint    Exception -- interrupt gate, fail-loud, cli;hlt
0x04     #OF Overflow      Exception -- interrupt gate, fail-loud, cli;hlt
0x05     #BR BOUND         Exception -- interrupt gate, fail-loud, cli;hlt
0x06     #UD Invalid Op    Exception -- interrupt gate, fail-loud, cli;hlt
0x07     #NM No Math       Exception -- interrupt gate, fail-loud, cli;hlt
0x08     #DF Double Fault  Exception -- interrupt gate, err_code=0, fail-loud
0x09     Copr Seg Ovr      Exception (reserved 386) -- spurious stub
0x0A     #TS Invalid TSS   Exception -- interrupt gate, err_code, fail-loud
0x0B     #NP Seg NotPres   Exception -- interrupt gate, err_code, fail-loud
0x0C     #SS Stack Fault   Exception -- interrupt gate, err_code, fail-loud
0x0D     #GP Gen Protect   Exception -- interrupt gate, err_code, fail-loud
0x0E     #PF Page Fault    Exception -- interrupt gate, err_code, fail-loud
0x0F     (reserved)        Spurious stub
0x10     #MF x87 FP Err    Exception -- interrupt gate, fail-loud
0x11     #AC Align Check   Exception -- interrupt gate, err_code, fail-loud
0x12     #MC Machine Chk   Exception -- interrupt gate, fail-loud
0x13     #XM SIMD FP       Exception -- interrupt gate, fail-loud
0x14     #VE Virt Excp     Exception -- interrupt gate, fail-loud
0x15     #CP Ctrl Prot     Exception -- interrupt gate, err_code, fail-loud
0x16-0x20 (reserved)       Spurious stubs
0x21     INT 21h syscall   Trap gate (0x8F), DPL=0 -- INT 21h dispatcher ***
0x22-0x27 (unused)         Spurious stubs
0x28     IRQ0 (PIT timer)  Interrupt gate (0x8E) -- deferred; spurious stub now
0x29     IRQ1 (keyboard)   Interrupt gate (0x8E) -- deferred; spurious stub now
0x2A     IRQ2 (cascade)    Interrupt gate (0x8E) -- cascade, spurious stub
0x2B     IRQ3 (COM2)       Interrupt gate (0x8E) -- deferred; spurious stub now
0x2C     IRQ4 (COM1)       Interrupt gate (0x8E) -- deferred; spurious stub now
0x2D     IRQ5 (LPT2/free)  Interrupt gate (0x8E) -- deferred; spurious stub now
0x2E     IRQ6 (FDC)        Interrupt gate (0x8E) -- deferred; spurious stub now
0x2F     IRQ7 (LPT1/spur)  Interrupt gate (0x8E) -- deferred; spurious stub now
0x30     IRQ8  (RTC)       Interrupt gate (0x8E) -- deferred; spurious stub now
0x31     IRQ9  (free)      Interrupt gate (0x8E) -- deferred; spurious stub now
0x32     IRQ10 (free)      Interrupt gate (0x8E) -- deferred; spurious stub now
0x33     IRQ11 (free)      Interrupt gate (0x8E) -- deferred; spurious stub now
0x34     IRQ12 (PS/2 mouse) Interrupt gate (0x8E) -- deferred; spurious stub now
0x35     IRQ13 (FPU)       Interrupt gate (0x8E) -- deferred; spurious stub now
0x36     IRQ14 (IDE pri)   Interrupt gate (0x8E) -- deferred; spurious stub now
0x37     IRQ15 (IDE sec)   Interrupt gate (0x8E) -- deferred; spurious stub now
0x38-0xFF (unused)         Spurious stubs

*** The collision between INT 21h (0x21) and IRQ1 (keyboard, which maps to 0x21
    when master base=0x20) is resolved by the non-conventional master base 0x28
    ratified in DEC-04a.2. IRQ1 is displaced to 0x29; 0x21 is exclusively the
    INT 21h software-interrupt gate.
```

*Source: Intel SDM Vol. 3A Table 6-1 (exception vectors); Intel 8259A datasheet (ICW2 remap); ground-truth brief §3.1, §4.2, §5.1.*

---

*— End of Record —*

<!--
This document is the confidential and proprietary information of Initech Systems
Corporation. Unauthorized review, use, disclosure, or distribution is prohibited.
If you have received this document in error, please shred it and notify the Help
Desk (ext. 2504). This footer is part of the controlled document and shall not be
removed. Tedium certified compliant with NFR-7.
-->
