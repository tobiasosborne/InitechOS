; isr.asm -- CPU exception entry stubs + common dispatch trampoline.
;
; beads: initech-a5a ("interrupt foundation").
; Ref:   docs/research/internals-int21h-ground-truth.md Sec 3.1 (which vectors
;        push an error code: 8,10,11,12,13,14,17,21 -- all others push none),
;        Sec 3.2 (uniform frame: dummy-0 error code for the no-error vectors,
;        then the vector number, then segs + pushad), Sec 8 Risk 2 (the frame
;        order is load-bearing and MUST match int_frame_t in idt.h). Intel SDM
;        Vol 2A PUSHAD (push order EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI -> EDI lands
;        lowest), Vol 3A Sec 6.12.1 (CPU-pushed EIP/CS/EFLAGS; no SS:ESP in a
;        ring0->ring0 delivery). CLAUDE.md Rule 2 (fail loud), Rule 12 (ASCII).
;
; ARTIFACT code: nasm elf32, linked into the flat kernel. The stubs build the
; EXACT stack image int_frame_t describes, then call the C dispatcher with a
; pointer to it. For an exception the C side halts (never returns); the iret
; tail exists so the SAME common stub can serve a clean-returning vector (the
; live IDT self-test on vector 0x80 returns and resumes -- kmain.c).

bits 32

extern isr_dispatch_c          ; void isr_dispatch_c(int_frame_t *)

global isr_spurious

; --- per-vector stubs ------------------------------------------------------
; A vector that does NOT push a CPU error code gets a dummy 0 so every frame is
; uniform; a vector that DOES push one omits the dummy. Then push the vector
; number and fall into the common trampoline.
;
; Error-code vectors (Intel SDM Vol 3A Table 6-1): 8,10,11,12,13,14,17,21.

%macro ISR_NOERR 1
global isr%1
isr%1:
    push dword 0               ; dummy error code (uniform frame)
    push dword %1              ; vector number
    jmp isr_common
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    ; CPU already pushed the real error code; do NOT push a dummy.
    push dword %1              ; vector number
    jmp isr_common
%endmacro

ISR_NOERR 0      ; #DE  Divide error
ISR_NOERR 1      ; #DB  Debug
ISR_NOERR 2      ; NMI
ISR_NOERR 3      ; #BP  Breakpoint
ISR_NOERR 4      ; #OF  Overflow
ISR_NOERR 5      ; #BR  BOUND range
ISR_NOERR 6      ; #UD  Invalid opcode
ISR_NOERR 7      ; #NM  Device not available
ISR_ERR   8      ; #DF  Double fault (err always 0)
ISR_NOERR 9      ; Coprocessor segment overrun (386 reserved)
ISR_ERR   10     ; #TS  Invalid TSS
ISR_ERR   11     ; #NP  Segment not present
ISR_ERR   12     ; #SS  Stack-segment fault
ISR_ERR   13     ; #GP  General protection
ISR_ERR   14     ; #PF  Page fault
ISR_NOERR 15     ; reserved
ISR_NOERR 16     ; #MF  x87 FPU error
ISR_ERR   17     ; #AC  Alignment check
ISR_NOERR 18     ; #MC  Machine check
ISR_NOERR 19     ; #XM  SIMD FP
ISR_NOERR 20     ; #VE  Virtualization
ISR_ERR   21     ; #CP  Control protection
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

; Spurious / unhandled-vector stub: no CPU error code, vector 0xFF marker.
isr_spurious:
    push dword 0               ; dummy error code
    push dword 0xFF            ; sentinel "vector" so the dump shows 255
    jmp isr_common

; 8259A spurious-IRQ stubs (bcg.7). Unlike isr_spurious these push their REAL
; vector so isr_dispatch_c can apply the correct EOI discipline: a MASTER
; spurious (IRQ7 -> 0x2F) gets NO EOI; a SLAVE spurious (IRQ15 -> 0x37) gets a
; master-only EOI (the master latched the cascade). Both then resume via the
; isr_common iret tail. Ref: Intel 8259A datasheet (spurious interrupt).
global isr_irq7_spurious
isr_irq7_spurious:
    push dword 0               ; dummy error code
    push dword 0x2F            ; master spurious vector (PIC_MASTER_BASE + 7)
    jmp isr_common

global isr_irq15_spurious
isr_irq15_spurious:
    push dword 0               ; dummy error code
    push dword 0x37            ; slave spurious vector (PIC_SLAVE_BASE + 7)
    jmp isr_common

