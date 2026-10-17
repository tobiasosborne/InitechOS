; Test-only flat program, not shipped. IBM DOS 3.30 TR p.6-140; ADR-0003 A.
; Exercise the real AH=40h entry, position/size and growth, no console model.
bits 32
org 0x40100
start:
    mov esi, cases
.case:
    mov edx,[esi]
    test edx,edx
    jz .guards
    mov eax,0x3d02
    int 0x21
    jc bad
    mov ebx,eax
    mov eax,0x4200
    xor ecx,ecx
    mov edx,[esi+4]
    int 0x21
    jc bad
    mov eax,0x4000
    xor ecx,ecx
    xor edx,edx
    int 0x21
    jc bad
    test eax,eax
    jnz bad
    mov eax,0x4201
    int 0x21
    jc bad
    cmp eax,[esi+4]
    jne bad
    mov eax,0x4202
    int 0x21
    jc bad
    cmp eax,[esi+4]
    jne bad
    cmp dword [esi+8],0
    je .close
    ; Retained-cluster gaps, both within and beyond the old tail.
    mov edx,600
    cmp dword [esi+8],1
    je .position
    mov edx,700
    cmp dword [esi+8],2
    je .position
    mov edx,1600
.position:
    mov eax,0x4200
    xor ecx,ecx
    int 0x21
    jc bad
    mov eax,0x4000
    mov ecx,1000
    mov edx,body
    int 0x21
    jc bad
    cmp eax,1000
    jne bad
.close:
    mov eax,0x3e00
    int 0x21
    jc bad
    add esi,12
    jmp .case
.guards:
    mov eax,0x3d02
    mov edx,ronly
    int 0x21
    jnc bad
    cmp ax,5
    jne bad
    mov eax,0x3d00
    mov edx,guard
    int 0x21
    jc bad
    mov ebx,eax
    mov eax,0x4000
    xor ecx,ecx
    xor edx,edx
    int 0x21
    jnc bad
    cmp ax,5
    jne bad
    mov eax,0x3e00
    int 0x21
    mov eax,0x4000
    mov ebx,255
    int 0x21
    jnc bad
    cmp ax,6
    jne bad
    mov eax,0x4c00
    int 0x21
bad:
    mov eax,0x4c01
    int 0x21
    hlt
cases:
    dd shrink,600,0, bound,512,0, empty,0,0, grow,600,1, child,600,0
    dd gapin,600,2, gapout,600,3
    dd extend,3000,0, shrink,600,0, 0
shrink: db 'SHRINK.BIN',0
bound: db 'BOUND.BIN',0
empty: db 'EMPTY.BIN',0
grow: db 'GROW.BIN',0
gapin: db 'GAPIN.BIN',0
gapout: db 'GAPOUT.BIN',0
child: db 'SUB\CHILD.BIN',0
guard: db 'GUARD.BIN',0
extend: db 'EXTEND.BIN',0
ronly: db 'RONLY.BIN',0
body: times 1000 db 'R'
