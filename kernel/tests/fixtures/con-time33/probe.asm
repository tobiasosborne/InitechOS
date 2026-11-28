bits 16
org 0x100
mov bx,1
mov ax,0x5700
int 0x21
jc fail
mov [records],cx
mov [records+2],dx
mov cx,0x1800
mov dx,0x2221
mov ax,0x5701
int 0x21
jc fail
mov ax,0x5700
int 0x21
jc fail
mov [records+4],cx
mov [records+6],dx
mov bx,2
mov ax,0x5700
int 0x21
jc fail
mov [records+8],cx
mov [records+10],dx

mov ah,0x2b
mov cx,1993
mov dx,0x0304
int 0x21
mov ax,0x3d00
mov dx,device
int 0x21
jc fail
mov bx,ax
mov ax,0x5700
int 0x21
jc fail
mov [records+12],cx
mov [records+14],dx

mov ax,0x3c00
xor cx,cx
mov dx,result
int 0x21
jc fail
mov bx,ax
mov ah,0x40
mov dx,records
mov cx,16
int 0x21
jc fail
mov ah,0x3e
int 0x21
mov al,0x10
out 0xf4,al
fail:
mov al,0x11
out 0xf4,al
jmp $
records times 8 dw 0
result db 'RESULT.BIN',0
device db "CON",0
