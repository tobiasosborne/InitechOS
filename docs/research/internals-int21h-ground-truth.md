<!-- docs/research/internals-int21h-ground-truth.md -->
<!-- Ground-truth brief: Interrupt Foundation (IDT/PIC) + INT 21h dispatcher. -->
<!-- Law 1: every claim cites a local file:line, the locked spec, the ADR, or a -->
<!-- clearly-stated Intel SDM / 8259A / DOS reference. NO code. Brief only. -->

# InitechDOS Interrupt Foundation and INT 21h Dispatcher — Ground-Truth Brief

**Scope:** IDT/PIC initialisation, CPU exception handling, 8259A remap/mask,
and the INT 21h calling convention for InitechOS's native 32-bit flat kernel.

**Sources consulted (all local unless stated):**

- `CLAUDE.md` — Laws 1-4, Rules 2, 3, 8, 12; minefield callouts
- `docs/adr/ADR-0003-InitechDOS-Base-OS-Personality.md` — DEC-04 (§5.4), DEC-08 (§5.8), Appendix A (INT 21h register), Appendix B.2 (PSP), Appendix C (message catalogue)
- `spec/int21h_register.json` — locked INT 21h function register (32 functions)
- `spec/dos_structs.h` — `psp_t` (offset 0x50: `int21_entry[12]`)
- `spec/dos_messages.json` — locked diagnostic message catalogue
- `os/boot/stage2.asm` — GDT at lines 499-526; PM entry at lines 425-457; `cli` at line 271
- `os/milton/kstart.asm` — kernel entry stub, ESP set to `0x0008FFFC`
- `os/milton/kmain.c` — `kernel_main`, hlt-loop; no IDT yet
- `os/milton/io.h` — `inb`/`outb` via inline asm
- `os/milton/kernel.ld` — linked at `0x00010000`, flat binary
- Intel 64 and IA-32 SDM Vol. 3A, Chapters 6 (Interrupt and Exception Handling) and 10 (APIC); Vol. 2A/2B for `LIDT`/`IRET` (external reference, stated explicitly)
- Intel 8259A Programmable Interrupt Controller datasheet (external reference, stated explicitly)
- DOS 3.3 Programmer's Reference Manual (external reference, stated explicitly)

---

## 1. Current State — What Exists vs. What Is Needed

**What exists** (verified by reading source files):

- `os/boot/stage2.asm`: Real-to-protected transition at line 276-280. `cli` at line 271 precedes `lgdt`. The far-jump `jmp dword CODE_SEL:pm_entry` (line 280) sets CS=0x08 (CODE_SEL, `gdt_code - gdt_start`). In `pm_entry` (line 425), DS/ES/FS/GS/SS are all set to DATA_SEL=0x10. Interrupts remain **masked** (the `cli` at line 271 is never paired with `sti`; protected-mode entry never enables them).
- `os/milton/kstart.asm` line 25: ESP set to `0x0008FFFC` (kernel stack, grows down from just below 576 KiB).
- `os/milton/kmain.c` line 119: `kernel_main()` runs with no IDT installed. The final `hlt` loop (lines 219-222) runs with `IF=0` (interrupts never enabled). **Any CPU exception or NMI currently triple-faults.**
- `os/milton/io.h`: `inb`/`outb` available for PIC port programming.
- GDT: null (index 0), CODE_SEL=0x08 (ring-0, exec/read, 32-bit, flat 4GiB), DATA_SEL=0x10 (ring-0, read/write, 32-bit, flat 4GiB). Confirmed at `stage2.asm` lines 499-526.

**What is absent:** No IDT, no IDTR load, no PIC remap, no interrupt handlers, no `sti`.

---

## 2. IDT Mechanics for 32-bit Protected Mode

**References:** Intel SDM Vol 3A §6.10 (IDT), §6.11 (IDT descriptors), §6.12 (exception/interrupt handling procedure), Table 6-1 (protected-mode exceptions and interrupts).

### 2.1 Gate descriptor format (8 bytes)

Each IDT entry is an 8-byte gate descriptor. For a **32-bit interrupt gate** (the primary type used here):

```
Bits 63:48  Offset 31:16   (handler address high word)
Bits 47:47  P=1            (present)
Bits 46:45  DPL            (descriptor privilege level; see below)
Bit  44:44  0              (must be 0 for system descriptor)
Bits 43:40  Type           (1110 = 32-bit interrupt gate = 0xE)
Bits 39:37  000            (reserved, must be 0)
Bit  36:32  (reserved)
Bits 31:16  Segment selector  (CODE_SEL = 0x08 for all kernel handlers)
Bits 15: 0  Offset 15:0    (handler address low word)
```

The type/attr byte (bit 47..40 of the 8-byte descriptor) is:
- **0x8E** = Present(1) | DPL=00 | Type=1110 = 32-bit **interrupt gate**
- **0x8F** = Present(1) | DPL=00 | Type=1111 = 32-bit **trap gate**
- **0xEE** = Present(1) | DPL=11 | Type=1110 = 32-bit interrupt gate, callable from ring-3

