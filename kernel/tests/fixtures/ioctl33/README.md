# IOCTL device information capture

`capture.sh` boots a disposable copy of the hash-pinned Packard Bell
MS-DOS 3.30 disk with a B: holding FILE.TXT and runs `ioctl33.asm`. It
calls AX=4400h on inherited handles 0-4, then on CON, NUL, AUX, PRN,
CLOCK$ and B:FILE.TXT opened by name, each closed after the query.

`dos33-res.bin` is the DOS version, then for each call the carry flag
and DX: handles 0-2 80D3h, AUX 80C0h, PRN A0C0h, opened CON 80D3h, NUL
80C4h, AUX 80C0h, PRN A0C0h, CLOCK$ 80C8h, the file 0041h (drive B:, not
written). Bit 15 is set for every character device and clear for the
file. PRN alone adds bit 13.

The gate replays the probe through the production INT 21h, device chain
and FAT12 backend. Its disk is drive A, so the file word's drive field is
rewritten to B: before the comparison.
