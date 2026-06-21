; greet_program.asm -- the FAT-sourced flat (.COM-equivalent) GREET program.
;
; beads: initech-saw (FAT-sourced program load). Unlike the other baked test
;        programs, GREET.COM is NOT compiled into the kernel: it is mcopy'd onto
;        the FAT12 data disk (build/fat_data.img) as GREET.COM and loaded BY NAME
;        from the mounted volume via load_program_from_fat / INT 21h AH=4Bh EXEC.
;        Its job is to prove a program ran FROM THE FAT VOLUME (not baked): it
;        prints a recognizable line via AH=09h and exits via AH=4Ch with rc=7.
;
; Ref:   docs/research/psp-loader-ground-truth.md Sec 5 (flat program model);
;        os/milton/int21.c do_puts (AH=09h: EDX -> '$'-terminated string, '$' not
;        emitted) + do_terminate (AH=4Ch: AL = exit code); spec/memory_map.h
;        (PROGRAM_IMAGE = 0x00040100 -- the org below MUST equal it); DOS 3.3 PRM
;        AH=09h/4Ch. CLAUDE.md Law 1 (cite), Rule 11 (deterministic: nasm -f bin
;        is reproducible, no timestamps), Rule 12 (ASCII).
;
; Assembled: nasm -f bin os/milton/greet_program.asm -o build/greet_program.bin
; Deployed:  mcopy build/greet_program.bin ::GREET.COM  (NOT baked into the kernel).
;
; ADDRESSING (Solution A): assembled at org 0x00040100 == PROGRAM_IMAGE so the
; absolute reference (mov edx, msg) resolves at the load address. The loader
; (load_program) copies the .COM to PROGRAM_IMAGE and JMPs in. If PROGRAM_IMAGE
; moves, this org constant moves with it (and spec/memory_map.h).
; [initech-o0td: PROGRAM_BASE shifted 0x30000->0x38000; org updated accordingly]
; [initech-re30.2: PROGRAM_BASE shifted 0x38000->0x40000; org updated accordingly]

bits 32
org 0x00040100                 ; == spec/memory_map.h PROGRAM_IMAGE

start:
    ; AH=09h DISPLAY STRING: EDX = flat ptr to the '$'-terminated message.
    mov ah, 0x09
    mov edx, msg
    int 0x21

    ; AH=4Ch TERMINATE WITH RETURN CODE: AL = 7 (the known rc the oracle asserts
    ; via the return-to-loader exit marker AND AH=4Dh GET-RETURN-CODE). Routes
    ; through int21 do_terminate -> the loader's exit hook -> load_program.
    mov ah, 0x4C
    mov al, 7
    int 0x21

    ; Defense in depth (Rule 2): if 4Ch ever failed to terminate, INT 20h also
    ; routes to the loader's exit hook (exit code 0) rather than running off into
    ; the data bytes.
    int 0x20

; The line AH=09h prints. The '$' (0x24) is the DOS terminator and is NOT
; emitted. The literal is what the test-exec oracle greps for on serial, proving
; a program loaded FROM THE FAT VOLUME ran; changing it makes the oracle RED.
msg:
    db "GREETINGS FROM A:GREET.COM", 0x0D, 0x0A, "$"
