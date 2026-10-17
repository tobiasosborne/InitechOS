; Factory-only baked EXEC probe: single-level flat region must reject nesting.
; ADR-0003 DEC-08, Appendix A 4Bh/4Ch; IBM DOS 3.30 TR p. 6-195 (CF/AX).
bits 32
org 0x40100
    mov eax,0x4b00
    xor ebx,ebx
    mov edx,name
    int 0x21
    jnc fail
    cmp ax,8
    jne fail
    mov edx,ok
    mov eax,0x0900
    int 0x21
    mov eax,0x4c07
    int 0x21
fail:
    mov edx,bad
    mov eax,0x0900
    int 0x21
    mov eax,0x4c01
    int 0x21
name db 'GREET.COM',0
ok db 'KHY-BAKED-BUSY-OK',13,10,'$'
bad db 'KHY-BAKED-FAIL',13,10,'$'
