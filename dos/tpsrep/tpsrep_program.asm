; tpsrep_program.asm -- TPSREP.COM, the INITECH TPS REPORT GENERATOR
; (beads initech-8479.1; the flagship vestigial-feature program).
;
; A command-line DOS utility in the 1980s corporate manner. It is run from the
; DOS prompt (exit the desktop first) as   TPSREP table.DBF   and:
;   1. takes the table name from the default FCB COMMAND.COM formatted at
;      PSP:5Ch (IBM DOS 3.30 Technical Reference p. 7-8);
;   2. enforces the OPERATING PROCEDURE of spec/tps_ritual.json (R1..R5) --
;      parameter file, top-of-form alignment (the alignment test line; the
;      operator advances the form with the space bar EXACTLY 7 times -- a count
;      the program NEVER states: it is taught only by the Operating Procedures,
;      OP-7.4, beads initech-5igf), report number keyed twice, memo
;      acknowledgement, cover-sheet attestation naming FORM TPS-1 (REV. 10/89);
;   3. reads the dBASE III table (header, field descriptors, records; deleted
;      records skipped) through FCB random reads of 128-byte records;
;   4. lists the .DBF files on file with FCB search first/next;
;   5. deletes TPSREPT.BAK, renames TPSREPT.TXT to TPSREPT.BAK, creates
;      TPSREPT.TXT and writes the paginated report (cover sheet first) to the
;      console AND, through FCB sequential writes, to TPSREPT.TXT.
; It issues NO handle call (AH=3Ch..46h): every file byte moves through the FCB
; API (AH=0Fh-24h). That is why the FCB API exists (initech-509.9).
;
; ONE source, two builds (the fcbx_program.asm pattern): -DREALDOS gives the
; 16-bit .COM that mints the golden under real MS-DOS 6.22; the default build is
; the flat-32 InitechOS program at PROGRAM_IMAGE 0x40100. 32-bit registers
; throughout; INT 21h pointers in EDX serve DS:DX and the flat ABI alike.
;
; Ref: IBM DOS 3.30 Technical Reference 80X0945 pp. 6-67..6-88 (FCB calls),
; pp. 7-8, 7-12..7-16 (PSP default FCBs, FCB layout); dBASE III PLUS .DBF
; header (../dbase3-decomp: 32-byte header, 32-byte field descriptors ended by
; 0Dh, records led by a deletion flag). Rule 12 ASCII; Rule 11 deterministic.
; Fault handling (audit 2026-10-23-das L004/L028/L029, bead initech-ycvk): every
; FCB write/close status is checked (TPS-009), LINES= must be a number
; (TPS-010), every live record's PAGES must be a number (TPS-011).
; Backup rotation must succeed before replacing the latest report
; (spec/tps_ritual.json E12, initech-56ch: TPS-012, return code 10).
; Mutants (Rule 6): -DTPS_MUTATE_NOWRCHK ignores the write status;
; -DTPS_MUTATE_LINESOK / -DTPS_MUTATE_PAGESOK skip the number checks;
; -DTPS_MUTATE_COUNT6 accepts 6 space-bar presses;
; -DTPS_MUTATE_SAY7 states the count in the R2 prompt (the tacit rule: the
; count and the key are written only in the manual, spec/tps_ritual.json R2).
; -DTPS_MUTATE_UNCHECKED_BACKUP restores unchecked delete/rename (initech-56ch).

%ifdef REALDOS
bits 16
org 0x0100
PSP equ 0x0000
%else
bits 32
org 0x00040100                 ; == spec/memory_map.h PROGRAM_IMAGE
PSP equ 0x00040000
%endif

%ifdef TPS_MUTATE_COUNT6
FORM_COUNT equ 6               ; MUTANT (Rule 6): six presses accepted; the
                               ; wording is unchanged, so only BEHAVIOUR differs
%else
FORM_COUNT equ 7               ; spec/tps_ritual.json R2 "count": 7
%endif

CURBLK equ 0x0C
RECSZ  equ 0x0E
FSIZE  equ 0x10
CURREC equ 0x20
RANDOM equ 0x21
PBUF_MAX equ 1024

%macro SAY 1+
    jmp %%over
%%s: db %1, '$'
%%over:
    mov edx, %%s
    call puts
%endmacro

%macro FCB 2                    ; AH=fn, EDX=fcb -> AL
    mov edx, %2
    mov ah, %1
    int 0x21
%endmacro

; ===========================================================================
start:
    ; The table FCB comes from PSP:5Ch (12 bytes: drive + name). Copy it out
    ; before any use: an opened FCB at 5Ch would overrun FCB #2 at 6Ch.
    xor ecx, ecx
.cp: mov al, [PSP + 0x5C + ecx]
    mov [tblfcb + ecx], al
    inc ecx
    cmp ecx, 12
    jne .cp

    mov edx, m_banner
    call puts

    ; ---- R1: the parameter file -----------------------------------------
    FCB 0x0F, parmfcb
    test al, al
    jz .r1_open
    mov edx, m_r1a
    call lineout
    mov edx, m_op73
    call lineout
    mov al, 1
    jmp finish