; --- common trampoline -----------------------------------------------------
; On entry the stack (high -> low address) holds, from the CPU + the per-vector
; stub: EFLAGS, CS, EIP, err_code, vector. We push the segment selectors then
; PUSHAD; the resulting image, read low-address-first, is EXACTLY int_frame_t:
;   edi,esi,ebp,esp_dummy,ebx,edx,ecx,eax  (pushad, EDI lowest)
;   ds,es,fs,gs                            (pushed gs,fs,es,ds so ds is just
;                                           above eax at +32)
;   vector, err_code, eip, cs, eflags
; ESP after pushad therefore points at int_frame_t field 0 (edi) -- hand it to C.
isr_common:
    push gs                    ; -> int_frame_t.gs (+44)
    push fs                    ; -> .fs (+40)
    push es                    ; -> .es (+36)
    push ds                    ; -> .ds (+32)
    pushad                     ; -> eax(+28)..edi(+0)

    ; Reload the kernel data selector into the segment registers so the C
    ; dispatcher runs with known-good segments even if a fault left them odd.
    ; DATA_SEL = 0x10 (stage2 GDT gdt_data - gdt_start).
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp               ; frame pointer = &int_frame_t (top of stack)
    push eax                   ; arg 1 (cdecl): int_frame_t *frame
    call isr_dispatch_c
    add esp, 4                 ; pop the arg

    ; Teardown (only reached for a clean-returning vector -- e.g. the live IDT
    ; self-test on 0x80; a CPU exception halts in C and never returns here).
    ; Reverse of the setup, in strict LIFO order:
    popad                      ; restore eax..edi (pushad image)
    pop ds                     ; reverse of push gs/fs/es/ds
    pop es
    pop fs
    pop gs
    add esp, 8                 ; discard the pushed vector + err_code dwords
    iretd                      ; pop EIP, CS, EFLAGS -> resume the interruptee

; --- INT 21h syscall trap stub (vector 0x21) -------------------------------
; beads: initech-509.5 (the `int 0x21` dispatcher spine). Gate: initech-1f9.
; Ref: docs/research/internals-int21h-ground-truth.md Sec 5.4 (the asm stub
;      builds the SAME uniform frame as the exception path so the C dispatcher
;      reads the caller's registers + writes CF into the saved EFLAGS image),
;      Sec 8 Risk 2 (frame order is load-bearing -- it MUST match int_frame_t).
;
; A trap gate at vector 0x21 is reached by a SOFTWARE `int 0x21`, so the CPU
; pushes NO error code. To present the EXACT int_frame_t the exception path uses
; (so int21_dispatch can reuse it + write frame->eflags), we push a dummy 0
; err_code then the vector sentinel 0x21, then segs + pushad -- byte-identical
; in shape to isr_common. The C dispatcher sets/clears CF in the saved EFLAGS;
; we do NOT popad EFLAGS, so the modified EFLAGS rides back out via iretd and the
; caller sees CF (the DOS `int 0x21 / jc error` idiom).
extern int21_dispatch          ; void int21_dispatch(int_frame_t *)
global int21_entry
int21_entry:
    push dword 0               ; dummy error code (uniform frame; no CPU err)
    push dword 0x21            ; vector sentinel (so a dump shows AH path = 0x21)
    push gs                    ; -> int_frame_t.gs (+44)
    push fs                    ; -> .fs (+40)
    push es                    ; -> .es (+36)
    push ds                    ; -> .ds (+32)
    pushad                     ; -> eax(+28)..edi(+0)

    ; Known-good kernel data selector for the C call (DATA_SEL = 0x10).
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp               ; frame pointer = &int_frame_t (top of stack)
    push eax                   ; arg 1 (cdecl): int_frame_t *frame
    call int21_dispatch        ; reads AH, writes return value + CF in frame
    add esp, 4                 ; pop the arg

    ; Teardown: reverse of setup. popad restores the dispatcher's return value
    ; (it wrote frame->eax) into EAX along with the other GPRs; popad does NOT
    ; restore EFLAGS, so the CF the dispatcher wrote into the saved EFLAGS image
    ; survives to the iretd below (which pops the modified EFLAGS).
    popad                      ; restore eax..edi
    pop ds
    pop es
    pop fs
    pop gs
    add esp, 8                 ; discard the pushed vector + err_code dwords
    iretd                      ; pop EIP, CS, EFLAGS -> resume caller (CF set)

; --- INT 20h legacy-terminate trap stub (vector 0x20) ----------------------
; beads: initech-509.5 (the loader keystone). Ref: docs/research/psp-loader-
;        ground-truth.md Sec 2.1 (the PSP int20[2] = CD 20) + Sec 4.4 (INT 20h
;        routes to the SAME terminate path as 4Ch, exit code 0). DEC-04a: vector
;        0x20 is FREE because the PIC master is remapped to 0x28 (not 0x20), so a
;        software `int 0x20` lands here and not on IRQ0.
;
; A program terminates the legacy way by `int 0x20` (or a near RET to PSP:0 which
; executes the CD 20 there). We build the SAME uniform int_frame_t the int21 stub
; does (so int20_dispatch can reuse it) then call int20_dispatch, which forces a
; terminate with code 0 through the bound exit hook. Like 4Ch, this normally does
; NOT return (the loader's hook is non-returning); the teardown + iretd tail
; exists only for the no-program / no-hook case (terminate just returns).
extern int20_dispatch          ; void int20_dispatch(int_frame_t *)
global int20_entry
int20_entry:
    push dword 0               ; dummy error code (uniform frame; no CPU err)
    push dword 0x20            ; vector sentinel (a dump shows the 0x20 path)
    push gs                    ; -> int_frame_t.gs (+44)
    push fs                    ; -> .fs (+40)
    push es                    ; -> .es (+36)
    push ds                    ; -> .ds (+32)
    pushad                     ; -> eax(+28)..edi(+0)

    mov ax, 0x10               ; known-good kernel data selector (DATA_SEL)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp               ; frame pointer = &int_frame_t (top of stack)
    push eax                   ; arg 1 (cdecl): int_frame_t *frame
    call int20_dispatch        ; forces terminate(0); normally does not return
    add esp, 4                 ; pop the arg (only reached if no hook is bound)

    popad                      ; restore eax..edi
    pop ds
    pop es
    pop fs
    pop gs
    add esp, 8                 ; discard the pushed vector + err_code dwords
    iretd                      ; resume caller (no-hook host-style fallback)

; --- INT 22h/23h/24h DOS handler trap stubs (vectors 0x22/0x23/0x24) --------
; beads: initech-509.8 (termination 22h, control-break 23h, critical-error 24h).
; Ref: docs/adr/ADR-0003-InitechDOS-Base-OS-Personality.md Sec 5.10 (DEC-10 --
;      handlers for 24h/23h/22h; 24h presents MSG-DOS-0001 + processes the
;      operator A/R/F response). DEC-04a: the PIC master is remapped to 0x28
;      (not 0x20), so software `int 0x22/0x23/0x24` land on these free vectors
;      and not on IRQs -- the SAME reasoning that frees 0x20/0x21.
;
; Each stub is byte-identical in shape to int21_entry / int20_entry: dummy 0
; err_code + the vector sentinel, then segs + pushad, so the C dispatcher reads
; the SAME uniform int_frame_t and writes the return value + CF into the saved
; image. 22h/23h normally TERMINATE (the dispatcher routes to the bound exit
; hook, non-returning); 24h RETURNS an A/R/F action in AL and irets -- the
; teardown + iretd tail serves exactly that case (and the no-hook fallback for
; 22h/23h). All three pushad-build the frame so the loader's PSP vector save
; (the SEPARATE next step, loader.c) can read the installed IDT gates.

extern int22_dispatch          ; void int22_dispatch(int_frame_t *)
global int22_entry
int22_entry:
    push dword 0               ; dummy error code (uniform frame; no CPU err)
    push dword 0x22            ; vector sentinel (a dump shows the 0x22 path)
    push gs                    ; -> int_frame_t.gs (+44)
    push fs                    ; -> .fs (+40)
    push es                    ; -> .es (+36)
    push ds                    ; -> .ds (+32)
    pushad                     ; -> eax(+28)..edi(+0)

    mov ax, 0x10               ; known-good kernel data selector (DATA_SEL)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp               ; frame pointer = &int_frame_t (top of stack)
    push eax                   ; arg 1 (cdecl): int_frame_t *frame
    call int22_dispatch        ; terminate(0) via the exit hook; normally no return
    add esp, 4                 ; pop the arg (only reached if no hook is bound)

    popad                      ; restore eax..edi
    pop ds
    pop es
    pop fs
    pop gs
    add esp, 8                 ; discard the pushed vector + err_code dwords
    iretd                      ; resume caller (no-hook host-style fallback)

extern int23_dispatch          ; void int23_dispatch(int_frame_t *)
global int23_entry
int23_entry:
    push dword 0               ; dummy error code (uniform frame; no CPU err)
    push dword 0x23            ; vector sentinel (a dump shows the 0x23 path)
    push gs                    ; -> int_frame_t.gs (+44)
    push fs                    ; -> .fs (+40)
    push es                    ; -> .es (+36)
    push ds                    ; -> .ds (+32)
    pushad                     ; -> eax(+28)..edi(+0)

    mov ax, 0x10               ; known-good kernel data selector (DATA_SEL)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp               ; frame pointer = &int_frame_t (top of stack)
    push eax                   ; arg 1 (cdecl): int_frame_t *frame
    call int23_dispatch        ; control-break default ABORTS (terminate); no return
    add esp, 4                 ; pop the arg (only reached if no hook is bound)

    popad                      ; restore eax..edi
    pop ds
    pop es
    pop fs
    pop gs
    add esp, 8                 ; discard the pushed vector + err_code dwords
    iretd                      ; resume caller (no-hook host-style fallback)

extern int24_dispatch          ; void int24_dispatch(int_frame_t *)
global int24_entry
int24_entry:
    push dword 0               ; dummy error code (uniform frame; no CPU err)
    push dword 0x24            ; vector sentinel (a dump shows the 0x24 path)
    push gs                    ; -> int_frame_t.gs (+44)
    push fs                    ; -> .fs (+40)
    push es                    ; -> .es (+36)
    push ds                    ; -> .ds (+32)
    pushad                     ; -> eax(+28)..edi(+0)

    mov ax, 0x10               ; known-good kernel data selector (DATA_SEL)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp               ; frame pointer = &int_frame_t (top of stack)
    push eax                   ; arg 1 (cdecl): int_frame_t *frame
    call int24_dispatch        ; presents MSG-DOS-0001, returns the A/R/F action in AL
    add esp, 4                 ; pop the arg

    ; 24h RETURNS the operator's action to the caller (DOS critical-error
    ; contract): the dispatcher wrote AL into the saved frame->eax and cleared
    ; CF, so popad restores AL and the iretd resumes the (would-be) caller.
    popad                      ; restore eax..edi (incl. the A/R/F action in AL)
    pop ds
    pop es
    pop fs
    pop gs
    add esp, 8                 ; discard the pushed vector + err_code dwords
    iretd                      ; resume caller with AL = action, CF clear

; --- INT 25h / 26h: ABSOLUTE DISK READ/WRITE (ADR-0003 DEC-15) --------------
; beads initech-4mq7. Two NEW separate software-interrupt vectors -- NOT INT 21h
; AH functions (AH=25h is SETVECT, AH=35h is GETVECT, handled in int21.c). Like
; int24h these vectors compute a result and RETURN it to the caller with CF, so
; each stub is BYTE-IDENTICAL IN SHAPE to int24_entry (the RETURN template, not
; the terminating 20/22/23 template): dummy 0 err_code + the vector sentinel,
; segs + pushad, reload DATA_SEL for the C call, call the dispatcher (which
; writes AX + CF into the saved frame), then teardown + IRETD. Vectors 0x25/0x26
; are free (PIC remapped to 0x28/0x30 per DEC-04a, so software `int 0x25/0x26`
; land here and not on IRQs -- the SAME reasoning that freed 0x20-0x24).
;
; FOOT-GUN (DEC-15.3): real DOS 3.x INT 25h/26h return with the original CPU
; FLAGS still PUSHED on the caller's stack (the CP/M-derived wart -- period
; callers follow with `add sp,2`/popf). InitechOS DELIBERATELY DOES NOT
; reproduce that wart: status is via CF in the saved EFLAGS, and the stub
; returns via the SAME uniform IRETD with NO stack imbalance and NO add sp,2/popf
; contract on callers. The omitted wart is intentional -- never "fix" the stack
; to reproduce it (the oracle's M7 mutant proves a stray-word stub goes RED).
extern int25_dispatch          ; void int25_dispatch(int_frame_t *)
global int25_entry
int25_entry:
    push dword 0               ; dummy error code (uniform frame; no CPU err)
    push dword 0x25            ; vector sentinel (a dump shows the 0x25 path)
    push gs                    ; -> int_frame_t.gs (+44)
    push fs                    ; -> .fs (+40)
    push es                    ; -> .es (+36)
    push ds                    ; -> .ds (+32)
    pushad                     ; -> eax(+28)..edi(+0)

    mov ax, 0x10               ; known-good kernel data selector (DATA_SEL)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp               ; frame pointer = &int_frame_t (top of stack)
    push eax                   ; arg 1 (cdecl): int_frame_t *frame
    call int25_dispatch        ; reads AL/CX/DX/EBX, writes AX + CF into the frame
    add esp, 4                 ; pop the arg

    ; 25h RETURNS the result + CF to the caller (DOS absolute-read contract):
    ; the dispatcher set CF and (on error) AX in the saved frame, so popad
    ; restores AX and the IRETD resumes the caller with a BALANCED stack (the
    ; leftover-FLAGS wart is deliberately omitted -- DEC-15.3, the foot-gun above).
    popad                      ; restore eax..edi (incl. AX)
    pop ds
    pop es
    pop fs
    pop gs
    add esp, 8                 ; discard the pushed vector + err_code dwords
    iretd                      ; resume caller with CF = status (no stack imbalance)

extern int26_dispatch          ; void int26_dispatch(int_frame_t *)
global int26_entry
int26_entry:
    push dword 0               ; dummy error code (uniform frame; no CPU err)
    push dword 0x26            ; vector sentinel (a dump shows the 0x26 path)
    push gs                    ; -> int_frame_t.gs (+44)
    push fs                    ; -> .fs (+40)
    push es                    ; -> .es (+36)
    push ds                    ; -> .ds (+32)
    pushad                     ; -> eax(+28)..edi(+0)

    mov ax, 0x10               ; known-good kernel data selector (DATA_SEL)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp               ; frame pointer = &int_frame_t (top of stack)
    push eax                   ; arg 1 (cdecl): int_frame_t *frame
    call int26_dispatch        ; reads AL/CX/DX/EBX, writes AX + CF into the frame
    add esp, 4                 ; pop the arg

    ; 26h RETURNS the result + CF to the caller (DOS absolute-write contract);
    ; SAME uniform IRETD + balanced stack as 25h. The leftover-FLAGS wart is
    ; deliberately omitted (DEC-15.3, the foot-gun above) -- never reproduce it.
    popad                      ; restore eax..edi (incl. AX)
    pop ds
    pop es
    pop fs
    pop gs
    add esp, 8                 ; discard the pushed vector + err_code dwords
    iretd                      ; resume caller with CF = status (no stack imbalance)

; --- hardware IRQ stubs: PIT IRQ0 (vector 0x28) + keyboard IRQ1 (0x29) ------
; beads: initech-3rs ("PS/2 keyboard (IRQ1) + PIT (IRQ0) tick").
; Ref: Intel SDM Vol 2A PUSHAD/IRET; Intel 8259A datasheet (the C handler issues
;      the EOI, OCW2 0x20 -> master cmd port 0x20); Intel 8254 / 8042 datasheets
;      (the C handlers read/program the device). CLAUDE.md Rule 2, Rule 12.
;
; These are installed as 0x8E INTERRUPT gates (idt_install_irq), so the CPU
; CLEARS IF on entry -> no IRQ nesting inside the handler (the cooperative
; model wants exactly one IRQ in flight at a time). Unlike the exception path
; these handlers take NO frame argument and ALWAYS iret cleanly, so the stub is
; the minimal "save volatiles + known-good DS/ES + call C + restore + iretd".
; We pushad (not just the C-clobbered set) so the interrupted context is fully
; preserved regardless of what the C handler touches; segs are reloaded to the
; kernel data selector (DATA_SEL=0x10) so the C call runs with known segments.
;
; REENTRANCY (beads initech-xk2): the C handlers (pit_irq_handler /
; kbd_irq_handler) touch ONLY their own tick/ring state -- never int21 globals
; -- so an IRQ landing inside an INT 21h trap (which keeps IF set) is safe.
; This is now MECHANICALLY ENFORCED: each stub brackets its C handler call with
; irq_enter/irq_leave (irq.c) so g_irq_depth is non-zero for the whole time the
; handler is on the stack. If a handler (or a future driver it calls) ever issues
; `int 0x21`, the dispatcher sees irq_depth() != 0 at entry and FAILS LOUD
; (dos_reentry_panic) instead of silently corrupting the interrupted syscall's
; frame/globals -- Rule 2. The IRQ gates clear IF (0x8E), so irq_enter/irq_leave
; are not themselves reentrant. Order: enter BEFORE the C call, leave AFTER the
; segment restore but BEFORE iretd, so the depth is correct across the whole
; handler body.

extern pit_irq_handler         ; void pit_irq_handler(void)  -- increments ticks + EOI
extern kbd_irq_handler         ; void kbd_irq_handler(void)  -- reads 0x60 + EOI
extern irq_enter               ; void irq_enter(void)  -- g_irq_depth++ (irq.c)
extern irq_leave               ; void irq_leave(void)  -- g_irq_depth-- (irq.c)
; PS/2 mouse IRQ12 C body (os/milton/mouse.c). WEAK external reference (ADR-0006
; FO-6): isr.o is SHARED by every kernel, but mouse.o is linked ONLY into the
; flair_live kernel(s). A weak extern lets the non-flair kernels link cleanly
; (the undefined reference resolves to 0) -- harmless because irq12_entry is
; INERT in those kernels (no IDT 0x34 gate installed, IRQ12 stays masked, so the
; stub is never reached). When mouse.o IS linked, the strong definition wins.
extern mouse_irq_handler:weak  ; void mouse_irq_handler(void) -- reads 0x60 + dual-PIC EOI

global irq0_entry
irq0_entry:
    pushad                     ; save all GPRs (interrupted context)
    push ds
    push es
    mov ax, 0x10               ; DATA_SEL -- known-good segments for the C call
    mov ds, ax
    mov es, ax
    call irq_enter             ; g_irq_depth++ (xk2 reentrancy guard arm)
    call pit_irq_handler       ; bumps g_ticks, sends EOI to the 8259A master
    call irq_leave             ; g_irq_depth-- (disarm before iretd)
    pop es
    pop ds
    popad
    iretd                      ; IF restored from the saved EFLAGS (was set)

global irq1_entry
irq1_entry:
    pushad
    push ds
    push es
    mov ax, 0x10               ; DATA_SEL
    mov ds, ax
    mov es, ax
    call irq_enter             ; g_irq_depth++ (xk2 reentrancy guard arm)
    call kbd_irq_handler       ; reads scancode @0x60, enqueues ASCII, sends EOI
    call irq_leave             ; g_irq_depth-- (disarm before iretd)
    pop es
    pop ds
    popad
    iretd

; --- PS/2 mouse IRQ12 stub (vector 0x34) -- ADR-0006 E-D3(b)/FO-6 ----------
; beads: initech-5l5z FO-6 (THE minefield: dual-8259A cascade EOI). IRQ12 = the
; SLAVE 8259A IR4 -> vector PIC_SLAVE_BASE(0x30) + 4 = 0x34. This stub is a
; BYTE-FOR-BYTE clone of irq1_entry (same pushad / push ds,es / DATA_SEL load /
; irq_enter / C-call / irq_leave / restore / iretd bracket): the C handler
; (mouse_irq_handler) reads the packet byte from 0x60, assembles the 3-byte
; PS/2 packet, posts ONE FLAIR_RAW_MOUSE on a complete packet, and issues its
; OWN dual-PIC EOI (SLAVE 0xA0 then MASTER 0x20). isr.o is shared, but only the
; flair_live kernel installs the 0x34 IDT gate + unmasks IRQ12, so the stub is
; inert (never reached) in every other kernel (mouse_irq_handler resolves weakly
; to 0 there -- see the weak extern above).
global irq12_entry
irq12_entry:
    pushad
    push ds
    push es
    mov ax, 0x10               ; DATA_SEL
    mov ds, ax
    mov es, ax
    call irq_enter             ; g_irq_depth++ (xk2 reentrancy guard arm)
    call mouse_irq_handler     ; reads packet @0x60, posts raw, dual-PIC EOI
    call irq_leave             ; g_irq_depth-- (disarm before iretd)
    pop es
    pop ds
    popad
    iretd

; --- live IDT self-test handler (vector 0x80) ------------------------------
; Proves on the REAL boot that a gate installs, dispatches, and IRETs cleanly so
; control RESUMES at the instruction after `int 0x80`. kmain.c installs this as
; a trap gate on an UNUSED vector (0x80, not an IRQ vector after the 0x28/0x30
; remap) and executes `int 0x80`. The handler just calls a tiny C reporter then
; irets. Self-contained frame (no error code for a software int via a gate).
extern selftest_report_c       ; void selftest_report_c(void)
global isr_selftest
isr_selftest:
    pushad
    push ds
    push es
    mov ax, 0x10               ; DATA_SEL -- known-good segments for the C call
    mov ds, ax
    mov es, ax
    call selftest_report_c
    pop es
    pop ds
    popad
    iretd

; Mark the stack non-executable (silences the ld GNU-stack warning).
section .note.GNU-stack noalloc noexec nowrite progbits
