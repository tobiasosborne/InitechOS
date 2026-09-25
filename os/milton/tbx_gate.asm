; tbx_gate.asm -- the Initech Toolbox Gate (INT 81h) entry stub and the
; tenant call / abort trampolines (THE ARTIFACT; bead initech-tdnl.14).
;
; Ref:   spec/toolbox_gate.h Sec 1-2 (vector 0x81, trap gate 0x8F, DPL0, sel
;          0x08; args at interrupted ESP == frame + 68);
;        docs/design/GUI-remediation-ADR-reconciliation.md Part B DEC-AC3-2
;          (the gate), DEC-AC3-3 (push-callback delivery, yield-is-return; crash
;          routed to FlairProcess_kill, never re-entering tenant code), Part C
;          item 7 (crash triage: fail-loud halt preserved for NON-tenant faults);
;        os/milton/isr.asm int21_entry (the uniform int_frame_t this stub builds
;          byte-for-byte: dummy err 0, vector sentinel, gs/fs/es/ds, pushad).
;
; Linked ONLY into the FLAIRTENANTS kernels (every other kernel image is
; byte-identical). ASCII-clean (Rule 12); no nondeterminism (Rule 11).

bits 32
section .text

; --- INT 81h: the Toolbox Gate -------------------------------------------------
; A tenant pushes its cdecl args, loads AX, executes `int 0x81`. We build the
; SAME int_frame_t as int21_entry so the C dispatcher reads AX from frame+28 and
; arg i from frame + 68 + 4*i (the interrupted ESP; ring0 -> ring0 pushes no
; SS:ESP). The dispatcher writes the result into the saved EAX; popad returns it.
extern tbx_gate_dispatch       ; void tbx_gate_dispatch(uint8_t *frame)
global tbx_gate_entry
tbx_gate_entry:
    push dword 0               ; dummy error code (uniform frame)
    push dword 0x81            ; vector sentinel
    push gs
    push fs
    push es
    push ds
    pushad
    mov ax, 0x10               ; DATA_SEL -- known-good segments for the C call
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    cld                        ; the C ABI requires DF=0 on entry
    mov eax, esp
    push eax
    call tbx_gate_dispatch
    add esp, 4
    popad
    pop ds
    pop es
    pop fs
    pop gs
    add esp, 8
    iretd

; --- int tbx_tenant_call(uint32_t fn, uint32_t psp) ----------------------------
; Calls tenant code at flat address `fn` as a cdecl near function with EBX = the
; tenant PSP (the DEC-08a.2 entry register convention). The tenant runs on the
; kernel stack and RETURNS -- returning is the cooperative yield (ADR-0013 Sec
; 3.4). Returns 0 on a normal return, 1 when tbx_tenant_abort unwound a crash.
; Saves EBX/ESI/EDI/EBP + EFLAGS (so IF is restored on the abort path, which
; arrives from an interrupt gate with IF=0) and chains the previous saved ESP so
; a nested call (not used in V1, but never corrupting) unwinds correctly.
global tbx_tenant_call
global tbx_tenant_abort
tbx_tenant_call:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    pushfd
    push dword [tbx_call_esp]  ; chain the outer frame (0 when not nested)
    mov [tbx_call_esp], esp
    mov eax, [ebp + 8]         ; fn
    mov ebx, [ebp + 12]        ; EBX = PSP
    cld
    call eax
    xor eax, eax               ; normal return -> 0
.unwind:
    pop edx
    mov [tbx_call_esp], edx
    popfd
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

; --- void tbx_tenant_abort(void) -- noreturn ------------------------------------
; Called from the C fault triage (panic.c -> tbx fault hook) ON THE FAULT'S STACK,
; deeper than the tbx_tenant_call frame. Discards every frame between (the
; faulting tenant code and the exception frame -- never returned into, Rule 2)
; and resumes tbx_tenant_call's epilogue with EAX = 1. Segments are already the
; kernel's (isr_common reloaded them).
tbx_tenant_abort:
    mov esp, [tbx_call_esp]
    mov eax, 1
    jmp tbx_tenant_call.unwind

section .data
align 4
global tbx_call_esp
tbx_call_esp: dd 0             ; saved ESP of the innermost live tenant call

section .note.GNU-stack noalloc noexec nowrite progbits
