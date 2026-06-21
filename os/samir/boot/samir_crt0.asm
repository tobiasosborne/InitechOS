; samir_crt0.asm -- SAMIR (InitechBase) flat .COM entry stub (32-bit flat).
;
; beads: initech-ap5g (SAMIR Milton integration -- the freestanding link
;        toolchain). ADR-0009 DEC-01/DEC-05.
;
; Ref (Law 1):
;   spec/memory_map.h : PROGRAM_IMAGE = 0x00040100 (the .COM load + entry
;     address; the loader JMPs to the first byte of the image). PROGRAM_STACK_TOP
;     = 0x0007FFFC (the initial program ESP).
;   os/milton/loader.c : the control transfer (the inline asm near line 348)
;     loads EBX = PSP_ADDR and does `mov stack_top, %esp` (ESP =
;     PROGRAM_STACK_TOP) BEFORE the `jmp` to the program entry. So the loader
;     ALREADY sets ESP -- this stub does NOT depend on its own ESP setup. We set
;     ESP defensively anyway (see below) so a direct/standalone load path that
;     forgets the stack still works (fail-safe, Rule 2).
;   os/milton/kstart.asm : THE PRECEDENT -- a 32-bit nasm entry stub linked
;     first that sets ESP, calls a C entry, and guards with a hlt-loop. This
;     mirrors it at org 0x40100 and ADDS .bss zeroing (DEC-05).
;
; Contract (shared with the pal_milton agent): the C entry symbol is
;   void samir_milton_entry(void);
; It constructs pal_milton + the interpreter and runs samir_repl. This stub:
;   (a) zeroes .bss from __bss_start to __bss_end (DEC-05 -- a flat .COM does
;       not carry .bss in its file; we must materialize it rather than rely on
;       the emulator's RAM reset, a latent Rule-3 bug);
;   (b) sets ESP = PROGRAM_STACK_TOP defensively (the loader already did this;
;       belt-and-braces so a stack-forgetting load path cannot crash here);
;   (c) calls samir_milton_entry;
;   (d) on return, exits via INT 21h AH=4Ch with AL = the return value (the exit
;       code). NASM cannot read the C header, so 0x4C / 0x21 are hardcoded with
;       this citation (DOS terminate-with-return-code; the loader's INT 21h
;       AH=4Ch exit hook in os/milton/loader.c unwinds back to the kernel).
;
; Reproducible (Rule 11): no timestamps. ASCII-clean (Rule 12).

bits 32

global _start
extern samir_milton_entry      ; void samir_milton_entry(void) -- C, pal_milton agent
extern __bss_start             ; from samir.ld -- start of .bss (NOLOAD)
extern __bss_end               ; from samir.ld -- end of .bss

; PROGRAM_STACK_TOP from spec/memory_map.h (NASM cannot include the C header;
; this MUST move with that define -- same duplication contract the os/milton
; program .asm files honor for PROGRAM_IMAGE/org).
PROGRAM_STACK_TOP   equ 0x0007FFFC

; INT 21h AH=4Ch -- DOS "terminate with return code". Hardcoded per the loader's
; exit hook (os/milton/loader.c) which catches AH=4Ch and unwinds to the kernel.
DOS_INT             equ 0x21
DOS_FN_EXIT         equ 0x4C

section .text._start
_start:
    ; (a) Zero .bss [__bss_start, __bss_end) (DEC-05). The region is NOLOAD, so
    ; the .COM file carries no bytes for it; we clear it here. EDI = dst, ECX =
    ; dword count, EAX = 0. The linker aligns sections to 4 (default), so a
    ; dword stosd run is safe; a trailing byte tail is cleared too for safety.
    cld
    mov     edi, __bss_start
    mov     ecx, __bss_end
    sub     ecx, edi                ; ECX = byte length of .bss
    xor     eax, eax
    ; dword bulk
    mov     edx, ecx
    shr     ecx, 2                  ; ECX = dword count
    rep     stosd
    ; byte tail (length not a multiple of 4)
    mov     ecx, edx
    and     ecx, 3
    rep     stosb

    ; (b) ESP defensively (the loader already set it to PROGRAM_STACK_TOP before
    ; the JMP here -- os/milton/loader.c). Re-asserting it is harmless and makes
    ; the stub robust to a load path that transfers without a stack (Rule 2).
    mov     esp, PROGRAM_STACK_TOP
    xor     ebp, ebp                ; null frame pointer => top of stack to debugger
    push    ebp

    ; (c) Run the engine. samir_milton_entry builds pal_milton + interp + REPL.
    call    samir_milton_entry      ; void samir_milton_entry(void)

    ; (d) Terminate. AH=4Ch, AL = exit code. samir_milton_entry returns void, so
    ; treat a clean return as exit code 0. (If a future contract returns a code
    ; in EAX, AL already carries the low byte -- but the void contract means
    ; we force AL=0 for determinism, Rule 11.)
    xor     al, al                  ; AL = 0 (clean exit)
    mov     ah, DOS_FN_EXIT         ; AH = 0x4C  (spec: see DOS_FN_EXIT above)
    int     DOS_INT                 ; INT 0x21 -- terminate with return code

.hang:
    ; Not reached: INT 21h AH=4Ch does not return. Fail-loud guard (Rule 2) in
    ; case the exit hook is somehow unbound.
    hlt
    jmp     .hang

; Mark the stack non-executable (silences the ld GNU-stack warning). Harmless
; for a flat binary; good hygiene. ASCII-only (Rule 12).
section .note.GNU-stack noalloc noexec nowrite progbits
