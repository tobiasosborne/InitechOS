; DOS 3.30 CREATE attribute capture. Writes B:RESULT.BIN, then exits QEMU.
; Matrix row (16 bytes): create CF,AX; write CF,AX; seek CF; read CF,AX;
; three bytes read back; get-attribute CF,CX.
bits 16
org 100h

    mov ah,30h
    int 21h
    mov [version],ax
    mov di,results
    mov si,matrix
row:
    lodsb                       ; CX for this row (FFh ends)
    cmp al,0ffh
    je extras
    mov [attr],al
    lodsw                       ; two name characters
    mov [name+3],ax
    push si
    mov ah,3ch
    xor ch,ch
    mov cl,[attr]
    mov dx,name
    int 21h
    call record
    jc .failed
    mov [handle],ax
    mov bx,ax
    mov ah,40h
    mov cx,3
    mov dx,payload
    int 21h
    call record
    mov ax,4200h
    mov bx,[handle]
    xor cx,cx
    xor dx,dx
    int 21h
    call flag
    mov ah,3fh
    mov bx,[handle]
    mov cx,3
    mov dx,di
    add dx,3                    ; read bytes land after the read record
    mov word [di+3],0
    mov byte [di+5],0
    int 21h
    call record
    add di,3
    mov ah,3eh
    mov bx,[handle]
    int 21h
    mov ax,4300h
    mov dx,name
    int 21h
    call flag
    mov ax,cx
    stosw
    pop si
    jmp row
.failed:
    mov cx,13
    mov al,0eeh
    rep stosb
    pop si
    jmp row

; Extra probes, each CF then AX (or CX for 4300h).
extras:
    mov word [name+3],'01'      ; read-only file from the matrix
    mov ah,3ch                  ; CREATE over an existing read-only file
    xor cx,cx
    mov dx,name
    int 21h
    call record
    jc .ro_open
    mov bx,ax
    mov ah,3eh
    int 21h
.ro_open:
    mov ax,3d01h                ; reopen it for writing
    mov dx,name
    int 21h
    call record
    jc .ro_read
    mov bx,ax
    mov ah,3eh
    int 21h
.ro_read:
    mov ax,3d00h                ; and for reading
    mov dx,name
    int 21h
    call record
    jc .hidden
    mov bx,ax
    mov ah,3eh
    int 21h
.hidden:
    mov word [name+3],'02'      ; CREATE over an existing hidden file, CX=0
    mov ah,3ch
    xor cx,cx
    mov dx,name
    int 21h
    call record
    jc .hidden_attr
    mov bx,ax
    mov ah,3eh
    int 21h
.hidden_attr:
    mov ax,4300h
    mov dx,name
    int 21h
    call record_cx
    mov word [name+3],'00'      ; truncate an ordinary file with CX=1
    mov ah,3ch
    mov cx,1
    mov dx,name
    int 21h
    call record
    jc .plain_attr
    mov bx,ax
    mov ah,3eh
    int 21h
.plain_attr:
    mov ax,4300h
    mov dx,name
    int 21h
    call record_cx
    mov word [name+3],'N3'      ; CREATE NEW, CX=3
    mov ah,5bh
    mov cx,3
    mov dx,name
    int 21h
    call record
    jc .new_again
    mov [handle],ax
    mov bx,ax
    mov ah,40h
    mov cx,3
    mov dx,payload
    int 21h
    call record
    mov ax,4200h
    mov bx,[handle]
    xor cx,cx
    xor dx,dx
    int 21h
    mov ah,3fh
    mov bx,[handle]
    mov cx,3
    mov dx,readback
    int 21h
    call record
    mov ax,[readback]
    stosw
    mov al,[readback+2]
    stosb
    mov ah,3eh
    mov bx,[handle]
    int 21h
.new_again:
    mov ax,4300h
    mov dx,name
    int 21h
    call record_cx
    mov ah,5bh                  ; CREATE NEW over the file just made
    xor cx,cx
    mov dx,name
    int 21h
    call create_done

    mov word [name+3],'02'      ; hidden file again, now with CX=2
    mov cx,2
    call create_close
    call get_attr
    mov word [name+3],'04'      ; system file with CX=0, then CX=4
    xor cx,cx
    call create_close
    mov cx,4
    call create_close
    call get_attr
    mov word [name+3],'N8'      ; CREATE NEW of a second label, CX=8
    mov ah,5bh
    mov cx,8
    call create_any
    mov word [name+3],'NX'      ; CREATE NEW, CX=10h
    mov ah,5bh
    mov cx,10h
    call create_any
    mov word [name+3],'H1'      ; CREATE, CX=0100h (CH only)
    mov cx,100h
    call create_close
    call get_attr

write_results:
    mov ah,3ch
    xor cx,cx
    mov dx,output
    int 21h
    jc finish
    mov bx,ax
    mov ah,40h
    mov cx,di
    sub cx,version
    mov dx,version
    int 21h
    mov ah,3eh
    int 21h
finish:
    mov al,10h
    out 0f4h,al
    jmp $

; AH=3Ch with CX, record, close any handle.
create_close:
    mov ah,3ch
create_any:
    mov dx,name
    int 21h
create_done:
    call record
    jc .done
    mov bx,ax
    mov ah,3eh
    int 21h
.done:
    ret

get_attr:
    mov ax,4300h
    mov dx,name
    int 21h
    call record_cx
    ret

; Save carry then AX, preserving both.
record:
    pushf
    push ax
    setc al
    stosb
    pop ax
    stosw
    popf
    ret

record_cx:
    pushf
    push ax
    setc al
    stosb
    mov ax,cx
    stosw
    pop ax
    popf
    ret

flag:
    pushf
    push ax
    setc al
    stosb
    pop ax
    popf
    ret

matrix:
    db 00h,'00', 01h,'01', 02h,'02', 04h,'04', 07h,'07', 20h,'20', 27h,'27'
    db 08h,'08', 10h,'10', 40h,'40', 80h,'80', 18h,'18'
    db 0ffh
attr db 0
handle dw 0
name db 'B:F', 0, 0, 0
output db 'B:RESULT.BIN',0
payload db 'abc'
readback db 0,0,0
version dw 0
results:
