; absdisk_program.asm -- the baked flat (.COM-equivalent) INT 25h / INT 26h
;                        ABSOLUTE-DISK round-trip self-test program.
;
; beads: initech-8403 (close the asm-stub coverage gap on int25_entry/int26_entry).
;        The HOST oracle (harness/diff/fat_diff/test_absdisk.c, beads initech-4mq7)
;        drives int25_dispatch / int26_dispatch DIRECTLY (C function calls); the
;        asm  int25_entry/int26_entry -> dispatch -> IRETD  return path is invoked
;        by NO host/emu gate. THIS program is the in-emulator keystone that closes
;        that gap: a real guest issues `int $0x26` then `int $0x25` through the live
;        IDT trap gates 0x25/0x26 and proves the WHOLE asm round-trip end-to-end.
;        It MIRRORS vect_program.asm (the int24_entry emu keystone) for INT 24h.
;
;        Sequence (each result framed by a grep-able serial marker the harness
;        asserts; serial is handle 1 -> int21_con_sink -> COM1):
;          1. Fill a 512-byte buffer with the SAME deterministic pattern the host
;             oracle uses: byte i = (i*0x6D + (LBA & 0xFF)) & 0xFF -- a pure
;             function of (index, LBA), no wall-clock, no rand (Rule 11).
;          2. `int $0x26` ABSOLUTE WRITE: AL=0 (drive A:), CX=1 (one sector),
;             DX=SCRATCH_LBA, EBX=pattern buffer. CF=0 success. Emit "ABS-W26=OK"
;             on CF=0 (else "ABS-W26-FAIL AX=<dec>"). This exercises int26_entry ->
;             int26_dispatch -> the bound g_absdisk.write seam -> IRETD.
;          3. `int $0x25` ABSOLUTE READ: AL=0, CX=1, DX=SCRATCH_LBA, EBX=readback
;             buffer. CF=0 success. Emit "ABS-R25=OK" (else "ABS-R25-FAIL AX=<dec>").
;             This exercises int25_entry -> int25_dispatch -> g_absdisk.read -> IRETD.
;          4. Compare the 512 read-back bytes to the pattern. Emit "ABS-RT=OK" if
;             EVERY byte matches (the asm WRITE landed and the asm READ returned it),
;             else "ABS-RT-FAIL idx=<dec>" at the first mismatch.
;          5. AH=4Ch EXIT rc=0.
;
;        SAFE SCRATCH LBA (Rule 2 / non-corruption): SCRATCH_LBA = 2879 ==
;        (1.44MB FAT12 total_logical_sectors - 1). On an mformat -f 1440 volume the
;        BPB total_sectors_16 == 2880, so LBA 2879 is the LAST data sector, well
;        past the boot sector (0), both FATs, and the root directory -- and on the
;        BLANK scratch disk the harness mounts it is FREE (no file occupies it).
;        This is the IDENTICAL safe LBA the host oracle picks (total-1, FAT entry
;        asserted free); writing it cannot corrupt boot/FAT/root sectors.
;
; WHY decimal: the kernel's only serial number primitive (and this program's
; cat_u32) is decimal -- mirrors vect_program.asm / datetime_program.asm.
;
; Ref:   spec/absdisk_int2526.json (the LOCKED contract; ABI: AL=drive, CX/ECX=
;        count, DX/EDX=start LBA, EBX=flat buffer, CF=status); os/milton/int21.c
;        absdisk_body / int25_dispatch / int26_dispatch; os/milton/isr.asm
;        int25_entry / int26_entry; do_write (40h: EBX=1 -> CON), do_terminate
;        (4Ch); spec/memory_map.h PROGRAM_IMAGE 0x00030100. ADR-0003 DEC-15.
;        CLAUDE.md Law 1, Law 2, Rule 2, Rule 11, Rule 12.
;
; Assembled: nasm -f bin os/milton/absdisk_program.asm -o build/absdisk_program.bin
; ADDRESSING (Solution A): org 0x00030100 == PROGRAM_IMAGE (mirrors vect_program).

bits 32
org 0x00030100

SCRATCH_LBA equ 2879           ; == 1.44MB FAT12 total_sectors_16 (2880) - 1 (SAFE)
LBA_LOW     equ (SCRATCH_LBA & 0xFF)   ; pattern seed byte == 63 (0x3F) for LBA 2879
SECTOR      equ 512

start:
    ; ===== 1. Fill PATBUF[0..511] with byte i = (i*0x6D + LBA_LOW) & 0xFF =====
    ; Pure (index, LBA): EXACT match to test_absdisk.c fill_pattern (Rule 11).
    mov edi, PATBUF
    xor ecx, ecx               ; ECX = byte index i = 0..511
