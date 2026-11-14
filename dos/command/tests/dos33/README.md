# MS-DOS 3.30 error and batch captures

`capture.sh ABSOLUTE-OUTPUT` uses the hash-pinned Packard Bell OEM DOS 3.30
media and the existing `goldens/dataloss/drive.c` driver. All inputs are
independent of InitechDOS. `reference/SHA256SUMS` pins the capture, including
complete post-operation disks, raw VGA, literal COPY stdout, batch input,
ERRORLEVEL sentinel, mtools listings and surviving bytes. No expectation is
minted from InitechDOS. `SEVEN.COM` is a five-byte DOS program setting status 7;
COPY leaves it 7 on failure and same-file refusal.

The new cases complement `goldens/dataloss/dos/`: binary concatenation,
omitted-target binary append with 8192 and 65536 bytes initially free, and
concatenation whose last ASCII Ctrl-Z exhausts the disk. All failed targets
are removed, including an appended-to source; all report zero files copied.
Concatenation prints each source operand (including the first append operand)
before the failure. Explicit copying onto a path alias preserves every byte.

The batch capture supplies physical lines 124 through 132 characters long,
with a Z in the last position and redirection before the payload. The files
show 127 characters retained, with the rest of the physical line discarded.
The long DEL case and its directory inventory are captured separately from
this byte-limit observation; truncation alone does not establish which files
a DOS wildcard matches.

A 529-byte REM with a command-looking tail is consumed as one physical line;
`REM.OUT` records execution of the following line. `DOS33_SCENARIOS=batch`
recaptures just this batch leg. Leading whitespace in captured E*.OUT is DOS's
ECHO/redirection framing; the host limit test compares it literally.
