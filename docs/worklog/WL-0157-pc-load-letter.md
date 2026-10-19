# PC LOAD LETTER (initech-s25)

The exception dispatcher now paints the physical display from the boot handoff,
before serial output, without CON state, heap allocation, Toolbox calls, BIOS
calls, or interrupts. This also works before console initialisation and after
the desktop has changed the DAC or bound a smaller DOS console viewport.
The tenant-fault hook and spurious-vector return path retain their order and
behaviour. The obsolete panic-to-console binding has been removed.

The field is black with light-gray VGA ROM text: `PC LOAD LETTER` at 2x,
centred above `ERROR nn -- SEE OPERATOR` at 1x. The vector is decimal, two
digits. The 64-pixel two-line panel is vertically centred; the inter-line gap
is 16 pixels. The renderer handles padded 8/24/32bpp rows. The indexed path
resets its two DAC entries at the negotiated 6/8-bit precision, independently
of the desktop palette. No new font strike is stored in the image.

The release object retains the existing serial `PANIC vec=NN err=NNNNNNNN`
and `HALTED` markers. `-DPANIC_DEBUG=1` retains the original serial register
dump byte format. No diagnostic registers or addresses reach the CRT.

## Ground truth

- `CLAUDE.md` Rule 2 and `InitechOS-PRD.md` Appendix B, line 328: the panic
  phrase, printer-error pastiche, and debug/serial split.
- PRD Sec 5, line 105: the CRT text strike is VGA ROM 8x16. The s25 lane brief
  explicitly permits 2x VGA or Chicago and specifies the operator-status idiom.
  Appendix A, lines 309-323, describes the desktop frame, which contains no
  panic screen. No panic glyph extraction from that frame is claimed.
- `docs/research/boot-to-text-ground-truth.md` Sec 1.2; `console.h`,
  `boot_info.h`, and `kmain.c`'s `vga_set_dac`: glyph rows, physical display
  handoff, byte order, and negotiated DAC precision. The absolute low-memory
  handoff requires GCC's `--param=min-pagesize=0`, as in other hardware paths.
- Existing `panic.c`, `idt.c`, `isr.asm`, and `idt.h`: real exception vectors,
  frame layout, unchanged register dump, fault triage, spurious return, halt.
- ADR-0003 Appendix A (3Ch/40h/3Eh) and DEC-08; DEC-04a's flat ABI:
  the test-only FAT-loaded `.COM` saves the real pre-fault font to `FONT.BIN`.
- Locally rendered and viewed IBM DOS 3.30 Technical Reference printed
  pp.6-122 and 6-139/140 (PDF pp.245 and 262/263): CREAT and WRITE, including
  the returned-byte-count check. MS-DOS 3.3 User's Guide printed p.45
  (PDF p.130) was also inspected for the existing CLS contract. DOS manuals
  do not specify this fictional panic panel; its wording comes from the PRD
  and lane brief, not from a claimed DOS original.

## Oracles and mutation evidence

Tests were written before the renderer. The initial host execution reported
`RED panic-layout: off-centre case=0 got=0,0 want=208,208`. The old handler
reached a real #UD and retained its dump, then failed the new pixel oracle:
`RED panic-pixels: x=0 y=0 got=111,160,142 expected=field`.

`test-panic-layout` grades eight literal message/status origins and guarded,
padded framebuffer writes at 8/24/32bpp. `test-panic-screen` injects the locked
`p,a,n,i,c,ret` trace at SHELL-READY, executes the real FAT `.COM`, and checks
#UD/#GP, VBE32, VBE8, VGA13 (320x200), and release/debug serial separation.
Every pixel is checked. The independent checker does not include the layout
header: it computes expected origins and glyph rows from the fixture's saved
ROM bytes, never from the captured screen. mtools reads back the identical
guest-written FAT image; dosfstools checks that image. The fixture is not
added to the shipped data volume.

- `PANIC_MUTATE_OFF_CENTRE`: `RED panic-layout: off-centre case=0
  got=216,208 want=208,208`.
- `PANIC_MUTATE_NO_8BPP`: real exception/serial/FAT checks pass, then
  `RED panic-pixels: x=0 y=0 got=111,160,142 expected=field`.

Both real gates and mutant gates are in the Makefile TEST vectors.
`test-panic-screen-bochs` is written and dry-run checked but deliberately not
executed, per the lane instruction. It checks the real FAT `.COM` #UD and
debug registers through the existing Bochs serial harness; that harness does
not expose a usable VGA screendump, so this leg claims no pixel coverage.

## Regression observations

Passed: test-boot, test-shell, test-spurious, test-idt, test-panic (#DE),
test-fpu-absent (#NM), test-fpu-mf, test-flair-desktop, image,
test-flair-app-launch (all ten legs, including contained tenant #UD), and
test-flair-finder-service (normal and damaged-volume legs).

The existing secondary-shell recipe at Makefile:9589 uses `$<`, which resolves
to `build/serial_trace_pack`, so a clean build fails to produce its object.
The boot/shell regression run used `/tmp/pcload-secondary.mk`, a build-only
override invoking the existing `kernel-main-compile` macro on `kmain.c` with
the intended BOOT_SHELL/BOOT_SECONDARY_SHELL flags. The tracked recipe was not
changed. The complete command and output are in `/tmp/pcload-regression.log`.

An isolated `test-flair-locked-purge` reaches `FINDER-TRASH-EMPTIED purged=17
refused=2` and `FINDER-LOCKED-ALERT`, with bytes/attributes preserved. Its pixel
oracle remains RED: `FULL Trash (TRASH_FULL): 617/1024 cells differ from the
mint-008 capture`, then `FAIL locked-trash: locked notice pixels missing`.
No Finder implementation, fixture, golden, or tolerance was changed.

The release panic object is 1,110 bytes total (1,106 text, 4 BSS), compared
with 1,281 bytes for the old handler compiled with the same low-level flags:
171 bytes smaller. The debug object is 1,899 bytes. This is object-level
evidence; the all-kernel headroom gate supplies the binding image measurement.

`test-kernel-headroom` is GREEN: all 237 kernels measured in the 196,608-byte
window, with a 4,096-byte floor. The flagship `kernel_flairtenants.elf` loads
176,916 bytes (19,692 bytes headroom). The binding image is
`kernel_flairtenants_db_write_skip.elf`, 178,196 bytes loaded and 18,412 bytes
headroom. Full output: `build/panic-screen/headroom.log`.

No full `make test`, Bochs execution, commits, git writes, pushes, bead edits,
CLAUDE.md edits, or HANDOFF.md edits were performed.
