bits 16
org 100h
 mov cx,224
.fill:
 push cx
 mov dx,name
 mov ah,3ch
 xor cx,cx
 int 21h
 jc .bad
 mov bx,ax
 mov ah,3eh
 int 21h
 inc byte [name+9]
 cmp byte [name+9],'9'+1
 jb .next
 mov byte [name+9],'0'
 inc byte [name+8]
 cmp byte [name+8],'9'+1
 jb .next
 mov byte [name+8],'0'
 inc byte [name+7]
.next:
 pop cx
 loop .fill
 mov dx,target
 mov ax,3c00h
 xor cx,cx
 int 21h
 mov [results],ax
 pushf
 pop word [results+2]
 mov dx,target
 mov ax,5b00h
 xor cx,cx
 int 21h
 mov [results+4],ax
 pushf
 pop word [results+6]
 mov dx,capture
 mov ax,3c00h
 xor cx,cx
 int 21h
 jc .bad
 mov bx,ax
 mov dx,results
 mov cx,8
 mov ah,40h
 int 21h
 mov ah,3eh
 int 21h
 mov ax,0x10
 out 0xf4,ax
.bad:
 mov ax,0x11
 out 0xf4,ax
 jmp $
name db 'B:\FILE000',0
target db 'B:\NEW',0
capture db 'A:\CAPTURE.BIN',0
results times 8 db 0
