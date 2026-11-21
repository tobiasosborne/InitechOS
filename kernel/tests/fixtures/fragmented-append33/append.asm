; Genuine MS-DOS 3.30 probe. Append one byte to the mtools-minted B: file.
bits 16
org 100h

    mov ah,30h
    int 21h
    cmp ax,1e03h
    jne failed

    mov ax,3d02h
    mov dx,filename
    int 21h
    jc failed
    mov bx,ax
    mov ax,4202h
    xor cx,cx
    xor dx,dx
    int 21h
    jc failed
    cmp ax,512
    jne failed
    test dx,dx
    jnz failed

    mov ah,40h
    mov cx,1
    mov dx,payload
    int 21h
    jc failed
    cmp ax,1
    jne failed

    mov ah,3eh
    int 21h
    jc failed
    mov al,10h
    jmp finish
failed:
    mov al,11h
finish:
    out 0f4h,al
    jmp $

filename db 'B:TARGET.BIN',0
payload db 'X'
