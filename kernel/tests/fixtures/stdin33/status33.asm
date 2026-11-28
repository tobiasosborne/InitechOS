; DOS 3.30 status and flush-and-input calls on redirected standard input.
; Standard input is B:IN0C.TXT ('ABCline' CR LF) and standard output
; B:ECHO0C.TXT, both by DUP2 (AH=46h). RES0C.BIN gets the DOS version,
; then AL after AH=0Bh, AH=0Ch with AL=01h, 07h, 08h, then the 0Ah
; buffer (capacity 16) after AH=0Ch AL=0Ah, then AL from AH=0Bh with the
; LF left, AH=07h for that LF, and AH=0Bh at end of file. It is saved after every call.
bits 16
org 100h

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

    mov ah,0bh
    call one
    mov ax,0c01h
    call one
    mov ax,0c07h
    call one
    mov ax,0c08h
    call one
    mov byte [line],16
    mov ax,0c0ah
    mov dx,line
    int 21h
    mov si,line
    mov cx,18
.copy:
    lodsb
    mov bx,[count]
    mov [results+bx],al
    inc word [count]
    loop .copy
    call save
    mov ah,0bh
    call one
    mov ah,07h
    call one
    mov ah,0bh
    call one
    mov bx,1
    mov ah,3eh
    int 21h
    jmp finish

one:
    int 21h
    mov bx,[count]
    mov [results+bx],al
    inc word [count]
    call save
    ret

fail:
    mov byte [results+1],0eeh
    call save
finish:
    mov al,10h
    out 0f4h,al
    jmp $

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

in_name db 'B:IN0C.TXT',0
echo_name db 'B:ECHO0C.TXT',0
res_name db 'B:RES0C.BIN',0
count dw 0
line times 18 db 0cch
results times 64 db 0
