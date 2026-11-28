; DOS 3.30: Ctrl-C on the keyboard while character input waits at the end
; of a redirected standard input. BREAK is set off, INT 23h is hooked, an
; empty B:EMPTY.IN becomes standard input by DUP2 (AH=46h), Ctrl-C (2E03h)
; is put in the BIOS keyboard buffer with INT 16h AH=05h, and then function
; EOF (01h, 07h, 08h or 0Ah) is called. RES.BIN is the DOS version, 'B'
; before the call, then 'C' if INT 23h was entered or 'R' and AL if the call
; returned, then the INT 16h AH=01h status of the keyboard buffer after it
; (ZF: 'E' empty, 'K' key left).
bits 16
org 100h

%ifndef EOF
%define EOF 08h
%endif
    mov ah,30h
    int 21h
    mov [results],ax
    mov word [count],2
    mov ax,3301h
    xor dl,dl
    int 21h
    mov ax,2523h
    mov dx,break
    int 21h
    mov ah,3ch
    xor cx,cx
    mov dx,in_name
    int 21h
    jc fail
    mov bx,ax
    mov ah,3eh
    int 21h
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
    mov ah,05h
    mov cx,2e03h
    int 16h
    mov al,'B'
    call put
    mov ah,EOF
    mov dx,line
    int 21h
    push ax
    mov al,'R'
    call put
    pop ax
    call put
    jmp keys
break:
    push cs
    pop ds
    mov al,'C'
    call put
keys:
    mov ah,01h
    int 16h
    mov al,'E'
    jz .k
    mov al,'K'
.k: call put
    jmp finish
fail:
    mov al,0eeh
    call put
finish:
    mov al,10h
    out 0f4h,al
    jmp $

; Append AL to the results and rewrite RES.BIN.
put:
    mov bx,[count]
    mov [results+bx],al
    inc word [count]
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

in_name db 'B:EMPTY.IN',0
res_name db 'B:RES.BIN',0
line db 8,0
     times 10 db 0
count dw 0
results times 32 db 0
