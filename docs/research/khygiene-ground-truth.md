# MILTON hygiene source record

Read locally before changing behavior: ADR-0003 Appendix A (39h, 43h, 56h,
4Bh, 31h), DEC-07/08, Appendix B.1, DEC-08a.2 (flat MZ entry), DEC-17.

IBM *DOS 3.30 Technical Reference*, April 1987, local
`../initech-manuals-ref/dos33/80X0945_DOS_3.30_Technical_Reference_Apr87.pdf`:
rendered at 110 dpi and viewed PDF pages 228, 268, 318, 329, corresponding
to printed pages 6-105 (KEEP), 6-145 (CHMOD), 6-195 (EXEC errors), 6-206
(RENAME). These establish the DOS interfaces; the single flat program region
and restricted CHMOD policy are the local ADR/kernel contracts.

The independent FAT oracle starts artifact and mtools from copies of one
mformatted image, runs mren/mmd/mattrib/mcopy, compares recursive names,
attributes, child bytes and dotdot, then checks both images with fsck.fat.
The fragmented fixture places GROW at the final data cluster before deleting
its large predecessor file. No free cluster can follow that directory.

ATA source acquired through the browser before protocol changes:
[T13/1321D revision 3, ATA/ATAPI-5 working draft](https://www.seagate.com/support/disc/manuals/ata/d1153r17.pdf)
(the URL basename differs from the document's title). Sections 7.3/7.4,
7.13/7.14 (printed pp. 58-59, 65-66) require idle taskfile writes and
forbid using other status bits while busy. Section 8.27.7 requires DRDY
before READ SECTORS; section 9.6 describes PIO data-out completion.
The implementation waits for BSY/DRQ clear and DRDY set before commands,
and checks completion before FLUSH. This is also the protocol gap explicitly
identified by bcg.12. Scripted ports exercise negative paths on the real TU;
QEMU tests exercise the real port instructions. The host shell could not
download the PDF (DNS restriction); this local digest records the acquired
primary source, following the existing block-device-ground-truth pattern.

Rule 11 is the source for deterministic host mappings and both kernel repro
comparisons. Fixed mappings use NOREPLACE, never destructive MAP_FIXED.
