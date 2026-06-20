; exith_program.asm -- the FAT-sourced "leaky" child for the EXIT-handle oracle.
;
; beads: initech-6hk (SFT teardown on process EXIT); epic initech-6qy. THE child
;        program that proves the handle leak is fixed: it OPENs FOUR files from
;        the mounted FAT12 volume and TERMINATES via AH=4Ch WITHOUT closing ANY
;        of them. Each run consumes 4 SFT file slots; if EXIT does not reclaim a
;        process's handles, repeated EXECs exhaust the 16-slot file table
;        (SFT_FIRST_FILE..SFT_MAX_ENTRIES) and a later OPEN fails.
;
;        On success (all 4 OPENs returned a handle) it prints EXITH-CHILD-OK and
;        exits rc=0. On ANY OPEN failure (CF=1 -- the table is exhausted) it
;        prints EXITH-CHILD-OPENFAIL and exits rc=1 (fail loud, Rule 2). It does
;        NOT issue AH=3Eh CLOSE on any handle -- the WHOLE point is to leak them
;        and let the kernel's EXIT teardown (sft_close_process) reclaim them.
;
; Ref:   os/milton/int21.c do_open (AH=3Dh: EDX->ASCIIZ path, AL=mode, EAX=handle,
;        CF=error), do_puts (AH=09h), do_terminate (AH=4Ch: AL=exit code ->
;        sft_close_process(g_cur_psp) then the loader exit hook); os/milton/sft.c
;        sft_close_process; spec/memory_map.h (PROGRAM_IMAGE = 0x00038100).
;        CLAUDE.md Law 1 (cite), Rule 2 (fail loud), Rule 11 (nasm -f bin is
;        deterministic), Rule 12 (ASCII).
;
; Assembled: nasm -f bin os/milton/exith_program.asm -o build/exith_program.bin
; Deployed:  mcopy build/exith_program.bin ::EXITH.COM (NOT baked into the kernel;
;            loaded BY NAME via load_program_from_fat / INT 21h AH=4Bh EXEC).
; org PROGRAM_IMAGE so absolute data references resolve at the load address.
; [initech-o0td: PROGRAM_BASE shifted 0x30000->0x38000; org updated accordingly]

bits 32
org 0x00038100                 ; == spec/memory_map.h PROGRAM_IMAGE

start:
    ; ---- OPEN HELLO.TXT (read) -- leak handle 1 ----
    mov eax, 0x00003D00        ; AH=3Dh OPEN, AL=0 (read)
    mov edx, fname_a
    int 0x21
    jc  open_failed

    ; ---- OPEN SECOND.TXT (read) -- leak handle 2 ----
    mov eax, 0x00003D00
    mov edx, fname_b
    int 0x21
    jc  open_failed

    ; ---- OPEN CHAIN.TXT (read) -- leak handle 3 ----
    mov eax, 0x00003D00
    mov edx, fname_c
    int 0x21
    jc  open_failed

    ; ---- OPEN BLOCK.BIN (read) -- leak handle 4 ----
    mov eax, 0x00003D00
    mov edx, fname_d
    int 0x21
    jc  open_failed

    ; ---- all four OPENs succeeded: announce + exit rc=0, leaving them OPEN ----
    mov ah, 0x09
    mov edx, msg_ok
    int 0x21
    mov eax, 0x00004C00        ; AH=4Ch TERMINATE, rc=0 (NO close of any handle)
    int 0x21
    int 0x20                   ; defense in depth (Rule 2): also routes to exit hook

; ---- fail-loud path (Rule 2): an OPEN returned CF=1 -- the SFT is exhausted ----
open_failed:
    mov ah, 0x09
    mov edx, msg_fail
    int 0x21
    mov eax, 0x00004C01        ; rc=1 (the parent's run loop detects this)
    int 0x21
    int 0x20

; ---- data -----------------------------------------------------------------
fname_a: db "HELLO.TXT", 0
fname_b: db "SECOND.TXT", 0
fname_c: db "CHAIN.TXT", 0
fname_d: db "BLOCK.BIN", 0

; '$'-terminated (AH=09h) markers, each on its own line, grep-able on serial.
msg_ok:   db "EXITH-CHILD-OK", 0x0D, 0x0A, "$"
msg_fail: db "EXITH-CHILD-OPENFAIL", 0x0D, 0x0A, "$"
