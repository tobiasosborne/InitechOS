; Test only: redirect standard error independently of COMMAND's < and >.
; Execute FIND with the remaining PSP tail and return its AH=4Dh status.
bits 16
org 0x100

    mov sp, stack_top
    mov bx, (stack_top - $$ + 0x100 + 15) / 16
    mov ah, 0x4a
    int 0x21
    jc failed
    mov [tail_segment], ds
    mov [fcb1_segment], ds
    mov [fcb2_segment], ds
    cld
    mov si, 0x81
.skip_spaces:
    lodsb
    cmp al, ' '
    je .skip_spaces
    dec si
    mov di, stderr_name
.copy_name:
    lodsb
    cmp al, 0x0d
    je .copy_tail
    cmp al, ' '
    je .copy_tail
    stosb
    jmp .copy_name
.copy_tail:
    dec si
    mov byte [di], '.'
    mov byte [di+1], 'E'
    mov byte [di+2], 'R'
    mov byte [di+3], 'R'
    mov di, tail + 1
.copy_byte:
    lodsb
    stosb
    cmp al, 0x0d
    jne .copy_byte
    sub di, tail + 2
    mov ax, di
    mov [tail], al

    mov ax, 0x3c00
    xor cx, cx
    mov dx, stderr_name
    int 0x21
    jc failed
    mov bx, ax
    mov cx, 2
    mov ah, 0x46
    int 0x21
    jc failed
    mov ah, 0x3e
    int 0x21
    jc failed
    mov ax, 0x4b00
    mov bx, exec
    mov dx, program
    int 0x21
    jc failed
    mov ah, 0x4d
    int 0x21
    mov ah, 0x4c
    int 0x21
failed:
    mov ax, 0x4c7f
    int 0x21

program db 'A:\FIND.EXE', 0
exec dw 0, tail
tail_segment dw 0
dw 0x5c
fcb1_segment dw 0
dw 0x6c
fcb2_segment dw 0
stderr_name times 13 db 0
tail times 129 db 0
times 256 db 0
stack_top:
