; triple_fault.asm -- harness self-test fixture: deliberate triple fault.
;
; beads: initech-f2s ("QEMU harness ...").
; Ref:   Multiboot1 spec; PRD Sec 8 (triple-fault detect, -no-reboot,
;        -d int,guest_errors,cpu_reset); CLAUDE.md hallucination callout
;        ("A triple-fault in QEMU silently reboots; turn on -d ... and
;        watch for it"); Rule 12 (ASCII-only).
;
; A multiboot1 guest that deliberately triple-faults so the detector has
; something to catch. We load a zero-limit IDT (no valid gates) then raise
; an interrupt with `int3`: the #BP has no handler -> #GP/#DF -> with an
; empty IDT the double-fault handler is also missing -> triple fault ->
; CPU reset. With the harness's -no-reboot, QEMU exits instead of looping,
; and the -d log carries the reset/triple-fault marker.
;
; It must NOT emit the HARNESS-OK marker, so the gate can assert the marker
; is ABSENT on the bad fixture.

bits 32

MB_MAGIC    equ 0x1BADB002
MB_FLAGS    equ 0x00000000
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

section .multiboot_header
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

section .text
global _start

_start:
    cli
    ; Load an IDT with limit 0 (no usable gates).
    lidt [zero_idt]
    ; Trigger an interrupt. With an empty IDT this cascades #BP -> #DF ->
    ; triple fault -> reset.
    int3
.hang:
    hlt
    jmp .hang

section .rodata
align 4
zero_idt:
    dw 0            ; limit = 0
    dd 0            ; base  = 0
