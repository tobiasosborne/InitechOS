<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0003 — InitechDOS Base Operating System Personality and Resident Kernel Architecture

**Issuing Body:** Initech Systems Corporation — Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record (ADR)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0003 |
| Title | InitechDOS Base Operating System Personality and Resident Kernel Architecture |
| Version | 1.0 |
| Status | **Accepted — Ratified** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | P. Gibbons (Software Engineering II) |
| Effective Date | 2026-06-08 |
| Next Scheduled Review | 2026-12-08 (semi-annual, per RECORDS-POL-002) |
| Supersedes | DRAFT-DOS-PERSONALITY-v0.7 (circulated for comment, not ratified) |
| Superseded By | (none at time of issue) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | 2026-05-29 | P. Gibbons | Initial draft for internal circulation. | — |
| 0.4 | 2026-06-02 | P. Gibbons | Incorporated feedback from the Architecture Review Board (ARB). Reworded §2 for alignment. No technical change. | M. Bolton |
| 0.6 | 2026-06-04 | P. Gibbons | Added §5.6; clarified that §5.4 introduces no new behaviour. Corrected header to use the approved cover-sheet format (ref. Memo 1.A.4). | S. Nagheenanajar |
| 0.7 | 2026-06-05 | P. Gibbons | Addressed comments regarding terminology consistency. Replaced three (3) instances of "we" with "the Programme." | M. Bolton, S. Nagheenanajar |
| 1.0 | 2026-06-08 | P. Gibbons | Ratified. No change from 0.7 other than status. | ARB |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author | P. Gibbons | Submitted | 2026-06-06 |
| Peer Reviewer (Engineering) | M. Bolton | Concur | 2026-06-06 |
| Peer Reviewer (Data) | S. Nagheenanajar | Concur | 2026-06-06 |
| QA Representative | (vacant) | Concur (verbal, to be documented) | 2026-06-07 |
| Information Security Liaison | (delegated) | No objection (in scope of ISEC-EXEMPT-09) | 2026-06-07 |
| Stakeholder Engagement | T. Smykowski | Acknowledged | 2026-06-07 |
| External Independent Review | Slydell & Porter Consulting | "Looks fine." | 2026-06-07 |
| Director, Platform Engineering | D. Portwood | Approve | 2026-06-08 |
| Vice President, Engineering | W. Lumbergh | Approve (yeah) | 2026-06-08 |
| Records Management | M. Waddams (Archive Annex B) | Filed | 2026-06-08 |

---

## 1. Purpose

The purpose of this Architecture Decision Record (hereinafter "this ADR" or "the Record") is to document, for audit and continuity purposes, the architectural decision (the "Decision") reached by the relevant stakeholders (the "Stakeholders") concerning the base operating system layer (the "Base OS," and as branded, "InitechDOS") of the InitechOS platform (the "Platform"). This Record is provided for informational and governance purposes and is intended to establish a single, authoritative point of reference, going forward, for the matters set out herein.

### 1.1 Scope

This Record addresses the resident kernel, the system-call surface, the on-disk file system, the resident and installable device-driver model, the program-loading mechanism, the command interpreter, and the associated nomenclature and diagnostic-message conventions of InitechDOS. The Record further establishes the classification of each architectural element as either **load-bearing** or **vestigial** (as defined in §1.4) for the purposes of implementation prioritization.

### 1.2 Out of Scope

The following are expressly out of scope of this Record and are, where applicable, addressed under separate cover:

- Execution of genuine third-party 16-bit real-mode executables (the "DOS Compatibility Subsystem"), which is deferred pending a future ADR and is **not** required for the current release (see §2 and ADR-0001 §6).
- The graphical environment (the "Toolbox"), addressed under ADR-0004 (pending).
- The database application ("InitechBase"), addressed under ADR-0006 (pending).
- The self-hosting compiler ("Turbo Initech"), addressed under ADR-0007 (pending).
- Procurement of fulfilment packaging, retail cartons, foam inserts, and printed manuals, which is the responsibility of the Documentation & Fulfilment workstream and is tracked separately.

### 1.3 Intended Audience

This Record is intended for Platform Engineering personnel, the Quality Assurance function, the Change Advisory Board, and such other parties as may, from time to time, require access in the ordinary course of their duties. No part of this Record should be construed as conferring any entitlement, expectation, or warranty.

### 1.4 Definitions and Acronyms

For the avoidance of doubt, the following terms, when capitalized, shall have the meanings ascribed to them below.

| Term | Definition |
|---|---|
| ADR | Architecture Decision Record. A document of this kind. |
| API | Application Programming Interface. The surface by which programs request services. |
| BPB | BIOS Parameter Block. The structure, resident in the boot sector, describing volume geometry. |
| DTA | Disk Transfer Area. The buffer used by certain legacy file operations. |
| FAT | File Allocation Table. The cluster-chain file system. |
| FCB | File Control Block. The legacy (CP/M-derived) file-access structure. |
| Handle | An integer index identifying an open file or device to a process. |
| JFT | Job File Table. The per-process table mapping handles to System File Table entries. |
| Load-Bearing | An element upon whose correct behaviour the Platform functionally depends. |
| MCB | Memory Control Block (arena header). The structure forming the memory-allocation chain. |
| PSP | Program Segment Prefix. The 256-byte control block preceding a loaded program. |
| Reference Frame | The canonical source artifact (the *Office Space* screen capture) constituting the fidelity specification, as established in ADR-0001 §4. |
| SFT | System File Table. The system-wide table holding open-file state. |
| Stakeholder | A person or function with a legitimate interest in the matters herein. |
| TSR | Terminate and Stay Resident. A program that remains in memory after returning control. |
| Vestigial | An element retained for fidelity, compatibility, or aesthetic conformance notwithstanding the absence of functional necessity. |

---

## 2. Context and Problem Statement

It has been determined, in alignment with the objectives of the STAPLER Programme and pursuant to the platform direction ratified under ADR-0001 (Target Hardware Platform; 386-or-greater; 32-bit protected/flat memory model) and ADR-0002 (Toolchain and Executable Format; `i686-elf` cross-compilation; C as the implementation language), that the Platform requires a base operating system layer.

A requirement exists for the said base layer to present, to applications and to the operator, an experience that is substantively indistinguishable from a commodity disk operating system of the 1987–1991 period, consistent with the Reference Frame. It is further noted that the Reference Frame exhibits characteristics of more than one heritage platform concurrently (specifically, Macintosh-derived window furniture, DOS-derived drive nomenclature, and a wait indicator of non-Macintosh provenance), and that fidelity to the Reference Frame therefore necessitates a deliberate and documented hybridization.

The problem to be resolved by this Decision is the manner in which a base operating system, delivering the foregoing experience, is to be realized upon the 32-bit substrate established under ADR-0001, having regard to the constraints of period plausibility, automated build and verification (the "Conformance" requirement, §7), and the experiential requirement set out at NFR-7 (§8).

---

## 3. Decision Drivers

The Decision was informed by the following drivers, which are enumerated for completeness and are not listed in any order of priority unless otherwise stated:

1. **DR-1 — Fidelity.** The Base OS shall align with the Reference Frame to the standard set out in ADR-0001 §3.
2. **DR-2 — Verifiability.** The Base OS shall be amenable to automated conformance verification via the Emulation Conformance Harness (the "ECH," §7).
3. **DR-3 — Period Plausibility.** The Base OS shall be consistent with software of the stated period and shall not exhibit anachronistic capability.
4. **DR-4 — Build Tractability.** The architecture shall be realizable by the automated implementation function (the "Programme Build Capability") within the bounds of available oracles.
5. **DR-5 — Continuity with North Star.** The architecture shall not preclude the eventual self-hosting compilation objective (ADR-0007, pending).
6. **DR-6 — Experiential Conformance (NFR-7).** The Base OS shall, at all times and without exception, present an experience of comprehensive, uniform, and dependable tedium consistent with enterprise software of the stated period.

---

## 4. Considered Options

In accordance with good architectural practice, the following options were considered. The summaries below are provided for the record and are necessarily abbreviated.

### 4.1 Option A — Authentic 8086/80286 Real-Mode Operating System

A faithful real-mode operating system targeting the 8086/80286, employing segmented addressing and a 640-kilobyte conventional memory ceiling.

- *Advantages:* Maximal heritage authenticity.
- *Disadvantages:* Materially impaired debuggability under the Programme Build Capability; substantially increased effort in the graphical layer; segmented-memory arithmetic; no fond institutional memory of the platform exists (per stakeholder testimony of record).
- *Disposition:* **Rejected** (superseded by ADR-0001).

### 4.2 Option B — Adoption or Port of an Existing Disk Operating System

Adoption of a pre-existing, freely available disk operating system implementation.

- *Advantages:* Reduced initial effort.
- *Disadvantages:* Misalignment with the 32-bit substrate and the hybridized Toolbox direction; reduced control over fidelity to the Reference Frame; forfeiture of the heritage-compatibility build objective; potential licensing entanglement requiring review by Legal (not undertaken).
- *Disposition:* **Rejected.** Retained as a reference implementation only (see §7).

### 4.3 Option C — Literal 16-Bit Binary Compatibility (Virtual 8086 Subsystem)

Provision of a virtual-8086 environment capable of executing genuine period executables unmodified.

- *Advantages:* Maximal application compatibility.
- *Disadvantages:* Significant additional effort not required for the current release; the verification of InitechBase (the database application) is satisfied by executing the Reference Implementation on the host emulator (§7) and therefore does not depend upon this capability.
- *Disposition:* **Deferred** to a future ADR. Out of scope (see §1.2).

### 4.4 Option D — DOS 3.3 Personality Layer upon a Native 32-Bit Kernel *(Selected)*

Provision of a native 32-bit kernel presenting the DOS 3.3 programming model — system-call surface, file model, control structures, command interpreter, and diagnostic conventions — to native 32-bit Platform applications.

- *Advantages:* Satisfies all Decision Drivers; preserves debuggability and build tractability; achieves fidelity and period plausibility; does not preclude the North Star.
- *Disadvantages:* Certain heritage structures are rendered functionally unnecessary and are retained on a vestigial basis (§5.3), incurring a modest, accepted, and intentional overhead.
- *Disposition:* **Selected.**

---

## 5. Decision

It is hereby recorded that **Option D** is adopted. The constituent sub-decisions (each a "Sub-Decision") are set out below. For the avoidance of doubt, and without limiting the generality of the foregoing, each Sub-Decision is binding upon the implementation unless and until amended through the change-control process.

### 5.1 Sub-Decision DEC-01 — Resident Kernel Structure (Two-File Model)

The resident kernel shall be partitioned into two (2) components, consistent with heritage practice and for reasons of separation of concerns:

- `IO.SYS` — the hardware-dependent component, comprising the resident device drivers and the system-initialization module ("SYSINIT"), which is responsible for kernel initialization and for processing the configuration file (`CONFIG.SYS`).
- `INITDOS.SYS` — the hardware-independent kernel, comprising the system-call dispatcher (§5.4), the file system (§5.7), the memory arena (§5.3), process and PSP management (§5.5), and the System File Table (§5.6).

Both files shall carry the hidden, system, and read-only attributes and shall occupy the first two (2) entries of the volume root directory. The two-file partition is classified **load-bearing** (architectural), notwithstanding that the said partition is largely organizational.

### 5.2 Sub-Decision DEC-02 — Operating Personality, Not Binary Compatibility

InitechDOS shall provide the DOS 3.3 *personality* to native 32-bit applications. The execution of genuine 16-bit real-mode executables is **out of scope** for the current release (see §1.2, §4.3).

### 5.3 Sub-Decision DEC-03 — Memory Arena (Retained on a Vestigial Basis)

A Memory Control Block chain (the "arena") shall be constructed and maintained, comprising sixteen-byte arena headers each bearing the conventional signature byte (`'M'` for a non-terminal block; `'Z'` for the terminal block), an owner identifier, and a size expressed in paragraphs. The allocation, release, and resize services (§5.4, functions `48h`/`49h`/`4Ah`) shall operate upon this chain.

It is acknowledged and accepted that, upon the 32-bit flat substrate, the segmented-paragraph model, the 640-kilobyte conventional ceiling, and the resident/transient division of the command interpreter are functionally unnecessary. These elements are classified **vestigial** and are retained in full for fidelity. The behaviour whereby the transient command interpreter is reloaded from disk following its overwrite (and the associated operator prompt, MSG-DOS-0013, Appendix C) is retained on a discretionary basis as a fidelity feature.

### 5.4 Sub-Decision DEC-04 — System-Call Interface (`INT 21h`) and Dual File API

The system-call surface shall be presented via interrupt `21h`, with the function selected by the value in register `AH`, as catalogued in **Appendix A**. Both heritage file-access models shall be implemented to ensure conformance:

- the **handle-based** model (introduced at the DOS 2.0 level), classified **load-bearing**; and
- the legacy **File Control Block (FCB)** model (CP/M-derived), classified **vestigial** but implemented for conformance.

This Sub-Decision introduces no new behaviour beyond that specified in Appendix A.

### 5.5 Sub-Decision DEC-05 — Program Segment Prefix (Retained on a Vestigial Basis)

Upon loading a program, a 256-byte Program Segment Prefix shall be constructed in accordance with **Appendix B.2**, including the terminating instruction at offset `00h`, the environment-segment pointer at offset `2Ch`, the command-tail and default Disk Transfer Area at offset `80h`, and the two default File Control Blocks. The majority of the PSP is classified **vestigial** and is retained in full.

### 5.6 Sub-Decision DEC-06 — File Handle Model (Load-Bearing)

A per-process Job File Table of twenty (20) entries shall map process file handles to entries in a system-wide System File Table, the capacity of which shall be governed by the `FILES=` directive of `CONFIG.SYS`. Handles `0` through `4` shall be predefined (standard input, standard output, standard error, standard auxiliary, standard printer). Input/output redirection shall be effected by the command interpreter through handle duplication (functions `45h`/`46h`). This Sub-Decision is classified **load-bearing**; all file and device input/output depends upon it.

### 5.7 Sub-Decision DEC-07 — File System (Load-Bearing)

The file system shall be the File Allocation Table file system, in its 12-bit (`FAT12`, for diskette media) and 16-bit (`FAT16`, for fixed-disk media) variants. The on-volume layout shall be: boot sector; first File Allocation Table; second (redundant) File Allocation Table; fixed-size root directory; data area. Directory entries shall conform to **Appendix B.1**, including the eight-plus-three (8.3) name convention, the attribute byte, and the cluster-chain pointer. This Sub-Decision is classified **load-bearing**.

### 5.8 Sub-Decision DEC-08 — Executable Format

The resident kernel shall be a flat binary image. Application executables shall, for the current release, employ a flat (`.COM`-equivalent) image. A relocatable `MZ`-format (`.EXE`) loader is **deferred** and shall be addressed when applications are realized as independently loadable files. (See §1.2 regarding genuine third-party executables.)

### 5.9 Sub-Decision DEC-09 — Device-Driver Model and Network Redirection

Device drivers shall conform to the heritage device-header model, exposing a Strategy entry point and an Interrupt entry point and responding to the standard command set by way of request packets. The following character devices shall be resident: `CON`, `PRN`, `AUX`, `CLOCK$`, and `NUL`. Installable drivers shall be loadable by way of the `DEVICE=` directive of `CONFIG.SYS`. A network redirector, registered through the multiplex interface (interrupt `2Fh`), shall provide mapped network drives, consistent with the operating environment depicted in the Reference Frame and with the enterprise operating context generally.

### 5.10 Sub-Decision DEC-10 — Critical-Error and Termination Handlers

Handlers shall be provided for the critical-error interrupt (`24h`), the control-break interrupt (`23h`), and the program-termination interrupt (`22h`). The critical-error handler shall present the diagnostic message MSG-DOS-0001 (Appendix C), and the operator response shall be processed accordingly. The vectors for these handlers shall be preserved in the Program Segment Prefix of the invoking process (Appendix B.2).

### 5.11 Sub-Decision DEC-11 — Command Interpreter

The command interpreter shall be designated `COMMAND.COM` (the canonical designation being retained without amendment). It shall provide internal commands, locate external commands by way of the `PATH` environment variable, support batch (`.BAT`) processing, present the operator prompt in the form `$P$G`, and resolve its own location by way of the `COMSPEC` environment variable. The operator banner shall conform to Appendix D.

### 5.12 Sub-Decision DEC-12 — Nomenclature and Branding

The base operating system shall be branded "InitechDOS." The version shall be designated `3.30`. The copyright notice shall identify Initech Systems Corporation and the year 1991. The configuration files shall retain the canonical designations `CONFIG.SYS` and `AUTOEXEC.BAT` without amendment. Where a choice exists between a distinctive designation and a generic, heritage-canonical designation, the generic designation shall be preferred, in furtherance of DR-6.

### 5.13 Sub-Decision DEC-13 — Diagnostic-Message Fidelity (Controlled Vocabulary)

Diagnostic messages shall conform verbatim to the Approved Diagnostic Message Catalogue (**Appendix C**), which constitutes a controlled vocabulary. No diagnostic message shall be added, removed, reworded, capitalized, punctuated, or otherwise modified except by way of an approved change to this Record. This Sub-Decision exists in furtherance of both DR-1 (Fidelity) and DR-6 (Experiential Conformance).

---

## 6. Consequences

### 6.1 Positive Consequences

- The Platform attains a base operating system aligned with the Reference Frame and the period (DR-1, DR-3).
- Each architectural element carries a defined verification oracle (§7), supporting automated conformance (DR-2).
- The architecture is realizable by the Programme Build Capability (DR-4) and does not preclude the North Star (DR-5).
- A consistent, uniform, and dependable experience of tedium is achieved across the Base OS (DR-6).

### 6.2 Negative Consequences

- Certain structures (the memory arena, the Program Segment Prefix, the FCB model) are retained without functional necessity, incurring an accepted, intentional, and modest implementation and maintenance overhead (§5.3, §5.5, §5.4).

### 6.3 Neutral Consequences

- The two-file kernel partition (§5.1) is largely organizational and is recorded for completeness.
- No change is anticipated to the procurement of foam packaging inserts.

### 6.4 Risks

Risks arising from this Decision are recorded in the Programme Risk Register (RR-0003) and are not reproduced here in full. The principal residual risk is **specification drift** within the automated implementation function, mitigated by the controlled vocabularies and structure specifications in the Appendices and by the Conformance regime (§7).

---

## 7. Compliance and Conformance

Conformance of the implementation to this Record shall be verified by the Emulation Conformance Harness (the "ECH") in accordance with the quality procedure for the Platform. Verification shall include, without limitation:

1. **Differential behavioural verification** of the `INT 21h` surface and the file system against a Reference Implementation (a genuine disk operating system of the period, executed upon the host emulator, per ADR-0001 §8), the said Reference Implementation being retained for verification purposes only (§4.2).
2. **Byte-identity verification** of File Allocation Table volume round-trips against the Reference Implementation.
3. **Boot-stage milestone verification** by way of diagnostic markers emitted upon the serial channel.
4. **Message-catalogue conformance** confirming that diagnostic output corresponds, verbatim, to Appendix C.

Non-conformances shall be raised, triaged, and dispositioned in the ordinary course.

---

## 8. Non-Functional Requirements (Selected)

- **NFR-1 (Period Plausibility).** No capability anachronistic to the stated period shall be exhibited.
- **NFR-2 (Determinism).** Behaviour shall be deterministic to the extent required for byte-identity verification (§7).
- **NFR-7 (Experiential Conformance).** The Base OS shall present, at all times and without exception, a comprehensive, uniform, and dependable experience of tedium consistent with enterprise software of the 1989–1993 period. This requirement is mandatory and is not waivable.

