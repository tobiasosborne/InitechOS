; ANSI console regression probe, test fixture only (not on the shipped volume).
; Ref: ADR-0003 DEC-08 flat COM, Appendix A INT 21h 06h/07h/09h/0Bh/4Ch;
; IBM DOS 3.30 Technical Reference printed pp. 3-3/3-4, 3-7/3-8,
; 3-10/3-11 (CPR via stdin; example reads two row and two column digits),
; 3-12 (save/restore), 3-15 (conceal). Expected reports are independent data.
; Same FAT-loaded NASM flat-image pattern as find_program.asm.
bits 32
org 0x00040100

start:
    mov ebx, cases
.case:
    mov edx, [ebx]
    test edx, edx
    jz .done
    mov eax, 0x0900
    int 0x21
    mov esi, [ebx + 4]
    mov edi, 8
    xor ebp, ebp
.read:
    mov eax, 0x0b00
    int 0x21
    cmp al, 0xff
    jne .fail
    mov eax, 0x0700
    int 0x21
    cmp al, [esi]
    je .next_byte
    ; Drain the complete report even on mismatch, so a mutant cannot leave
    ; response bytes in front of the injected EXIT at the shell prompt.
    mov ebp, 1
.next_byte:
    inc esi
    dec edi
    jnz .read
    ; Ensure there are no trailing bytes or duplicate responses.
    mov eax, 0x0600
    mov edx, 0xff
    int 0x21
    jnz .fail
    test ebp, ebp
    jnz .fail
    mov edx, [ebx + 8]
    mov eax, 0x0900
    int 0x21
    add ebx, 12
    jmp .case
.fail:
    mov edx, failed
    mov eax, 0x0900
    int 0x21
    mov eax, 0x4c01
    int 0x21
.done:
    mov edx, passed
    mov eax, 0x0900
    int 0x21
    mov eax, 0x4c00
    int 0x21
    jmp $

%macro probe 4
seq_%1: db 27, '[10;20H', %2, 27, '[6n', '$'
rep_%1: db 27, '[', %3, 'R'
msg_%1: db 13, 10, 'ANSI-PROBE PASS ', %4, 13, 10, '$'
%endmacro
probe up,    {27,'[3A'}, '07;20', 'CUU'
probe down,  {27,'[3B'}, '13;20', 'CUD'
probe right, {27,'[3C'}, '10;23', 'CUF'
probe left,  {27,'[3D'}, '10;17', 'CUB'
probe za, {27,'[0A'}, '09;20', 'ZERO-A'
probe zb, {27,'[0B'}, '11;20', 'ZERO-B'
probe zc, {27,'[0C'}, '10;21', 'ZERO-C'
probe zd, {27,'[0D'}, '10;19', 'ZERO-D'
probe ha, {27,'[3000000000A'}, '01;20', 'HUGE-A'
probe hb, {27,'[3000000000B'}, '25;20', 'HUGE-B'
probe hc, {27,'[3000000000C'}, '10;80', 'HUGE-C'
probe hd, {27,'[3000000000D'}, '10;01', 'HUGE-D'
probe mix, {27,'[4;7H',27,'[3B',27,'[5C',27,'[2A',27,'[4D',27,'[s',27,'[H',27,'[u'}, '05;08', 'SAVE-REL'
probe live, {27,'[Habc',13,10,'xy'}, '02;03', 'LIVE'
probe wrap, {27,'[1;80HX'}, '02;01', 'WRAP'

%macro entry 1
    dd seq_%1, rep_%1, msg_%1
%endmacro
align 4
cases:
    entry up
    entry down
    entry right
    entry left
    entry za
    entry zb
    entry zc
    entry zd
    entry ha
    entry hb
    entry hc
    entry hd
    entry mix
    entry live
    entry wrap
    dd 0
failed: db 13,10,'ANSI-PROBE FAIL DSR',13,10,'$'
passed: db 'ANSI-PROBE GREEN',13,10,'$'
