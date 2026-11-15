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

; ParamCount/ParamStr: DOS 3.30 PSP tail, AH=62h. Parameters are separated
; by spaces/tabs (TP 5.5 Reference Guide; FPC oracle). ParamStr copies into
; caller-owned ShortString storage so nested results do not alias.
rtl_tail:
    mov eax, 0x6200
    int 0x21
    movzx esi, bx
    shl esi, 4
    movzx ecx, byte [esi+0x80]
    add esi, 0x81
    ret

global rtl_paramcount
rtl_paramcount:
    push ebx
    push esi
    push edi
    call rtl_tail
    xor eax, eax
    xor edi, edi
    xor edx, edx
.loop:
    test ecx, ecx
    jz .done
    lodsb
    ; AL is the character, keep the count in EDI.
    cmp al, ' '
    je .space
    cmp al, 9
    je .space
    test edx, edx
    jnz .next
    inc edi
    mov edx, 1
    jmp .next
.space:
    xor edx, edx
.next:
    dec ecx
    jmp .loop
.done:
    mov eax, edi
    pop edi
    pop esi
    pop ebx
    ret

global rtl_paramstr
rtl_paramstr:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    mov edi, [ebp+12]        ; destination ShortString
    mov byte [edi], 0
    call rtl_tail
    mov ebx, [ebp+8]         ; requested index
    cmp ebx, 0
    jl .done
    je .program
    xor edx, edx            ; current parameter index
.skip:
    test ecx, ecx
    jz .done
    cmp byte [esi], ' '
    je .skipbyte
    cmp byte [esi], 9
    jne .start
.skipbyte:
    inc esi
    dec ecx
    jmp .skip
.start:
    inc edx
.word:
    test ecx, ecx
    jz .done
    mov al, [esi]
    cmp al, ' '
    je .skip
    cmp al, 9
    je .skip
    cmp edx, ebx
    jne .next
    inc byte [edi]
    movzx eax, byte [edi]
    mov bl, [esi]
    mov [edi+eax], bl
    mov ebx, [ebp+8]
.next:
    inc esi
    dec ecx
    jmp .word
.program:
    ; DOS 3.x environments may carry a count and executable path after
    ; double NUL. An absent trailer gives an empty result, never guessed.
    sub esi, 0x81
    movzx esi, word [esi+0x2c]
    shl esi, 4
    mov ecx, 4093
.env:
    cmp word [esi], 0
    je .trailer
    inc esi
    loop .env
    jmp .done
.trailer:
    cmp word [esi+2], 1
    jne .done
    add esi, 4
.name:
    mov al, [esi]
    test al, al
    jz .done
    movzx edx, byte [edi]
    cmp edx, 255
    je .done
    inc edx
    mov [edi+edx], al
    mov [edi], dl
    inc esi
    jmp .name
.done:
    mov eax, edi
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

; ScreenPage(op, byte*): OEM page packet, kernel/include/dos_oem.h,
; kernel README AH=FBh. The kernel validates the complete fixed packet.
global rtl_screen_page
rtl_screen_page:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    sub esp, 4004
    ; Pascal char-array elements occupy dword slots. Marshal the published
    ; packed 4002-byte packet instead of exposing that language layout.
    cmp dword [ebp+8], 0
    je .call
    mov esi, [ebp+12]
    mov edi, esp
    mov ecx, 4002
.pack:
    mov al, [esi]
    stosb
    add esi, 4
    loop .pack
.call:
    mov eax, [ebp+8]
    and eax, 0xff
    or eax, 0xfb00
    mov edx, esp
    int 0x21
    jc .failed
    cmp dword [ebp+8], 0
    jne .done
    mov esi, esp
    mov edi, [ebp+12]
    mov ecx, 4002
.unpack:
    movzx eax, byte [esi]
    stosd
    inc esi
    loop .unpack
.done:
    add esp, 4004
    jmp rtl_file_return
.failed:
    add esp, 4004
    jmp rtl_file_fail

; Execute(path): existing DOS AH=4Bh/4Dh, empty command tail, inherited
; environment. Return negative DOS load error, otherwise full exit status.
global rtl_execute
rtl_execute:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    sub esp, 280
    mov esi, [ebp+8]
    movzx ecx, byte [esi]
    inc esi
    mov edi, esp
    rep movsb
    mov byte [edi], 0
    mov edx, esp
    lea ebx, [esp+260]
    mov dword [ebx], 0
    lea eax, [esp+276]
    mov dword [ebx+4], eax
    mov word [eax], 0x0d00
    mov dword [ebx+8], 0
    mov dword [ebx+12], 0
    mov eax, 0x4b00
    int 0x21
    jc .error
    mov eax, 0x4d00
    int 0x21
    movzx eax, ax
    jmp .done
.error:
    movzx eax, ax
    neg eax
.done:
    add esp, 280
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

global rtl_halt
rtl_halt:
    mov eax, [esp+4]
    and eax, 0xff
    or eax, 0x4c00
    int 0x21
    cli
    hlt
    jmp $-1

; DOS AH=48h largest-free-block probe (request 65535 paragraphs). The
; published conventional arena is below 1 MiB; an unexpected allocation
; is freed through AH=49h. Result is bytes, for the IDE compile box.
global rtl_dos_memory
rtl_dos_memory:
    push ebx
    mov ebx, 65535
    mov eax, 0x4800
    int 0x21
    jnc .free
    movzx eax, bx
    shl eax, 4
    pop ebx
    ret
.free:
    movzx ebx, ax
    mov eax, 0x4900
    int 0x21
    mov eax, 1048560
    pop ebx
    ret
