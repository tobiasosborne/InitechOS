%include "dos_messages.inc" ; DEC-13; MS-DOS 3.3 printed pp.337/341.
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
;   - /+n starts at column n (1-based), per Guide p.116. Unknown/malformed
;     switches refuse explicitly; exact diagnostic/status awaits genuine media.
;   - Every AH=40h write is checked (CF and byte count): a failed or short
;     write (full disk) refuses with "Insufficient disk space", status 1
;     (initech-8ao8f). Exact DOS text/status awaits genuine media.
;   - Local safety limits: 64 KiB input and 7680 line records. These are
;     implementation bounds, not established DOS 3.30 limits (initech-0e5gu).
;     The pinned media lacks SORT. Guide pp.116-117 documents sorting and
;     p.357 lists SORT: Insufficient memory, without a numeric input limit.
;
; Ref:   os/initechdos/int21.c do_read (AH=3Fh: EBX=handle, ECX=count, EDX=buf ->
;        EAX=bytes, 0=EOF), do_write (AH=40h), do_puts (AH=09h), do_terminate
;        (AH=4Ch); kernel/include/memory_map.h (PROGRAM_IMAGE=0x00040100 == org; PSP at
;        0x00040000, cmd tail at 0x00040080); MS-DOS 3.3 User's Guide "SORT".
;        the repository contract (cite), the repository contract (fail loud on overflow), the repository contract
;        (deterministic: nasm -f bin), the repository contract (ASCII).
;
; Assembled: nasm -f bin os/initechdos/sort_program.asm -o build/sort_program.bin
; Deployed:  mcopy build/sort_program.bin ::SORT.COM  (NOT baked into kernel).
;
; Mutant (the repository contract): nasm -DSORT_MUTATE_REVERSE flips the default order to
; descending, so the ascending-order emu assertion goes RED (proves the gate and
; the comparator are load-bearing).

bits 32
org 0x00040100                 ; == kernel/include/memory_map.h PROGRAM_IMAGE

