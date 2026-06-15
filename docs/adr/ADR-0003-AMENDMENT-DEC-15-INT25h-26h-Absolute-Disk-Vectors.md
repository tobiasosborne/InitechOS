<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0003 Amendment DEC-15 -- INT 25h/26h Absolute Disk Read/Write Vectors

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record Amendment (ADR-A)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0003-A3 |
| Title | ADR-0003 Amendment DEC-15: INT 25h/26h Absolute Disk Read/Write Vectors |
| Version | 1.0 |
| Status | **Ratified** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | P. Gibbons (Software Engineering II) |
| Effective Date | 2026-06-15 |
| Next Scheduled Review | 2026-12-15 (semi-annual, per RECORDS-POL-002) |
| Supersedes | (none; adds two new software-interrupt vectors in the DEC-04a-reserved 0x22-0x27 band) |
| Superseded By | (none at time of ratification) |
| Related Documents | ADR-0003 (OEA-ADR-0003); ADR-0003 Amendment DEC-04a (OEA-ADR-0003-A1); ADR-0003 Amendment DEC-14 (OEA-ADR-0003-A2) |
| Related Issues | beads initech-4mq7 (decision + implementation); initech-40oq (capstone scope note); initech-bsy (epic); initech-8479 (external-utilities consumer epic) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | 2026-06-15 | P. Gibbons (Software Engineering II) | Initial draft for ARB committee review. Incorporates the DOS 3.3 Programmer's Reference Manual INT 25h/26h ABI, the DEC-04a vector map (Appendix E.4), and the DEC-14 user-pointer-validation precedent. | -- |
| 1.0 | 2026-06-15 | ARB Chair (synthesis) | Ratified following committee review. Reconciles the DOS-3.3 compatibility, kernel-architecture/vector-allocation, scope/house-style, and oracle/reproducibility assessments. The leftover-FLAGS-on-stack wart adjudicated document-and-omit; the error-code mapping adjudicated to the honest single-code-per-condition table the blockdev seam can deliver. Status updated to Ratified. | ARB (full committee) |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter | P. Gibbons (Software Engineering II) | Submitted | 2026-06-15 |
| ARB Reviewer -- Technical Correctness | M. Bolton (Senior Engineer, Platform) | Concur-with-comment | 2026-06-15 |
| ARB Reviewer -- Period Authenticity | S. Nagheenanajar (Engineering, Heritage Conformance) | Concur-with-comment | 2026-06-15 |
| ARB Reviewer -- Governance & Compliance | T. Smykowski (QA / Change Advisory) | Concur-with-comment | 2026-06-15 |
| ARB Chair (Synthesis) | Slydell & Porter Consulting (delegated per beads initech-4mq7) | Ratified | 2026-06-15 |
| Operator Delegation | T. Osborne (Operator) | Delegated to ARB per beads initech-4mq7 (AMEND-NOW directive) | 2026-06-15 |
| Records Management | M. Waddams (Archive Annex B) | Filed | 2026-06-15 |

*Note on committee composition: The Technical/Correctness, Period-Authenticity, and Governance/Compliance reviewers correspond to the in-programme engineering functions designated in ADR-0003 1.3. Four independent assessments were submitted (DOS-3.3 compatibility historian; kernel-architecture / vector-allocation; scope / north-star / house-style; oracle / risk / reproducibility); the Chair synthesized the concurrences and adjudicated the two contested points (the stack-flags wart and the error-code mapping) on the record in 3.3 and 3.2 respectively. The operator delegated ratification authority to the committee per beads initech-4mq7 (AMEND-NOW directive); no separate operator sign-off is required for this amendment.*

---

## 1. Purpose and Scope

### 1.1 Purpose

The purpose of this Amendment (the "Amendment" or "DEC-15") is to admit two new software-interrupt vectors into the InitechDOS interrupt surface: **INT 25h (Absolute Disk Read)** and **INT 26h (Absolute Disk Write)**. These vectors expose absolute (volume-relative logical) sector read/write services beneath the file abstraction, as required by the external disk-utility layer (FORMAT, SYS, CHKDSK, DISKCOPY/DISKCOMP, FDISK, LABEL -- beads epic initech-8479).

DEC-04 (ADR-0003 5.4) established the INT 21h AH-function surface; DEC-10 (ADR-0003 5.10) sanctioned the separate DOS interrupt vectors INT 22h/23h/24h; DEC-04a (OEA-ADR-0003-A1) fixed the IDT gate policy, the PIC remap that frees the low vectors, and the flat 32-bit calling convention. None of those documents ratified INT 25h/26h. This Amendment is the approved change that does so, locks the associated spec-data in a new sibling file, and records the forward obligations arising therefrom.

### 1.2 Scope

This Amendment governs:

- The admission of INT 25h and INT 26h as separate software-interrupt vectors, their IDT gate type/DPL/selector, and their placement in the DEC-04a vector map (Sub-Decision DEC-15.1).
- The flat 32-bit register-level calling convention and the DOS hardware-error return contract for both vectors (Sub-Decision DEC-15.2).
- The disposition of the historical leftover-FLAGS-on-stack behaviour (Sub-Decision DEC-15.3).
- The home of the locked spec-data for the INT 25h/26h contract (Sub-Decision DEC-15.4).
- The exclusion of the DOS 4.0+ >32 MB absolute-I/O packet form (Sub-Decision DEC-15.5).