.r1_open:
    mov edx, dta
    call setdta
    xor edi, edi                ; bytes read into pbuf
.r1_read:
    FCB 0x14, parmfcb           ; sequential read, 128-byte records
    cmp al, 1
    je .r1_eof                  ; EOF, nothing read
    xor ecx, ecx
.r1_cp:
    cmp edi, PBUF_MAX
    jae .r1_eof
    mov dl, [dta + ecx]
    mov [pbuf + edi], dl
    inc edi
    inc ecx
    cmp ecx, 128
    jne .r1_cp
    cmp al, 3                   ; partial last record -> done
    jne .r1_read
.r1_eof:
    mov byte [pbuf + edi], 0
    FCB 0x10, parmfcb
    mov esi, k_title
    call getparam
    mov [p_title], esi
    mov [l_title], ecx
    mov esi, k_div
    call getparam
    mov [p_div], esi
    mov [l_div], ecx
    mov esi, k_memo
    call getparam
    mov [p_memo], esi
    mov [l_memo], ecx
    mov esi, k_lines
    call getparam
%ifndef TPS_MUTATE_LINESOK
    test ecx, ecx               ; LINES= must be a decimal number (TPS-010);
    jz .badlines                ; only the documented 0 is taken as 1
    xor ebx, ebx
.lchk:
    movzx eax, byte [esi + ebx]
    sub eax, '0'
    cmp eax, 9
    ja .badlines
    inc ebx
    cmp ebx, ecx
    jb .lchk
%endif
    call atoi                   ; EAX = LINES value
    test eax, eax
    jnz .r1_lines
    mov eax, 1
.r1_lines:
    mov [lines], eax
    jmp .lines_ok
.badlines:
    mov edx, m_r10
    call lineout
    mov edx, m_op73
    call lineout
    mov al, 1
    jmp finish
.lines_ok:

    ; ---- the table named on the command line ------------------------------
    cmp byte [tblfcb + 1], ' '
    je .no_table
    FCB 0x0F, tblfcb
    test al, al
    jz .tbl_open
.no_table:
    mov edx, m_e7a
    call lineout
    mov edx, m_e7b
    call lineout
    mov al, 7
    jmp finish
.tbl_open:
    mov word [tblfcb + RECSZ], 128
    mov dword [cached], 0xFFFFFFFF
    xor eax, eax
    call rdbyte
    cmp al, 0x03                ; dBASE III table signature
    jne .not_tps
    mov eax, 1
    call rdbyte
    mov [upd_yy], al
    mov eax, 2
    call rdbyte
    mov [upd_mm], al
    mov eax, 3
    call rdbyte
    mov [upd_dd], al
    mov eax, 4
    call rd32
    mov [nrecs], eax
    mov eax, 8
    call rd16
    mov [hdrlen], eax
    mov eax, 10
    call rd16
    mov [reclen], eax
    ; walk the 32-byte field descriptors from offset 32 until 0Dh, summing
    ; lengths (the record starts with the deletion flag, offset 1 = field 1)
    mov dword [fpos], 32
    mov dword [foff], 1
.fd:
    mov eax, [fpos]
    call rdbyte
    cmp al, 0x0D
    je .fd_done
    xor ecx, ecx                ; name -> fname (11 bytes)
.fd_n:
    mov eax, [fpos]
    add eax, ecx
    call rdbyte
    mov [fname + ecx], al
    inc ecx
    cmp ecx, 11
    jne .fd_n
    mov eax, [fpos]
    add eax, 16
    call rdbyte
    movzx eax, al
    mov [flen], eax
    xor ebx, ebx                ; match against the six required fields
.fd_m:
    mov esi, [reqtab + ebx * 4]
    call nameeq
    jne .fd_mn
    mov eax, [foff]
    mov [offtab + ebx * 4], eax
    mov eax, [flen]
    mov [lentab + ebx * 4], eax
.fd_mn:
    inc ebx
    cmp ebx, 6
    jne .fd_m
    mov eax, [flen]
    add [foff], eax
    add dword [fpos], 32
    cmp dword [fpos], 32 * 130
    jb .fd
.fd_done:
    xor ebx, ebx
.chk:
    cmp dword [lentab + ebx * 4], 0
    je .missing
    inc ebx
    cmp ebx, 6
    jne .chk
%ifndef TPS_MUTATE_PAGESOK
    call validpages
%endif
    jmp ritual
.missing:
    mov edx, m_e6a
    call puts
    mov esi, [reqtab + ebx * 4]
    call putz
    mov edx, m_e6b
    call lineout
    mov al, 6
    jmp finish
.not_tps:
    mov edx, m_e6a
    call puts
    mov esi, s_sig
    call putz
    mov edx, m_e6b
    call lineout
    mov al, 6
    jmp finish

; PAGES must be a number in every live record: blanks, digits, blanks (the
; dBASE III numeric field, right-justified; all blank is an empty field).
; Anything else is refused before the ritual and before any file is touched
; (TPS-011, audit L029). EBX/ECX/EDX/ESI are free here.
validpages:
    mov dword [rec], 0
.r: mov eax, [rec]
    cmp eax, [nrecs]
    jae .ok
    mul dword [reclen]
    add eax, [hdrlen]
    mov [rbase], eax
    call rdbyte                 ; deletion flag
    cmp al, '*'
    je .n
    mov ebx, 3                  ; PAGES
    call fieldptr
    mov ecx, [lentab + ebx * 4]
    xor edx, edx                ; 0 leading blanks, 1 digits, 2 trailing blanks
.b: mov eax, [fptr]
    call rdbyte
    cmp al, ' '
    je .sp
    cmp al, '0'
    jb .bad
    cmp al, '9'
    ja .bad
    cmp edx, 2
    je .bad                     ; a digit after a trailing blank
    mov edx, 1
    jmp .nx
.sp: test edx, edx
    jz .nx
    mov edx, 2
.nx: inc dword [fptr]
    dec ecx
    jnz .b
.n: inc dword [rec]
    jmp .r
.bad:
    mov edx, m_e11a
    call puts
    mov eax, [rec]
    inc eax
    call putdec
    mov edx, m_e11b
    call lineout
    mov al, 6
    jmp finish
.ok: ret

; ---- R2..R5: the operating procedure --------------------------------------
; R2 top of form alignment (spec/tps_ritual.json R2; Ref: IBM 4230 Printer
; User's Guide SA40-0593-04 pp. 5-4..5-5, the Top of Form Alignment test
; reprinted until the form is accepted). The prompt names neither the count
; nor the key (tacit, OP-7.4); a wrong count is refused and the prompt reprinted.
ritual:
    mov edx, m_r2a
    call lineout
    mov edx, m_r2b
    call lineout
    mov edx, m_r2t
    call lineout
    xor ebx, ebx                ; presses this attempt
.r2k:
    call getkey
    cmp al, 0x0D
    je .r2e
    cmp al, ' '
    jne .r2k                    ; other keys are ignored
    inc ebx
    mov al, '.'
    call putc
    jmp .r2k
.r2e:
    call crlf
    cmp ebx, FORM_COUNT
    je .r2ok
    mov edx, m_r2c
    call puts
    mov eax, ebx
    call putdec
    mov edx, m_r2d
    call lineout
    jmp ritual                  ; reprint the alignment prompt, count reset
.r2ok:
    mov edx, m_r2ok
    call lineout

.r3:
    call crlf
    mov edx, m_r3a
    call puts
    mov edi, rno1
    mov ebx, 6
    call getfield
    mov [rno1len], ecx
    mov edx, m_r3b
    call puts
    mov edi, rno2
    mov ebx, 6
    call getfield
    cmp ecx, [rno1len]
    jne .r3bad
    test ecx, ecx
    jz .r3bad
    xor edx, edx
.r3c:
    mov al, [rno1 + edx]
    cmp al, [rno2 + edx]
    jne .r3bad
    inc edx
    cmp edx, ecx
    jne .r3c
    jmp .r4
.r3bad:
    mov edx, m_r3c
    call lineout
    jmp .r3

.r4:
    call crlf
    mov edx, m_r4a
    call puts
    mov edi, memo
    mov ebx, 8
    call getfield
    cmp ecx, [l_memo]
    jne .r4bad
    mov esi, [p_memo]
    xor edx, edx
.r4c:
    test ecx, ecx
    jz .r5
    mov al, [memo + edx]
    cmp al, [esi + edx]
    jne .r4bad
    inc edx
    dec ecx
    jmp .r4c
.r4bad:
    mov edx, m_r4b
    call lineout
    jmp .r4

.r5:
    call crlf
    mov edx, m_r5a
    call puts
.r5k:
    call getkey
    and al, 0xDF                ; fold to upper case
    cmp al, 'Y'
    je .r5y
    cmp al, 'N'
    jne .r5k
    mov al, 'N'
    call putc
    call crlf
    mov edx, m_r5b
    call puts
    mov esi, [p_memo]
    mov ecx, [l_memo]
    call putn
    mov edx, m_r5c
    call lineout
    mov edx, m_r5d
    call lineout
    mov al, 5
    jmp finish
.r5y:
    mov al, 'Y'
    call putc
    call crlf

; ---- the report ---------------------------------------------------------
report:
    ; tables on file: FCB search first/next over ????????.DBF
    mov edx, fdta
    call setdta
    mov dword [tlen], 0
    FCB 0x11, srchfcb
.ls:
    test al, al
    jnz .ls_done
    cmp dword [tlen], 0
    je .ls_first
    mov al, ','
    call tadd
    mov al, ' '
    call tadd
.ls_first:
    mov ebx, fdta + 1
    call nameout
    FCB 0x12, srchfcb
    jmp .ls
.ls_done:
    ; keep one generation: delete TPSREPT.BAK, rename TPSREPT.TXT -> .BAK
%ifdef TPS_MUTATE_UNCHECKED_BACKUP
    FCB 0x13, bakfcb
    FCB 0x17, renfcb
    mov [renamed], al
