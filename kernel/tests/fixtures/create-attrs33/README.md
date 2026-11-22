# CREATE attribute capture

`capture.sh` boots a disposable copy of the hash-pinned Packard Bell
MS-DOS 3.30 disk with a blank mtools 1.44 MB B:. `creat33.asm` records
the DOS version, then for CX = 00, 01, 02, 04, 07, 20, 27, 08, 10, 40, 80
and 18h creates B:F<cx> with AH=3Ch. Each succeeding handle writes `abc`,
seeks to 0, reads three bytes and closes, and AH=4300h reads back the
attribute. Extra probes follow, covering CREATE over existing read-only,
hidden and system files with and without the matching CX bits, reopening
the read-only file, truncation with CX=1, CREATE NEW with CX=3 (twice),
a second volume label, CX=10h and CX=0100h. The record layout is
documented in the source.

`dos33-result.bin` is that capture's RESULT.BIN. Two runs were byte
identical. `attrib.txt` is `mattrib -/` of the resulting B:, without
RESULT.BIN. `files.txt` holds each file's bytes as read by mtype.

What DOS 3.30 does: the new entry's attribute is CL OR 20h, and CH is
ignored. The creation handle reads and writes even when the file is
created read-only. CX with 10h, 40h or 80h is refused with 5. CREATE over
a read-only file is refused with 5. CREATE over a hidden or system file
is refused with 5 unless CX carries the same bits, and when it does the
file is truncated to the new attribute. CX=08h creates a volume label
whose handle refuses writes (5) and reads zero bytes. A second label is
refused with 5.

InitechDOS does not create volume labels through CREATE. It refuses
CX=08h with 5 (initech-78dat). The gate checks that row against that
refusal and every other byte against the capture.