**Interrupt gate vs. trap gate:** The difference is IF (Interrupt Flag) behavior on entry. An **interrupt gate clears IF** automatically when the handler is entered (Intel SDM Vol 3A §6.12.1.2), preventing nested hardware interrupts. A **trap gate leaves IF unchanged**.

**Which to use:**
- CPU exceptions (vectors 0-31): **interrupt gate** (0x8E). Exceptions require exclusive CPU attention; nested interrupts during exception dispatch would corrupt the handler's stack frame. The only exception is the double-fault (vector 8), where a task gate is theoretically appropriate for a completely isolated stack, but for InitechOS's cooperative single-task model, an interrupt gate pointing to a known-good stack is sufficient.
- INT 21h syscall gate (vector 0x21): **trap gate** (0x8F) with DPL=0. Rationale: the syscall may itself invoke console I/O, which in the future may need timer ticks (PIT IRQ0) to remain enabled without nesting concerns. However, since InitechOS is cooperative and non-preemptive (CLAUDE.md hallucination callout), and since the ring model is ring-0-only in the current release (ADR-0003 DEC-02), a trap gate with DPL=0 is the pragmatic choice. DPL must equal the caller's CPL (0) since our flat programs run at ring 0; a DPL=3 gate would fault on `int 0x21` from ring 0. **Important:** if the kernel later supports ring-3 flat programs, DPL must be raised to 3 for the INT 21h gate (and only that gate) so ring-3 code can issue the software interrupt without a General Protection Fault.
- Hardware IRQ handlers (vectors 0x20-0x2F after remap): **interrupt gate** (0x8E). Hardware interrupts must not nest until the handler issues EOI.

### 2.2 IDT size

The IDT covers vectors 0-255: **256 entries × 8 bytes = 2048 bytes**. Only vectors 0-31 (CPU exceptions) and 0x20-0x2F (hardware IRQs after remap) and 0x21 (INT 21h, though this collides with IRQ1 at 0x21 — see §3 below for remap resolution) need handlers at IDT bootstrap; remaining entries should point to a "spurious interrupt" handler that prints diagnostics and returns cleanly (Rule 2: fail loud).

**REMAP COLLISION NOTE:** Before the PIC is remapped (§3), the BIOS maps master IRQs to vectors 0x08-0x0F, directly overlapping CPU exceptions 8-15. This is precisely why the PIC remap must occur before `sti`, and before installing exception handlers at those vectors is meaningful. After remap, master IRQs are at 0x20-0x27 and INT 21h lands at 0x21 — which is IRQ1 (keyboard) in the remapped space. **This collision between INT 21h (0x21) and remapped IRQ1 (0x21) is a real problem.** See §4 for the resolution.

### 2.3 IDTR load — the `lidt` pseudo-descriptor

`lidt` takes a 6-byte memory operand (Intel SDM Vol 2A, LIDT instruction):
- Bytes 0-1: limit = (256 * 8) - 1 = 0x7FF
- Bytes 2-5: base = linear (= physical, flat model) address of the IDT array

The IDT array should be placed at a stable, known physical address. Reasonable choices: just above the BDA/IVT area, e.g. at `0x0800` (above the BDA at 0x0400-0x04FF, above `BOOT_INFO_ADDR` at 0x0500 + 24 bytes = 0x0518). At `0x0800`, the IDT occupies `0x0800`-`0x0FFF` (2048 bytes), not colliding with `FONT_STASH` at `0x1000` (stage2.asm line 58) or the kernel at `0x10000`. This placement must be confirmed against the memory map and locked in a spec update or a comment in the IDT setup code.

**Sequence:**
1. Allocate IDT storage (e.g. a `uint64_t idt[256]` in `.bss` or at a fixed physical address).
2. Fill all 256 entries with a stub pointing to the spurious-interrupt handler.
3. Fill vectors 0-31 with exception handlers (§2.4).
4. Remap and mask the PIC (§3) — must occur before populating 0x20-0x2F handlers.
5. Fill vectors 0x20-0x2F with IRQ stub handlers (all masked/spurious for now; §3).
6. Fill vector 0x21 with the INT 21h dispatcher (§4) — **after** resolving the 0x21 collision.
7. `lidt [idt_descriptor]`.
8. `sti`.

**The `cli`/`sti` discipline is the paramount risk** (see §6). Never `sti` before step 7 is complete.

---

## 3. CPU Exceptions 0-31 — Error Code List, Handler Behavior

**Reference:** Intel SDM Vol 3A Table 6-1 "Protected-Mode Exceptions and Interrupts."

### 3.1 Which vectors push an error code

This is architecturally fixed by the Intel specification and determines the asm stub layout.

