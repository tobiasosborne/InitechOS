; kstart.asm -- InitechDOS flat C kernel entry stub (32-bit protected/flat).
;
; beads: initech-d00 ("stage2 -> C kernel handoff").
; Ref:   PRD Sec 5 (flat binary kernel handed off by stage2; ADR-0003 DEC-08);
;        docs/research/boot-to-text-ground-truth.md Sec 3.2 (entry stub) +
;        Sec 5 Risk 3 (kernel stack must NOT overlap stage2's 0x90000 stack
;        nor the kernel .bss). CLAUDE.md Rule 2 (fail loud), Rule 12 (ASCII).
;
; stage2 far-jumps here (CODE_SEL:0x00010000) in 32-bit flat protected mode:
; CR0.PE=1, A20 on, flat GDT loaded, DS/ES/FS/GS/SS = DATA_SEL. We are linked
; FIRST in the flat binary so physical 0x00010000 == _start (kernel.ld).
;
; We set our OWN stack (stage2's ESP=0x90000 region is now abandoned) and call
; the C entry. kernel_main never returns; the hlt-loop is a fail-loud guard.

bits 32

global _start
extern kernel_main

_start:
    ; Kernel stack: 0x0008FFFC, 4-byte aligned, below the abandoned stage2
    ; stack anchor at 0x90000 and well above the kernel image (linked at
    ; 0x10000, a few KiB). Grows down into free conventional RAM.
    mov esp, 0x0008FFFC
    xor ebp, ebp                ; null frame pointer => top of stack to debugger
    push ebp
    call kernel_main            ; void kernel_main(void) -- never returns

.hang:
    hlt
    jmp .hang

; Mark the stack non-executable (silences the ld GNU-stack warning). Harmless
; for a flat binary; good hygiene. ASCII-only (Rule 12).
section .note.GNU-stack noalloc noexec nowrite progbits