%else
    ; Ref: IBM DOS 3.30 TR pp. 6-70/71, 6-74, 6-79/80; FCBX real-DOS
    ; golden T15/T16: AL=FFh for a rename collision or read-only delete.
    ; Missing files are normal on the first run, so search before rotating.
    ; Never create/truncate the latest report unless its rename succeeded.
    mov byte [renamed], 0xFF
    FCB 0x11, outsearch        ; hidden/system reports must not look absent
    test al, al
    jnz .backup_ready          ; no previous report; leave any .BAK alone
    FCB 0x11, bakfcb
    test al, al
    jnz .backup_rename         ; no visible .BAK to delete
    FCB 0x13, bakfcb
    test al, al
    jnz .backup_failed
.backup_rename:
    FCB 0x17, renfcb
    mov [renamed], al
    test al, al
    jz .backup_ready
.backup_failed:
    mov edx, m_e12
    call lineout
    mov al, 10
    jmp finish
.backup_ready:
%endif
    FCB 0x16, outfcb
    test al, al
    jz .created
    mov edx, m_e8
    call lineout
    mov al, 8
    jmp finish
.created:
    mov byte [outfcb + CURREC], 0
    mov dword [opos], 0
    mov byte [tofile], 1
    call crlf
    call cover
    ; detail pages
    mov dword [page], 0
    mov dword [onpage], 0
    mov dword [rec], 0
    mov dword [listed], 0
    mov dword [pagesum], 0
    mov dword [nocover], 0
.rec:
    mov eax, [rec]
    cmp eax, [nrecs]
    jae .recs_done
    mul dword [reclen]
    add eax, [hdrlen]
    mov [rbase], eax
    call rdbyte                 ; deletion flag
    cmp al, '*'
    je .next
    mov eax, [onpage]
    test eax, eax
    jz .newpage
    cmp eax, [lines]
    jb .row
.newpage:
    call pagebreak
    call pagehead
    mov dword [onpage], 0
.row:
    call detail
    inc dword [onpage]
    inc dword [listed]
.next:
    inc dword [rec]
    jmp .rec
.recs_done:
    call totals
    call flushout
    FCB 0x10, outfcb
%ifndef TPS_MUTATE_NOWRCHK
    test al, al
    jnz writefail
%endif
    mov byte [tofile], 0
    call crlf
    mov edx, m_done1
    call lineout
    cmp byte [renamed], 0
    jne .nobak
    mov edx, m_done2
    call lineout
.nobak:
    xor al, al

; AL = return code
finish:
    push eax
    mov edx, m_end1
    call puts
    pop eax
    push eax
    movzx eax, al
    call putdec
    mov edx, m_end2
    call lineout
    pop eax
    mov ah, 0x4C
    int 0x21

; ---- report pieces --------------------------------------------------------
cover:
    call rule
    mov esi, s_tps
    call ctr
    mov esi, [p_title]
    mov ecx, [l_title]
    call ctrn
    call rule
    mov esi, s_form
    call zline
    call nl
    mov esi, s_c1
    call zout
    call rnoout
    call nl
    mov esi, s_c2
    call zout
    mov esi, [p_div]
    mov ecx, [l_div]
    call outn
    call nl
    mov esi, s_c3
    call zout
    mov ebx, tblfcb + 1
    call nameout_o
    call nl
    mov esi, s_c4
    call zout
    movzx eax, byte [upd_mm]
    call out2
    mov al, '/'
    call out
    movzx eax, byte [upd_dd]
    call out2
    mov al, '/'
    call out
    movzx eax, byte [upd_yy]
    call out2
    call nl
    mov esi, s_c5
    call zout
    mov esi, tlist
    mov ecx, [tlen]
    call outn
    call nl
    mov esi, s_c6
    call zout
    mov esi, [p_memo]
    mov ecx, [l_memo]
    call outn
    call nl
    mov esi, s_c7
    call zout
    mov eax, [nrecs]
    call outdec
    call nl
    call nl
    mov esi, s_c8
    call zline
    mov esi, s_c9
    call zline
    jmp rule

pagehead:
    inc dword [page]
    mov esi, s_h1
    call zout
    call rnoout
    mov esi, s_h2
    call zout
    mov eax, [page]
    call outdec
    call nl
    call nl
    mov esi, s_h3
    call zline
    mov esi, s_h4
    jmp zline

detail:                         ; one row from the record at [rbase]
    mov ebx, 0                  ; RPT_NO, zero-filled to 6
    call fieldptr
    call zfill6
    mov esi, s_2sp
    call zout
    mov ebx, 1                  ; EMPLOYEE (20)
    call fieldout
    mov esi, s_2sp
    call zout
    mov ebx, 2                  ; DEPT (12)
    call fieldout
    mov esi, s_4sp
    call zout
    mov ebx, 3                  ; PAGES (3), summed
    call fieldout
    mov ebx, 3
    call fieldval
    add [pagesum], eax
    mov esi, s_2sp
    call zout
    mov ebx, 5                  ; FILED CCYYMMDD -> MM/DD/YY
    call fieldptr
    mov ecx, 4
    call fbytes2
    mov al, '/'
    call out
    mov ecx, 6
    call fbytes2
    mov al, '/'
    call out
    mov ecx, 2
    call fbytes2
    mov esi, s_2sp
    call zout
    mov ebx, 4                  ; COVER (L)
    call fieldptr
    mov eax, [fptr]
    call rdbyte
    and al, 0xDF
    cmp al, 'T'
    je .yes
    cmp al, 'Y'
    je .yes
    inc dword [nocover]
    mov esi, s_no
    call zout
    jmp nl
.yes:
    mov esi, s_yes
    call zout
    jmp nl

totals:
    call nl
    mov esi, s_t1
    call zout
    mov eax, [listed]
    call outdec
    call nl
    mov esi, s_t2
    call zout
    mov eax, [pagesum]
    call outdec
    call nl
    mov esi, s_t3
    call zout
    mov eax, [nocover]
    call outdec
    call nl
    cmp dword [nocover], 0
    je .ok
    mov esi, s_t4
    call zout
    mov esi, [p_memo]
    mov ecx, [l_memo]
    call outn
    mov al, '.'
    call out
    call nl
.ok:
    call nl
    mov esi, s_t5
    jmp zline

rule:
    mov ecx, 64
.r: mov al, '='
    call out
    dec ecx
    jnz .r
    jmp nl

pagebreak:                      ; file: form feed; console: a blank line
    mov byte [conoff], 1
    mov al, 0x0C
    call out
    mov byte [conoff], 0
    mov byte [fileoff], 1
    call nl
    mov byte [fileoff], 0
    ret

rnoout:                         ; the report number, zero-filled to 6
    mov ecx, 6
    sub ecx, [rno1len]
.z: test ecx, ecx
    jz .d
    mov al, '0'
    call out
    dec ecx
    jmp .z
.d: mov esi, rno1
    mov ecx, [rno1len]
    jmp outn

ctr:                            ; centre the ASCIIZ at ESI in 64 columns
    call zlen
ctrn:                           ; centre ESI/ECX in 64 columns
    push esi
    push ecx
    mov eax, 64
    sub eax, ecx
    shr eax, 1
.p: test eax, eax
    jz .t
    push eax
    mov al, ' '
    call out
    pop eax
    dec eax
    jmp .p
.t: pop ecx
    pop esi
    call outn
    jmp nl

; ---- field access -----------------------------------------------------------
fieldptr:                       ; EBX = field index -> [fptr] = file offset
    mov eax, [rbase]
    add eax, [offtab + ebx * 4]
    mov [fptr], eax
    ret

fieldout:                       ; EBX = field index -> raw field bytes out
    call fieldptr
    mov ecx, [lentab + ebx * 4]
.l: mov eax, [fptr]
    call rdbyte
    call out
    inc dword [fptr]
    dec ecx
    jnz .l
    ret

fieldval:                       ; EBX = numeric field -> EAX value
    call fieldptr
    mov ecx, [lentab + ebx * 4]
    xor edx, edx
.l: push edx
    mov eax, [fptr]
    call rdbyte
    pop edx
    cmp al, '0'
    jb .s
    cmp al, '9'
    ja .s
    imul edx, edx, 10
    sub al, '0'
    movzx eax, al
    add edx, eax
.s: inc dword [fptr]
    dec ecx
    jnz .l
    mov eax, edx
    ret

zfill6:                         ; RPT_NO (6, right-justified) with '0' fill
    mov ecx, 6
.l: mov eax, [fptr]
    call rdbyte
    cmp al, ' '
    jne .o
    mov al, '0'
.o: call out
    inc dword [fptr]
    dec ecx
    jnz .l
    ret

fbytes2:                        ; two bytes of FILED at offset ECX
    mov eax, [fptr]
    add eax, ecx
    push eax
    call rdbyte
    call out
    pop eax
    inc eax
    call rdbyte
    jmp out

; ---- FCB random-record reader over the table (128-byte records) ------------
rdbyte:                         ; EAX = file offset -> AL = byte
    push ebx
    push ecx
    push edx
    mov ebx, eax
    shr eax, 7
    cmp eax, [cached]
    je .hit
    mov [cached], eax
    mov [tblfcb + RANDOM], eax
    mov edx, dbfrec
    call setdta
    FCB 0x21, tblfcb            ; random read (p. 6-85); 03 = partial, zero-filled
.hit:
    and ebx, 127
    mov al, [dbfrec + ebx]
    pop edx
    pop ecx
    pop ebx
    ret
rd16:
    push eax
    inc eax
    call rdbyte
    mov ah, al
    pop edx
    push eax
    mov eax, edx
    call rdbyte
    pop edx
    mov ah, dh
    movzx eax, ax
    ret
rd32:
    push eax
    add eax, 2
    call rd16
    shl eax, 16
    pop edx
    push eax
    mov eax, edx
    call rd16
    pop edx
    or eax, edx
    ret

; ---- output: console (AH=02h) and the report file (AH=15h) ----------------
out:                            ; AL -> console and, if tofile, the file
    cmp byte [conoff], 0
    jne .f
    call putc
.f: cmp byte [tofile], 0
    je .x
    cmp byte [fileoff], 0
    jne .x
    push ebx
    mov ebx, [opos]
    mov [outrec + ebx], al
    inc ebx
    mov [opos], ebx
    cmp ebx, 128
    jne .k
    call wrrec