### 1.3 Out of Scope

The following are expressly out of scope of this Amendment:

- The DOS 4.0+ >32 MB absolute-I/O **packet form** (CX=0xFFFF, DS:BX -> parameter block). Deferred; see DEC-15.5 and Consequence C-6.
- **Multi-volume drive resolution.** This release mounts a single volume; AL referencing any other drive fails loud (DEC-15.2). A multi-volume drive table is a separate future work item.
- **FAT16 fixed-disk absolute I/O** beyond the single mounted volume. DEC-07 names FAT16 for fixed-disk media; the write/mount path is FAT12-only today.
- Real hardware-error granularity from a true FDC/ATA driver. The blockdev seam (`blockdev.h`) returns only success/negative; the AL error table in DEC-15.2 maps that to a single honest code per condition (Consequence C-5).

### 1.4 Additional Defined Terms

The following terms, when used herein, supplement the definitions in ADR-0003 1.4 and DEC-04a 1.4. The first two rows are **load-bearing** and exist to prevent the single highest-risk reading error of this Amendment.

| Term | Definition |
|---|---|
| **INT 25h vector** (the vector; `int 0x25`) | The CPU interrupt vector 0x25 = Absolute Disk Read. The subject of this Amendment. |
| **INT 26h vector** (the vector; `int 0x26`) | The CPU interrupt vector 0x26 = Absolute Disk Write. The subject of this Amendment. |
| **INT 21h function AH=25h** (SETVECT) | A *function* of the INT 21h dispatcher (Set Interrupt Vector), selected by AH=25h. Listed in ADR-0003 Appendix A and `spec/int21h_register.json`, wired via `g_setvect` (`os/milton/int21.c`). It is **entirely distinct** from the INT 25h vector and is **already implemented**. INT 21h function AH=35h (GETVECT) is its companion. The numerals 25h/35h in Appendix A name AH functions, not vectors. |
| Absolute (logical) sector | A volume/partition-relative sector number; boot sector = sector 0 = blockdev LBA 0. NOT physical CHS; NOT whole-disk LBA. |
| Block-device seam | The bound function-pointer interface (`int21_absdisk_backend_t`) through which the INT 21h spine reaches `blockdev_t.read_sectors`/`write_sectors` without including the FAT backend (Law 3). |

---

## 2. Context

### 2.1 What Is Missing and Why Now

The external disk-utility layer (epic initech-8479: FORMAT.COM, SYS.COM, CHKDSK.COM, DISKCOPY/DISKCOMP, FDISK, LABEL) is, per its own description, REQUIRED-BEFORE the GUI and the dBASE subset. Every one of those utilities is built on the period DOS primitive of absolute sector I/O via INT 25h/26h. The kernel already owns a read+write absolute-LBA seam -- `blockdev_t.read_sectors`/`write_sectors` (`os/milton/blockdev.h:45,50`), implemented by `ata.c` (`ata_read_sectors`/`ata_write_sectors`) on target and by the host file backend (`harness/diff/fat_diff/blockdev_file.c`) for oracles -- so the implementation surface is two thin, validating veneers over an existing primitive. What is missing is the *governance act* admitting the two vectors and the *contract* (drive numbering, error codes, the stack-flags disposition, the spec-data home).

### 2.2 Why This Is a Separate-Vector Decision, Not an Appendix A Change

ADR-0003 Appendix A ends with the closed-scope clause: *"This register is the controlled scope. Functions not listed are out of scope absent an approved change."* That clause governs the **INT 21h AH-function register only**. INT 25h and INT 26h are **not** AH-selected INT 21h functions -- they are independent interrupt vectors, exactly like INT 22h/23h/24h, which DEC-10 sanctioned as a 5 sub-decision and which DEC-04a placed in the vector map (Appendix E.4). Adding 25h/26h rows to Appendix A would be semantically wrong and would collide with the existing **AH=25h SETVECT** row (Appendix A; `spec/int21h_register.json:17`). The correct governance mechanism is therefore a new sub-decision plus an explicit Scope-Clause Delta (6), mirroring DEC-10 -- **not** an Appendix A row. See 1.4 for the mandatory disambiguation between the INT 25h *vector* and the INT 21h *function* AH=25h.

### 2.3 Vector Safety (the band is already reserved)

DEC-04a.2 remapped the master PIC to base 0x28 and the slave to 0x30, deliberately freeing the low vectors 0x20-0x27 for software interrupts (that remap is the resolution of the IRQ1/INT 21h collision, DEC-04a 4.3). DEC-04a Appendix E.4 lists "0x22-0x27 (unused) Spurious stubs." Vectors 0x25 and 0x26 sit inside that already-reserved band. Admitting them is a clean fill of two reserved slots with **no PIC/IRQ-map change**. See 7 for the explicit collision-freedom confirmation.

### 2.4 Relationship to the North Star

