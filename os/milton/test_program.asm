; test_program.asm -- the baked flat (.COM-equivalent) test program.
;
; beads: initech-509.5 (the MILTON loader keystone). This is the FIRST program
;        InitechDOS runs: it prints a '$'-terminated string via INT 21h AH=09h,
;        then terminates via INT 21h AH=4Ch AL=0 -- exercising the OS syscall
;        path AND the loader's return-to-loader mechanism end-to-end.
;
; Ref:   docs/research/psp-loader-ground-truth.md Sec 5 (what it does + the
;        org-at-load-address decision: Solution A, Sec 5.3), Sec 3.3 (entry
;        state: EBX = PSP_ADDR, EIP = PROGRAM_IMAGE); os/milton/int21.c
;        do_puts (AH=09h: EDX -> '$'-terminated string, '$' not emitted) +
;        do_terminate (AH=4Ch: AL = exit code); spec/memory_map.h
;        (PROGRAM_IMAGE = 0x00040100 -- the org below MUST equal it).
;        CLAUDE.md Law 1 (cite), Rule 11 (deterministic: no timestamps /
;        randomness; nasm -f bin is reproducible), Rule 12 (ASCII).
;
; Assembled: nasm -f bin os/milton/test_program.asm -o build/test_program.bin
; Embedded:  build/test_program.bin -> a C byte array via tools/bin2c (Makefile),
;            linked into the kernel .rodata; the loader copies it to 0x40100.
;
; ADDRESSING (Sec 5.3 Solution A / Risk 2): the loader places this image at the
; FIXED address PROGRAM_IMAGE = 0x00040100. We assemble with `org 0x00040100`
; so every absolute reference (the `mov edx, msg`) resolves at the load address.
; If PROGRAM_IMAGE ever moves, this org constant moves with it (and so does the
; PROGRAM_IMAGE in spec/memory_map.h). Solution B (PC-relative via EBX = PSP+0x100)
; is deferred to FAT-sourced load at arbitrary addresses (Sec 5.3 / Sec 6.2).
; [initech-o0td: PROGRAM_BASE shifted 0x30000->0x38000; org updated accordingly]
; [initech-re30.2: PROGRAM_BASE shifted 0x38000->0x40000; org updated accordingly]

bits 32
org 0x00040100                 ; == spec/memory_map.h PROGRAM_IMAGE (Sec 5.3)

; Entry is at byte 0 of the binary (== PROGRAM_IMAGE == the loader's JMP target,
; Sec 3.3). Code first, then the string, so `start` is the first emitted byte.
start:
    ; AH=09h DISPLAY STRING: EDX = flat ptr to the '$'-terminated string.
    ; `msg` is an absolute address (org 0x40100) so EDX is correct at load.
    mov ah, 0x09
    mov edx, msg
    int 0x21

    ; AH=4Ch TERMINATE WITH RETURN CODE: AL = 0 (clean exit). This routes
    ; through int21 do_terminate -> the loader's exit hook, which returns
    ; control to load_program() (Sec 4). Control never falls past this int.
    mov ah, 0x4C
    xor al, al
    int 0x21

    ; Defense in depth (Rule 2): if 4Ch ever failed to terminate, do NOT run off
    ; into the string bytes -- issue INT 20h (the legacy terminate path, Sec 2.1 /
    ; Sec 4.4) which also routes to the loader's exit hook (exit code 0).
    int 0x20

; The message AH=09h prints. The '$' (0x24) is the DOS terminator and is NOT
; emitted (int21.c do_puts). The literal must match the boot oracle's expected
; line (Makefile test-program); changing it makes the oracle go RED (Rule 6).
msg:
    db "Hello from InitechOS program.", 0x0D, 0x0A, "$"
