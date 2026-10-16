# Physical disk evidence (initech-aai2 / initech-ur7k)

Local scans in the DOS 3.3 reference directory were rendered at 110 DPI and
visually inspected: IBM DOS 3.30 Technical Reference PDF pages 393..396
(printed 9-6..9-9), MS-DOS 3.3 User's Guide PDF pages 372,378,380
(printed 287,293,295). The former specifies MBR entries, CHS packing,
active flags, and the first partition starting at head 1, sector 1. The
latter supplies FDISK text, columns, and the default-No deletion prompt.
Create/restart and cylinder-size screens were inspected on printed pp288..289;
active-partition and delete menus on pp291..292; the primary-already-exists
diagnostic on p353. All these pages were rendered at 110 DPI and viewed again
for the second-launch certificate (PDF pages 372..374,376..378,380,438).

## BIOS source correction

The DOS Technical Reference does not contain the BIOS function table.
Its printed p6-25 describes INT25/26, whose error bytes differ. Use IBM's
*PS/2 and PC BIOS Interface Technical Reference*, April 1987, instead:
[scan](https://bitsavers.trailing-edge.com/pdf/ibm/pc/ps2/PS2_and_PC_BIOS_Interface_Technical_Reference_Apr87.pdf),
[searchable reproduction](https://pdfcoffee.com/ps2-and-pc-bios-interface-technical-reference-apr87-pdf-free.html).
The following source digest was acquired from the reproduction before code.
The PDF download was unavailable in this lane (shell DNS restricted; web
fetch timed out); do not claim it was rendered locally.

Printed pp2-58..61 specify reset/read/write using DL fixed-disk numbers,
packed CX cylinders/sectors, DH heads, AL counts, and a transfer pointer.
Errors set CF and return AH. Status values needed here are 00 success,
01 invalid request, 03 write protection, 04 missing sector, 07 parameter
failure, 20 controller failure, and 80 timeout. Printed p2-64 gives AH08
maximum CHS coordinates and attached-drive count; invalid-drive status is
07, with CX/DX zero. Printed p2-67 gives AH15: AH03 denotes a fixed disk,
CX:DX its count of 512-byte sectors; an absent drive gives AH00 and zero
capacity, with carry clear.

## Deliberate Initech decisions

The PM ruling chooses vector 13h, fixed 16/63 translation, two ATA slots,
no FDC unless contracted, and the DEC-04a flat ABI. These are decisions,
not claims about unmodified DOS or the geometry of all period machines.
The hardware contract contains ATA PIO and no floppy controller.
The fixed translation makes the head-1 first-partition rule LBA 63.
INT25/26 retain the DEC-07a mounted-volume origin.

## One-sector-cluster fixture ruling

The second-launch PM ruling preserves `fat12.c`'s sectors-per-cluster=1
mount guard. The local mformat(1) manual documents that -c may be raised
automatically if the FAT would have too many clusters, and that -r sets
the root-directory size in sectors. The FDISK host differential therefore
uses -c 1, checks the actual BPB byte, and uses a 1024-sector root for its
66-cylinder (66465-sector) type06 volume. This keeps fewer than 65525 data
clusters without changing the partition extent. The 513-cylinder packed-CHS
comparison remains full-size; its mount leg deletes/recreates a 66-cylinder
primary in both images before formatting. Kernel bytes must still equal
mcopy, and dosfstools fsck.fat must still report clean on every mount leg.

MS-DOS UG p289's custom-size path is compared against mpartition with -d
explicitly: mpartition otherwise auto-activates its first primary even
when -a was omitted. No active flag is patched out of the byte comparison.
Acceptance and final gate tails: `docs/worklog/WL-0141-physical-block-device-fdisk.md`.
