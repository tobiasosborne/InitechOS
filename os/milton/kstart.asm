; kstart.asm -- InitechDOS flat C kernel entry stub (32-bit protected/flat).
;
; beads: initech-d00 ("stage2 -> C kernel handoff").
; Ref:   PRD Sec 5 (flat binary kernel handed off by stage2; ADR-0003 DEC-08);
;        docs/research/boot-to-text-ground-truth.md Sec 3.2 (entry stub) +
;        docs/design/kernel-runway-relocation.md K2 (bead initech-tdnl.29:
;        deterministic post-image runway zero). The design's proposed high
;        stack [0x4F0000,0x500000) was stopped by Step 0.3 (it overlaps the
;        locked FLAIR heap); bead initech-xu97 ratified the disjoint home
;        [0x5F0000,0x600000) instead (docs/adr/ADR-0003-AMENDMENT-DEC-08b-
;        Kernel-Stack-Window.md). CLAUDE.md Rule 2 (fail loud), Rule 12 (ASCII).
;
; stage2 far-jumps here (CODE_SEL:0x00500000) in 32-bit flat protected mode:
; CR0.PE=1, A20 on, flat GDT loaded, DS/ES/FS/GS/SS = DATA_SEL. We are linked
; FIRST in the flat binary so physical KERNEL_BASE == _start (kernel.ld).
;
; We set our OWN stack (stage2's ESP=0x90000 region is now abandoned) and call
; the C entry. kernel_main never returns; the hlt-loop is a fail-loud guard.

bits 32

; Duplicated from spec/memory_map.h because NASM cannot include C headers.
; Ref: kernel-runway-relocation.md K1/K2; bead initech-tdnl.29.
KERNEL_BASE       equ 0x00500000
KERNEL_RUNWAY     equ 0x00100000
KERNEL_CEIL       equ 0x00600000
KERNEL_STACK_BOT  equ 0x005F0000
KERNEL_STACK_TOP  equ 0x005FFFFC

; The stack equates are a duplicate of spec/memory_map.h, so they carry the
; same drift guard as KERNEL_SECTORS below: the Makefile parses the header's
; KERNEL_STACK_BOT/TOP and passes them as MK_KERNEL_STACK_BOT/TOP; any
; disagreement is an assembly error, never a silently-misplaced ESP
; (bead initech-xu97; CLAUDE.md Rule 2).
%ifndef MK_KERNEL_STACK_TOP
  %error "MK_KERNEL_STACK_TOP not defined: kstart.asm must be assembled by the Makefile rule so its stack equates can be checked against spec/memory_map.h."
%elif MK_KERNEL_STACK_TOP != KERNEL_STACK_TOP
  %error "KERNEL_STACK_TOP MISMATCH: spec/memory_map.h (MK_KERNEL_STACK_TOP) differs from this kstart.asm equate -- move BOTH."
%endif
%ifndef MK_KERNEL_STACK_BOT
  %error "MK_KERNEL_STACK_BOT not defined: kstart.asm must be assembled by the Makefile rule so its stack equates can be checked against spec/memory_map.h."
%elif MK_KERNEL_STACK_BOT != KERNEL_STACK_BOT
  %error "KERNEL_STACK_BOT MISMATCH: spec/memory_map.h (MK_KERNEL_STACK_BOT) differs from this kstart.asm equate -- move BOTH."
%endif

; ---------------------------------------------------------------------------
; KERNEL_SECTORS -- the padded disk window, and THE THIRD COPY OF IT.
;
; This value is load-bearing in a way that is easy to miss: the stosd below
; starts zeroing at KERNEL_BASE + KERNEL_SECTORS*512, so a value SMALLER than
; the Makefile's does not merely under-zero the runway -- it ZEROES THE TOP OF
; THE KERNEL'S OWN LOADED IMAGE, in place, before kernel_main runs.
;
; That is exactly what happened. Bead initech-uzjc bumped KERNEL_SECTORS
; 352 -> 384 and mechanised the Makefile <-> stage2.asm pairing (stage2 gets
; -DMK_KERNEL_SECTORS and %errors on a mismatch) -- but THIS third copy was
; missed and stayed at 352. The bug was LATENT only because the kernel image
; was still smaller than 352 sectors: the wiped range 0x52C000.. held nothing
; but padding. Bead initech-tdnl.10 grew the image past that line and the
; kernel started erasing its own .rodata tail (the symptom: serial_puthex32
; printing NULs because its digit table read as zero, and the second banner
; line printing garbage) with no fault and no marker -- the quietly-wrong
; failure Rule 2 exists to make impossible.
;
; ROOT-CAUSE FIX (Rule 3, and the uzjc mechanisation finished): the Makefile
; passes its KERNEL_SECTORS as MK_KERNEL_SECTORS and this file %errors if the
; two disagree -- the SAME guard stage2.asm carries, now on all three copies,
; so the constant can never drift silently again.
; ---------------------------------------------------------------------------
KERNEL_SECTORS    equ 384

%ifndef MK_KERNEL_SECTORS
  %error "MK_KERNEL_SECTORS not defined: kstart.asm must be assembled by the Makefile rule so its KERNEL_SECTORS equate can be checked against the Makefile's KERNEL_SECTORS."
%elif MK_KERNEL_SECTORS != KERNEL_SECTORS
  %error "KERNEL_SECTORS MISMATCH: the Makefile's KERNEL_SECTORS (MK_KERNEL_SECTORS) differs from this kstart.asm equate. Bump BOTH -- the Makefile pads the kernel .bin to its value and THIS file starts the runway zeroing just past it; a stale value here erases the top of the loaded kernel."
%endif

global _start
extern kernel_main

_start:
    ; Kernel-private 64 KiB stack [0x5F0000,0x600000): the top of the RAM-gated
    ; kernel runway, above the transient COMMAND.COM window and disjoint from
    ; the FLAIR heap, the kernel image, the program window and the staging
    ; window (spec/memory_map.h WINDOW DISJOINTNESS asserts; bead initech-xu97).
    ; stage2's transient ESP=0x90000 stack is abandoned at this handoff. The
    ; runway zeroing below covers this stack too; it runs BEFORE the first push,
    ; so no live stack slot is ever cleared.
    mov esp, KERNEL_STACK_TOP
    xor ebp, ebp                ; null frame pointer => top of stack to debugger

    ; Zero the post-image runway slack [KERNEL_BASE + padded disk window,
    ; KERNEL_CEIL). stage2 copied the entire padded KERNEL_SECTORS window, whose
    ; deterministic zero padding covers objcopy's dropped NOBITS/.bss bytes;
    ; this stosd completes the remaining runway. Together they make BSS and all
    ; post-image slack zero on real hardware, independent of emulator RAM init.
    ; Ref: kernel-runway-relocation.md K2; bead initech-tdnl.29.
    cld
    mov edi, KERNEL_BASE + (KERNEL_SECTORS * 512)
    mov ecx, (KERNEL_RUNWAY - (KERNEL_SECTORS * 512)) / 4
    xor eax, eax
    rep stosd

    push ebp
    call kernel_main            ; void kernel_main(void) -- never returns

.hang:
    hlt
    jmp .hang

; Mark the stack non-executable (silences the ld GNU-stack warning). Harmless
; for a flat binary; good hygiene. ASCII-only (Rule 12).
section .note.GNU-stack noalloc noexec nowrite progbits
