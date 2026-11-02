; Independent DOS 3.30 capture; never executed as InitechDOS artifact code.
; Ref: adjacent README.txt; IBM DOS 3.30 TR pp.6-196/197,11-4/5.
bits 16
org 0x100
start:
    mov ax,cs
    mov ds,ax
    mov es,ax
    cli
    mov sp,4096
    sti
    mov bx,256
    mov ah,0x4a
    int 0x21
    jc failed
    mov bx,64
    mov ah,0x48
    int 0x21
    jc failed
    mov [result+4],ax
    mov [block],ax
    mov bx,64
    mov ah,0x48
    int 0x21
    jc failed
    mov [result+6],ax
%ifdef EQUAL_FACTOR
    mov dx,[result+4]
    mov [block+2],dx
%else
    mov [block+2],ax
%endif
    mov es,ax
    mov word [es:0],0x1111
    mov ax,cs
    mov es,ax
    mov bx,block
    mov dx,name
    mov ax,0x4b03
    int 0x21
    mov [cs:result+2],ax
    pushf
    pop ax
    and ax,1
    mov [cs:result],ax
    mov ax,cs
    mov ds,ax
    mov es,[result+4]
    mov ax,[es:0]
    mov [result+8],ax
    mov es,[result+6]
    mov ax,[es:0]
    mov [result+10],ax
    xor cx,cx
    mov dx,report
    mov ah,0x3c
    int 0x21
    jc failed
    mov bx,ax
    mov dx,result
    mov cx,12
    mov ah,0x40
    int 0x21
    mov ah,0x3e
    int 0x21
    mov ax,0x4c00
    int 0x21
failed:
    mov ax,0x4c01
    int 0x21
name: db 'B:\PROBE.OVL',0
report: db 'A:\RESULT.BIN',0
block: dw 0,0
result: times 6 dw 0