Per epic initech-bsy, neither Turbo Initech (tps) nor InitechBase (samir) calls INT 25h/26h -- both need handle-level INT 21h (heap 48h, EXEC, subdirs, LSEEK, RENAME, FINDFIRST, date/time), not sector-level services. DR-5 (continuity with the North Star) is therefore satisfied **negatively**: this Amendment does not preclude the North Star, but it does not advance it directly either. Its value maps under **DR-1 (Fidelity** -- a period DOS exposes INT 25h/26h), **DR-3 (Period Plausibility)**, and **DR-4 (Build Tractability** -- it makes the FORMAT/SYS/CHKDSK utility layer buildable).

---

## 3. The Decision (DEC-15)

It is hereby recorded that the following five Sub-Decisions collectively constitute Amendment DEC-15 to ADR-0003. Each is binding upon the implementation unless and until amended through the change-control process. For the avoidance of doubt, ADR-0003 Appendix A (the INT 21h AH-function register) is **unchanged**; these are separate vectors, not AH functions (6).

### 3.1 Sub-Decision DEC-15.1 -- Vectors, Gate Type, and Stub Shape

INT 25h (Absolute Disk Read) shall be installed at vector **0x25**, and INT 26h (Absolute Disk Write) at vector **0x26**, each as a **32-bit trap gate** with the following descriptor attributes -- identical to the INT 21h / 20h / 22h / 23h / 24h software-interrupt vectors (DEC-04a.1; Appendix E.2):

| Attribute | Value | Rationale |
|---|---|---|
| Gate type | `0x8F` (32-bit trap gate) | Leaves IF unchanged on entry, consistent with the cooperative non-preemptive model (DEC-04a 4.1); keeps these handlers reachable for future PIT-tick delivery during a long multi-sector transfer. An interrupt gate (0x8E) is **rejected** -- it would clear IF and diverge from the established software-INT house style. |
| DPL | 0 (ring-0) | Applications run at CPL=0 in the current release (ADR-0003 DEC-02). DPL=0 is correct now and inherits the DEC-04a C-4 DPL=0->3 forward obligation automatically by going through `idt_install_trap`. |
| Selector | `0x08` (CODE_SEL) | The flat ring-0 code segment (ADR-0001). |

The asm entry stubs `int25_entry` / `int26_entry` shall be **byte-identical in shape to `int24_entry`** (`os/milton/isr.asm:315-345`), because, like 24h, these vectors **RETURN a result + CF to the caller** (they do not terminate like 20h/22h/23h): push dummy `0` err_code; push the vector sentinel (`0x25` / `0x26`); push gs/fs/es/ds; pushad; reload DATA_SEL (0x10) into the segment registers; `mov eax, esp`; push eax; `call int25_dispatch` / `int26_dispatch`; `add esp, 4`; popad; pop ds/es/fs/gs; `add esp, 8`; **`iretd`**. The `iretd` tail is load-bearing (it returns a balanced frame and propagates CF via the saved EFLAGS -- see DEC-15.3). The pushed frame is the locked `int_frame_t` (DEC-04a Appendix E.3; `os/milton/idt.h`; 68 bytes). A one-line foot-gun comment at each new stub shall cite the deliberately-omitted leftover-FLAGS wart (DEC-15.3).

The gates are installed via `idt_install_trap(0x25u, int25_entry)` and `idt_install_trap(0x26u, int26_entry)` in `sysinit_early()`, immediately after the existing `idt_install_trap(0x24u, ...)` (`os/milton/sysinit.c:129`), before STI.

