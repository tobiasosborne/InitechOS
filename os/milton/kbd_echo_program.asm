; initech-wuxo0: real program reads alternating INT 21h AH=07h/08h bytes.
; Ref: IBM DOS 3.30 TR pp.6-11--6-12; IBM PC AT TR 6280070 (Sep 1985),
; System BIOS "Extended Functions", pp.5-18--5-19 (uncited locally).
; Hex output makes NUL prefixes observable. CR terminates the injected trace.
bits 32
org 0x00040100
    mov eax, 0x0900
    mov edx, prefix
    int 0x21
    mov esi, 7
    mov ebp, bytes
.read:
    mov eax, esi
    shl eax, 8
    int 0x21
    xor esi, 15                 ; alternate 07h and 08h, including pair halves
    cmp al, 13
    je .print
    mov [ebp], al
    inc ebp
    cmp ebp, bytes_end
    jb .read
    mov eax, 0x4c01             ; fail loud if the trace outgrows the fixture
    int 0x21
.print:
    mov esi, bytes
.next:
    cmp esi, ebp
    je .done
    movzx edi, byte [esi]
    inc esi
    mov eax, edi
    shr al, 4
    call nibble
    mov eax, edi
    and al, 15
    call nibble
    jmp .next
.done:
    mov dl, 13
    call put
    mov dl, 10
    call put
    mov eax, 0x4c00
    int 0x21
nibble:
    add al, '0'
    cmp al, '9'
    jbe .digit
    add al, 7
.digit:
    mov dl, al
put:
    mov eax, 0x0200
    int 0x21
    ret
; Capture before displaying: DOS output polling implements Ctrl-P/Ctrl-S,
; so a diagnostic write between a NUL prefix and its scan is not a raw read.
; Keep the exact full-byte golden; print only after the terminating CR.
bytes: times 256 db 0
bytes_end:
prefix: db 'KBD-BYTES=','$'
