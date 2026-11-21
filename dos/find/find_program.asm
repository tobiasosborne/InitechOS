%include "dos_messages.inc"

; FIND.COM, using the fixed native COM program and scratch layout.
; Grammar, streams and statuses: find/tests/compat33 (genuine MS-DOS 3.30).
; /I is the existing InitechDOS extension.
bits 32
org 0x00040100

PSP_TAIL_LEN equ 0x00040080
PSP_TAIL_TXT equ 0x00040081
LINEBUF      equ 0x00050000
LINE_MAX     equ 0x00010000
READBUF      equ LINEBUF + LINE_MAX
READ_MAX     equ 0x00001000

F_V equ 1
F_C equ 2
F_N equ 4
F_I equ 8

start:
    cld
    movzx ebp, byte [PSP_TAIL_LEN]
    add ebp, PSP_TAIL_TXT
    mov esi, PSP_TAIL_TXT
.option:
    call skip_spaces
    cmp esi, ebp
    jae missing_string
    lodsb
    cmp al, '"'
    je .search_string
    cmp al, '/'
    jne syntax_error
    cmp esi, ebp
    jae missing_string
    mov al, [esi]
    cmp al, ' '
    je .option
    cmp al, 9
    je .option
    inc esi
    cmp al, 'a'
    jb .upper
    cmp al, 'z'
    ja .upper
    sub al, 'a' - 'A'
.upper:
    cmp al, 'V'
    je .invert
    cmp al, 'C'
    je .count
    cmp al, 'N'
    je .number
    cmp al, 'I'
    je .ignore_case
    mov [invalid_letter], al
    mov edx, msg_switch
    mov ecx, msg_switch_len
    call err_write
    jmp .option
.invert:
    or byte [flags], F_V
    jmp .option
.count:
    or byte [flags], F_C
    jmp .option
.number:
    or byte [flags], F_N
    jmp .option
.ignore_case:
    or byte [flags], F_I
    jmp .option

.search_string:
    mov edi, needle
.copy_string:
    cmp esi, ebp
    jae syntax_error
    lodsb
    cmp al, '"'
    jne .copy_char
    cmp esi, ebp
    jae .end_string
    cmp byte [esi], '"'
    jne .end_string
    inc esi                    ; a doubled quote is literal
.copy_char:
    stosb
    jmp .copy_string
.end_string:
    sub edi, needle
    jz status_one
    mov [nlen], edi
    call skip_spaces
    cmp esi, ebp
    jae .stdin
    mov [tail_end], ebp
.next_file:
    mov edi, filename
.copy_file:
    lodsb
    stosb
    cmp esi, ebp
    jae .end_file
    mov al, [esi]
    cmp al, ' '
    je .end_file
    cmp al, 9
    jne .copy_file
.end_file:
    mov byte [edi], 0
    sub edi, filename
    mov [filename_len], edi
    call skip_spaces
    mov [tail_cursor], esi

    mov eax, 0x3D00
    mov edx, filename
    int 0x21
    jc .open_failed
    mov [source], eax
    mov edx, heading
    mov ecx, heading_len
    call out_write
    mov edx, filename
    mov ecx, [filename_len]
    call out_write
    test byte [flags], F_C
    jnz .scan
    call out_crlf
.scan:
    call scan_source
    mov eax, 0x3E00
    mov ebx, [source]
    int 0x21
    jmp .advance
.open_failed:
    mov edx, msg_missing
    mov ecx, msg_missing_len
    cmp ax, 5                  ; DOS access denied, including a directory
    jne .file_diagnostic
    mov edx, msg_denied
    mov ecx, msg_denied_len
.file_diagnostic:
    call err_write
    mov edx, filename
    mov ecx, [filename_len]
    call err_write
    mov edx, crlf
    mov ecx, 2
    call err_write
.advance:
    mov esi, [tail_cursor]
    mov ebp, [tail_end]
    cmp esi, ebp
    jb .next_file
    jmp success

.stdin:
    call scan_source
success:
    mov ax, 0x4C00
    int 0x21
status_one:
    mov ax, 0x4C01
    int 0x21

missing_string:
    mov edx, msg_missing_string
    mov ecx, msg_missing_string_len
    jmp parse_error
syntax_error:
    mov edx, msg_syntax
    mov ecx, msg_syntax_len
parse_error:
    call err_write
    mov ax, 0x4C02
    int 0x21

skip_spaces:
.next:
    cmp esi, ebp
    jae .done
    cmp byte [esi], ' '
    je .skip
    cmp byte [esi], 9
    jne .done
.skip:
    inc esi
    jmp .next
.done:
    ret

; Read successive chunks while retaining the current line. The scratch cap
; applies to one line; a file can contain any number of short lines.
scan_source:
    mov dword [count], 0
    mov dword [lineno], 0
    mov dword [line_len], 0
.read:
    mov eax, 0x3F00
    mov ebx, [source]
    mov ecx, READ_MAX
    mov edx, READBUF
    int 0x21
    jc read_error
    test eax, eax
    jz .eof
    mov esi, READBUF
    lea ebp, [esi + eax]
.byte:
    cmp esi, ebp
    jae .read
    lodsb
    cmp al, 0x1A
    je .eof
    cmp al, 0x0A
    je .newline
    mov ecx, [line_len]
    cmp ecx, LINE_MAX
    jae limit_error
    mov [LINEBUF + ecx], al
    inc dword [line_len]
    jmp .byte
