# Executable environment, MS-DOS 3.30

These byte dumps run from the first environment string through the executable
path's terminating NUL. The 16-bit reader used INT 21h AH=62h and PSP offset
2Ch, then wrote that range to disk. The boot disk was the Packard Bell MS-DOS
3.30 system disk used by Turbo Pascal MINT07, SHA-256
`3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30`.

- `PARENT.BIN` was written by A:\PARENT.COM after it ran A:\SUB\DUMP.COM
  through EXEC with environment segment zero.
- `CHILD.BIN` was written by that child. It matches the dumps from
  COMMAND.COM's relative launch in A:\SUB and PATH launch from A:\.

At offset 46 the last environment string ends in two zero bytes. At offset
48 the little-endian word is 1. The ASCIIZ executable pathname starts at
offset 50. Microsoft documents the arrangement in the [MS-DOS Encyclopedia,
section II, EXEC, figure 10-2](https://www.pcjs.org/documents/books/mspl13/msdos/encyclopedia/section2/).

COMMAND.COM qualified and uppercased the path for both shell launches. Direct
EXEC on this particular DOS disk preserved the input spelling when the
probe supplied `sub\dump.com` or `a:.\sub\..\sub\dump.com`. InitechDOS
uses the qualified form for direct EXEC as well, as required by initech-311t2.
The tier 0 test compares the captured bytes for the parent and the child,
substituting the drive letter for the second child on C:. The raw reader and
capture commands are in the phase 4 reports repository.
