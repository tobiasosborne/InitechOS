<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0003 — Programme Engineering Work Record (PEWR)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Engineering Work Record (Worklog)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | PE-WL-0003 |
| Title | InitechDOS (MILTON) FAT12 Read Path and Differential Oracle |
| Version | 1.0 |
| Status | Issued |
| Classification | Internal Use Only |
| Period Covered | 2026-06-08 |
| Recording Function | Build Orchestration (supervised multi-agent) |
| Related | WL-0002; ADR-0003 (DEC-07); PRD Sec 6.1, Sec 11 (M2); beads initech-adf, initech-8e7, initech-5cu, initech-dao |

---

## 1. Purpose

This Record memorializes the implementation of the FAT12 **read** path for the
InitechDOS kernel (codename MILTON) and its mechanical differential oracle, being
the first substantive M2 (DOS personality) deliverable. The work was executed as a
supervised, serial multi-agent orchestration: a research pass established ground
truth; four implementation passes built the subsystem under Red→Green discipline;
the orchestrator independently verified each pass against the locked spec and the
oracle.

## 2. Scope of the Period

The period delivered the FAT12 **read** path and its oracle. FAT12/16 **write**
(the DEC-07 re-scope of initech-adf) and the byte-identity round-trip oracle
(initech-509.11) are expressly **out of scope** and remain open.

## 3. Ground Truth Established (Law 1)

A research pass minted a real 1.44 MB FAT12 volume with the period reference tool
(`mformat` 4.0.43) and read its actual bytes, producing
`docs/research/fat12-ground-truth.md`: the BPB geometry, derived-geometry formulae,
the 12-bit FAT decode rule (worked example verified on real bytes), directory read
semantics, the differential-oracle reference command set, and seven enumerated
implementation risks. The orchestrator independently re-minted a volume and
confirmed the geometry byte-for-byte.

## 4. Spec Disposition

A **spec gap** was surfaced and closed: the BIOS Parameter Block was only
glossary-mentioned in ADR-0003, not locked as data (Rule 8). A packed 62-byte
`bpb_t` (boot prefix + BPB proper + extended BPB), with a compile-time size
assertion and derived-geometry macros, was extracted into `spec/dos_structs.h` and
placed under the `make test-spec` gate (beads initech-8e7, closed).

## 5. Artifacts Delivered

- **Artifact (C, freestanding, no heap — Law 3):** `os/milton/blockdev.h` (sector
  block-device interface); `os/milton/fat12.{h,c}` (mount + BPB parse + FAT12/16
  classification; whole-FAT read eliminating the sector-straddle case (RISK-1);
  12-bit entry decode; cluster-chain walk with a mandatory anti-hang loop guard
  (Rule 2); 8.3 name formatting; root-directory enumeration with correct
  end-of-directory/deleted/LFN handling; find-by-name; and exact-size file read
  honouring the partial last cluster (RISK-5)); `os/milton/ata.{h,c}` (ATA PIO
  LBA28 read backend — compiles freestanding; hardware validation **deferred** to
  M1 boot, as it is not host-testable).
- **Factory (C / Python, hosted):** `harness/diff/fat_diff/blockdev_file.{h,c}`
  (host image-backed block device); `fat_dump.c` (the artifact-under-test's
  normalized manifest/extract tool); `fat12_ref.py` (an independent third
  implementation); three unit oracles (`test_fat12_bpb`, `test_fat12_chain`,
  `test_fat12_dir`); deterministic image-mint rules and fixtures.

## 6. Oracle Disposition (Law 2)

`make test-fat`, previously a failing stub, is now a **real gate** (beads
initech-5cu, closed). It asserts triple agreement — our reader == `mtools`
(`mdir`/`mcopy`) == the independent Python reader — on the file-name set, each
file's size, and each file's content bytes, across five fixtures (including an
empty file, an exact-cluster-multiple file, and a multi-cluster file with a partial
last cluster). It fails loud if a reference tool is absent. The gate was
**mutation-proven** (Rule 6): the implementing passes demonstrated three distinct
implementation perturbations driving the gate red; the orchestrator then
**independently** removed the file-size truncation, observed the gate go red at the
RISK-5 guard (exit 2), restored, and confirmed the source unchanged (SHA-256
identical) and the gate green.

## 7. Verification of Record

At the close of the period the following pass without exception: `make test-spec`;
the three FAT12 unit oracles (24 / 20 / 73 checks, zero failures); and `make
test-fat` (the differential gate). The artifact translation units compile clean
under both the hosted house flags and `gcc -m32 -ffreestanding -nostdlib
-std=c11 -Wall -Wextra -Werror`. All new sources are ASCII-clean (Rule 12). No
2026-isms or nondeterminism were introduced (Rule 11).

## 8. Follow-On Items Filed

- **initech-dao (P2):** `fat12_read_file` holds a geometry-sized cluster array
  (~5.6 KiB) on the stack; safe for the floppy but a kernel-stack hazard for the
  FAT16 HDD path — recorded as a blocker of initech-z01.
- **initech-adf** remains open for the FAT12/16 **write** re-scope; **initech-509.11**
  (byte-identity round-trip oracle) remains open and is the natural successor.
- A durable note (issue-tracker memory) records that the directory-scan
  end-of-directory termination is *not* exercised by a naturally minted image and
  is constrained only by the synthetic unit case — which must be retained.

## 9. Phase Disposition

The FAT12 read path is complete, oracle-green, and trustworthy. The natural next
increment is FAT write + the byte-identity oracle (initech-509.11), after which the
program loader and `INT 21h` file surface (initech-my3) can consume the file
system. The read path does **not** unblock a booted file system on its own; the M1
boot-to-text chain remains the gating dependency for exercising MILTON on the
emulator.

---

*— End of Record —*

<!-- Tedium certified compliant with NFR-7. -->
