; more_program.asm -- the FAT-sourced flat (.COM-equivalent) MORE filter.
;
; beads: initech-m0dc (the classic DOS filters MORE/SORT/FIND). MORE paginates its
;        input one screenful (24 lines) at a time.  Like GREET/GOBBLE it is NOT
;        baked into the kernel: mcopy'd onto a FAT12 data disk as MORE.COM and
;        loaded BY NAME via load_program_from_fat / AH=4Bh EXEC.
;
; The device-vs-file fork (the key to making MORE emu-testable):
;   Authentic MS-DOS MORE paginates ONLY when its STANDARD OUTPUT is the CON
;   device.  When stdout is redirected to a FILE (or a pipe), there is no screen
;   to pause for, so MORE passes its input straight through with NO "-- More --"
;   prompt.  MORE learns which it is with INT 21h AH=44h AL=00h (IOCTL Get Device
;   Information) on handle 1: bit 15 (ISDEV) of the returned DX is SET for a
;   character device (CON), CLEAR for a disk file.  This is exactly how real MORE
;   decides, and it makes `TYPE FILE | MORE > OUT.TXT` (stdout = a file) a
;   deterministic PASSTHROUGH the emulator can gate -- a keywait would otherwise
;   block the headless harness forever.
;
; Semantics ground truth:
;   - MS-DOS 3.3 User's Guide "MORE": displays output one screen at a time.
;   - MS-DOS 3.3 Programmer's Reference / RBIL INT 21,44,00: AH=44h AL=00h,
;     BX=handle -> DX = device-information word; bit 15 = 1 iff handle is a
;     character device.  (os/milton/int21.c do_ioctl AL=00h returns
;     INT21_DEVINFO_CON with bit15 set for CON, INT21_DEVINFO_FILE with bit15
;     clear for a disk file.)
;   - When stdout IS CON: read stdin, echo to stdout, and after every 24 output
;     lines pause for a keypress (AH=08h CHARACTER INPUT, no echo) before
;     continuing -- the authentic pager.  (This path is present but is NOT
;     exercised by the headless emu gate, which drives the passthrough path.)
;
; Ref:   os/milton/int21.c do_ioctl (AH=44h AL=00h), do_read (AH=3Fh), do_write
;        (AH=40h), do_conin_noecho (AH=08h), do_terminate (AH=4Ch);
;        spec/memory_map.h (PROGRAM_IMAGE=0x00040100 == org);
;        MS-DOS 3.3 PRM INT 21h Fn 44h.  CLAUDE.md Law 1 (cite), Rule 2, Rule 11
;        (nasm -f bin deterministic), Rule 12 (ASCII).
;
; Assembled: nasm -f bin os/milton/more_program.asm -o build/more_program.bin
; Deployed:  mcopy build/more_program.bin ::MORE.COM  (NOT baked into kernel).
;
; Mutant (Rule 6): nasm -DMORE_MUTATE_DROP drops every input chunk after the
; first, so the passthrough is LOSSY and the "output == input" emu assertion goes
; RED (proves MORE actually forwards ALL bytes, not just the head).

bits 32
org 0x00040100                 ; == spec/memory_map.h PROGRAM_IMAGE

RBUF     equ 0x00050000         ; read buffer base
RBUF_MAX equ 0x00001000         ; 4 KiB chunk
PAGE_LINES equ 24               ; screenful before the pager pauses

start:
    ; ---- AH=44h AL=00h Get Device Information for handle 1 (stdout). ----
    mov eax, 0x00004400        ; AH=44h, AL=00h (get device info)
    mov ebx, 1                 ; handle 1 = stdout
    int 0x21
    ; DX = device-info word.  bit15 (0x8000) SET -> character device (CON, paginate);
    ; CLEAR -> disk file / pipe (passthrough).  CF set (error) -> be safe: passthrough.
    jc .passthrough
    test edx, 0x8000
    jz .passthrough
    jmp .paginate

; ---------------------------------------------------------------------------
; PASSTHROUGH: stdout is a file/pipe.  Copy stdin -> stdout in chunks, verbatim,
; no pause.  This is the authentic no-CON behavior AND the emu-gated path.
; ---------------------------------------------------------------------------
.passthrough:
.pt_loop:
    mov ah, 0x3F               ; READ handle 0
    xor ebx, ebx
    mov ecx, RBUF_MAX
    mov edx, RBUF
    int 0x21
    jc .done                   ; read error -> stop
    test eax, eax
    jz .done                   ; EOF
    mov ecx, eax               ; bytes to write
%ifdef MORE_MUTATE_DROP
    dec ecx                    ; MUTANT (Rule 6): forward one byte fewer per chunk
                               ; -> passthrough is LOSSY, "output == input" goes RED
%endif
    mov ah, 0x40               ; WRITE handle 1
    mov ebx, 1
    mov edx, RBUF
    int 0x21
    jmp .pt_loop

; ---------------------------------------------------------------------------
; PAGINATE: stdout is CON.  Echo stdin to stdout, counting newlines; after every
; PAGE_LINES lines wait for one key (AH=08h) before continuing.  Present for
; authenticity; the headless emu gate does not drive this path.
; ---------------------------------------------------------------------------
.paginate:
    xor ebp, ebp               ; line counter within the current screenful
.pg_read:
    mov ah, 0x3F
    xor ebx, ebx
    mov ecx, RBUF_MAX
    mov edx, RBUF
    int 0x21
    jc .done
    test eax, eax
    jz .done
    ; write byte-by-byte so we can pause exactly on the 24th newline.
    xor edi, edi               ; index into RBUF
    mov [chunk_len], eax
.pg_byte:
    cmp edi, [chunk_len]
    jae .pg_read
    push edi
    mov ecx, 1
    mov ah, 0x40
    mov ebx, 1
    mov edx, RBUF
    add edx, edi
    int 0x21
    pop edi
    mov al, [RBUF + edi]
    inc edi
    cmp al, 0x0A               ; LF ends a line
    jne .pg_byte
    inc ebp
    cmp ebp, PAGE_LINES
    jb .pg_byte
    ; screenful complete: pause for a key, then reset the counter.
    mov ah, 0x08               ; CHARACTER INPUT, no echo
    int 0x21
    xor ebp, ebp
    jmp .pg_byte

.done:
    mov ah, 0x4C
    mov al, 0
    int 0x21
    int 0x20                    ; defense in depth (Rule 2)

; --- scratch ----------------------------------------------------------------
chunk_len: dd 0
