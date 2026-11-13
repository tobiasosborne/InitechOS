; Rule-6 oracle mutant: change the private C: boot sector's OEM byte.
; Ref: ADR-0003 DEC-15/18, INT 25h/26h flat ABI; PRD 8 disk-byte oracle.
bits 32
org 0x40100
    mov eax,2
    mov ebx,0x50000
    mov ecx,1
    xor edx,edx
    int 0x25
    jc failed
    xor byte [0x50003],0x5a
    mov eax,2
    int 0x26
    jc failed
    mov edx,done
    mov eax,0x0900
    int 0x21
    mov eax,0x4c00
    int 0x21
failed:
    mov edx,bad
    mov eax,0x0900
    int 0x21
    mov eax,0x4c01
    int 0x21
    hlt
done db 'BOOT-WRITE-COMMITTED',13,10,'$'
bad db 'BOOT-WRITE-FAILED',13,10,'$'
