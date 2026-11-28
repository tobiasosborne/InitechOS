bits 16
org 0x100
push cs
pop es
mov ax,0x3000
mov bx,0x1234
mov cx,0x5678
int 0x21
mov [ver],ax
mov [ver+2],bx
mov [ver+4],cx

mov ax,0x3d82
mov dx,result
int 0x21
jnc failure
mov ax,0x3c00
xor cx,cx
mov dx,testfile
int 0x21
jc failure
mov bx,ax
mov ah,0x3e
int 0x21
mov ax,0x3d82
mov dx,testfile
int 0x21
jc failure
mov [live],ax
mov bx,ax
mov ah,0x40
mov cx,4
mov dx,old_data
int 0x21
jc failure
mov bx,ax
mov ax,0x4200
mov cx,1
mov dx,0x2345
int 0x21
jc failure
mov [seek],ax
mov [seek+2],dx
mov ax,0x4201
mov cx,-1
mov dx,-2
int 0x21
jc failure
mov [seek+4],ax
mov [seek+6],dx

mov ax,0x3c00
xor cx,cx
mov dx,testfile
int 0x21
mov [creat],ax
pushf
pop word [creat+2]
jc .child
mov bx,ax
mov ah,0x3e
int 0x21
.child:
mov bx,[live]
mov ax,0x4202
xor cx,cx
xor dx,dx
int 0x21
jc failure
mov [length],ax
mov ax,0x4200
xor cx,cx
xor dx,dx
int 0x21
jc failure
mov ah,0x3f
mov cx,4
mov dx,read_buf
int 0x21
mov [got],ax
pushf
pop word [got+2]
mov ax,0x4a00
mov bx,0x100
int 0x21
mov ax,cs
mov [pb+4],ax
mov [pb+8],ax
mov [pb+12],ax
mov [saved_sp],sp
mov [saved_ss],ss
mov ax,0x4b00
mov dx,child
mov bx,pb
int 0x21
cli
mov ss,[cs:saved_ss]
mov sp,[cs:saved_sp]
sti
push cs
pop ds
jc failure
mov ah,0x4d
int 0x21
mov [inherited],ax
mov ax,0x3c00
xor cx,cx
mov dx,result
int 0x21
jc failure
mov bx,ax
mov ah,0x40
mov cx,evidence_end-ver
mov dx,ver
int 0x21
jc failure
mov ah,0x3e
int 0x21
mov al,0x10
out 0xf4,al
failure:
mov al,0x11
out 0xf4,al
jmp $
ver times 3 dw 0
seek times 4 dw 0
creat times 2 dw 0
inherited dw 0
live dw 0
length dw 0
got dw 0,0
evidence_end:
saved_ss dw 0
saved_sp dw 0
pb dw 0, tail, 0, 0x5c, 0, 0x6c, 0
tail db 0, 13
read_buf times 4 db 0
old_data db "OLD!"
testfile db 'TEST.DAT',0
result db 'RESULT.BIN',0
child db 'CHILD.COM',0