; --- Fixed scratch addresses (mirrors datetime_program's absolute-page idiom).
; Scratch ends before ENV_BLOCK (0x6F000); the stack starts at 0x70000.
; and above the program image; the program is tiny so nothing else uses them.
INBUF     equ 0x00050000       ; stdin buffer base
INBUF_MAX equ 0x00010000       ; 64 KiB cap (the repository contract: overflow -> fail loud)
LINEIDX   equ 0x00060000       ; array of 8-byte line records: [off:u32][clen:u32]
REC_SIZE  equ 8
LINE_MAX  equ (0x0006F000 - LINEIDX) / REC_SIZE ; exclusive ENV_BLOCK bound
PSP_TAIL_LEN equ 0x00040080    ; PSP:80h byte0 = command-tail length
PSP_TAIL_TXT equ 0x00040081    ; PSP:81h.. = command-tail text

start:
    cld
    ; ---- Parse the command tail (PSP:80h) for /R and /+n. ----
    xor ebp, ebp               ; EBP = reverse flag (0 = ascending)
    movzx ecx, byte [PSP_TAIL_LEN]
    mov esi, PSP_TAIL_TXT
.tail_scan:
    test ecx, ecx
    jz .tail_done
    mov al, [esi]
    cmp al, ' '
    je .tail_next
    cmp al, 9
    je .tail_next
    cmp al, '/'
    jne .bad_switch
    inc esi
    dec ecx
    jz .bad_switch
    mov al, [esi]
    and al, 0xDF
    cmp al, 'R'
    je .reverse
    cmp byte [esi], '+'
    jne .bad_switch
    inc esi
    dec ecx
    jz .bad_switch
    xor edx, edx
.column_digits:
    movzx eax, byte [esi]
    sub eax, '0'
    cmp eax, 9
    ja .bad_switch
    ; Refuse arithmetic overflow rather than silently selecting another column.
    cmp edx, 429496729
    ja .bad_switch
    imul edx, edx, 10
    add edx, eax
    jc .bad_switch
    inc esi
    dec ecx
    jz .column_done
    mov al, [esi]
    cmp al, '0'
    jb .column_done
    cmp al, '9'
    jbe .column_digits
.column_done:
    test edx, edx
    jz .bad_switch
    dec edx
    mov [column], edx
    jmp .separator
.reverse:
    or ebp, 1
    inc esi
    dec ecx
.separator:
    test ecx, ecx
    jz .tail_done
    mov al, [esi]
    cmp al, '/'
    je .tail_scan
    cmp al, ' '
    je .tail_next
    cmp al, 9
    jne .bad_switch
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
    jz .probe_eof              ; capacity alone does not establish EOF
    mov ecx, eax               ; count = remaining
    mov ah, 0x3F
    xor ebx, ebx               ; handle 0 = stdin
    mov edx, INBUF
    add edx, edi               ; append point
    int 0x21
    jc .read_error             ; never emit a silently partial sort
    test eax, eax
    jz .read_done              ; 0 bytes = EOF
    ; DOS 3.30 SORTEOF treats Ctrl-Z as logical EOF. Scan each chunk
    ; before deciding whether the whole input exceeds the buffer.
    mov ecx, eax
    add edi, INBUF
    mov al, 0x1A
    repne scasb
    jne .chunk_done
    dec edi
    sub edi, INBUF
    jmp .read_done
.chunk_done:
    sub edi, INBUF
    jmp .read_loop
.probe_eof:
    mov eax, 0x3F00
    xor ebx, ebx
    mov ecx, 1
    mov edx, eof_probe
    int 0x21
    jc .read_error
    test eax, eax
    jz .read_done
    cmp byte [eof_probe], 0x1A
    jne .overflow
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
    ; Bound BEFORE either store: no record may reach the inherited environment
    ; or program stack. Safety invariant; exact DOS limits await initech-0e5gu.
    cmp ebx, LINE_MAX
    jae .overflow
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
    push ecx                        ; requested count survives the call
    mov ebx, 1
    add edx, INBUF                  ; flat ptr to content
    mov ah, 0x40
    int 0x21
    pop ecx
    pop edi
    jc .write_error                 ; AH=40h error (CF) is never success
    cmp eax, ecx
    jne .write_error                ; short count = full disk (TR 6-139)
.out_crlf:
    push edi
    mov ah, 0x40
    mov ebx, 1
    mov ecx, 2
    mov edx, crlf
    int 0x21
    pop edi
    jc .write_error
    cmp eax, 2
    jne .write_error
    inc edi
    jmp .out_loop
.out_done:

    ; ---- Terminate cleanly. ----
    mov ah, 0x4C
    mov al, 0
    int 0x21
    int 0x20                    ; defense in depth (the repository contract)

.write_error:
    ; A failed or short write leaves a truncated output file behind the
    ; shell's redirection; it must never report success (initech-8ao8f).
    ; The diagnostic goes to stdout like the other refusals (the shell
    ; repeats "Insufficient disk space" on CON, initech-66oc); ERRORLEVEL
    ; 1 is the signal a batch file can act on. Exact genuine SORT text
    ; and status await authentic media (initech-0e5gu) -- stated safety
    ; assumption: period MSG-DOS-0005 vocabulary, status 1.
    mov edx, msg_write
    jmp .refuse
.bad_switch:
    mov edx, msg_switch
    jmp .refuse
.read_error:
    mov edx, msg_read
    jmp .refuse
.overflow:
    mov edx, msg_oom
.refuse:
    ; stdin exceeded the buffer: fail loud with the DOS 3.3 diagnostic (the repository contract).
    mov ah, 0x09
    int 0x21
    mov ah, 0x4C
    mov al, 1
    int 0x21
    int 0x20

; ---------------------------------------------------------------------------
; compare_lines: case-insensitive ASCII comparison of line A vs line B, then
; adjusted for the reverse flag.  Inputs: EDX=a.off, ECX=a.clen, EBX=b.off,
; EBP=b.clen.  Output: EAX <0 if A<B, 0 if equal, >0 if A>B (AFTER applying
; rflag: reverse negates the result). Clobbers EAX/EBX/ECX/EDX/ESI/EDI/EBP.
; ---------------------------------------------------------------------------
compare_lines:
    ; MS-DOS 3.3 Guide p.116: compare from column n (one-based).
    ; A line shorter than the selected column contributes an empty suffix.
    mov eax, [column]
    cmp ecx, eax
    jae .a_column
    mov eax, ecx
.a_column:
    add edx, eax
    sub ecx, eax
    mov eax, [column]
    cmp ebp, eax
    jae .b_column
    mov eax, ebp
.b_column:
    add ebx, eax
    sub ebp, eax
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
; Period vocabulary, provisional diagnostics, not a captured SORT run.
msg_switch: db MSG_DOS_0064, 0x0D, 0x0A, "$"
msg_read:  db "Error reading file", 0x0D, 0x0A, "$"
msg_write: db MSG_DOS_0005, 0x0D, 0x0A, "$"
eof_probe: db 0
msg_oom:   db MSG_DOS_0060, 0x0D, 0x0A, "$"

; --- BSS-ish scratch (small; baked as zeros in the .COM image) --------------
column:    dd 0                 ; zero-based offset
rflag:     dd 0
total:     dd 0
nlines:    dd 0
key_off:   dd 0
key_clen:  dd 0
