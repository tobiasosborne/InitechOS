# Genuine DOS 3.30 FDISK restart

Captured with genuine MS-DOS 3.30 FDISK.COM on a QEMU 486 with 8 MiB,
RTC 1992-03-02. A copy of the original disk 1 boots as A; FDISK creates
the maximum primary DOS partition on a new 40320-sector IDE fixed disk.
The capture stops before the key which restarts the machine. Independent
`sfdisk --dump` reads back an active type04 partition, start63, size39249.
This is a fixed-disk target capture, not a claim that DOS booted from C.

The original disk 1 MD5 is 5c2c58602d688c636ea1c9801e6106f4, independently
published by PCjs at
https://www.pcjs.org/software/pcx86/sys/dos/microsoft/3.30/ . Its sector
source is
https://diskettes.pcjs.org/pcx86/sys/dos/microsoft/3.30/MSDOS330-DISK1.json .
FDISK.COM SHA256 is
7dec413ba8ffa5d7c8e825f481d5fcc3cdc8160011b072ed7c2834471a891a35.
No proprietary executable is checked in or installed in the product.

MENU.TXT and RESTART.TXT are unedited 80x25 VGA character captures.
Their SHA256s are a9c3c4e47b0a5f83df63572bd0fda1a6a0bd827ab2b9351542fbd0023c3336fc
and 74e21b927dbdd2d88be561a0a4b95efe5389f060629ebfc1b93d5427609b9bdd.
Run `sh dos/fdisk/tests/compat33-restart/capture.sh DISK1 NEW-OUT` to repeat.

Genuine FDISK prints "System will now restart", then asks for a DOS
diskette in A, then "Press any key when ready . . .". The operator's
ATA-only product ruling explicitly forbids calling data A a diskette.
InitechDOS therefore omits that obsolete insertion instruction and keeps
the two captured restart/key lines and key-before-restart behavior. It
does not invent an instruction to insert a fixed volume. The host session
checks both restart branches; the independent MBR differential remains
unchanged. The exact adapted wording is an operator decision, not a claim
that genuine DOS omitted its floppy instruction.