.fill:
    mov eax, ecx
    imul eax, eax, 0x6D        ; i * 0x6D
    add eax, LBA_LOW           ; + (LBA & 0xFF)
    mov [edi + ecx], al        ; store low byte (& 0xFF implicit)
    inc ecx
    cmp ecx, SECTOR
    jb  .fill

    ; ===== 2. INT 26h ABSOLUTE WRITE: AL=0, CX=1, DX=SCRATCH_LBA, EBX=PATBUF ==
    ; Exercises int26_entry -> int26_dispatch -> g_absdisk.write -> IRETD.
    xor eax, eax               ; AL = 0 (drive A:; the single mounted volume)
    mov ecx, 1                 ; CX = 1 sector
    mov edx, SCRATCH_LBA       ; DX = start LBA (the DEC-15 register role: DX=LBA)
    mov ebx, PATBUF            ; EBX = flat buffer (DEC-15: buffer in EBX)
    int 0x26
    jc  .w26_fail
    ; Emit "ABS-W26=OK" CRLF.
    mov edi, LBUF
    mov esi, str_w26_ok
    call build_tag
    call flush_lbuf
    jmp .do_read
.w26_fail:
    ; Emit "ABS-W26-FAIL AX=<dec>" (AX = the absolute-disk error pair).
    movzx eax, ax
    mov [R_AX], eax
    mov edi, LBUF
    mov esi, str_w26_fail
    mov eax, [R_AX]
    call build_field
    call flush_lbuf
    jmp .exit                  ; a failed write means the round-trip cannot pass

.do_read:
    ; ===== 3. INT 25h ABSOLUTE READ: AL=0, CX=1, DX=SCRATCH_LBA, EBX=RDBUF ====
    ; Exercises int25_entry -> int25_dispatch -> g_absdisk.read -> IRETD.
    xor eax, eax               ; AL = 0
    mov ecx, 1                 ; CX = 1 sector
    mov edx, SCRATCH_LBA       ; DX = start LBA
    mov ebx, RDBUF             ; EBX = read-back buffer (distinct from PATBUF)
    int 0x25
    jc  .r25_fail
    ; Emit "ABS-R25=OK" CRLF.
    mov edi, LBUF
    mov esi, str_r25_ok
    call build_tag
    call flush_lbuf
    jmp .compare
.r25_fail:
    movzx eax, ax
    mov [R_AX], eax
    mov edi, LBUF
    mov esi, str_r25_fail
    mov eax, [R_AX]
    call build_field
    call flush_lbuf
    jmp .exit

.compare:
    ; ===== 4. Byte-compare RDBUF[i] == PATBUF[i] for all 512 bytes ===========
    ; If every byte matches, the asm WRITE landed at SCRATCH_LBA and the asm READ
    ; returned EXACTLY it -- the int26_entry/int25_entry round-trip is proven.
    xor ecx, ecx               ; ECX = index i
.cmp_loop:
    mov al, [PATBUF + ecx]
    cmp al, [RDBUF + ecx]
    jne .rt_fail
    inc ecx
    cmp ecx, SECTOR
    jb  .cmp_loop
    ; All bytes matched -- emit "ABS-RT=OK" CRLF.
    mov edi, LBUF
    mov esi, str_rt_ok
    call build_tag
    call flush_lbuf
    jmp .exit
.rt_fail:
    ; Emit "ABS-RT-FAIL idx=<dec>" at the first mismatching index (fail loud).
    mov [R_AX], ecx
    mov edi, LBUF
    mov esi, str_rt_fail
    mov eax, [R_AX]
    call build_field
    call flush_lbuf

.exit:
    ; ===== 5. AH=4Ch TERMINATE rc=0 (clean exit) ============================
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
str_w26_ok:   db "ABS-W26=OK", 0
str_w26_fail: db "ABS-W26-FAIL AX=", 0
str_r25_ok:   db "ABS-R25=OK", 0
str_r25_fail: db "ABS-R25-FAIL AX=", 0
str_rt_ok:    db "ABS-RT=OK", 0
str_rt_fail:  db "ABS-RT-FAIL idx=", 0

; Scratch (off the code page; mirrors vect_program.asm's layout). LBUF assembles
; each serial line; PATBUF holds the written pattern; RDBUF the read-back; R_AX a
; recorded dword for the *-FAIL fields.
LBUF   equ 0x31000             ; line-assembly scratch (one page above the image)
PATBUF equ 0x31200             ; 512B written pattern
RDBUF  equ 0x31400             ; 512B read-back (distinct from PATBUF)
R_AX   equ 0x31600             ; recorded value for the FAIL diagnostic fields
