; irqstorm_program.asm -- the baked flat (.COM-equivalent) IRQ-STORM program.
;
; beads: initech-xk2 (INT 21h reentrancy hardening when IRQs are unmasked). THE
;        in-emulator binding oracle for the reentrancy guard: it drives the INT
;        21h dispatcher functions that USE the dispatcher's GLOBAL state -- the
;        FINDFIRST/FINDNEXT search state (g_find) + the DTA (g_dta), the FAT
;        cache + cluster scratch (a multi-cluster positioned READ), and a SECOND
;        concurrent open handle (the multiopen/SFT path) -- WHILE the harness
;        injects a STORM of keystrokes (IRQ1) and the free-running 100 Hz PIT
;        (IRQ0) ticks throughout. Because INT 21h is a 0x8F TRAP gate (IF stays
;        set), an IRQ CAN land mid-syscall; if an IRQ corrupted the frame or a
;        shared global, the enumeration order or the read bytes below would come
;        back WRONG and the oracle goes RED.
;
;        Sequence (each result framed by grep-able markers the harness asserts):
;          1. SETDTA -> FINDFIRST "*.*" -> FINDNEXT loop: write each 8.3 filename
;             (CRLF-separated) between STORM-DIR-BEGIN / STORM-DIR-END. The disk
;             has a KNOWN set in a KNOWN order; the harness diffs it EXACTLY.
;          2. OPEN STORM.DAT (handle A) AND ALPHA.TXT (handle B) -- two handles
;             open AT ONCE (the multi-tenant path). LSEEK A past the first cluster
;             to a fixed offset and READ a 13-byte SIGNATURE that sits there
;             (forces a multi-cluster chain walk through the FAT cache + cluster
;             scratch -- the slowest path, so a PIT IRQ WILL land inside it).
;             Write it as STORM-SIG=<signature>.
;          3. READ 5 bytes from handle B at offset 0 -> STORM-B=<bytes> (proves
;             the second handle's independent position survived the storm).
;          4. CLOSE both, exit rc=0 (STORM-EXIT rc=0). Any error -> a fail-loud
;             marker + rc=1 (Rule 2).
;
; Ref:   os/milton/int21.c do_setdta (AH=1Ah), do_findfirst/next (AH=4Eh/4Fh:
;        g_dta + g_find), do_open (AH=3Dh), do_lseek (AH=42h), do_read (AH=3Fh:
;        FAT cache + cluster scratch), do_write (AH=40h: EBX=1 -> CON), do_close
;        (AH=3Eh), do_terminate (AH=4Ch); spec/find_data.h (fname at 0x1E, real DOS);
;        spec/memory_map.h (PROGRAM_IMAGE = 0x00030100). CLAUDE.md Law 1, Law 2,
;        Rule 2 (fail loud), Rule 11 (deterministic: nasm -f bin), Rule 12 (ASCII).
;
; Assembled: nasm -f bin os/milton/irqstorm_program.asm -o build/irqstorm_program.bin
; org PROGRAM_IMAGE so absolute references resolve at the load address.

bits 32
org 0x00030100                 ; == spec/memory_map.h PROGRAM_IMAGE

FIND_FNAME_OFF equ 0x1E        ; spec/find_data.h: fname at offset 0x1E (real DOS, dww)

; STORM.DAT signature: 13 ASCII bytes placed at SIG_OFF (past the first 512-byte
; cluster, so reading it forces a multi-cluster chain walk). Keep in sync with the
; Makefile mint (printf at seek=SIG_OFF).
SIG_OFF equ 1500               ; > 512 (one cluster on 1.44 MB FAT12)
SIG_LEN equ 13                 ; len("STORM-SIGNAL!")

start:
    ; ---- two concurrent opens FIRST (handle A = STORM.DAT, B = ALPHA.TXT) ----
    ; A is opened up front so the enumeration loop below can INTERLEAVE a slow
    ; multi-cluster READ on it between FINDNEXT steps -- that makes the
    ; enumeration window span many PIT ticks (each read is slow ATA PIO), so a
    ; PIT IRQ reliably lands WHILE g_find.next_index is live. Under the real
    ; kernel this is harmless (the read uses its own handle/position); under
    ; mutant A (the PIT ISR scribbles g_find.next_index) a tick landing here
    ; corrupts the enumeration -> the oracle goes RED. Two handles open at once
    ; also exercises the multi-tenant SFT path during the storm.
    mov eax, 0x00003D00        ; OPEN STORM.DAT (read) -> handle A
    mov edx, fname_storm
    int 0x21
    jc  open_a_failed
    mov [ha], eax

    mov eax, 0x00003D00        ; OPEN ALPHA.TXT (read) -> handle B (A still open)
    mov edx, fname_alpha
    int 0x21
    jc  open_b_failed
    mov [hb], eax

    ; ---- enumerate the directory (g_dta + g_find), interleaving a slow read ----
    mov eax, 0x00001A00        ; AH=1Ah SETDTA: EDX -> our find-data block
    mov edx, dta
    int 0x21

    mov edx, lbl_dir_begin
    call puts                  ; STORM-DIR-BEGIN

    mov eax, 0x00004E00        ; AH=4Eh FINDFIRST "*.*", ECX=0 (regular files)
    xor ecx, ecx
    mov edx, spec
    int 0x21
    jc  dir_done               ; CF=1 -> no files at all

.print:
    mov esi, dta + FIND_FNAME_OFF
    xor ecx, ecx
.strlen:
    mov al, [esi + ecx]
    test al, al
    jz  .have_len
    inc ecx
    jmp .strlen
.have_len:
    test ecx, ecx
    jz  .next                  ; empty name (defensive) -> skip

    mov eax, 0x00004000        ; AH=40h WRITE: EBX=1, ECX=len, EDX=name
    mov ebx, 1
    mov edx, dta + FIND_FNAME_OFF
    int 0x21
    call newline

    ; SLOW multi-cluster READ on handle A between names: LSEEK to SIG_OFF then
    ; read+discard the whole multi-cluster span -- interleaves a read with the
    ; live FINDFIRST/NEXT search state (proves the FAT cache/cluster scratch
    ; coexists with g_find -- the seam being hardened).
    call slow_read_a
    ; Then a calibrated BUSY-WAIT so the enumeration window spans MULTIPLE PIT
    ; ticks (in QEMU the ATA reads are near-instant, so without this the whole
    ; enumeration finishes inside one 10 ms PIT period and no IRQ ever lands while
    ; g_find is live). The spin is pure CPU with IF set, so IRQ0 (PIT) and IRQ1
    ; (the keystroke storm) fire THROUGHOUT it -- which is exactly the in-flight
    ; reentry pressure this oracle needs, and what makes mutant A (PIT scribbles
    ; g_find.next_index) reliably corrupt the enumeration.
    call spin_delay

.next:
    mov eax, 0x00004F00        ; AH=4Fh FINDNEXT
    int 0x21
    jnc .print                 ; CF=0 -> another match

dir_done:
    mov edx, lbl_dir_end
    call puts                  ; STORM-DIR-END

    ; ---- the authoritative multi-cluster positioned READ (STORM-SIG) ----
    ; LSEEK A to SIG_OFF (past the first cluster) -- SEEK_SET.
    mov ebx, [ha]
    mov eax, 0x00004200        ; AH=42h, AL=0
    xor ecx, ecx
    mov edx, SIG_OFF
    int 0x21
    jc  read_failed

    ; READ SIG_LEN bytes -> the signature (multi-cluster chain walk over the FAT
    ; cache + cluster scratch: the slow path a PIT IRQ lands inside).
    mov edx, lbl_sig
    call puts                  ; STORM-SIG=
    mov ebx, [ha]
    mov ecx, SIG_LEN
    call read_and_echo
    call newline

    ; READ 5 bytes from B at offset 0 -> proves B's position survived the storm.
    ; B has only ever been read here, so its position is still 0 -> "alpha".
    mov ebx, [hb]
    mov eax, 0x00004200        ; LSEEK B to 0 (defensive; B was never advanced)
    xor ecx, ecx
    xor edx, edx
    int 0x21
    mov edx, lbl_b
    call puts                  ; STORM-B=
    mov ebx, [hb]
    mov ecx, 5
    call read_and_echo
    call newline

    ; CLOSE both, exit rc=0.
    mov ebx, [ha]
    mov eax, 0x00003E00
    int 0x21
    mov ebx, [hb]
    mov eax, 0x00003E00
    int 0x21

    mov eax, 0x00004C00        ; rc=0
    int 0x21
    int 0x20

; slow_read_a: LSEEK handle A to SIG_OFF then READ+discard SIG_LEN bytes (a
; multi-cluster chain walk). Preserves all caller registers the enumeration loop
; relies on (it saves/restores EBX/ECX/EDX/ESI). Used to stretch the enumeration
; window across PIT ticks.
slow_read_a:
    push esi
    push ebx
    push ecx
    push edx
    mov ebx, [ha]
    mov eax, 0x00004200        ; LSEEK A, SEEK_SET SIG_OFF
    xor ecx, ecx
    mov edx, SIG_OFF
    int 0x21
    mov ebx, [ha]
    mov eax, 0x00003F00        ; READ SIG_LEN bytes into rbuf (discard)
    mov ecx, SIG_LEN
    mov edx, rbuf
    int 0x21
    pop edx
    pop ecx
    pop ebx
    pop esi
    ret

; spin_delay: a calibrated busy-wait (IF stays set, so IRQ0/IRQ1 fire throughout)
; long enough that the enumeration window spans several 100 Hz PIT ticks. Pure
; register loop; clobbers nothing the caller needs (saves/restores ECX). The
; count is generous (QEMU executes this fast) but bounded so the whole program
; stays well within the harness timeout.
SPIN_COUNT equ 800000
spin_delay:
    push ecx
    mov ecx, SPIN_COUNT
.loop:
    dec ecx
    jnz .loop
    pop ecx
    ret

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
read_failed:
    mov edx, msg_rd
    jmp die
die:
    mov ah, 0x09
    int 0x21
    mov eax, 0x00004C01        ; rc=1
    int 0x21
    int 0x20

; ---- data -----------------------------------------------------------------
spec:        db "*.*", 0
fname_storm: db "STORM.DAT", 0
fname_alpha: db "ALPHA.TXT", 0

lbl_dir_begin: db "STORM-DIR-BEGIN", 0x0D, 0x0A, "$"
lbl_dir_end:   db "STORM-DIR-END", 0x0D, 0x0A, "$"
lbl_sig:       db "STORM-SIG=$"
lbl_b:         db "STORM-B=$"

crlf:    db 0x0D, 0x0A

msg_oa:  db "STORM-OPENA-FAIL", 0x0D, 0x0A, "$"
msg_ob:  db "STORM-OPENB-FAIL", 0x0D, 0x0A, "$"
msg_rd:  db "STORM-READ-FAIL", 0x0D, 0x0A, "$"

; handle storage
ha:      dd 0
hb:      dd 0

; the 43-byte DTA find-data block (find_data_t)
dta:     times 43 db 0

; read buffer
BUFSZ equ 64
rbuf:    times BUFSZ db 0
