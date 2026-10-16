; Factory-only flat test program, never installed on the shipped volume.
; Ref: IBM DOS 3.30 TR pp6-24..25; ADR-0003 DEC-15 (flat ABI, balanced stack).
; Read logical boot sector, assert BPB, rewrite it unchanged, then round-trip
; the last FREE data sector. The fixture has only a few allocated files.
bits 32
org 0x40100
start:
    xor eax,eax
    mov ebx,0x50000
    mov ecx,1
    xor edx,edx
    int 0x25
    jc fail
    cmp word [0x5000b],512
    jne fail
    cmp word [0x501fe],0xaa55
    jne fail
    mov eax,[0x5001c]
    cmp eax,0
    je .hidden_ok
    cmp eax,63
    jne fail
.hidden_ok:
    movzx eax,word [0x50013]
    dec eax
    mov [last],eax
    xor eax,eax
    mov ebx,0x50000
    mov ecx,1
    xor edx,edx
    int 0x26
    jc fail
    mov edi,0x50200
    mov ecx,128
    mov eax,0x73667a6b
    cld
    rep stosd
    xor eax,eax
    mov ebx,0x50200
    mov ecx,1
    mov edx,[last]
    int 0x26
    jc fail
    xor eax,eax
    mov ebx,0x50400
    mov ecx,1
    mov edx,[last]
    int 0x25
    jc fail
    mov esi,0x50200
    mov edi,0x50400
    mov ecx,512
    repe cmpsb
    jne fail
    mov edx,ok
    mov eax,0x0900
    int 0x21
    mov eax,0x4c00
    int 0x21
fail:
    mov edx,bad
    mov eax,0x0900
    int 0x21
    mov eax,0x4c01
    int 0x21
last: dd 0
ok: db 'PART-ABS-OK',13,10,'$'
bad: db 'PART-ABS-FAIL',13,10,'$'
