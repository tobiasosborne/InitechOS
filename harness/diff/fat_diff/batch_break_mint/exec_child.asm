; Same DOS-call sequence for real DOS 3.30 and the flat InitechDOS ABI.
; An ordinary return from AH=08h is a failure: Ctrl-C must terminate us.
%ifdef REAL_DOS
bits 16
org 0x100
%define AXREG ax
%define DXREG dx
%define BXREG bx
%define CXREG cx
%else
bits 32
org 0x40100
%define AXREG eax
%define DXREG edx
%define BXREG ebx
%define CXREG ecx
%endif
    mov AXREG,0x0900
    mov DXREG,ready
    int 0x21
    ; Bind the child's stdin to CON so the redirected parent input stays
    ; untouched, and leave this handle for the child's terminate cleanup.
    mov AXREG,0x3d00
    mov DXREG,console
    int 0x21
    mov BXREG,AXREG
    mov CXREG,0
    mov AXREG,0x4600
    int 0x21
    mov AXREG,0x0800
    int 0x21
    mov AXREG,0x0900
    mov DXREG,survived
    int 0x21
    mov AXREG,0x4c01
    int 0x21
ready: db 'EXEC-CHILD-READY',13,10,'$'
survived: db 'EXEC-CHILD-SURVIVED',13,10,'$'
console: db 'CON',0
