; DOS 3.30 buffered input (AH=0Ah) from redirected standard input. Standard
; input is B:IN0A.TXT and standard output B:ECHO0A.TXT, both by DUP2. Each
; call's 10-byte buffer (max 8) is appended to RES0A.BIN, saved after every
; call. A fourth call runs at end of file.
bits 16
org 100h
%ifndef CALLS
%define CALLS 4
%endif

    mov ah,30h
    int 21h
    mov [results],ax
    mov word [count],2
    mov ax,3d00h
    mov dx,in_name
    int 21h
    jc fail
    mov bx,ax
    xor cx,cx
    mov ah,46h
    int 21h
    jc fail
    mov ah,3eh
    int 21h
    mov ah,3ch
    xor cx,cx
    mov dx,echo_name
    int 21h
    jc fail
    mov bx,ax
    mov cx,1
    mov ah,46h
    int 21h
    jc fail
    mov ah,3eh
    int 21h
    mov byte [calls],CALLS
next:
    cmp byte [calls],0
    je done
    dec byte [calls]
    mov di,buffer
    mov cx,10
    mov al,0cch
    rep stosb
    mov byte [buffer],8
    mov dx,buffer
    mov ah,0ah
    int 21h
    mov si,buffer
    mov di,results
    add di,[count]
    mov cx,10
    rep movsb
    add word [count],10
    call save
    jmp next
done:
    mov bx,1                    ; flush ECHO0A.TXT
    mov ah,3eh
    int 21h
    call save
finish:
    mov al,10h
    out 0f4h,al
    jmp $
fail:
    mov byte [results+1],0eeh
    call save
    jmp finish

save:
    mov ah,3ch
    xor cx,cx
    mov dx,res_name
    int 21h
    jc .done
    mov bx,ax
    mov ah,40h
    mov cx,[count]
    mov dx,results
    int 21h
    mov ah,3eh
    int 21h
.done:
    ret

in_name db 'B:IN0A.TXT',0
echo_name db 'B:ECHO0A.TXT',0
res_name db 'B:RES0A.BIN',0
calls db 0
count dw 0
buffer times 10 db 0
results times 64 db 0
