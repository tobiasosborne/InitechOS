; vect_program.asm -- the baked flat (.COM-equivalent) SETVECT/GETVECT + INT 24h
; critical-error end-to-end program.
;
; beads: initech-509.8 (INT 22/23/24 handlers + SETVECT/GETVECT + PSP-vector
;        save/restore across EXEC/EXIT; ADR-0003 DEC-10). THIS is the in-emulator
;        keystone (Oracle 2) for the acceptance criteria: "A critical error
;        presents MSG-DOS-0001 and processes Abort/Retry/Fail; vectors
;        saved/restored across EXEC/EXIT."
;
;        Sequence (each result framed by a grep-able serial marker the harness
;        asserts; serial is handle 1 -> int21_con_sink -> COM1):
;          1. AH=35h GETVECT 0x24 -> the ORIGINAL kernel critical-error handler.
;             Emit "V24PRE=<decimal>" so the harness can later compare it to the
;             post-EXIT vector kmain reports.
;          2. Emit "VECT-PROG-READY" so the harness injects the operator key
;             ('a') for the upcoming critical-error prompt (--keys-after).
;          3. `int $0x24` -- raise a critical error. The KERNEL handler runs (we
;             have NOT overridden it): int24_dispatch_body prints MSG-DOS-0001
;             ("Abort, Retry, Fail?") to CON (-> serial), reads ONE key (the
;             injected 'a'), and returns AL = the A/R/F action (Abort -> 1).
;             Emit "CRIT-AL=<decimal>" so the harness asserts AL == 1.
;          4. AH=25h SETVECT 0x24 to a BOGUS handler (0x00031234) -- the child
;             overrides its own critical-error vector, then EXITs WITHOUT
;             restoring it. The loader's EXIT path (load_program, beads 509.8)
;             MUST restore the parent's saved 0x24 vector into the live IDT; the
;             harness proves this by having kmain GETVECT 0x24 AFTER the program
;             exits and emit "V24POST=<decimal>" -- which must equal V24PRE.
;
; WHY decimal (not hex): the kernel's only serial number primitive is decimal
; (serial_putu); the asm mirrors datetime_program.asm's cat_u32 so V24PRE/V24POST
; are emitted in the SAME base kmain uses for V24POST -- an exact string compare.
;
; Ref:   os/milton/int21.c do_getvect (35h: EBX=handler, ES=0), do_setvect
;        (25h: AL=vec, EDX=handler), int24_dispatch_body (MSG-DOS-0001 + A/R/F),
;        do_write (40h: EBX=1 -> CON), do_terminate (4Ch); loader.c EXEC-save /
;        EXIT-restore of IDT 0x22/0x23/0x24; spec/memory_map.h PROGRAM_IMAGE
;        0x00040100. CLAUDE.md Law 1, Law 2, Rule 2, Rule 11, Rule 12.
;
; Assembled: nasm -f bin os/milton/vect_program.asm -o build/vect_program.bin
; ADDRESSING (Solution A): org 0x00040100 == PROGRAM_IMAGE.
; [initech-o0td: PROGRAM_BASE shifted 0x30000->0x38000; org + scratch equs updated]
; [initech-re30.2: PROGRAM_BASE shifted 0x38000->0x40000; org + scratch equs updated]

bits 32
org 0x00040100

BOGUS_24 equ 0x00041234        ; the child's bogus 0x24 handler (never restored); [initech-o0td: shifted +0x8000; initech-re30.2: shifted +0x8000 again]

start:
    ; ===== 1. GETVECT 0x24: record the ORIGINAL kernel handler ============
    mov eax, 0x00003524        ; AH=35h GETVECT, AL=0x24
    int 0x21
    mov [R_V24PRE], ebx        ; EBX = flat handler offset (do_getvect)

    ; Emit "V24PRE=<decimal>" CRLF.
    mov edi, LBUF
    mov esi, str_v24pre
    mov eax, [R_V24PRE]
    call build_field
    call flush_lbuf

    ; ===== 2. Announce readiness so the harness injects the operator key ===
    mov edi, LBUF
    mov esi, str_ready
    call build_tag
    call flush_lbuf

    ; ===== 3. Raise a critical error -- the KERNEL int24 handler runs ======
    ; It prints MSG-DOS-0001 ("Abort, Retry, Fail?") to CON (-> serial) and
    ; reads the injected key ('a' -> Abort -> AL=1), returning AL in the frame.
    xor eax, eax               ; clear AX so AL after int24 is purely the action
    int 0x24
    movzx ebx, al              ; EBX = AL = the A/R/F action (Abort -> 1)
    mov [R_CRITAL], ebx

    ; Emit a CRLF first so MSG-DOS-0001 (no trailing newline) and CRIT-AL land
    ; on their own lines for clean grep matching.
    mov edi, LBUF
    mov byte [edi], 0x0D
    inc edi
    mov byte [edi], 0x0A
    inc edi
    mov esi, str_crital
    mov eax, [R_CRITAL]
    call build_field
    call flush_lbuf

    ; ===== 4. SETVECT 0x24 to a bogus handler, then EXIT without restoring ==
    mov eax, 0x00002524        ; AH=25h SETVECT, AL=0x24
    mov edx, BOGUS_24          ; EDX = the bogus flat handler
    int 0x21

    ; AH=4Ch TERMINATE, AL=0 (clean exit). The loader's EXIT path restores the
    ; parent's 0x24 vector from the child PSP -- proven by kmain's V24POST.
    mov eax, 0x00004C00
    int 0x21
    int 0x20                   ; defense in depth (legacy terminate)

; ---------------------------------------------------------------------------
; flush_lbuf: write [LBUF..EDI) to stdout (handle 1) via AH=40h, then return.
flush_lbuf:
    mov ecx, edi
    sub ecx, LBUF
    mov eax, 0x00004000
    mov ebx, 1
    mov edx, LBUF
    int 0x21
    ret

; build_field: append "<ESI tag>" + decimal(EAX) + CRLF to [EDI]. ESI ->
; NUL-terminated tag (includes '='); EAX = value; EDI = running cursor.
build_field:
    push eax
    call cat_esi
    pop eax
    call cat_u32
    mov byte [edi], 0x0D
    inc edi
    mov byte [edi], 0x0A
    inc edi
    ret

; build_tag: append just the ESI tag + CRLF to [EDI].
build_tag:
    call cat_esi
    mov byte [edi], 0x0D
    inc edi
    mov byte [edi], 0x0A
    inc edi
    ret

; cat_esi: append NUL-terminated [ESI] to [EDI]; advance EDI.
cat_esi:
    mov al, [esi]
    test al, al
    jz .d
    mov [edi], al
    inc edi
    inc esi
    jmp cat_esi
.d:
    ret

; cat_u32: append EAX as unsigned decimal (no leading zeros) to [EDI].
cat_u32:
    push ebx
    push ecx
    push edx
    mov ecx, 0
    mov ebx, 10
.dv:
    xor edx, edx
    div ebx
    add dl, '0'
    push edx
    inc ecx
    test eax, eax
    jnz .dv
.pp:
    pop edx
    mov [edi], dl
    inc edi
    dec ecx
    jnz .pp
    pop edx
    pop ecx
    pop ebx
    ret

; Data -----------------------------------------------------------------------
str_v24pre: db "V24PRE=", 0
str_ready:  db "VECT-PROG-READY", 0
str_crital: db "CRIT-AL=", 0

; Scratch (off the code page; the report buffer lives a page higher than the
; image at 0x40100). LBUF assembles each line; R_* hold the recorded dwords.
; [initech-o0td: scratch pages shifted +0x8000: LBUF 0x31000->0x39000,
;  R_* 0x32000->0x3A000, keeping them above PROGRAM_BASE (0x38000)]
; [initech-re30.2: scratch pages shifted +0x8000 again: LBUF 0x39000->0x41000,
;  R_* 0x3A000->0x42000, keeping them above PROGRAM_BASE (0x40000)]
LBUF     equ 0x41000
R_V24PRE equ 0x42000
R_CRITAL equ 0x42004
