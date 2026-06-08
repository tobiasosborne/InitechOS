<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0001 — Programme Engineering Work Record (PEWR)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Engineering Work Record (Worklog)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | PE-WL-0001 |
| Title | Programme Engineering Work Record — Foundations Phase and InitechDOS Reprioritization |
| Version | 1.0 |
| Status | Issued |
| Classification | Internal Use Only |
| Period Covered | 2026-06-08 (single working session) |
| Document Owner | Platform Engineering |
| Recording Function | Build Orchestration (automated) |
| Effective Date | 2026-06-08 |
| Retention | Per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |
| Related | ADR-0003 (OEA-ADR-0003); PRD InitechOS-PRD.md; CLAUDE.md |

---

## 1. Purpose

The purpose of this Work Record (the "Record") is to memorialize, for audit and continuity purposes, the engineering activities undertaken during the period stated above; to record the architectural decisions noted during the said period; to document the reprioritization of Programme deliverables consequent upon the ratification of ADR-0003; and to register the dispatch of a conformance assessment. This Record is provided for governance purposes and confers no entitlement, expectation, or warranty.

## 2. Programme Phase

The Programme entered the present period in the **Foundations** phase (Milestone M0) with a thin end-to-end vertical demonstration (the "Tracer Bullet," Milestone M0.5) in progress. The phase designation at the close of the period is set out at §6.

## 3. Summary of Work Performed

The following work items (the "Items") were planned, delegated to the automated implementation function, independently verified by the recording function, and dispositioned as **Closed**. Each Item carries a mechanical verification oracle (a "Gate") consistent with the quality regime.

| Bead | Item | Disposition | Gate of Record |
|---|---|---|---|
| initech-tse | Repository skeleton and factory Makefile | Closed | `make factory` (a defect — a non-failing oracle stub — was identified and remediated) |
| initech-uba | Emulation and assembler toolchain provisioning | Closed | QEMU 8.2.2, NASM 2.16.01, Bochs provisioned by the operator |
| initech-znb | Seed cross-compiler (lexer, parser, code generation) | Closed | `make test-seed`, `make test-seed-codegen` |
| initech-f2s | Emulation Conformance Harness (QEMU driver) | Closed | `make test-harness` |
| initech-f8v.1 | Tracer Smoke Test (`make smoke`) | Closed | `make smoke` |
| initech-f8v.2 | Tracer Boot (MBR → protected/flat → VESA LFB) | Closed | `make test-tracer-boot` |

It is recorded that, at the close of the period, six (6) verification Gates pass without exception: `make smoke`, `make test-seed`, `make test-seed-codegen`, `make test-harness`, `make test-tracer-boot`, and the `factory` build. A seed-compiled program is observed to boot under emulation and to emit a diagnostic marker upon the serial channel; a from-scratch boot chain is observed to attain the 32-bit protected/flat mode and to render the framebuffer, the said rendering being verified at the level of the individual picture element.

Subsidiary non-conformances surfaced during verification were raised in the ordinary course as beads (`initech-7r0`, `initech-rgd`, `initech-xcg`, `initech-fot`, `initech-ta2`).

## 4. Architectural Decision Noted — ADR-0003

It is noted that **ADR-0003 (InitechDOS Base Operating System Personality and Resident Kernel Architecture)** was ratified effective 2026-06-08 and is, accordingly, authoritative. The Record adopts the position that, where the Product Requirements Document or the engineering conventions (CLAUDE.md) diverge from a ratified ADR, **the ADR shall govern**, and the implementation plan (the beads) and supporting documents shall be brought into conformance through the change-control process.

ADR-0003 selects Option D (a DOS 3.3 personality presented by a native 32-bit kernel) and establishes thirteen (13) binding Sub-Decisions and four (4) controlled Appendices (the `INT 21h` function register; the on-disk and in-memory structure layouts; the Approved Diagnostic Message Catalogue; and the Approved Baseline Configuration).

## 5. Reprioritization Directive

Pursuant to operator direction received during the period, and consistent with ADR-0003, it is recorded that **InitechDOS (the Base Operating System, Milestone M2, codename MILTON) is hereby designated the first Programme deliverable and the first Programme milestone.** The graphical Toolbox workstream (addressed under ADR-0004, pending) and the associated visual asset workstream are, accordingly, **deferred** in sequence, notwithstanding any contrary sequencing previously recorded. The Tracer Bullet (M0.5) is to be re-pointed toward a DOS-aligned vertical slice (boot → InitechDOS banner → command interpreter → directory and file operations over the File Allocation Table file system), pending the outcome of the assessment noted at §7.

## 6. Phase Disposition at Close

The Programme is recorded as transitioning from a Toolbox-leading sequence to an **InitechDOS-leading** sequence. The Foundations factory (M0) and the verified portions of the Tracer Bullet (M0.5) remain in force and are not invalidated. Final re-sequencing of the beads is deferred to the assessment (§7).

## 7. Conformance Assessment Dispatched

A conformance assessment (read-only) has been dispatched to determine the extent to which the existing implementation plan (the beads), the Product Requirements Document, and the engineering conventions conform to ADR-0003 (and to the decisions ADR-0003 attributes to ADR-0001 and ADR-0002). The assessment is directed to report, without limitation, upon: the implementation-language question (C, per ADR-0002, versus the Pascal artifact recorded in the PRD); the toolchain question (`i686-elf` cross-compilation); the executable-format question (flat `.COM`-equivalent per DEC-08, versus the previously-recorded MZ disposition); the decomposition of the DOS workstream against DEC-01…DEC-13 and the Appendices; and the extraction of the controlled Appendices as locked specification data. The assessment's findings shall be dispositioned in a subsequent Record.

## 8. Provisional Non-Conformances Identified (Pending Assessment)

The following provisional non-conformances were observed by the recording function and are stated here for completeness; their formal disposition awaits the assessment (§7):

1. **PNC-1 (Implementation Language).** The PRD and CLAUDE.md record the OS artifact as Pascal; ADR-0002 (per ADR-0003 §2) mandates C via `i686-elf`. *Escalated for operator ruling.*
2. **PNC-2 (Executable Format).** PRD §15 records MZ `.EXE` as decided for applications; DEC-08 defers MZ and adopts a flat `.COM`-equivalent for the current release.
3. **PNC-3 (DOS Decomposition Granularity).** The existing M2 beads (`m2-1`…`m2-7`) are materially coarser than the thirteen Sub-Decisions and four Appendices of ADR-0003.

## 9. Open Actions

| Action | Owner | Status |
|---|---|---|
| Complete conformance assessment and disposition findings | Build Orchestration | In progress (§7) |
| Obtain operator ruling on PNC-1 (language) | Operator | Open |
| Re-sequence beads to InitechDOS-first upon assessment | Build Orchestration | Pending assessment |
| Extract controlled Appendices as locked spec-data | Build Orchestration | Pending assessment |

---

*— End of Record —*

<!-- Tedium certified compliant with NFR-7. If you have received this document in error, please shred it and notify the Help Desk (ext. 2504). -->
