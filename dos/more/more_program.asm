; MORE.COM uses the published DOS handle and console services.
; The console sequence comes from more/tests/compat33, captured on MS-DOS 3.30.
; File and pipe output use the InitechDOS byte-for-byte filter path.
bits 32
org 0x00040100                 ; == kernel/include/memory_map.h PROGRAM_IMAGE

RBUF     equ 0x00050000         ; read buffer base
RBUF_MAX equ 0x00001000         ; 4 KiB chunk
PAGE_LINES equ 24               ; screenful before the pager pauses

start:
    ; ---- AH=44h AL=00h Get Device Information for handle 1 (stdout). ----
    mov eax, 0x00004400        ; AH=44h, AL=00h (get device info)
    mov ebx, 1                 ; handle 1 = stdout
    int 0x21
    ; DX = device-info word.  bit15 (0x8000) SET -> character device (CON, paginate);
    ; CLEAR -> disk file / pipe (passthrough).  CF set (error) -> be safe: passthrough.
    jc .passthrough
    test edx, 0x8000
    jz .passthrough
    jmp .paginate

; ---------------------------------------------------------------------------
; PASSTHROUGH: stdout is a file/pipe.  Copy stdin -> stdout in chunks, verbatim,
; without a pause or page cue.
; ---------------------------------------------------------------------------
.passthrough:
.pt_loop:
    mov ah, 0x3F               ; READ handle 0
    xor ebx, ebx
    mov ecx, RBUF_MAX
    mov edx, RBUF
    int 0x21
    jc .done                   ; read error -> stop
    test eax, eax
    jz .done                   ; EOF
    mov ecx, eax               ; bytes to write
    mov ah, 0x40               ; WRITE handle 1
    mov ebx, 1
    mov edx, RBUF
    int 0x21
    jmp .pt_loop

; ---------------------------------------------------------------------------
; PAGINATE: echo stdin to CON and count newlines between key waits.
; ---------------------------------------------------------------------------
.paginate:
    xor ebp, ebp               ; line counter within the current screenful
.pg_read:
    mov ah, 0x3F
    xor ebx, ebx
    mov ecx, RBUF_MAX
    mov edx, RBUF
    int 0x21
    jc .done
    test eax, eax
    jz .done
    ; write byte-by-byte so we can pause exactly on the 24th newline.
    xor edi, edi               ; index into RBUF
    mov [chunk_len], eax
.pg_byte:
    cmp edi, [chunk_len]
    jae .pg_read
    push edi
    mov ecx, 1
    mov ah, 0x40
    mov ebx, 1
    mov edx, RBUF
    add edx, edi
    int 0x21
    pop edi
    mov al, [RBUF + edi]
    inc edi
    cmp al, 0x0A               ; LF ends a line
    jne .pg_byte
    inc ebp
    cmp ebp, PAGE_LINES
    jb .pg_byte
    mov edx, page_prompt
    mov ah, 0x09
    int 0x21
    mov eax, 0x0C01           ; flush typeahead, echo a key, check Ctrl-C
    int 0x21
    mov edx, page_break
    mov ah, 0x09
    int 0x21
    xor ebp, ebp
    dec edi                    ; replay the boundary LF on the new page
    jmp .pg_byte

.done:
    mov ah, 0x4C
    mov al, 0
    int 0x21
    int 0x20                    ; defense in depth (the repository contract)

page_prompt: db 0x0D, "-- More --", "$"
page_break:  db 0x0D, 0x0A, "$"
chunk_len: dd 0
