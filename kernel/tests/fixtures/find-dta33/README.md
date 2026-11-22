# FINDFIRST/FINDNEXT continuation capture

`capture.sh` boots a disposable copy of the hash-pinned Packard Bell
MS-DOS 3.30 disk with a B: minted by `mint.sh` (root A1.TXT, B1.TXT,
A2.TXT, BSUB, SUB, A3.TXT, B2.TXT; SUB holds S1.DAT and S2.DAT; fixed
timestamps). `findta33.asm` fills three DTAs with CCh, then interleaves
searches. FINDFIRST runs `A*.*` in DTA A and `B*.*` in DTA B, both with
attribute 10h, and FINDNEXT alternates between them. DTA A is then copied
into DTA C and FINDNEXT runs on C, then on A again. B restarts on `\SUB\*.*`
and both are run to exhaustion. Each step records the operation, CF, AX and
all 43 bytes of the DTA it used. Two captures were byte identical.

What DOS 3.30 does: the DTA alone holds the search. Byte 0 is the drive
(A=1), bytes 1-11 the template, byte 12 the attribute mask, bytes 13-14 the
matched entry's index in its directory and bytes 15-16 that directory's
start cluster (0 for the root). A copied DTA continues from the same entry,
so FINDNEXT on C and then on A both return A3.TXT. An exhausted FINDNEXT
returns 12h and leaves the DTA unchanged. Bytes 17-20 are DOS-internal (the
same value in every record) and the name is written as far as its NUL plus
padding, leaving the rest of the DTA untouched.

The gate compares the operation, CF, AX, DTA bytes 0-16 and 21-29, and the
name through its NUL. It does not compare bytes 17-20 or the name's padding.
