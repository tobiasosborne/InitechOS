; find_program.asm -- the FAT-sourced flat (.COM-equivalent) FIND filter.
;
; beads: initech-m0dc (the classic DOS filters MORE/SORT/FIND). FIND prints the
;        lines of its input that CONTAIN a literal quoted string. Like GREET/
;        GOBBLE it is NOT baked into the kernel: mcopy'd onto a FAT12 data disk as
;        FIND.COM and loaded BY NAME via load_program_from_fat / AH=4Bh EXEC.
;
;        With NO file operand (the only form the harness exercises) FIND filters
;        STDIN (handle 0): it reads all of stdin to EOF, splits it into lines, and
;        for each line writes the line to stdout (handle 1) iff it contains the
;        search string.  Under `TYPE IN.TXT | FIND "needle"` the pipe (bsy.8)
;        feeds stdin; under `FIND "needle" < IN.TXT` the `<` redirect (bsy.7) does.
;
; Semantics ground truth (MS-DOS 3.3 User's Guide "FIND"):
;   - FIND "string" [drive:][path]filename ...   The quotes are REQUIRED; the
;     search string is the text between the first and the next double-quote.
;   - Default match is CASE-SENSITIVE and prints each line that CONTAINS string.
;   - /V  prints the lines that do NOT contain string.
;   - /C  prints only the COUNT of matching lines (suppresses the lines).
;   - /N  precedes each printed line with its line number in the form [n].
;   - /I  ignores case when matching (case-insensitive).  [DOS 3.3 FIND has no
;     /I; it arrived in later DOS -- implemented here as a cheap, documented,
;     backward-compatible superset and reported as such.]
;   - A missing/empty quoted string is a parameter error ("FIND: Parameter format
;     not correct") -> fail loud (Rule 2).
;
; Ref:   os/milton/int21.c do_read (AH=3Fh), do_write (AH=40h), do_puts (AH=09h),
;        do_terminate (AH=4Ch); spec/memory_map.h (PROGRAM_IMAGE=0x00040100 == org;
;        PSP 0x00040000, cmd tail at PSP:80h == 0x00040080); MS-DOS 3.3 User's
;        Guide "FIND". CLAUDE.md Law 1 (cite), Rule 2 (fail loud), Rule 11
;        (nasm -f bin deterministic), Rule 12 (ASCII).
;
; Assembled: nasm -f bin os/milton/find_program.asm -o build/find_program.bin
; Deployed:  mcopy build/find_program.bin ::FIND.COM  (NOT baked into kernel).
;
; Mutant (Rule 6): nasm -DFIND_MUTATE_INVERT flips the match sense (prints the
; NON-matching lines), so the "only matching lines appear" emu assertion goes RED.

bits 32
org 0x00040100                 ; == spec/memory_map.h PROGRAM_IMAGE

INBUF        equ 0x00050000     ; stdin buffer base
INBUF_MAX    equ 0x00010000     ; 64 KiB cap
PSP_TAIL_LEN equ 0x00040080     ; PSP:80h byte0 = command-tail length
PSP_TAIL_TXT equ 0x00040081     ; PSP:81h.. = command-tail text

; switch bits in [flags]
F_V equ 1                       ; /V invert
F_C equ 2                       ; /C count only
F_N equ 4                       ; /N line numbers
F_I equ 8                       ; /I ignore case

start:
    ; ---- Parse the command tail (PSP:80h): switches + the quoted string. ----
    mov dword [flags], 0
    mov dword [nlen], 0
    mov byte  [have_quote], 0
    movzx ecx, byte [PSP_TAIL_LEN]
    mov esi, PSP_TAIL_TXT
.tail_scan:
    test ecx, ecx
    jz .tail_done
    mov al, [esi]
    cmp al, '/'
    je .sw
    cmp al, '"'
    je .quote
    jmp .tail_next
.sw:
    ; '/x' switch: look at the next char.
    cmp ecx, 1
    jbe .tail_next
    mov dl, [esi + 1]
    and dl, 0xDF               ; fold to upper
    cmp dl, 'V'
    jne .not_v
    or dword [flags], F_V
    jmp .sw_consumed
.not_v:
    cmp dl, 'C'
    jne .not_c
    or dword [flags], F_C
    jmp .sw_consumed
.not_c:
    cmp dl, 'N'
    jne .not_n
    or dword [flags], F_N
    jmp .sw_consumed
.not_n:
    cmp dl, 'I'
    jne .sw_consumed
    or dword [flags], F_I
.sw_consumed:
    inc esi                    ; also consume the switch letter
    dec ecx
    jmp .tail_next
.quote:
    ; first '"': copy chars up to the next '"' into `needle`.
    mov byte [have_quote], 1
    inc esi                    ; skip opening quote
    dec ecx
    xor edx, edx               ; edx = needle length
.q_copy:
    test ecx, ecx
    jz .q_done
    mov al, [esi]
    cmp al, '"'
    je .q_close
    cmp edx, 127
    jae .q_skip                ; clamp (Rule 2: never overflow the 128-byte buf)
    mov [needle + edx], al
    inc edx
.q_skip:
    inc esi
    dec ecx
    jmp .q_copy
.q_close:
    inc esi                    ; consume closing quote
    dec ecx
.q_done:
    mov [nlen], edx
    jmp .tail_scan             ; ecx already advanced; continue scanning switches
.tail_next:
    inc esi
    dec ecx
    jmp .tail_scan
.tail_done:
    ; A quoted string is REQUIRED (DOS: quotes mandatory). No quote -> param error.
    cmp byte [have_quote], 0
    je .param_err

    ; ---- Read ALL of stdin (handle 0) into INBUF until EOF. ----
    xor edi, edi
.read_loop:
    mov eax, INBUF_MAX
    sub eax, edi
    jz .read_done              ; buffer full: process what we have (best-effort)
    mov ecx, eax
    mov ah, 0x3F
    xor ebx, ebx
    mov edx, INBUF
    add edx, edi
    int 0x21
    jc .read_done
    test eax, eax
    jz .read_done
    add edi, eax
    jmp .read_loop
.read_done:
    mov [total], edi

    ; ---- Walk the input line by line; test + emit. ----
    mov dword [count], 0
    mov dword [lineno], 0
    xor esi, esi               ; scan cursor
.line_loop:
    cmp esi, [total]
    jae .lines_done
    mov edx, esi               ; line start offset
.find_lf:
    cmp esi, [total]
    jae .line_noterm
    mov al, [INBUF + esi]
    inc esi
    cmp al, 0x0A
    jne .find_lf
    ; content = [edx .. esi-1), strip trailing CR
    mov ecx, esi
    dec ecx
    sub ecx, edx               ; clen (incl possible CR)
    jz .have_line
    mov eax, edx
    add eax, ecx
    dec eax
    cmp byte [INBUF + eax], 0x0D
    jne .have_line
    dec ecx
    jmp .have_line
.line_noterm:
    mov ecx, [total]
    sub ecx, edx
    test ecx, ecx
    jz .lines_done             ; empty trailing run -> no line
    mov eax, edx
    add eax, ecx
    dec eax
    cmp byte [INBUF + eax], 0x0D
    jne .have_line
    dec ecx
.have_line:
    ; EDX = content offset, ECX = content length. Bump the line number.
    ; ESI is the NEXT-line scan cursor; the helper routines below (line_contains,
    ; emit_*) clobber ESI/EDI/EBP, so stash the cursor and reload it at .next_line
    ; (int 0x21 itself preserves ESI/EDI via pushad/popad -- only our helpers do not).
    mov [scan], esi
    inc dword [lineno]
    mov [ln_off], edx
    mov [ln_len], ecx
    ; does the line contain `needle`?
    call line_contains         ; -> EAX = 1 if contained, 0 if not
    ; apply /V invert
    test dword [flags], F_V
    jz .no_invert
    xor eax, 1
.no_invert:
%ifdef FIND_MUTATE_INVERT
    xor eax, 1                 ; MUTANT (Rule 6): flip match sense -> the "only
                               ; matching lines appear" assertion goes RED.
%endif
    test eax, eax
    jz .next_line              ; no match -> next line
    ; matched.
    inc dword [count]
    test dword [flags], F_C
    jnz .next_line             ; /C: count only, suppress the line
    ; /N: emit "[n]" prefix
    test dword [flags], F_N
    jz .emit_line
    call emit_lineno
.emit_line:
    ; write content then CRLF
    mov ecx, [ln_len]
    test ecx, ecx
    jz .emit_crlf
    mov edx, [ln_off]
    add edx, INBUF
    mov ebx, 1
    mov ah, 0x40
    int 0x21
.emit_crlf:
    mov ah, 0x40
    mov ebx, 1
    mov ecx, 2
    mov edx, crlf
    int 0x21
.next_line:
    mov esi, [scan]            ; restore the scan cursor clobbered by the helpers
    jmp .line_loop
.lines_done:
    ; /C: print the final count as a decimal line.
    test dword [flags], F_C
    jz .fin
    mov eax, [count]
    call emit_decimal
    mov ah, 0x40
    mov ebx, 1
    mov ecx, 2
    mov edx, crlf
    int 0x21
.fin:
    mov ah, 0x4C
    mov al, 0
    int 0x21
    int 0x20

.param_err:
    mov ah, 0x09
    mov edx, msg_param
    int 0x21
    mov ah, 0x4C
    mov al, 1
    int 0x21
    int 0x20

; ---------------------------------------------------------------------------
; line_contains: does line [ln_off,ln_len) contain the substring `needle`
; (length [nlen])?  Honors /I (case fold).  Output EAX = 1 (yes) / 0 (no).
; An empty needle (nlen==0) is contained in every line -> 1.
; Clobbers EAX/EBX/ECX/EDX/ESI/EDI.
; ---------------------------------------------------------------------------
line_contains:
    mov ecx, [nlen]
    test ecx, ecx
    jz .yes                    ; empty needle matches everything
    mov edx, [ln_len]
    cmp edx, ecx
    jb .no                     ; line shorter than needle -> cannot contain
    ; last start index = ln_len - nlen ; try each start s = 0..that
    sub edx, ecx               ; EDX = max start offset (inclusive)
    xor ebx, ebx               ; EBX = start offset s
.try:
    ; compare needle[0..nlen) against line[ln_off+s ..]
    mov esi, [ln_off]
    add esi, ebx
    add esi, INBUF             ; ptr into line
    mov edi, needle
    mov ecx, [nlen]
.cmp:
    mov al, [esi]
    mov ah, [edi]
    test dword [flags], F_I
    jz .nofold
    ; fold both to upper
    cmp al, 'a'
    jb .a_ok
    cmp al, 'z'
    ja .a_ok
    sub al, 0x20
.a_ok:
    cmp ah, 'a'
    jb .b_ok
    cmp ah, 'z'
    ja .b_ok
    sub ah, 0x20
.b_ok:
.nofold:
    cmp al, ah
    jne .next_start
    inc esi
    inc edi
    dec ecx
    jnz .cmp
    ; all nlen bytes matched
    jmp .yes
.next_start:
    inc ebx
    cmp ebx, edx
    jbe .try
.no:
    xor eax, eax
    ret
.yes:
    mov eax, 1
    ret

; ---------------------------------------------------------------------------
; emit_lineno: write "[<lineno>]" to stdout (handle 1).  Clobbers eax..edi.
; ---------------------------------------------------------------------------
emit_lineno:
    mov ah, 0x40
    mov ebx, 1
    mov ecx, 1
    mov edx, lbrack
    int 0x21
    mov eax, [lineno]
    call emit_decimal
    mov ah, 0x40
    mov ebx, 1
    mov ecx, 1
    mov edx, rbrack
    int 0x21
    ret

; ---------------------------------------------------------------------------
; emit_decimal: write the unsigned integer in EAX as ASCII decimal to handle 1.
; Builds the digits backward into `numbuf` (right-aligned), then writes them.
; Clobbers eax..edi.
; ---------------------------------------------------------------------------
emit_decimal:
    mov edi, numbuf_end        ; write pointer (exclusive), fill backward
    mov ecx, 10
    test eax, eax
    jnz .conv
    dec edi
    mov byte [edi], '0'
    jmp .write
.conv:
    test eax, eax
    jz .write
    xor edx, edx
    div ecx                    ; EAX/=10, EDX=digit
    add dl, '0'
    dec edi
    mov [edi], dl
    jmp .conv
.write:
    mov edx, edi
    mov ecx, numbuf_end
    sub ecx, edi               ; digit count
    mov ah, 0x40
    mov ebx, 1
    int 0x21
    ret

; --- Data -------------------------------------------------------------------
crlf:      db 0x0D, 0x0A
lbrack:    db "["
rbrack:    db "]"
msg_param: db "FIND: Parameter format not correct", 0x0D, 0x0A, "$"

; --- scratch (baked as zeros/space in the .COM image) -----------------------
flags:      dd 0
nlen:       dd 0
have_quote: db 0
total:      dd 0
count:      dd 0
lineno:     dd 0
ln_off:     dd 0
ln_len:     dd 0
scan:       dd 0
needle:     times 128 db 0
numbuf:     times 12 db 0
numbuf_end:
