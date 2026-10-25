; ovltest_program.asm -- OVLTEST.COM, the in-emulator driver of INT 21h
; AH=4Bh AL=03h LOAD OVERLAY (make test-exec-overlay).
;
; Ref: docs/adr/ADR-0003-AMENDMENT-DEC-19-INT21h-AH4Bh-AL03h-Load-Overlay.md
;      IBM DOS 3.30 TR p. 6-196 (AL=03H), p. 6-197 (the block), p. 11-5 (the
;        caller owns the target memory; no PSP, no allocation), p. 6-42 (codes
;        2 file not found, 9 invalid memory block address);
;      spec/dos_structs.h exec_overlay_block_t {00h load, 04h factor};
;      os/milton/ovl_fixture.asm (the overlay's fixed +0/+4/+8 layout).
;      CLAUDE.md Law 2 (each marker is printed only after its check passed),
;      Rule 2 (every failure names itself and exits rc=1), Rule 12 (ASCII).
;
; Runs top-level (PSP at PROGRAM_BASE; the kernel EXECs it by name), position
; independent (call/pop base, like nested_exec_program.asm). Overlay buffers
; live in the program's own BSS (the DEC-19.4 image/BSS window) or in an
; AH=48h block (the TR p. 11-5 way). The overlay is CALLed only after its
; relocated dword checked out, so an unrelocated image is never executed.
;
; Markers (in order): OVL-MZ-CALLED (printed BY the overlay), OVL-MZ-OK,
; OVL-FACTOR-OK, OVL-RAW-VERBATIM (printed FROM the loaded raw bytes),
; OVL-BLOCK-OK, OVL-REFUSE-OK, OVL-ALL-OK; any failure: OVL-FAIL <what>.
; -DOVL_FOREIGN builds the untagged-MZ leg: the kernel must panic before
; OVL-FOREIGN-AFTER can print (DEC-08a.5).
bits 32
org 0

FACTOR  equ 0x00123400          ; deliberately unrelated to any load address
RAW_LEN equ 19                  ; "OVL-RAW-VERBATIM",13,10,"$" (OVLR.BIN)
KERNEL  equ 0x00500000          ; KERNEL_BASE (spec/memory_map.h)

start:
    call base
base:
    pop ebp
    sub ebp, base               ; ebp = load delta; ebx = our PSP (entry ABI)
    mov [ebp+psp_lin], ebx

%ifdef OVL_FOREIGN
    lea edi, [ebp+buf1]
    mov [ebp+blk], edi
    mov [ebp+blk+4], edi
    lea edx, [ebp+name_foreign]
    call ovl_load               ; MUST NOT RETURN: PANIC foreign-mz
    lea edx, [ebp+m_foreign_after]
    call puts
    mov eax, 0x4C00
    int 0x21
%else
    ; ---- 1. tagged InitechMZ into our own BSS, factor == load address ----
    lea edi, [ebp+buf1]
    mov [ebp+blk], edi
    mov [ebp+blk+4], edi
    lea edx, [ebp+name_mz]
    call ovl_load
    jc near f_mz_load
    mov eax, [edi+4]
    add eax, edi
    cmp eax, [edi]              ; relocated +0 == load + (msg - $$)?
    jne near f_mz_reloc
    lea eax, [edi+8]
    call eax                    ; into the overlay: prints OVL-MZ-CALLED
    cmp eax, [edi]              ; it read +0 back through a relocated address
    jne near f_mz_call
    lea edx, [ebp+m_mz_ok]
    call puts

    ; ---- 2. the factor, not the load address, is what gets added ----
    lea edi, [ebp+buf2]
    mov [ebp+blk], edi
    mov dword [ebp+blk+4], FACTOR
    lea edx, [ebp+name_mz]
    call ovl_load
    jc near f_factor_load
    mov eax, [edi+4]
    add eax, FACTOR
    cmp eax, [edi]
    jne near f_factor
    lea edx, [ebp+m_factor_ok]
    call puts

    ; ---- 3. a raw file is copied verbatim (factor ignored), exact size ----
    lea edi, [ebp+buf3]
    mov ecx, 64
    mov al, 0xA5
    push edi
    rep stosb                   ; canary-fill the whole buffer
    pop edi
    mov [ebp+blk], edi
    mov dword [ebp+blk+4], FACTOR
    lea edx, [ebp+name_raw]
    call ovl_load
    jc near f_raw_load
    cmp byte [edi+RAW_LEN], 0xA5
    jne near f_raw_size
    mov edx, edi
    call puts                   ; prints OVL-RAW-VERBATIM from the loaded bytes

    ; ---- 4. an AH=48h block: exact fit at its very end is accepted ----
    mov ebx, 4                  ; 4 paragraphs = 64 bytes
    mov eax, 0x4800
    int 0x21
    jc near f_alloc
    movzx esi, ax
    mov [ebp+blk_seg], esi
    shl esi, 4                  ; esi = the block's flat address
    lea edi, [esi+64-RAW_LEN]
    mov [ebp+blk], edi
    mov dword [ebp+blk+4], 0
    lea edx, [ebp+name_raw]
    call ovl_load
    jc near f_block_fit
    cmp dword [edi], 0x2D4C564F ; "OVL-"
    jne near f_block_fit
    lea edx, [ebp+m_block_ok]
    call puts

    ; ---- 5. refusals: CF=1 and the DOS code, nothing loaded ----
    lea edi, [esi+64-RAW_LEN+1] ; one byte past the block (onto the next MCB)
    lea edx, [ebp+name_raw]
    call ovl_expect9
    jne near f_one_past
    mov edi, KERNEL             ; the kernel image
    lea edx, [ebp+name_raw]
    call ovl_expect9
    jne near f_kernel
    mov edi, [ebp+psp_lin]      ; our own PSP
    lea edx, [ebp+name_raw]
    call ovl_expect9
    jne near f_psp
    lea edi, [esi-16]           ; the block's own MCB header
    lea edx, [ebp+name_raw]
    call ovl_expect9
    jne near f_mcb
    lea edx, [ebp+name_raw]     ; EBX = 0: no parameter block
    xor ebx, ebx
    mov eax, 0x4B03
    int 0x21
    jnc near f_noblock
    cmp ax, 9
    jne near f_noblock
    lea edi, [ebp+buf2]         ; a file that does not exist -> 0002h
    mov [ebp+blk], edi
    lea edx, [ebp+name_none]
    call ovl_load
    jnc near f_missing
    cmp ax, 2
    jne near f_missing
    lea edx, [ebp+m_refuse_ok]
    call puts

    mov ebx, [ebp+blk_seg]      ; give the block back (AH=49h, BX = segment)
    mov eax, 0x4900
    int 0x21
    jc near f_free
    lea edx, [ebp+m_all_ok]
    call puts
    mov eax, 0x4C00
    int 0x21
%endif

; AH=4Bh AL=03h with EDX = name, EBX -> blk. Returns CF/AX untouched.
ovl_load:
    lea ebx, [ebp+blk]
    mov eax, 0x4B03
    int 0x21
    ret

; Load OVLR.BIN at EDI; ZF=1 iff refused with CF=1, AX=0009h.
ovl_expect9:
    mov [ebp+blk], edi
    mov dword [ebp+blk+4], 0
    call ovl_load
    jnc .loaded
    cmp ax, 9
    ret
.loaded:
    or ebp, ebp                 ; ZF=0 (ebp is never 0 here: the image base)
    ret

puts:
    mov ah, 0x09
    int 0x21
    ret

%macro FAIL 2
%1:
    lea edx, [ebp+%%m]
    jmp fail
%%m: db "OVL-FAIL ", %2, 0x0D, 0x0A, "$"
%endmacro
    FAIL f_mz_load,     "mz-load"
    FAIL f_mz_reloc,    "mz-reloc"
    FAIL f_mz_call,     "mz-call"
    FAIL f_factor_load, "factor-load"
    FAIL f_factor,      "factor"
    FAIL f_raw_load,    "raw-load"
    FAIL f_raw_size,    "raw-size"
    FAIL f_alloc,       "alloc"
    FAIL f_block_fit,   "block-fit"
    FAIL f_one_past,    "one-past"
    FAIL f_kernel,      "kernel"
    FAIL f_psp,         "psp"
    FAIL f_mcb,         "mcb-header"
    FAIL f_noblock,     "no-block"
    FAIL f_missing,     "missing"
    FAIL f_free,        "free"
fail:
    call puts
    mov eax, 0x4C01
    int 0x21

name_mz:        db "OVLT.OVL", 0
name_raw:       db "OVLR.BIN", 0
name_none:      db "NOPE.OVL", 0
name_foreign:   db "FOREIGN.OVL", 0
m_mz_ok:        db "OVL-MZ-OK", 0x0D, 0x0A, "$"
m_factor_ok:    db "OVL-FACTOR-OK", 0x0D, 0x0A, "$"
m_block_ok:     db "OVL-BLOCK-OK", 0x0D, 0x0A, "$"
m_refuse_ok:    db "OVL-REFUSE-OK", 0x0D, 0x0A, "$"
m_all_ok:       db "OVL-ALL-OK", 0x0D, 0x0A, "$"
m_foreign_after: db "OVL-FOREIGN-AFTER", 0x0D, 0x0A, "$"

align 4
blk:            dd 0, 0         ; exec_overlay_block_t
psp_lin:        dd 0
blk_seg:        dd 0

; ---- BSS: past the file image, inside our own image/BSS window ----
absolute $
alignb 16
buf1:           resb 256
buf2:           resb 256
buf3:           resb 64
