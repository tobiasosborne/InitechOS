; start_dos.asm -- InitechDOS-load runtime for the seed cross-compiler's output.
;
; beads: initech-ogxv (B8 -- the fileio.pas ON-InitechDOS oracle leg).
; Ref:   os/samir/boot/samir_crt0.asm + samir.ld (THE PRECEDENT: the flat .COM
;        entry stub -- first byte of the image IS the entry point at
;        PROGRAM_IMAGE 0x40100, loader JMPs with ESP = PROGRAM_STACK_TOP;
;        zero NOLOAD .bss at entry; exit via INT 21h AH=4Ch);
;        spec/memory_map.h (PROGRAM_IMAGE / PROGRAM_STACK_TOP);
;        seed/rt/start.asm (the FROZEN bare-metal runtime per ADR-0007 DEC-05
;        -- this file is a SEPARATE runtime TARGET, not an edit to it: the
;        serial helpers below are duplicated VERBATIM from start.asm so the
;        compiled program body links unchanged against either runtime).
;        CLAUDE.md Law 1 (cite) / Rule 2 (fail loud) / Rule 11 / Rule 12.
;
; Difference from start.asm (bare-metal Multiboot):
;   - NO multiboot header (COMMAND.COM EXECs this as a flat .COM; the loader
;     inspects no header -- the first byte is the entry).
;   - .bss is zeroed here (a flat .COM carries no .bss bytes and the loader
;     does not zero it -- the samir_crt0 DEC-05 rationale verbatim).
;   - Exit is INT 21h AH=4Ch (terminate, unwind to the kernel shell), never
;     isa-debug-exit/hlt: the desktop/shell must keep running after the
;     program ends (the samir-boot precedent).
;   - The stack: the loader already set ESP = PROGRAM_STACK_TOP; set it again
;     defensively (samir_crt0's belt-and-braces rule) instead of the private
;     16 KiB bare-metal stack.

bits 32

global _start
extern pas_main
extern __bss_start             ; from seed_dos.ld -- start of .bss (NOLOAD)
extern __bss_end               ; from seed_dos.ld -- end of .bss

COM1 equ 0x3F8

; PROGRAM_STACK_TOP from spec/memory_map.h (NASM cannot include the C header;
; this MUST move with that define -- the samir_crt0 duplication contract).
PROGRAM_STACK_TOP   equ 0x0007FFFC

; INT 21h AH=4Ch -- DOS terminate-with-return-code (the loader's exit hook
; unwinds to the kernel; os/milton/loader.c).
DOS_INT             equ 0x21
DOS_FN_EXIT         equ 0x4C

section .text._start
_start:
    ; (a) Zero .bss [__bss_start, __bss_end) -- NOLOAD, so the .COM file
    ; carries no bytes for it (samir_crt0 DEC-05 rationale).
    cld
    mov     edi, __bss_start
    mov     ecx, __bss_end
    sub     ecx, edi
    xor     eax, eax
    mov     edx, ecx
    shr     ecx, 2
    rep stosd
    mov     ecx, edx
    and     ecx, 3
    rep stosb

    ; (b) Defensive ESP (the loader already did this; fail-safe, Rule 2).
    mov     esp, PROGRAM_STACK_TOP

    ; (c) Run the compiled program body.
    call    pas_main

    ; (d) Terminate: INT 21h AH=4Ch, AL=0 (clean exit; the shell prompt
    ; returns). A flat .COM must NEVER fall through or hlt -- the kernel owns
    ; the machine after us.
    mov     eax, (DOS_FN_EXIT << 8)
    int     DOS_INT
.unreachable:
    hlt
    jmp     .unreachable

; ---------------------------------------------------------------------------
; Serial helpers -- duplicated VERBATIM from seed/rt/start.asm (the frozen
; bare-metal runtime) so codegen's emitted calls link identically against
; either runtime target. Any fix must land in BOTH files (grep marker:
; SEED-RT-SERIAL-HELPERS).
; ---------------------------------------------------------------------------

; -- serial_putc: write AL to COM1 -----------------------------------------
; In:  AL = byte. Clobbers: DX. Preserves nothing else of interest.
global serial_putc
serial_putc:
    mov dx, COM1
    out dx, al
    ret

; -- serial_puts: write a NUL-terminated string at EAX to COM1 -------------
; In:  EAX = pointer to NUL-terminated bytes. Clobbers: EAX, DX, ESI.
global serial_puts
serial_puts:
    push esi
    mov esi, eax
.next:
    mov al, [esi]
    test al, al
    jz .done
    mov dx, COM1
    out dx, al
    inc esi
    jmp .next
.done:
    pop esi
    ret

; -- serial_put_int: write signed 32-bit EAX as decimal to COM1 ------------
; In:  EAX = signed value. Clobbers: EAX, EBX, ECX, EDX, ESI.
; Method: handle sign, then divide by 10 building digits on the stack (least
; significant first), then emit them in reverse. INT_MIN (-2147483648) is
; handled correctly because we negate into an UNSIGNED magnitude path: we
; track the sign flag, then operate on the magnitude via unsigned division.
global serial_put_int
serial_put_int:
    push esi
    xor esi, esi                ; esi = sign flag (1 if negative)
    test eax, eax
    jns .positive
    mov esi, 1
    neg eax                     ; magnitude (for INT_MIN this wraps to the
                                ; correct unsigned 0x80000000 magnitude)
.positive:
    ; Build decimal digits onto the stack. ESP grows down; we count digits.
    mov ecx, 0                  ; digit count
    mov ebx, 10                 ; divisor
.div_loop:
    xor edx, edx                ; clear high dividend word
    div ebx                     ; unsigned: edx:eax / 10 -> eax, rem in edx
    add dl, '0'                 ; remainder -> ASCII digit
    push edx                    ; stash digit byte (in dl) on the stack
    inc ecx
    test eax, eax
    jnz .div_loop

    ; Emit sign if negative.
    test esi, esi
    jz .emit
    mov al, '-'
    mov dx, COM1
    out dx, al
.emit:
    ; Pop ecx digits, most significant first (they were pushed LS-first).
.emit_loop:
    pop eax                     ; digit byte in al (high bytes are remainder)
    mov dx, COM1
    out dx, al
    dec ecx
    jnz .emit_loop

    pop esi
    ret
