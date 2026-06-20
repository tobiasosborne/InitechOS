; write_program.asm -- the baked flat (.COM-equivalent) WRITE round-trip program.
;
; beads: initech-509.11 (FAT write). The end-to-end in-emulator proof that the
;        WRITE half works THROUGH the OS on real ATA in one boot: it
;          1. AH=3Ch CREAT  "OUT.TXT"        -> a write handle
;          2. AH=40h WRITE  "hello\r\n"       -> buffered
;          3. AH=3Eh CLOSE                    -> FLUSH to disk (cluster + FAT + dir)
;          4. AH=3Dh OPEN   "OUT.TXT" (read)  -> read the file we just wrote
;          5. AH=3Fh READ                     -> the bytes back
;          6. AH=40h WRITE to handle 1 (CON)  -> echo them to stdout (serial)
;          7. AH=3Eh CLOSE + AH=4Ch EXIT rc=0
;        The harness asserts the echoed "hello" appears between markers AND
;        triple_fault=0 -- write+read round-trips THROUGH the OS on real ATA.
;
; Ref:   os/milton/int21.c do_creat (AH=3Ch: EDX -> ASCIIZ path, CX=attr,
;        EAX=handle), do_write (AH=40h FILE: EBX=handle, ECX=count, EDX=buf),
;        do_close (AH=3Eh: flush write handle), do_open/do_read (3Dh/3Fh);
;        spec/memory_map.h (PROGRAM_IMAGE = 0x00038100). CLAUDE.md Law 1, Rule 2
;        (fail loud), Rule 11 (deterministic: nasm -f bin), Rule 12 (ASCII).
;
; Assembled: nasm -f bin os/milton/write_program.asm -o build/write_program.bin
; org PROGRAM_IMAGE so absolute references (the name, the buffers) resolve at the
; load address (Solution A, mirrors type_program.asm).
; [initech-o0td: PROGRAM_BASE shifted 0x30000->0x38000; org updated accordingly]

bits 32
org 0x00038100                 ; == spec/memory_map.h PROGRAM_IMAGE

start:
    ; AH=3Ch CREAT: EDX = "OUT.TXT", CX = 0 (normal attribute). EAX = handle.
    mov eax, 0x00003C00        ; AH=3Ch, AL=0
    mov ecx, 0                 ; attribute = 0 (normal)
    mov edx, fname
    int 0x21
    jc  creat_failed
    mov ebx, eax               ; EBX = write handle

    ; AH=40h WRITE: EBX=handle, ECX=len, EDX=payload.
    push ebx
    mov eax, 0x00004000        ; AH=40h
    mov ecx, payload_len
    mov edx, payload
    int 0x21
    pop ebx
    jc  write_failed

    ; AH=3Eh CLOSE: EBX=handle -> FLUSH the buffered bytes to disk.
    mov eax, 0x00003E00        ; AH=3Eh
    int 0x21                   ; EBX still holds the handle
    jc  close_failed

    ; ---- Read it back THROUGH the OS to prove the write committed. ----
    ; AH=3Dh OPEN "OUT.TXT", AL=0 (read).
    mov eax, 0x00003D00        ; AH=3Dh, AL=0 (read)
    mov edx, fname
    int 0x21
    jc  reopen_failed
    mov ebx, eax               ; EBX = read handle

.read_loop:
    ; AH=3Fh READ: EBX=handle, ECX=count, EDX=rbuf. EAX = bytes read.
    push ebx
    mov eax, 0x00003F00        ; AH=3Fh
    mov ecx, BUFSZ
    mov edx, rbuf
    int 0x21
    pop ebx
    jc  done
    test eax, eax
    jz  done                   ; 0 -> EOF

    ; AH=40h WRITE to stdout (handle 1, CON): ECX = bytes read, EDX = rbuf.
    mov ecx, eax
    push ebx
    mov eax, 0x00004000        ; AH=40h
    mov ebx, 1                 ; handle 1 = stdout (CON)
    mov edx, rbuf
    int 0x21
    pop ebx
    jmp .read_loop

done:
    ; AH=3Eh CLOSE the read handle.
    mov eax, 0x00003E00
    int 0x21
    ; AH=4Ch TERMINATE rc=0.
    mov eax, 0x00004C00
    int 0x21
    int 0x20

; Fail-loud paths (Rule 2): each prints a grep-able marker, exits non-zero. -----
creat_failed:
    mov ah, 0x09
    mov edx, msg_creat
    int 0x21
    jmp die
write_failed:
    mov ah, 0x09
    mov edx, msg_write
    int 0x21
    jmp die
close_failed:
    mov ah, 0x09
    mov edx, msg_close
    int 0x21
    jmp die
reopen_failed:
    mov ah, 0x09
    mov edx, msg_reopen
    int 0x21
    jmp die
die:
    mov eax, 0x00004C01        ; exit code 1
    int 0x21
    int 0x20

; Data -----------------------------------------------------------------------
fname:
    db "OUT.TXT", 0
payload:
    db "hello", 0x0D, 0x0A
payload_len equ $ - payload
msg_creat:
    db "WRITE-CREAT-FAIL", 0x0D, 0x0A, "$"
msg_write:
    db "WRITE-WRITE-FAIL", 0x0D, 0x0A, "$"
msg_close:
    db "WRITE-CLOSE-FAIL", 0x0D, 0x0A, "$"
msg_reopen:
    db "WRITE-REOPEN-FAIL", 0x0D, 0x0A, "$"

BUFSZ equ 64
rbuf:
    times BUFSZ db 0
