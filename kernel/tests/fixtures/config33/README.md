# CONFIG.SYS refusals

The hash list pins genuine DOS 3.30 CFGA..CFGF captures owned by DOS under
`dos/command/tests/compat33/expected/`. The test reads each CONFIG input from
the captured foreign FAT image with mtools and compares the product console
diagnostics with its VGA transcription. No expected wording comes from the
product catalogue. See that capture package for media provenance.

The additional omissions obey GM2: configurable BUFFERS and FILES sizes beyond
the physical kernel table receive a visible period refusal. They do not claim
that genuine DOS refuses those valid requests. A supported control stays silent.

Further isolated genuine probes retained in the phase-2 report establish numeric
and suffix syntax: FILES0/256 and numeric junk refuse; FILES7/8/255 accept;
FILES=20 20, LASTDRIVE=ZZ and BREAK=ON junk apply their initial values. The
fuzzer uses host-library conversion instead of copying product arithmetic.

`files5` through `files8` were independently captured on the same genuine
Packard Bell OEM DOS 3.30 media (SHA256
3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30), at 486/8 MiB,
with `probe.asm`. CFG-STATE contains hexadecimal BREAK state, logical drive
count, then successful independent opens of A:\COMMAND.COM under the normal
20-entry process JFT. The final bytes are 02,03,04,05 respectively. The host
gate decodes those captured counts and drives the real INT21/SFT/FAT stack.
One extra physical SFT slot accounts for this kernel's separate CON input and
output entries, retaining a configured DOS capacity of20. Larger configurations
are visibly refused; no implementation of their capacity is claimed.

`lastdrive-state.txt` comes from the same disk and probe in QEMU at 486/8 MiB
with a blank 1.44 MiB B:. Each CONFIG contains `BREAK=OFF`, `LASTDRIVE=` with
the indicated letter, and `FILES=8`. AUTOEXEC runs PROBE and prints
CAPTURE-DONE. The VGA capture from `goldens/dataloss/drive.c` supplies each
CFG-STATE value. The A setting reports two logical floppy letters. D, E, F,
and Z report 4, 5, 6, and 26 respectively.

`startup-state.txt` records CFG-STATE from the phase 4 Packard Bell DOS
3.30 boot captures `audit-config-dos-3.30/boots.txt`. The gate reproduces
the absent, empty, blank, Ctrl-Z, longconfig, unknown-next, CR-only, LF-only
and ANSI cases. The two length-boundary captures use LF padding followed
by FILES=5 and LASTDRIVE=Z. Their complete CONFIG files measure 65533 and
65534 bytes on the same medium and emulator. Bare ANSI is checked with a
file present on the mounted volume, and the missing ANSI case compares
the captured console wording. The product visibly refuses its driver
arguments and files above the captured limit.
