<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0002 — Programme Engineering Work Record (PEWR)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Engineering Work Record (Worklog)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | PE-WL-0002 |
| Title | ADR-0003 Conformance Reconciliation and InitechDOS-First Re-sequencing |
| Version | 1.0 |
| Status | Issued |
| Classification | Internal Use Only |
| Period Covered | 2026-06-08 (continuation of the WL-0001 session) |
| Recording Function | Build Orchestration (automated) |
| Related | WL-0001; ADR-0003 (OEA-ADR-0003); CDR-0001 (OEA-CDR-0001); PRD; CLAUDE.md |

---

## 1. Purpose

This Record memorializes the conformance reconciliation undertaken consequent upon the ratification of ADR-0003 and the operator directive designating InitechDOS the first Programme deliverable. It records the disposition of the provisional non-conformances raised in WL-0001 §8, the decisions obtained, and the artifacts amended.

## 2. Decisions Obtained (Operator)

| Ref | Question | Disposition |
|---|---|---|
| PNC-1 | Implementation language (C per ADR-0002 vs Pascal per PRD) | **Option A.** The OS is implemented in **C**; **Pascal is reserved for Turbo Initech** (the self-hosting compiler, ADR-0007, pending) and programs authored in it. The self-host fixed point concerns Turbo Initech, not the C kernel/Toolbox (which the factory cross-toolchain rebuilds). |
| PNC-2 | Executable format (MZ vs flat) | **DEC-08 adopted.** Flat binary kernel; flat `.COM`-equivalent applications for the current release; MZ `.EXE` **deferred**. |
| Toolchain | `i686-elf` (ADR-0002) provisioning | **Deferred** to a more capable development device. Interim measure: host `gcc -m32 -ffreestanding -nostdlib` + `nasm` + `ld`, recorded as an accepted, time-limited deviation in **CDR-0001**. |
| Design stance | Governing aesthetic posture | Recorded: the blandness is a deliberate, rigorous stance — maximal genericness, every canonical name and vestigial structure preserved in full; InitechDOS is "DOS with the soul extracted and the legacy lovingly preserved." Corporate software accretes and never deletes. |
| Documentation | House style | All documents henceforth authored in enterprise corporate-committee ("Initech") house style (NFR-7, Experiential Conformance). |

## 3. Artifacts Amended

- **Beads.** InitechDOS (epic `initech-509`) re-prioritized to first (P0); the Toolbox milestones (M3/M4) deferred. Ten beads re-scoped for conformance (loader → flat `.COM`/MZ-deferred; FAT → read+write+byte-identity; banner → exact Appendix D.1 text; shell → batch/PATH/COMSPEC/`$P$G`; the M7 rebuild and seed-growth beads re-scoped to Option A; the specs-as-data scaffold). Twelve new DEC-aligned children created under `initech-509` (`509.1`–`509.11`) covering the two-file kernel, JFT/SFT + redirection, full PSP, the `INT 21h` dispatcher, the MCB arena (vestigial, retained in full), the resident device drivers + `INT 2Fh` redirector, the `INT 22/23/24` handlers, the FCB legacy API (vestigial), the diagnostic-message-catalogue enforcement, the Appendix-D baseline configuration, and the FAT byte-identity oracle. The M0.5 Tracer Bullet was inverted from a graphical keystone to an **InitechDOS vertical slice** (`f8v.3`/`f8v.4`). Dependency cycles introduced by the inversion were detected and remediated. A toolchain-provisioning bead (`initech-6pm`) and an ADR-0007 authoring bead (`initech-79s`) were filed.
- **Specification data.** Six controlled files extracted verbatim from the ADR-0003 appendices into `spec/` (the `INT 21h` register, the on-disk/in-memory structures with size assertions, the sixteen-entry diagnostic catalogue, and the byte-exact banner / `CONFIG.SYS` / `AUTOEXEC.BAT` baselines), enforced by the `make test-spec` gate.
- **Product Requirements Document and CLAUDE.md.** Reconciled to Option A, DEC-08, and the CDR-0001 interim toolchain; the "artifact is Pascal" framing removed; Law 3 rewritten; the self-host narrative re-scoped to Turbo Initech.
- **Governance.** CDR-0001 issued (toolchain deviation). Persistent directives recorded via the issue tracker's memory facility.

## 4. Verification of Record

At the close of the period, the following gates pass without exception: `make test-spec`, `make smoke`, `make test-seed`, `make test-seed-codegen`, `make test-harness`, `make test-tracer-boot`, and the `factory` build. No dependency cycles are present. The residual-contradiction inspection of the amended documents returned clean.

## 5. Phase Disposition

The Programme is **InitechDOS-first** and conformant to ADR-0003. The DOS implementation path is unblocked under the interim toolchain. No outstanding operator decision blocks commencement of M2 implementation.

---

*— End of Record —*

<!-- Tedium certified compliant with NFR-7. -->
