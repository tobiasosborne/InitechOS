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
    mov ebp, [esp+20]
    mov eax, [ebp]
    mov ebx, [ebp+4]
    mov ecx, [ebp+8]
    mov edx, [ebp+12]
    mov esi, [ebp+16]
    mov edi, esi                 ; AH=47 uses ESI; AH=56 uses EDI
    int 0x21
    pushfd
    mov [ebp], eax
    mov [ebp+4], ebx
    mov [ebp+8], ecx
    mov [ebp+12], edx
    mov [ebp+16], esi
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
