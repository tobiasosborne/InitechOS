Independent genuine MS-DOS 3.30 overlay fixup-address probe.
Captured 2026-10-25; review follow-up initech-i15.

Reference media (never modified or committed):
  Packard Bell OEM MS-DOS 3.30 Sys and Util, 360 KB, disk 1.
  SHA256 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30
  Acquisition/provenance: docs/research/sidekick/PROVENANCE.md, DOS33 row.
  The captured VER output is version.raw.

Source: probe.asm shrinks its own PSP block to 256 paragraphs, allocates A
and B (64 paragraphs each), initializes B:0 to 1111h, and loads PROBE.OVL
at A. overlay.asm is a real 16-bit DOS MZ container with a 16-byte module,
initial WORD 0010h, and one relocation {offset=0,segment=0} at file offset 32.
The default probe supplies factor B; -DEQUAL_FACTOR supplies A as a control.

Each capture is six little-endian WORDs:
  CF, AX (undefined on success), A segment, B segment, A:0, B:0.

factor-different.bin: 0000 3e01 0e3c 0e7d 0010 1f8e
  CF clear. A:0 stayed 0010h. B:0 became 1f8eh = 1111h + 0e7dh.
factor-equal.bin:     0000 3e01 0e3c 0e7d 0e4c 1111
  CF clear. A:0 became 0e4ch = 0010h + 0e3ch. B:0 stayed 1111h.

Thus genuine DOS 3.30 uses the factor in locating the fixup itself.
InitechDOS now follows this addressing behavior for its flat InitechMZ format.
The product comparison uses the captured numeric allocation/factor values,
initial 0010/1111 values and expected final words. Container changes are explicit:
an InitechMZ tag and dword offsets/addends replace 16-bit MZ segment fixups; no
foreign binary is executed. Low words compare directly to capture bytes and
high words must remain zero for these vectors. Both destinations must be owned;
unowned factor addresses and wrapping arithmetic refuse before writing.
The old load-address-only fixup deviation is removed (initech-i1swl).
Ref: IBM DOS 3.30 TR pp.6-196/197,11-4/5.

Reproduce (writes only disposable copies under the output directory):
  sh kernel/tests/fixtures/dos33_overlay/mint.sh /path/to/PB_MSDOS33_Sys_and_Util.img
Optional second argument: output directory (default build/overlay-dos33-mint).
QEMU: CPU 486, 16 MB, RTC 1992-03-02T09:30:00, floppy boot A and data B.
EXITEMU.COM exits QEMU at port F4h with status 33 after the probe returns.
The checker ignores success AX and derives expected fixup words from the
captured segments and independent literal originals, not InitechDOS code.
No genuine DOS system binary is distributed here.
