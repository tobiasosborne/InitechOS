<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# Boot-to-Text Ground-Truth Brief

**Document ID:** PE-RESEARCH-0002  
**Programme:** STAPLER (InitechOS)  
**Scope:** M1 milestone -- stage2 -> C kernel handoff, VGA ROM font capture,
80x25 LFB text console, InitechDOS banner.  
**Relevant beads:** initech-d00 (handoff), initech-yqb (console), initech-bea
(banner + gate). Epic: initech-2rb (M1).  
**Law 1 compliance:** every claim below cites a file:line in this repo OR an
explicit hardware/BIOS reference.

---

## 1. VGA ROM 8x16 Font -- Acquisition, Format, Stash Plan

### 1.1 BIOS call

In real mode (before the PM switch), execute:

```
AX = 1130h
BH = 06h         ; "8x16 ROM BIOS font" selector
INT 10h
```

On return:
- **ES:BP** points to the first byte of the 8x16 font table in the BIOS ROM.
- **CX** contains the glyph height (= 16 for this selector).
- **DL** contains the number of rows on screen minus one (= 24 for 80x25).

Source: IBM VGA Technical Reference, INT 10h Function 11h Subfunction 30h
(AX=1130h); also RBIL (Ralf Brown's Interrupt List), INT 10/AX=1130h,
BH=06h. This is the standard way every period DOS and embedded OS captures
the ROM font. The call is read-only and has no side effects on the mode or
the display.

### 1.2 Glyph format

- 256 glyphs, indexed by ASCII/CP437 code.
- Each glyph = 16 bytes (rows top to bottom), 1 byte per row.
- **MSB = leftmost pixel** (bit 0x80 = column 0; bit 0x01 = column 7).
- A set bit = ink (foreground); clear bit = background.
- Total table size: 256 * 16 = **4096 bytes**.
- Cell width is always 8 pixels (the VGA hardware provides column 8 from
  the line-graphics register, but in LFB mode we blit 8 px per glyph, not 9).

This format is identical to the chicago8x16.h format (spec/assets/chicago8x16.h:
"One byte per row, 16 rows per glyph, MSB = leftmost pixel" -- chicago8x16.h
lines 32-34). The VGA ROM font is the correct font for the DOS text console
(PRD Sec 5: "Text: 80x25 rendered by blitting the VGA 8x16 ROM font into the
LFB"); chicago8x16.h is the GUI font (system/dialog), not the console font.

### 1.3 Capture slot in stage2

The VGA ROM font call MUST execute in real mode, before the PM switch --
INT 10h is a BIOS service unavailable after CR0.PE=1 (CLAUDE.md hallucination
callout: "INT 10h/13h must happen before the PM switch"). The natural insertion
point is immediately after the VBE setup succeeds and before the A20 enable,
parallel to how stage2 already calls VBE (INT 10h) at stage2.asm:68 (the
`call vbe_setup` line). Concretely, the new code slots in after line 71
(msg_vbe serial marker) and before line 74 (A20 block comment):

```
; -- VGA ROM font capture (must be in real mode, before PM switch) --
mov ax, 0x1130
mov bh, 0x06
int 0x10
; ES:BP now points at the 256*16-byte ROM font table.
; Copy 4096 bytes to FONT_STASH (physical low-memory address, see Sec 2).
mov si, bp          ; source offset within ES
mov di, FONT_STASH  ; flat destination (ds=0)
mov cx, 4096
cld
rep movsb
; Stash the physical address of FONT_STASH in boot_info (Sec 2).
```

Because ES is set to the ROM segment by INT 10h (not necessarily 0), the
movsb loop must use ES:SI as source and DS:DI as destination, with DS=0.
After the font copy, the captured bytes are at a known physical address in
low memory; the kernel can read them from C without any BIOS involvement.

### 1.4 Portability across the tri-emulator set

QEMU (SeaBIOS), Bochs (internal VGA BIOS), and 86Box (period Cirrus/ET4000
VGA BIOS) all implement INT 10h AX=1130h BH=06h and return a valid 8x16
font pointer. This is a mandatory part of the VGA BIOS spec and has been
standard since the IBM VGA Reference. The only known risk is if a BIOS
returns a null/zero pointer (indicating "font not initialized"); this is
non-standard and should be guarded with a fail-loud halt if ES:BP == 0:0
after the call. The three emulators in this project's tri-emulator suite
(PRD Sec 8; QEMU as dev loop, Bochs for transition accuracy, 86Box for
period authenticity -- Makefile:191-192 PRD reference) all expose the
standard VGA ROM font through this call. Risk: LOW; guard the return anyway
(CLAUDE.md Rule 2 -- fail fast, fail loud).

---

## 2. boot_info Handoff Contract

### 2.1 What stage2 currently captures (stored vs. transient)

Stored variables (stage2.asm:384-387, the data section):

```asm
lfb_addr:  dd 0   ; PhysBasePtr  (ModeInfoBlock offset 0x28, dword)
lfb_pitch: dd 0   ; BytesPerScanLine (ModeInfoBlock offset 0x10, word)
lfb_bpp:   dd 0   ; BitsPerPixel (ModeInfoBlock offset 0x19, byte)
```

**Width and height are NOT stored.** They are read transiently (stage2.asm:
159-163) only to filter the mode and then discarded. The 32-bit fill uses the
compile-time constants WANT_W (640) and WANT_H (480) from the equates at
stage2.asm:47-48. The C kernel needs the actual width and height, so they
must be added to boot_info (using the compile-time constants is fine since
the VBE scan asserts equality before proceeding -- stage2.asm:159-163).

### 2.2 Proposed boot_info struct

```c
/* boot_info.h -- stage2 -> C kernel parameter block.
 *
 * Ref: PRD Sec 5 (hardware contract: VBE 2.0 LFB, 640x480, 8/32bpp, flat
 *      binary kernel handed off by stage2, ADR-0003 DEC-08); stage2.asm
 *      lfb_addr/pitch/bpp variables (lines 384-387); CLAUDE.md ADR-0002 +
 *      CDR-0001 (freestanding C, gcc -m32 -ffreestanding -nostdlib).
 * beads: initech-d00.
 *
 * ARTIFACT code: freestanding, <stdint.h> only, no libc.
 */
#ifndef INITECH_BOOT_INFO_H
#define INITECH_BOOT_INFO_H

#include <stdint.h>

typedef struct {
    uint32_t lfb_addr;    /* physical base of the linear framebuffer        */
    uint32_t lfb_pitch;   /* bytes per scanline (may exceed width * bpp/8)  */
    uint32_t lfb_bpp;     /* bits per pixel: 24 or 32                       */
    uint32_t lfb_width;   /* horizontal resolution in pixels (640)          */
    uint32_t lfb_height;  /* vertical resolution in pixels (480)            */
    uint32_t font_addr;   /* physical address of the 4096-byte ROM font     */
                          /* (256 glyphs * 16 bytes; MSB=leftmost pixel)    */
} boot_info_t;

/* Fixed physical address of the boot_info block (see Sec 2.3). */
#define BOOT_INFO_ADDR  0x00000500u

/* Fixed physical address of the captured ROM font (4096 bytes). */
#define FONT_STASH_ADDR 0x00001000u

#endif /* INITECH_BOOT_INFO_H */
```

All fields are 32-bit to keep the struct simple and naturally aligned on a
32-bit boundary. Total size: 24 bytes -- well within any reasonable block.

### 2.3 Chosen physical address and justification

**boot_info at 0x00000500 (80 bytes into the conventional low-memory area).**

Physical memory map in use:

| Range               | Owner                                                        |
|---------------------|--------------------------------------------------------------|
| 0x00000000..0x003FF | Real-mode IVT (1 KiB; left intact -- BIOS owns it)          |
| 0x00000400..0x004FF | BIOS Data Area (256 bytes; left intact)                      |
| 0x00000500..0x0051F | **boot_info_t (24 bytes; chosen here)**                      |
| 0x00001000..0x001FFF | **FONT_STASH: 4096-byte ROM font copy (chosen here)**       |
| 0x00007000..0x0073FF | VBE_CTRL_BUF + VBE_MODE_BUF (stage2.asm:51-52, 768 bytes)  |
| 0x00007C00..0x007DFF | MBR load address (consumed; reused as stack base)           |
| 0x00008000..0x009FFF | stage2 binary (loaded by MBR; 8 KiB = 16 sectors)           |
| 0x00090000          | stage2 32-bit stack (esp = 0x00090000; stage2.asm:251)      |
| kernel (TBD)        | 0x00010000 or 0x00100000 (see Sec 3)                         |
| LFB (VBE)           | High physical address (typically 0xE0000000 under QEMU;      |
|                     | captured in lfb_addr; accessed via flat segments)            |

0x500 is above the BIOS Data Area (which ends at 0x4FF) and is the
conventional "safe low-memory scratch" region used by bootloaders since the
early PC era. It is well below the 0x7000 VBE buffers and the 0x8000 stage2
load. 0x1000 (for FONT_STASH) is safe -- above BDA, below VBE buffers.

stage2 must:
1. Capture the ROM font to FONT_STASH (0x1000) in real mode (Sec 1.3).
2. Write all boot_info fields to 0x500 before the PM switch.
3. Far-jump into the kernel (see Sec 3).

### 2.4 Passing mechanism: fixed address vs. register

**Recommendation: write boot_info to the fixed address 0x500; the kernel
reads it directly from memory. Do NOT pass a pointer in a register.**

Rationale: the kernel is a flat binary with a fixed link address (Sec 3).
After the far jump, the kernel entry stub sets up its own stack and then
calls into C. Register state across a far jump from 32-bit asm to flat C is
not guaranteed to be preserved cleanly across C ABI boundaries -- EAX might
hold a useful value, but relying on it conflicts with the calling convention
(cdecl: EAX, ECX, EDX are caller-saved and the compiler does not assume any
initial value). A fixed physical address is unambiguous, needs no register
convention, and survives any intervening asm stub. The kernel simply casts
the compile-time constant to a pointer:

```c
const boot_info_t *bi = (const boot_info_t *)BOOT_INFO_ADDR;
```

This approach is flat-binary and no-relocation safe: the address is a
physical constant, not a virtual address that requires fixup.

---

## 3. Flat C Kernel: Link Address, Load Mechanism, Build Recipe

### 3.1 Link address choice

**Recommendation: link the kernel at 0x00010000 (64 KiB).**

Discussion:
- The kernel MUST be linked at a physical address where it will actually
  reside. There is no paging, no virtual memory (PRD Sec 5: "Flat 32-bit;
  bump + free-list allocator; no demand paging"), so link address == load
  address == physical address.
- **0x00010000 (64 KiB):** safe -- above FONT_STASH (ends at 0x1FFF), well
  below the VBE buffers (0x7000) and stage2 (0x8000). Real-mode INT 13h
  loads below 1 MiB are required (see constraint below); 0x10000 satisfies
  this trivially. Leaves ~24 KiB between font stash and kernel (fine).
- **0x00100000 (1 MiB):** standard multiboot location, but it is NOT directly
  loadable via INT 13h in real mode. INT 13h CHS loads go into a real-mode
  segment:offset target, which means ES:BX must address the destination.
  Real mode can only address the first 1 MiB (segment 0xFFFF:0xFFFF =
  physical 0x10FFEF, but the A20 line complicates this above 0xFFFFF).
  Loading at 1 MiB from real mode requires a two-step: INT 13h to a
  sub-1MB staging buffer, then a 32-bit copy after the PM switch. That is
  correct but adds complexity. For the minimal-correct first cut, 0x10000
  is simpler and avoids the real-mode 1 MiB constraint entirely.

**Constraint (CLAUDE.md hallucination callout):** INT 13h is a real-mode
BIOS service. The load destination ES:BX must be a valid real-mode address
(i.e., physical address < 1 MiB). 0x10000 corresponds to ES=0x1000 BX=0x0000
-- a valid real-mode segment:offset pair. The kernel is therefore directly
loadable by INT 13h before the PM switch.

### 3.2 Build recipe

```
# Linker script (os/boot/kernel.ld):
SECTIONS {
    . = 0x00010000;
    .text   : { *(.text*)   }
    .rodata : { *(.rodata*) }
    .data   : { *(.data*)   }
    .bss    : { *(.bss*)  *(COMMON) }
    end = .;
}

# Entry stub (os/boot/kstart.asm, nasm -f elf32):
bits 32
global _kstart
_kstart:
    mov esp, 0x0008FFFC   ; stack below stage2's stack at 0x90000; 4-byte aligned
    xor ebp, ebp
    push ebp              ; 0 frame pointer (signals top of stack to debugger)
    call kernel_main      ; C entry: void kernel_main(void)
.hang:
    hlt
    jmp .hang

# Compile:
gcc -m32 -ffreestanding -nostdlib -fno-stack-protector \
    -Wall -Wextra -Werror -std=c11 -Ispec -Ios/milton \
    -c os/milton/kernel.c -o build/kernel.o

nasm -f elf32 os/boot/kstart.asm -o build/kstart.o

ld -m elf_i386 -T os/boot/kernel.ld --oformat binary \
    build/kstart.o build/kernel.o -o build/kernel.bin
```

The `--oformat binary` (or equivalently `objcopy -O binary`) strips ELF
headers and emits a raw flat binary at the link address -- exactly what
ADR-0003 DEC-08 specifies ("flat binary image"; Makefile line 210 in the
ADR text, CDR-0001 line 48: "The 32-bit protected/flat memory model and the
executable-format decisions of ADR-0001 and ADR-0003 (DEC-08) are unaffected
by this Deviation").

**No timestamps, no host paths** in the binary (CLAUDE.md Rule 11): the
only date-bearing data would come from __DATE__/__TIME__ macros, which must
not be used (they break reproducibility and the self-host certificate).

### 3.3 Image layout and stage2 kernel load

The kernel is appended to the raw disk image after stage2. Proposed layout:

```
sector  0      : MBR (512 bytes)
sectors 1..16  : stage2 (STAGE2_SECTORS=16, 8 KiB padded; Makefile:105-108)
sectors 17..N  : kernel (flat binary; N = 17 + ceil(kernel_size / 512) - 1)
sectors N+1..63: zero pad (IMG_SECTORS=64 currently; expand as needed)
```

stage2 loads the kernel in real mode via INT 13h CHS before the PM switch
(because INT 13h is a real-mode BIOS call -- same constraint as VBE and the
ROM font call). The loading code slots in after the ROM font capture and
before the A20/GDT sequence:

```asm
; -- Load flat kernel into 0x10000 (real mode, INT 13h CHS) --
mov ax, 0x1000          ; ES = segment for physical 0x10000
mov es, ax
xor bx, bx              ; BX = 0 => ES:BX = 0x1000:0x0000 = physical 0x10000
mov ah, 0x02            ; BIOS read sectors
mov al, KERNEL_SECTORS  ; sector count (compile-time constant)
mov ch, 0               ; cylinder 0
mov cl, 18              ; sector 18 = LBA 17 + 1 (1-based CHS)
mov dh, 0               ; head 0
mov dl, [boot_drive]    ; boot drive (stash from MBR's dl, passed to stage2)
int 0x13
jc .err_kernel          ; CF set => fail loud
```

**Note:** The MBR currently discards DL after saving it only to boot_drive
(mbr.asm:55). stage2 does not receive DL from MBR. The MBR's boot_drive
is at 0x7C00 + offset of boot_drive variable; stage2 must either receive DL
from the MBR jump or read the saved byte from the MBR's memory at 0x7C00.
The cleanest fix: MBR passes DL to stage2 via the DL register on the
`jmp STAGE2_SEG:STAGE2_OFF` at mbr.asm:84 (DL is already in DL there, since
it was loaded from [boot_drive] for the INT 13h read just above -- mbr.asm:
77-78). stage2 saves it on entry:

```asm
; stage2 start: save boot drive before clobbering DL
mov [boot_drive_s2], dl
```

This matches BIOS convention (DL = boot drive on entry to any stage of the
boot chain).

---

## 4. Boot Oracle: make test-boot

### 4.1 Serial signal

The kernel entry stub and kernel_main emit serial markers:

```
KERNEL    -- C kernel entered (emitted from kstart.asm or first line of kernel_main)
BANNER    -- banner blit complete (after the two banner lines are rendered)
```

These extend the existing serial protocol (stage2.asm:22-29: S2/VBE/A20/GDT/
PM/LFB/OK). The full expected sequence becomes:
`S1 S2 VBE A20 GDT PM LFB OK KERNEL BANNER`

The harness recipe (extending the test-tracer-boot pattern, Makefile:747-794)
greps the serial capture file for each required marker. A missing KERNEL or
BANNER marker is a hard FAIL.

### 4.2 Screendump pixel assertion

The test-tracer-boot screendump check (ppm_seafoam_check, tools/ppm_seafoam_check.c:1-145)
performs a 9x9 grid sample of the framebuffer and asserts seafoam. For the
boot-to-text gate this check becomes:

**(a) The framebuffer is NO LONGER uniform seafoam.** If the kernel has blitted
even one glyph, at least one sample in the 9x9 grid will deviate from seafoam
color. This is the lightest assertion (exit 1 if ALL 81 samples are still
seafoam -- meaning nothing was drawn). This is a new C tool:
`tools/ppm_text_check.c`, patterned after ppm_seafoam_check.c.

**(b) Specific pixel assertion at the banner origin.** Banner line 1 starts
at (0, 0) or a known margin (e.g., col=0, row=0 = pixel x=0 y=0, or a small
inset; the kernel chooses). In the 640x480 LFB at 32bpp:

- For glyph 'I' (ASCII 0x49) at cell (0,0): pixel (0,0) is the top-left of
  that glyph. The VGA ROM 8x16 font for 'I' has ink in the top row (row 0);
  the foreground color is white (0xFFFFFF) or the chosen console fg color.
  At the known pixel position the sample should be fg-colored, not seafoam.

The lightweight oracle: sample pixel (4, 4) (center of first glyph cell in
32bpp, assuming banner starts at x=0, y=0). If it is the foreground color
(not seafoam), the test passes. If it is still seafoam, the blit did not
happen.

**Implementation recommendation:** a single `ppm_text_check.c` that
(1) verifies at least 1 of the 81 grid samples is NOT seafoam (the "something
was drawn" assertion), and (2) samples the known banner pixel at (4, 4) and
asserts it is the fg color within tolerance. Both assertions must hold. This
is cheap, deterministic, and extension-proof (when more is drawn, the grid
diversity check becomes trivially true).

### 4.3 Make target

```makefile
BOOTTEXT_IMG  := $(BUILD)/boottext.img
BOOTTEXT_NAME := boottext
BOOTTEXT_SERIAL := $(BUILD)/$(BOOTTEXT_NAME).serial
BOOTTEXT_PPM    := $(BUILD)/$(BOOTTEXT_NAME).ppm

test-boot: $(HARNESS_BIN) $(BOOTTEXT_IMG) $(PPM_TEXT_CHECK_BIN)
	# same shell recipe as test-tracer-boot (Makefile:747-794) but:
	# - greps for KERNEL and BANNER in addition to S1/PM/OK
	# - calls ppm_text_check instead of ppm_seafoam_check
```

The harness is already wired for `--disk` boot with screendump capture and
serial capture (Makefile:757-760 pattern). No changes to the harness binary
are needed; only the post-processing assertions change.

---

## 5. Risks and Minefield

### Risk 1 (HIGH): real-mode <1 MiB load constraint + INT 13h ordering

INT 10h (VBE + ROM font) and INT 13h (kernel disk load) MUST all execute
before CR0.PE=1. The ordering in stage2 must be:

```
1. INT 10h VBE (already done: stage2.asm:68)
2. INT 10h ROM font capture  [NEW -- must be added here]
3. INT 13h kernel load       [NEW -- must be added here]
4. A20 enable (port 0x92)    (already done: stage2.asm:74-84)
5. lgdt                      (already done: stage2.asm:88-90)
6. CR0.PE = 1, far jump      (already done: stage2.asm:94-98)
```

Missing either BIOS call before step 6 is a silent failure. If the font
capture is attempted after PM switch, the INT 10h will either triple-fault
(QEMU: silent reboot unless -d int is on) or return garbage (Bochs: logged).
Bochs strict-mode transition checking (CLAUDE.md: "verify in the Bochs
debugger -- strict transition checking, not just QEMU") will catch this.

### Risk 2 (HIGH): bpp mismatch between stage2 fill and the C kernel blitter

stage2 (stage2.asm:166-172) scans modes in list order, accepts the FIRST
mode that is 640x480 LFB AND is either 32bpp or 24bpp (preferred 32 -- the
`cmp al, 32; je .good` test comes first). Under QEMU (SeaBIOS) the VBE mode
list for 640x480 almost universally contains a 32bpp mode first, so
`lfb_bpp` is expected to be 32 (XRGB8888) in the standard path. Under Bochs
and 86Box the mode list order may differ; a 24bpp mode might appear first.

**The C kernel console blitter must branch on boot_info.lfb_bpp (32 vs 24)**
exactly as stage2 already does (stage2.asm:272-290). Using the wrong pixel
packing silently produces garbage colors rather than a hard error. This is a
tri-emulator portability risk (CLAUDE.md Rule 5: "differential/tri-emulator
from day one" -- do not let a fix pass on QEMU alone when Bochs/86Box call
for tri-emulator agreement). The bpp is captured in boot_info.lfb_bpp and
the kernel must use it, not assume 32bpp.

The seafoam screendump test already passes under the actual lfb_bpp because
stage2 branches correctly (test-tracer-boot is green). The console blitter
must follow the same pattern.

### Risk 3 (MEDIUM): stack setup for the C kernel

stage2 sets ESP to 0x00090000 for the 32-bit asm code (stage2.asm:251).
The C kernel entry stub must set up its own stack at a different address
before calling into C. If the kernel's stack overlaps stage2's stack or
the kernel's own .bss, the first C function call will corrupt data.

Recommendation (Sec 3.2 above): set the kernel stack to 0x0008FFFC (just
below stage2's 0x90000 stack, 4-byte aligned). The kernel binary lives at
0x10000 upward; the stack grows down from 0x8FFFC. The gap between the
kernel binary end and 0x8FFFC is available RAM. For the M1 kernel (a few KiB
of code + data) this is ample. As the kernel grows, the stack base should be
moved above the kernel text+data, computed from the linker `end` symbol.

### Additional minefields (from CLAUDE.md / PRD, repeated here for completeness)

- **A20 must be on before any access above 1 MiB.** The LFB is above 1 MiB;
  stage2 enables A20 before the PM switch (stage2.asm:74-84). The C kernel
  inherits this -- do not re-disable it.
- **GDT must remain loaded.** The kernel should load its own GDT early (or
  keep using stage2's flat GDT, which is valid at the kernel's physical
  address since it is a base=0, limit=4GiB descriptor pair).
- **The font stash (FONT_STASH at 0x1000) must be copied before stage2's
  VBE buffers (0x7000-0x73FF) might be clobbered.** In the proposed order
  (Sec 1.3 and Sec 3.3) the font is captured and copied before the kernel
  load; nothing clobbers 0x1000 thereafter.
- **Reproducibility (CLAUDE.md Rule 11):** the kernel binary must not contain
  __DATE__, __TIME__, or host paths. Use `-frandom-seed=` or no randomization
  in codegen; the two-stage self-host certificate (K2==K3) depends on it,
  even though M1 is the C kernel (the C kernel is rebuilt by the factory
  toolchain, not Turbo Initech; but determinism is a project-wide invariant).

---

## 6. Summary Table: Physical Memory Map for M1

| Physical Range        | Owner / Content                                      | Size   |
|-----------------------|------------------------------------------------------|--------|
| 0x00000000-0x000003FF | Real-mode IVT (BIOS)                                 | 1 KiB  |
| 0x00000400-0x000004FF | BIOS Data Area (BDA)                                 | 256 B  |
| 0x00000500-0x00000517 | boot_info_t (24 bytes)                               | 24 B   |
| 0x00001000-0x00001FFF | ROM font stash (4096 bytes, 256 glyphs x 16 rows)    | 4 KiB  |
| 0x00007000-0x000073FF | stage2 VBE buffers (VBE_CTRL_BUF + VBE_MODE_BUF)    | 768 B  |
| 0x00007C00-0x00007DFF | MBR (consumed; stack during real mode)               | 512 B  |
| 0x00008000-0x00009FFF | stage2 binary (16 sectors = 8 KiB)                  | 8 KiB  |
| 0x00010000-0x0007FFFF | Flat C kernel (linked at 0x10000; ~448 KiB available) | <448 K |
| 0x0008FFFC downward   | C kernel stack (grows down; base just below 0x90000) | ~4 KiB |
| 0x00090000            | stage2 stack anchor (stage2.asm:251)                 | --     |
| LFB (0xE0000000 typ.) | VESA linear framebuffer (640x480x32bpp typ.)         | ~1.2 M |

---

*End of document.*
