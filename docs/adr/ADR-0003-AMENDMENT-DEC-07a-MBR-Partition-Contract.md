<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->
<!-- ============================================================================ -->
<!-- STATUS: RATIFIED 2026-06-15 (operator T. Osborne, "ratify as drafted").       -->
<!-- The operator ratified the three forks as the Chair recommended:               -->
<!--   Fork 1 (start-LBA authority on MBR/BPB conflict): require-match-fail-loud    -->
<!--     (FAT12_ERR_PARTITION), PROVISIONAL pending an 86Box round-trip on a        -->
<!--     mismatched image (likely flips to partition-table-authoritative; erratum). -->
<!--   Fork 2 (offset layer): mount-layer -- kmain discovers the partition, the     -->
<!--     volume carries base_lba, fat12 biases its reads; blockdev stays pure.      -->
<!--   Fork 3 (detection): BIOS-drive-number first + sector-0 byte-sniff cross-     -->
<!--     validation, fail-loud (FAT12_ERR_PARTITION) on irreconcilable.             -->
<!-- The proposed locked spec-data (spec/mbr_partition_contract.json,               -->
<!-- FAT12_ERR_PARTITION, fat12_volume_t.base_lba) is AUTHORIZED but is created in  -->
<!-- the implementing bead (initech-kzfs) per Rule 8 -- NOT by this document.       -->
<!-- ============================================================================ -->

# ADR-0003 Amendment DEC-07a -- On-Disk Partition Contract (MBR Partition-Table Parse + hidden_sectors LBA Offset)

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record Amendment (ADR-A)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0003-A5 (DEC-16 took A4; both ratified 2026-06-15) |
| Title | ADR-0003 Amendment DEC-07a: On-Disk Partition Contract (MBR Partition-Table Parse + hidden_sectors LBA Offset) |
| Version | 1.0 (Ratified) |
| Status | **RATIFIED 2026-06-15 (operator, "ratify as drafted"; Fork 1 PROVISIONAL pending 86Box)** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | P. Gibbons (Software Engineering II), drafting on behalf of the ARB |
| Effective Date | 2026-06-15 |
| Next Scheduled Review | (set on ratification; semi-annual per RECORDS-POL-002) |
| Supersedes | (none; refines DEC-07 by adding the on-disk partition/volume-origin contract that DEC-07 left implicit) |
| Superseded By | (none) |
| Related Documents | ADR-0003 (OEA-ADR-0003) 5.7 (DEC-07 FAT12/16 file system); ADR-0003 Amendment DEC-04a (OEA-ADR-0003-A1); ADR-0003 Amendment DEC-14 (OEA-ADR-0003-A2); ADR-0003 Amendment DEC-15 (OEA-ADR-0003-A3, INT 25h/26h) |
| Related Issues | beads initech-t1on (this decision, tracking); initech-kzfs (gated implementation -- MBR parse + hidden_sectors LBA offset); initech-slvd (forward multi-volume work); initech-bsy (epic) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | 2026-06-15 | P. Gibbons (Software Engineering II) | Initial committee DRAFT for ARB / operator review. Rules on the three WL-0026 kzfs forks (start-LBA authority on MBR/BPB conflict; the application layer for the LBA offset; the volume-detection heuristic), records the per-volume `base_lba` forward obligation for initech-slvd, notes the DEC-15 INT 25h/26h `AL=drive` interaction, and proposes a new locked spec file plus a new `FAT12_ERR_PARTITION` code. **No locked spec-data edited.** Awaiting operator ratification + an 86Box round-trip on a mismatched image for Fork 1. | -- (pending) |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter | P. Gibbons (Software Engineering II) | Submitted (DRAFT) | 2026-06-15 |
| ARB Reviewer -- Technical Correctness | M. Bolton (Senior Engineer, Platform) | (pending) | -- |
| ARB Reviewer -- Period Authenticity | S. Nagheenanajar (Engineering, Heritage Conformance) | (pending) | -- |
| ARB Reviewer -- Governance & Compliance | T. Smykowski (QA / Change Advisory) | (pending) | -- |
| ARB Chair (Synthesis) | (delegated per beads initech-t1on) | **Recommendation accepted by operator** | 2026-06-15 |
| Operator | T. Osborne (Operator) | **RATIFIED (as drafted; Fork 1 provisional pending 86Box)** | 2026-06-15 |
| Records Management | M. Waddams (Archive Annex B) | (filed on ratification) | -- |

*Note on committee composition: This DRAFT was prepared by deliberating the question from four named in-programme perspectives designated in ADR-0003 1.3 -- (1) period-authenticity per real DOS 3.3 / the Microsoft FAT and MBR specifications; (2) robustness / fail-loud (Rule 2); (3) north-star minimalism (do not build the multi-volume machine before it earns its place); and (4) forward-compatibility (initech-slvd multi-volume, FAT16 fixed disk). The Chair recommendations below are the drafter's synthesis for the operator to ratify, amend, or reject. Unlike DEC-15, the operator has NOT delegated ratification authority for this amendment; it is presented as a DRAFT and the three forks are surfaced as explicit decisions-for-operator. Fork 1 additionally carries an external-evidence obligation (an 86Box round-trip) before its ruling is locked.*

---

## 1. Purpose and Scope

### 1.1 Purpose

The purpose of this Amendment (the "Amendment" or "DEC-07a") is to make explicit the **on-disk partition / volume-origin contract** that DEC-07 (ADR-0003 5.7) left implicit. DEC-07 ratified the FAT12/FAT16 file system and the *intra-volume* layout (boot sector; FAT #1; FAT #2; root directory; data area) but said nothing about **where on the physical medium a volume begins** -- i.e. whether sector 0 of the volume is sector 0 of the device, or whether the device carries a Master Boot Record (MBR) partition table and the volume begins at a non-zero start-LBA. The FAT12/16 fixed-disk variant DEC-07 named *requires* this contract, because fixed disks are partitioned and the boot sector lives at the partition's first sector, not the device's.

This Amendment gates beads **initech-kzfs** (MBR partition-table parse + `hidden_sectors` LBA-offset application). It rules on the three forks surfaced by the WL-0026 forward-grounding brief, proposes a new locked spec file (`spec/mbr_partition_contract.json`) and a new error code (`FAT12_ERR_PARTITION`), records the per-volume `base_lba` forward obligation that lets initech-slvd (multi-volume) scale without a refactor, and notes the interaction with the DEC-15 INT 25h/26h `AL=drive` mapping.

### 1.2 Scope

This Amendment governs:

- The authority that resolves the volume start-LBA when the MBR partition entry's start-LBA and the BPB `hidden_sectors` field **conflict** (Sub-Decision DEC-07a.1 -- **Fork 1**).
- The application layer at which the volume-origin offset (`base_lba`) is applied (Sub-Decision DEC-07a.2 -- **Fork 2**).
- The heuristic by which the kernel decides whether a device is partitioned (carries an MBR) or is a raw single-volume medium such as a floppy (Sub-Decision DEC-07a.3 -- **Fork 3**).
- The per-volume `base_lba` data carried by every mounted volume (Sub-Decision DEC-07a.4) and its forward obligation toward initech-slvd.
- The home of the locked spec-data for the partition contract and the new `FAT12_ERR_PARTITION` error code (Sub-Decision DEC-07a.5).

### 1.3 Out of Scope

The following are expressly out of scope of this Amendment:

- **Multi-volume drive resolution / a drive-letter -> volume table.** This release still mounts a SINGLE volume (consistent with DEC-15 1.3). DEC-07a makes that single volume *partition-aware* and carries the per-volume `base_lba` so multi-volume is a clean later extension (initech-slvd), but it does not build the drive table. See Consequence C-5.
- **Booting from a partition / writing an MBR / FDISK partition creation.** This Amendment covers *reading* an existing partition table to locate a volume. Creating partitions (FDISK) and writing an MBR are deferred to the disk-utility epic (initech-8479) and are not ruled on here.
- **Extended / logical partitions (the 0x05 / 0x0F extended-partition chain).** Only the four primary MBR partition entries are in scope. An extended-partition chain walk is deferred (Consequence C-6).
- **GPT.** Anachronistic for a 3.30 personality (DEC-12); never in scope.
- **The DOS 4.0+ >32 MB absolute-I/O packet form** -- already out of scope per DEC-15.5; unchanged here.

### 1.4 Additional Defined Terms

The following terms supplement ADR-0003 1.4 and the DEC-04a / DEC-15 term tables. The first two rows are **load-bearing** and exist to prevent the highest-risk reading errors of this Amendment.

| Term | Definition |
|---|---|
| **base_lba** (volume origin) | The device-relative LBA of a volume's boot sector -- i.e. the LBA at which the volume begins on the backing block device. For a raw floppy, `base_lba == 0`. For a partitioned fixed disk, `base_lba ==` the partition's start-LBA. A volume-relative (absolute, INT 25h/26h) sector `s` maps to device LBA `base_lba + s`. |
| **hidden_sectors** | The BPB field at offset 0x1C (`spec/dos_structs.h:210`, `bpb_t.hidden_sectors`). For a partitioned volume, DOS writes here the count of sectors PRECEDING the volume on the device -- i.e. the partition start-LBA. For a non-partitioned floppy it is 0. It is the BPB's OWN record of its `base_lba`. |
| **MBR partition entry** | One of the four 16-byte primary entries in the Master Boot Record partition table at device-LBA-0, offsets 0x1BE / 0x1CE / 0x1DE / 0x1EE. The start-LBA is the 4-byte little-endian field at entry offset 0x08; the partition type is the 1-byte field at offset 0x04. (Microsoft / IBM MBR layout; operator/implementer must confirm exact offsets against the Microsoft FAT/MBR specification or an 86Box round-trip -- see 8.) |
| **Boot signature 0x55AA** | The 2-byte little-endian `0x55 0xAA` at offset 0x1FE (510) of a 512-byte sector. Present on BOTH an MBR and a FAT boot sector, so its presence alone does NOT distinguish them -- the disambiguator is what lies before it (a valid partition table vs a valid BPB). NOTE the byte-order tell: stored bytes are `0x55,0xAA`; read as a little-endian `uint16` the value is `0xAA55` (`spec/dos_structs.h:78`, `FAT12_BOOTSIG_VALUE 0xAA55`). |

---

## 2. Context

### 2.1 What Is Missing and Why Now

DEC-07 ratified FAT12 (diskette) and FAT16 (fixed disk). The current release mounts a 1.44 MB FAT12 floppy, whose volume begins at device LBA 0 -- so `fat12_mount` reads sector 0 from the device and finds the BPB there (`os/milton/kmain.c:736`, `fat12_mount(&vol, &fatdev, sector_buf)`). The moment a FAT16 **fixed disk** enters the picture (forward tranche bead initech-z01, gated behind the streaming walk dao; WL-0026 "Pointers / next"), sector 0 of the device is the **MBR**, not the BPB: the boot sector lives at the partition's start-LBA, and every subsequent FAT/root/data LBA must be biased by that origin. Mounting such a disk by blindly reading device-LBA-0 as a BPB would parse the MBR's partition table as nonsense BPB geometry and fail (or worse, mount garbage).

What is missing is the *contract*: which authority owns the start-LBA, where the offset is applied, and how the kernel knows a device is partitioned at all. The forward-grounding brief (WL-0026) surfaced exactly three forks, reproduced and ruled on in 3.

### 2.2 Why This Is a DEC-07 Refinement, Not a New DEC

DEC-07 is the file-system decision and is classified **load-bearing**. The volume-origin / partition contract is intrinsically part of "where the FAT file system lives on the medium" -- it is the missing on-disk-layout clause of DEC-07, not a new subsystem. Per the DEC-04a/DEC-14/DEC-15 house pattern, a refinement that adds a contract to an existing load-bearing Sub-Decision is recorded as a lettered amendment (`DEC-07a`) rather than a new numbered DEC. It touches no INT 21h AH function and adds no interrupt vector, so ADR-0003 Appendix A and `spec/int21h_register.json` are **unchanged** (Neutral Consequences, 5.3).

### 2.3 Relationship to the North Star

Per epic initech-bsy, neither Turbo Initech (tps) nor InitechBase (samir) reads a partition table directly; they reach files through handle-level INT 21h over an already-mounted volume. DEC-07a's value therefore maps under **DR-1 (Fidelity** -- a period DOS fixed disk is partitioned and DOS honours `hidden_sectors`), **DR-3 (Period Plausibility)**, and **DR-4 (Build Tractability** -- it unblocks the FAT16 fixed-disk read path initech-z01 and the disk-utility epic initech-8479). DR-5 (continuity with the North Star) is satisfied **negatively**, as in DEC-15: it does not preclude the North Star and does not advance it directly.

---

## 3. The Decision (DEC-07a) -- the Three Forks Ruled

It is hereby **recommended** (DRAFT; pending operator ratification) that the following Sub-Decisions constitute Amendment DEC-07a to ADR-0003. ADR-0003 Appendix A is unchanged.

### 3.1 Sub-Decision DEC-07a.1 -- FORK 1: Start-LBA Authority on MBR/BPB Conflict

**The fork.** When a device is partitioned, the volume start-LBA is available from TWO independent on-disk sources that, on a well-formed disk, agree: (a) the MBR primary partition entry's start-LBA field (entry offset 0x08), and (b) the volume's own BPB `hidden_sectors` field (offset 0x1C). The fork is what to do when they **conflict**:

- **(i) partition-table-authoritative** -- trust the MBR entry's start-LBA; treat `hidden_sectors` as advisory.
- **(ii) hidden_sectors-authoritative** -- trust the BPB's own `hidden_sectors`; treat the MBR entry as advisory.
- **(iii) require-match-fail-loud** -- require the two to be equal; on mismatch, refuse to mount with a distinct error (Rule 2).

**Deliberation (four perspectives).**

- *Period-authenticity (real DOS 3.3).* This is the load-bearing uncertainty of this Amendment and the one the committee **cannot resolve from local sources alone**. Period DOS reaches a partition through the BIOS (INT 13h) using the MBR partition entry's CHS/LBA to find the boot sector, then reads the BPB; `hidden_sectors` is then the volume's self-description. What real DOS 3.3 does when the two DISAGREE -- whether it silently prefers one, recomputes, or misbehaves -- is **not derivable from the PRD, the locked spec-data, or any local golden**, and must NOT be fabricated (Law 1). It requires an 86Box round-trip on a deliberately-mismatched image (see 8.1).
- *Robustness / fail-loud (Rule 2).* A conflict between two on-disk authorities is precisely the "invariant violation" class Rule 2 exists to catch: it means the disk was authored by two disagreeing tools, or one field is corrupt. Silently picking either source risks mounting the volume at the WRONG origin and then reading/writing the FAT, root, and data at biased-but-wrong LBAs -- a quietly-corrupt mount, the worst Law-2 outcome. Fail-loud (iii) is the safe default for a single-volume release that has no legitimate reason to see a mismatch.
- *North-star minimalism.* The current release mints its OWN test images; a well-formed minted image will have the two fields equal, so (iii) costs nothing on the happy path and adds one equality check. (i) and (ii) each silently absorb a class of malformed disk we have no requirement to support yet.
- *Forward-compat.* If a real-world or 86Box-authored disk legitimately carries a benign mismatch (some partitioning tools historically zeroed `hidden_sectors` on the volume even when partitioned), (iii) would over-reject it. That is exactly the case the 86Box round-trip must settle before the ruling is locked.

**CHAIR RECOMMENDATION (provisional, pending 86Box evidence): (iii) require-match-fail-loud as the ratified default, with a documented fallback rule pending the round-trip.** Rationale: for a single-volume, self-minted-image release the equality invariant is free, period-plausible (a correctly-authored DOS disk has the two equal), and fails loud on the corruption/disagreement case Rule 2 targets. The new `FAT12_ERR_PARTITION` code (3.5) carries the mismatch. **HOWEVER**, this ruling is **conditional on an 86Box round-trip** (8.1): if real DOS 3.3 demonstrably mounts a mismatched image by preferring one source, the Chair recommendation flips to that source (most likely **(i) partition-table-authoritative**, since DOS physically reaches the boot sector via the MBR entry, making the MBR entry the operative origin and `hidden_sectors` the volume's possibly-stale self-report). The implementer must NOT lock Fork 1 until that evidence is in hand; until then `FAT12_ERR_PARTITION` on mismatch is the conservative placeholder.

### 3.2 Sub-Decision DEC-07a.2 -- FORK 2: Application Layer for the LBA Offset

**The fork.** Where is `base_lba` ADDED to volume-relative sector numbers?

- **(a) mount-layer** -- the caller (kmain) owns partition discovery, computes `base_lba`, and the volume carries it; `fat12_*` stays a pure FAT reader operating in device-LBA space by having every internal LBA biased by `base_lba` at the seam, OR the caller installs a biasing block-device adapter. The partition logic lives OUTSIDE `fat12.c`.
- **(b) inside blockdev_t.read_sectors** -- the block-device implementation itself adds the offset, so every consumer transparently sees a partition-relative device.

**Deliberation (four perspectives).**

- *Robustness / Law 3 (separation of concerns).* `fat12.c` today is a pure FAT reader: `fat12_mount(vol, dev, buf)` reads sector 0 from `dev` and the caller (`kmain.c:736`) owns the block device. Partition discovery is a property of the *medium*, not of the *file system* -- a FAT volume does not know or care that it sits in a partition. Putting the offset inside `blockdev_t.read_sectors` would couple the generic block-device abstraction to a DOS-partitioning concept and would mean the absolute-disk seam (DEC-15), the FAT reader, and any future raw consumer all silently see a shifted device -- making it impossible to address device-LBA-0 (the MBR itself) through the same seam, which the partition PARSER needs.
- *Period-authenticity.* DOS keeps `hidden_sectors` in the BPB (a per-volume datum) and the BIOS reaches the partition via the MBR -- the OS layer that knows about partitions is the mount/volume layer, not the raw disk. Mount-layer mirrors the heritage decomposition.
- *North-star minimalism.* Mount-layer adds one field to `fat12_volume_t` and biases at one well-defined seam. Blockdev-layer would need a wrapper context per volume and complicates the single existing backend.
- *Forward-compat.* With `base_lba` on the VOLUME (mount-layer), initech-slvd mounts N volumes each with its own `base_lba` over the SAME device with zero changes to the block-device layer. Blockdev-layer would need N biasing adapters.

**CHAIR RECOMMENDATION: (a) mount-layer.** This is the brief's recommendation and it is correct on Law 3 grounds. The partition parser runs in the caller (kmain) reading device-LBA-0 directly; it computes `base_lba`; the mounted volume carries `base_lba` (DEC-07a.4); and `fat12_*` adds `base_lba` to every device read/write at its block-device seam (the FAT reader stays partition-agnostic -- it just gains an origin). The generic `blockdev_t` stays a pure device-LBA interface so the partition parser and the DEC-15 absolute-disk seam can both still address LBA 0. **Justification of record:** keeps `fat12.c` a pure FAT reader (Law 3), keeps `blockdev_t` a pure device abstraction, and puts the partition knowledge where the medium is owned -- the mount layer.

### 3.3 Sub-Decision DEC-07a.3 -- FORK 3: Volume-Detection Heuristic

**The fork.** How does the kernel decide whether device-LBA-0 is an MBR (partitioned) or a FAT boot sector (raw floppy)?

- **(b1) BIOS-drive-number + BPB hint** -- use the BIOS drive number (DL >= 0x80 => fixed disk => partitioned; DL < 0x80 => floppy => raw) plus the BPB `drive_number` / media-descriptor hint.
- **(b2) sector-0 byte-sniff** -- read device-LBA-0 and classify by content: a valid MBR (0x55AA signature AND at least one plausible partition entry: nonzero type, in-range start-LBA) vs a valid BPB (0x55AA AND plausible BPB geometry: `bytes_per_sector == 512`, sane fats/reserved/root counts).

**Deliberation (four perspectives).**

- *Period-authenticity.* Real DOS keys off the BIOS drive number: 0x00-0x7F are floppies (no partition table), 0x80+ are fixed disks (partitioned). That is the canonical, period-correct first discriminator. (Operator/implementer must confirm the DL>=0x80 fixed-disk convention against RBIL INT 13h / the DOS 3.3 PRM -- it is well-established but cite it, Law 1.)
- *Robustness / fail-loud.* The drive number is the INTENT; the byte content is the GROUND TRUTH. A pure-BIOS-number scheme would misread a "superfloppy" (a fixed-disk-numbered device with a BPB at LBA 0 and no partition table) or a partition image fed in as a floppy. The byte-sniff catches the mismatch. Belt-and-suspenders: use the drive number as the primary signal, then VALIDATE against sector-0 content and fail loud (`FAT12_ERR_PARTITION`) on a contradiction (e.g. DL>=0x80 but LBA 0 is a valid BPB with no partition table, or DL<0x80 but LBA 0 sniffs as an MBR).
- *North-star minimalism.* For THIS release the device is a floppy (DL would be 0x00); the simplest correct behaviour is "floppy => raw, base_lba=0", which both schemes agree on. The byte-sniff fallback is the small forward investment that makes the FAT16-fixed-disk path (z01) work without re-deciding.
- *Forward-compat.* BIOS-first cleanly scales: each future device announces its drive number; byte-sniff disambiguates the edge cases (superfloppy, partition-image-as-floppy) that a fixed-disk world will encounter.

**CHAIR RECOMMENDATION: (b1)+(b2) BIOS-first, byte-sniff-fallback (and cross-validate).** This is the brief's recommendation. Use the BIOS drive number as the PRIMARY discriminator (DL >= 0x80 => attempt MBR parse at LBA 0; DL < 0x80 => treat as raw single volume, `base_lba = 0`), then CROSS-VALIDATE against the sector-0 byte-sniff and fail loud on a contradiction the heuristic cannot reconcile. **Justification of record:** the BIOS drive number is the period-correct, cheap primary signal; the byte-sniff is the Rule-2 ground-truth check that prevents a silently-wrong mount when intent and content disagree. The classification rule (0x55AA presence is necessary-but-not-sufficient; the disambiguator is "valid partition entry" vs "valid BPB geometry") is locked in the new spec file.

### 3.4 Sub-Decision DEC-07a.4 -- Per-Volume base_lba (the Forward Obligation)

Every mounted volume shall carry its own **`base_lba`** (device-relative origin), proposed as a new `uint32_t base_lba` field on `fat12_volume_t` (`os/milton/fat12.h:83`), set by `fat12_mount` (or by the caller passing it in -- see 9). For the single-volume floppy release `base_lba == 0` and behaviour is byte-identical to today. Every FAT/root/data device access biases its LBA by `base_lba` (DEC-07a.2). Because the origin is per-VOLUME data, initech-slvd (multi-volume) mounts N volumes -- each with its own `base_lba` over the same or different devices -- **without a refactor** of the FAT reader or the block-device layer. This is the load-bearing forward-compatibility property of the whole Amendment: the offset is volume state, not a global.

### 3.5 Sub-Decision DEC-07a.5 -- Locked Spec-Data Home + New Error Code

**(a) New locked spec file `spec/mbr_partition_contract.json`** (CLAUDE.md Rule 8), a NEW sibling spec (mirroring how DEC-15 created `spec/absdisk_int2526.json` rather than wedging into an INT-21h file). It shall record: the MBR partition-entry layout (the four entry offsets, the type byte at 0x04, the start-LBA field at 0x08), the boot-signature `0x55AA` at offset 510 (necessary-not-sufficient), the Fork-1 conflict-resolution rule (PROVISIONAL pending the 86Box round-trip; CF/fail-loud is the locked placeholder, the chosen authority is the erratum-corrected value), the Fork-2 mount-layer offset application, the Fork-3 BIOS-first + byte-sniff-fallback classification with the "valid partition entry vs valid BPB geometry" disambiguator, and the `base_lba` per-volume semantics. It shall carry its OWN consistency gate under `make test-spec` and be EXPLICITLY EXCLUDED from the INT-21h AH-consistency walk (it is not an AH function), exactly as `spec/absdisk_int2526.json` is.

**(b) New error code `FAT12_ERR_PARTITION`** in the FAT12 error enum (`os/milton/fat12.h:30-45`, currently ending at `FAT12_ERR_ACCESS = -15`), proposed as **`FAT12_ERR_PARTITION = -16`** (no gap; next sequential value). Returned by the partition-aware mount path when: the device is detected as partitioned but no valid primary FAT partition is found; the MBR start-LBA and BPB `hidden_sectors` conflict (Fork 1, until the 86Box golden settles the authority); or the detection heuristic hits an irreconcilable intent/content contradiction (Fork 3). This is a SEPARATE code space from the DEC-15 absolute-disk AL/AH hardware bytes and from the INT 21h `INT21_ERR_*` enum; none of the three may be conflated.

---

## 4. Rationale (Cross-Reference)

- **Fork 1 (4.1).** Covered on the record in 3.1. The decisive factor is Law 1: the conflict-resolution behaviour of real DOS 3.3 is NOT a local fact and must be settled by an 86Box round-trip before locking; `require-match-fail-loud` is the conservative ratifiable default, with the authority flip pre-authorized as an erratum if the round-trip shows DOS prefers a source.
- **Fork 2 (4.2).** Covered in 3.2. Mount-layer keeps `fat12.c` a pure FAT reader and `blockdev_t` a pure device abstraction (Law 3), and makes `base_lba` per-volume state so slvd scales.
- **Fork 3 (4.3).** Covered in 3.3. BIOS drive number is the period-correct primary signal; byte-sniff is the Rule-2 ground-truth cross-check.
- **base_lba (4.4).** Covered in 3.4. Volume-state, not global, is the property that makes multi-volume a non-refactor.
- **Period authenticity (4.5).** `hidden_sectors` (BPB 0x1C), the MBR partition table at LBA 0, the `0x55AA` signature, and the DL>=0x80 fixed-disk convention all match the Microsoft FAT/MBR specifications and the DOS 3.3 PRM. Extended partitions and GPT are excluded as out-of-scope / anachronistic (1.3).

---

## 5. Consequences

### 5.1 Binding Constraints (on ratification)

**C-1 -- Mount becomes partition-aware (mount-layer offset).** The mount path gains a partition-discovery step (in the caller, kmain) that classifies device-LBA-0 (Fork 3), computes `base_lba` (Fork 1), and the volume carries it (DEC-07a.4). `fat12.c` stays a pure FAT reader -- it gains an origin, not partition logic (Law 3).

**C-2 -- base_lba on the volume.** A new `uint32_t base_lba` on `fat12_volume_t`; `base_lba == 0` for the floppy release; every device LBA biased by it.

**C-3 -- New locked spec-data + error code.** `spec/mbr_partition_contract.json` (own gate, excluded from the AH walk) and `FAT12_ERR_PARTITION = -16`. Any change to the partition-entry layout, the Fork-1 authority, the Fork-3 classification, or the `base_lba` semantics requires a further amendment and a green spec gate + mutation oracle.

**C-4 -- Floppy behaviour byte-identical.** On the single-volume floppy (`base_lba == 0`, DL < 0x80, no MBR) the mount, FAT, root, and data LBAs are unchanged; no existing FAT/absdisk oracle regresses. This is an explicit non-regression constraint (Stop-Condition: never weaken an existing oracle).

### 5.2 Forward Obligations

**C-5 -- Multi-volume (initech-slvd) inherits base_lba.** The per-volume `base_lba` (DEC-07a.4) is the mechanism by which slvd mounts N partition-relative volumes without refactoring the FAT reader or block-device layer. slvd adds the drive-letter -> volume table; DEC-07a provides the origin per volume.

**C-6 -- Extended/logical partitions deferred.** Only the four primary MBR entries are parsed. An extended-partition chain walk (type 0x05/0x0F) is a future amendment if a multi-partition fixed disk needs it.

**C-7 -- FAT16 fixed-disk read path (initech-z01) is unblocked but separate.** DEC-07a provides the volume origin a FAT16 fixed disk needs; z01 implements the FAT16 read itself. The two land separately (z01 DEPENDS-ON kzfs).

**C-8 -- Fork 1 authority is provisional pending 86Box.** Until the 86Box round-trip (8.1) settles real-DOS-3.3 behaviour on a mismatched image, `require-match-fail-loud` (`FAT12_ERR_PARTITION`) is the locked behaviour; the chosen authority on a confirmed mismatch is an editorial erratum to `spec/mbr_partition_contract.json`, not a further full amendment (mirroring the DEC-15 provisional-AH pattern).

### 5.3 Neutral Consequences / DEC-15 Interaction

- ADR-0003 Appendix A (the INT 21h AH-function register) is **unchanged**; DEC-07a adds no AH function and no interrupt vector.
- **DEC-15 INT 25h/26h `AL=drive` interaction (load-bearing note).** DEC-15.2 fixed INT 25h/26h `AL` as a zero-based explicit drive (0=A:), and the absolute (logical) sector as **volume/partition-relative** ("boot sector = sector 0 = blockdev LBA 0", DEC-15 1.4; the seam's `total_sectors` is the volume's total-logical-sectors, `kmain.c:783`). DEC-07a makes that statement precise on a partitioned disk: INT 25h/26h absolute sector `s` is VOLUME-relative and resolves to **device LBA `base_lba + s`**. On the floppy (`base_lba == 0`) this is exactly today's behaviour, so DEC-15 and its `test-absdisk` oracle are **unaffected** in the current release. The forward obligation: when the absolute-disk seam is bound for a PARTITIONED volume (post-z01/slvd), it must be bound from that volume's `base_lba`+geometry (the bind site `kmain.c:781-791`), so `AL=0` -> the mounted partition's origin, and an absolute sector past the partition's `total_sectors` still fails loud (DEC-15 sector-not-found). DEC-15's `AL != mounted volume -> invalid drive` becomes, under slvd, `AL` indexing the drive table -- but that is slvd's obligation, recorded here for traceability, not changed by this Amendment.
- `spec/dos_structs.h` `bpb_t.hidden_sectors` (0x1C) is unchanged; DEC-07a gives it its operative meaning (the BPB's self-recorded `base_lba`). No struct edit is proposed to `dos_structs.h`; the `base_lba` field is on `fat12_volume_t` (runtime state), not the on-disk BPB.
- No change to the procurement of foam packaging inserts.

---

## 6. Scope-Clause Delta (the Governance Act)

ADR-0003 Appendix A's closed-scope clause governs the INT 21h AH-function register and is **unchanged** by this Amendment -- DEC-07a adds no AH function and no interrupt vector. DEC-07a refines DEC-07 (5.7, load-bearing) by making explicit the on-disk volume-origin / partition contract that DEC-07 left implicit. It admits no new vector (unlike DEC-15) and no new AH function (unlike a DEC-04 register change); it adds a runtime volume field (`base_lba`), a locked spec file (`spec/mbr_partition_contract.json`), and one FAT error code (`FAT12_ERR_PARTITION`). The governance act is therefore the lettered refinement of a load-bearing Sub-Decision, recorded here and locked (on ratification) in the new spec file.

---

## 7. Verification (proposed; lands with initech-kzfs, NOT with this draft)

### 7.1 The Partitioned-FAT12 Fixture

A new gitignored, deterministically-minted build fixture: a disk image with a valid MBR at device-LBA-0 carrying one primary FAT12 partition whose start-LBA = `base_lba` (a fixed nonzero value, e.g. 63 -- the classic CHS-aligned first-partition LBA; confirm against the Microsoft/IBM convention), with a well-formed FAT12 volume (BPB, two FATs, root, data) beginning at `base_lba`, and `hidden_sectors == base_lba` in that BPB (the well-formed, Fork-1-agreeing case). Minted by a host tool (the `fat_diff` family) with NO wall-clock / no rand (Rule 11). The existing raw 1.44 MB floppy fixture (`base_lba == 0`) is retained as the non-regression case (C-4).

### 7.2 The Differential / Round-Trip Oracle (test-kzfs)

A host oracle (NO QEMU) that, against the partitioned fixture: (1) classifies LBA 0 as an MBR (Fork 3), (2) parses the partition entry and computes `base_lba` (Fork 1), (3) mounts the FAT12 volume at `base_lba`, (4) lists the root directory and reads a known file, cross-checking every device LBA touched equals `base_lba + volume_relative_lba` against an independent `mtools`/python reference reading the same image with the same partition offset. The raw-floppy fixture (`base_lba == 0`) must still pass byte-identically (C-4). Fork-1 mismatch and Fork-3 contradiction cases assert `FAT12_ERR_PARTITION`.

### 7.3 Mutation Plan (each one-branch perturbation must turn the oracle RED -- Rule 6)

| Mutant | Perturbation | RED signal |
|---|---|---|
| M-hs-off | drop the `base_lba` offset (read volume-relative LBA as device LBA -- the off-by-`base_lba` bug) | the device-LBA cross-check [7.2(4)] AND the file read (boot sector parses as MBR garbage on the partitioned image) |
| M-mbr-ignore | ignore the MBR; read device-LBA-0 as a BPB on the partitioned fixture | mount fails / parses MBR-as-BPB -> wrong geometry; the mount-success assertion goes RED |
| M-detect-wrong | force Fork-3 the wrong way (treat the partitioned disk as raw, or the floppy as partitioned) | partitioned: reads MBR as BPB (RED); floppy: looks for a nonexistent MBR -> `FAT12_ERR_PARTITION` where success was expected |
| M-conflict | author the fixture with `hidden_sectors != ` MBR start-LBA, OR drop the Fork-1 conflict guard | the Fork-1 case must return `FAT12_ERR_PARTITION` (placeholder ruling); dropping the guard mounts at the wrong origin and the cross-check goes RED |

### 7.4 Locked Spec Authority

`spec/mbr_partition_contract.json` is the locked authoritative contract for the partition layout, the Fork-1 authority (provisional per C-8), the Fork-3 classification, and the `base_lba` semantics (Rule 8). Its consistency gate runs under `make test-spec`, separate from and additional to the INT-21h AH-consistency walk, and lands with the implementation (beads initech-kzfs); the file is inert to the existing AH-walk steps. No implementation may deviate from it without a further amendment (or, for the Fork-1 authority only, the C-8 erratum once the 86Box golden is in hand).

---

## 8. Evidence Still Required Before Locking

### 8.1 86Box Round-Trip on a Mismatched Image (gates Fork 1)

The Fork-1 ruling (3.1) is **provisional** and MUST be grounded before its authority is locked, per Law 1 (the conflict behaviour of real DOS 3.3 is not a local fact). The required experiment:

1. Mint two partitioned FAT12 images that are byte-identical EXCEPT: image A has `hidden_sectors == ` MBR start-LBA (well-formed); image B has `hidden_sectors != ` MBR start-LBA (e.g. `hidden_sectors == 0` while the MBR entry start-LBA is 63, the historically-common partitioner discrepancy).
2. Boot real DOS 3.3 under 86Box with each image as a fixed disk.
3. Observe, on image B: does DOS mount the volume at all; if so, at the MBR start-LBA (=> partition-table-authoritative, Fork-1 (i)) or at the `hidden_sectors` origin (=> hidden_sectors-authoritative, Fork-1 (ii)); or does it refuse / corrupt (=> fail-loud (iii) is faithful)?
4. Record the result as the golden that selects the locked Fork-1 authority (erratum to `spec/mbr_partition_contract.json` per C-8). Until then, ship `require-match-fail-loud` (`FAT12_ERR_PARTITION`).

**Operator/implementer must confirm** the MBR partition-entry field offsets (type at 0x04, start-LBA at 0x08, the four entries at 0x1BE/0x1CE/0x1DE/0x1EE) and the DL >= 0x80 fixed-disk convention against the Microsoft FAT/MBR specification and RBIL INT 13h respectively, and the first-partition CHS-aligned start-LBA (commonly 63) against the period partitioning convention -- these are stated from well-established memory but are NOT in the local sources and must be cited from a real reference (Law 1) before the spec file is locked.

---

## 9. Implementation Disposition (ratify the contract; implement under kzfs)

On ratification, the contract is fixed; the kernel changes (partition parse in kmain; `base_lba` on `fat12_volume_t`; the biased FAT seam; the new error code; `spec/mbr_partition_contract.json`; the biting `test-kzfs` oracle) land under beads **initech-kzfs** and are NOT part of this docs+spec draft. An open implementation sub-fork to resolve at kzfs time (not a governance fork): whether `base_lba` is computed by `fat12_mount` itself (mount reads LBA 0, sniffs, parses) or by the caller (kmain) which passes `base_lba` into a `fat12_mount_at(vol, dev, base_lba, buf)` variant. The mount-layer ruling (Fork 2) is satisfied either way; the Chair leans to the **caller-passes-base_lba** form to keep `fat12_mount` itself partition-agnostic (purest Law 3), but this is an implementer's call recorded for kzfs, not a ratification gate.

---

## 10. Related Decisions and References

- ADR-0003 (OEA-ADR-0003) 5.7 (DEC-07 FAT12/16 file system, load-bearing -- the Sub-Decision this Amendment refines), 5.8 (DEC-08 flat executables), 5.12 (DEC-12 3.30 personality), Appendix A closed-scope clause, Appendix B.1 (directory entries).
- ADR-0003 Amendment DEC-04a (OEA-ADR-0003-A1) -- flat ABI, vector map.
- ADR-0003 Amendment DEC-14 (OEA-ADR-0003-A2) -- document-and-omit / provisional-value precedent.
- ADR-0003 Amendment DEC-15 (OEA-ADR-0003-A3) -- INT 25h/26h absolute disk; the `AL=drive` / volume-relative-sector interaction (5.3); the new-sibling-spec-file + provisional-value pattern this draft mirrors.
- `spec/dos_structs.h:210` -- `bpb_t.hidden_sectors` (0x1C); `:78` `FAT12_BOOTSIG_VALUE 0xAA55`; geometry macros `:226-233`.
- `os/milton/fat12.h:30-45` -- `FAT12_ERR_*` enum (ends at `FAT12_ERR_ACCESS = -15`; `FAT12_ERR_PARTITION = -16` proposed); `:83-94` `fat12_volume_t` (`base_lba` field proposed); `:125` `fat12_mount`.
- `os/milton/blockdev.h:43-51` -- `blockdev_t.read_sectors`/`write_sectors` (the pure device-LBA seam that stays partition-agnostic, Fork 2).
- `os/milton/kmain.c:736` -- `fat12_mount(&vol, &fatdev, sector_buf)` mount site (partition discovery would precede this); `:781-791` -- DEC-15 absolute-disk seam bind site (the `base_lba` forward-obligation site).
- `spec/mbr_partition_contract.json` -- the locked partition contract proposed herein (DEC-07a.5; created on ratification under kzfs, NOT by this draft).
- Microsoft FAT specification (BPB `hidden_sectors`, FAT12/16 classification); Microsoft/IBM MBR partition-table layout; DOS 3.3 Programmer's Reference Manual; Ralf Brown's Interrupt List (RBIL) INT 13h (BIOS drive number, fixed-disk DL>=0x80) -- all to be cited from a real reference or an 86Box round-trip before the spec file is locked (Law 1; see 8.1).
- beads initech-t1on (this decision, tracking), initech-kzfs (gated implementation), initech-slvd (forward multi-volume), initech-bsy (epic).

---

*-- End of DRAFT Record (PENDING OPERATOR RATIFICATION) --*

<!--
This document is a DRAFT. It is the confidential and proprietary information of
Initech Systems Corporation. Unauthorized review, use, disclosure, or distribution
is prohibited. No locked spec-data was edited in producing this draft; the spec
edits and implementation are deferred to the implementing bead (initech-kzfs)
post-ratification. If you have received this document in error, please shred it and
notify the Help Desk (ext. 2504). Tedium certified compliant with NFR-7.
-->
