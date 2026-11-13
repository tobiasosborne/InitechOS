; Factory-only flat probe. DEC-17: install a scratch MBR AFTER boot, allowing
; an empty table without invoking stage2's documented legacy raw fallback.
bits 32
org 0x40100
    mov eax,0x3d00
    mov edx,path
    int 0x21
    jc bad
    mov ebx,eax
    mov eax,0x3f00
    mov ecx,512
    mov edx,buffer
    int 0x21
    jc bad
    cmp eax,512
    jne bad
    mov eax,0x3e00
    int 0x21
    mov eax,0x0301
    mov ecx,1
    mov edx,0x80
    mov ebx,buffer
    int 0x13
    jc bad
    mov eax,0x4c00
    int 0x21
bad:
    mov eax,0x0900
    mov edx,error
    int 0x21
    mov eax,0x4c01
    int 0x21
path: db 'PREP.BIN',0
error: db 'FDISK-FIXTURE-ERROR',13,10,'$'
buffer: times 512 db 0
