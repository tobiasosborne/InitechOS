; DOS 3.30 IOCTL AX=4400h device information words. RES.BIN gets the DOS
; version, then for each case the carry flag byte and DX (little endian):
; inherited handles 0-4, then opened CON, NUL, AUX, PRN, CLOCK$ and the
; file B:FILE.TXT, each closed after the query. A failed open records EEh.
bits 16
org 100h

    mov ah,30h
    int 21h
    mov [results],ax
    mov word [count],2
    xor bx,bx
.std:
    push bx
    call query
    pop bx
    inc bx
    cmp bx,5
    jb .std
    mov si,names
.open:
    cmp byte [si],0
    je finish
    mov dx,si
    mov ax,3d02h
    int 21h
    jnc .opened
    mov ax,3d00h
    int 21h
    jnc .opened
    mov bx,[count]
    mov byte [results+bx],0eeh
    add word [count],3
    jmp .skip
.opened:
    mov bx,ax
    push bx
    call query
    pop bx
    mov ah,3eh
    int 21h
.skip:
    lodsb
    or al,al
    jnz .skip
    jmp .open

query:
    mov ax,4400h
    xor dx,dx
    int 21h
    mov bx,[count]
    mov byte [results+bx],0
    adc byte [results+bx],0
    mov [results+bx+1],dx
    add word [count],3
    ret

finish:
    mov ah,3ch
    xor cx,cx
    mov dx,res_name
    int 21h
    jc .out
    mov bx,ax
    mov ah,40h
    mov cx,[count]
    mov dx,results
    int 21h
    mov ah,3eh
    int 21h
.out:
    mov al,10h
    out 0f4h,al
    jmp $

names db 'CON',0,'NUL',0,'AUX',0,'PRN',0,'CLOCK$',0,'B:FILE.TXT',0,0
res_name db 'B:RES.BIN',0
count dw 0
results times 64 db 0
