; type_program.asm -- the baked flat (.COM-equivalent) TYPE program.
;
; beads: initech-509.5 (read-side file-handle functions). This is the
;        end-to-end integration program that proves OPEN+READ+WRITE+CLOSE over a
;        real mounted FAT12 volume: it OPENs HELLO.TXT, READs it in a loop, WRITEs
;        each chunk to stdout (handle 1, the CON device), CLOSEs, and EXITs --
;        the in-universe `TYPE HELLO.TXT`.
;
; Ref:   docs/research/fs-mount-sft-ground-truth.md Sec 5.2 (the TYPE program
;        sketch); os/milton/int21.c do_open (AH=3Dh: EDX -> ASCIIZ path, AL=mode,
;        EAX=handle), do_read (AH=3Fh: EBX=handle, ECX=count, EDX=buf, EAX=bytes),
;        do_write (AH=40h: EBX=1 -> CON), do_close (AH=3Eh), do_terminate
;        (AH=4Ch: AL=exit code); spec/memory_map.h (PROGRAM_IMAGE = 0x00030100).
;        CLAUDE.md Law 1 (cite), Rule 2 (fail loud on OPEN error), Rule 11
;        (deterministic: nasm -f bin), Rule 12 (ASCII).
;
; Assembled: nasm -f bin os/milton/type_program.asm -o build/type_program.bin
; Embedded:  bin2c -> a C .rodata array; the loader copies it to 0x30100 and JMPs.
;
; ADDRESSING (Solution A): assembled at org 0x00030100 == PROGRAM_IMAGE so the
; absolute references (the filename + the read buffer) resolve at the load
; address. If PROGRAM_IMAGE moves, this org constant moves with it.

bits 32
org 0x00030100                 ; == spec/memory_map.h PROGRAM_IMAGE

start:
    ; AH=3Dh OPEN: EDX = flat ptr to ASCIIZ "HELLO.TXT", AL = 0 (read).
    mov eax, 0x00003D00        ; AH=3Dh, AL=0 (read mode)
    mov edx, fname
    int 0x21
    jc  open_failed            ; CF=1 -> file not found / no volume
    mov ebx, eax               ; EBX = handle (returned in EAX/AX)

.read_loop:
    ; AH=3Fh READ: EBX=handle, ECX=count, EDX=buf. EAX = bytes read.
    push ebx                   ; preserve the handle across the call
    mov eax, 0x00003F00        ; AH=3Fh
    mov ecx, BUFSZ
    mov edx, rbuf
    int 0x21
    pop ebx
    jc  done                   ; a read error -> stop (defensive)
    test eax, eax
    jz  done                   ; 0 bytes -> EOF

    ; AH=40h WRITE to stdout (handle 1): ECX = bytes just read, EDX = buf.
    mov ecx, eax               ; ECX = bytes read
    push ebx                   ; preserve the file handle
    mov eax, 0x00004000        ; AH=40h
    mov ebx, 1                 ; handle 1 = stdout (CON)
    mov edx, rbuf
    int 0x21
    pop ebx
    jmp .read_loop

done:
    ; AH=3Eh CLOSE: EBX = handle.
    mov eax, 0x00003E00        ; AH=3Eh
    int 0x21                   ; EBX still holds the handle

    ; AH=4Ch TERMINATE, AL = 0 (clean exit).
    mov eax, 0x00004C00
    int 0x21
    int 0x20                   ; defense in depth (legacy terminate)

open_failed:
    ; Print a grep-able marker via AH=09h so the oracle can distinguish a failed
    ; OPEN from a silent hang, then terminate with a non-zero code (Rule 2).
    mov ah, 0x09
    mov edx, openmsg
    int 0x21
    mov eax, 0x00004C01        ; exit code 1
    int 0x21
    int 0x20

; Data -----------------------------------------------------------------------
fname:
    db "HELLO.TXT", 0
openmsg:
    db "TYPE-OPEN-FAIL", 0x0D, 0x0A, "$"

BUFSZ equ 64
rbuf:
    times BUFSZ db 0
