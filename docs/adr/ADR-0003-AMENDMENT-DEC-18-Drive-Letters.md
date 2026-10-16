# ADR-0003 Amendment DEC-18: Drive letters over the physical block-device service

Status: DECIDED by the PM, second FORMAT/SYS lane launch; initech-slvd,
first half. The second half (general mounted-volume switching and path
resolution) remains outside this lane. Appendix A is unchanged.

A is BIOS 81h (primary slave), its raw FAT volume or primary partition 1.
B is unassigned. C is BIOS 80h partition 1. D is BIOS 80h partition 2,
when present. Partition identities never depend on active flags or discovery
order. C/D require a bounded primary FAT type 01/04/06 entry and 55AA MBR;
extended and malformed entries fail closed. A's diskette role is a logical
role under the PM's ATA-only hardware ruling, not an invented floppy driver.

`spec/drive_letters.json` is the locked ABI; generated
`spec/drive_letters.h` has a byte-equality gate. DEC-17 gains AH60h:
AL is zero-based DOS letter, EBX a flat pointer to five little-endian
uint32 fields: BIOS disk, primary partition number (0=raw), base LBA,
sector count, flags. Flag 1 identifies the mounted current volume by
(disk, base), flag 2 the A diskette role. CF/AH retain DEC-17 status
semantics. B/missing/invalid volumes return CF and AH01. This one kernel
resolver is used by all four utilities; programs never interpret an MBR.
The existing mounted A drive and A prompt remain unchanged.

FORMAT refuses the running identity with the catalogue's `Format not
supported on drive x:` (MS-DOS UG p333). This refusal is the explicit PM
policy; the manual's catalogue supplies the text, not a claim that DOS
3.3 prohibited all formatting of its current floppy. C/D destruction
requires the printed p75 warning and Y/RETURN, with current volume-label
verification when a valid formatted target has a label. Tests use scratch
D on a copied boot disk, or an A medium copied to --disk2. Shipped images
are never destructive targets.

FORMAT emits SPC=1 only. Cluster counts below 4085 select FAT12, below
65525 select FAT16; larger sizes fail with `Format requires more than one
sector per cluster`. This explicit InitechOS limitation preserves
initech-spc and does not pretend to be a DOS message. IBM TR pp5-4..8,
2-31 describes layout/BPB; mformat and fsck.fat independently judge the
cluster-count selection. The boot-code template is the C volume's VBR,
the same mformat sector tools/mk_boot_system.sh mints. Existing MBR/stage2
prefix sectors are retained. A bootable output requires that prefix;
FORMAT/SYS do not install an MBR or stage2 on a bare physical device.

SYS transfers IO.SYS and INITDOS.SYS, root slots 0/1, attributes 07h,
and the boot code, preserving destination BPB and all unrelated files.
It never copies COMMAND.COM (MS-DOS UG p119). Occupied first slots,
insufficient free clusters, or absent system source fail before writing.
Existing correctly named system entries may be updated without requiring
contiguous source or destination chains. FORMAT /S additionally copies
COMMAND.COM. Both use C as the shipped system source per the PM ruling.
FORMAT /B reserves the first two HSR entries and enough clusters for the
current system files, with zero content; a subsequent SYS fills those
reservations. Its directory/chain representation is the implementation's
deterministic choice for UG p74's space reservation, independently read
back by mtools. It does not claim a captured DOS /B byte-for-byte golden.
The /1, /4, /8, /T, /N geometry choices are checked against mformat with
SPC=1; the media code follows IBM TR p5-6 even when mformat calls a
nonstandard SPC=1 floppy F0. In-place FORMAT C: /S requires the shipped
contiguous source chains and unchanged data origin; a different layout
fails before writing with the manual's system-transfer memory message.

DISKCOPY/DISKCOMP accept only the A diskette role; B is invalid, and C/D
are rejected as fixed disks (UG pp59-63). A-to-A operations alternate
SOURCE/TARGET or FIRST/SECOND prompts and buffer a bounded group of sectors
between swaps. Geometry for report tracks/sides comes from the source BPB,
while physical I/O always uses DEC-17's 63x16 translation. The factory's
injected medium-change event attempts QMP replacement of the --disk2 ATA
medium; an unsupported QEMU medium change remains a named RED leg, rather
than a fabricated two-drive success.

Ground truth scans: MS-DOS 3.3 User's Guide printed pp59-63 (PDF144-148),
26 (39), 74-76 (159-161), 119 (204), 318 (403), 332-337 (417-422), 340 (425),
348 (433), 358 (443). IBM DOS 3.30 Technical Reference pp1-7 (PDF24), 2-26/2-31
(53/58), 5-4..11 (111..118). Source citations and gate witnesses are
recorded in the lane worklog. The PM's fixed mapping is an explicit
adaptation, not attributed to IBM's original hardware allocation.
