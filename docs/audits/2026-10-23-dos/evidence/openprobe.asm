bits 32
org 0x40100
mov eax,0x3d00
mov edx,name
int 0x21
jc open_fail
mov [handle],eax
mov eax,0x4100
mov edx,name
int 0x21
jc refused
mov edx,deleted
jmp message
refused: mov edx,denied
message: mov eax,0x900
int 0x21
mov ebx,[handle]
mov ecx,64
mov edx,buffer
mov eax,0x3f00
int 0x21
jc read_fail
mov ecx,eax
mov eax,0x4000
mov ebx,1
mov edx,buffer
int 0x21
mov eax,0x3e00
mov ebx,[handle]
int 0x21
mov eax,0x4c00
int 0x21
open_fail: mov edx,operr
jmp failure
read_fail: mov edx,rderr
failure: mov eax,0x900
int 0x21
mov eax,0x4c01
int 0x21
name db 'A:\T\OPEN.TXT',0
deleted db 'DELETE_OK',13,10,'$'
denied db 'DELETE_REFUSED',13,10,'$'
operr db 'OPEN_FAILED',13,10,'$'
rderr db 'READ_FAILED',13,10,'$'
handle dd 0
buffer times 64 db 0
