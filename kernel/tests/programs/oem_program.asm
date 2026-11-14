; OEM INT21 service probe. Separate disposable disk client, never in INITDOS.
; Existing flat COM ABI; Microsoft DOS OEM range and tsr_contract page layout.
bits 32
org 0x40100
    cld
    mov edx,once
    mov ax,0x3d00
    int 0x21
    jnc restarted
    mov byte [stage],'1'
    mov ax,0xf900
    int 0x21
    jc failed
    test eax,eax
    jz failed
    cmp ebx,eax
    ja failed
    cmp ecx,ebx
    ja failed
    mov byte [stage],'2'
    mov edx,original
    mov ax,0xfb00
    int 0x21
    jc failed
    mov edi,known
    mov ecx,2000
    mov ax,0x2e5a
    rep stosw
    mov word [known+4000],0x0b03
    mov byte [stage],'3'
    mov edx,known
    mov ax,0xfb02
    int 0x21
    jc failed
    mov byte [stage],'4'
    mov edx,actual
    mov ax,0xfb00
    int 0x21
    jc failed
    mov esi,known
    mov edi,actual
    mov ecx,4002
    repe cmpsb
    jne failed
    mov byte [stage],'5'
    mov edx,original
    mov ax,0xfb01
    int 0x21
    jc failed
    mov edx,actual
    mov ax,0xfb00
    int 0x21
    jc failed
    mov esi,original
    mov edi,actual
    mov ecx,4002
    repe cmpsb
    jne failed
    mov byte [stage],'6'
    mov edx,pagefile
    mov ax,0x3c00
    xor ecx,ecx
    int 0x21
    jc failed
    movzx ebx,ax
    mov byte [stage],'b'
    mov edx,known
    mov ecx,4002
    mov ah,0x40
    int 0x21
    jc failed
    mov byte [stage],'c'
    cmp eax,4002
    jne failed
    mov byte [stage],'d'
    mov ah,0x3e
    int 0x21
    jc failed
    mov byte [stage],'7'
    mov edx,once
    mov ax,0x3c00
    xor ecx,ecx
    int 0x21
    jc failed
    movzx ebx,ax
    mov ah,0x3e
    int 0x21
    jc failed
    mov edx,okay
    mov ah,9
    int 0x21
    mov ax,0xfa00
    int 0x21
failed:
    mov edx,bad
    mov ah,9
    int 0x21
    mov ax,0x4c01
    int 0x21
restarted:
    movzx ebx,ax
    mov ah,0x3e
    int 0x21
    mov edx,again
    mov ah,9
    int 0x21
    mov ax,0x4c00
    int 0x21
once db 'RESTART.DAT',0
pagefile db 'PAGE.BIN',0
okay db 'OEM-CHECKS-OK',13,10,'$'
again db 'OEM-RESTART-OK',13,10,'OEM-END',13,10,'$'
bad db 'OEM-FAIL stage='
stage db '0'
db 13,10,'OEM-END',13,10,'$'
original times 4002 db 0
known times 4002 db 0
actual times 4002 db 0
