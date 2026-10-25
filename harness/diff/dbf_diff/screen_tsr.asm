; W4.0 guest capture shim. Extends samirctl-mint/scrdump.asm's B800 copy.
; BIOS data area keyboard/page/cursor layout: IBM PC Technical Reference;
; DOS AH=25h/35h/31h: existing samirctl mint / spec/tsr_contract.h model.
; No DOS calls from INT 1Ch. Capture before cleanup, save later with /S.
bits 16
org 100h
    jmp install
old_tick dd 0
old_mux dd 0
old_dos dd 0
pos dw events
wait_ticks dw 0
mask dw 0
finished dw 0

; Pin the clock presented to the real program, rather than masking its output.
; DOS AH=2Ch (the same service TIME() uses), 12:00:00.00, date set by autoexec.
dos_clock:
    cmp ah,2Ch
    jne .chain
    mov cx,0C00h
    xor dx,dx
    iret
.chain:
    jmp far [cs:old_dos]

mux:
    cmp ax,0D740h
    jne .chain
    mov ax,074Dh
    push cs
    pop es
    mov di,pages
    mov dx,[cs:mask]
    mov bx,[cs:finished]
    iret
.chain:
    jmp far [cs:old_mux]

tick:
    pushf
    pusha
    push ds
    push es
    push cs
    pop ds
    cmp word [finished],0
    jne .done
    mov ax,40h
    mov es,ax
    ; Consume the previous event before starting its explicit wait/dump.
    mov ax,[es:1Ah]
    cmp ax,[es:1Ch]
    jne .done
    cmp word [wait_ticks],0
    je .event
    dec word [wait_ticks]
    jmp .done
.event:
    mov si,[pos]
    lodsb
    mov dx,[si]
    add si,2
    cmp al,0
    je .end
    cmp al,1
    je .key
    cmp al,2
    je .wait
    cmp al,3
    jne .error
    ; Only 80x25 colour text pages are valid. No fabricated screen.
    cmp word [es:4Ah],80
    jne .error
    cmp byte [es:84h],24
    jne .error
    mov [pos],si
    mov cx,dx
    mov ax,1
    shl ax,cl
    or [mask],ax
    mov ax,4002
    mul dx
    mov di,pages
    add di,ax
    mov bx,[es:4Eh]             ; active text page start
    xor ax,ax
    mov al,[es:62h]             ; active page number
    shl ax,1
    mov si,ax
    mov dx,[es:si+50h]          ; DH=row, DL=column
    push cs
    pop es
    mov ax,0B800h
    mov ds,ax
    mov si,bx
    mov cx,2000
    cld
    rep movsw
    mov al,dh
    stosb
    mov al,dl
    stosb
    jmp .done
.key:
    mov bx,[es:1Ch]
    mov [es:bx],dx
    add bx,2
    cmp bx,3Eh
    jb .tail
    mov bx,1Eh
.tail:
    mov [es:1Ch],bx
    jmp .advance
.wait:
    mov [wait_ticks],dx
.advance:
    mov [pos],si
    jmp .done
.end:
    mov word [finished],1
    jmp .done
.error:
    mov word [finished],2
.done:
    pop es
    pop ds
    popa
    popf
    jmp far [cs:old_tick]

events times 8192 db 0
pages times 8*4002 db 0
resident_end:

install:
    cmp byte [80h],0
    jne save
    ; Ref: screen_mint.sh's pinned MOUNT geometry. Refuse host-dependent space.
    mov ah,36h
    mov dl,3
    int 21h
    cmp ax,32
    jne fail
    cmp bx,1024
    jne fail
    cmp cx,512
    jne fail
    cmp dx,2048
    jne fail
    mov ax,3D00h
    mov dx,event_name
    int 21h
    jc fail
    mov bx,ax
    mov ah,3Fh
    mov cx,8192
    mov dx,events
    int 21h
    jc fail
    test ax,ax
    jz fail
    mov ah,3Eh
    int 21h
    mov ax,351Ch
    int 21h
    mov [old_tick],bx
    mov [old_tick+2],es
    mov ax,352Fh
    int 21h
    mov [old_mux],bx
    mov [old_mux+2],es
    mov ax,3521h
    int 21h
    mov [old_dos],bx
    mov [old_dos+2],es
    mov ax,251Ch
    mov dx,tick
    int 21h
    mov ax,252Fh
    mov dx,mux
    int 21h
    mov ax,2521h
    mov dx,dos_clock
    int 21h
    mov dx,(resident_end-$$+100h+15)/16
    mov ax,3100h
    int 21h
save:
    mov ax,0D740h
    int 2Fh
    cmp ax,074Dh
    jne fail
    cmp bx,1
    je .ready
    cmp bx,0
    jne fail
    ; QUIT can return before the script's final wait/END tick has run.
    int 28h
    jmp save
.ready:
    mov [saved_seg],es
    mov [saved_off],di
    mov [saved_mask],dx
    xor bp,bp
.loop:
    bt [saved_mask],bp
    jnc .next
    mov ax,bp
    add al,'0'
    mov [dump_digit],al
    mov ah,3Ch
    xor cx,cx
    mov dx,dump_name
    int 21h
    jc fail
    mov bx,ax
    push ds
    mov dx,[saved_off]
    mov ds,[saved_seg]
    mov ah,40h
    mov cx,4002
    int 21h
    pop ds
    jc fail
    cmp ax,4002
    jne fail
    mov ah,3Eh
    int 21h
    jc fail
.next:
    add word [saved_off],4002
    inc bp
    cmp bp,8
    jb .loop
    ; Only successful, closed captures can produce this fresh completion token.
    cmp word [saved_mask],0
    je fail
    mov ah,3Ch
    xor cx,cx
    mov dx,success_name
    int 21h
    jc fail
    mov bx,ax
    mov ah,40h
    mov cx,2
    mov dx,saved_mask
    int 21h
    jc fail
    cmp ax,2
    jne fail
    mov ah,3Eh
    int 21h
    jc fail
    mov ax,4C00h
    int 21h
fail:
    mov ah,41h
    mov dx,success_name
    int 21h
    mov ax,4C01h
    int 21h
event_name db 'EVENTS.BIN',0
dump_name db 'DUMP'
dump_digit db '0.BIN',0
saved_seg dw 0
saved_off dw 0
saved_mask dw 0
success_name db 'CAPTURE.OK',0
