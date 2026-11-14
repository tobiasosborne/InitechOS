; Test-only 16-bit probe for genuine DOS 3.30 captures (never shipped).
; Prints one line to stdout (AH=40h): CLK year-mm-dd Ddow hh:mm:ss.cc BRK=n
; from INT 21h AH=2Ah (date), AH=2Ch (time), AH=33h AL=00h (break state).
bits 16
org 100h
start:
    mov ah, 2Ah
    int 21h
    mov [year], cx
    mov [mon], dh
    mov [day], dl
    mov [dow], al
    mov ah, 2Ch
    int 21h
    mov [hh], ch
    mov [mi], cl
    mov [vsec], dh
    mov [vcen], dl
    mov ax, 3300h
    int 21h
    mov [brk], dl
    ; build line in buf
    mov di, buf
    mov si, pfx
    call copyz
    mov ax, [year]
    call put4
    mov al, '-'
    stosb
    mov al, [mon]
    call put2
    mov al, '-'
    stosb
    mov al, [day]
    call put2
    mov al, ' '
    stosb
    mov al, 'D'
    stosb
    mov al, [dow]
    add al, '0'
    stosb
    mov al, ' '
    stosb
    mov al, [hh]
    call put2
    mov al, ':'
    stosb
    mov al, [mi]
    call put2
    mov al, ':'
    stosb
    mov al, [vsec]
    call put2
    mov al, '.'
    stosb
    mov al, [vcen]
    call put2
    mov si, mid
    call copyz
    mov al, [brk]
    add al, '0'
    stosb
    mov al, 0Dh
    stosb
    mov al, 0Ah
    stosb
    mov cx, di
    sub cx, buf
    mov dx, buf
    mov bx, 1
    mov ah, 40h
    int 21h
    mov ax, 4C00h
    int 21h
copyz:
    lodsb
    test al, al
    jz .done
    stosb
    jmp copyz
.done:
    ret
; put4: AX -> 4 digits at DI
put4:
    mov bx, 1000
    call digit
    mov bx, 100
    call digit
    mov bx, 10
    call digit
    add al, '0'
    stosb
    ret
digit:
    xor dx, dx
    div bx
    add al, '0'
    stosb
    mov ax, dx
    ret
; put2: AL -> 2 digits at DI (clobbers AX,BX,DX)
put2:
    xor ah, ah
    mov bl, 10
    div bl
    add al, '0'
    stosb
    mov al, ah
    add al, '0'
    stosb
    ret
pfx db 'CLK ', 0
mid db ' BRK=', 0
year dw 0
mon db 0
day db 0
dow db 0
hh db 0
mi db 0
vsec db 0
vcen db 0
brk db 0
buf times 64 db 0
