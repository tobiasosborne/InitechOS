# FIND under MS-DOS 3.30

The retained results come from Microsoft MS-DOS 3.30 Disk 1, original image
MD5 `5c2c58602d688c636ea1c9801e6106f4`. FIND.EXE SHA-256 is
`513f58937c7713572d8f204ce3f8a769c93d35c5e66c008f21b4fa54af5ccfa4`.
The [utility acquisition notes](../../../command/tests/compat33/UTILITY-MEDIA.md)
document the archive and disk reconstruction. No DOS binary is distributed
with this fixture.

The original short-batch FIND files recorded in initech-p00t were under
a private temporary path. The first eleven rows in `cases` replay their
recorded outcomes, including the three scans of the 72,007-byte fixture.
The remaining rows exercise switch ordering, diagnostics, named-file
counting, matching across a read boundary, and DOS text EOF. Names and
commands retain their entered case.

`EXECUTE.COM` is built from the test-only `execute.asm` for the reference guest.
COMMAND redirects handles 0 and 1. The probe redirects handle 2 to a
separate file, EXECs FIND with the supplied command tail and passes its
AH=4Dh status back for the batch ERRORLEVEL checks. The empty files and
line terminators are part of the capture. Status greater than 2 marks a
failed probe. In particular, a missing file or a successful scan without
matches returns 0, an empty string returns 1 with no diagnostic, and
invalid syntax returns 2. An unknown option letter warns on stderr before
the utility continues parsing. The reference probe separates stderr from
physical-console writes. The native test launches the shipped flat COM
through COMMAND and compares stderr at the console, separately from the
redirected stdout and the batch status.

Replay on a quiet emulator slot with
`sh dos/find/tests/compat33/capture.sh <disk1.img> <new-output-directory>`.
The script authenticates the media, boots DOS, waits for CAPTURE-DONE,
then reads the output files from the disposable volume with mtools.
`make -C dos smoke-find-args` runs the commands in the booted product
and compares all three outputs with the captured files.
