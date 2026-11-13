; fcbx_program.asm -- FCBX.COM, the FCB legacy-API exerciser (beads initech-509.9).
;
; ONE source, TWO builds, the same transcript:
;   nasm -f bin -DREALDOS  -> a 16-bit real-mode .COM run under REAL MS-DOS 6.22
;                              (booted in dosbox-x) to MINT the golden transcript;
;   nasm -f bin            -> the flat-32 .COM-equivalent InitechOS loads at
;                              spec/memory_map.h PROGRAM_IMAGE (0x40100).
; The code is written with 32-bit registers throughout; under `bits 16` NASM
; emits operand/address-size prefixes (a 386 executes them in real mode) and
; every address is < 64 KiB, so both builds walk the same logic. INT 21h takes
; DS:DX in real mode and EDX flat (ADR-0003 DEC-04a.3) -- `mov edx, fcb` serves
; both. Only AH=1Bh/1Ch differ: real DOS returns DS:BX, the flat ABI EBX.
;
; Prints FCBX-BEGIN, one line per step, FCBX-END. Leaves on disk, for the mtools
; FAT differential: FCBHID.DAT (hidden), FCBRO.DAT (read-only), FCBKEPT.DAT
; (sequential + random records). Deletes FCBTEST.DAT and FCBTRE.DAT.
;
; Ref: IBM DOS 3.30 Technical Reference 80X0945 pp. 6-67..6-88 (PDF 190-211;
; the 0Fh..24h contracts), pp. 7-12..7-16 (PDF 375-379; FCB layout). Rule 12
; ASCII; Rule 11 deterministic (nasm -f bin).

%ifdef REALDOS
bits 16
org 0x0100
%else
bits 32
org 0x00040100                 ; == spec/memory_map.h PROGRAM_IMAGE
%endif

%macro SAY 1+
    jmp %%over
%%s: db %1, '$'
%%over:
    push eax
    mov edx, %%s
    mov ah, 0x09
    int 0x21
    pop eax
%endmacro

%macro DOS 2                    ; DOS fn, fcb  -> AL
    mov edx, %2
    mov ah, %1
    int 0x21
%endmacro

; field offsets from the drive byte (Tech Ref p. 7-13/7-14)
CURBLK equ 0x0C
RECSZ  equ 0x0E
FSIZE  equ 0x10
CURREC equ 0x20
RANDOM equ 0x21

start:
    mov edx, dta
    mov ah, 0x1A                ; SET DTA (p. 6-82)
    int 0x21
    SAY 'FCBX-BEGIN', 13, 10

    ; T01 OPEN of a missing file -> FFh (p. 6-67)
    SAY 'T01 OPEN-MISSING'
    DOS 0x0F, fmiss
    call pal
    call crlf

    ; T02 CREATE -> 00h, drive set, block 0, record size 80h, size 0 (p. 6-77, 6-67)
    SAY 'T02 CREATE'
    DOS 0x16, ftest
    call pal
    mov ah, 0x19
    int 0x21
    inc al
    SAY ' DRV='
    cmp al, [ftest]
    je .t02ok
    SAY 'BAD'
    jmp .t02n
.t02ok:
    SAY 'DEF'
.t02n:
    mov ebx, ftest
    call pblk
    call prsz
    call psize
    call crlf

    ; T03 three SEQUENTIAL WRITEs of 16-byte records 'A','B','C' (p. 6-76)
    SAY 'T03 SEQWRITE'
    mov word [ftest+RECSZ], 16
    mov byte [ftest+CURREC], 0
    mov al, 'A'
.t03:
    push eax
    call filldta
    DOS 0x15, ftest
    call pal
    pop eax
    inc al
    cmp al, 'D'
    jne .t03
    mov ebx, ftest
    call pblk
    call prec
    call psize
    call crlf

    ; T04 RANDOM WRITE record 10 ('K'): position follows the random field (p. 6-86)
    SAY 'T04 RANDWRITE'
    mov dword [ftest+RANDOM], 10
    mov al, 'K'
    call filldta
    DOS 0x22, ftest
    call pal
    mov ebx, ftest
    call pblk
    call prec
    call prnd
    call psize
    call crlf

    ; T04B fill the hole (records 3..9, 'D'..'J') sequentially from block 0
    ; record 3. Real DOS leaves an extension hole holding whatever the newly
    ; allocated clusters held (observed under MS-DOS 6.22: stale sector bytes);
    ; InitechOS zero-fills it. No contract covers hole bytes, so the transcript
    ; never prints one: the hole is overwritten before anything is read.
    SAY 'T04B FILLHOLE'
    mov word [ftest+CURBLK], 0
    mov byte [ftest+CURREC], 3
    mov al, 'D'
