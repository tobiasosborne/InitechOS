; Genuine DOS handle-search attribute matrix; no InitechDOS code used.
bits 16
org 100h
    mov dx, dta
    mov ah, 1ah
    int 21h
    xor bp, bp
    mov di, results
next_mask:
    mov byte [exact+4], '0'
    mov byte [wild+4], '0'
next_file:
    xor cx,cx
    mov cl,[masks+bp]
    mov dx,exact
    mov ah,4eh
    int 21h
    pushf
    pop ax
    and al,1
    stosb
    xor cx,cx
    mov cl,[masks+bp]
    mov dx,wild
    mov ah,4eh
    int 21h
    pushf
    pop ax
    and al,1
    stosb
    inc byte [exact+4]
    inc byte [wild+4]
    cmp byte [exact+4], '7'
    jb next_file
    inc bp
    cmp bp,64
    jb next_mask
    mov dx,outfile
    xor cx,cx
    mov ah,3ch
    int 21h
    jc failed
    mov bx,ax
    mov dx,results
    mov cx,896
    mov ah,40h
    int 21h
    jc failed
    cmp ax,896
    jne failed
    mov ah,3eh
    int 21h
    jc failed
    mov ax,10h
    out 0f4h,ax
failed:
    mov ax,11h
    out 0f4h,ax
    hlt
masks db 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63
exact db 'B:\A0',0
wild db 'B:\A0.*',0
outfile db 'A:\CAPTURE.BIN',0
dta times 43 db 0
results times 896 db 0
