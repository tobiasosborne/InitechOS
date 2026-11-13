; Factory-only baked EXEC probe: resident caller must survive nesting.
; ADR-0003 DEC-08; IBM DOS 3.30 TR pp. 6-196/200/201 (caller and exit code).
bits 32
org 0x40100
    push ebx
    mov eax,0x4b00
    xor ebx,ebx
    mov edx,name
    int 0x21
    jc fail
    mov eax,0x4d00
    int 0x21
    cmp ax,7
    jne fail
    mov eax,0x6200
    int 0x21
    mov edx,[esp]
    shr edx,4
    cmp bx,dx
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
ok db 'KHY-BAKED-NESTED-OK',13,10,'$'
bad db 'KHY-BAKED-FAIL',13,10,'$'
