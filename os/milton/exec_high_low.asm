; DEC-03a conventional nested child; PIC, remains on its low MCB stack.
bits 32
org 0
    cmp ebx, 0x100000
    jae fail
    cmp esp, 0x100000
    jae fail
    mov ax,0x4c00
    int 0x21
fail:
    mov ax,0x4c01
    int 0x21
