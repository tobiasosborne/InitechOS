; Test-only caller: EXEC COMMAND.COM with the tail InitechBase's RUN COMMAND
; sends (apps/initechbase/pal/pal_dos.c dos_exec), then report the return on
; stderr. Never installed on the product disk.
bits 32
org 0x40100
mov eax,0x4b00
mov edx,path
mov ebx,block
int 0x21
mov edx,back
mov ecx,back_end-back
jnc report
mov edx,failed
mov ecx,failed_end-failed
report:
mov eax,0x4000
mov ebx,2
int 0x21
mov eax,0x4c00
int 0x21
path: db 'A:\COMMAND.COM',0
block: dd 0, tail, 0, 0
tail: db 11,' /C COMMAND',13
back: db 'PARENT-BACK',13,10
back_end:
failed: db 'PARENT-FAIL',13,10
failed_end:
