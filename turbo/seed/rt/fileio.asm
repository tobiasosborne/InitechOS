; fileio.asm -- thin file-I/O and keyboard RTL over InitechDOS INT 21h.
;
; beads: initech-ogxv (B8), initech-n6r9v (recoverable I/O, Close, Erase,
; Rename, IOResult, ReadKey). Ref: ADR-0007 DEC-05;
; spec/int21h_calling_convention.json; live dispatcher functions in
; kernel/int21.c (do_creat/do_open/do_close/do_read/do_write/do_unlink/
; do_rename/do_conin_raw).
;
; This is a SEPARATE hand-assembled object following seed/rt/start.asm's
; convention. It is linked only into programs whose typechecked AST uses a
; file-I/O builtin; fileless programs retain the pre-B8 runtime bytes.
;
; Object layout (static/frame storage, no heap):
;   +0  dword live JFT handle (0 = closed/unassigned; real files are >= 5)
;   +4  256-byte ASCIIZ filename buffer
; Reset/Rewrite accept only record-size 1, making transfer counts bytes
; rather than typed records. All entries are cdecl and preserve
; ebx/esi/edi/ebp.
;
; INT-21h flat ABI (DEC-04a): AH=function, EDX=flat pointer (EDI the second
; one for RENAME), ECX=count, EBX=handle, EAX=result, CF=error with AX=error
; code.
;
; Errors follow Turbo Pascal's InOutRes model (TP 5.5 Reference Guide,
; IOResult and the $I compiler directive; Free Pascal behaves the same and
; is the independent reference in the corpus): an operation that fails
; stores the DOS error code in rtl_inoutres and returns; while it is
; non-zero, I/O operations do nothing. IOResult returns and clears it.
; Code compiled in the default {$I+} state calls rtl_io_check after each
; I/O call, which prints the same deterministic serial diagnostic as before
; and exits via AH=4Ch (CLAUDE.md, The constitution).

bits 32

extern serial_putc
extern serial_puts
extern serial_put_int

section .text

; eax = &rtl_inoutres. Position independent: the dword lives beside the code
; (flat image, no memory protection), so neither output form needs a
; relocation for it.
rtl_io_state:
    call .here
.here:
    pop eax
    add eax, rtl_inoutres - .here
    ret

; ZF clear when an earlier error is still pending (skip the operation).
rtl_io_pending:
    call rtl_io_state
    cmp dword [eax], 0
    ret

; Close [esi]'s handle if one is open. CF set with AX=error on failure.
rtl_file_close_current:
    mov ebx, [esi]
    cmp ebx, 5
    jb .closed
    mov eax, 0x3E00           ; AH=3Eh CLOSE, EBX=handle
    int 0x21
    jc .failed
    mov dword [esi], 0
.closed:
    clc
.failed:
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
    jc rtl_file_fail
    mov ebx, [ebp+8]
    mov esi, [ebp+12]
    movzx ecx, byte [esi]
    inc esi
    lea edi, [ebx+4]
    cld
    rep movsb
    mov byte [edi], 0
    mov dword [ebx], 0
    jmp rtl_file_return

global rtl_file_reset
rtl_file_reset:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    call rtl_io_pending
    jnz rtl_file_return
    cmp dword [ebp+12], 1
    jne rtl_file_bad_argument
    mov esi, [ebp+8]
    call rtl_file_close_current
    jc rtl_file_fail
    lea edx, [esi+4]
    mov eax, 0x3D00           ; AH=3Dh OPEN, AL=0 read-only
    int 0x21
    jc rtl_file_fail
    mov [esi], eax
    jmp rtl_file_return

global rtl_file_rewrite
rtl_file_rewrite:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    call rtl_io_pending
    jnz rtl_file_return
    cmp dword [ebp+12], 1
    jne rtl_file_bad_argument
    mov esi, [ebp+8]
    call rtl_file_close_current
    jc rtl_file_fail
    lea edx, [esi+4]
    xor ecx, ecx              ; CX=0 normal file attributes
    mov eax, 0x3C00           ; AH=3Ch CREAT/truncate
    int 0x21
    jc rtl_file_fail
    mov [esi], eax
    jmp rtl_file_return

