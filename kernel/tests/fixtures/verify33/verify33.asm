; DOS 3.30 VERIFY flag services. RES.BIN gets the DOS version, then AL
; from AH=54h at start, and for each AL in the script the AL that AH=2Eh
; leaves followed by AL from AH=54h. DL is zero on every AH=2Eh call.
bits 16
org 100h

    mov ah,30h
    int 21h
    mov [results],ax
    mov word [count],2
    mov ah,54h
    int 21h
    call keep
    mov si,script
.next:
    lodsb
    cmp al,0eeh
    je finish
    mov ah,2eh
    xor dl,dl
    int 21h
    call keep
    mov ah,54h
    int 21h
    call keep
    jmp .next

keep:
    mov bx,[count]
    mov [results+bx],al
    inc word [count]
    ret

finish:
    mov ah,3ch
    xor cx,cx
    mov dx,res_name
    int 21h
    jc .out
    mov bx,ax
    mov ah,40h
    mov cx,[count]
    mov dx,results
    int 21h
    mov ah,3eh
    int 21h
.out:
    mov al,10h
    out 0f4h,al
    jmp $

script db 01h,00h,02h,03h,0ffh,80h,01h,0eeh
res_name db 'B:RES.BIN',0
count dw 0
results times 64 db 0
