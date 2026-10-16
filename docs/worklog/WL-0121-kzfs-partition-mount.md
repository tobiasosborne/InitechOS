# initech-kzfs: partition-aware data-volume mount

The one-shot lane adds a fixture, without changing the flagship boot or data
image layout. PRD 6.1/8 and ADR-0003 DEC-07a govern the FAT mount. The lane's
explicit content-sniff rule supersedes DEC-07a's old BIOS-first detection:
the shipped primary-slave ATA data image is itself a superfloppy. DEC-07a
section 9 permits discovery inside fat12_mount. The new locked contract is
spec/mbr_partition_contract.json, independently checked outside the Appendix-A
AH register. Error -20 preserves the existing -16 through -19 allocations;
DEC-07a proposed -16 before those features landed.

Ground truth inspected as rendered scans: IBM DOS 3.30 Technical Reference
80X0945, printed p2-31 (PDF 58: jump, BPB, media/hidden sectors), pp6-24..25
(PDF 147..148: INT25/26), pp9-6..9 (PDF 393..396: four primary entries,
offsets, flags, types, CHS and relative sectors, signatures). Chapter 9 p9-3
(PDF 390) defines a partition as the entire disk from its utilities' view.
The scan documents types 01/04/05; type 06 compatibility is the explicit lane
contract and the installed mtools mpartition type policy, not a DOS 3.30
manual claim. CHS is recorded by mpartition; the ATA path uses the entry's
little-endian relative-sector count. Cylinder 0/head 1/sector 1 on the fixture's
63-sector track yields base LBA 63 (IBM p9-9).

The volume owns base_lba; geometry stays volume-relative. A shared bounded I/O
seam adds the origin once for all FAT/root/data reads and writes and for the
INT25/26 binding used by CHKDSK. The raw ATA/blockdev layer is unchanged. Raw
vector failures retain their CF/AX error path rather than entering the file
path's INT24 prompt. Mount disagreement reports rc, base_lba, hidden_sectors
on serial. Hidden/start equality is DEC-07a's provisional fail-loud policy;
no real-DOS mismatch experiment or 86Box claim is made.

Before implementation, test-fat-partition passed the raw image and failed
with `RED partition MOUNT base=63` on the MBR image; test-fat-partition-emu
failed `RED partition MOUNT` with the partitioned primary-slave device.
The host differential reads identical images with kernel FAT and offset mtools,
then extracts the partition for fsck.fat -n. Both FAT12 and FAT16 are covered.
The injected QEMU gate drives DIR, TYPE, COPY, an INT25/26 boot-sector and
free-sector probe, and CHKDSK /F on a bad FAT chain terminator. It checks copied
bytes with mtools, the repaired partition with dosfstools, and the entire
pre-partition prefix byte-for-byte. Invalid extended, unsigned, and conflicting
hidden-sector images must report the precise contextual mount error.

Rule 6 mutants are FAT_PART_MUT_IGNORE_BASE, FAT_PART_MUT_EXTENDED and
FAT_PART_MUT_SIGNATURE; both host and emulator vectors include their gates.
The contract gate also rejects a table-offset 446-to-445 mutation. All sources
are ASCII. No beads, CLAUDE.md, HANDOFF.md or git metadata are changed by the
lane; the PM owns issue status and commits.
