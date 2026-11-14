# Genuine DOS 3.30 raw path captures

Captured 2026-11-14 before changing kernel DOS name handling, at a 486/8 MiB
under QEMU. The boot media is the pinned PB MS-DOS 3.30 system disk from
the local reference collection:

`/home/tobias/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img`

SHA256: `3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30`.
No DOS binary is included here. The probes are independently written 16-bit
clients of genuine DOS INT21. They never invoke COMMAND's operand parser.

Reproduction: assemble `probe.asm` with `nasm -f bin` to RAWNAME.COM. Copy the
boot media to a disposable floppy and use mtools to replace AUTOEXEC.BAT with
`@echo off`, `B:`, `RAWNAME.COM`, `echo CAPTURE-DONE`, each terminated by CRLF.
Format a second 1.44 MiB floppy with mtools, create SUBDIREC, and copy RAWNAME.COM
onto it. Run `goldens/dataloss/drive.c` with OUT BOOT BDISK - STEPS; STEPS waits
for CAPTURE-DONE, captures `result`, then ends. The driver uses CPU486, 8 MiB,
fixed RTC and private monitor pipes. Extract RESULT.BIN with mtools.
The retained data.img.gz is that genuine DOS-written second floppy; result.vga
and result.txt show completion. SHA256SUMS pins captures and probe inputs.

The first probe has20 records in the same order as cases.tsv. Each23-byte
record is little-endian AX/FLAGS for CREAT, WRITE, OPEN, READ, then7 read bytes.
On a failed CREAT, unused fields remain FF. Each successful case writes and
reopens `raw API`. Success handle numbers and unrelated flags are not contracts;
carry, refusal AX, transfer counts and bytes are graded. cases.tsv pairs the
original operand with its name independently read from the native FAT using
mtools. `-` means refusal. Native results distinguish wildcard characters beyond
an already full field (ignored) from punctuation beyond it (refused), spaces
ending a field, and second dots (refused). Parent components shorten separately.

Repeat with namespace.asm on a fresh second floppy. namespace.bin contains13
AX/FLAGS records then7 read bytes: MKDIR long directory, CHDIR long directory,
CREAT long file, WRITE, CLOSE, GETATTR, RENAME long old/new names, OPEN long
renamed spelling, READ, CLOSE, UNLINK long renamed spelling, CHDIR root, RMDIR
long directory. All13 native carries are clear; counts and payload are retained.
The host test grades carries, transfer counts and payload; successful operations'
unspecified AX values are ignored. The remaining host image has only the seeded
SUBDIREC, independently listed by mtools. Cluster conservation is checked by the
independent FAT reader. CREATE_NEW also checks the non-destructive existing-alias
invariant on each successful name. No synthetic expected result comes from the
product's normalizer; the strict low-level FAT format tests remain unchanged.
