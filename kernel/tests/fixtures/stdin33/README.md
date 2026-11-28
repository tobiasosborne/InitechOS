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

`buffered33.asm` makes four AH=0Ah calls (max 8, buffers pre-filled with
CCh) on IN0A.TXT; `dos33-res0a.bin` and `dos33-echo0a.txt` are its result
and echo. `status33.asm` uses IN0C.TXT (`ABCline` CR LF): AH=0Bh, AH=0Ch
with AL=01h, 07h, 08h, AH=0Ch AL=0Ah (max 16), AH=0Bh with the LF left,
AH=07h, then AH=0Bh at end of file. `dos33-res0c.bin` is FFh, `A B C`,
the line `line` (count 4), FFh, 0Ah, 00h; `dos33-echo0c.txt` holds `A`
and `line` CR. Run each with `PROGRAM=<name> sh capture.sh <new dir>`.

`ctrlc33.asm` sets BREAK off, hooks INT 23h, makes a new empty
B:EMPTY.IN standard input, puts Ctrl-C (2E03h) in the BIOS keyboard buffer
with INT 16h AH=05h and calls one function at end of file. Run it with
`PROGRAM=ctrlc33 EOF=<01h|07h|08h|0Ah>`. `dos33-ctrlc<ah>.bin` is the
version, `B` before the call, `C` when INT 23h was entered, and `E` when
the keyboard buffer was then empty. AH=01h, 08h and 0Ah entered INT 23h
and took the key; AH=07h was still waiting after 25 seconds (`B` only).

Not captured: a 03h byte in the file and AH=06h.

`../../smoke/redirected-input.sh` boots the product image with
`date <BLANK.IN` and `pause <KEY.IN >P2.OUT` in AUTOEXEC and sends no key;
the DOS 3.30 clock-input and pause-input captures (phase 4 command audit,
rows 12 and 13) finish the same batch without keyboard input.
