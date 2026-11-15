# Genuine MS-DOS 3.30 XCOPY large-file capture

Captured 2026-11-15 with QEMU, 486, 8 MiB, RTC 1992-03-02. The original
MS-DOS 3.30 disk 1 has MD5 5c2c58602d688c636ea1c9801e6106f4; genuine
XCOPY.EXE from disk 2 has SHA256
c4c694e1c1625e8d22bf1cfa658eb814ba1e1259400deef0fa4a9a1ee2bb8e31.
The raw disk images were reconstructed from PCjs's Microsoft 3.30 archive;
both raw-image MD5s match its independently published original-image hashes:
https://www.pcjs.org/software/pcx86/sys/dos/microsoft/3.30/
https://diskettes.pcjs.org/pcx86/sys/dos/microsoft/3.30/MSDOS330-DISK1.json
https://diskettes.pcjs.org/pcx86/sys/dos/microsoft/3.30/MSDOS330-DISK2.json
Disk 2 MD5: 7e7ed2b3f9864a1a383238f263999fc3. No proprietary executable
is checked in or installed in the product.

Run `sh dos/xcopy/tests/compat33-large/capture.sh DISK1 XCOPY.EXE NEW-OUT`.
It copies the genuine boot disk, removes only 4201.CPI from that copy to
make room for CONFIG/AUTOEXEC, and executes the genuine utility against a
new FAT data disk. The input contains 20000 ten-byte hexadecimal CRLF
records, exactly 200000 bytes, followed by ZZZ.TXT; DST has older, different
LARGE.BIN bytes. `XCOPY B:\SRC B:\DST /V` completes with ERRORLEVEL 0.
RESULT.TXT, STATUS.TXT and VERSION.TXT are unedited captured files.
Mtools extracts both destination files; cmp requires identical source bytes.
The large source and result both have SHA256
b777871862e6350d378b6686166da264c2e70259d06823b17598bf29ff6b4561.

This establishes successful copying above 128 KiB and continued enumeration.
It makes no claim about genuine XCOPY read-fault or rollback semantics.
InitechDOS's retention requirement comes from the operator; its fault tests
use independent mtools readback. The original capture directory, raw VGA,
QEMU transcript and before/after disks are retained in the owner handoff.
