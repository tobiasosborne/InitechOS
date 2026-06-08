; start.asm -- freestanding runtime for the seed cross-compiler's output.
;
; beads: initech-znb ("Step B of the InitechOS seed cross-compiler").
; Ref:   PRD Sec 6.7 (seed emits freestanding x86; serial is the oracle
;        signal, PRD Sec 8); Multiboot1 spec (header magic 0x1BADB002).
;        Adapted from harness/emu/fixtures/serial_hello.asm (the proven
;        multiboot1 + load-at-1MiB + COM1-0x3F8 pattern this environment
;        expects). CLAUDE.md Law 1 (cite) / Rule 2 (fail loud) / Rule 12
;        (ASCII-only) / Rule 11 (no timestamps/nondeterminism).
;
; QEMU's `-kernel` multiboot loader enters here in 32-bit protected mode,
; A20 on, flat segments -- no MBR/A20/GDT boot needed (that is M1).
;
; Layout: _start sets up a stack, calls the compiled program (pas_main),
; then performs a clean exit via the isa-debug-exit device (out 0xF4) like
; the good harness fixture, with an hlt-loop fallback. The serial helpers
; (serial_putc / serial_puts / serial_put_int) live here so codegen only
; emits the program body.

bits 32

; -- Multiboot1 header ------------------------------------------------------
MB_MAGIC    equ 0x1BADB002
MB_FLAGS    equ 0x00000000
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

section .multiboot_header
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

; -- Entry ------------------------------------------------------------------
section .text
global _start
extern pas_main

COM1 equ 0x3F8

_start:
    cli
    mov esp, stack_top          ; set up a real stack (multiboot esp is unsafe)
    call pas_main               ; run the compiled program body

    ; Clean exit via QEMU isa-debug-exit (iobase 0xF4): writing makes QEMU
    ; exit with status ((value<<1)|1). The harness asserts on serial +
    ; triple_fault + guest_errors, NOT on this exit code, so 0x00 is fine
    ; (matches serial_hello.asm). Belt-and-suspenders hlt loop if absent.
    mov dx, 0xF4
    mov al, 0x00
    out dx, al
.hang:
    hlt
    jmp .hang

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

; -- Stack ------------------------------------------------------------------
section .bss
align 16
stack_bottom:
    resb 16384                  ; 16 KiB runtime stack
stack_top:
