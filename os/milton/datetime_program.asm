; datetime_program.asm -- the baked flat (.COM-equivalent) DATE/TIME program.
;
; beads: initech-yv9. THE in-emulator capability proof for the resident clock +
;        free-space + PSP query functions. Booted with a PINNED RTC
;        (-rtc base=2026-06-09T12:34:56,clock=vm -> reproducible), it issues
;        AH=2Ah GET DATE, AH=2Ch GET TIME, AH=36h GET DISK FREE SPACE, AH=62h GET
;        PSP and writes the decoded results to serial (handle 1), ONE FIELD PER
;        LINE with a unique tag the harness greps + asserts:
;          DT-YEAR=2026  DT-MON=6  DT-DAY=9  DT-DOW=2   (2026-06-09 = Tuesday)
;          DT-HOUR=12    DT-MIN=34 DT-SEC=56
;          DT-SPC=1  DT-BPS=512  DT-FREE=N  DT-TOTAL=N  (free > 0)
;          DT-PSP=NNNN                                  (nonzero)
;
; DESIGN: each query stashes its results to a scratch block (0x42000) disjoint
; from both the code page (0x40000) and the report buffer (0x41000). The WHOLE
; report is assembled into one buffer (one field per line: "<tag>=<decimal>") and
; written with a SINGLE AH=40h syscall. Every field is emitted TWICE: in the
; guest run exactly one build is dropped deterministically (reproducible even on
; KVM -- a pre-existing kernel/loader quirk, NOT in the INT 21h query logic, which
; the host oracle test-int21 validates), so duplicating guarantees every field
; has at least one clean, grep-able copy. The harness asserts each field by
; substring grep, so the duplicate is harmless.
;
; Ref:   os/milton/int21.c do_getdate (2Ah: CX=year, DH=month, DL=day, AL=dow),
;        do_gettime (2Ch: CH=hour, CL=min, DH=sec), do_getspace (36h: DL=drive;
;        AX=spc, BX=free, CX=bps, DX=total), do_getpsp (62h: BX=psp para),
;        do_write (40h: EBX=1 -> CON), do_terminate (4Ch). spec/memory_map.h
;        PROGRAM_IMAGE 0x00040100. CLAUDE.md Law 1, Rule 2, Rule 11, Rule 12.
;
; Assembled: nasm -f bin os/milton/datetime_program.asm -o build/datetime_program.bin
; [initech-o0td: PROGRAM_BASE shifted 0x30000->0x38000; org + scratch equs updated]
; [initech-re30.2: PROGRAM_BASE shifted 0x38000->0x40000; org + scratch equs updated]

bits 32
org 0x00040100

; LBUF holds the WHOLE report (~150 bytes); give it a generous 0x200 window so
; the growing buffer never reaches the register stash. The stash lives a full
; page higher (0x42000) -- disjoint from both the code page (0x40000) and the
; report buffer (0x41000..0x411FF). (A prior layout put R_* at 0x31080, which the
; growing report buffer overran -- clobbering not-yet-read values; that is the
; real bug behind the "one field comes out wrong/missing" symptom.)
; [initech-o0td: LBUF 0x31000->0x39000, R_* 0x32000->0x3A000]
; [initech-re30.2: LBUF 0x39000->0x41000, R_* 0x3A000->0x42000]
LBUF    equ 0x41000            ; report buffer (off the code page)
R_YEAR  equ 0x42000
R_MON   equ 0x42004
R_DAY   equ 0x42008
R_DOW   equ 0x4200C
R_HOUR  equ 0x42010
R_MIN   equ 0x42014
R_SEC   equ 0x42018
R_SPC   equ 0x4201C
R_BPS   equ 0x42020
R_FREE  equ 0x42024
R_TOTAL equ 0x42028
R_PSP   equ 0x4202C

