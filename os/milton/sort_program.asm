; sort_program.asm -- the FAT-sourced flat (.COM-equivalent) SORT filter.
;
; beads: initech-m0dc (the classic DOS filters MORE/SORT/FIND). SORT is the
;        line-sorting filter. Like GREET/GOBBLE it is NOT baked into the kernel:
;        it is mcopy'd onto a FAT12 data disk as SORT.COM and loaded BY NAME via
;        load_program_from_fat / INT 21h AH=4Bh EXEC. It reads ALL of stdin
;        (handle 0, AH=3Fh to EOF) into a fixed buffer, splits it into CRLF /
;        LF-terminated lines (a final unterminated line counts), sorts them
;        ASCENDING with a CASE-INSENSITIVE ASCII collation (lower a-z folded to
;        upper for the compare -- authentic DOS 3.3 SORT), and writes each sorted
;        line CRLF-terminated to stdout (handle 1, AH=40h). Switch /R (or /r)
;        reverses the order. Under `SORT < IN.TXT` the shell re-points handle 0 at
;        the file (bsy.7); under `TYPE IN.TXT | SORT` the pipe (bsy.8) feeds it.
;
; Semantics ground truth (MS-DOS 3.3 User's Guide / SORT.EXE documented behavior):
;   - SORT reads standard input, sorts lines, writes standard output.
;   - The collating sequence for the default (no /+n) sort folds case: letters
;     compare case-insensitively (a == A). Non-letters compare by ASCII value.
;   - /R sorts in REVERSE (descending) order.
;   - /+n starts the sort at column n (1-based). NOT IMPLEMENTED here (reported):
;     a program that reads /+n but ignores it would silently mis-sort, so we do
;     NOT accept it -- only plain ascending + /R are honored; any other switch is
;     ignored (the fixtures never pass one).
;   - Buffer bound: real DOS SORT buffers a bounded amount (~63 KB in 3.3). We use
;     a fixed 64 KiB buffer and FAIL LOUD ("SORT: Insufficient memory", the DOS
;     3.3 diagnostic) if stdin exceeds it -- Rule 2, never silently truncate.
;
; Ref:   os/milton/int21.c do_read (AH=3Fh: EBX=handle, ECX=count, EDX=buf ->
;        EAX=bytes, 0=EOF), do_write (AH=40h), do_puts (AH=09h), do_terminate
;        (AH=4Ch); spec/memory_map.h (PROGRAM_IMAGE=0x00040100 == org; PSP at
;        0x00040000, cmd tail at 0x00040080); MS-DOS 3.3 User's Guide "SORT".
;        CLAUDE.md Law 1 (cite), Rule 2 (fail loud on overflow), Rule 11
;        (deterministic: nasm -f bin), Rule 12 (ASCII).
;
; Assembled: nasm -f bin os/milton/sort_program.asm -o build/sort_program.bin
; Deployed:  mcopy build/sort_program.bin ::SORT.COM  (NOT baked into kernel).
;
; Mutant (Rule 6): nasm -DSORT_MUTATE_REVERSE flips the default order to
; descending, so the ascending-order emu assertion goes RED (proves the gate and
; the comparator are load-bearing).

bits 32
org 0x00040100                 ; == spec/memory_map.h PROGRAM_IMAGE

; --- Fixed scratch addresses (mirrors datetime_program's absolute-page idiom).
; All lie inside the program's own 64 KiB arena (PROGRAM_BASE 0x40000 .. 0x80000)
; and above the program image; the program is tiny so nothing else uses them.
INBUF     equ 0x00050000       ; stdin buffer base
INBUF_MAX equ 0x00010000       ; 64 KiB cap (Rule 2: overflow -> fail loud)
LINEIDX   equ 0x00060000       ; array of 8-byte line records: [off:u32][clen:u32]
REC_SIZE  equ 8
PSP_TAIL_LEN equ 0x00040080    ; PSP:80h byte0 = command-tail length
PSP_TAIL_TXT equ 0x00040081    ; PSP:81h.. = command-tail text

start:
    ; ---- Parse the command tail (PSP:80h) for /R (reverse). ----
    xor ebp, ebp               ; EBP = reverse flag (0 = ascending)
%ifdef SORT_MUTATE_REVERSE
    mov ebp, 1                 ; MUTANT: default to descending -> ascending gate RED
%endif
    movzx ecx, byte [PSP_TAIL_LEN]
    mov esi, PSP_TAIL_TXT
.tail_scan:
    test ecx, ecx
    jz .tail_done
    mov al, [esi]
    cmp al, '/'
    jne .tail_next
    ; '/': look at the following char for R/r.
    cmp ecx, 1
    jbe .tail_next
    mov dl, [esi + 1]
    and dl, 0xDF               ; fold to upper (R)
    cmp dl, 'R'
    jne .tail_next
    xor ebp, 1                 ; toggle reverse (so /R under the mutant cancels)
.tail_next:
    inc esi
    dec ecx
    jmp .tail_scan
.tail_done:
    mov [rflag], ebp

    ; ---- Read ALL of stdin (handle 0) into INBUF until EOF. ----
    xor edi, edi               ; EDI = total bytes read so far
.read_loop:
    mov eax, INBUF_MAX
    sub eax, edi               ; remaining capacity
    jz .overflow               ; buffer full and stdin not at EOF -> fail loud
    mov ecx, eax               ; count = remaining
    mov ah, 0x3F
    xor ebx, ebx               ; handle 0 = stdin
    mov edx, INBUF
    add edx, edi               ; append point
    int 0x21
    jc .read_done              ; read error -> treat like EOF
    test eax, eax
    jz .read_done              ; 0 bytes = EOF
    add edi, eax
    jmp .read_loop
.read_done:
    mov [total], edi

    ; ---- Split INBUF[0..total) into line records at LINEIDX. ----
    ; A line runs from `start` up to and INCLUDING the next LF (0x0A); a trailing
    ; CR (0x0D) before that LF is stripped from the compared content. A final run
    ; with no LF is still a line. clen = content length (terminators excluded).
    xor esi, esi               ; ESI = scan cursor (offset into INBUF)
    xor ebx, ebx               ; EBX = line count
    mov edi, [total]
.split_loop:
    cmp esi, edi
    jae .split_done
    mov edx, esi               ; EDX = current line start offset
.find_lf:
    cmp esi, edi
    jae .line_end_noterm       ; hit EOF with no LF -> unterminated final line
    mov al, [INBUF + esi]
    inc esi
    cmp al, 0x0A               ; LF ?
    jne .find_lf
    ; Found LF at (esi-1). Content = [edx .. esi-1), minus a trailing CR.
    mov ecx, esi
    dec ecx                    ; ecx = index of the LF
    sub ecx, edx               ; ecx = content length INCLUDING a possible CR
    jz .store_rec              ; empty line (bare LF)
    ; strip one trailing CR if present
    mov eax, edx
    add eax, ecx
    dec eax                    ; index of last content byte
    cmp byte [INBUF + eax], 0x0D
    jne .store_rec
    dec ecx                    ; drop the CR from clen
    jmp .store_rec
.line_end_noterm:
    ; content = [edx .. edi); strip a trailing CR if present.
    mov ecx, edi
    sub ecx, edx               ; clen
    test ecx, ecx
    jz .split_done             ; empty trailing run (start==EOF) -> no line
    mov eax, edx
    add eax, ecx
    dec eax
    cmp byte [INBUF + eax], 0x0D
    jne .store_rec
    dec ecx
.store_rec:
    ; record[ebx] = { off=edx, clen=ecx }
    mov eax, ebx
    shl eax, 3                 ; * REC_SIZE (8)
    mov [LINEIDX + eax], edx
    mov [LINEIDX + eax + 4], ecx
    inc ebx
    jmp .split_loop
.split_done:
    mov [nlines], ebx

    ; ---- Insertion sort the record array by the case-insensitive comparator. ----
    ; for i in 1..nlines-1: key=rec[i]; j=i-1; while j>=0 && cmp(rec[j],key)>0:
    ;   rec[j+1]=rec[j]; j--; rec[j+1]=key.  Order sense flips with rflag.
    mov ecx, [nlines]
    cmp ecx, 2
    jb .sort_done              ; 0 or 1 line: already sorted
    mov edi, 1                 ; EDI = i
.sort_outer:
    cmp edi, [nlines]
    jae .sort_done
    ; key = rec[i]
    mov eax, edi
    shl eax, 3
    mov edx, [LINEIDX + eax]        ; key.off
    mov ebx, [LINEIDX + eax + 4]    ; key.clen
    mov [key_off], edx
    mov [key_clen], ebx
    mov esi, edi
    dec esi                    ; ESI = j
.sort_inner:
    cmp esi, 0
    jl .sort_place
    ; compare rec[j] vs key -> want rec[j] > key to shift
    mov eax, esi
    shl eax, 3
    ; push loop regs across the compare (compare_lines clobbers eax..edx)
    push esi
    push edi
    mov edx, [LINEIDX + eax]        ; a.off
    mov ecx, [LINEIDX + eax + 4]    ; a.clen
    mov ebx, [key_off]             ; b.off
    mov ebp, [key_clen]            ; b.clen  (compare uses EBP for b.clen)
    call compare_lines             ; -> EAX = <0 / 0 / >0 (already order-adjusted)
    pop edi
    pop esi
    test eax, eax
    jle .sort_place            ; rec[j] <= key: stop
    ; shift rec[j] -> rec[j+1]
    mov eax, esi
    shl eax, 3
    mov ecx, [LINEIDX + eax]
    mov edx, [LINEIDX + eax + 4]
    mov [LINEIDX + eax + 8], ecx
    mov [LINEIDX + eax + 12], edx
    dec esi
    jmp .sort_inner
.sort_place:
    ; rec[j+1] = key
    mov eax, esi
    inc eax
    shl eax, 3
    mov ecx, [key_off]
    mov edx, [key_clen]
    mov [LINEIDX + eax], ecx
    mov [LINEIDX + eax + 4], edx
    inc edi
    jmp .sort_outer
.sort_done:

    ; ---- Write the sorted lines, each CRLF-terminated, to stdout (handle 1). ----
    xor edi, edi               ; EDI = i
.out_loop:
    cmp edi, [nlines]
    jae .out_done
    mov eax, edi
    shl eax, 3
    mov edx, [LINEIDX + eax]        ; off
    mov ecx, [LINEIDX + eax + 4]    ; clen
    ; write content (may be zero-length)
    test ecx, ecx
    jz .out_crlf
    push edi
    mov ebx, 1
    add edx, INBUF                  ; flat ptr to content
    mov ah, 0x40
    int 0x21
    pop edi
.out_crlf:
    push edi
    mov ah, 0x40
    mov ebx, 1
    mov ecx, 2
    mov edx, crlf
    int 0x21
    pop edi
    inc edi
    jmp .out_loop
.out_done:

    ; ---- Terminate cleanly. ----
    mov ah, 0x4C
    mov al, 0
    int 0x21
    int 0x20                    ; defense in depth (Rule 2)

.overflow:
    ; stdin exceeded the buffer: fail loud with the DOS 3.3 diagnostic (Rule 2).
    mov ah, 0x09
    mov edx, msg_oom
    int 0x21
    mov ah, 0x4C
    mov al, 1
    int 0x21
    int 0x20

; ---------------------------------------------------------------------------
; compare_lines: case-insensitive ASCII comparison of line A vs line B, then
; adjusted for the reverse flag.  Inputs: EDX=a.off, ECX=a.clen, EBX=b.off,
; EBP=b.clen.  Output: EAX <0 if A<B, 0 if equal, >0 if A>B (AFTER applying
; rflag: reverse negates the result).  Clobbers EAX/ECX/EDX/ESI/EDI (not EBX/EBP).
; ---------------------------------------------------------------------------
compare_lines:
    ; ESI walks A content, EDI walks B content; count = min(a.clen,b.clen).
    lea esi, [INBUF + edx]
    lea edi, [INBUF + ebx]
    mov eax, ecx               ; a.clen
    cmp eax, ebp
    jbe .have_min
    mov eax, ebp               ; min = b.clen
.have_min:
    ; EAX = min count
    mov edx, eax               ; loop counter
.cmp_loop:
    test edx, edx
    jz .cmp_len
    mov al, [esi]
    mov ah, [edi]
    ; fold both to upper (a-z -> A-Z)
    cmp al, 'a'
    jb .a_folded
    cmp al, 'z'
    ja .a_folded
    sub al, 0x20
.a_folded:
    cmp ah, 'a'
    jb .b_folded
    cmp ah, 'z'
    ja .b_folded
    sub ah, 0x20
.b_folded:
    cmp al, ah
    jb .less
    ja .greater
    inc esi
    inc edi
    dec edx
    jmp .cmp_loop
.cmp_len:
    ; common prefix equal: shorter line is smaller
    mov eax, ecx               ; a.clen
    cmp eax, ebp               ; b.clen
    jb .less
    ja .greater
    xor eax, eax               ; equal
    jmp .apply_rflag
.less:
    mov eax, -1
    jmp .apply_rflag
.greater:
    mov eax, 1
.apply_rflag:
    cmp dword [rflag], 0
    je .cmp_ret
    neg eax                    ; reverse: descending
.cmp_ret:
    ret

; --- Data -------------------------------------------------------------------
crlf:      db 0x0D, 0x0A
msg_oom:   db "SORT: Insufficient memory", 0x0D, 0x0A, "$"

; --- BSS-ish scratch (small; baked as zeros in the .COM image) --------------
rflag:     dd 0
total:     dd 0
nlines:    dd 0
key_off:   dd 0
key_clen:  dd 0