---

## 9. Related Decisions and References

- ADR-0001 — Target Hardware Platform and Memory Model. *(Accepted.)*
- ADR-0002 — Toolchain, Implementation Language, and Executable Format. *(Accepted.)*
- ADR-0004 — Graphical Toolbox and Region Engine. *(Pending.)*
- ADR-0006 — InitechBase Database Application. *(Pending.)*
- ADR-0007 — Turbo Initech Self-Hosting Compiler. *(Pending.)*
- RR-0003 — Programme Risk Register, InitechDOS section.
- INITECH-STD-0042 — Corporate Standard for the Authoring of Architecture Decision Records.
- RECORDS-POL-002 — Document Review and Retention Policy.
- Memo 1.A.4 — Revised Cover-Sheet Guidance.

---

## Appendix A — `INT 21h` Function Register (Implementation Set)

Functions are selected by register `AH`. Conformance Class is one of: **Core** (handle-based / process / memory), **Legacy** (FCB / CP/M-derived, vestigial), **Resident** (console / date-time / TSR).

| AH | Mnemonic | Description | Class |
|---|---|---|---|
| 00h | TERMINATE | Terminate program (legacy) | Resident |
| 01h–0Ch | CON I/O | Character input/output, buffered input, status | Resident |
| 0Eh / 19h | SELDISK / GETDISK | Select / get default drive | Resident |
| 1Ah / 2Fh | SETDTA / GETDTA | Set / get Disk Transfer Area | Resident |
| 25h / 35h | SETVECT / GETVECT | Set / get interrupt vector | Core |
| 2Ah–2Dh | DATE / TIME | Get/set system date and time | Resident |
| 30h | GETVER | Get operating system version | Resident |
| 31h | KEEP (TSR) | Terminate and stay resident | Resident |
| 36h | GETSPACE | Get free disk space | Core |
| 39h / 3Ah / 3Bh | MKDIR / RMDIR / CHDIR | Directory operations | Core |
| 3Ch | CREAT | Create or truncate file (handle) | Core |
| 3Dh | OPEN | Open file (handle) | Core |
| 3Eh | CLOSE | Close file (handle) | Core |
| 3Fh | READ | Read from file or device (handle) | Core |
| 40h | WRITE | Write to file or device (handle) | Core |
| 41h | UNLINK | Delete file | Core |
| 42h | LSEEK | Move file pointer | Core |
| 43h | CHMOD | Get/set file attributes | Core |
| 44h | IOCTL | Device control | Core |
| 45h / 46h | DUP / DUP2 | Duplicate / force-duplicate handle (redirection) | Core |
| 47h | GETCWD | Get current directory | Core |
| 48h / 49h / 4Ah | ALLOC / FREE / SETBLOCK | Allocate / free / resize memory (arena) | Core |
| 4Bh | EXEC | Load and/or execute program | Core |
| 4Ch | EXIT | Terminate with return code | Core |
| 4Dh | WAIT | Get child return code | Core |
| 4Eh / 4Fh | FINDFIRST / FINDNEXT | Directory search | Core |
| 56h | RENAME | Rename file | Core |
| 57h | FILETIME | Get/set file date and time | Core |
| 59h | GETERR | Get extended error information | Core |
| 5Bh | CREATNEW | Create file, fail if existing | Core |
| 62h | GETPSP | Get Program Segment Prefix address | Core |
| 0Fh–24h | FCB ops | Open/close/find/read/write/delete (File Control Block) | Legacy |

*This register is the controlled scope. Functions not listed are out of scope absent an approved change.*

---

## Appendix B — On-Disk and In-Memory Structure Layouts

### B.1 Directory Entry (32 bytes)

| Offset | Size | Field |
|---|---|---|
| 00h | 8 | Filename (space-padded, upper-case) |
| 08h | 3 | Extension (space-padded, upper-case) |
| 0Bh | 1 | Attribute byte (RO 01h / Hidden 02h / System 04h / VolLabel 08h / Directory 10h / Archive 20h) |
| 0Ch | 10 | Reserved |
| 16h | 2 | Time of last modification (two-second resolution) |
| 18h | 2 | Date of last modification |
| 1Ah | 2 | Starting cluster |
| 1Ch | 4 | File size in bytes |

First byte `00h` denotes an unused entry (end of directory); `E5h` denotes a deleted entry.

### B.2 Program Segment Prefix (256 bytes; selected offsets)

| Offset | Field |
|---|---|
| 00h | `INT 20h` instruction (legacy termination) |
| 02h | Segment of first byte beyond allocated memory |
| 0Ah–15h | Saved `INT 22h` / `23h` / `24h` vectors |
| 16h | Parent Program Segment Prefix |
| 18h–2Bh | Job File Table (20 handle entries) |
| 2Ch | Environment-block segment |
| 50h | `INT 21h` / far-return entry |
| 5Ch / 6Ch | Default File Control Blocks (#1, #2) |
| 80h | Command-tail length and text; also default Disk Transfer Area |

### B.3 Memory Control Block / Arena Header (16 bytes)

| Offset | Size | Field |
|---|---|---|
| 00h | 1 | Signature: `'M'` (non-terminal) or `'Z'` (terminal) |
| 01h | 2 | Owner identifier (0 = free) |
| 03h | 2 | Block size in 16-byte paragraphs |
| 05h | 11 | Reserved |

---

## Appendix C — Approved Diagnostic Message Catalogue (Controlled Vocabulary)

No entry shall be modified except by an approved change to this Record (§5.13). `%c` denotes a drive letter substitution.

| Message ID | Text |
|---|---|
| MSG-DOS-0001 | `Abort, Retry, Fail?` |
| MSG-DOS-0002 | `Bad command or file name` |
| MSG-DOS-0003 | `File not found` |
| MSG-DOS-0004 | `Invalid drive specification` |
| MSG-DOS-0005 | `Insufficient disk space` |
| MSG-DOS-0006 | `Not ready reading drive %c:` |
| MSG-DOS-0007 | `General failure reading drive %c:` |
| MSG-DOS-0008 | `Write protect error writing drive %c:` |
| MSG-DOS-0009 | `Access denied` |
| MSG-DOS-0010 | `Too many parameters` |
| MSG-DOS-0011 | `Required parameter missing` |
| MSG-DOS-0012 | `Are you sure (Y/N)?` |
| MSG-DOS-0013 | `Insert disk with COMMAND.COM in drive %c:` |
| MSG-DOS-0014 | `Bad or missing Command Interpreter` |
| MSG-DOS-0015 | `Non-System disk or disk error` |
| MSG-DOS-0016 | `Press any key to continue . . .` |

---

## Appendix D — Approved Baseline Configuration

### D.1 Operator Banner

```
InitechDOS  Version 3.30
Copyright (C) 1991 Initech Systems Corporation.  All Rights Reserved.
```

### D.2 `CONFIG.SYS` (Approved Baseline)

```
FILES=20
BUFFERS=20
LASTDRIVE=Z
DEVICE=ANSI.SYS
DEVICE=INITNET.SYS
INSTALL=SHARE.EXE
SHELL=COMMAND.COM /P /E:512
```

### D.3 `AUTOEXEC.BAT` (Approved Baseline)

```
@ECHO OFF
PATH C:\DOS;C:\INITECH
INITNET LOGIN /SERVER:INITSRV01
ECHO Welcome to InitechDOS.  Please contact the Help Desk (ext. 2504).
PROMPT $P$G
```

---

*— End of Record —*

<!--
This document is the confidential and proprietary information of Initech Systems
Corporation. Unauthorized review, use, disclosure, or distribution is prohibited.
If you have received this document in error, please shred it and notify the Help
Desk (ext. 2504). This footer is part of the controlled document and shall not be
removed. Tedium certified compliant with NFR-7.
-->
