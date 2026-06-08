; hang.asm -- harness self-test fixture: a guest that never finishes.
;
; beads: initech-f2s ("QEMU harness ...").
; Ref:   Multiboot1 spec; PRD Sec 8; the task brief's timeout-path test
;        ("a fixture that just hlt-loops without emitting the expected
;        marker -- it should report timed_out/marker-absent, not hang the
;        harness"); CLAUDE.md Rule 12 (ASCII-only).
;
; A multiboot1 guest that immediately hlt-loops, emitting NOTHING on serial
; and never requesting exit. It exists to prove the harness's wall-clock
; timeout reaps a wedged guest instead of hanging the harness itself
; (the critical "never let the harness hang" requirement). The expected
; result: timed_out=1, marker_found=0, triple_fault=0.

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
.hang:
    hlt
    jmp .hang
