; kstart.asm -- InitechDOS flat C kernel entry stub (32-bit protected/flat).
;
; Boot contract: IO.SYS enters 08:00500000 with CR0.PE=1, A20 on,
; IF=0, flat DS/ES/FS/GS/SS=10. This entry is first in the linked image.
; The first 16 bytes are EB 0E, K386, loaded byte count, absolute BSS end,
; and a reserved zero word. Stage2 checks these BEFORE copying the image.
; boot_info at 0x500 appends loaded_bytes (+36) to the existing 9 dwords.
; Clear BSS, alignment gap and unused runway from linker symbols, before
; using the stack [0x5f0000,0x600000); never clear loaded .rodata.

bits 32

; Duplicated from spec/memory_map.h because NASM cannot include C headers.
; Ref: kernel-runway-relocation.md K1/K2; bead initech-tdnl.29.
KERNEL_BASE       equ 0x00500000
KERNEL_RUNWAY     equ 0x00100000
KERNEL_CEIL       equ 0x00600000
KERNEL_STACK_BOT  equ 0x005F0000
KERNEL_STACK_TOP  equ 0x005FFFFC

; The stack equates are a duplicate of spec/memory_map.h, so they carry the
; drift guard: the Makefile parses the header's
; KERNEL_STACK_BOT/TOP and passes them as MK_KERNEL_STACK_BOT/TOP; any
; disagreement is an assembly error, never a silently-misplaced ESP
; (bead initech-xu97; CLAUDE.md Rule 2).
%ifndef MK_KERNEL_STACK_TOP
  %error "MK_KERNEL_STACK_TOP not defined: kstart.asm must be assembled by the Makefile rule so its stack equates can be checked against spec/memory_map.h."
%elif MK_KERNEL_STACK_TOP != KERNEL_STACK_TOP
  %error "KERNEL_STACK_TOP MISMATCH: spec/memory_map.h (MK_KERNEL_STACK_TOP) differs from this kstart.asm equate -- move BOTH."
%endif
%ifndef MK_KERNEL_STACK_BOT
  %error "MK_KERNEL_STACK_BOT not defined: kstart.asm must be assembled by the Makefile rule so its stack equates can be checked against spec/memory_map.h."
%elif MK_KERNEL_STACK_BOT != KERNEL_STACK_BOT
  %error "KERNEL_STACK_BOT MISMATCH: spec/memory_map.h (MK_KERNEL_STACK_BOT) differs from this kstart.asm equate -- move BOTH."
%endif

global _start
extern kernel_main
extern _kernel_image_end, _bss_start, _bss_end, _kernel_end
section .text._start

_start:
    jmp short .entry
    db 'K386'
    dd _kernel_image_end - KERNEL_BASE
    dd _bss_end
    dw 0
.entry:
    ; Kernel-private 64 KiB stack [0x5F0000,0x600000): the top of the RAM-gated
    ; kernel runway, above the transient COMMAND.COM window and disjoint from
    ; the FLAIR heap, the kernel image, the program window and the staging
    ; window (spec/memory_map.h WINDOW DISJOINTNESS asserts; bead initech-xu97).
    ; stage2's transient ESP=0x90000 stack is abandoned at this handoff. The
    ; runway zeroing below covers this stack too; it runs BEFORE the first push,
    ; so no live stack slot is ever cleared.
    mov esp, KERNEL_STACK_TOP
    xor ebp, ebp                ; null frame pointer => top of stack to debugger

    ; Linker boundaries, never a padded disk size: clear BSS explicitly,
    ; then the unused runway through 0x600000 before the first stack push.
    ; _kernel_image_end follows ALL loaded sections, including .rodata.
    cld
    xor eax, eax
    mov edi, _bss_start
    mov ecx, _bss_end
    sub ecx, edi
    rep stosb
    mov edi, _kernel_image_end
    mov ecx, _bss_start
    sub ecx, edi
    rep stosb
    mov edi, _kernel_end
    mov ecx, KERNEL_CEIL
    sub ecx, edi
    rep stosb
%ifdef MK_BIG_KERNEL
    call bigkernel_check
%endif

    push ebp
    call kernel_main            ; void kernel_main(void) -- never returns

.hang:
    hlt
    jmp .hang

%ifdef MK_BIG_KERNEL
; Opt-in reproducible load proof: 128KiB with every byte value repeated 512
; times. Read every byte from high .rodata after BSS/runway clearing. A failure
; halts before kernel_main; normal images contain neither blob nor checker.
bigkernel_check:
    mov esi, bigkernel_blob
    mov ecx, 131072
    xor ebx, ebx
.sum:
    lodsb
    movzx eax, al
    add ebx, eax
    loop .sum
    cmp ebx, 0x00ff0000
    jne .bad
    mov esi, bigkernel_ok
    call .puts
    mov eax, [0x500+36]
    mov edi, 10
    xor ecx, ecx
.decimal:
    xor edx, edx
    div edi
    push edx
    inc ecx
    test eax, eax
    jnz .decimal
.digit:
    pop eax
    add al, '0'
    call .putc
    loop .digit
    mov esi, bigkernel_sum
    call .puts
    mov ecx, 8
.hex:
    rol ebx, 4
    mov eax, ebx
    and al, 15
    add al, '0'
    cmp al, '9'
    jbe .emit
    add al, 7
.emit:
    call .putc
    loop .hex
    mov al, 10
    call .putc
    ret
.bad:
    mov esi, bigkernel_bad
    call .puts
    cli
.halt:
    hlt
    jmp .halt
.puts:
    lodsb
    test al, al
    jz .done
    call .putc
    jmp .puts
.done:
    ret
.putc:
    push eax
    push edx
    mov ah, al
.wait:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz .wait
    mov al, ah
    mov dx, 0x3f8
    out dx, al
    pop edx
    pop eax
    ret
section .rodata
bigkernel_ok: db 'BIGKERNEL-OK size=',0
bigkernel_sum: db ' sum=',0
bigkernel_bad: db 'BIGKERNEL-BAD',10,0
bigkernel_blob:
%rep 512
%assign byteval 0
%rep 256
    db byteval
%assign byteval byteval+1
%endrep
%endrep
%endif

; Mark the stack non-executable (silences the ld GNU-stack warning). Harmless
; for a flat binary; good hygiene. ASCII-only (Rule 12).
section .note.GNU-stack noalloc noexec nowrite progbits
