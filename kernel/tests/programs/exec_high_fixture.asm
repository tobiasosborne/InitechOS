; Independent DEC-03a.2-.5 oracle. Literal header, addresses and ABI; no
; artifact constants or packer. One nonzero flat fixup, entry 7, >old window.
bits 32
org 0
db 'MZ'
dw ((file_end-$$) % 512), ((file_end-$$) + 511) / 512, 1, 4, 2058, 0xffff
dw 0,0,0,7,0,0x38,0,0x5849,0
dd 1,24,65536,32912,65536,4096
dd 8
align 16, db 0
module:
times 7 db 0xcc
start:
    mov edx, marker - module     ; reloc site 8, intentionally nonzero
    call .base
.base:
    pop esi
    sub esi, .base - module
    cmp esi, 0x600000
    jne fail
    lea eax, [esi + marker - module]
    cmp edx, eax
    jne fail
    cmp ebx, 0x100000
    jae fail
    cmp esp, 0x7f0000
    jb fail
    cmp esp, 0x800000
    jae fail
    mov edi, ebx
    cmp word [edi+2], 0xffff     ; alloc-end must describe LOW controls
    je fail
    ; Arbitrary RAM must be cleared in reserve, free heap and high stack.
    cmp dword [esi + 200000], 0
    jne fail
    cmp dword [0x7ff000], 0
    jne fail
    ; >1 MiB request, tests EBX count and EAX pointer without truncation.
    mov ah, 0x48
    mov ebx, 0x10001
    int 0x21
    jc fail
    cmp eax, 0x600000
    jb fail
    cmp eax, 0x6eefe0
    ja fail
    mov ebp, eax
    cmp dword [ebp], 0
    jne fail
    mov dword [ebp], 0x1234abcd
    mov dword [ebp+0x100000], 0x5a16c0de
    ; shrink then grow into the adjacent free tail, preserving bytes
    mov ah, 0x4a
    mov ebx, ebp
    mov ecx, 0x10000
    int 0x21
    jc fail
    mov ah, 0x4a
    mov ebx, ebp
    mov ecx, 0x10001
    int 0x21
    jc fail
    cmp dword [ebp], 0x1234abcd
    jne fail
    ; Both KEEP entry points MUST return invalid function and leave us live.
    mov ax, 0x3109
    mov edx, 16
    int 0x21
    jnc fail
    cmp ax, 1
    jne fail
    mov edx, 256
    int 0x27
    jnc fail
    cmp ax, 1
    jne fail
    ; A second high EXEC is refused before touching module or heap.
    lea edx, [esi + high_name - module]
    lea ebx, [esi + params - module]
    mov ax, 0x4b00
    int 0x21
    jnc fail
    cmp ax, 8
    jne fail
    cmp dword [ebp], 0x1234abcd
    jne fail
    ; Conventional PIC child uses the retained low chain and returns.
    lea edx, [esi + low_name - module]
    lea ebx, [esi + params - module]
    mov ax, 0x4b00
    int 0x21
    jc fail
    ; Parent allocator and its owned block survived the child.
    mov ebx, ebp
    mov ah, 0x49
    int 0x21
    jc fail
    mov ebx, ebp
    mov ah, 0x49
    int 0x21
    jnc fail
    cmp ax, 9
    jne fail
    ; Exhaustion returns a full-width (>65535) largest count.
    mov ah, 0x48
    mov ebx, 0xffffffff
    int 0x21
    jnc fail
    cmp ax, 8
    jne fail
    cmp ebx, 0x10000
    jbe fail
    ; Leave a live block for process teardown/reclaim, then execute again.
    mov ah, 0x48
    mov ebx, 0x10001
    int 0x21
    jc fail
    lea edx, [esi + marker - module]
    mov ah, 9
    int 0x21
%ifdef HIGH_EXIT_00
    xor eax,eax
    int 0x21
%elifdef HIGH_EXIT_20
    int 0x20
%else
    mov ax, 0x4c09
    int 0x21
%endif
fail:
    mov ax, 0x4c01
    int 0x21
marker: db 'EXEC-HIGH-OK',13,10,'$'
high_name: db 'HIGH.EXE',0
low_name: db 'LOW.COM',0
params: times 24 db 0
%ifdef HIGH_OVERSIZE
    times 262145-($-module) db 0x5a
%else
    times 200000-($-module) db 0x5a
%endif
file_end:
