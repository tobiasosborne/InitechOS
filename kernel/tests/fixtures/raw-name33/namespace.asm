; Raw DOS namespace operations, without COMMAND parsing.
bits 16
org 100h
mov di,results
mov dx,directory
mov ax,3900h
int 21h
call record
mov ax,3b00h
int 21h
call record
mov dx,original
xor cx,cx
mov ax,3c00h
int 21h
call record
mov bx,ax
mov dx,payload
mov cx,7
mov ax,4000h
int 21h
call record
mov ax,3e00h
int 21h
call record
mov dx,original
mov ax,4300h
int 21h
call record
mov dx,original
push di
mov di,renamed
mov ax,5600h
int 21h
pop di
call record
mov dx,renamed
mov ax,3d00h
int 21h
call record
mov bx,ax
mov dx,readback
mov cx,7
mov ax,3f00h
int 21h
call record
mov ax,3e00h
int 21h
call record
mov dx,renamed
mov ax,4100h
int 21h
call record
mov dx,root
mov ax,3b00h
int 21h
call record
mov dx,directory
mov ax,3a00h
int 21h
call record
mov dx,result_name
xor cx,cx
mov ax,3c00h
int 21h
mov bx,ax
mov dx,results
mov cx,13*4+7
mov ax,4000h
int 21h
mov ax,3e00h
int 21h
mov ax,4c00h
int 21h
record:
pushf
pop word [di+2]
mov [di],ax
lea di,[di+4]
ret
directory db '\DIRECTORYLONG',0
original db 'ORIGINALNAME.LONG',0
renamed db 'RENAMEDLONG.EXTLONG',0
root db '\',0
result_name db 'RESULT.BIN',0
payload db 'raw API'
results times 13*4 db 0ffh
readback times 7 db 0ffh
