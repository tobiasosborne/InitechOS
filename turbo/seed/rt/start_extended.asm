; Extended InitechMZ startup. Authority: kernel/include/initechmz_extended.h,
; kernel/include/memory_map.h and apps/initechbase/boot/ibase_crt0.asm.
; Link at zero; the loader relocates the module to 0x600000, clears its
; reserved tail and supplies a separate 64 KiB stack. Retain that ESP.
; Serial helpers match start_dos.asm; files and termination use INT 21h.

bits 32

global _start
extern pas_main
extern __bss_start             ; from seed_extended.ld -- start of .bss (NOLOAD)
extern __bss_end               ; from seed_extended.ld -- end of .bss

COM1 equ 0x3F8


; INT 21h AH=4Ch -- DOS terminate-with-return-code (the loader's exit hook
; unwinds to the kernel; kernel/loader.c).
DOS_INT             equ 0x21
DOS_FN_EXIT         equ 0x4C

section .text._start
_start:
    ; (a) Zero .bss [__bss_start, __bss_end) -- NOLOAD, so the .COM file
    ; carries no bytes for it.
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

    ; Run on the loader-supplied extended stack.
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
