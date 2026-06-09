; sysinit_program.asm -- the FAT-sourced child that proves the FILES= cap BITES.
;
; beads: initech-509.2 (SYSINIT + CONFIG.SYS FILES= cap). THE child program that
;        proves the CONFIG.SYS FILES=N directive has REAL effect on the SFT: it
;        OPENs the same file (HELLO.TXT) over and over WITHOUT closing, counting
;        how many OPENs succeed before one returns CF=1 (the SFT file slots are
;        exhausted at the FILES= cap). It then reports the count on serial as
;        SYSINIT-PROG-OPENS=<n> and exits rc=0.
;
;        With CONFIG.SYS FILES=8 the runtime cap is 8 -> file SFT slots 4..7 (4
;        slots), so EXACTLY 4 OPENs succeed and the 5th fails. The in-emulator
;        oracle (make test-sysinit) asserts the count == the expected cap (4) --
;        i.e. the cap bites at exactly the limit, not before, not after.
;
; Ref:   os/milton/int21.c do_open (AH=3Dh: EDX->ASCIIZ path, AL=mode, EAX=handle,
;        CF=error on TOO_MANY_OPEN); do_puts (AH=09h); do_terminate (AH=4Ch);
;        os/milton/sft.c sft_alloc (respects g_files_limit); os/milton/sysinit.c
;        sysinit_apply_config (sets the cap); spec/memory_map.h (PROGRAM_IMAGE).
;        CLAUDE.md Law 1 (cite), Rule 2 (fail loud), Rule 11 (nasm -f bin is
;        deterministic), Rule 12 (ASCII).
;
; Assembled: nasm -f bin os/milton/sysinit_program.asm -o build/sysinit_program.bin
; Deployed:  mcopy build/sysinit_program.bin ::SYSI.COM (loaded BY NAME via
;            load_program_from_fat). org PROGRAM_IMAGE so abs refs resolve.

bits 32
org 0x00020100                 ; == spec/memory_map.h PROGRAM_IMAGE

start:
    xor ebx, ebx               ; ebx = successful-open count

open_loop:
    ; Safety bound: never loop more than 32 times (well above any cap).
    cmp ebx, 32
    jae  done

    mov eax, 0x00003D00        ; AH=3Dh OPEN, AL=0 (read)
    mov edx, fname
    int 0x21
    jc  done                   ; CF=1 -> SFT exhausted at the FILES= cap
    inc ebx                    ; one more handle leaked open
    jmp open_loop

done:
    ; ---- report the count: "SYSINIT-PROG-OPENS=" + decimal(ebx) + CRLF ----
    mov ah, 0x09
    mov edx, msg_prefix
    int 0x21

    ; Decompose ebx (0..32) into tens (ecx) + ones (eax) by repeated subtraction
    ; (no DIV needed; the value is small). Print the tens digit only if non-zero.
    mov eax, ebx               ; value
    xor ecx, ecx               ; ecx = tens digit
.tens:
    cmp eax, 10
    jb  .have_digits
    sub eax, 10
    inc ecx
    jmp .tens
.have_digits:
    ; eax = ones digit (0..9), ecx = tens digit (0..3)
    test ecx, ecx
    jz  .ones                  ; no tens digit -> print ones only
    push eax                   ; save the ones digit
    mov dl, cl
    add dl, '0'
    mov eax, 0x00000200        ; AH=02h DISPLAY CHAR, DL = char
    int 0x21
    pop eax                    ; restore ones digit
.ones:
    mov dl, al
    add dl, '0'
    mov eax, 0x00000200        ; AH=02h DISPLAY CHAR
    int 0x21

    ; trailing CRLF via AH=09h
    mov ah, 0x09
    mov edx, msg_crlf
    int 0x21

    mov eax, 0x00004C00        ; AH=4Ch TERMINATE rc=0
    int 0x21
    int 0x20                   ; defense in depth (Rule 2)

; ---- data -----------------------------------------------------------------
fname:      db "HELLO.TXT", 0
msg_prefix: db "SYSINIT-PROG-OPENS=", "$"
msg_crlf:   db 0x0D, 0x0A, "$"