| Vector | Mnemonic | Name | Error Code? |
|--------|----------|------|-------------|
| 0  | #DE | Divide Error | **No** |
| 1  | #DB | Debug | **No** |
| 2  | NMI | Non-Maskable Interrupt | **No** |
| 3  | #BP | Breakpoint | **No** |
| 4  | #OF | Overflow | **No** |
| 5  | #BR | BOUND Range Exceeded | **No** |
| 6  | #UD | Invalid Opcode | **No** |
| 7  | #NM | Device Not Available | **No** |
| 8  | #DF | Double Fault | **Yes** (always 0) |
| 9  | — | Coprocessor Segment Overrun (reserved, 386 only) | **No** |
| 10 | #TS | Invalid TSS | **Yes** |
| 11 | #NP | Segment Not Present | **Yes** |
| 12 | #SS | Stack-Segment Fault | **Yes** |
| 13 | #GP | General Protection Fault | **Yes** |
| 14 | #PF | Page Fault | **Yes** |
| 15 | — | Reserved | — |
| 16 | #MF | x87 FPU Floating-Point Error | **No** |
| 17 | #AC | Alignment Check | **Yes** |
| 18 | #MC | Machine Check | **No** |
| 19 | #XM | SIMD Floating-Point Exception | **No** |
| 20 | #VE | Virtualization Exception | **No** |
| 21 | #CP | Control Protection Exception | **Yes** |
| 22-31 | — | Reserved | — |

Summary of error-code vectors: **8, 10, 11, 12, 13, 14, 17, 21** (Intel SDM Vol 3A Table 6-1).

### 3.2 Uniform stack frame — the dummy error code pattern

To allow a single C dispatcher function accepting a pointer to a uniform register frame, all exception stubs must push a **dummy error code** (conventionally 0) for the no-error-code vectors, then push the vector number, then push all general-purpose registers (`pusha` or individual pushes), then call the C handler. The stack frame seen by C is then uniform regardless of which exception fired.

Stack layout at C handler entry (stack grows down, ESP points at top):

```
[ESP+0]  ... pushed GPRs (pusha order: EAX ECX EDX EBX ESP_saved EBP ESI EDI)
[+32]    vector number (pushed by stub, before pusha)
[+36]    error code (real or dummy 0, pushed before vector)
[+40]    EIP (CPU-pushed)
[+44]    CS  (CPU-pushed)
[+48]    EFLAGS (CPU-pushed)
; If CPL change (not applicable in ring-0-only model):
[+52]    ESP_old (CPU-pushed)
[+56]    SS_old  (CPU-pushed)
```

Since the current kernel is ring-0-only (ADR-0003 DEC-02), no CPL change occurs and the CPU does not push SS:ESP. The frame is fixed size.

### 3.3 Handler behavior for InitechOS — fail loud (CLAUDE.md Rule 2)

Every CPU exception in the current release is fatal. The handler must:

1. **Disable further interrupts** (already done by the interrupt gate, IF=0 on entry).
2. **Dump to serial:** emit the vector number (decimal or hex), the error code, EIP, CS, EFLAGS, EAX-EDI, ESP. Format: `EXCEPTION %02Xh ERR=%08Xh EIP=%08Xh` etc. — ASCII only (CLAUDE.md Rule 12). Use the `serial_putc`/`serial_puts` pattern already established in `kmain.c`.
3. **Emit a console panic line:** a short, visible message to the framebuffer. CLAUDE.md Rule 2 states the in-universe panic screen renders `PC LOAD LETTER`; the full PC LOAD LETTER screen is a separate bead (`initech-s25`). For the IDT milestone the panic line is sufficient: e.g. `PC LOAD LETTER` + the vector in the corner.
4. **Halt permanently:** `cli; hlt` loop. Do NOT triple-fault (the triple-fault recovery is QEMU silently resetting — CLAUDE.md hallucination callout).

**The double-fault (vector 8) is the critical case.** If the exception handler itself faults (e.g. because the stack is corrupt), the CPU delivers a double-fault. If the double-fault handler also faults, the CPU triple-faults and the guest resets — undetectable in QEMU unless `-d cpu_reset` is active (stage2.asm comment line 10; CLAUDE.md hallucination callout). For the current stack layout (ESP=`0x0008FFFC`, adequate depth for the frame), a separate double-fault handler printing `DF ERR=0` + halting is sufficient. A Task State Segment for a separate double-fault stack is deferred.

---

## 4. 8259A PIC Remap and Mask

**Reference:** Intel 8259A Programmable Interrupt Controller datasheet (external). The initialization sequence is a well-known, stable protocol; it is stated explicitly here as an external reference.

### 4.1 The problem with the BIOS mapping

The BIOS initializes the master 8259A to deliver IRQ0-7 at vectors 0x08-0x0F. This directly overlaps CPU exception vectors 8 (Double Fault) through 15 (reserved). In protected mode, a hardware IRQ arriving before the PIC is remapped would be interpreted as a CPU exception — a spurious IRQ0 (timer tick) would look like a double-fault. The PIC must be remapped before `sti`.

### 4.2 ICW initialization sequence

The 8259A is programmed via four Initialization Command Words (ICWs). The sequence is identical for master and slave, but at different port addresses.

**Port addresses:**
- Master PIC: command port `0x20`, data port `0x21`
- Slave PIC: command port `0xA0`, data port `0xA1`

**After remap:**
- Master IRQ0-7 mapped to vectors `0x20`-`0x27`
- Slave IRQ8-15 mapped to vectors `0x28`-`0x2F`

**Initialization sequence (both PICs):**

```
ICW1 to command port:  0x11   (0x10 = init, 0x01 = ICW4 needed; edge triggered)
ICW2 to data port:     0x20 (master) / 0x28 (slave)  (base vector)
ICW3 to data port:     0x04 (master: slave on IRQ2, bit 2 set)
                       0x02 (slave:  cascade identity = IRQ2)
ICW4 to data port:     0x01   (8086/88 mode; no auto-EOI)
```

Each write to the data port must be followed by a short I/O delay (one `outb` to port `0x80` with value 0, or equivalent) to allow the PIC to settle. The `outb` primitive in `os/milton/io.h` is available for this.

### 4.3 Masking all IRQs

After ICW4, write OCW1 (the interrupt mask register) to mask all IRQs:

```
outb(0x21, 0xFF);   /* mask all master IRQs */
outb(0xA1, 0xFF);   /* mask all slave IRQs  */
```

An unmasked bit (0) enables the IRQ; a masked bit (1) suppresses it. Writing `0xFF` to both data ports suppresses all 15 hardware IRQs. The shell phase (or device-driver init) unmasks specific IRQs (PIT timer = IRQ0 at `0x20`, keyboard = IRQ1 at `0x21`) when needed.

### 4.4 End of Interrupt (EOI) protocol — for future IRQ handlers

When a hardware IRQ handler is later implemented, it must issue EOI before `iret`. For IRQ0-7 (master only): `outb(0x20, 0x20)`. For IRQ8-15 (slave + master cascade): `outb(0xA0, 0x20)` then `outb(0x20, 0x20)`. No EOI is needed for software interrupts (INT 21h).

---

## 5. INT 21h Calling Convention — The Key Decision

### 5.1 The collision: INT 21h = vector 0x21 = remapped IRQ1

After the PIC remap, IRQ1 (keyboard) maps to vector 0x21 — the same vector as INT 21h. **This is a genuine conflict** that must be resolved before any IDT is installed.

**Resolution options:**

1. **Move INT 21h to a different vector.** Assign the syscall gate to a different vector, e.g. `0x30` (used by Linux on x86), and have all programs use `int 0x30`. This avoids any collision but diverges from the DOS convention (`int 0x21`).
2. **Keep INT 21h at 0x21 and move IRQ1 elsewhere.** ICW2 for master can map IRQs to any 8-aligned base. Remap master to `0x28`-`0x2F` and slave to `0x30`-`0x37`. Then `0x21` is free for INT 21h. This is non-standard but functional.
3. **Keep INT 21h at 0x21 and disable IRQ1 at the PIC.** The keyboard IRQ is masked initially (§4.3), so the conflict is latent. When the keyboard is eventually enabled, the IRQ1 handler at `0x21` and the INT 21h dispatcher share the gate. A software interrupt from ring 0 via `int 0x21` is distinguishable from a hardware IRQ delivery: the CPU delivers them differently (hardware IRQ vs. software INT), but they share the same gate descriptor. This is **incorrect and dangerous**: the same handler code would be invoked for both, and a hardware IRQ delivery would attempt to run the syscall dispatcher.

**Recommendation: Option 2** — remap master PICs to `0x28`-`0x2F`, slave to `0x30`-`0x37`, leaving `0x21` clean for INT 21h. This keeps the DOS-idiomatic `int 0x21` syscall number while avoiding the IRQ1 collision. The INT 21h gate is installed at vector 0x21; IRQ0-7 handlers are at 0x28-0x2F; IRQ8-15 at 0x30-0x37.

**This remap choice (master base `0x28` instead of `0x20`) is a design decision that affects the IDT vector map and must be locked as spec-data or ratified in an ADR decision record (see §5.6).**

### 5.2 Flat 32-bit calling convention — concrete specification

**Context:** InitechDOS does not run genuine 16-bit real-mode executables (ADR-0003 §1.2, §5.2). INT 21h is the syscall surface for InitechOS's own flat 32-bit programs (ADR-0003 DEC-04 §5.4; DEC-08 §5.8). There are no usable segments in the flat model — DS/ES/SS are all the flat DATA_SEL base-0 descriptor. Real DOS's 16-bit register widths (AX, DX, DS:DX pointers) do not map directly; an explicit 32-bit adaptation is required.

**Proposed convention:**

