; Test fixture, never shipped. DEC-16 C-6; MS-DOS 3.3 Reference p.38.
; ON spins in actual disk reads; OFF returns only after raw ^C is read.
; spec/memory_map.h PROGRAM_IMAGE; spec/dos_structs.h PSP tail at 80h.
bits 32
org 0x40100
    mov eax,0x3d00
    mov edx,filename
    int 0x21
    jc failed
    mov [handle],eax
    mov eax,0x0900
    mov edx,ready
    int 0x21
again:
    mov eax,0x3f00
    mov ebx,[handle]
    mov ecx,1
    mov edx,buffer
    int 0x21
    cmp byte [0x40082],'o' ; command tail " off" vs " on"
    jne again
    cmp byte [0x40083],'f'
    jne again
    mov eax,0x0600
    mov edx,0xff
    int 0x21
    cmp al,3
    jne again
    mov eax,0x0900
    mov edx,offok
    int 0x21
    mov eax,0x4c00
    int 0x21
failed:
    mov eax,0x0900
    mov edx,error
    int 0x21
    mov eax,0x4c01
    int 0x21
filename: db 'INPUT.TXT',0
ready: db 'BREAK-DISK-READY',13,10,'$'
offok: db 'BREAK-OFF-RAW-OK',13,10,'$'
error: db 'BREAK-FIXTURE-ERROR',13,10,'$'
handle: dd 0
buffer: db 0
