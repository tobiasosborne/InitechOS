; Flat .COM-equivalent C entry / register bridge. Ref: ADR-0003 DEC-08,
; Appendix A, DEC-04a; spec/memory_map.h; loader's EBX = current PSP.
bits 32
section .entry
global start
extern du_main, __bss_start, __bss_end
start:
    cld
    mov ebp, ebx
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb
    movzx ecx, byte [ebp+0x80]
    lea esi, [ebp+0x81]
    mov edi, tail
    rep movsb
    and esp, -16
    sub esp, 8
    push tail
%ifdef TREE_PROGRAM
    push dword 1
%else
    push dword 0
%endif
    call du_main
    and eax, 255
    or eax, 0x4c00
    int 0x21
    int 0x20

section .text
global du_call
du_call:
    push ebp
    push ebx
    push esi
    push edi
    mov edi, [esp+20]
    mov eax, [edi]
    mov ebx, [edi+4]
    mov ecx, [edi+8]
    mov edx, [edi+12]
    mov esi, [edi+16]
    int 0x21
    pushfd
    mov [edi], eax
    mov [edi+4], ebx
    mov [edi+8], ecx
    mov [edi+12], edx
    mov [edi+16], esi
    pop eax
    and eax, 1
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
section .bss
tail: resb 128
section .note.GNU-stack noalloc noexec nowrite progbits
