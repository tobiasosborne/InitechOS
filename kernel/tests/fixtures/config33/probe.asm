bits 16
org 100h
mov ax,3300h
int 21h
mov [break_state],dl
xor dx,dx
mov ah,0eh
int 21h
mov [drive_count],al
mov si,handles
open_next:
mov dx,command_name
mov ax,3d00h
int 21h
jc opened
mov [si],ax
add si,2
inc byte [open_count]
cmp byte [open_count],20
jb open_next
opened:
mov si,handles
xor cx,cx
mov cl,[open_count]
close_next:
jcxz report
lodsw
mov bx,ax
mov ah,3eh
int 21h
loop close_next
report:
mov dx,prefix
mov ah,9
int 21h
mov al,[break_state]
call hex_byte
mov al,[drive_count]
call hex_byte
mov al,[open_count]
call hex_byte
mov dx,newline
mov ah,9
int 21h
mov ax,4c00h
int 21h
hex_byte:
push ax
shr al,4
call hex_digit
pop ax
and al,15
hex_digit:
add al,'0'
cmp al,'9'
jbe put
add al,7
put:
mov dl,al
mov ah,2
int 21h
ret
prefix db 'CFG-STATE=', '$'
newline db 13,10,'$'
command_name db 'A:\COMMAND.COM',0
break_state db 0
drive_count db 0
open_count db 0
handles times 20 dw 0