| Role | Register | Rationale |
|------|----------|-----------|
| Function selector | AH (= bits 15:8 of EAX) | Preserves byte-compatibility with DOS mnemonics in the locked register (`spec/int21h_register.json`). AH=02h, AH=09h, AH=4Ch etc. map directly. AL carries sub-function or input byte as in DOS. |
| Primary pointer argument | EDX (32-bit linear address) | DOS used DS:DX; in flat mode DS is always base-0, so EDX is the flat pointer. Compatible: a 16-bit DOS program's `mov dx, offset buf` becomes `mov edx, buf_flat_addr`. |
| Byte count / secondary arg | ECX | DOS used CX for count (e.g. INT 21h AH=40h: CX=count). Zero-extend: ECX=count. |
| Handle / sub-selector | EBX | DOS used BX for file handles (AH=3Fh/40h/3Eh etc.). |
| Return value | EAX (32-bit) | DOS returned results in AX; we use full EAX. AL for single-byte returns (character input), AX for 16-bit legacy results, EAX for 32-bit quantities (file position, etc.). |
| Error flag | Carry flag (CF) in saved EFLAGS | DOS convention: CF=0 on success, CF=1 on error with AX=error code. See §5.3 for the mechanics of propagating CF back to the caller. |

**Pointer arguments are flat 32-bit linear addresses throughout.** No far-pointer (segment:offset) arithmetic occurs. This is the consequence of ADR-0003 DEC-02 (no binary compatibility) and DEC-08 (flat executables).

### 5.3 How C code invokes INT 21h

A flat 32-bit program issues `int 0x21` with registers pre-loaded. In C with inline asm (the pattern consistent with `io.h`):

```c
/* Example: INT 21h AH=02h (character output), DL = char */
static inline void dos_putchar(char c) {
    __asm__ __volatile__(
        "int $0x21"
        :                               /* no outputs */
        : "a"(0x0200u | (uint8_t)c)     /* AH=02h, AL=char */
        : "cc", "memory"
    );
}
```

For functions that return values and set CF:

```c
static inline int dos_write(uint16_t handle, const void *buf, uint32_t count,
                             uint32_t *written) {
    uint32_t result;
    uint32_t cf;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "setc %b1"
        : "=a"(result), "=r"(cf)
        : "a"(0x4000u), "b"(handle), "c"(count), "d"((uint32_t)buf)
        : "cc", "memory"
    );
    if (!cf && written) *written = result;
    return cf ? -(int)result : 0;
}
```

### 5.4 How the dispatcher reads caller registers — the register frame

The asm entry stub (to be written in NASM in a file such as `os/milton/int21_entry.asm`) must:

1. Preserve all caller registers (the caller's C code may have values in any register).
2. Present a uniform frame to the C dispatcher.
3. On return, restore all registers except those that carry return values (EAX, EFLAGS).

**Recommended stub pattern (NASM, 32-bit flat):**

```asm
; Pseudo-code — not source code, this is the brief.
; CPU has pushed: EFLAGS, CS, EIP (no SS:ESP — ring-0 caller).
; No error code for INT (software interrupt gate).
pushad                  ; push EAX,ECX,EDX,EBX,ESP_saved,EBP,ESI,EDI
                        ; (Intel pushad order: EAX ECX EDX EBX ESP ECX EBP ESI EDI)
push ds
push es
push fs
push gs
push esp                ; pointer to the frame on the stack -> arg to C dispatcher
call int21_dispatch     ; void int21_dispatch(struct regs *r)
add esp, 4              ; pop the frame pointer arg
pop gs
pop fs
pop es
pop ds
; Before popad: set or clear CF in the saved EFLAGS image.
; The saved EFLAGS is at a known offset above ESP (see below).
; The C dispatcher has stored its CF result in r->eflags (or a dedicated field).
popad                   ; restores EAX (the return value) and all other GPRs
                        ; CF is NOT restored by popad -- must fix EFLAGS on stack
iretd                   ; pops EIP, CS, EFLAGS -- EFLAGS includes CF
```

**The carry-flag return problem:** `popad` does not restore EFLAGS; `iretd` pops the EFLAGS that was pushed by the CPU. To return CF to the caller, the C dispatcher must modify the **saved EFLAGS image on the stack** before `iretd`. The stub must compute the offset of the saved EFLAGS and expose it to the C dispatcher through the `regs` struct.

**Recommended regs struct shape:**

```c
/* Ref: Intel SDM Vol 3A §6.12.1 (interrupt stack frame) +
 *      NASM pushad order (Intel Vol 2A, PUSHAD). */
typedef struct {
    /* Pushed by stub in this order (top of stack = lowest address): */
    uint32_t gs, fs, es, ds;    /* segment registers (selector values) */
    /* pushad order: EAX,ECX,EDX,EBX,old_ESP,EBP,ESI,EDI */
    uint32_t edi, esi, ebp, esp_saved, ebx, edx, ecx, eax;
    /* Pushed by CPU (no CPL change, ring-0 -> ring-0): */
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;        /* C dispatcher writes CF here before iretd */
} int21_frame_t;
```

**Carry-flag mechanism detail:** The C dispatcher receives `int21_frame_t *r`. On success: `r->eflags &= ~0x1u` (clear CF bit 0). On error: `r->eflags |= 0x1u` (set CF bit 0); `r->eax = error_code`. The `iretd` then restores the modified EFLAGS, so the caller sees CF correctly. **This is cleaner than a separate error-in-register convention** because it preserves the DOS `int 0x21 / jc error` idiom that period programs (and Turbo Initech-compiled programs, ADR-0007) will use.

**AH dispatch in C:**

```c
void int21_dispatch(int21_frame_t *r) {
    uint8_t func = (uint8_t)(r->eax >> 8);  /* AH = bits 15:8 of EAX */
    switch (func) {
        case 0x02: ...
        case 0x09: ...
        case 0x40: ...
        case 0x4C: ...
        default:
            /* Controlled scope: unlisted AH -> diagnostic per DOS message catalogue */
            /* ADR-0003 DEC-13: MSG-DOS-0002 "Bad command or file name" is the */
            /* closest controlled message for an invalid syscall. Serial: SYSCALL-UNKNOWN AH=XX. */
            /* Return CF=1, AX=0x01 (invalid function). Do NOT silent no-op or hang. */
            r->eflags |= 0x1u;
            r->eax = 0x0001u;
            return;
    }
}
```

### 5.5 Does the INT 21h calling convention warrant an ADR?

**Yes. An ADR (or a formal Amendment to ADR-0003) is warranted, not merely a worklog note.**

Reasons:
1. The PIC remap base choice (§5.1) affects the entire vector map — an architectural decision with no correct answer derivable from existing ADRs.
2. The flat 32-bit register adaptation (AH dispatch, EDX=pointer, CF return) is a **new convention** not derivable from any existing ratified document. The locked register (`spec/int21h_register.json`) is declared as "COARSE: ah/mnemonic/description/class only" and explicitly does not specify the per-register calling convention.
3. The `int21_frame_t` struct is load-bearing for all future syscall implementations; its layout must be locked as spec-data (CLAUDE.md Rule 8).
4. The DPL decision (ring-0-only or ring-3-callable gate) is a forward-compatibility choice affecting Turbo Initech programs (ADR-0007, pending).

A new **ADR-0005** (or Amendment DEC-04a to ADR-0003) should ratify: (a) the PIC remap base vectors; (b) the flat 32-bit INT 21h calling convention (AH dispatch, register roles, CF return); (c) the `int21_frame_t` struct as locked spec-data; (d) DPL policy. Pending ratification, the convention should be implemented exactly as specified here and flagged as provisional in the code comment.

---

## 6. First Functions to Implement — Console Subset (Pre-Filesystem)

These require no SFT, no FAT, no file handles beyond predefined handles 0-4, and no memory arena. They can land before `initech-509.3` (or whatever the first filesystem milestone is).

**Reference:** DOS 3.3 Programmer's Reference Manual (external, stated). ADR-0003 Appendix A (locked register). `spec/int21h_register.json` (function class = Resident). `spec/dos_messages.json` (controlled output messages).

### 6.1 AH=02h — Character Output (CON I/O, class Resident)

DOS convention: DL = character to output to stdout (handle 1 / CON device). Returns AL=DL (last char written) in DOS; no CF return. Flat 32-bit adaptation: **DL** (low byte of EDX) = character; output to CON device (call `console_putc` or equivalent). Return: EAX unchanged (or AL=DL for fidelity). CF=0 always (no error path for console output in DOS 3.3).

### 6.2 AH=09h — String Output (CON I/O, class Resident)

DOS convention: DS:DX = pointer to '$'-terminated string; writes to stdout. Flat adaptation: **EDX** = flat 32-bit linear address of the '$'-terminated string. Walk bytes from EDX until `'$'`, calling the character output path for each. Return: AL='$' (DOS convention). CF=0.

**Note on the '$' terminator:** This is the locked DOS convention. InitechDOS must reproduce it for period authenticity (ADR-0003 DR-1, DR-3). Do not substitute NUL termination.

### 6.3 AH=40h — Write to File or Device (WRITE, class Core), restricted to CON handles

DOS convention: BX=handle, CX=count, DS:DX=buffer. Returns AX=bytes written on success (CF=0); AX=error code on failure (CF=1). Flat adaptation: **EBX**=handle (uint16, though 32-bit register), **ECX**=byte count, **EDX**=flat linear address of buffer. Implementation constraint: for the pre-filesystem phase, only accept handles 1 (stdout) and 2 (stderr), which are predefined to map to CON. For any other handle, return CF=1, AX=0x06 (invalid handle — DOS error code 6). This restriction should be noted in a comment citing the milestone scope.

### 6.4 AH=4Ch — Terminate with Return Code (EXIT, class Core)

DOS convention: AL=return code. Process terminates; control returns to parent. In the initial single-task model (no process hierarchy yet), `int 0x21 / AH=4Ch` from the shell or initial program means "halt the OS" — emit the return code to serial (`EXIT rc=XX`) and execute the hlt-loop. This is the `kernel_main` pattern from `kmain.c` lines 219-222.

### 6.5 AH=00h — Terminate Program (TERMINATE, class Resident)

Legacy CP/M-derived: no register arguments. Equivalent to `AH=4Ch, AL=0`. Implement as an alias.

### 6.6 AH=30h — Get OS Version (GETVER, class Resident)

DOS convention: returns AL=major, AH=minor, BH=OEM. InitechDOS version is 3.30 (ADR-0003 DEC-12 §5.12; `spec/dos_banner.txt`). Flat adaptation: return **EAX** = `0x1E03` (AL=3, AH=30=0x1E), BH=0x00 (OEM). CF=0. This is trivial but needed early since programs often call GETVER to verify the DOS version before proceeding.

---

## 7. Locking the Calling Convention as Spec-Data (Rule 8)

CLAUDE.md Rule 8 mandates: "Region algebra, xBase coercion table, chrome metrics, asset sheet, hardware contract live as versioned plain JSON / C headers under `spec/`. The locked spec is the contract."

**Recommendation:** Create `spec/int21h_calling_convention.json` as a new locked spec-data file. The existing `spec/int21h_register.json` is explicitly "COARSE" (its own `_comment`) and covers only ah/mnemonic/description/class.

**Proposed schema:**

```json
{
  "_comment": [
    "InitechDOS INT 21h Flat 32-bit Calling Convention -- LOCKED spec-data (CLAUDE.md Rule 8).",
    "Source: docs/research/internals-int21h-ground-truth.md (provisional pending ADR-0005).",
    "Companion to spec/int21h_register.json (the function set) and",
    "spec/dos_structs.h (psp_t, dir_entry_t, mcb_t).",
    "AH-dispatch: function selector in bits 15:8 of EAX (the AH byte).",
    "All pointer arguments are FLAT 32-bit LINEAR addresses -- no segment:offset."
  ],
  "source": "docs/research/internals-int21h-ground-truth.md",
  "status": "provisional -- pending ADR-0005 ratification",
  "abi": {
    "function_selector": "AH (bits 15:8 of EAX)",
    "primary_pointer":   "EDX (flat 32-bit linear address)",
    "byte_count":        "ECX",
    "handle":            "EBX",
    "return_value":      "EAX (AL for single-byte; AX for 16-bit legacy; EAX for 32-bit)",
    "error_flag":        "CF in saved EFLAGS (CF=1 -> error; AX=error_code)",
    "cf_mechanism":      "dispatcher writes bit 0 of int21_frame_t.eflags before iretd"
  },
  "pic_remap": {
    "master_base": "0x28",
    "slave_base":  "0x30",
    "reason":      "0x21 reserved for INT 21h syscall gate; IRQ1 (KB) would collide at 0x21 with master_base=0x20"
  },
  "functions": [
    {
      "ah": "00h", "mnemonic": "TERMINATE",
      "in": {},
      "out": {"AL": "0 (return code)"},
      "cf": "never set",
      "notes": "Alias for 4Ch AL=00h in single-task model: emit serial EXIT rc=0, hlt-loop."
    },
    {
      "ah": "02h", "mnemonic": "PUTCHAR",
      "in": {"DL": "character to output"},
      "out": {"AL": "DL (character written)"},
      "cf": "never set",
      "notes": "Output DL to CON device (stdout handle 1)."
    },
    {
      "ah": "09h", "mnemonic": "PUTS",
      "in": {"EDX": "flat ptr to $-terminated string"},
      "out": {"AL": "0x24 ('$')"},
      "cf": "never set",
      "notes": "Walk bytes at EDX until '$' (0x24); emit each via PUTCHAR path."
    },
    {
      "ah": "30h", "mnemonic": "GETVER",
      "in": {},
      "out": {"AL": "3", "AH": "30 (0x1E)", "BH": "0 (OEM)"},
      "cf": "never set",
      "notes": "Version 3.30 per ADR-0003 DEC-12; spec/dos_banner.txt."
    },
    {
      "ah": "40h", "mnemonic": "WRITE",
      "in": {"EBX": "handle", "ECX": "byte count", "EDX": "flat ptr to buffer"},
      "out": {"EAX": "bytes written on success"},
      "cf": "set on error (AX=error code; 0x06=invalid handle for non-CON handles in pre-FS phase)",
      "notes": "Pre-FS phase: accept handles 1 (stdout) and 2 (stderr) only."
    },
    {
      "ah": "4Ch", "mnemonic": "EXIT",
      "in": {"AL": "return code"},
      "out": {},
      "cf": "never set (handler does not return to caller)",
      "notes": "Emit serial EXIT rc=AL; hlt-loop. In future: restore parent context."
    }
  ],
  "unlisted_ah_behavior": {
    "action": "return CF=1, AX=0x0001 (invalid function)",
    "serial_diagnostic": "SYSCALL-UNKNOWN AH=XX",
    "controlled_message": "MSG-DOS-0002 (Bad command or file name) -- via console if initialized",
    "rationale": "CLAUDE.md Rule 2 (fail loud); ADR-0003 DEC-13 (controlled vocabulary). Silent no-op or hang is forbidden."
  },
  "frame_struct": {
    "name": "int21_frame_t",
    "layout_top_to_bottom_in_memory": [
      {"field": "gs",       "bytes": 4},
      {"field": "fs",       "bytes": 4},
      {"field": "es",       "bytes": 4},
      {"field": "ds",       "bytes": 4},
      {"field": "edi",      "bytes": 4, "note": "pushad order per Intel SDM Vol 2A PUSHAD"},
      {"field": "esi",      "bytes": 4},
      {"field": "ebp",      "bytes": 4},
      {"field": "esp_saved","bytes": 4},
      {"field": "ebx",      "bytes": 4},
      {"field": "edx",      "bytes": 4},
      {"field": "ecx",      "bytes": 4},
      {"field": "eax",      "bytes": 4, "note": "return value written here by dispatcher"},
      {"field": "eip",      "bytes": 4, "note": "pushed by CPU"},
      {"field": "cs",       "bytes": 4, "note": "pushed by CPU (padded to 32 bits)"},
      {"field": "eflags",   "bytes": 4, "note": "pushed by CPU; dispatcher writes CF (bit 0) here"}
    ],
    "cf_bit": "bit 0 of eflags field"
  }
}
```

This file must be created as a deliberate act with a beads issue and worklog note (CLAUDE.md Rule 8). It is not a silent edit — it is the first lock of the calling convention.

---

## 8. Risks and Minefield

### Risk 1 — cli/sti discipline (HIGHEST priority)

The kernel currently runs with IF=0 (interrupts disabled from stage2.asm line 271's `cli`, never re-enabled). The IDTR is not loaded. **Any `sti` before a valid IDT is installed causes an immediate triple-fault** if any interrupt arrives (including the spurious IRQ that the uninitialized 8259A may deliver at power-on). The mandatory sequence is: install IDT gates -> remap+mask PIC -> lidt -> sti. Violating this order results in a silent QEMU guest reset (CLAUDE.md minefield callout; stage2.asm comment line 10).

The guard: use Bochs with `<exception breakpoint>` enabled to catch #UD and #GP before they cascade. Verify with `-d int,guest_errors,cpu_reset` (CLAUDE.md Rule 5 + Build section).

### Risk 2 — regs-struct/stack-frame layout error

The `int21_frame_t` field order must exactly match the asm stub's push sequence. A single transposition (e.g. `edi` and `esi` swapped in the C struct relative to the `pushad` machine order) means the dispatcher reads the wrong register value, corrupts the caller's frame, and returns garbage. The classic manifestation: a `WRITE` call with EDX as the buffer pointer silently passes a random value to the C function because the struct layout was off by one field.

**Mitigation:** Write a unit test in the harness (host C) that assembles a known `int21_frame_t` byte pattern and verifies field extraction by the C dispatcher. This is the "write the oracle first" step (CLAUDE.md Rule 1, TDD shape 1).

The `pushad` order per Intel SDM Vol 2A is: **EAX, ECX, EDX, EBX, ESP (pre-push value), EBP, ESI, EDI** (pushed in this order, so EDI ends up at lowest address on stack). The C struct must match this reversed order when read from ESP upward.

### Risk 3 — INT 21h / IRQ vector collision and PIC base selection

If the PIC is remapped with master base `0x20` (common practice, e.g. Linux), IRQ1 (keyboard) lands at vector `0x21`, permanently colliding with the INT 21h gate. Hardware delivery of IRQ1 invokes the syscall dispatcher — a completely wrong handler — and the keyboard generates spurious syscall-dispatcher invocations, corrupting EAX/EBX/EDX and returning via `iretd` with a bogus EFLAGS. The dispatcher loop sees AH=whatever was in EAX at keyboard interrupt time.

**Mitigation:** Use master base `0x28` as recommended in §5.1. This is the primary reason the remap base choice needs an ADR/locked spec-data decision — it cannot be left as an implementation detail.

---

## 9. Summary of Open Architecture Decisions Requiring ADR/Locked Spec

| Topic | Status | Action |
|-------|--------|--------|
| PIC remap base vectors (master=0x28, slave=0x30) | Proposed here, not ratified | ADR-0005 or DEC-04a amendment |
| INT 21h flat 32-bit calling convention (AH dispatch, EDX ptr, CF return) | Proposed here, not ratified | ADR-0005 + `spec/int21h_calling_convention.json` |
| `int21_frame_t` struct layout (locked spec-data) | Proposed here, not locked | `spec/int21h_calling_convention.json` |
| INT 21h gate DPL (ring-0 only vs. ring-3 callable) | Proposed ring-0 DPL=0 for current release | Must be explicit in ADR-0005; ring-3 DPL=3 required when user programs gain CPL change |
| IDT physical base address (proposed 0x0800) | Proposed here | Verify against memory map; lock in hardware.json or a new spec/idt.h |

---

*End of brief. No code was produced. All claims cite local files (file:line) or explicitly stated external references (Intel SDM Vol 3A, 8259A datasheet, DOS 3.3 Programmer's Reference). Law 1 satisfied.*
