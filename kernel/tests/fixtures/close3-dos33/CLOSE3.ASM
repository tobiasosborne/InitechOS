bits 16
org 100h
mov bx,3
mov ah,3eh
int 21h
jc failed
mov dx,filename
mov ax,3d00h
int 21h
jc failed
cmp ax,3
jne failed
mov bx,ax
mov ah,3eh
int 21h
mov dx,okay
mov ah,09h
int 21h
mov ax,4c00h
int 21h
failed:
mov dx,bad
mov ah,09h
int 21h
mov ax,4c01h
int 21h
filename db 'MINT.PRG',0
okay db 'CLOSE3-OK',13,10,'$'
bad db 'CLOSE3-FAILED',13,10,'$'
