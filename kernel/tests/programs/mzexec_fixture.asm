; mzexec_fixture.asm -- the InitechMZ (.EXE) RUNTIME-PROOF load module.
;
; beads: initech-0kiq (dtw.2-emu -- the in-emulator EXEC proof that complements
;        the host oracle test_mzload). This is the LOAD MODULE (the flat 32-bit
;        code) that mzlink wraps in an InitechMZ container; the container is
;        mcopy'd onto a FAT12 data disk as MZEXEC.EXE and loaded BY NAME through
;        the REAL kernel MZ path (load_program_from_fat -> mz_is_mz ->
;        load_program_mz_in_place -> loader_prepare_mz: parse + move-down +
;        flat-32 reloc + the one shared loader_run_plan transfer).
;
; Ref:   docs/adr/ADR-0003-AMENDMENT-DEC-08a-MZ-EXE-Flat-Loader.md
;          DEC-08a.1: the load module is flat 32-bit code; relocations are a
;            list of uint32 flat offsets; for each, the loader ADDS the flat load
;            base PROGRAM_IMAGE (0x40100) to the 32-bit dword at that offset.
;          DEC-08a.2: the load module lands at PROGRAM_IMAGE; entry = load base
;            (cs=ip=0); EBX = PROGRAM_BASE on entry, as for a flat .COM.
;        kernel/int21.c do_puts (AH=09h: EDX -> '$'-terminated string, '$' not
;          emitted) + do_terminate (AH=4Ch: AL = exit code); kernel/include/memory_map.h
;          (PROGRAM_IMAGE = 0x00040100). the repository contract (cite), the repository contract (the
;          marker is the oracle signal), the repository contract (fail loud / defense in depth),
;          the repository contract (deterministic: nasm -f bin is reproducible), the repository contract (ASCII).
;
; Assembled: nasm -f bin os/initechdos/mzexec_fixture.asm -o build/mzexec_fixture.bin
; Linked:    build/mzlink build/mzexec_fixture.bin build/mzexec.exe --reloc 1
;            (the InitechMZ container with the 0x4943 tag + the one reloc entry).
;
; THE RELOCATION-PROOF ADDRESSING (the whole point -- contrast greet_program.asm):
; greet_program.asm assembles at `org 0x00040100`, so `mov edx, msg` already
; encodes the FINAL runtime address and needs NO relocation. THIS fixture
; assembles at `org 0` instead, so `mov edx, msg` encodes msg's offset RELATIVE
; TO LOAD BASE 0 -- a LINK-TIME-relative absolute that is WRONG at the real load
; address (0x40100) UNLESS the loader relocates it. nasm encodes
; `mov edx, msg` as the opcode 0xBA at byte 0 followed by the imm32 at byte 1, so
; the imm32 dword at OFFSET 1 is the single relocation site. The loader's
; mz_apply_relocs adds PROGRAM_IMAGE to that dword:
;   WITH reloc:    EDX = 0x40100 + (msg offset)  == the real string -> prints the marker.
;   WITHOUT reloc: EDX = (msg offset, ~0x11)     == low kernel RAM  -> NO marker.
; So a green gate that sees "MZEXEC-OK" on serial PROVES the flat-32 relocation
; resolved AT RUNTIME -- not merely in a host unit test (the repository contract). The no-reloc
; container variant (mzlink --no-reloc) leaves the dword unrelocated and the
; marker is ABSENT, mutation-proving the gate bites (the repository contract).

bits 32
org 0                          ; LINK BASE 0 -- forces the reloc (see header above)

%ifdef MZEXEC_TSR_CHECK
; h4bk: the post-KEEP fixture has entry 7, reloc 8. Ignoring e_ip traps.
    times 7 db 0xCC
%endif

; Entry is byte 0 of the load module (== PROGRAM_IMAGE == the loader's JMP
; target, DEC-08a.2). The `mov edx, msg` is FIRST so its imm32 reloc site is at
; the fixed file offset 1 (opcode 0xBA at 0, imm32 at 1) -- mzlink relocates
; exactly that dword.
start:
    ; AH=09h DISPLAY STRING: EDX = flat ptr to the '$'-terminated marker string.
    ; `msg` is an org-0 absolute (the reloc site); the loader adds PROGRAM_IMAGE
    ; so EDX is the real runtime address ONLY if the relocation was applied.
    mov edx, msg               ; <-- opcode 0xBA @0, imm32 (reloc site) @1
%ifdef MZEXEC_TSR_CHECK
    ; Independently recover the actual address. A wrong-base mutant exits
    ; quietly instead of making AH=09h walk arbitrary resident/kernel bytes.
    call .base
.base:
    pop eax
    add eax, msg - .base
    cmp eax, edx
    jne .fail
    cmp ebx, 0x40000           ; relocated PSP above the KEEP'd image
    jbe .fail
    cmp esp, 0x70000           ; canonical top-level stack, separate from MCBs
    jb .fail
    cmp esp, 0x80000
    jae .fail
    push edx
    push ebx
    mov ah, 0x48
    mov ebx, 16               ; real AH=48h heap after relocated image+BSS
    int 0x21
    jc .fail
    movzx edi, ax
    shl edi, 4
    mov eax, [esp]
    add eax, 0x10100          ; PSP + 100h + independent 64 KiB BSS reserve
    cmp edi, eax
    jb .fail
    cmp edi, 0x6EF00          ; 256 bytes stay below the environment ceiling
    ja .fail
    mov dword [edi], 0x5A16C0DE
    cmp dword [edi], 0x5A16C0DE
    jne .fail
    ; Leave the allocation live: both successive executions must reclaim it.
    pop ebx
    pop edx
%endif
    mov ah, 0x09
    int 0x21

    ; AH=4Ch TERMINATE WITH RETURN CODE: AL = 9 (the known rc the gate asserts via
    ; the return-to-loader exit marker). Routes through int21 do_terminate -> the
    ; loader's exit hook -> load_program_mz_in_place / loader_run_plan.
    mov ah, 0x4C
    mov al, 9
    int 0x21

    ; Defense in depth (the repository contract): if 4Ch ever failed to terminate, INT 20h also
    ; routes to the loader's exit hook (exit code 0) rather than running off into
    ; the string bytes.
    int 0x20

%ifdef MZEXEC_TSR_CHECK
.fail:
    mov ax, 0x4C01
    int 0x21
%endif

; The marker AH=09h prints. The '$' (0x24) is the DOS terminator and is NOT
; emitted. "MZEXEC-OK" is UNIQUE on the serial wire (no other gate prints it), so
; its presence is the unambiguous runtime proof. Changing it makes the gate RED.
msg:
    db "MZEXEC-OK", 0x0D, 0x0A, "$"
