bits 32
org 0x40100
mov edx, message
mov eax, 0x0900
int 0x21
mov eax, 0x4c00
int 0x21
message db 'EXTERNAL-PATH', 13, 10, '$'