global rtl_file_blockread
rtl_file_blockread:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    call rtl_io_pending
    jnz rtl_file_return
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
    jmp rtl_file_return

global rtl_file_blockwrite
rtl_file_blockwrite:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    call rtl_io_pending
    jnz rtl_file_return
    mov ecx, [ebp+16]
    test ecx, ecx
    js rtl_file_bad_argument
    mov edi, [ebp+8]
    mov ebx, [edi]
    mov edx, [ebp+12]
    mov eax, 0x4000           ; AH=40h WRITE
    int 0x21
    jc rtl_file_fail
    mov edi, [ebp+20]
    mov [edi], eax
    jmp rtl_file_return

; Close(f): TP error 103 "File not open" when no handle is open.
global rtl_file_close
rtl_file_close:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    call rtl_io_pending
    jnz rtl_file_return
    mov esi, [ebp+8]
    cmp dword [esi], 5
    jb .not_open
    call rtl_file_close_current
    jc rtl_file_fail
    jmp rtl_file_return
.not_open:
    mov eax, 103
    jmp rtl_file_fail

; Erase(f): delete the file named by f (AH=41h, EDX=name).
global rtl_file_erase
rtl_file_erase:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    call rtl_io_pending
    jnz rtl_file_return
    mov esi, [ebp+8]
    lea edx, [esi+4]
    mov eax, 0x4100           ; AH=41h UNLINK
    int 0x21
    jc rtl_file_fail
    jmp rtl_file_return

; Rename(f, newname): AH=56h, EDX=old name, EDI=new name (ASCIIZ built on
; the stack); on success f carries the new name, as in TP.
global rtl_file_rename
rtl_file_rename:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    call rtl_io_pending
    jnz rtl_file_return
    sub esp, 260
    mov esi, [ebp+12]
    movzx ecx, byte [esi]
    inc esi
    mov edi, esp
    cld
    rep movsb
    mov byte [edi], 0
    mov esi, [ebp+8]
    lea edx, [esi+4]
    mov edi, esp
    mov eax, 0x5600           ; AH=56h RENAME
    int 0x21
    jc .failed
    mov esi, esp
    mov edi, [ebp+8]
    add edi, 4
    mov ecx, 256
    rep movsb
    add esp, 260
    jmp rtl_file_return
.failed:
    add esp, 260
    jmp rtl_file_fail

rtl_file_bad_argument:
    mov eax, 1
    jmp rtl_file_fail

; Record AX as the pending error and return from the cdecl frame above.
rtl_file_fail:
    movzx edx, ax
    call rtl_io_state
    mov [eax], edx
rtl_file_return:
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

; IOResult: return the pending error and clear it.
global rtl_ioresult
rtl_ioresult:
    call rtl_io_state
    mov edx, eax
    mov eax, [edx]
    mov dword [edx], 0
    ret

; {$I+}: abort on a pending error with the B8 diagnostic.
global rtl_io_check
rtl_io_check:
    push eax
    call rtl_io_state
    mov eax, [eax]
    test eax, eax
    jnz .abort
    pop eax
    ret
.abort:
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

; ReadKey: AH=07h direct console input without echo. An extended key
; arrives as 0 and then its scan code on the next call (DOS 3.30 TR 6-11).
global rtl_readkey
rtl_readkey:
    push ebx
    mov eax, 0x0700
    int 0x21
    movzx eax, al
    pop ebx
    ret

rtl_inoutres:
    dd 0

section .rodata
rtl_file_error_prefix:
    db 'FILEIO-ERROR AX=', 0

section .text
; Initech FileAttributes(file): DOS AH=43h AL=0. Returns the mask or
; negative DOS error; it does not use/alter the Pascal pending I/O error.
; TP5.5 MINT06 54 requires a read-only check before write-aside renames.
global rtl_file_attributes
rtl_file_attributes:
    push ebx
    mov edx, [esp+8]
    add edx, 4
    mov eax, 0x4300
    int 0x21
    jc .error
    movzx eax, cx
    pop ebx
    ret
.error:
    movzx eax, ax
    neg eax
    pop ebx
    ret
