; External completion probe: ordinary INT21 program, stderr independent of
; the stdout redirection being graded. Never installed on the product disk.
bits 32
org 0x40100
mov eax,0x4000
mov ebx,2
mov ecx,message_end-message
mov edx,message
int 0x21
mov eax,0x4c00
int 0x21
message: db 'TRANSPORT-DONE',13,10
message_end:
