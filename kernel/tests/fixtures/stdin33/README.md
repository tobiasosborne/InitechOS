# Redirected character input capture

`capture.sh` boots a disposable copy of the hash-pinned Packard Bell
MS-DOS 3.30 disk with a B: minted by `mint.sh`. B:IN.TXT holds the eight
bytes `H i CR LF NUL E0h ^Z z`. `stdin33.asm` makes IN.TXT standard input
and a new B:ECHO.TXT standard output with DUP2 (AH=46h). It then calls
AH=01h, 07h, 08h, 01h, 07h, 08h, 01h, 07h and records AL after each call
in B:RES.BIN (saved after every call). With the default build it then
closes standard output so ECHO.TXT is complete.

`dos33-res.bin` is the DOS version followed by the eight AL values, which
are the file's bytes in order. `dos33-echo.txt` is ECHO.TXT, holding the
bytes that the three AH=01h calls echoed (`H`, LF, ^Z) unchanged. Built with
`EOF=01h`, `EOF=07h` or `EOF=08h`, the program makes one more call of that
function at end of file. None of the three returned within 30 seconds, and
RES.BIN still held the same ten bytes.

`buffered33.asm` records four AH=0Ah requests in `dos33-res0a.bin` and
their output in `dos33-echo0a.txt`. The file supplies editing and line
terminators, a leading LF is dropped, and a file with no next byte waits
at EOF. An independent DOS 3.30 poll of H, NUL and Ctrl-Z returned FFh
from AH=0Bh before each byte and 00h after those three bytes. Polls
left all three bytes available for the following reads.

The paired COMMAND audit uses a file containing CR LF for DATE and one
containing y CR LF for PAUSE. Genuine DOS completes both with no keystrokes.
The host gate also reads a literal K CR LF from a separately saved CON
handle while standard input points to a file.
