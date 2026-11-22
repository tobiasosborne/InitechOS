; DOS 3.30 character input from redirected standard input. Standard input
; is B:IN.TXT and standard output B:ECHO.TXT, both by DUP2 (AH=46h).
; RES.BIN gets the DOS version, then one byte AL per call in the script,
; saved after every call so a call that never returns still leaves a file.
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
    mov si,script
next:
    lodsb
    or al,al
    jz finish
    cmp al,0ffh                 ; FFh: close standard output (flush ECHO.TXT)
    jne .call
    mov bx,1
    mov ah,3eh
    int 21h
    jmp next
.call:
    mov ah,al
    push si
    int 21h
    mov bx,[count]
    mov [results+bx],al
    inc word [count]
    call save
    pop si
    jmp next
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

; Eight calls consume IN.TXT. EOF selects what follows: 0FFh closes
; standard output, or one more call of that function at end of file.
%ifndef EOF
%define EOF 0ffh
%endif
script db 01h,07h,08h,01h,07h,08h,01h,07h,EOF,0
in_name db 'B:IN.TXT',0
echo_name db 'B:ECHO.TXT',0
res_name db 'B:RES.BIN',0
count dw 0
results times 64 db 0
