; Independent DOS program that reads the kernel flag after COMMAND sets it.
bits TARGET_BITS
%if TARGET_BITS = 16
org 0x0100
%define message_reg dx
%else
org 0x40100
%define message_reg edx
%endif
    mov ax, 0x5400
    clc
    int 0x21
    jc unavailable
    test al, al
    jz off
    mov message_reg, flag_on
    jmp print
off:
    mov message_reg, flag_off
    jmp print
unavailable:
    mov message_reg, no_service
print:
    mov ah, 0x09
    int 0x21
    mov ax, 0x4c00
    int 0x21
flag_on: db 'VERIFY-GATE-ON', 13, 10, '$'
flag_off: db 'VERIFY-GATE-OFF', 13, 10, '$'
no_service: db 'VERIFY-GATE-UNAVAILABLE', 13, 10, '$'