start:
    ; ===== QUERY PHASE: run all four, stash each result as a full dword =====
    mov eax, 0x00002A00        ; GET DATE
    int 0x21
    movzx ebp, cx
    mov [R_YEAR], ebp
    movzx ebp, dh
    mov [R_MON], ebp
    movzx ebp, dl
    mov [R_DAY], ebp
    movzx ebp, al
    mov [R_DOW], ebp

    mov eax, 0x00002C00        ; GET TIME
    int 0x21
    ; Stash via the stack first (uniform push/pop -- the same robustness pattern
    ; used for the 36h group below). CH=hour, CL=min, DH=sec.
    push edx                    ; sec in DH
    push ecx                    ; hour in CH, min in CL
    pop ebp                     ; EBP = ...CH CL
    mov eax, ebp
    and eax, 0xFF               ; min  = CL
    mov [R_MIN], eax
    mov eax, ebp
    shr eax, 8
    and eax, 0xFF               ; hour = CH
    mov [R_HOUR], eax
    pop ebp                     ; EBP = ...DH DL
    mov eax, ebp
    shr eax, 8
    and eax, 0xFF               ; sec  = DH
    mov [R_SEC], eax

    mov edx, 1                 ; GET DISK FREE SPACE (DL=1 -> A:)
    mov eax, 0x00003600
    int 0x21
    ; Stash the four 16-bit results via the stack FIRST (a uniform, encoding-
    ; stable push/pop sequence), THEN move them to the scratch block. Keeping the
    ; load-bearing stash off the instruction slot immediately following this
    ; particular syscall return is more robust under the emulator's code cache.
    push edx                    ; total
    push ecx                    ; bps
    push ebx                    ; free
    push eax                    ; spc
    pop ebp                     ; spc
    and ebp, 0xFFFF
    mov [R_SPC], ebp
    pop ebp                     ; free
    and ebp, 0xFFFF
    mov [R_FREE], ebp
    pop ebp                     ; bps
    and ebp, 0xFFFF
    mov [R_BPS], ebp
    pop ebp                     ; total
    and ebp, 0xFFFF
    mov [R_TOTAL], ebp

    mov eax, 0x00006200        ; GET PSP
    int 0x21
    movzx ebp, bx
    mov [R_PSP], ebp

    ; ===== PRINT PHASE: build the ENTIRE multi-line report into ONE buffer, then
    ; write it with a SINGLE AH=40h syscall. Doing all the formatting with no
    ; intervening syscalls -- and one write at the end -- sidesteps the per-line
    ; syscall-adjacency fragility observed in-emulator.
    mov edi, LBUF
    mov esi, str_year
    mov eax, [R_YEAR]
    call build_field
    mov esi, str_mon
    mov eax, [R_MON]
    call build_field
    mov esi, str_day
    mov eax, [R_DAY]
    call build_field
    mov esi, str_dow
    mov eax, [R_DOW]
    call build_field
    mov esi, str_hour
    mov eax, [R_HOUR]
    call build_field
    mov esi, str_min
    mov eax, [R_MIN]
    call build_field
    mov esi, str_sec
    mov eax, [R_SEC]
    call build_field
    mov esi, str_spc
    mov eax, [R_SPC]
    call build_field
    mov esi, str_bps
    mov eax, [R_BPS]
    call build_field
    mov esi, str_free
    mov eax, [R_FREE]
    call build_field
    mov esi, str_total
    mov eax, [R_TOTAL]
    call build_field
    mov esi, str_psp
    mov eax, [R_PSP]
    call build_field
    ; (The earlier emit-every-field-TWICE workaround is REMOVED: the real cause
    ; was the loader writing the env block at PROGRAM_IMAGE+0x100, zeroing 2 bytes
    ; of any .COM > 256 B -- fixed by relocating ENV_BLOCK out of the image arena
    ; (beads initech-2og). Each field is emitted ONCE; the gate honestly verifies
    ; the fix -- if the corruption returned, a field would go missing and RED.)
    mov esi, str_done
    call build_tag
    ; one write of the whole buffer [LBUF..EDI)
    mov ecx, edi
    sub ecx, LBUF
    mov eax, 0x00004000
    mov ebx, 1
    mov edx, LBUF
    int 0x21

    mov eax, 0x00004C00
    int 0x21
    int 0x20

; ---- build_field: append "<ESI tag>" + decimal(EAX) + CRLF to [EDI]. No
; syscall (the whole report is written once at the end). ESI -> NUL-terminated
; tag (includes '='); EAX = value; EDI = running buffer cursor (advanced).
build_field:
    push eax
    call cat_esi                ; append the tag
    pop eax
    call cat_u32                ; append the decimal value
    mov byte [edi], 0x0D
    inc edi
    mov byte [edi], 0x0A
    inc edi
    ret

; build_tag: append just the ESI tag + CRLF to [EDI] (for DT-DONE).
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

str_year:  db "DT-YEAR=", 0
str_mon:   db "DT-MON=", 0
str_day:   db "DT-DAY=", 0
str_dow:   db "DT-DOW=", 0
str_hour:  db "DT-HOUR=", 0
str_min:   db "DT-MIN=", 0
str_sec:   db "DT-SEC=", 0
str_spc:   db "DT-SPC=", 0
str_bps:   db "DT-BPS=", 0
str_free:  db "DT-FREE=", 0
str_total: db "DT-TOTAL=", 0
str_psp:   db "DT-PSP=", 0
str_done:  db "DT-DONE", 0
crlf:      db 0x0D, 0x0A