.newline:
    mov dword [line_end_len], 1
    call find_line
    jmp .byte
.eof:
    cmp dword [line_len], 0
    je .finish
    mov dword [line_end_len], 2 ; supply CRLF for an unterminated final line
    call find_line
.finish:
    test byte [flags], F_C
    jz .done
    cmp dword [source], 0
    je .emit_count
    mov ecx, 2
    mov edx, colon_space
    call out_write
.emit_count:
    mov eax, [count]
    call emit_decimal
    call out_crlf
.done:
    ret

find_line:
    push esi                    ; input cursor across matching and ASCII output
    inc dword [lineno]
    call line_contains
    test byte [flags], F_V
    jz .no_invert
    xor eax, 1
.no_invert:
    test eax, eax
    jz .next_line              ; no match -> next line
    ; matched.
    inc dword [count]
    test byte [flags], F_C
    jnz .next_line
    test byte [flags], F_N
    jz .emit
    call emit_lineno
.emit:
    mov ecx, [line_len]
    jecxz .terminator          ; zero-byte AH=40h has file-truncation semantics
    mov edx, LINEBUF
    call out_write
.terminator:
    mov ecx, [line_end_len]
    mov edx, crlf + 2
    sub edx, ecx
    call out_write
.next_line:
    mov dword [line_len], 0
    pop esi
    ret

; Match the substring within the line's content, ignoring the trailing CR.
line_contains:
    mov edx, [line_len]
    test edx, edx
    jz .no
    cmp byte [LINEBUF + edx - 1], 0x0D
    jne .content
    dec edx
.content:
    sub edx, [nlen]
    jb .no                     ; line shorter than needle
    xor ebx, ebx               ; candidate offset
.try:
    lea esi, [LINEBUF + ebx]
    mov edi, needle
    mov ecx, [nlen]
.cmp:
    mov al, [esi]
    mov ah, [edi]
    test byte [flags], F_I
    jz .compare
    cmp al, 'a'
    jb .fold_needle
    cmp al, 'z'
    ja .fold_needle
    sub al, 'a' - 'A'
.fold_needle:
    cmp ah, 'a'
    jb .compare
    cmp ah, 'z'
    ja .compare
    sub ah, 'a' - 'A'
.compare:
    cmp al, ah
    jne .next
    inc esi
    inc edi
    dec ecx
    jnz .cmp
    mov eax, 1
    ret
.next:
    inc ebx
    cmp ebx, edx
    jbe .try
.no:
    xor eax, eax
    ret

emit_lineno:
    mov ecx, 1
    mov edx, lbrack
    call out_write
    mov eax, [lineno]
    call emit_decimal
    mov ecx, 1
    mov edx, rbrack
    jmp out_write

emit_decimal:
    mov edi, numbuf_end
    mov ecx, 10
.digit:
    xor edx, edx
    div ecx
    add dl, '0'
    dec edi
    mov [edi], dl
    test eax, eax
    jnz .digit
    mov edx, edi
    mov ecx, numbuf_end
    sub ecx, edi
    jmp out_write

out_crlf:
    mov ecx, 2
    mov edx, crlf
out_write:
    mov ebx, 1
    mov ah, 0x40
    int 0x21
    jc write_error
    cmp eax, ecx
    jne write_error
    ret
err_write:
    mov ebx, 2
    mov ah, 0x40
    int 0x21
    ret

; Provisional I/O and line-capacity failures, tracked in initech-pd6s8.
read_error:
    mov edx, msg_read
    jmp refuse
limit_error:
    mov edx, msg_limit
    jmp refuse
write_error:
    mov edx, msg_write
refuse:
    mov ah, 0x09
    int 0x21
    jmp status_one

crlf:       db 0x0D, 0x0A
heading:    db 0x0D, 0x0A, '---------- '
heading_len equ $ - heading
colon_space: db ': '
lbrack:     db '['
rbrack:     db ']'

msg_missing_string: db 'FIND: Invalid number of parameters', 0x0D, 0x0A
msg_missing_string_len equ $ - msg_missing_string
msg_syntax: db 'FIND: Syntax error', 0x0D, 0x0A
msg_syntax_len equ $ - msg_syntax
msg_missing: db 'FIND: File not found '
msg_missing_len equ $ - msg_missing
msg_denied: db 'FIND: Access denied '
msg_denied_len equ $ - msg_denied
msg_switch: db 'FIND: Invalid Parameter '
invalid_letter: db 0, 0x0D, 0x0A
msg_switch_len equ $ - msg_switch
msg_limit: db MSG_DOS_0060, 0x0D, 0x0A, '$'
msg_read:  db 'Error reading file', 0x0D, 0x0A, '$'
msg_write: db MSG_DOS_0005, 0x0D, 0x0A, '$'

flags:        db 0
source:       dd 0
tail_cursor:  dd 0
tail_end:     dd 0
nlen:         dd 0
filename_len: dd 0
line_len:     dd 0
line_end_len: dd 0
count:        dd 0
lineno:       dd 0
needle:       times 128 db 0
filename:     times 128 db 0
numbuf:       times 10 db 0
numbuf_end:
