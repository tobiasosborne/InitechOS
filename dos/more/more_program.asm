%include "dos_messages.inc"

; MORE.COM uses the published DOS handle and console services.
; The console sequence comes from more/tests/compat33, captured on MS-DOS 3.30.
; File and pipe output preserve bytes up to the text EOF marker.
bits 32
org 0x00040100                 ; == kernel/include/memory_map.h PROGRAM_IMAGE

RBUF     equ 0x00050000         ; read buffer base
RBUF_MAX equ 0x00001000         ; 4 KiB chunk
PAGE_LINES equ 24               ; screenful before the pager pauses
PSP_TAIL_LEN equ 0x00040080
PSP_TAIL_TXT equ 0x00040081

start:
    cld
    ; DOS 3.30 MOREZ ignored its tail. The golden master requires an
    ; explicit refusal for unsupported switches.
    movzx ecx, byte [PSP_TAIL_LEN]
    mov esi, PSP_TAIL_TXT
.tail:
    jecxz .device
    lodsb
    dec ecx
    cmp al, ' '
    je .tail
    cmp al, 9
    je .tail
    mov edx, msg_switch
    mov ah, 0x09
    int 0x21
    jmp .handle_error
.device:
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
; PASSTHROUGH: stdout is a file/pipe. Copy chunks until Ctrl-Z or physical EOF.
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
    mov esi, eax               ; untrimmed bytes
    mov ecx, eax
    mov edi, RBUF
    mov al, 0x1A               ; DOS 3.30 MOREOUT: text EOF
    repne scasb
    jne .pt_chunk
    dec edi
.pt_chunk:
    sub edi, RBUF
    jz .done
    mov ecx, edi               ; bytes before text EOF, or the full chunk
    mov ah, 0x40               ; WRITE handle 1
    mov ebx, 1
    mov edx, RBUF
    int 0x21
    cmp edi, esi
    jb .done
    jmp .pt_loop

; ---------------------------------------------------------------------------
; PAGINATE: echo stdin to CON and count newlines between key waits.
; ---------------------------------------------------------------------------
.paginate:
    ; DOS 3.30 saves the data handle before putting stderr (CON) in slot 0.
    ; AH=0Ch/01h then reads the keyboard even when the data comes from a file.
    mov eax, 0x4500            ; DUP stdin
    xor ebx, ebx
    int 0x21
    jc .handle_error
    mov [input_handle], eax
    mov eax, 0x4600            ; force-duplicate stderr onto stdin
    mov ebx, 2
    xor ecx, ecx
    int 0x21
    jc .handle_error
    xor ebp, ebp               ; line counter within the current screenful
.pg_read:
    mov ah, 0x3F
    mov ebx, [input_handle]
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
    cmp byte [RBUF + edi], 0x1A
    je .done
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
    mov ax, 0x4C00
    jmp .exit
.handle_error:
    mov ax, 0x4C01
.exit:
    int 0x21
    int 0x20                    ; defense in depth (the repository contract)

msg_switch:  db MSG_DOS_0064, 0x0D, 0x0A, "$"
page_prompt: db 0x0D, "-- More --", "$"
page_break:  db 0x0D, 0x0A, "$"
chunk_len: dd 0
input_handle: dd 0
