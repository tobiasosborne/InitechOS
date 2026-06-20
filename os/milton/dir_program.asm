; dir_program.asm -- the baked flat (.COM-equivalent) DIR program.
;
; beads: initech-509.5 (read-side file-handle functions). The end-to-end
;        integration program for FINDFIRST/FINDNEXT: it sets a DTA, calls AH=4Eh
;        FINDFIRST "*.*", then loops AH=4Fh FINDNEXT, writing each matched 8.3
;        filename (read from the 43-byte DTA find-data block at offset 0x1E) +
;        CRLF to stdout, until no more files -- the in-universe `DIR`.
;
; Ref:   docs/research/fs-mount-sft-ground-truth.md Sec 5.3 (the DIR program);
;        spec/find_data.h (the 43-byte find_data_t: fname at 0x1E, real DOS); os/milton/
;        int21.c do_setdta (AH=1Ah), do_findfirst (AH=4Eh: EDX -> spec, ECX=attr),
;        do_findnext (AH=4Fh), do_write (AH=40h: EBX=1 -> CON), do_terminate.
;        CLAUDE.md Law 1 (cite), Rule 11 (deterministic), Rule 12 (ASCII).
;
; Assembled: nasm -f bin os/milton/dir_program.asm -o build/dir_program.bin
; ADDRESSING (Solution A): org 0x00038100 == PROGRAM_IMAGE.
; [initech-o0td: PROGRAM_BASE shifted 0x30000->0x38000; org updated accordingly]

bits 32
org 0x00038100

FIND_FNAME_OFF equ 0x1E        ; spec/find_data.h: fname at offset 0x1E (real DOS, dww)

start:
    ; AH=1Ah SETDTA: EDX = flat ptr to our 43-byte find-data block.
    mov eax, 0x00001A00        ; AH=1Ah
    mov edx, dta
    int 0x21

    ; AH=4Eh FINDFIRST: EDX -> "*.*", ECX = attribute mask (0 = regular files).
    mov eax, 0x00004E00        ; AH=4Eh
    xor ecx, ecx
    mov edx, spec
    int 0x21
    jc  done                   ; CF=1 -> no files at all

.print:
    ; Write the NUL-terminated filename at dta+0x1E, then CRLF, to stdout.
    mov esi, dta + FIND_FNAME_OFF
    ; compute length into ECX
    xor ecx, ecx
.strlen:
    mov al, [esi + ecx]
    test al, al
    jz  .have_len
    inc ecx
    jmp .strlen
.have_len:
    test ecx, ecx
    jz  .next                  ; empty name (defensive) -> skip the write

    ; AH=40h WRITE: EBX=1 (stdout), ECX=len, EDX=ptr.
    mov eax, 0x00004000
    mov ebx, 1
    mov edx, dta + FIND_FNAME_OFF
    int 0x21

    ; CRLF
    mov eax, 0x00004000
    mov ebx, 1
    mov ecx, 2
    mov edx, crlf
    int 0x21

.next:
    ; AH=4Fh FINDNEXT: continue the search.
    mov eax, 0x00004F00
    int 0x21
    jnc .print                 ; CF=0 -> another match in the DTA

done:
    mov eax, 0x00004C00        ; AH=4Ch, AL=0
    int 0x21
    int 0x20

; Data -----------------------------------------------------------------------
spec:
    db "*.*", 0
crlf:
    db 0x0D, 0x0A
dta:
    times 43 db 0              ; the 43-byte DTA find-data block (find_data_t)
