<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0003 Amendment DEC-15a — INT 25h/26h Absolute Disk I/O on Every Mounted Volume

**Issuing Body:** Initech Systems Corporation — Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record Amendment (ADR-A)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0003-A7 |
| Title | ADR-0003 Amendment DEC-15a: INT 25h/26h Absolute Disk I/O on Every Mounted Volume |
| Version | 1.0 |
| Status | **Ratified** (PM decision, 2026-10-23, lane drivext merge ruling) |
| Classification | Internal Use Only |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | STAPLER Programme, lane drivext |
| Effective Date | 2026-10-23 |
| Next Scheduled Review | 2027-04-23 (semi-annual) |
| Supersedes | **Partial** — supersedes the *single-volume clauses* of ADR-0003 DEC-15 (DEC-15.2 drive row "the only valid AL is 0"; Out-of-scope "Multi-volume drive resolution"; Consequence C-6's multi-volume deferral). The register layout, error-code table, packet-form rejection, reentrancy bracket and every other DEC-15 provision are **RETAINED, UNCHANGED**. |
| Related Documents | ADR-0003 DEC-15 (INT 25h/26h); ADR-0003 DEC-17 (physical block-device service); ADR-0003 DEC-18 (drive letters); `spec/absdisk_int2526.json`; `spec/drive_letters.json`; WL-0156 |
| Related Issues | beads initech-yyi2 (this amendment); initech-q45q (the fix lane); audit `docs/audits/2026-10-23-dos` L013, L015 |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |

---

## 1. Context

ADR-0003 DEC-15 admitted INT 25h/26h for a release that mounted exactly one
volume, and so accepted only AL=0; any other AL failed with the invalid-drive
pair. It named multi-volume resolution "a separate future work item".

ADR-0003 DEC-18 (lane dosvolumes, WL-0156) then made A:, C: and D: real,
independently mounted volumes. The file system reached all three; the
absolute-disk vectors still reached only A:. The 2026-10-23 DOS audit
recorded the result:

- **L013** — `CHKDSK C:` and `CHKDSK D:` print `Disk error reading FAT` on
  valid volumes that DIR, TYPE and COPY read and write.
- **L015** — `VOL D:` shows the label, but `LABEL D:` refuses the drive.

Both utilities are, per DEC-15 §1, built on the period primitive of absolute
sector I/O. The MS-DOS 3.3 User's Reference gives them a drive operand:
`chkdsk [drive:][pathname]` (p.43, PDF128) and `label [drive:][label]`
(p.86, PDF171).

## 2. Decision

### DEC-15a — AL names any volume DEC-18 currently mounts.

INT 25h/26h AL remains zero-based and explicit (0=A:, 1=B:, 2=C:, 3=D:;
DEC-15.2 unchanged). AL is valid **exactly** when DEC-18 currently mounts that
letter. Any other AL — B:, an absent D:, a partition that failed to mount,
AL ≥ 4 — fails loud with the locked invalid-drive pair (AL=0x0F, AH=0x0C)
before any I/O. The locked spec text now reads "AL is not a mounted volume".

### DEC-15a.1 — One seam per letter, bound with the mount.

The kernel keeps one absolute-disk seam per letter
(`int21_set_absdisk_drive`; `int21_set_blockdev` is the A: case). A C:/D:
seam is bound when that letter mounts, and unbound when a rewritten boot
record no longer mounts (`mount_fixed_volume`). It reads and writes the
volume's own partition-relative sectors through the same DEC-07a origin as
the FAT mount. Its bounds check uses that volume's own total sector count.

### DEC-15a.2 — FAT16 volumes have no write handler.

WL-0156 keeps FAT16 read-only. A FAT16 volume's seam is bound with a read
handler and **no** write handler, so INT 26h reports the locked
write-protect pair (AL=0x00, AH=0x0A, DEC-15.2). A FAT12 seam has a writer
when the device does.

### DEC-15a.3 — One per-drive FAT refresh path.

An INT 26h write to C:/D: goes through the same notification as DEC-17's
INT 13h writes (`fixed_volume_written`). A write to logical sector 0 remounts
the volume; any other write mirrors its primary-FAT bytes into that letter's
cache (`fileio_fat_raw_written`). The cached FAT therefore never goes stale
under CHKDSK /F or LABEL, and one letter's write never touches another
letter's cache. A: keeps its DEC-15 notification (`fileio_fat_absdisk_written`).

## 3. Rationale

- DEC-18 already defines which letters are volumes. Answering INT 25h/26h
  from the same mount table means the vectors cannot disagree with the file
  system about whether a drive exists.
- The alternative — CHKDSK and LABEL computing partition origins themselves
  through INT 13h AH=60h — would bypass the kernel's FAT caches and duplicate
  the DEC-18 resolver. DEC-18 says programs never interpret an MBR.
- No new code space, register role or message is introduced; DEC-15's
  honest error table covers every condition.

## 4. Oracle

- `make test-drive-operands`: QEMU, a two-volume boot disk (C = BIOS 80h
  partition 1, D = 80h partition 2, A = primary slave). `CHKDSK C:` and
  `CHKDSK D:` must report the total disk space computed from the
  mtools-minted boot sector and the free bytes `mdir` reports. `LABEL D:` must
  leave the label `mlabel` reads on D: and nowhere else. `VOL D:` must show
  it. `fsck.fat` must pass on every volume.
- `make test-drive-operands-mutant`: the `absdisk` leg boots a kernel built
  with `ABSDISK_MUTATE_DRIVE_A_ONLY` (the pre-DEC-15a vector) and must go RED
  on `L013 CHKDSK C: reported a disk error`. The `label` leg
  (`DU_MUTATE_LABEL_DRIVE`) must go RED on L015.
- `make test-absdisk`, `test-absdisk-mutant`, `test-absdisk-emu`: the DEC-15
  A: contract and its eight mutants stay green, unchanged.

## 5. Supersession

The DEC-15 clauses listed under Document Control are superseded only as far
as they restrict AL to 0. The vector numbers, the register layout, the CX=0xFFFF
packet rejection, the bounds-before-I/O order, the DEC-14 buffer guard, the
error-code table, the stack-flags disposition and the reentrancy bracket stand
as ratified.
