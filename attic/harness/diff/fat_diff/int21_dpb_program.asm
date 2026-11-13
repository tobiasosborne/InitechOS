; Test-only flat program, not shipped (WL-0167). Drives the real INT 21h
; AH=1Fh / AH=32h in the emulator and prints what came back, one line per
; call, in the format int21_dpb_host.c prints (int21_dpb.sh grades both):
;   DPB <AH> <DL> <AL> <CF> <64 hex digits | - | CLOBBERED> <next | END | ->
; It computes nothing; minfo/mdir are the golden. Ref:
; docs/research/dpb-ground-truth.md Sec 3 (call contract, DOS 3.x layout);
; ADR-0003 Appendix A + B.4; spec/int21h_calling_convention.json 1Fh/32h.
bits 32
org 0x40100
SENTINEL equ 0xDEADBEEF
start:
    mov esi, script
.step:
    movzx eax, byte [esi]           ; op: 0 end, 0Eh select, 1Fh/32h query
    movzx edx, byte [esi+1]         ; DL
    add esi, 2
    test eax, eax
    jz .done
    cmp al, 0x0E
    jne .query
    mov eax, 0x0E00
    int 0x21
    jmp .step
.query:
    mov [cur_ah], al
    mov [cur_dl], dl
    shl eax, 8
    mov ebx, SENTINEL
    int 0x21
    setc cl
    mov [res_al], al
    mov [res_cf], cl
    mov [res_ebx], ebx
    push esi
    mov edi, msg_dpb
    call puts
    mov al, [cur_ah]
    call hex2
    call space
    mov al, [cur_dl]
    call hex2
    call space
    mov al, [res_al]
    call hex2
    call space
    mov al, [res_cf]
    add al, '0'
    call putc
    call space
    cmp byte [res_al], 0
    je .valid
    mov edi, msg_dash
    cmp dword [res_ebx], SENTINEL
    je .inv
    mov edi, msg_clob
.inv:
    call puts
    mov edi, msg_dash_tail
    call puts
    jmp .eol
.valid:
    mov ebx, [res_ebx]
    xor ecx, ecx
.byte:
    mov al, [ebx+ecx]
    call hex2
    inc ecx
    cmp ecx, 32
    jb .byte
    call space
    mov edx, [ebx+0x18]
    cmp edx, 0xFFFFFFFF
    jne .link
    mov edi, msg_end
    call puts
    jmp .eol
.link:
    mov al, [edx]
    add al, 'A'
    call putc
.eol:
    mov edi, msg_crlf
    call puts
    pop esi
    jmp .step
.done:
    mov eax, 0x4c00
    int 0x21
    hlt

; putc: AL -> stdout (AH=02h). Preserves every register.
putc:
    pushad
    mov dl, al
    mov eax, 0x0200
    int 0x21
    popad
    ret
space:
    push eax
    mov al, ' '
    call putc
    pop eax
    ret
; puts: NUL-terminated string at EDI.
puts:
    push eax
    push edi
.p:
    mov al, [edi]
    test al, al
    jz .q
    call putc
    inc edi
    jmp .p
.q:
    pop edi
    pop eax
    ret
; hex2: AL as two uppercase hex digits.
hex2:
    push eax
    push eax
    shr al, 4
    call nib
    pop eax
    and al, 0x0F
    call nib
    pop eax
    ret
nib:
    add al, '0'
    cmp al, '9'
    jbe .o
    add al, 7
.o:
    jmp putc

; Same order as int21_dpb_host.c main() and int21_dpb.sh EXPECT.
script:
    db 0x0E,0, 0x1F,0, 0x32,0, 0x32,1, 0x32,2, 0x32,3, 0x32,4, 0x32,5
    db 0x32,0x1B
    db 0x0E,2, 0x1F,0, 0x32,0, 0x0E,0, 0, 0
msg_dpb: db 'DPB ',0
msg_dash: db '-',0
msg_clob: db 'CLOBBERED',0
msg_dash_tail: db ' -',0
msg_end: db 'END',0
msg_crlf: db 13,10,0
cur_ah: db 0
cur_dl: db 0
res_al: db 0
res_cf: db 0
res_ebx: dd 0
