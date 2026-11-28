# VERIFY flag capture

`capture.sh` boots a disposable copy of the hash-pinned Packard Bell
MS-DOS 3.30 disk with a blank B: and runs `verify33.asm`. It reads the
flag with AH=54h, then for AL = 01h, 00h, 02h, 03h, FFh, 80h, 01h calls
AH=2Eh (DL=0) and AH=54h, keeping AL after each.

`dos33-res.bin` is the DOS version, the boot value 00h, then the pairs
01 01, 00 00, 00 00, 01 01, 01 01, 00 00, 01 01: AH=2Eh stores AL AND 1
and leaves that value in AL, and AH=54h returns it.

The gate replays the probe through the production INT 21h. It then
writes through the FAT12 backend to a disk that silently drops the
writes of one AH=40h call: with VERIFY off the write reports success,
with VERIFY on the readback fails it (MS-DOS 3.3 User's Reference,
VERIFY). The DOS critical-error prompt for a failed verify is not
captured; the failure takes the kernel's ordinary write-error path.
