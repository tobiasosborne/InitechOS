; Ref: ADR-0003 DEC-08; spec/memory_map.h; samir_crt0.asm precedent.
bits 32
global _start
extern __bss_start, __bss_end, chkdsk_entry
section .text._start
_start:
    cld
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb
    call chkdsk_entry
    mov ah, 0x4c
    int 0x21
    int 0x20
section .note.GNU-stack noalloc noexec nowrite progbits
