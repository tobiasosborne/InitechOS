; Temporary COMMAND.COM disk entry: invoke the resident secondary processor.
; Ref: resident legacy shell EXEC of COMSPEC; INT21 AH4Bh flat parameter block.
bits 32
org 0x40100
start:
 mov eax,0x4b00
 mov ebx,params
 mov edx,name
 int 0x21
 mov eax,0x4c00
 int 0x21
params: dd 0,0x40080,0,0
name: db 'COMMAND.COM',0
