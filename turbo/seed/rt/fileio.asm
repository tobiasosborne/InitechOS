; fileio.asm -- B8 thin file-I/O RTL over InitechDOS INT 21h.
;
; beads: initech-ogxv. Ref: ADR-0007 DEC-05;
; spec/int21h_calling_convention.json; live dispatcher functions in
; os/milton/int21.c (do_creat/do_open/do_close/do_read/do_write).
;
; This is a SEPARATE hand-assembled object following seed/rt/start.asm's
; convention. It is linked only into programs whose typechecked AST uses a
; B8 file-I/O builtin; fileless programs retain the pre-B8 runtime bytes.
;
; Object layout (static/frame storage, no heap):
;   +0  dword live JFT handle (0 = closed/unassigned; real files are >= 5)
;   +4  256-byte ASCIIZ filename buffer
; Surface: Assign/Reset/Rewrite/BlockRead/BlockWrite only. Reset/Rewrite
; accept only record-size 1, making transfer counts bytes rather than typed
; records. All entries are cdecl and preserve ebx/esi/edi/ebp.
;
; INT-21h flat ABI (DEC-04a): AH=function, EDX=flat pointer, ECX=count,
; EBX=handle, EAX=result, CF=error with AX=error code. ANY CF error prints a
; deterministic serial diagnostic and exits via AH=4Ch (Rule 2).

bits 32

extern serial_putc
extern serial_puts
extern serial_put_int

section .text

rtl_file_close_current:
    mov ebx, [esi]
    cmp ebx, 5
    jb .closed
    mov eax, 0x3E00           ; AH=3Eh CLOSE, EBX=handle
    int 0x21
    jc rtl_file_fail
    mov dword [esi], 0
.closed:
    ret

global rtl_file_assign
rtl_file_assign:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    mov esi, [ebp+8]
    call rtl_file_close_current
    mov ebx, [ebp+8]
    mov esi, [ebp+12]
    movzx ecx, byte [esi]
    inc esi
    lea edi, [ebx+4]
    cld
    rep movsb
    mov byte [edi], 0
    mov dword [ebx], 0
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

global rtl_file_reset
rtl_file_reset:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    cmp dword [ebp+12], 1
    jne rtl_file_bad_argument
    mov esi, [ebp+8]
    call rtl_file_close_current
    mov esi, [ebp+8]
    lea edx, [esi+4]
    mov eax, 0x3D00           ; AH=3Dh OPEN, AL=0 read-only
    int 0x21
    jc rtl_file_fail
    mov [esi], eax
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

global rtl_file_rewrite
rtl_file_rewrite:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    cmp dword [ebp+12], 1
    jne rtl_file_bad_argument
    mov esi, [ebp+8]
    call rtl_file_close_current
    mov esi, [ebp+8]
    lea edx, [esi+4]
    xor ecx, ecx              ; CX=0 normal file attributes
    mov eax, 0x3C00           ; AH=3Ch CREAT/truncate
    int 0x21
    jc rtl_file_fail
    mov [esi], eax
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

global rtl_file_blockread
rtl_file_blockread:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    mov ecx, [ebp+16]
    test ecx, ecx
    js rtl_file_bad_argument
    mov esi, [ebp+8]
    mov ebx, [esi]
    mov edx, [ebp+12]
    mov eax, 0x3F00           ; AH=3Fh READ
    int 0x21
    jc rtl_file_fail
    mov edi, [ebp+20]
    mov [edi], eax
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

global rtl_file_blockwrite
rtl_file_blockwrite:
    xor eax, eax
    jmp rtl_file_blockwrite_enter

global rtl_file_blockwrite_short
rtl_file_blockwrite_short:
    mov eax, 1
    jmp rtl_file_blockwrite_enter

global rtl_file_blockwrite_wrong_handle
rtl_file_blockwrite_wrong_handle:
    mov eax, 2

rtl_file_blockwrite_enter:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    mov esi, eax
    mov ecx, [ebp+16]
    test ecx, ecx
    js rtl_file_bad_argument
    cmp esi, 1
    jne .count_ready
    test ecx, ecx
    jz .count_ready
    dec ecx                   ; SEED_MUT_FILEIO_SHORT_WRITE
.count_ready:
    mov edi, [ebp+8]
    mov ebx, [edi]
    cmp esi, 2
    jne .handle_ready
    inc ebx                   ; SEED_MUT_FILEIO_WRONG_HANDLE
.handle_ready:
    mov edx, [ebp+12]
    mov eax, 0x4000           ; AH=40h WRITE
    int 0x21
    jc rtl_file_fail
    cmp esi, 1
    jne .actual_ready
    cmp dword [ebp+16], 0
    je .actual_ready
    inc eax                   ; lie: report original count, hide dropped tail
.actual_ready:
    mov edi, [ebp+20]
    mov [edi], eax
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

rtl_file_bad_argument:
    mov eax, 1
    jmp rtl_file_fail

rtl_file_fail:
    movzx eax, ax
    push eax
    mov eax, rtl_file_error_prefix
    call serial_puts
    pop eax
    call serial_put_int
    mov al, 10
    call serial_putc
    mov eax, 0x4C01           ; AH=4Ch EXIT, AL=1
    int 0x21
    cli
.halt:
    hlt
    jmp .halt

section .rodata
rtl_file_error_prefix:
    db 'FILEIO-ERROR AX=', 0
