; Factory fixture: resident flat caller, real AH=4Bh/4Dh and INT20 return.
; Ref: IBM DOS 3.30 TR pp. 6-195..201, 11-4/5; dos_structs.h PSP:16h.
bits 32
org 0
%ifdef NEST_TERM20
    ; A direct child's parent PSP is the resident NEST.COM, not the kernel.
    cmp ebx,0x40000
    jbe term_bad
    cmp word [ebx+0x16],0x4000
    jne term_bad
    call .base
.base:
    pop edx
    add edx,root-.base
    mov eax,0x3b00
    int 0x21
    jc term_bad
    int 0x20
term_bad:
    mov eax,0x4c01
    int 0x21
root db '\',0
%elifdef NEST_DEPTH
    ; DOS requires moving the stack BEFORE shrinking the initial block.
    ; IBM DOS 3.30 TR pp. 11-4/5. Each child leaves room for another EXEC.
    mov edi,ebx
    lea esp,[ebx+8192-4]
    shr ebx,4
    mov ecx,512
    mov eax,0x4a00
    int 0x21
    jc depth_bad
    call .base
.base:
    pop ebp
    sub ebp,.base
    lea edx,[ebp+myname]
    xor ebx,ebx
    mov eax,0x4b00
    int 0x21
    jnc depth_return
    cmp ax,8
    jne depth_bad
    ; The deepest failure must occur with FOUR real resident PSPs, not OOM.
    mov edx,edi
    mov ecx,1
.parents:
    cmp edx,0x40000
    je .count
    movzx edx,word [edx+0x16]
    shl edx,4
    inc ecx
    cmp ecx,4
    jbe .parents
    jmp depth_bad
.count:
    cmp ecx,4
    jne depth_bad
    lea edx,[ebp+depth_ok]
    mov eax,0x0900
    int 0x21
    mov eax,0x4c00
    int 0x21
depth_return:
    mov eax,0x4d00
    int 0x21
    test ax,ax
    jnz depth_bad
    mov eax,0x4c00
    int 0x21
depth_bad:
    mov eax,0x4c01
    int 0x21
myname db '\DEPTH.COM',0
depth_ok db 'EXEC-NESTED-DEPTH-4-BUSY',13,10,'$'
%else
    call base
base:
    pop ebp
    sub ebp,base
    push ebx                    ; caller PSP survives child's register use
    ; Keep a caller-owned heap sentinel below the child's allocated MCB.
    mov eax,0x4800
    mov ebx,1
    int 0x21
    jc fail
    movzx edi,ax
    shl edi,4
    mov dword [edi],0x13572468
    lea edx,[ebp+subdir]
    mov eax,0x3b00
    int 0x21
    jc fail
    mov esi,3
again:
    mov eax,0x4b00
    xor ebx,ebx
    lea edx,[ebp+greet]
    int 0x21
    jc fail
    mov eax,0x4d00
    int 0x21
    cmp ax,7
    jne fail
    mov eax,0x4d00
    int 0x21
    test ax,ax
    jnz fail
    cmp dword [edi],0x13572468
    jne fail
    mov eax,0x6200
    int 0x21
    mov edx,[esp]
    shr edx,4
    cmp bx,dx
    jne fail
    dec esi
    jnz again
    mov eax,0x4b00
    xor ebx,ebx
    lea edx,[ebp+term20]
    int 0x21
    jc fail
    mov eax,0x4d00
    int 0x21
    test ax,ax
    jnz fail
    ; TERM20 changed to root; the resident caller must still see SUB.
    mov eax,0x4700
    xor edx,edx
    lea esi,[ebp+cwd_buf]
    int 0x21
    jc fail
    cmp dword [ebp+cwd_buf],0x00425553
    jne fail
    lea edx,[ebp+depth]
    mov eax,0x4b00
    xor ebx,ebx
    int 0x21
    jc fail
    mov eax,0x4d00
    int 0x21
    test ax,ax
    jnz fail
    lea edx,[ebp+ok]
    mov eax,0x0900
    int 0x21
    mov eax,0x4c00
    int 0x21
fail:
    lea edx,[ebp+bad]
    mov eax,0x0900
    int 0x21
    mov eax,0x4c01
    int 0x21
greet db '\GREET.COM',0
term20 db '\TERM20.COM',0
depth db '\DEPTH.COM',0
subdir db 'SUB',0
cwd_buf times 64 db 0
ok db 'EXEC-NESTED-RC=7 PSP-RESTORED INT20-OK',13,10,'$'
bad db 'EXEC-NESTED-FAIL',13,10,'$'
%endif