.k: pop ebx
.x: ret
wrrec:                          ; write the 128-byte record buffer (p. 6-76)
    push eax
    push edx
    mov edx, outrec
    call setdta
    FCB 0x15, outfcb            ; AL 0 = written; 1 = disk full; 2 = DTA wrap
%ifndef TPS_MUTATE_NOWRCHK
    test al, al
    jnz writefail
%endif
    mov dword [opos], 0
    pop edx
    pop eax
    ret
writefail:                      ; TPS-009: the report did not reach the disk
    mov byte [tofile], 0        ; (no more writes; the console says so, not "WRITTEN")
    call crlf
    mov edx, m_e9
    call lineout
    FCB 0x10, outfcb            ; close, then drop the truncated report: the
    FCB 0x13, outfcb            ; previous one stays as TPSREPT.BAK
    mov al, 9
    jmp finish
flushout:                       ; pad the last record with Ctrl-Z (CP/M heritage)
    mov ebx, [opos]
    test ebx, ebx
    jz .x
.p: mov byte [outrec + ebx], 0x1A
    inc ebx
    cmp ebx, 128
    jne .p
    call wrrec
.x: ret

nl: mov al, 13
    call out
    mov al, 10
    jmp out
zout:                           ; ASCIIZ at ESI
    mov al, [esi]
    test al, al
    jz .x
    call out
    inc esi
    jmp zout
.x: ret
zline:
    call zout
    jmp nl
outn:                           ; ESI/ECX
    test ecx, ecx
    jz .x
.l: mov al, [esi]
    call out
    inc esi
    dec ecx
    jnz .l
.x: ret
out2:                           ; EAX 0..99 as two digits
    xor edx, edx
    mov ecx, 10
    div ecx
    push edx
    add al, '0'
    call out
    pop eax
    add al, '0'
    jmp out
outdec:                         ; EAX unsigned decimal
    mov ecx, 10
    xor ebx, ebx
.d: xor edx, edx
    div ecx
    push edx
    inc ebx
    test eax, eax
    jnz .d
.o: pop eax
    add al, '0'
    call out
    dec ebx
    jnz .o
    ret
nameout_o:                      ; 11-byte FCB name at EBX as NAME.EXT -> out
    mov byte [nsink], 0
    jmp nameout.go
nameout:                        ; ... -> the tables-on-file list
    mov byte [nsink], 1
.go:
    xor ecx, ecx
.n: mov al, [ebx + ecx]
    cmp al, ' '
    je .e
    call nput
    inc ecx
    cmp ecx, 8
    jne .n
.e: cmp byte [ebx + 8], ' '
    je .x
    mov al, '.'
    call nput
    mov ecx, 8
.x2:
    mov al, [ebx + ecx]
    cmp al, ' '
    je .x
    call nput
    inc ecx
    cmp ecx, 11
    jne .x2
.x: ret
nput:
    cmp byte [nsink], 0
    je out
tadd:                           ; append AL to the tables-on-file list
    push ebx
    mov ebx, [tlen]
    cmp ebx, 60
    jae .f
    mov [tlist + ebx], al
    inc dword [tlen]
.f: pop ebx
    ret

; ---- console-only helpers ---------------------------------------------------
puts:                           ; EDX -> '$' string
    mov ah, 0x09
    int 0x21
    ret
lineout:
    call puts
crlf:
    mov edx, m_crlf
    jmp puts
putc:                           ; AL -> console, registers preserved
    push eax
    push edx
    mov dl, al
    mov ah, 0x02
    int 0x21
    pop edx
    pop eax
    ret
putz:                           ; ASCIIZ at ESI -> console
    mov al, [esi]
    test al, al
    jz .x
    call putc
    inc esi
    jmp putz
.x: ret
putn:                           ; ESI/ECX -> console
    test ecx, ecx
    jz .x
.l: mov al, [esi]
    call putc
    inc esi
    dec ecx
    jnz .l
.x: ret
putdec:                         ; EAX -> console decimal
    mov ecx, 10
    xor ebx, ebx
.d: xor edx, edx
    div ecx
    push edx
    inc ebx
    test eax, eax
    jnz .d
.o: pop eax
    add al, '0'
    call putc
    dec ebx
    jnz .o
    ret
getkey:                         ; AH=08h: one key, no echo (stdin on real DOS)
    mov ah, 0x08
    int 0x21
    ret
getfield:                       ; digits (and '-') into EDI, max EBX, ENTER ends
    xor ecx, ecx
.k: call getkey
    cmp al, 0x0D
    je .e
    cmp al, '-'
    je .ok
    cmp al, '0'
    jb .k
    cmp al, '9'
    ja .k
.ok:
    cmp ecx, ebx
    jae .k
    mov [edi + ecx], al
    inc ecx
    call putc
    jmp .k
.e: push ecx
    call crlf
    pop ecx
    ret
setdta:                         ; EDX -> AH=1Ah
    push eax
    mov ah, 0x1A
    int 0x21
    pop eax
    ret

