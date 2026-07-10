; gobble_program.asm -- the FAT-sourced flat (.COM-equivalent) GOBBLE program.
;
; beads: initech-bsy.7 (Shell INPUT redirection `<`). The stdin-side sibling of
;        GREET.COM (initech-saw). Like GREET it is NOT baked into the kernel: it
;        is mcopy'd onto a FAT12 data disk as GOBBLE.COM and loaded BY NAME via
;        load_program_from_fat / INT 21h AH=4Bh EXEC. Its job is to prove that a
;        `<` INPUT redirect actually re-points the EXEC child's STDIN (handle 0)
;        at a file: it READS handle 0 (AH=3Fh) to EOF and echoes every byte to
;        handle 1 (AH=40h), bracketed by fixed BEGIN/END markers. Under
;        `GOBBLE < IN.TXT` the child inherits the parent's DUP2'd JFT (beads
;        bsy.9), so handle 0 is IN.TXT and its content round-trips to serial.
;        With the `<` parse disabled (CMD_MUTATE_REDIR_NO_LT) handle 0 is the
;        keyboard, the file marker never appears, and the emu gate goes RED.
;
; Ref:   os/milton/int21.c do_read (AH=3Fh: EBX=handle, ECX=count, EDX=buf ->
;        EAX=bytes, 0=EOF) + do_write (AH=40h: EBX=handle, ECX=count, EDX=buf) +
;        do_puts (AH=09h: EDX -> '$'-terminated string) + do_terminate (AH=4Ch:
;        AL=exit code); spec/memory_map.h (PROGRAM_IMAGE = 0x00040100 == org);
;        DOS 3.3 PRM AH=3Fh/40h/09h/4Ch. CLAUDE.md Law 1 (cite), Rule 11
;        (deterministic: nasm -f bin, no timestamps), Rule 12 (ASCII).
;
; Assembled: nasm -f bin os/milton/gobble_program.asm -o build/gobble_program.bin
; Deployed:  mcopy build/gobble_program.bin ::GOBBLE.COM  (NOT baked into kernel).
;
; ADDRESSING mirrors GREET.COM: assembled at org 0x00040100 == PROGRAM_IMAGE so
; the absolute references resolve at the load address (the loader copies the .COM
; to PROGRAM_IMAGE and JMPs in). If PROGRAM_IMAGE moves, this org moves with it
; (and spec/memory_map.h + greet_program.asm).

bits 32
org 0x00040100                 ; == spec/memory_map.h PROGRAM_IMAGE

start:
    ; AH=09h DISPLAY STRING: the BEGIN marker (proves GOBBLE ran at all).
    mov ah, 0x09
    mov edx, msg_begin
    int 0x21

.read_loop:
    ; AH=3Fh READ FROM HANDLE: EBX=0 (stdin), ECX=count, EDX=buffer.
    ; Under `GOBBLE < IN.TXT` handle 0 is the redirected file (inherited JFT).
    mov ah, 0x3F
    xor ebx, ebx               ; handle 0 = stdin
    mov ecx, BUFLEN
    mov edx, buf
    int 0x21
    ; EAX = bytes read (0 = EOF). CF set on error -> treat like EOF (stop).
    jc .done
    test eax, eax
    jz .done

    ; AH=40h WRITE TO HANDLE: EBX=1 (stdout), ECX=EAX (bytes just read), EDX=buf.
    ; With no `>` on the line, handle 1 is CON -> serial, so the echoed stdin
    ; bytes appear on the serial log the oracle greps.
    mov ecx, eax               ; count = bytes read
    mov ah, 0x40
    mov ebx, 1                 ; handle 1 = stdout
    mov edx, buf
    int 0x21

    jmp .read_loop

.done:
    ; AH=09h DISPLAY STRING: the END marker (proves the read loop reached EOF).
    mov ah, 0x09
    mov edx, msg_end
    int 0x21

    ; AH=4Ch TERMINATE WITH RETURN CODE: AL = 0 (clean exit).
    mov ah, 0x4C
    mov al, 0
    int 0x21

    ; Defense in depth (Rule 2): if 4Ch ever failed to terminate, INT 20h also
    ; routes to the loader's exit hook rather than running into the data bytes.
    int 0x20

; The BEGIN/END markers frame the echoed stdin on serial. '$' (0x24) is the DOS
; AH=09h terminator and is NOT emitted. Changing these makes the oracle RED.
msg_begin:
    db "GOBBLE-BEGIN", 0x0D, 0x0A, "$"
msg_end:
    db 0x0D, 0x0A, "GOBBLE-END", 0x0D, 0x0A, "$"

; STDIN read buffer (128 bytes: one TYPE-sized chunk, ample for the test fixture).
BUFLEN equ 128
buf:
    times BUFLEN db 0
