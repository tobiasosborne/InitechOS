; ADR-0003 DEC-08 flat COM; spec/memory_map.h.
bits 32
global _start
extern __bss_start, __bss_end, diskutil_entry
section .text._start
_start:
 cld
 mov edi,__bss_start
 mov ecx,__bss_end
 sub ecx,edi
 xor eax,eax
 rep stosb
 call diskutil_entry
 mov ah,0x4c
 int 0x21
 int 0x20
section .note.GNU-stack noalloc noexec nowrite progbits