; ---- parameter + name helpers -------------------------------------------
getparam:                       ; ESI = "KEY=" (ASCIIZ) -> ESI/ECX value
    mov edi, pbuf               ; line starts only
.line:
    mov ebx, esi
    mov edx, edi
.c: mov al, [ebx]
    test al, al
    jz .hit
    cmp al, [edx]
    jne .skip
    inc ebx
    inc edx
    jmp .c
.hit:
    mov esi, edx
    xor ecx, ecx
.v: mov al, [esi + ecx]
    cmp al, 13
    je .vx
    cmp al, 10
    je .vx
    cmp al, 0x1A
    je .vx
    test al, al
    jz .vx
    inc ecx
    jmp .v
.vx:
    ret
.skip:                          ; to the start of the next line
    mov al, [edi]
    test al, al
    jz .miss
    cmp al, 0x1A
    je .miss
    inc edi
    cmp al, 10
    jne .skip
    jmp .line
.miss:
    push esi
    mov edx, m_r1c
    call puts
    pop esi
.mk: mov al, [esi]              ; the key without its '='
    cmp al, '='
    je .mx
    call putc
    inc esi
    jmp .mk
.mx:
    mov edx, m_r1d
    call lineout
    mov edx, m_op73
    call lineout
    mov al, 1
    jmp finish
atoi:                           ; ESI/ECX decimal -> EAX
    xor eax, eax
.l: test ecx, ecx
    jz .x
    movzx edx, byte [esi]
    sub edx, '0'
    cmp edx, 9
    ja .x
    imul eax, eax, 10
    add eax, edx
    inc esi
    dec ecx
    jmp .l
.x: ret
nameeq:                         ; ESI = ASCIIZ required name vs fname[11] -> ZF
    xor ecx, ecx
.l: mov al, [esi + ecx]
    test al, al
    jz .end
    cmp al, [fname + ecx]
    jne .x
    inc ecx
    jmp .l
.end:
    cmp byte [fname + ecx], 0   ; dBASE pads the name with NULs
.x: ret
zlen:                           ; ASCIIZ ESI -> ECX
    xor ecx, ecx
.l: cmp byte [esi + ecx], 0
    je .x
    inc ecx
    jmp .l
.x: ret

; ===========================================================================
; Wording: spec/tps_ritual.json (LOCKED). '$'-terminated for AH=09h.
m_crlf:  db 13, 10, '$'
m_banner:
    db 'INITECH CORPORATION', 13, 10
    db 'TPS REPORT GENERATOR  VERSION 2.11', 13, 10
    db '(C) 1989 INITECH CORPORATION. FOR INTERNAL USE ONLY.', 13, 10, 13, 10, '$'
m_r1a:   db 'TPS-001 PARAMETER FILE TPSPARM.DAT NOT FOUND.$'
m_op73:  db 'REFER TO OPERATING PROCEDURE OP-7.3 (PARAMETER FILES).$'
m_r1c:   db 'TPS-002 PARAMETER $'
m_r1d:   db ' MISSING FROM TPSPARM.DAT.$'
m_r2a:   db 'TOP OF FORM ALIGNMENT$'
m_r2b:   db '----- ALIGNMENT TEST LINE ----- FORM TPS-1 (REV. 10/89) -----$'
%ifdef TPS_MUTATE_SAY7
m_r2t:   db 'ADVANCE THE FORM 7 LINE-SPACES TO THE TEST LINE, THEN PRESS ENTER.$'
%else
m_r2t:   db 'ADVANCE THE FORM TO THE TEST LINE, THEN PRESS ENTER.$'
%endif
m_r2c:   db 'FORM NOT ALIGNED ($'
m_r2d:   db ' PRESSES). THE COUNT HAS BEEN RESET.$'
m_r2ok:  db 'FORM ALIGNED.$'
m_r3a:   db 'ENTER TPS REPORT NUMBER: $'
m_r3b:   db 'RE-ENTER TPS REPORT NUMBER: $'
m_r3c:   db 'TPS-003 REPORT NUMBERS DO NOT AGREE. PLEASE BEGIN AGAIN.$'
m_r4a:   db 'KEY THE NUMBER OF THE MEMO ABOUT THE NEW COVER SHEETS: $'
m_r4b:   db 'TPS-004 THAT IS NOT THE MEMO NUMBER. SEE YOUR SUPERVISOR.$'
m_r5a:   db 'DID YOU REMEMBER TO ATTACH THE NEW COVER SHEET, FORM TPS-1 (REV. 10/89)? (Y/N) $'
m_r5b:   db 'TPS-005 REPORT NOT PRINTED. A COPY OF MEMO $'
m_r5c:   db ' WILL BE$'
m_r5d:   db 'FORWARDED TO YOUR SUPERVISOR.$'
m_e6a:   db 'TPS-006 TABLE IS NOT A TPS TABLE (FIELD $'
m_e6b:   db ' NOT FOUND).$'
m_e7a:   db 'TPS-007 TABLE NOT FOUND.$'
m_e7b:   db 'USAGE: TPSREP table.DBF$'
m_e8:    db 'TPS-008 TPSREPT.TXT COULD NOT BE CREATED.$'
m_e9:    db 'TPS-009 TPSREPT.TXT COULD NOT BE WRITTEN. THE DISK MAY BE FULL.$'
m_r10:   db 'TPS-010 PARAMETER LINES IN TPSPARM.DAT IS NOT A NUMBER.$'
m_e11a:  db 'TPS-011 PAGES IS NOT A NUMBER (RECORD $'
m_e11b:  db ').$'
m_e12:   db 'TPS-012 PREVIOUS REPORT COULD NOT BE RETAINED AS TPSREPT.BAK.$'
m_done1: db 'TPS REPORT WRITTEN TO TPSREPT.TXT.$'
m_done2: db 'PREVIOUS REPORT RETAINED AS TPSREPT.BAK.$'
m_end1:  db 'TPS REPORT GENERATOR TERMINATED. RETURN CODE $'
m_end2:  db '.$'

