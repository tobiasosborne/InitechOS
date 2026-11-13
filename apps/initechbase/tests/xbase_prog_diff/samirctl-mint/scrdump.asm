; scrdump.com -- mint-only helper: copy the 80x25 colour text page B800:0000
; (4000 bytes, char+attr pairs) to SCREEN.BIN so the post-run dBASE screen can
; be read back on the host. Factory tool for the samirctl mint (not shipped).
org 100h
    mov ah,3Ch
    xor cx,cx
    mov dx,fname
    int 21h
    jc done
    mov bx,ax
    push ds
    mov ax,0B800h
    mov ds,ax
    mov ah,40h
    mov cx,4000
    xor dx,dx
    int 21h
    pop ds
    mov ah,3Eh
    int 21h
done:
    mov ax,4C00h
    int 21h
fname db 'SCREEN.BIN',0
