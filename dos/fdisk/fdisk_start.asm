; DEC-08 / DEC-04a flat COM; spec/memory_map.h PROGRAM_IMAGE.
bits 32
global _start
extern __bss_start, __bss_end, fdisk_entry
section .text._start
_start:
    cld
    mov edi,__bss_start
    mov ecx,__bss_end
    sub ecx,edi
    xor eax,eax
    rep stosb
    ; PSP 80h holds the command-tail length. Pass the bounded tail to C.
    movzx eax,byte [0x40080]
    push eax
    push dword 0x40081
    call fdisk_entry
    add esp,8
    mov ah,0x4c
    int 0x21
    int 0x20
section .note.GNU-stack noalloc noexec nowrite progbits
