# ADR-0003 Amendment DEC-17: Physical Block Device Service

Status: DECIDED by the PM for initech-aai2; implementation lane initech-ur7k
consumes this interface. DEC-17 is the next unused numbered decision in
ADR-0003 and its amendments. Appendix A remains the INT21h register.

## Decision and period model

Install a 32-bit trap gate at vector 13h for the BIOS disk-service subset
00h (reset), 02h (read), 03h (write), 08h (parameters), 15h (disk type).
Use the DEC-04a ABI: 32-bit saved registers and flat EBX transfer pointers,
AL count, CX packed cylinder/sector, DH head, DL BIOS disk. IRETD balances
the frame and returns CF and AH status. This is a separate physical service;
INT25h/26h keep their DEC-07a volume-relative semantics. No EDD functions,
real-mode thunk, new INT21h function, or DOS drive-letter reinterpretation.

Vector 13h is decimal 19, previously isr19. Intel exception 19 (#XM) does
not occur on the ADR-0001 486-class target. By explicit PM decision we
repurpose it as period real-mode DOS did; the panic path for a true #XM
on a later SSE machine is forfeited. Other exception vectors are unchanged.

## Locked contract

`spec/block_device_contract.json` is authoritative. Its checked generated
header is `spec/block_device_contract.h`; `test-block-device-spec` compares
fresh generator output byte for byte. It is outside the Appendix-A AH walk.

DL80h is primary master; DL81h is primary slave (the mounted data volume).
`spec/hardware.json` contracts ATA PIO and no FDC. Thus DL00h/01h return
CF=1/AH=01h for every function; no fictitious floppy geometry is supplied.
Unknown functions return the same invalid-command status. AH15 returns
AH00/zero capacity/carry clear for an absent fixed disk, as the BIOS source
specifies. AH08 on an absent mapped drive returns AH07, CX/DX zero, CF set.

Geometry is fixed at 16 heads and 63 sectors per track, not read from the
volume BPB. Cylinders are the IDENTIFY DEVICE LBA28 capacity divided by
1008, clamped to 1..1024. AH08 returns maximum coordinates (cylinders minus
one, head 15, sector 63); AH15 returns the native capacity in CX:DX. A short
final cylinder remains addressable within native capacity through valid
10-bit CHS. Transfers reject zero count, null/overflowing buffers, invalid
CHS, and ranges beyond the native device. The first-partition exception
(cylinder 0, head 1, sector 1) therefore equals LBA63. This is the same
alignment as `spec/mbr_partition_contract.json`; no flagship image layout
or existing FAT mount selection changes.

Reads/writes call the ATA PIO backend at the computed device LBA. They
never call the FAT volume or add its partition base. Error mapping:
invalid argument/request=01h, read-only=03h, invalid sector/range=04h,
parameters unavailable=07h, ATA controller error=20h, missing device or
timeout=80h. Errors set carry; normal success clears carry and AH.
AH15's AH03 is a successful disk type, not the write-protect status.

A successful write intersecting the mounted volume calls the existing
INT26h `fileio_fat_absdisk_written` coherence hook (WL-0131 CHKDSK row).
Only the overlapping notification is translated to volume-relative
sectors; its buffer is sliced accordingly. Writes to the MBR/reserved
prefix do not masquerade as FAT writes. The currently mounted origin
does not change during a partition editor session; restart/remount is
required after changes, matching the FDISK restart workflow.

## Sources and acceptance

IBM DOS 3.30 Technical Reference, printed pp9-6..9-9: four 16-byte entries,
active80h, single-active rule, packed CHS and head-1 first partition.
That book's p6-25 is INT25h/26h, **not** the BIOS status table. The BIOS
source is IBM *PS/2 and PC BIOS Interface Technical Reference*, April 1987,
pp2-58..61,2-64,2-67; acquired source digest and access limitations are in
`docs/research/block-device-ground-truth.md`. Fixed translation and vector
repurposing are the PM ruling, not attributed to the period manual.

Host arithmetic/status/register/coherence oracle uses independent literal
coordinates and statuses. HEADS and CARRY implementation mutants must fail.
QEMU runs a flat NASM probe against a scratch copy of the mtools partitioned
fixture: MBR55AA/type01 at raw LBA0, marker at raw LBA32, every other sector
unchanged on host readback. RELATIVE adds the mounted base to writes and
must fail at the LBA32 assertion. Gates and mutants belong to TEST_UNIT_GATES
and TEST_EMU_GATES. No Bochs/full-vector execution is authorized in this lane.
