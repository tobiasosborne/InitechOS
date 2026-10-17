# Indexed VBE handoff (initech-2gva)

The production bootstrap is now split: stage2.asm loads IO.SYS, and
os/boot/io_sys.asm owns vbe_setup. Its preference is 32, then 24, then
640x480x8 mode 0101h, then the existing VGA13 floor. Enumeration order must
not decide depth. Geometry, packed/direct memory model, LFB availability,
pitch and physical base are checked before the handoff.

Local authorities: PRD sections 5, 6.1, 6.3 and 8; spec/hardware.json video;
ADR-0004 OD-2 / AM-7; docs/research/boot-to-text-ground-truth.md sections 2
and 5. The DOS INT21 function register (ADR-0003 Appendix A) is unchanged.
The VBE work lives in IO.SYS's BIOS layer (ADR-0003 DEC-01).

Hardware reference acquired before implementation: VESA VBE Core 2.0,
revision 1.1, source https://www.phatcode.net/res/221/files/vbe20.pdf .
The PDF was inspected through the web PDF reader; shell download was denied
by DNS. This local transcription records the register facts used here:

- Printed p6 (PDF13): mode 101h is 640x480 with 256 colors.
- pp13-14 (PDF20-21): controller capability D0 advertises a switchable DAC;
  D1 identifies non-VGA hardware. A mode set resets precision to six bits.
- pp16-20 (PDF23-27): 4F01h fills the 256-byte mode block. Attributes D0,
  D4 and D7 identify supported graphics with LFB; D5 clear permits VGA ports.
  Offsets 10h,12h,14h,19h,1Bh,28h give pitch, width, height, depth, memory
  model and physical base. Models 4 and 6 mean packed pixels and direct color.
- p25 (PDF32): 4F02h sets BX's mode with bit 14 requesting the LFB.
- p28 (PDF35), section 4.9: 4F06h BL=0 sets logical scanline pixels in CX;
  returned BX is the actual byte stride, independent of visible width.
- p30 (PDF37), section 4.11: 4F08h BL=0 requests BH bits per component;
  AX=004Fh signals success and BH reports the precision actually obtained.

IO.SYS requests eight-bit DAC precision only on switchable indexed hardware;
fixed six-bit hardware remains supported. boot_info appends lfb_dac_bits at
offset 32 (36 bytes total); earlier fields and kernel_base at 28 are unchanged.
The kernel scales DAC writes by that actual width. VGA13 keeps six bits.
SeaBIOS's 0101h descriptor reports attributes 00BBh (D5 set), packed-pixel
model 4, eight bits/pixel, controller capabilities 1. VBE p20 says D5 set
does not guarantee VGA registers. The kernel therefore verifies the palette
ports by write/readback at an unused index before relying on them; an
unavailable DAC fails loud. The QMP RGB oracle verifies those ports drive the
actual displayed colors, not just a register shadow.

The device CLUT and the product surface are distinct index spaces.
spec/assets/clut.json supplies the complete device index domain. The locked
color_canon.json explicit entries override the product's otherwise gray ramp.
tools/flair_dac_gen.c emits all 256 entries, rather than a hand-kept list of
currently visible roles. This includes every named ramp and unnamed app gray.
test-flair-dac is an equality gate; test-color-canon and test-clut remain the
independent decomp value authorities (ADR-0010 CD-5).

The host disk oracle is the existing test-boot-system differential: mdir,
mattrib, mcopy/cmp and fsck.fat on the identical FAT image. Manual pages
rendered at 110 dpi and visually read: IBM DOS 3.30 Technical Reference,
printed 1-7 / PDF24 (BIOS/IO initialization split, system-file load order);
MS-DOS 3.3 User's Guide, printed 119 / PDF204 (SYS, hidden files, order and
permitted fragmentation). They govern the preserved disk layout, not VESA
registers, which postdate DOS 3.3. No new DOS command text or switches ship.

## Test seam and captures

FORCE_VBE_101 is assembled ONLY into build/vesa8/IO.SYS and its stage2 twin.
It filters the BIOS mode list to 0101h and enters the production query,
validation, mode set and DAC negotiation. It never manufactures boot_info or
changes the kernel's depth. make image uses the normal IO.SYS. The gate copies
the flagship image and replaces only IO.SYS plus stage2's embedded raw copy;
INITDOS.SYS and its linked COMMAND.COM transient are the shipped bytes.
The seam additionally calls 4F06h with 648 logical pixels; the visible 640
pixels are unchanged, and the actual returned pitch is handed off. This makes
the pitch obligation observable instead of accidentally passing at pitch=width.
Replacement IO.SYS timestamps are pinned to the same DOS epoch as the normal
image recipe. test-boot-vesa-images runs mtools/dosfstools on these exact images.

test-flair-desktop-8bpp captures the canonical scene and grades it with the
unchanged ppm_flair_check. It also boots the flagship before/after the locked
spec/flair_appswitch_trace.mk NOTES click. The same appswitch checker runs,
and all 307200 RGB pixels of each capture must equal the real 32bpp QMP
capture. Thus the gate observes the hardware DAC conversion performed by QEMU,
not merely the host render.c branch. The default image must report bpp=32;
the forced image must report bpp=8. Every QEMU call exports INITECH_QMP_STDIO=1.

Representative exact argv (pre8 capture, relative to repo root):

```
qemu-system-i386 -display none -no-reboot -rtc base=1993-03-01T09:00:00,clock=vm -device isa-debug-exit,iobase=0xF4,iosize=0x04 -serial file:build/vesa8/vesa8-pre8.serial -d int,guest_errors,cpu_reset -D build/vesa8/vesa8-pre8.qemu.log -drive format=raw,file=build/vesa8/flagship.img -drive file=build/vesa8/vesa8-pre8-data.img,format=raw,if=ide,index=1 -qmp stdio
```

The harness issues qmp_capabilities, waits for FLAIR-TENANTS-READY, then
screendump to build/vesa8/vesa8-pre8.ppm and quit. Post8 additionally injects
m70:30,m70:30,l1,l0 after FLAIR-LIVE-READY and waits for the NOTES dispatch.
The report records the actual debug log filename; keep this example synchronized.

Red-first evidence: the original 0..8 table failed at idx9 (the locked alert
extension); the forced gate with unchanged IO.SYS failed expected-8bpp.
Mutants omit119/omit165 skip those actual DAC writes, rowcopy uses width as stride,
and dac6 scales writes as six bits while the hardware is eight. Every pixel
mutant must reach the desktop, produce a valid capture without triple fault,
and fail specifically on RGB inequality. Generator omissions also fail the
host equality gate at the omitted index.
test-boot-vesa-mutant removes the 0101h fallback and reverses depth preference
in separate real IO.SYS images; the same mode oracle rejects both.

Rule 5 remains a PM obligation: test-boot-bochs, test-flair-desktop-bochs,
test-flair-appswitch-bochs, and the 86Box boot/desktop leg on period hardware.
No Bochs was run in this lane. Rule-14 acceptance clips remain with the PM's
record-flair workflow; this lane changes the video backend, not window behavior.
