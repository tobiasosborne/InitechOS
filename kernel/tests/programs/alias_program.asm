; Ordinary flat DOS client. Retained alias fixture is minted outside the OS.
; initech-oaouv: a committed destination owns its adopted allocation.
bits 32
org 0x40100
    cld
    mov edx,stage
    mov ax,0x3c00
    xor ecx,ecx
    int 0x21
    jnc failed
    cmp ax,5
    jne failed
    mov ax,0x3d02
    mov edx,stage
    int 0x21
    jc failed
    movzx ebx,ax
    mov [handle],ebx
    xor ecx,ecx
    call refused_write
    mov ecx,1
    call refused_write
    mov ecx,700
    call refused_write
    mov ebx,[handle]
    xor ecx,ecx
    xor edx,edx
    mov ax,0x4201
    int 0x21
    jc failed
    test eax,eax
    jnz failed
    test dx,dx
    jnz failed
    call read_expected
    mov ebx,[handle]
    mov ah,0x3e
    int 0x21
    jc failed
    mov edx,dest
    mov ax,0x3d00
    int 0x21
    jc failed
    movzx ebx,ax
    mov [handle],ebx
    call read_expected
    mov ebx,[handle]
    mov ah,0x3e
    int 0x21
    jc failed
    mov edx,okay
    mov ah,9
    int 0x21
    mov ax,0x4c00
    int 0x21
refused_write:
    mov ebx,[handle]
    mov edx,poison
    mov ah,0x40
    int 0x21
    jnc failed
    cmp ax,5
    jne failed
    ret
read_expected:
    mov ebx,[handle]
    mov edx,actual
    mov ecx,expected_end-expected
    mov ah,0x3f
    int 0x21
    jc failed
    cmp eax,expected_end-expected
    jne failed
    mov esi,expected
    mov edi,actual
    mov ecx,expected_end-expected
    repe cmpsb
    jne failed
    ret
failed:
    mov edx,bad
    mov ah,9
    int 0x21
    mov ax,0x4c01
    int 0x21
stage db 'STAGE.BIN',0
dest db 'DEST.BIN',0
okay db 'ALIAS-GUARD-OK',13,10,'$'
bad db 'ALIAS-GUARD-FAIL',13,10,'$'
handle dd 0
expected db 'independently minted replacement bytes',10
expected_end:
actual times 64 db 0
poison times 700 db 'Z'