s_tps:   db 'T P S   R E P O R T', 0
s_form:  db 'COVER SHEET                                FORM TPS-1 (REV. 10/89)', 0
s_c1:    db 'REPORT NUMBER ........ ', 0
s_c2:    db 'DIVISION ............. ', 0
s_c3:    db 'SOURCE TABLE ......... ', 0
s_c4:    db 'TABLE LAST UPDATED ... ', 0
s_c5:    db 'TABLES ON FILE ....... ', 0
s_c6:    db 'COVER SHEET MEMO ..... ', 0
s_c7:    db 'RECORDS IN TABLE ..... ', 0
s_c8:    db 'THIS COVER SHEET MUST ACCOMPANY ALL TPS REPORTS.', 0
s_c9:    db 'DO NOT FORWARD A TPS REPORT WITHOUT THE NEW COVER SHEET.', 0
s_h1:    db 'TPS REPORT NO. ', 0
s_h2:    db '                                         PAGE ', 0
s_h3:    db 'RPT NO  EMPLOYEE              DEPARTMENT    PAGES  FILED     COVER', 0
s_h4:    db '------  --------------------  ------------  -----  --------  -----', 0
s_2sp:   db '  ', 0
s_4sp:   db '    ', 0
s_yes:   db 'YES', 0
s_no:    db 'NO', 0
s_t1:    db 'REPORTS LISTED ....... ', 0
s_t2:    db 'PAGES FILED .......... ', 0
s_t3:    db 'WITHOUT COVER SHEET .. ', 0
s_t4:    db 'CORRECTIVE ACTION REQUIRED. SEE MEMO ', 0
s_t5:    db '*** END OF TPS REPORT ***', 0
s_sig:   db 'SIGNATURE', 0

k_title: db 'TITLE=', 0
k_div:   db 'DIVISION=', 0
k_memo:  db 'MEMO=', 0
k_lines: db 'LINES=', 0
f_rpt:   db 'RPT_NO', 0
f_emp:   db 'EMPLOYEE', 0
f_dept:  db 'DEPT', 0
f_pages: db 'PAGES', 0
f_cover: db 'COVER', 0
f_filed: db 'FILED', 0
reqtab:  dd f_rpt, f_emp, f_dept, f_pages, f_cover, f_filed

; FCBs (standard: drive 0 + name + 25 zero bytes + slack)
%macro SFCB 1
    db 0, %1
    times 40 db 0
%endmacro
parmfcb: SFCB 'TPSPARM DAT'
srchfcb: SFCB '????????DBF'
bakfcb:  SFCB 'TPSREPT BAK'
renfcb:  db 0, 'TPSREPT TXT', 0, 0, 0, 0, 0, 'TPSREPT BAK'
         times 30 db 0
; Extended search includes hidden/system files (IBM DOS 3.30 TR 6-70/71).
; The create still uses the standard FCB; a failed rename must refuse first.
outsearch: db 0xFF, 0, 0, 0, 0, 0, 0x06
outfcb:  SFCB 'TPSREPT TXT'
tblfcb:  times 48 db 0

conoff:  db 0
fileoff: db 0
tofile:  db 0
nsink:   db 0
renamed: db 0
upd_yy:  db 0
upd_mm:  db 0
upd_dd:  db 0
align 4
offtab:  times 6 dd 0
lentab:  times 6 dd 0
p_title: dd 0
l_title: dd 0
p_div:   dd 0
l_div:   dd 0
p_memo:  dd 0
l_memo:  dd 0
lines:   dd 0
nrecs:   dd 0
hdrlen:  dd 0
reclen:  dd 0
fpos:    dd 0
foff:    dd 0
flen:    dd 0
fptr:    dd 0
rbase:   dd 0
rec:     dd 0
page:    dd 0
onpage:  dd 0
listed:  dd 0
pagesum: dd 0
nocover: dd 0
cached:  dd 0
opos:    dd 0
tlen:    dd 0
rno1len: dd 0
fname:   times 12 db 0
rno1:    times 8 db 0
rno2:    times 8 db 0
memo:    times 8 db 0
tlist:   times 64 db 0
dta:     times 128 db 0
fdta:    times 64 db 0
dbfrec:  times 128 db 0
outrec:  times 128 db 0
pbuf:    times PBUF_MAX + 4 db 0
