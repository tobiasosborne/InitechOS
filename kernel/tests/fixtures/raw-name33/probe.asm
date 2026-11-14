; Genuine DOS 3.30 raw AH=3Ch/3Dh name probe; never invokes COMMAND parsing.
bits 16
org 100h
mov si,names
mov word [record_ptr],results
next:
cmp byte [si],0
je finish
mov di,[record_ptr]
mov dx,si
xor cx,cx
mov ax,3c00h
int 21h
call record
jc advance
mov bx,ax
mov dx,payload
mov cx,7
mov ax,4000h
int 21h
call record
mov ax,3e00h
int 21h
mov dx,si
mov ax,3d00h
int 21h
call record
jc advance
mov bx,ax
lea dx,[di+4]
mov cx,7
mov ax,3f00h
int 21h
call record
mov ax,3e00h
int 21h
advance:
add word [record_ptr],23
scan:
lodsb
test al,al
jnz scan
jmp next
record:
pushf
pop word [di+2]
mov [di],ax
lea di,[di+4]
ret
finish:
mov dx,result_name
xor cx,cx
mov ax,3c00h
int 21h
jc failed
mov bx,ax
mov dx,results
mov cx,20*23
mov ax,4000h
int 21h
mov ax,3e00h
int 21h
mov ax,4c00h
int 21h
failed:
mov ax,4c01h
int 21h
record_ptr dw 0
payload db 'raw API'
result_name db 'RESULT.BIN',0
names db 'EXACT123.BIN',0
 db 'LONGFILENAME.LONG',0
 db 'BASE.EXTLONG',0
 db 'SUBDIRECTORY\CHILDNAME.TXT',0
 db 'BAD?.TXT',0
 db 'MISSING\LONGFILENAME.TXT',0
 db 'LONGFILE?.LON',0
 db 'LONGFILE+JUNK.LON',0
 db 'NAME.EXT?',0
 db 'NAME..EXT',0
 db 'DOTS.EXT.DOTS',0
 db 'LONGNAME.TOOLONG.MORE',0
 db 'LONGNAME*.TXT',0
 db 'QUOTE123"BAD.TXT',0
 db 'SPACE123 SPACE.TXT',0
 db 'BAD NAME.TXT',0
 db 'EXTTAIL.EXT SPACE',0
 db 'SUBDIRECTORY\CHILD.EXT.DOTS',0
 db 'COMMA123.BAD,TAIL',0
 db 'EQUAL123.BAD=TAIL',0
 db 0
results times 20*23 db 0ffh