The C dispatchers `int25_dispatch` / `int26_dispatch` shall mirror `int24_dispatch` (`os/milton/int21.c:2786-2814`): the reentrancy bracket runs **first** -- `if (irq_depth() != 0u) dos_reentry_panic();` -- then `g_indos++` / body / `g_indos--`. This enforces the invariant that no ISR or driver issues `int 0x25`/`int 0x26` (which would corrupt an interrupted syscall's FAT/sector scratch), failing loud per CLAUDE.md Rule 2. INT 25h/26h share the FAT/sector scratch with INT 21h and therefore **inherit the DEC-04a C-5 reentrancy forward-obligation** (Consequence C-7).

### 3.2 Sub-Decision DEC-15.2 -- Register Convention, Drive Numbering, and Error Contract

**Input registers (DOS 3.3 PRM small-partition form, flat-adapted).** This is the *only* form in scope (DEC-15.5). Note the **deliberate register-role swap** relative to the DEC-04a INT 21h default: INT 25h/26h historically use BX for the transfer buffer, so in the flat ABI **EBX is the buffer pointer** (not the DEC-04a handle register), and the starting sector is in DX/EDX (not the DEC-04a EDX-pointer role). Silence would inherit the wrong default; this swap is binding and is recorded in the locked spec-data.

| Role | Register | Notes |
|---|---|---|
| Drive | AL | **Zero-based explicit: 0=A:, 1=B:, 2=C:.** There is no "default-drive" sentinel. This DIFFERS from INT 21h AH=0Eh/19h/36h where 0=default and 1=A:; it is a classic transcription bug and is called out here deliberately. This single-volume release: AL=0 -> the one mounted volume; any other AL -> CF=1 invalid-drive (never silent success, never a fault). |
| Sector count | CX (ECX) | Number of consecutive sectors. CX=0 -> no-op success (CF=0), preserving the DOS contract (cf. DEC-14 zero-count). CX=0xFFFF is the DOS 4.0+ packet sentinel -> **rejected** (DEC-15.5), not misread as a count. |
| Starting absolute logical sector | DX (EDX) | Volume/partition-relative; maps 1:1 onto `blockdev_t` `lba` (boot sector = LBA 0). |
| Transfer buffer | **EBX** (flat 32-bit linear) | Real DOS DS:BX -> flat EBX. Validated through the existing `user_buf_ok(EBX, CX*512)` guard (DEC-14) BEFORE any sector read/write touches it; `CX*512` is computed so it cannot 32-bit-wrap the buffer span (DEC-14 wrap discipline). |

**Return.** Success: **CF=0** (written to bit 0 of the saved EFLAGS in `int_frame_t`; IRETD restores it). Failure: **CF=1**, AX = error, where **AL = DOS hardware error low byte** and **AH = error class byte**.

**Register preservation.** Real DOS 3.3 INT 25h/26h preserve only the segment registers (they destroy the GP registers). InitechOS consciously chooses the **benign superset**: the dispatchers preserve EBX/ECX/EDX/ESI/EDI/EBP per the DEC-04a default (Appendix E.1), writing only EAX (AX error/return) and CF. This is stated explicitly so the implementer does not silently inherit either extreme; a caller that relies only on documented outputs sees identical behaviour.

**Error-code table (the contested point -- adjudicated).** The committee split between a fine-grained eight-code historian table (0x00 write-protect ... 0x80 timeout) and the honest "single code per condition the seam can actually deliver" table. The Chair adjudicates to the **honest table**, on Law 2 grounds: the `blockdev_t` seam returns only success/negative and has no real hardware-error granularity, so an over-specified table would imply a fidelity the backend cannot deliver and would make the error oracle assert a contract that is not mechanically reachable. The AL codes are nevertheless drawn from the **DOS driver / INT 24h low-byte space** (period-correct) and are **a SEPARATE code space from the INT 21h `INT21_ERR_*` extended-error enum** (`os/milton/int21.h:34-44`); the two MUST NOT be conflated. The locked AL/AH pairs are:

| Condition | AL (hardware code) | AH (class) | Source |
|---|---|---|---|
| Write-protect (INT 26h on a read-only backend: `write_sectors == NULL`) | `0x00` | `0x0A` | DOS write-protect low byte; ties to MSG-DOS-0008 (`Write protect error writing drive %c:`). |
| Invalid drive (AL not the mounted volume) | `0x0F` | `0x0C` | DOS "invalid drive" low byte. |
| Sector not found / out-of-range or count overflow (DX >= total_sectors, or DX+CX > total_sectors, or DX+CX wraps) | `0x08` | `0x0B` | DOS "sector not found"; the kernel bounds-checks BEFORE calling `read_sectors`/`write_sectors` (a raw LBA past EOF must fail loud, never a short/garbage read -- `blockdev.h` contract, Rule 2). |
| General read/write failure (backend `read_sectors`/`write_sectors` returns negative) | `0x0C` | `0x0B` | DOS "general failure"; the single honest mapping of a negative seam return for the current floppy personality. |
| Bad buffer (NULL / 32-bit wrap, per DEC-14) | `0x0F` (AX surfaced as the DEC-14 path) | `0x0C` | The DEC-14 `user_buf_ok` guard fires first; CF=1, the call performs no I/O. |

The AH class bytes 0x0A / 0x0B / 0x0C above are **PROVISIONAL** pending an 86Box golden of the real DOS 3.3 INT 25h/26h AH error class returned for each condition; **CF and AL are the load-bearing, ratified values** and are not provisional. Should the 86Box golden return a different AH class for any row, that AH is corrected by editorial erratum to the locked spec-data without a further full amendment; the CF/AL contract and the condition-to-code mapping are fixed. The full table (vectors, the EBX-buffer register swap, the AL/AH pairs, the CX=0xFFFF rejection, the bounds-check rule, the scratch-LBA-must-be-free oracle rule) is the locked spec-data of DEC-15.4. The values above are the contract; the table file is authoritative in case of conflict.

### 3.3 Sub-Decision DEC-15.3 -- The Leftover-FLAGS-on-Stack Wart: Document-and-OMIT

In real DOS 3.x, INT 25h/26h return with the original CPU FLAGS still **pushed** on the caller's stack (the historical CP/M-derived wart), so every period caller follows `int 25h`/`int 26h` with an `add sp,2` (or `popf`) to rebalance the stack. This is the single most famous INT 25h/26h ABI tell. **The committee was split (reproduce-for-fidelity vs normalize); the Chair adjudicates: document-and-OMIT -- the wart is intentionally NOT reproduced.** Rationale, on the record:

1. **No caller depends on it.** Our callers are 100% in-tree flat 32-bit native programs (ADR-0003 DEC-02 personality-not-binary-compat, DEC-08 flat executables); the DOS Compatibility Subsystem is out of scope (ADR-0003 1.2/4.3). There is no genuine 16-bit `.COM`/`.EXE` that hand-codes the `add sp,2`. The wart has zero fidelity value to a flat native caller.
2. **The machine model forbids it cleanly.** These vectors are 0x8F trap gates dispatched through the uniform `int_frame_t` and returned via **IRETD**, which restores a balanced EFLAGS frame. There is no clean place to leave a stray word on a same-privilege ring-0->ring-0 IRETD stack without desynchronizing the gate's own return. Reproducing the wart would require a bespoke non-IRETD return path for two vectors alone, breaking the locked stub uniformity (`isr.asm`) and the DEC-04a Appendix E.3 frame contract -- a Rule-2 fail-loud hazard for zero benefit.
3. **It is the same divergence philosophy DEC-14 already adopted** (validate where DOS trusts, for a bulletproof base; Law 4 fidelity is about *observable* behaviour). Observable behaviour -- CF set/clear and the AX codes -- is faithful; the invisible stack wart is not.

Therefore: error status is conveyed via **CF in the saved EFLAGS** per DEC-04a 3.3, the stub returns via the standard uniform IRETD with **no stack imbalance**, and no `add sp,2`/`popf` contract is imposed on callers. The oracle asserts stack balance across the call (Mutation M7). A one-line foot-gun comment at `int25_entry`/`int26_entry` records the omission.

### 3.4 Sub-Decision DEC-15.4 -- Locked Spec-Data Home: a NEW Sibling File

The INT 25h/26h contract shall be recorded in a **NEW locked spec file `spec/absdisk_int2526.json`** (CLAUDE.md Rule 8), **NOT** as a stanza in `spec/int21h_calling_convention.json`. The calling-convention file is INT-21h-scoped by its own `_comment` and its `make test-spec` gate (DEC-04a 7.1) asserts that **every documented `ah` exists in `spec/int21h_register.json`**; INT 25h/26h are vectors, not AH functions, so a stanza there would break or corrupt that invariant. The new file shall carry its own consistency gate (it asserts the two vectors, the per-register input layout including the EBX-buffer swap, the success/error AL/AH pairs of DEC-15.2, the CX=0xFFFF out-of-scope rejection, and the scratch-LBA-must-be-free oracle rule), and shall be **explicitly excluded** from the test-spec AH-consistency walk. The `INT21_ERR_*` enum shall **not** be reused for the AL hardware byte.

### 3.5 Sub-Decision DEC-15.5 -- DOS 4.0+ >32 MB Packet Form: OUT OF SCOPE

The DOS 4.0+ absolute-I/O **packet form** (CX=0xFFFF; DS:BX -> a parameter block carrying a 32-bit start sector and count) exists only to address logical sectors >= 65536, i.e. FAT16 partitions exceeding 32 MB. This release is a 1.44 MB FAT12 floppy personality (2880 sectors); the 16-bit DX start and 16-bit CX count are more than sufficient. The packet form is a DOS 4.0 (1988) feature and is a period-creep risk for a "3.30" personality (DEC-12). It is declared **OUT OF SCOPE** and deferred (Consequence C-6). A caller passing **CX=0xFFFF** shall be **rejected with CF=1** and a distinct serial diagnostic (the DEC-04a not-yet-impl pattern) -- **never** silently misinterpreted as an ordinary 65535-sector count and read off the end of the volume.

---

## 4. Rationale

### 4.1 Trap Gate, RETURN Stub Template (DEC-15.1)

The software-INT service house style is the 0x8F trap gate (DEC-04a.1): IF stays set, consistent with the cooperative model and avoiding STI/CLI inside the handler. The `int24_entry` stub -- not the terminating `int20/22/23_entry` stubs -- is the correct template because, like the critical-error handler, INT 25h/26h compute a result and RETURN it to the caller with CF; the `iretd` tail is the mechanism, not a fallback.

### 4.2 The Register-Role Swap (DEC-15.2)

DEC-04a fixed EDX=pointer / EBX=handle for INT 21h. INT 25h/26h are a different vector family with a DOS-defined layout (AL=drive, CX=count, DX=start, **BX=buffer**). The frame-extraction code must read `f->ebx` as the buffer and `f->edx`/`f->dx` as the start sector. A silent inheritance of the DEC-04a default would read EBX as a handle and EDX as the pointer -- precisely the field-transposition class of bug DEC-04a Appendix E.3 warns of -- producing a quietly-wrong dispatcher. Stating the swap in both this Amendment and the locked spec-data is the guard.

### 4.3 Drive Numbering Inversion (DEC-15.2)

INT 25h/26h are zero-based-explicit (0=A:), whereas INT 21h AH=0Eh/19h/36h treat 0 as the default drive. Failing to call this out invites a transcription bug. For this single-volume milestone the only valid AL is 0; everything else fails loud.

### 4.4 Honest Error Codes over Aspirational Granularity (DEC-15.2)

Law 2 makes the oracle, not the prose, the truth. The `blockdev` seam cannot distinguish CRC from seek from controller failure -- it returns negative. An eight-code table would assert fidelity the backend cannot deliver and would seed a quietly-wrong error oracle (the worst outcome under Law 2). The honest table maps each *reachable* condition to one period-correct AL/AH pair; finer codes are deferred to a real FDC/ATA driver (Consequence C-5). The write-protect case (AL=0x00) is genuinely reachable today (read-only `blockdev_file` open / `write_sectors == NULL`) and ties to MSG-DOS-0008. The AH class bytes remain provisional pending an 86Box golden (3.2); CF and AL are load-bearing and ratified.

### 4.5 The Wart Disposition (DEC-15.3)

Covered on the record in 3.3. The decisive factors are: no dependent caller exists; the IRETD trap-gate model cannot reproduce it without a bespoke unbalanced-stack return path (a fail-loud hazard); and DEC-14 already set the document-and-omit precedent for an invisible DOS trust behaviour.

### 4.6 Spec-Data Home (DEC-15.4)

Wedging a non-AH vector into `spec/int21h_calling_convention.json` would break its DEC-04a 7.1 AH-consistency gate or force that gate to be weakened -- a Stop-Condition violation (locked specs are strengthened, not relaxed). A new sibling file with its own gate is the only Law-2-clean home.

### 4.7 Period Authenticity (DEC-15.5)

The literal `int 0x25`/`int 0x26` vectors, the AL=drive/CX=count/DX=start/DS:BX=buffer layout, the CF-error idiom, and the AL hardware-error low byte all match the DOS 3.3 Programmer's Reference Manual. The packet form is a DOS 4.0 feature; excluding it keeps the "3.30" personality period-true (DEC-12, NFR-1).

---

## 5. Consequences

### 5.1 Binding Constraints

**C-1 -- Two new trap gates at 0x25/0x26.** Gate type 0x8F, DPL=0, selector 0x08, installed via `idt_install_trap` after the 0x24 install and before STI. These vectors leave the DEC-04a "0x22-0x27 spurious stub" set; 0x27 remains spurious. No PIC/IRQ-map change.

**C-2 -- The DEC-04a vector map is updated.** DEC-04a Appendix E.2 (the "0x22-0x27 unused/spurious" row) and Appendix E.4 (the 0x25/0x26 rows) are amended to reclassify 0x25/0x26 as INT 25h/26h trap-gate service vectors -- the same explicit carve-out DEC-10 made for 0x22-0x24. Without this the locked vector map would contradict the install. (The map text update is recorded here; the DEC-04a document's Appendix E is read as amended by this reclassification. 0x27 remains spurious.)

**C-3 -- The register-role swap and AL error table are locked spec-data.** Recorded in the new file `spec/absdisk_int2526.json` (DEC-15.4). Any change to the EBX-buffer convention, the AL/AH pairs, or the bounds-check rule requires a further amendment, a green new-file consistency gate, and a green mutation oracle.

**C-4 -- A new block-device seam, not a FAT include.** INT 25h/26h reach disk ONLY through a new bound seam (`int21_set_blockdev` / `int21_absdisk_backend_t` exposing `read(lba,count,buf)`/`write(lba,count,buf)`), bound in `kmain.c` from the already-mounted `vol->dev` (`fatdev.read_sectors`/`write_sectors`) at the `fileio_fat_bind` site, only on the `mounted == 1` path. `int21.c` shall NOT include `fat12.h`/`blockdev.h` or reference `g_vol` (Law 3; preserves hosted testability). With no seam bound, the handlers fail loud (CF=1), never fault. `write_sectors == NULL` -> INT 26h CF=1, AL=0x00 write-protect.

### 5.2 Forward Obligations

**C-5 -- Finer hardware-error codes deferred to a real driver.** When a true FDC/ATA driver with hardware-error granularity lands, the honest single-code-per-condition AL table (DEC-15.2) may be widened (CRC 0x10, seek 0x40, controller 0x20, timeout 0x80, ...) by a further amendment and a strengthened error oracle. Until then, the seam returns negative and the kernel maps it to one honest code per condition.

**C-6 -- DOS 4.0+ >32 MB packet form deferred.** Required only for FAT16 partitions exceeding 32 MB (not in this release). A future FAT16-fixed-disk amendment may add it; until then CX=0xFFFF is rejected loudly (DEC-15.5). Multi-volume drive resolution is deferred on the same footing.

**C-7 -- Reentrancy obligation inherited.** INT 25h/26h share the FAT/sector scratch with INT 21h and therefore inherit the DEC-04a C-5 reentrancy forward-obligation. The `irq_depth()` panic guard suffices for the current cooperative single-task release; when IRQs that issue DOS calls appear, the shared-scratch hazard must be re-audited at that milestone.

**C-8 -- DPL=0->3 upgrade inherited.** By going through `idt_install_trap`, the new gates inherit the DEC-04a C-4 obligation: their DPL must be upgraded to 3 before any ring-3 program issues `int 0x25`/`int 0x26`.

### 5.3 Neutral Consequences

- ADR-0003 Appendix A (the INT 21h AH-function register) is **unchanged**; INT 25h/26h are vectors, not AH functions (6). The existing AH=25h SETVECT / AH=35h GETVECT row is untouched.
- The DOS Compatibility Subsystem, the MZ loader (DEC-08), and all other ADR-0003 Sub-Decisions are unaffected.
- `spec/dos_messages.json` and ADR-0003 Appendix C are unchanged -- MSG-DOS-0008 ("Write protect error writing drive %c:") already exists and is reused; no new controlled message is added.
- No change is anticipated to the procurement of foam packaging inserts.

---

## 6. Scope-Clause Delta (the Governance Act)

This section is the load-bearing governance act of this Amendment.

ADR-0003 Appendix A's closed-scope clause -- *"This register is the controlled scope. Functions not listed are out of scope absent an approved change."* -- governs the **INT 21h AH-function register** and is **unchanged** by this Amendment. The present Amendment is the "approved change" contemplated by that clause's escape hatch, but it admits two NEW **software-interrupt vectors** -- **INT 25h (Absolute Disk Read)** and **INT 26h (Absolute Disk Write)** -- into the InitechDOS interrupt surface, occupying vectors 0x25 and 0x26 within the software-interrupt band reserved by DEC-04a Appendix E.4. **These are vectors, not INT 21h functions; they do not appear in `spec/int21h_register.json` and are out of scope of the initech-40oq Appendix-A coverage oracle.** This mirrors DEC-10, which admitted INT 22h/23h/24h as 5 sub-decisions rather than Appendix-A rows, and DEC-04a 6, which records that "Appendix A is unchanged." No row is added to Appendix A or to `spec/int21h_register.json`; the AH=25h/35h SETVECT/GETVECT row in Appendix A is a distinct, already-implemented INT 21h *function* and is not affected.

---

## 7. Vector-Safety Confirmation

Confirmed against DEC-04a Appendix E.2/E.4 and `os/milton/idt.c`:

- **Above CPU exceptions.** 0x25, 0x26 > 0x1F. CPU exception stubs (isr0..isr31) occupy 0x00-0x1F. No overlap.
- **Outside the remapped PIC range.** DEC-04a.2 (binding) places master IRQs at 0x28-0x2F and slave IRQs at 0x30-0x37. 0x25/0x26 sit in the gap 0x22-0x27 that Appendix E.4 labels "(unused) Spurious stubs." No IRQ collision -- unlike the IRQ1/0x21 collision that forced the remap.
- **No trap-vector collision.** 0x21 (INT 21h), 0x20 (legacy terminate), and 0x22/0x23/0x24 (DEC-10) are installed via `idt_install_trap`. 0x25/0x26 are adjacent-but-distinct and currently point at the fail-loud `isr_spurious` sentinel (zero existing 0x25/0x26 consumers anywhere in `os/`). This Amendment fills two genuinely-free slots, exactly as DEC-10 filled 0x22-0x24.

---

## 8. Verification

### 8.1 The Differential Oracle (test-absdisk)

A new `make test-absdisk` host oracle binds a mock block-device seam (the host file backend, `blockdev_file_open_rw`) and exercises both vectors against a host-memory disk, with NO QEMU. This oracle lands with the implementation (beads initech-4mq7); it is not part of this docs+spec amendment:

1. Open a WRITABLE 1.44 MB image rw; `fat12_mount`; bind the absdisk seam.
2. Compute the **SAFE scratch LBA** from mounted geometry (see 8.2) and **assert its FAT entry is free** (`fat12_next_cluster == 0x000`) before touching it; fail loud otherwise.
3. Fill a 512-byte deterministic pattern (byte i = `(i*0x6D + (LBA & 0xFF)) & 0xFF` -- a pure function of index and LBA; no wall-clock, no rand; Rule 11).
4. **INT 26h WRITE** (AL=0, CX=1, DX=scratch_LBA, EBX=pattern). Assert CF=0.
5. **INT 25h READ** the same LBA into a zeroed buffer. Assert CF=0 and buffer == pattern byte-for-byte (the round-trip).
6. **Cross-check** the same LBA via an independent `blockdev_file_read` (or `fat12_ref.py` raw-sector read) of the underlying file and assert it equals the pattern -- proves INT 26h hit `lba*512` in the backing store, not a cache.
7. **Non-corruption proof:** snapshot the boot sector (LBA 0), both FATs, and the root directory BEFORE and AFTER the round-trip; assert byte-identical. This is the explicit guard that no other FAT oracle's golden region is perturbed (Stop-Condition: never weaken an existing oracle).
8. **Error-path cases** (each with its locked AL/AH pair, NOT INT21_ERR_*): invalid drive (AL!=0); out-of-range sector (DX >= total_sectors); count overflow/wrap (DX+CX); write-protect (`write_sectors == NULL` -> AL=0x00); zero-count success (CX=0); CX=0xFFFF packet rejection (CF=1 + serial diagnostic, never a 65535-sector read).

### 8.2 The SAFE Scratch LBA

On the standard 1.44 MB FAT12 floppy: reserved=1, 2 FATs x 9 = 18, root_dir = 224x32/512 = 14 sectors, so first_data_sector = 33 and the data area is LBA 33..2879 (total_sectors 2880). The scratch sector is the **last data sector, LBA = total_logical_sectors - 1 (= 2879)** -- computed from mounted geometry (`BPB_FIRST_DATA_SECTOR`, `spec/dos_structs.h:232`), **never a magic number**, so it self-adjusts if the fixture geometry changes. The oracle asserts the corresponding FAT entry is free before writing, so the round-trip stays entirely inside unallocated data space and cannot perturb the boot/FAT/root region or any allocated file. The test image is a **gitignored build intermediate minted fresh per run** (or the scratch sector is restored at teardown), since the WRITE oracle mutates it -- Rule 11 reproducibility / clean tree.

### 8.3 Mutation Plan (each one-branch perturbation must turn the oracle RED -- Rule 6)

| Mutant | Perturbation | RED signal |
|---|---|---|
| M1 | off-by-one LBA (or dispatcher adds the FAT data offset that absolute calls must NOT add) | round-trip [5] AND file cross-check [6] |
| M2 | stub-drop the WRITE (26h returns CF=0 but no-ops) | read-back [5] / file cross-check [6] |
| M3 | swap/transpose count (honor CX=1 but write 0, or DX/CX register transposition) | round-trip [5] |
| M4 | drop the bounds check (DX >= total returns CF=0) | out-of-range error case [8] |
| M5 | wrong error class (return INT21_ERR_* instead of the AL/AH pair) | error-AX assertion [8] (proves the spec-data contract is asserted) |
| M6 | corrupt a neighbor (write CX=2 when CX=1 asked) | non-corruption snapshot [7] |
| M7 | FLAGS-stack-balance (leave a stray word on the stack -- the omitted wart) | stack-balance assertion |
| M8 | treat CX=0xFFFF as a literal count | reads off the end -> bounds-check RED |

### 8.4 Locked Spec Authority

`spec/absdisk_int2526.json` is the locked authoritative contract for the INT 25h/26h vectors (Rule 8). Its consistency gate runs under `make test-spec`, separate from and additional to the INT-21h AH-consistency walk, and lands with the implementation (beads initech-4mq7); the file itself is inert to the existing test-spec steps (which read only the five INT-21h/struct/banner spec files by name, not a glob). No implementation may deviate from the vectors, the EBX-buffer register swap, the AL/AH pairs, the bounds-check rule, or the CX=0xFFFF rejection recorded therein without a further amendment.

---

## 9. Implementation Disposition (ratify now; implement non-blocking)

The contract is **ratified now**; this Amendment records the decision and locks the spec-data only. The kernel handlers (`int21.c`/`isr.asm`/`sysinit.c`/`kmain.c`) and the biting `test-absdisk` oracle are a SEPARATE later landing under beads initech-4mq7. Implementation (beads initech-4mq7) may land immediately on the existing blockdev/ATA read+write seam, but it is **not blocking**: 4mq7's consumers (epic initech-8479 utilities) are themselves gated behind the kernel-complete capstone initech-40oq, which DEPENDS-ON 4mq7. The value of implementing is not realized until 40oq and 8479 land regardless. 40oq's ratification shall NOT wait on the 4mq7 implementation, and 40oq's Appendix-A AH-coverage oracle does not cover 25h/26h (they are not AH functions); 40oq's DEPENDS-ON of 4mq7 is about the util layer, not the AH coverage count (see the capstone scope note). Kernel-feature-complete for the DOS-3.3 controlled scope = full Core+Resident AH coverage (the Appendix-A walk) AND the wired sector-service vectors (INT 25h/26h) present and green under `make test-absdisk`.

---

## 10. Related Decisions and References

- ADR-0003 (OEA-ADR-0003) -- InitechDOS Base OS Personality. 5.4 (DEC-04 INT 21h surface), 5.7 (DEC-07 FAT12/16), 5.10 (DEC-10 separate vectors 22h/23h/24h), 5.12 (DEC-12 3.30 personality), 5.13 (DEC-13 controlled vocabulary), Appendix A closed-scope clause, Appendix C (MSG-DOS-0008).
- ADR-0003 Amendment DEC-04a (OEA-ADR-0003-A1) -- gate policy (3.1), PIC remap (3.2), flat ABI + CF-via-saved-EFLAGS (3.3/4.4), Appendix E.2/E.3/E.4 (gate-type policy, int_frame_t, vector map), C-4/C-5 forward obligations.
- ADR-0003 Amendment DEC-14 (OEA-ADR-0003-A2) -- NULL/32-bit-wrap buffer guard; document-and-omit divergence precedent.
- `os/milton/blockdev.h:45,50` -- `read_sectors`/`write_sectors` absolute-LBA seam; `write_sectors == NULL` = read-only.
- `os/milton/isr.asm:315-345` -- `int24_entry` RETURN stub template.
- `os/milton/sysinit.c:120-129` -- `idt_install_trap` install site for the separate vectors.
- `os/milton/int21.c:2786-2814` -- `int24_dispatch` reentrancy bracket; `:723` `user_buf_ok`.
- `os/milton/kmain.c:697-731` -- function-scope `vol`/`fatdev`, `fat12_mount`, `fileio_fat_bind` site.
- `harness/diff/fat_diff/blockdev_file.c:93-104` -- host rw backend; `fat12_ref.py` raw-sector cross-check.
- `spec/absdisk_int2526.json` -- locked INT 25h/26h vector contract ratified herein (DEC-15.4).
- DOS 3.3 Programmer's Reference Manual (INT 25h/26h Absolute Disk Read/Write).
- beads initech-4mq7, initech-40oq, initech-bsy, initech-8479.

---

*-- End of Record --*

<!--
This document is the confidential and proprietary information of Initech Systems
Corporation. Unauthorized review, use, disclosure, or distribution is prohibited.
If you have received this document in error, please shred it and notify the Help
Desk (ext. 2504). This footer is part of the controlled document and shall not be
removed. Tedium certified compliant with NFR-7.
-->
