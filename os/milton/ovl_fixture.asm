; ovl_fixture.asm -- the InitechMZ OVERLAY load module for make test-exec-overlay.
;
; Ref: docs/adr/ADR-0003-AMENDMENT-DEC-19-INT21h-AH4Bh-AL03h-Load-Overlay.md
;        DEC-19.3: each InitechMZ relocation names a module-relative dword;
;        AH=4Bh AL=03h ADDS the caller's relocation factor to it.
;      IBM DOS 3.30 TR p. 6-196 (AL=03H: load, no PSP, no execution) -- the
;        CALLER transfers control into the loaded overlay itself.
;      ADR-0003 DEC-08a.1 (flat code, uint32 relocations); tools/mzlink.c
;        --reloc-diff (every absolute is DERIVED from a second assembly at
;        org OVL_ORG, so a missed site cannot hide). CLAUDE.md Rule 11/12.
;
; Fixed layout the test program (ovltest_program.asm) relies on:
;   +0  dd msg            absolute -> a relocation site (== factor + msg-$$)
;   +4  dd msg - $$       a label difference -> NOT relocated
;   +8  entry             near-CALLed by the program; prints the marker
;                         through a relocated absolute, then returns the
;                         dword at +0 read through another relocated absolute.
bits 32
%ifndef OVL_ORG
%define OVL_ORG 0
%endif
org OVL_ORG

ovl_ptr:    dd msg                  ; +0: reloc site
ovl_off:    dd msg - ovl_ptr        ; +4: constant (never relocated)
ovl_entry:                          ; +8
    mov edx, msg                    ; reloc site (imm32)
    mov ah, 0x09
    int 0x21
    mov eax, [ovl_ptr]              ; reloc site (moffs32)
    ret

msg:
    db "OVL-MZ-CALLED", 0x0D, 0x0A, "$"
