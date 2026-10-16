; Test client, flat .COM-equivalent (ADR-0003 DEC-08 / Appendix A).
; MS-DOS 3.3 User's Guide pp.154-159,165-169: saved lines exercise the actual
; AH=0Ah editor via injected PS/2 keys. Expected bytes live independently in
; harness/conkeys/lines.txt, and are planted by mtools on an identical image.
bits 32
org 0x40100
start:
    mov eax,0x0900
    mov edx,ready
    int 0x21
    mov eax,0x3c00
    xor ecx,ecx
    mov edx,name
    int 0x21
    jc fail
    mov [handle],eax
    mov esi,7
.next:
    mov eax,0x0a00
    mov edx,line
    int 0x21
    movzx ecx,byte [line+1]
    mov byte [line+ecx+2],13
    mov byte [line+ecx+3],10
    add ecx,2
    mov eax,0x4000
    mov ebx,[handle]
    mov edx,line+2
    int 0x21
    jc fail
    dec esi
    jnz .next
    mov eax,0x3e00
    mov ebx,[handle]
    int 0x21
    mov eax,0x0900
    mov edx,done
    int 0x21
    mov eax,0x4c00
    int 0x21
fail:
    mov eax,0x4c01
    int 0x21
ready: db 'CONKEYS-READY',13,10,'$'
done: db 'CONKEYS-DONE',13,10,'$'
name: db 'LINES.TXT',0
handle: dd 0
line: db 64,0
    times 65 db 0
