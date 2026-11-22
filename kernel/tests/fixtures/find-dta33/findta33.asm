; DOS 3.30 FINDFIRST/FINDNEXT continuation across DTAs. Writes B:RESULT.BIN.
; Each step records: step id, CF, AX, then the 43 bytes of the DTA used.
bits 16
org 100h

    mov ah,30h
    int 21h
    mov di,results
    stosw
    mov al,0cch                 ; mark every DTA byte DOS does not write
    mov cx,43*3
    push di
    mov di,dta_a
    rep stosb
    pop di

    mov si,script
step:
    lodsb                       ; op: 0 end, 1 first, 2 next
    or al,al
    jz done
    mov bl,al
    lodsw                       ; DTA address
    mov [cur],ax
    lodsw                       ; spec address (first only)
    mov [spec],ax
    push si
    mov dx,[cur]
    mov ah,1ah
    int 21h
    cmp bl,1
    jne .next
    mov ah,4eh
    mov cx,10h                  ; ordinary files and directories
    mov dx,[spec]
    int 21h
    jmp .rec
.next:
    cmp bl,3
    jne .plain
    push di                     ; op 3: copy DTA A into DTA C, then FINDNEXT on C
    mov si,dta_a
    mov di,dta_c
    mov cx,43
    rep movsb
    pop di
.plain:
    mov ah,4fh
    int 21h
.rec:
    pushf
    mov [di],bl
    inc di
    setc [di]
    inc di
    stosw
    popf
    mov si,[cur]
    mov cx,43
    rep movsb
    pop si
    jmp step

done:
    mov dx,dta_a
    mov ah,1ah
    int 21h
    mov ah,3ch
    xor cx,cx
    mov dx,output
    int 21h
    jc finish
    mov bx,ax
    mov ah,40h
    mov cx,di
    sub cx,results
    mov dx,results
    int 21h
    mov ah,3eh
    int 21h
finish:
    mov al,10h
    out 0f4h,al
    jmp $

; op, DTA, spec
script:
    db 1
    dw dta_a, spec_a
    db 1
    dw dta_b, spec_b
    db 2
    dw dta_a, 0
    db 2
    dw dta_b, 0
    db 3                        ; C := A, FINDNEXT on C
    dw dta_c, 0
    db 2                        ; A again: continues from A's own copy
    dw dta_a, 0
    db 1
    dw dta_b, spec_s            ; B restarts on the subdirectory
    db 2
    dw dta_a, 0
    db 2
    dw dta_b, 0
    db 2
    dw dta_a, 0
    db 2
    dw dta_b, 0
    db 2
    dw dta_b, 0
    db 2
    dw dta_b, 0
    db 2
    dw dta_c, 0
    db 2
    dw dta_c, 0
    db 0

cur dw 0
spec dw 0
spec_a db 'B:A*.*',0
spec_b db 'B:B*.*',0
spec_s db 'B:\SUB\*.*',0
output db 'B:RESULT.BIN',0
dta_a times 43 db 0
dta_b times 43 db 0
dta_c times 43 db 0
results:
