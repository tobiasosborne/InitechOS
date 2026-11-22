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

Not captured: a 03h byte in the file, buffered input (0Ah) and the polling
functions (06h, 0Bh).
