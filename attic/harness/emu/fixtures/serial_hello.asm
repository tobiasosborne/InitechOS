; serial_hello.asm -- harness self-test fixture: boot + serial proof.
;
; beads: initech-f2s ("QEMU harness ...").
; Ref:   Multiboot1 spec (header magic 0x1BADB002, flags, checksum);
;        PRD Sec 8 (-serial capture); CLAUDE.md Rule 12 (ASCII-only).
;
; A multiboot1 guest. QEMU's `-kernel` loader enters here in 32-bit
; protected mode, A20 on, flat segments -- no MBR/A20/GDT boot needed
; (that is the separate tracer-boot task, M1). We write the marker
; "HARNESS-OK\n" to COM1 (port 0x3F8) byte-by-byte, then hlt-loop.
;
; COM1 init: for QEMU you can write bytes straight to 0x3F8 with no full
; UART setup and the chardev (-serial file:...) captures them. That is
; fine for a fixture (per the task brief); a real driver would program
; the divisor/LCR/FCR first.

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

; -- Code -------------------------------------------------------------------
section .text
global _start

COM1 equ 0x3F8

_start:
    cli
    ; esi -> marker string; write each byte to COM1 until the NUL.
    mov esi, marker
.next:
    mov al, [esi]
    test al, al
    jz .done
    mov dx, COM1
    out dx, al
    inc esi
    jmp .next
.done:
    ; Clean exit so the GOOD path does not rely on the harness timeout
    ; (a timed-out run is not `ok`). The harness adds QEMU's isa-debug-exit
    ; device at iobase 0xF4; writing there makes QEMU exit with status
    ; ((value<<1)|1). We write 0x00 -> QEMU exit code 1, which the harness
    ; treats as the fixture's clean-stop convention (it asserts on serial +
    ; triple_fault, not on this exit code).
    mov dx, 0xF4
    mov al, 0x00
    out dx, al
    ; Belt and suspenders: if the device is absent, park forever and let
    ; the wall-clock timeout reap us (marker already captured).
.hang:
    hlt
    jmp .hang

section .rodata
marker: db "HARNESS-OK", 0x0A, 0x00
