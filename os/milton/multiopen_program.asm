; multiopen_program.asm -- the baked flat (.COM-equivalent) MULTI-OPEN program.
;
; beads: initech-0qh (multi-tenant positioned file I/O); epic initech-6qy. THE
;        in-emulator capability proof for the whole multi-tenant step: it
;          1. OPENs TWO different files (HELLO.TXT, SECOND.TXT) CONCURRENTLY --
;             two live handles at once (impossible under the old single 64 KiB
;             whole-file buffer, which returned 0x0004 on the 2nd OPEN).
;          2. Does INTERLEAVED positioned reads with LSEEK on BOTH, proving each
;             handle has its OWN position with NO cross-talk: read 3 from A, 3
;             from B, then LSEEK A back to 0 and read 5 -- A still sees "Hello"
;             regardless of B's intervening read.
;          3. Round-trips a file LARGER THAN 64 KiB (BIG.DAT, 96 KiB): LSEEK to
;             an offset PAST 0x10000 (the old hard limit) and READ a 13-byte
;             signature placed there -- only positioned cluster-chain read (no
;             whole-file buffer) can serve this.
;        Each result is echoed to stdout (handle 1, serial) framed by grep-able
;        markers the harness asserts. Exit rc=0 on success; a fail-loud marker +
;        rc=1 on any error (Rule 2).
;
; Ref:   os/milton/int21.c do_open (AH=3Dh: EDX->ASCIIZ path, AL=mode, EAX=handle),
;        do_read (AH=3Fh: EBX=handle, ECX=count, EDX=buf, EAX=bytes), do_lseek
;        (AH=42h: EBX=handle, AL=whence, ECX:EDX offset), do_write (AH=40h:
;        EBX=1 -> CON), do_close (AH=3Eh), do_terminate (AH=4Ch);
;        spec/memory_map.h (PROGRAM_IMAGE = 0x00030100). CLAUDE.md Law 1, Rule 2
;        (fail loud), Rule 11 (deterministic: nasm -f bin), Rule 12 (ASCII).
;
; Assembled: nasm -f bin os/milton/multiopen_program.asm -o build/multiopen_program.bin
; org PROGRAM_IMAGE so absolute references resolve at the load address.

bits 32
org 0x00030100                 ; == spec/memory_map.h PROGRAM_IMAGE

; Handle storage (this is a single-shot program; globals at fixed addrs are fine).
%define HA  dword [ha]
%define HB  dword [hb]

start:
    ; ---- OPEN HELLO.TXT (read) -> handle A ----
    mov eax, 0x00003D00
    mov edx, fname_a
    int 0x21
    jc  open_a_failed
    mov [ha], eax

    ; ---- OPEN SECOND.TXT (read) -> handle B, while A is still open ----
    mov eax, 0x00003D00
    mov edx, fname_b
    int 0x21
    jc  open_b_failed
    mov [hb], eax

    ; ---- MO-A1: read 3 bytes from A at offset 0 -> "Hel" ----
    mov edx, lbl_a1
    call puts
    mov ebx, [ha]
    mov ecx, 3
    call read_and_echo
    call newline

    ; ---- MO-B1: read 3 bytes from B at offset 0 -> "Sec" ----
    mov edx, lbl_b1
    call puts
    mov ebx, [hb]
    mov ecx, 3
    call read_and_echo
    call newline

    ; ---- interleave: advance B by reading 10 more (does NOT touch A) ----
    mov ebx, [hb]
    mov ecx, 10
    call read_discard

    ; ---- MO-A2: LSEEK A back to 0, read 5 -> "Hello" (A position independent) --
    mov ebx, [ha]
    call lseek0
    mov edx, lbl_a2
    call puts
    mov ebx, [ha]
    mov ecx, 5
    call read_and_echo
    call newline

    ; ---- OPEN BIG.DAT (read) -> handle C ----
    mov eax, 0x00003D00
    mov edx, fname_c
    int 0x21
    jc  open_c_failed
    mov [hc], eax              ; stash handle C (A + B still open: 3 concurrent)
    mov ebx, eax               ; EBX = handle C (kept in EBX through the seek)

    ; ---- LSEEK C to BIG_SIG_OFF (PAST 64 KiB) ----
    push ebx
    mov eax, 0x00004200        ; AH=42h, AL=0 (SEEK_SET)
    mov ecx, 0
    mov edx, BIG_SIG_OFF
    int 0x21
    pop ebx
    jc  big_failed
    ; EAX now = new offset; sanity is the read result, not the value.

    ; ---- MO-BIG: read BIG_SIG_LEN bytes at that offset -> the signature ----
    mov edx, lbl_big
    call puts
    mov ecx, BIG_SIG_LEN
    call read_and_echo         ; EBX = handle C
    call newline

    ; ---- close A, B, and C, exit rc=0 (all three were concurrently open) ----
    mov ebx, [ha]
    mov eax, 0x00003E00
    int 0x21
    mov ebx, [hb]
    mov eax, 0x00003E00
    int 0x21
    mov ebx, [hc]
    mov eax, 0x00003E00
    int 0x21

    mov edx, lbl_done
    call puts
    call newline
    mov eax, 0x00004C00        ; rc=0
    int 0x21
    int 0x20

