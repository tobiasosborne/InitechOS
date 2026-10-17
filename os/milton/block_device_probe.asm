; Oracle authored before service: IBM DOS TR pp9-7..9, DEC-17.
; Device MBR != volume boot sector. Write ONLY to a harness scratch --disk2.
bits 32
org 0x40100
start:
    mov eax,0x1500
    mov edx,0x81
    int 0x13
    jc bad
    cmp ah,3
    jne bad
    cmp cx,0
    jne bad
    cmp dx,2943
    jne bad
    mov eax,0x0800
    mov edx,0x81
    int 0x13
    jc bad
    cmp cx,0x013f
    jne bad
    cmp dh,15
    jne bad
    xor eax,eax
    mov edx,0x81
    int 0x13
    jc bad
    mov eax,0x0201
    mov ecx,1
    mov edx,0x1081
    mov ebx,buffer
    int 0x13
    jnc bad
    cmp ah,4
    jne bad
    mov ah,9
    mov edx,params
    int 0x21
    mov eax,0x0201
    mov ecx,1
    mov edx,0x81
    mov ebx,buffer
    int 0x13
    jc bad
    cmp word [buffer+510],0xaa55
    jne bad
    cmp byte [buffer+450],1
    jne bad
    mov ah,9
    mov edx,mbr
    int 0x21
    mov eax,0x0301
    mov ecx,33
    mov edx,0x81
    mov ebx,marker
    int 0x13
    jc bad
    mov ah,9
    mov edx,done
    int 0x21
    mov eax,0x4c00
    int 0x21
bad:
    mov ah,9
    mov edx,failed
    int 0x21
    mov eax,0x4c01
    int 0x21
mbr: db 'BLOCK-MBR signature=55AA type=01',13,10,'$'
params: db 'BLOCK-PARAMS-RESET-STATUS-OK',13,10,'$'
done: db 'BLOCK-WRITE-OK',13,10,'$'
failed: db 'BLOCK-ERROR',13,10,'$'
marker: db 'INITECH INT13 DEVICE LBA 32',13,10
times 512-($-marker) db 0x5a
buffer: times 512 db 0
