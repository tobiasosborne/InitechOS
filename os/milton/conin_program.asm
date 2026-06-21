; conin_program.asm -- the baked flat (.COM-equivalent) CON-INPUT test program.
;
; beads: initech-n62 (INT 21h CON input functions). This is the end-to-end
;        integration program that proves a real program reads a LINE from the
;        keyboard via INT 21h AH=0Ah BUFFERED INPUT and writes it back -- the
;        input half of the CON device, exercised through the live PS/2 keyboard.
;
;        It prints a fixed prompt via AH=09h, reads a line into a DOS buffered-
;        input block via AH=0Ah (byte0 = max length incl. CR; AH=0Ah writes
;        byte1 = count, bytes2.. = the chars + a CR), then writes a fixed prefix
;        ("CONIN-LINE=") followed by the captured chars (using the count byte as
;        the length) + CRLF via AH=40h to stdout, and EXITs. The harness injects
;        --keys "d,i,r,ret" and asserts "CONIN-LINE=dir" comes back on serial.
;
; Ref:   DOS 3.3 Programmer's Reference Manual AH=09h / AH=0Ah / AH=40h / AH=4Ch;
;        os/milton/int21.c do_buffered_input (AH=0Ah), do_puts (AH=09h),
;        do_write (AH=40h: EBX=1 -> CON), do_terminate (AH=4Ch).
;        spec/memory_map.h (PROGRAM_IMAGE = 0x00040100). CLAUDE.md Law 1 (cite),
;        Rule 2 (no overflow), Rule 11 (deterministic nasm -f bin), Rule 12 (ASCII).
;
; Assembled: nasm -f bin os/milton/conin_program.asm -o build/conin_program.bin
; ADDRESSING (Solution A): org 0x00040100 == PROGRAM_IMAGE.
; [initech-o0td: PROGRAM_BASE shifted 0x30000->0x38000; org updated accordingly]
; [initech-re30.2: PROGRAM_BASE shifted 0x38000->0x40000; org updated accordingly]

bits 32
org 0x00040100

BUF_MAX equ 32                 ; max line length incl. the CR (caller-set byte0)

start:
    ; AH=09h DISPLAY STRING: print the prompt (ends with '$').
    mov eax, 0x00000900        ; AH=09h
    mov edx, prompt
    int 0x21

    ; AH=0Ah BUFFERED INPUT: EDX -> the buffered-input block. byte0 is preset to
    ; BUF_MAX below; on return byte1 = chars read (NOT counting CR), bytes2.. =
    ; the chars then a CR.
    mov eax, 0x00000A00        ; AH=0Ah
    mov edx, inbuf
    int 0x21

    ; Write the fixed prefix "CONIN-LINE=" to stdout (handle 1).
    mov eax, 0x00004000        ; AH=40h
    mov ebx, 1                 ; handle 1 = stdout (CON)
    mov ecx, prefix_len
    mov edx, prefix
    int 0x21

    ; Write the captured chars: ECX = the count byte (inbuf+1), EDX = inbuf+2.
    movzx ecx, byte [inbuf + 1]
    test ecx, ecx
    jz   .crlf                 ; empty line -> skip the chars
    mov eax, 0x00004000        ; AH=40h
    mov ebx, 1
    mov edx, inbuf + 2
    int 0x21

.crlf:
    ; Write a trailing CRLF so the harness can match a whole line.
    mov eax, 0x00004000        ; AH=40h
    mov ebx, 1
    mov ecx, 2
    mov edx, crlf
    int 0x21

    ; AH=4Ch TERMINATE, AL = 0 (clean exit).
    mov eax, 0x00004C00
    int 0x21
    int 0x20                   ; defense in depth (legacy terminate)

; Data -----------------------------------------------------------------------
prompt:
    db "CONIN-PROMPT>", "$"
prefix:
    db "CONIN-LINE="
prefix_len equ $ - prefix
crlf:
    db 0x0D, 0x0A

; The DOS buffered-input block: byte0 = max (incl. CR), byte1 = count (written),
; bytes2.. = the chars + CR. Reserve BUF_MAX+2 bytes (the +2 for byte0/byte1).
inbuf:
    db BUF_MAX                  ; byte0: max length including the CR
    times (BUF_MAX + 1) db 0    ; byte1 (count) + room for BUF_MAX chars/CR
