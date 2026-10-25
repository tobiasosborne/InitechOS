; Factory oracle only. BIOS INT 13h changes one private raw-disk sector.
; Ref: PRD 8 (independent disk bytes), CLAUDE Rule 6.
bits 16
org 0x7c00
    xor ax,ax
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov sp,0x7c00
    mov [drive],dl
    mov bx,0x8000
    mov ax,0x0201
    mov cx,2
    xor dh,dh
    int 0x13
    jc halt
    xor byte [0x8000],0x5a
    mov ax,0x0301
    mov dl,[drive]
    int 0x13
    jc halt
    mov si,marker
    mov dx,0x3f8
print:
    lodsb
    test al,al
    jz finish
    out dx,al
    jmp print
finish:
%ifdef EXIT_IMMEDIATE
    mov dx,0xf4
    xor al,al
    out dx,al
%endif
halt:
    cli
    hlt
    jmp halt
drive db 0
marker db 'BOOT-WRITE-DONE',10,0
times 510-($-$$) db 0
dw 0xaa55
