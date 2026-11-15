; GM8 accepted C: delivery layout. Ordinary INT21 disk program; no resident hook.
bits 32
org 0x40100
    mov eax, 0x1900
    int 0x21
    cmp al, 2
    jne fail
    mov eax, 0x3c00
    xor ecx, ecx
    mov edx, filename
    int 0x21
    jc fail
    mov ebx, eax
    mov eax, 0x4000
    mov ecx, length
    mov edx, payload
    int 0x21
    jc fail
    cmp eax, length
    jne fail
    mov eax, 0x3e00
    int 0x21
    jc fail
    mov eax, 0x0e00
    xor edx, edx
    int 0x21
    mov eax, 0x1900
    int 0x21
    mov edx, absent
    cmp al, 2
    je report
    mov edx, present
    test al, al
    jnz fail
report:
    mov eax, 0x0900
    int 0x21
    mov eax, 0x0e00
    mov edx, 2
    int 0x21
    mov eax, 0x0900
    mov edx, okay
    int 0x21
    mov eax, 0x4c00
    int 0x21
fail:
    mov eax, 0x0900
    mov edx, bad
    int 0x21
    mov eax, 0x4c01
    int 0x21
filename: db 'CSTART.TXT',0
payload: db 'C system persisted bytes',13,10
length equ $-payload
absent: db 'CSTART-A-ABSENT',13,10,'$'
present: db 'CSTART-A-PRESENT',13,10,'$'
okay: db 'CSTART-OK',13,10,'$'
bad: db 'CSTART-FAIL',13,10,'$'