.t04b:
    push eax
    call filldta
    DOS 0x15, ftest
    call pal
    pop eax
    inc al
    cmp al, 'K'
    jne .t04b
    mov ebx, ftest
    call pblk
    call prec
    call psize
    call crlf

    ; T05 CLOSE (p. 6-69)
    SAY 'T05 CLOSE'
    DOS 0x10, ftest
    call pal
    call crlf

    ; T06 re-OPEN: record size back to 80h, size from the directory (p. 6-67)
    SAY 'T06 OPEN'
    DOS 0x0F, ftest
    call pal
    mov ebx, ftest
    call pblk
    call prsz
    call psize
    call crlf

    ; T07 FILE SIZE in records, rounded up, record size 16 then 128 (p. 6-87)
    SAY 'T07 FILESIZE'
    mov word [fsz+RECSZ], 16
    DOS 0x23, fsz
    call pal
    mov ebx, fsz
    call prnd
    mov word [fsz+RECSZ], 128
    DOS 0x23, fsz
    call pal
    call prnd
    call crlf

    ; T08 SEQUENTIAL READ all eleven 16-byte records, then EOF (p. 6-75)
    SAY 'T08 SEQREAD'
    mov word [ftest+RECSZ], 16
    mov word [ftest+CURBLK], 0
    mov byte [ftest+CURREC], 0
    mov ecx, 12
.t08:
    push ecx
    mov al, 0xEE
    call filldta
    DOS 0x14, ftest
    SAY ' '
    call hex2
    SAY ':'
    mov al, [dta]
    call hex2
    pop ecx
    dec ecx
    jnz .t08
    mov ebx, ftest
    call pblk
    call prec
    call crlf

    ; T09 RANDOM READ of a partial record: 76 of 100 bytes, rest zero-filled, AL=03
    SAY 'T09 PARTIAL'
    mov word [ftest+RECSZ], 100
    mov dword [ftest+RANDOM], 1
    mov al, 0xEE
    call filldta
    DOS 0x21, ftest
    call pal
    SAY ' SUM='
    xor eax, eax
    xor ecx, ecx
.t09:
    movzx edx, byte [dta+ecx]
    add eax, edx
    inc ecx
    cmp ecx, 100
    jne .t09
    call hex4
    SAY ' B75='
    mov al, [dta+75]
    call hex2
    SAY ' B76='
    mov al, [dta+76]
    call hex2
    mov ebx, ftest
    call pblk
    call prec
    call crlf

    ; T10 block crossing: record 127 of block 0 -> next is block 1 record 0
    SAY 'T10 BLOCKCROSS'
    mov word [ftest+RECSZ], 1
    mov word [ftest+CURBLK], 0
    mov byte [ftest+CURREC], 127
    DOS 0x14, ftest
    call pal
    SAY ':'
    mov al, [dta]
    call hex2
    mov ebx, ftest
    call pblk
    call prec
    DOS 0x14, ftest
    call pal
    SAY ':'
    mov al, [dta]
    call hex2
    call crlf

    ; T11 SET RELATIVE RECORD: 3 bytes used at record size >= 64, 4 below (p. 7-14)
    SAY 'T11 SETRND'
    mov word [ftest+RECSZ], 128
    mov word [ftest+CURBLK], 2
    mov byte [ftest+CURREC], 3
    mov dword [ftest+RANDOM], 0xFFFFFFFF
    DOS 0x24, ftest
    mov ebx, ftest
    call prnd
    mov word [ftest+RECSZ], 16
    mov dword [ftest+RANDOM], 0xFFFFFFFF
    DOS 0x24, ftest
    call prnd
    DOS 0x10, ftest
    call pal
    call crlf

    ; T12 a second file for the wildcard search
    SAY 'T12 CREATE2'
    DOS 0x16, ftwo
    call pal
    mov al, 'T'
    call filldta
    DOS 0x15, ftwo
    call pal
    DOS 0x10, ftwo
    call pal
    call crlf

    ; T13 SEARCH FIRST / NEXT with '?' (p. 6-70..6-73)
    SAY 'T13 FIND'
    DOS 0x11, fsrch
    call pfound
    DOS 0x12, fsrch
    call pfound
    DOS 0x12, fsrch
    call pal
    call crlf

    ; T14 extended FCB: create a HIDDEN file; a standard FCB cannot see it
    SAY 'T14 HIDDEN'
    DOS 0x16, xhid
    call pal
    DOS 0x10, xhid
    call pal
    DOS 0x11, shid
    call pal
    DOS 0x0F, shid
    call pal
    DOS 0x11, xhids
    call pal
    SAY ' X='
    mov al, [dta]
    call hex2
    mov al, [dta+6]
    call hex2
    mov al, [dta+7]
    call hex2
    SAY ' '
    mov ebx, dta+8
    call pname
    SAY ' A='
    mov al, [dta+8+0x0B]
    call hex2
    call crlf

    ; T15 RENAME with '?' in the new name; onto an existing name; missing
    SAY 'T15 RENAME'
    DOS 0x17, frn1
    call pal
    DOS 0x17, frn2
    call pal
    DOS 0x17, frn3
    call pal
    call crlf

    ; T16 read-only: created via extended FCB; create + delete refused. RENAME of
    ; a read-only file SUCCEEDS on real DOS (golden, MS-DOS 6.22): the Tech Ref
    ; p. 6-80 "specified in read-only mode" is the open ACCESS mode, not the
    ; attribute -- so FCBRO.DAT becomes FCBRW.DAT and the search then fails.
    SAY 'T16 READONLY'
    DOS 0x16, xro
    call pal
    DOS 0x10, xro
    call pal
    DOS 0x16, sro
    call pal
    DOS 0x13, sro2
    call pal
    DOS 0x17, frn4
    call pal
    DOS 0x11, xros
    call pal
    SAY ' A='
    mov al, [dta+8+0x0B]
    call hex2
    call crlf

    ; T17 DELETE every match of FCBT????.DAT (p. 6-74)
    SAY 'T17 DELETE'
    DOS 0x13, fdel
    call pal
    DOS 0x11, fdel
    call pal
    DOS 0x13, fdel
    call pal
    call crlf

    ; T18 the kept file: 10-byte records, 3 sequential + random record 3, renamed
    SAY 'T18 KEEP'
    DOS 0x16, fkeep
    call pal
    mov word [fkeep+RECSZ], 10
    mov byte [fkeep+CURREC], 0
    mov al, '1'
.t18:
    push eax
    call keeprec
    DOS 0x15, fkeep
    call pal
    pop eax
    inc al
    cmp al, '4'
    jne .t18
    mov dword [fkeep+RANDOM], 3
    mov al, 'R'
    call keeprec
    DOS 0x22, fkeep
    call pal
    mov ebx, fkeep
    call psize
    DOS 0x10, fkeep
    call pal
    DOS 0x17, frn5
    call pal
    call crlf

    ; T19 ALLOCATION TABLE INFORMATION (p. 6-83/6-84)
    SAY 'T19 ALLOC'
    mov ah, 0x1B
    call alloc
    mov dl, 0
    mov ah, 0x1C
    call alloc
    mov dl, 1
    mov ah, 0x1C
    call alloc
    mov dl, 9
    mov ah, 0x1C
    int 0x21
    call pal
    call crlf

    ; T20 the undocumented reserved slots 18h/1Dh/1Eh/20h (golden: real DOS)
    SAY 'T20 RESERVED'
    mov ax, 0x1855
    int 0x21
    call pal
    mov ax, 0x1D55
    int 0x21
    call pal
    mov ax, 0x1E55
    int 0x21
    call pal
    mov ax, 0x2055
    int 0x21
    call pal
    call crlf

    SAY 'FCBX-END', 13, 10
    mov ax, 0x4C00
    int 0x21

; ---- helpers --------------------------------------------------------------
alloc:                          ; AH = 1Bh/1Ch (DL set) -> print AL CX DX [media]
%ifdef REALDOS
    push ds
    int 0x21
    mov bl, [bx]                ; DS:BX -> media byte (real mode)
    pop ds
%else
    int 0x21
    mov bl, [ebx]               ; EBX -> media byte (flat, DEC-04a.3)
%endif
    mov [acx], cx
    mov [adx], dx
    mov [amed], bl
    call pal
    SAY ' '
    mov ax, [acx]
    call hex4
    SAY ' '
    mov ax, [adx]
    call hex4
    SAY ' '
    mov al, [amed]
    jmp hex2

pal:                            ; print " AL=hh", preserve EAX
    SAY ' '
    jmp hex2

pfound:                         ; AL + the 11-char name the DTA unopened FCB got
    call pal
    SAY ' D='
    mov al, [dta]
    call hex2
    SAY ' '
    mov ebx, dta+1
    call pname
    SAY ' A='
    mov al, [dta+1+0x0B]
    call hex2
    SAY ' S='
    mov eax, [dta+1+0x1C]
    jmp hex8

pname:                          ; 11 raw chars at EBX
    push eax
    push ecx
    xor ecx, ecx
.l: mov al, [ebx+ecx]
    call putc
    inc ecx
    cmp ecx, 11
    jne .l
    pop ecx
    pop eax
    ret

pblk: SAY ' BLK='
    mov ax, [ebx+CURBLK]
    jmp hex4
prec: SAY ' REC='
    mov al, [ebx+CURREC]
    jmp hex2
prsz: SAY ' RSZ='
    mov ax, [ebx+RECSZ]
    jmp hex4
psize: SAY ' SIZE='
    mov eax, [ebx+FSIZE]
    jmp hex8
prnd: SAY ' RND='               ; all four bytes, high first
    mov eax, [ebx+RANDOM]
    jmp hex8

filldta:                        ; fill 128 DTA bytes with AL
    push ecx
    xor ecx, ecx
.f: mov [dta+ecx], al
    inc ecx
    cmp ecx, 128
    jne .f
    pop ecx
    ret

keeprec:                        ; 10-byte record "KEEP-n...." + CR LF
    push ecx
    xor ecx, ecx
.k: mov dl, [keeptpl+ecx]
    mov [dta+ecx], dl
    inc ecx
    cmp ecx, 10
    jne .k
    mov [dta+5], al
    pop ecx
    ret

crlf:
    SAY 13, 10
    ret

hex8:
    push eax
    shr eax, 16
    call hex4
    pop eax
hex4:
    push eax
    mov al, ah
    call hex2
    pop eax
hex2:
    push eax
    shr al, 4
    call nib
    pop eax
    push eax
    call nib
    pop eax
    ret
nib:
    and al, 0x0F
    add al, '0'
    cmp al, '9'
    jbe putc
    add al, 7
putc:                           ; AL -> CON, preserve EAX/EDX
    push eax
    push edx
    mov dl, al
    mov ah, 0x02
    int 0x21
    pop edx
    pop eax
    ret

; ---- data -----------------------------------------------------------------
keeptpl: db 'KEEP-?', 'abc', 10
; standard FCBs: drive 0 (default) + name + 25 zero bytes (+ slack)
%macro SFCB 1
    db 0, %1
    times 40 db 0
%endmacro
%macro XFCB 2                   ; extended: FFh, 5 x 0, attribute, standard FCB
    db 0xFF, 0, 0, 0, 0, 0, %1
    SFCB %2
%endmacro
fmiss:  SFCB 'NOSUCH  DAT'
ftest:  SFCB 'FCBTEST DAT'
fsz:    SFCB 'FCBTEST DAT'
ftwo:   SFCB 'FCBTWO  DAT'
fsrch:  SFCB 'FCB?????DAT'
xhid:   XFCB 0x02, 'FCBHID  DAT'
shid:   SFCB 'FCBHID  DAT'
xhids:  XFCB 0x02, 'FCBHID  DAT'
frn1:   db 0, 'FCBTWO  DAT', 0, 0, 0, 0, 0, 'FCBTRE  ???'
        times 30 db 0
frn2:   db 0, 'FCBTRE  DAT', 0, 0, 0, 0, 0, 'FCBTEST DAT'
        times 30 db 0
frn3:   db 0, 'NOSUCH  DAT', 0, 0, 0, 0, 0, 'NOSUCH2 DAT'
        times 30 db 0
xro:    XFCB 0x01, 'FCBRO   DAT'
sro:    SFCB 'FCBRO   DAT'
sro2:   SFCB 'FCBRO   DAT'
frn4:   db 0, 'FCBRO   DAT', 0, 0, 0, 0, 0, 'FCBRW   DAT'
        times 30 db 0
xros:   XFCB 0x01, 'FCBRO   DAT'
fdel:   SFCB 'FCBT????DAT'
fkeep:  SFCB 'FCBKEEP DAT'
frn5:   db 0, 'FCBKEEP DAT', 0, 0, 0, 0, 0, 'FCBKEPT DAT'
        times 30 db 0
acx:    dw 0
adx:    dw 0
amed:   db 0
dta:    times 160 db 0