; ---- helpers --------------------------------------------------------------

; read_and_echo: EBX=handle, ECX=count. READ into rbuf, WRITE the bytes read to
; stdout (handle 1). Clobbers EAX/ECX/EDX; preserves EBX.
read_and_echo:
    push ebx
    mov eax, 0x00003F00        ; AH=3Fh READ
    mov edx, rbuf
    int 0x21                   ; EAX = bytes read
    pop ebx
    jc  .ret
    test eax, eax
    jz  .ret
    mov ecx, eax               ; ECX = bytes read
    push ebx
    mov eax, 0x00004000        ; AH=40h WRITE
    mov ebx, 1                 ; stdout
    mov edx, rbuf
    int 0x21
    pop ebx
.ret:
    ret

; read_discard: EBX=handle, ECX=count. READ and throw away (advance position).
read_discard:
    push ebx
    mov eax, 0x00003F00
    mov edx, rbuf
    int 0x21
    pop ebx
    ret

; lseek0: EBX=handle -> SEEK_SET 0.
lseek0:
    push ebx
    mov eax, 0x00004200        ; AH=42h, AL=0
    mov ecx, 0
    mov edx, 0
    int 0x21
    pop ebx
    ret

; puts: EDX -> '$'-terminated string, print via AH=09h.
puts:
    push eax
    mov ah, 0x09
    int 0x21
    pop eax
    ret

; newline: write CR LF to stdout.
newline:
    push ebx
    push ecx
    push edx
    mov eax, 0x00004000
    mov ebx, 1
    mov ecx, 2
    mov edx, crlf
    int 0x21
    pop edx
    pop ecx
    pop ebx
    ret

; ---- fail-loud paths (Rule 2) ----
open_a_failed:
    mov edx, msg_oa
    jmp die
open_b_failed:
    mov edx, msg_ob
    jmp die
open_c_failed:
    mov edx, msg_oc
    jmp die
big_failed:
    mov edx, msg_big
    jmp die
die:
    mov ah, 0x09
    int 0x21
    mov eax, 0x00004C01        ; rc=1
    int 0x21
    int 0x20

; ---- data -----------------------------------------------------------------
fname_a: db "HELLO.TXT", 0
fname_b: db "SECOND.TXT", 0
fname_c: db "BIG.DAT", 0

lbl_a1:  db "MO-A1=$"
lbl_b1:  db "MO-B1=$"
lbl_a2:  db "MO-A2=$"
lbl_big: db "MO-BIG=$"
lbl_done:db "MO-DONE$"

crlf:    db 0x0D, 0x0A

msg_oa:  db "MO-OPENA-FAIL", 0x0D, 0x0A, "$"
msg_ob:  db "MO-OPENB-FAIL", 0x0D, 0x0A, "$"
msg_oc:  db "MO-OPENC-FAIL", 0x0D, 0x0A, "$"
msg_big: db "MO-BIG-FAIL", 0x0D, 0x0A, "$"

; BIG.DAT signature: 13 ASCII bytes placed at BIG_SIG_OFF (past 0x10000). The
; mint step writes EXACTLY this string at that offset; the program seeks there
; and reads it back. Keep in sync with the Makefile mint (printf at seek=80000).
BIG_SIG_OFF equ 80000          ; > 65536 (the old 64 KiB whole-file cap)
BIG_SIG_LEN equ 13             ; len("BEYOND-64KiB!")

; handle storage
ha:      dd 0
hb:      dd 0
hc:      dd 0

; read buffer (one cluster worth is plenty; reads here are <= 13 bytes)
BUFSZ equ 64
rbuf:    times BUFSZ db 0
