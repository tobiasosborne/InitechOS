; Factory-only flat probe. ADR-0003 Appendix A 3Ch/3Eh/56h;
; IBM DOS 3.30 Technical Reference pp. 6-206/207. No shipped messages.
bits 32
org 0x40100
start:
    mov eax,0x5600
    mov edx,old
    mov edi,new
    int 0x21
    jc fail
    mov edx,renamed
    call puts
    mov esi,names
    mov ebp,15
next:
    mov eax,0x3c00
    xor ecx,ecx
    mov edx,esi
    int 0x21
    jc fail
    mov ebx,eax
    mov eax,0x3e00
    int 0x21
    jc fail
    add esi,13
    dec ebp
    jnz next
    mov edx,grown
    call puts
    mov eax,0x4c00
    int 0x21
puts:
    mov eax,0x0900
    int 0x21
    ret
fail:
    mov edx,failed
    call puts
    mov eax,0x4c01
    int 0x21
old db 'OLD',0
new db 'NEW',0
renamed db 'KHY-RENAME-OK',13,10,'$'
grown db 'KHY-GROW-OK',13,10,'$'
failed db 'KHY-FAIL',13,10,'$'
names:
%assign n 0
%rep 15
    db 'GROW\F',('0'+n/10),('0'+n % 10),'.TXT',0
%assign n n+1
%endrep
