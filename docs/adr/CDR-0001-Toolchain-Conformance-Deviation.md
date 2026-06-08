<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# CDR-0001 — Conformance Deviation Record: Interim Implementation Toolchain

**Issuing Body:** Initech Systems Corporation — Office of Enterprise Architecture (OEA)
**Document Class:** Conformance Deviation Record (CDR) / Engineering Variance Authorization
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-CDR-0001 |
| Title | Interim Implementation Toolchain — Deviation from ADR-0002 |
| Version | 1.0 |
| Status | **Approved — Time-Limited** |
| Classification | Internal Use Only |
| Document Owner | Office of Enterprise Architecture |
| Deviates From | ADR-0002 (Toolchain, Implementation Language, and Executable Format) |
| Effective Date | 2026-06-08 |
| Expiry / Review Trigger | Migration of the development environment to a more capable host device |
| Retention | Per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Requesting Engineer | Build Orchestration (automated) | Submitted | 2026-06-08 |
| Approving Authority | Operator (T. Osborne) | Approved | 2026-06-08 |
| Records Management | (filed) | Filed | 2026-06-08 |

---

## 1. Purpose

This Conformance Deviation Record (the "Deviation") documents, for audit and continuity purposes, an approved, time-limited departure from the toolchain provision established under ADR-0002, and records the interim measure adopted, the justification therefor, and the conditions governing remediation.

## 2. Requirement Deviated From

ADR-0002 (as recited in ADR-0003 §2) establishes `i686-elf` cross-compilation as the toolchain for the implementation of the Platform. A bare-metal `i686-elf` toolchain is not, at the time of this Deviation, provisioned upon the development host, is not available through the host package repository, and would require a from-source build (binutils and GCC) materially burdensome upon the present development device.

## 3. Interim Measure (Approved)

Pending remediation, the Programme shall employ the host system compiler in a freestanding configuration — namely `gcc -m32 -ffreestanding -nostdlib` (with `nasm` for assembly and `ld` for linking) — for the production of freestanding 32-bit Platform artifacts. This is the configuration already exercised, and verified under emulation, by the Foundations factory and the Tracer Bullet workstream. **The 32-bit protected/flat memory model and the executable-format decisions of ADR-0001 and ADR-0003 (DEC-08) are unaffected by this Deviation.**

## 4. Justification

The interim measure is functionally sufficient to produce freestanding x86 artifacts that boot under the Emulation Conformance Harness, and preserves Programme velocity on the InitechDOS deliverable without awaiting a burdensome toolchain build. The difference between the host freestanding configuration and a dedicated `i686-elf` cross-toolchain is immaterial to the present conformance objectives (boot-stage markers, `INT 21h` differential behaviour, File Allocation Table byte-identity).

## 5. Remediation

Upon migration of the development environment to a more capable host device, a from-source `i686-elf` cross-toolchain shall be provisioned (a build script is to be prepared by Build Orchestration), and `CC_KERNEL` shall be repointed accordingly. This Deviation shall thereupon be closed. The remediation is tracked under bead `initech-6pm`.

## 6. Risk and Acceptance

The residual risk (host-toolchain idiosyncrasy not representative of a bare-metal target) is mitigated by the tri-emulator differential regime (ADR-0003 §7) and is **accepted** for the duration of this Deviation. No anachronistic capability is introduced (NFR-1); determinism (NFR-2) is preserved.

---

*— End of Record —*

<!-- Tedium certified compliant with NFR-7. -->
