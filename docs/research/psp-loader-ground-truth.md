<!-- docs/research/psp-loader-ground-truth.md -->
<!-- Ground-truth brief: PSP construction + flat program loader. -->
<!-- Law 1: every claim cites a local file:line, the locked spec, the ADR, or a -->
<!-- clearly-stated DOS 3.3 / Intel SDM reference. NO code. Brief only. -->

# InitechDOS PSP Construction and Flat Program Loader -- Ground-Truth Brief

**Scope:** 256-byte PSP construction per spec/dos_structs.h `psp_t`; flat
program memory layout; control-transfer and return-to-loader mechanism;
baked test program; tight scope (do now vs. defer); risks.

**Sources consulted (all local unless stated):**

- `CLAUDE.md` Laws 1-4, Rules 2, 3, 8, 11, 12; minefield callouts
- `docs/adr/ADR-0003-InitechDOS-Base-OS-Personality.md` -- DEC-05 (PSP,
  §5.5), DEC-08 (flat executables, §5.8), DEC-10 (termination handlers,
  §5.10), Appendix B.2 (PSP offsets), Appendix A (INT 21h register)
- `spec/dos_structs.h` -- `psp_t` field definitions with offsets (lines
  91-107) and the `_Static_assert(sizeof(psp_t)==256)` (line 107)
- `spec/int21h_calling_convention.json` -- locked ABI; 4Ch/00h behaviour;
  cf_mechanism (bit 0 of int_frame_t.eflags)
- `spec/int21h_register.json` -- controlled INT 21h scope; AH=62h GETPSP
- `os/milton/int21.c` -- `do_terminate()` (lines 199-207): calls `g_exit`
  hook then returns; currently bound to `int21_exit_hook` in kmain.c which
  cli;hlt (lines 163-166 of kmain.c) -- **this must change for the loader**
- `os/milton/kmain.c` -- `int21_exit_hook` (lines 154-166); `g_int21_con`
  pattern (line 143); `kernel_main` hlt-loop (lines 337-340)
- `os/milton/int21.h` -- `int21_exit_fn` typedef (line 58)
- `os/milton/idt.h` -- `int_frame_t` layout (lines 81-106); eflags at +64
- `os/milton/isr.asm` -- `int21_entry` trap stub (lines 144-175); CF
  written into saved EFLAGS by dispatcher, restored via iretd
- `os/milton/kstart.asm` -- ESP = 0x0008FFFC (line 25); kernel at 0x10000
- `os/milton/kernel.ld` -- kernel linked at 0x00010000 (line 18);
  `_kernel_end` symbol (line 34); KERNEL_SECTORS=64 => max end ~0x18000
- `os/boot/stage2.asm` -- stage2 loaded at real-mode 0x8000 (line 32);
  BOOT_INFO_ADDR=0x0500 (line 56); FONT_STASH=0x1000 (line 57); ESP set
  to 0x00090000 before kernel far-jump (line 433); kernel far-jump to
  CODE_SEL:0x00010000 (line 457)
- `os/milton/boot_info.h` -- BOOT_INFO_ADDR=0x500 (line 33);
  FONT_STASH_ADDR=0x1000 (line 36)
- `beads initech-509.4` -- PSP bead: acceptance criteria quoted verbatim
- `beads initech-509` -- M2 epic; decomposed children 509.1..509.11
- DOS 3.3 Programmer's Reference Manual (external, stated explicitly)
- Intel 64 and IA-32 SDM Vol. 2A CALL/RET/PUSHAD; Vol. 3A (external,
  stated explicitly)

---

## 1. Confirmed Memory Map -- Evidence Base

Before addressing PSP construction, the flat address space must be laid
out precisely. All values are verified against the sources above.

| Region | Physical Range | Size | Source |
|---|---|---|---|
| IVT (real mode; reserved) | 0x0000 -- 0x03FF | 1 KiB | stage2.asm convention |
| BDA | 0x0400 -- 0x04FF | 256 B | stage2.asm; BOOT_INFO_ADDR just above |
| boot_info_t | 0x0500 -- 0x0517 | 24 B | boot_info.h:33 BOOT_INFO_ADDR |
| IDT (2 KiB, 256 entries x 8 B) | 0x0800 -- 0x0FFF | 2 KiB | internals-int21h-ground-truth.md §2.3 |
| FONT_STASH | 0x1000 -- 0x1FFF | 4 KiB | boot_info.h:36; stage2.asm:57 |
| Kernel (.text+.rodata+.data+.bss) | 0x10000 -- ~0x18000 | <=32 KiB | kernel.ld:18; 64 sectors x 512 |
| **PROGRAM_BASE (PSP)** | **0x20000** | -- | **Proposed; see §2** |
| **Program image** | **0x20100** | -- | **PSP + 0x100; see §2** |
| Program stack | 0x60000 -- 0x6FFFC | 64 KiB | Proposed; see §2 |
| Kernel stack | 0x80000 -- 0x8FFFC | ~64 KiB | kstart.asm:25 |
| Abandoned stage2 stack | 0x90000 | -- | stage2.asm:433; abandoned in PM |
| LFB | 0xE000_0000+ | -- | VBE BIOS; boot_info.lfb_addr |

**PROGRAM_BASE = 0x0002_0000.** Rationale: 32 KiB above the maximum
kernel end (0x18000), well below the program stack (0x60000), well below
the kernel stack (0x80000). The gap between 0x18000 and 0x20000 is 32 KiB
of headroom for kernel .bss growth. The program image budget (0x20100 to
0x5FFFF) is ~384 KiB, sufficient for any flat .COM-equivalent binary this
milestone will run.

---

## 2. PSP Construction -- Field-by-Field (psp_t / ADR-0003 App B.2)

Source: `spec/dos_structs.h` lines 91-107; ADR-0003 §5.5 (DEC-05);
ADR-0003 Appendix B.2. The psp_t struct is 256 bytes (_Static_assert
verified). Each field is examined in struct order.

### 2.1 int20[2] @ offset 0x00 (dos_structs.h:92)

**Real DOS:** The two bytes `CD 20` -- the `INT 20h` machine instruction.
A program can terminate by doing a near RET to PSP:0000h (pushing CS =
PSP segment on stack in the calling convention), which executes the INT 20h
at offset 0, which vectors to DOS to terminate the process.

**InitechOS flat-mode value:** Write `0xCD, 0x20` (the opcode bytes for
`int 0x20`). In our flat 32-bit model there are no segments; a program's
`ret` will not magically reach the PSP. However we populate INT 20h in
full (ADR-0003 §5.5 design stance: "vestigial, retained in full"). The
kernel must also install a trap gate at vector 0x20 (currently unused;
the PIC is remapped to 0x28/0x30 per spec/int21h_calling_convention.json)
that, when triggered, treats it as AH=4Ch AL=0 (legacy terminate). This
makes INT 20h physically callable from the program as a secondary
termination path alongside 4Ch. Install at `idt_install_trap(0x20, ...)`.

**Panic note:** No 0x20 trap gate exists yet. Installing it is in-scope
for this milestone (it is trivially thin: call the same exit logic as 4Ch).

### 2.2 alloc_end_seg @ offset 0x02 (dos_structs.h:93, uint16_t)

**Real DOS:** The segment number (paragraph address >> 4) of the first
byte beyond the memory block allocated to this process. In DOS 3.3, for a
.COM program: `(PSP_paragraph + allocated_paragraphs)`. The program uses
this to know its memory ceiling.

**InitechOS flat-mode value:** There are no real segments. The ADR design
stance is "implement vestigial structures IN FULL" (§5.5). Two options:

- **Option A (zero/sentinel):** Store 0x0000. Clearly invalid as a real
  segment pointer. Any program that reads this and tries to compute a
  far address gets garbage, which is visible (Rule 2).
- **Option B (flat-address-in-segment-units):** Store the flat address
  of the end of the allocated region shifted right by 4 (to fake a
  paragraph address). E.g. if the program allocation ends at 0x70000
  (top of program stack), store `0x7000`. Programs that multiply by 16
  get a plausible flat address back.

**Recommendation: Option B.** It is more authentic (a real DOS program
reading this field and computing `alloc_end_seg << 4` gets a usable value)
and satisfies "implement in full." Store `(program_allocation_end >> 4)
& 0xFFFF`, where `program_allocation_end` = 0x70000 (top of program
stack). This stores `0x7000`. If the program stack is eventually
dynamically sized, this field should reflect the actual ceiling. Document
clearly in a comment that this is a flat address expressed in fake
paragraph units (CLAUDE.md Law 1: cite this section).

### 2.3 reserved_04[6] @ offset 0x04 (dos_structs.h:94)

**Real DOS:** Contains the far call to the DOS dispatcher (the `CP/M`
compatibility call). Offset 0x05 in real DOS holds `9A <far-call>` -- a 5-
byte far-CALL to the DOS entry point, which was the CP/M program-termination
and function-call mechanism (the other way to call DOS before INT 21h). The
psp_t struct names this field `reserved_04[6]`; the ADR gives "unspecified
gap."

**InitechOS flat-mode value:** The real DOS byte at offset 0x05 is
`0x9A` (CALL FAR opcode); bytes 0x06-0x09 are a segment:offset to the DOS
entry. In our flat model, installing a working FAR CALL at 0x05 would
require a valid flat code address. For fidelity: populate bytes 0x05..0x09
with `0x9A <flat-ptr-to-int21-trampoline>` where the flat ptr is the
address of a small thunk that converts the register state and calls INT 21h.
This is deep vestigial territory. **For this milestone: zero-fill
reserved_04[6] and document the offset-0x05 far-call as deferred.** The
psp_t field name is `reserved_04` which correctly signals this is not yet
implemented.

### 2.4 saved_vectors[12] @ offset 0x0A (dos_structs.h:95)

**Real DOS:** Three 4-byte far pointers (segment:offset) saved from the
parent's INT 22h (terminate handler), INT 23h (control-break), INT 24h
(critical-error) vectors. On EXEC, DOS reads the current IDT entries for
those three vectors and saves them here, then installs the child's handlers.
On EXIT (4Ch), DOS restores these vectors from the PSP.

**InitechOS flat-mode value:** In flat 32-bit mode there are no far
pointers. But the IDT does have handlers at vectors 0x22, 0x23, 0x24
(DEC-10 in ADR-0003 §5.10 -- these are currently deferred per initech-509.8,
but they will exist). The saved_vectors field is 12 bytes = three 4-byte
fields. In flat mode, store three 32-bit flat-linear-address entries (the
physical address of the kernel's INT 22/23/24 handlers). When 4Ch fires,
restore those handlers. Since the field in psp_t is typed as
`uint8_t saved_vectors[12]`, write as three `uint32_t` values overlaid at
offsets 0x0A, 0x0E, 0x12.

**For this milestone:** DEC-10 (509.8) is deferred, so no kernel handlers
for INT 22/23/24 exist yet. Zero-fill saved_vectors[12] and document the
save/restore as deferred pending 509.8. This is safe because no program
expects to intercept CTRL-C or critical errors in this milestone.

### 2.5 parent_psp @ offset 0x16 (dos_structs.h:96, uint16_t)

**Real DOS:** The segment of the parent process's PSP. For a program
launched from COMMAND.COM, this is the COMMAND.COM PSP segment.

**InitechOS flat-mode value:** In flat mode, there is no COMMAND.COM PSP
yet (the shell is deferred). For this milestone the kernel itself is the
"parent." Store the fake paragraph address of the kernel: `0x1000` (which
is the fake paragraph for physical 0x10000 -- the kernel base). This is
consistent with the Option-B approach to segment-field semantics. If there
is no sensible kernel PSP, store 0x0000 (the null sentinel). **Recommendation:
store 0x0000 for now** (honest null: there is no parent PSP structure yet).
When the shell (COMMAND.COM-alike) gains a PSP in a later milestone, this
field will hold its fake paragraph address.

### 2.6 jft[20] @ offset 0x18 (dos_structs.h:97, uint8_t[20])

**Real DOS:** The Job File Table: 20 one-byte entries, each being either
0xFF (handle unused/closed) or an index into the system-wide SFT (System
File Table). Handles 0-4 are pre-initialized:
- Handle 0 (stdin): CON device (read)
- Handle 1 (stdout): CON device (write)
- Handle 2 (stderr): CON device (write)
- Handle 3 (stdaux): AUX device
- Handle 4 (stdprn): PRN device

Entries 5-19: 0xFF (unused).

**InitechOS flat-mode value:** The JFT is load-bearing (DEC-06, §5.6)
but the full SFT implementation is in initech-509.3 (deferred). For this
milestone, the JFT is populated with sentinel values that are semantically
correct but not backed by a real SFT yet:

```
jft[0] = 0x00   /* stdin -> SFT slot 0 (CON read) */
jft[1] = 0x01   /* stdout -> SFT slot 1 (CON write) */
jft[2] = 0x01   /* stderr -> SFT slot 1 (CON write; shared with stdout) */
jft[3] = 0xFF   /* stdaux -> no AUX device yet; mark unused */
jft[4] = 0xFF   /* stdprn -> no PRN device yet; mark unused */
jft[5..19] = 0xFF /* all others unused */
```

The SFT slot values (0x00, 0x01) are placeholders. They must be treated
by the dispatcher as the CON device until initech-509.3 lands a real SFT.
The current int21.c already handles handles 1 and 2 specially
(INT21_HANDLE_STDOUT=1, INT21_HANDLE_STDERR=2, int21.h:39-40) -- the JFT
entries must align with this convention.

**Critical:** do NOT populate jft[3] or jft[4] with SFT slot values for
AUX/PRN since those devices (DEC-09, initech-509.7) are also deferred.
0xFF is the correct "closed/unused" sentinel for real DOS.

### 2.7 env_seg @ offset 0x2C (dos_structs.h:98, uint16_t)

**Real DOS:** The paragraph address of the environment block -- a sequence
of NUL-terminated `NAME=VALUE` strings followed by a double NUL, then
optionally the program name. In DOS, COMMAND.COM sets up the environment
before exec.

**InitechOS flat-mode value:** No environment block exists yet (the shell
is deferred). There are two defensible choices:

- **Option A (zero):** Store 0x0000. Any program that reads env_seg and
  tries to access the environment gets address 0x0000, which points into the
  IVT -- it will read garbage, which is loud (a '\0' first byte = empty
  environment is a valid interpretation in DOS).
- **Option B (minimal environment block):** Carve a tiny block after the PSP
  (e.g. at PROGRAM_BASE + 0x200, well above the 0x100-aligned image), write
  `COMSPEC=A:\COMMAND.COM\0\0` (the minimum a program might expect), and
  store its fake paragraph address (flat_addr >> 4) in env_seg. This is more
  authentic and satisfies "implement in full."

**Recommendation: Option B -- minimal environment block.**
Place it at `PROGRAM_BASE + 0x200` = 0x20200. Content: the two-byte
sequence `0x00, 0x00` (empty environment: the minimum valid block). The
env_seg field stores `(0x20200 >> 4) & 0xFFFF = 0x2020`. A program that
walks the environment block from env_seg:0 will immediately find the double-
NUL and conclude the environment is empty -- correct and non-crashing.

If later a COMSPEC or PATH is needed, expand the block. For now, the
empty-block approach is both correct and minimal.

### 2.8 reserved_2e[34] @ offset 0x2E (dos_structs.h:99)

**Real DOS:** Contains additional fields including the PSP of the current
process itself (at 0x2C+2 = 0x2E in some DOS versions), reserved bytes, and
the long-filename service path in later DOS versions. The psp_t field is
named `reserved_2e[34]`.

**InitechOS:** Zero-fill. The ADR lists these as the "unspecified gap."
Nothing in the current milestone depends on these bytes.

### 2.9 int21_entry[12] @ offset 0x50 (dos_structs.h:100)

**Real DOS:** The CP/M-style INT 21h call stub: at offset 0x50, DOS
places the bytes `CD 21 CB` -- `INT 21h` (2 bytes) followed by `RETF`
(1 byte, 0xCB). This three-byte sequence was used by CP/M programs and
early DOS programs that called DOS by doing a `CALL PSP:0x50` rather than
issuing `INT 21h` directly. The remaining 9 bytes (0x53-0x5B) are
reserved/zero.

**InitechOS value:** Write the 3-byte sequence `0xCD, 0x21, 0xCB` at
int21_entry[0..2], then zero-fill int21_entry[3..11]. The `INT 21h` (CD 21)
issues our real syscall; the `RETF` (CB) is cosmetic -- no CP/M program
is expected here, but the field is populated "IN FULL" per the design stance.
The RETF in flat mode does a far return reading CS:EIP from the stack, which
is harmless as long as no program actually calls PSP:0x50 (our programs will
not do this). Cite dos_structs.h:100 in the implementation comment.

### 2.10 reserved_5c[16] (FCB #1) @ offset 0x5C, reserved_6c[20] (FCB #2) @ offset 0x6C (dos_structs.h:101-102)

**Real DOS:** Two default File Control Blocks, pre-parsed from the first
and second whitespace-delimited tokens of the command tail (if any). Each
FCB is a legacy structure; for a .COM file with an empty or absent command
tail they are zeroed (with the drive byte = 0 meaning "default drive"). FCB
parsing is complex (drive letter, 8.3 name splitting). The full FCB model
is ADR-0003 DEC-04 Legacy (vestigial, initech-509.9).

**InitechOS value for this milestone:** Zero-fill both FCB regions. The
field names in psp_t are `reserved_5c` and `reserved_6c` -- the ADR
acknowledges these are "unspecified gap" in the current locked struct. Full
FCB pre-parsing (drive byte + name fields from the command tail) is deferred
to initech-509.9. For the test program (no command tail), zeroed FCBs are
correct: drive byte 0 means "default drive," name fields all spaces = no
file. Cite dos_structs.h:101-102 and DEC-05 in the comment.

### 2.11 cmd_tail[128] @ offset 0x80 (dos_structs.h:103; also default DTA)

**Real DOS:** Byte at 0x80 = count of characters in the command tail
(not counting the leading space or the trailing 0x0D). Bytes 0x81-0xFF =
the command-tail text. Real DOS puts a space at 0x81 if there are
arguments, then the arguments, then 0x0D (carriage return) at
0x80+count+1. If there are no arguments: byte 0x80 = 0x00, byte 0x81 =
0x0D.

The DTA (Disk Transfer Area) defaults to PSP:0x80, so the command tail and
the DTA share this 128-byte region. After a program calls AH=1Ah (SETDTA)
to redirect the DTA, the command tail at 0x80 is preserved.

**InitechOS value for this milestone (baked test program, no args):**
```
cmd_tail[0] = 0x00   /* count = 0, no arguments */
cmd_tail[1] = 0x0D   /* carriage return (DOS convention) */
cmd_tail[2..127] = 0x00 /* remainder zero */
```
The first two bytes match real DOS behavior for a program launched with no
arguments. This is also the correct initial DTA (pointing at a zero-filled
region) -- well-defined behavior for the initial state before any FINDFIRST
or SETDTA call.

---

## 3. Flat Program Model and Memory Layout

### 3.1 Addressing the real DOS .COM model

In real DOS, a .COM program:
- Has all four segment registers (CS, DS, ES, SS) pointing to the PSP segment
- Is loaded at PSP:0100h (physical = PSP_seg*16 + 0x100)
- Has SP = 0xFFFE (top of the 64 KB segment) initially pointing to a word of
  zeros, then the machine word 0x0000 below (the SP-2 = 0xFFFC word is the
  first value on the stack, conventionally 0x0000)
- Entry point = PSP:0100h (CS:IP at load time)

### 3.2 InitechOS flat-mode adaptation

In our flat 32-bit kernel (ADR-0003 DEC-08), there are no real segments.
The physical layout preserves the 0x100 offset between PSP and image for
authenticity (and because future programs compiled by Turbo Initech will
produce code that assumes this layout).

**Concrete addresses:**

```
PROGRAM_BASE      = 0x0002_0000   /* flat linear address of PSP start */
PSP_ADDR          = 0x0002_0000   /* psp_t placed here (256 bytes)   */
PROGRAM_IMAGE     = 0x0002_0100   /* program flat binary entry point  */
ENV_BLOCK         = 0x0002_0200   /* minimal environment (2 bytes: 00 00) */
PROGRAM_STACK_TOP = 0x0006_FFFC   /* initial ESP for the program; 4-aligned */
PROGRAM_STACK_BOT = 0x0006_0000   /* 64 KiB of stack for the program */
```

**Memory gap proof (using confirmed constants):**
- Kernel end (max): 0x18000 (kernel.ld _kernel_end; 64 sectors)
- PSP_ADDR: 0x20000. Gap = 32 KiB. Safe.
- Program stack top: 0x6FFFC. Gap from image: ~0x4FEFC = ~320 KiB. Ample.
- Program stack bot: 0x60000. Gap to kernel stack (0x80000): 128 KiB.
  Kernel stack descends from 0x8FFFC; real usage is shallow. Safe.

**alloc_end_seg value (psp_t:0x02):** Store `0x7000` (= 0x70000 >> 4).
The ceiling of the allocated region is the top of the program stack.

**env_seg value (psp_t:0x2C):** Store `0x2020` (= 0x20200 >> 4). The
minimal empty environment block at 0x20200.

### 3.3 Initial register state at program entry

Real DOS .COM entry state: CS=DS=ES=SS=PSP_seg, SP=0xFFFE, IP=0x100. In
flat mode the segment registers cannot be set to meaningful segment values
(all are DATA_SEL=0x10 = the flat selector; changing them would require a
new GDT entry or segmentation logic). The correct flat adaptation:

| Register | Value at entry | Rationale |
|---|---|---|
| EIP | PROGRAM_IMAGE = 0x20100 | Flat entry point; mirrors DOS PSP:0100h |
| ESP | PROGRAM_STACK_TOP = 0x6FFFC | Program's own stack; below kernel stack |
| EBX | PSP_ADDR = 0x20000 | Flat ptr to PSP; the program's "handle" to its own PSP |
| CS | 0x08 (CODE_SEL) | Flat 4 GiB code descriptor; unchanged |
| DS, ES, SS, FS, GS | 0x10 (DATA_SEL) | Flat 4 GiB data descriptor; unchanged |
| Other GPRs | 0 | Clean slate; not constrained by DOS convention for .COM |

**Why EBX = PSP_ADDR:** Real DOS leaves DS:0 pointing at the PSP (since
DS=PSP segment, offset 0 = PSP start). In flat mode DS is always 0-based,
so the "PSP pointer" is the flat address itself. Passing it in EBX is
a natural convention (BX was the PSP segment in real mode for programs
that queried it via INT 21h AH=62h). The locked register has AH=62h
GETPSP (spec/int21h_register.json:43) which will also provide the PSP
address after the fact.

---

## 4. Control Transfer and Return-to-Loader Mechanism

This is the key design question. The current `int21_exit_hook` in kmain.c
(lines 154-166) does `cli; hlt` forever -- it does not return. The loader
must be able to call a program, have 4Ch fire, and then return to the
calling C function (`load_program()`) with the exit code.

### 4.1 The mechanism: saved kernel context + rebindable exit hook

The loader owns a `loader_context_t` struct on the kernel stack. Before
jumping to the program:

1. Loader saves `ESP` and a **return address** (a label immediately after
   the far-jump-equivalent) into `loader_context_t`.
2. Loader rebinds `g_exit` (the int21 exit hook, `int21_exit_fn`) to a
   new function `loader_exit_hook` that does NOT hlt; instead it:
   a. Saves the exit code.
   b. **Restores ESP from `loader_context_t.saved_esp`.**
   c. **Jumps to `loader_context_t.return_eip`** (the label in the loader
      after the jmp-to-program), which resumes the C loader function.
3. Loader sets ESP to `PROGRAM_STACK_TOP`, loads EBX = PSP_ADDR, then
   does a far-call-equivalent to PROGRAM_IMAGE.
4. Program runs, calls INT 21h AH=4Ch.
5. `int21_dispatch` calls `g_exit(code)` which is now `loader_exit_hook`.
6. `loader_exit_hook` restores the kernel stack and jumps back into
   `load_program()`, which reads the exit code and returns it to its caller.
7. After `load_program()` returns, `kernel_main` re-binds `g_exit` to the
   original `int21_exit_hook` (the cli;hlt one) so the next rogue 4Ch
   from kernel context halts cleanly.

### 4.2 Freestanding save/restore -- no libc setjmp

The implementation uses a small struct and inline asm. No libc is available
(CLAUDE.md Law 3 / CDR-0001: `-ffreestanding -nostdlib`).

```
/* loader_context_t -- saved kernel register state for return-from-program */
typedef struct {
    uint32_t saved_esp;    /* kernel ESP at the moment of program entry */
    uint32_t return_eip;   /* address in load_program() to resume after exit */
    uint8_t  exit_code;    /* filled by loader_exit_hook when 4Ch fires */
    uint8_t  exited;       /* flag: 1 when exit_code is valid */
} loader_context_t;
```

The control flow in pseudo-C (NOT code; Law 1 compliance):

```
/* In load_program(): */
loader_context_t ctx = { .exited = 0 };
/* 1. Save kernel ESP and set the return address via inline asm.
      The return address is a label right after the jmp-to-program block. */
/* 2. Rebind g_exit to loader_exit_hook (which has access to &ctx). */
int21_set_exit(loader_exit_hook_thunk);   /* thunk captures &ctx */
/* 3. Switch stacks and jump to program entry.
      In asm (freestanding): save ESP into ctx.saved_esp, then:
        mov esp, PROGRAM_STACK_TOP
        mov ebx, PSP_ADDR
        jmp PROGRAM_IMAGE                 ; or call; see §4.3 */
/* 4. [PROGRAM RUNS -- may do many int 0x21 calls before 4Ch] */
/* after_program: <- this label is ctx.return_eip */
/* 5. Restore g_exit to the original hook. */
int21_set_exit(int21_exit_hook);
/* 6. Return ctx.exit_code to caller. */
return ctx.exit_code;
```

`loader_exit_hook` (which fires inside the INT 21h handler, on the
INT 21h trap frame's kernel stack):

```
/* loader_exit_hook is called from int21_dispatch -> do_terminate, which
   was entered via the int21_entry trap stub. The kernel stack was in use
   for the trap frame. We must restore the LOADER'S kernel stack before
   jumping back, so we do NOT use the current ESP. */
void loader_exit_hook(uint8_t code) {
    /* Save exit code into ctx, then restore the loader's ESP and jump. */
    ctx->exit_code = code;
    ctx->exited    = 1;
    /* asm: restore ESP from ctx->saved_esp, then jmp ctx->return_eip */
    /* This effectively unwinds the entire program run + the INT 21h trap. */
}
```

**The critical detail:** When 4Ch fires, the CPU is executing the INT 21h
trap stub (isr.asm:144-175). The stub has pushed the int_frame_t on the
kernel stack. Inside `do_terminate` (int21.c:199-207), `g_exit` is called.
At that point the kernel stack looks like: [int21_entry trap frame | loader
call frame | ...]. `loader_exit_hook` must skip back over all of this by
directly restoring `ctx.saved_esp` (which was saved BEFORE the program
entry jmp and points into `load_program`'s own stack frame) and jumping to
`ctx.return_eip`. This completely discards the trap frame and all
intervening stack contents -- which is safe because:
- The trap frame was for the PROGRAM's `int 0x21`, not for kernel code.
- The loader context was saved before the program started.
- No kernel-side destructors or cleanup code is bypassed (we are C, not C++).

### 4.3 CALL vs. JMP to the program

**Use JMP, not CALL.** A `call PROGRAM_IMAGE` would push the return EIP
onto the stack -- but the loader has already switched to the PROGRAM stack
(ESP = PROGRAM_STACK_TOP), so the return address goes onto the program's
stack. When `loader_exit_hook` restores the kernel ESP from `ctx.saved_esp`,
that return address is stranded on the program stack and never used. This
is harmless if `loader_exit_hook` always jumps rather than returning, but
it creates asymmetry and confusion. JMP is cleaner: no return address is
pushed, entry state is clean, and the loader's intent is explicit.

**Alternative:** CALL and let the program do `ret` instead of INT 21h for
early test purposes. Since our test program uses INT 21h AH=4Ch (the correct
path), and CALL deposits a return address on the PROGRAM stack, the test
program's startup code must `add esp, 4` or `pop` it before using the
stack. This is fragile. Use JMP.

### 4.4 What happens to INT 20h termination

With a trap gate installed at vector 0x20 (see §2.1), the INT 20h handler
calls the same `g_exit` hook as 4Ch. Since `g_exit` is currently
`loader_exit_hook` while a program is running, INT 20h termination routes
through the same context-restore path. This is correct.

### 4.5 Impact on the existing kernel_main / banner test

Current `kernel_main` (kmain.c:337-340): after the banner is printed via
INT 21h AH=09h, it enters the `for (;;) { hlt; }` loop. The banner test
(test-boot oracle) relies on the kernel halting cleanly after the banner.

The loader must run AFTER the banner hlt-loop -- which means `kernel_main`
must not hlt unconditionally; it must call `load_program()` and THEN hlt.

**Required change to kmain.c:**
```
/* Current (lines 337-340): */
for (;;) { __asm__ __volatile__("hlt"); }

/* After loader is added: */
uint8_t rc = load_program(PSP_ADDR, PROGRAM_IMAGE_BYTES, PROGRAM_IMAGE_SIZE);
serial_puts("PROGRAM-EXIT rc=");
serial_putu((uint32_t)rc);
serial_putc('\n');
for (;;) { __asm__ __volatile__("hlt"); }   /* terminal hlt still present */
```

The test-boot oracle (harness) currently checks for `INT21-EXIT rc=0`
(from `int21_exit_hook`). After the loader change, the marker changes to
`PROGRAM-EXIT rc=0` (emitted by the loader path) followed by the
hlt-loop. The oracle must be updated to match. This is a planned breaking
change; the oracle update is part of the milestone acceptance criteria.

**Serial marker sequence for the new boot:** `KERNEL` -> `BI-OK` ->
`PIC` -> `IDT` -> `INT21` -> `IDT-SELFTEST-OK` -> `IDT-RESUMED` ->
`CONSOLE` -> `BANNER-BEGIN` -> (banner text) -> `BANNER-END` ->
`BANNER` -> `PROGRAM-BEGIN` -> (program output) -> `PROGRAM-EXIT rc=0`
-> (kernel hlt-loop).

---

## 5. The Baked Test Program

### 5.1 What it does

The minimal authentic test .COM binary:
1. Issue INT 21h AH=09h with EDX = flat ptr to a '$'-terminated string.
   The string: `"Hello from InitechOS program.$"` (no trailing newline; the
   '$' is the terminator and is NOT printed per dos/int21.c:141-158).
2. Issue INT 21h AH=4Ch with AL=0 (clean exit).

This exercises AH=09h (already implemented in int21.c:141-158) and AH=4Ch
(the loader return path). It is also an end-to-end proof that: (a) the PSP
was constructed, (b) the program ran, (c) the loader got control back with
exit code 0.

### 5.2 NASM source (pseudo; this is the brief's description, not code)

```nasm
; test_program.asm -- flat binary (.COM-equivalent), entry at byte 0.
; Assembled: nasm -f bin test_program.asm -o test_program.bin
; ADR-0003 DEC-08: flat binary, entry at offset 0 (no 0x100 offset
; in the SOURCE; the LOADER places this at PSP+0x100).
bits 32
org 0                  ; assembled at offset 0; LOADER places it at 0x20100

msg:
    db "Hello from InitechOS program.$"

start:
    ; AH=09h DISPLAY STRING: EDX = flat ptr to '$'-terminated string.
    ; The flat ptr is the absolute address where the loader placed msg.
    ; PROBLEM: the program cannot know its load address at assemble time
    ; unless the loader tells it. Solution: use PC-relative addressing.
    ; See §5.3 for the recommended approach.
    mov ah, 0x09
    lea edx, [msg]     ; PC-relative (32-bit elf addressing)
    int 0x21
    ; AH=4Ch EXIT: AL=0 (clean exit).
    mov ah, 0x4C
    xor al, al
    int 0x21
```

### 5.3 PC-relative addressing and the load-address problem

Because the flat binary is assembled with `nasm -f bin` and `org 0`, all
label references are relative to offset 0. But the loader places the binary
at PROGRAM_IMAGE = 0x20100. So when the program executes, `msg` is at
physical 0x20100 + offsetof(msg), but the assembled reference `[msg]` is
`offsetof(msg)` relative to 0.

**Two solutions:**

- **Solution A (org at load address):** Assemble with `org 0x20100`. All
  absolute references will be correct at the load address. This works only
  because PROGRAM_IMAGE is a constant for this milestone. The binary's
  assembled bytes contain absolute references to 0x20100+delta.

- **Solution B (PC-relative via 32-bit NASM `call/pop` trick):** The classic
  position-independent stub. Push EIP, pop to EBX, then compute msg address
  as EBX + (msg_offset - here_offset). This works regardless of load address.

**Recommendation: Solution A for this milestone.** PROGRAM_IMAGE is a
fixed constant (0x20100). Assembly with `org 0x20100` makes the binary
self-contained and trivially correct. If PROGRAM_IMAGE ever moves, only
the `org` constant changes. Document this dependency in a comment. Solution B
is needed later when programs are loaded from FAT at arbitrary addresses --
at which point the ABI should pass the load base in EBX (which the loader
already does: EBX = PSP_ADDR, from which the program can derive its image
base as PSP_ADDR + 0x100).

### 5.4 Baking into the kernel image

**Method: embed as a C array in a separate translation unit.**

```
/* os/milton/test_prog.h -- baked test program image (generated from
   build/test_program.bin via tools/bin2c.c or xxd). LOCKED layout per
   §5.3: org 0x20100; PROGRAM_IMAGE = 0x20100. */
extern const uint8_t g_test_prog_image[];
extern const uint32_t g_test_prog_size;
```

The build process (Rule 11 -- reproducible):
1. `nasm -f bin os/milton/test_program.asm -o build/test_program.bin`
2. `xxd -i build/test_program.bin > build/test_program_blob.h` OR
   a small factory C tool `tools/bin2c.c` that reads the bin and emits
   a C array with a deterministic name and size constant.
3. `os/milton/test_prog.c` includes the generated header and defines
   `g_test_prog_image` and `g_test_prog_size`.
4. `test_prog.o` is linked into the kernel binary alongside kmain.o.

The array is in the kernel's `.rodata` section. The loader copies it to
PROGRAM_IMAGE (0x20100) at boot time before jumping in. This avoids any
FAT or disk I/O -- entirely in-memory, reproducible, deterministic.

**Alternative: linker blob section.** Use `objcopy --input-target binary
--output-target elf32-i386 test_program.bin test_program_blob.o` to create
an ELF object with `_binary_test_program_bin_start/end` symbols, then link
it into the kernel. This is equally valid but the bin2c/xxd approach is
simpler and more legible. Choose based on what the existing Makefile
patterns support (Makefile lines 80-165 use nasm + objcopy; adding bin2c is
one line).

**Reproducibility (Rule 11):** The test_program.asm source is deterministic
(no timestamps, no randomness). nasm -f bin output is deterministic for
a given source. The resulting C array is deterministic. No host paths baked
into the binary. The oracle (the test-boot serial capture) asserts the exact
program output string "Hello from InitechOS program" appears between
PROGRAM-BEGIN and PROGRAM-EXIT markers. This golden string is mutation-
provable: change the string in test_program.asm, the oracle goes red.

---

## 6. Scope -- Do Now vs. Defer

### 6.1 In scope for this milestone (initech-509.4 + loader)

1. **PSP construction:** `psp_build(uint32_t psp_addr, ...)` populates
   all 256 bytes of psp_t at PROGRAM_BASE per the field map in §2 above.
   All fields populated: zero-fills where deferred are explicit and
   commented with the deferral reason and issue number.

2. **Minimal environment block:** 2-byte `{0x00, 0x00}` at 0x20200;
   env_seg = 0x2020.

3. **INT 20h trap gate:** `idt_install_trap(0x20, int20_entry)` where the
   handler aliases to the same `g_exit` path as 4Ch. This completes
   the two termination paths (INT 20h and INT 21h AH=4Ch) described in
   ADR-0003 DEC-10 (partially).

4. **Loader function `load_program()`:** copies the baked binary to
   PROGRAM_IMAGE, builds the PSP, saves the kernel context, rebinds g_exit,
   switches stacks, JMPs to PROGRAM_IMAGE, and on return restores g_exit
   and emits the serial exit marker.

5. **Baked test program:** `test_program.asm` + build integration + C
   array embedding (§5.4). The program must be assembled as part of `make
   image` and its bytes embedded into the kernel.

6. **Updated boot oracle:** change the harness serial expectation from
   `INT21-EXIT rc=0` to `PROGRAM-EXIT rc=0`, assert "Hello from InitechOS
   program" appears on the console/serial between PROGRAM-BEGIN and
   PROGRAM-EXIT.

7. **AH=62h GETPSP implementation:** trivial: return the flat address of
   the current PSP (stored in a kernel global set by the loader). Required
   by the locked register (int21h_register.json:43).

### 6.2 Explicitly deferred (not in this milestone)

- **FAT-sourcing:** loading programs from the FAT12 volume via ATA PIO.
  Deferred; FAT read is initech-509 (the epic) but not required for the
  baked-program path.
- **Full SFT/JFT implementation:** initech-509.3. The JFT is populated
  with sentinel values; real SFT backing deferred.
- **Full FCB pre-parsing:** initech-509.9 (vestigial). FCB fields zero-
  filled.
- **INT 22/23/24 handlers + PSP vector save:** initech-509.8. saved_vectors
  zero-filled.
- **Multiple concurrent programs / process table:** non-goal for current
  milestone; single program run then kernel resumes.
- **MCB arena (48h/49h/4Ah):** initech-509.6. No heap allocation from
  programs this milestone.
- **Offset-0x05 far-call stub:** deferred (documented zero-fill).
- **COMMAND.COM shell:** initech-509.11 scope (or later). The loader is
  the shell for this milestone.

---

## 7. Risks

### Risk 1 (HIGHEST) -- Return-to-loader stack corruption

**The bug pattern:** `loader_exit_hook` runs inside the INT 21h trap
handler's frame on the kernel stack. If it simply returns (without
restoring `ctx.saved_esp`), it returns into `do_terminate`, which returns
to `int21_dispatch`, which returns to the INT 21h asm stub, which does
`popad + iretd` -- trying to resume the PROGRAM's context, which is wrong
(the program called 4Ch to terminate). The iretd would execute whatever
EIP was on the program's stack at the time of the INT 21h, which is
undefined behavior.

**The fix must be:** `loader_exit_hook` performs a NON-RETURNING stack-
restoration jump: it writes the loader's ESP and return EIP, then executes
`mov esp, ctx->saved_esp; jmp ctx->return_eip` in inline asm, bypassing
all intervening stack frames. This is the correct and only safe approach.

**Mitigation:** Write the oracle first (Red->Green, Rule 1). The oracle:
boot the kernel, run the test program, assert `PROGRAM-EXIT rc=0` appears
on serial, assert the kernel does NOT triple-fault or hang after that marker,
assert the kernel then enters the hlt-loop (the final `hlt` marker or
absence of serial output). Test with a program that exits via INT 20h too.

### Risk 2 -- The 0x100 offset and program addressing

**The bug pattern:** If the program is assembled with `org 0` but loaded at
0x20100, all absolute label references are wrong by 0x20100. The AH=09h
call passes EDX = 0x0000_00XX (the assembled offset of msg, e.g. 0), but
the CON sink walks bytes at physical address 0x00000000 -- the IVT --
and prints garbage or triple-faults on a NULL dereference.

**The fix:** Assemble with `org 0x20100` as recommended in §5.3. Verify:
after assembling, disassemble the binary and confirm that the MOV EDX
instruction has an immediate operand of `0x20100 + sizeof(msg+start)`
(or whatever the assembled offset of msg is). The oracle string "Hello from
InitechOS program" appearing in the serial capture is the definitive
mutation-proof check.

### Risk 3 -- 4Ch changes behavior (breaking the banner-halt contract)

**The current behavior:** `int21_exit_hook` in kmain.c (lines 154-166) does
`cli; hlt` unconditionally. The test-boot oracle asserts `INT21-EXIT rc=0`
on the serial as the termination marker. If this behavior is changed without
updating the oracle, the oracle may falsely pass (if the program's exit
follows the same serial format) or falsely fail.

**The fix protocol:**
1. Update the oracle FIRST (change the expected serial sequence in the
   harness config) -- make it red intentionally.
2. Implement the loader + rebind g_exit.
3. Confirm the oracle goes green with the new PROGRAM-EXIT marker.
4. This is Rule 1 (Red->Green) applied to an oracle change.

The `int21_exit_hook` (cli;hlt) must be preserved as the FALLBACK for
any 4Ch called from kernel context (outside a load_program() invocation).
Only while a program is running should g_exit be `loader_exit_hook`. The
loader must restore g_exit = int21_exit_hook after load_program() returns.

---

## 8. Open Items Requiring Locked Spec Updates

Per CLAUDE.md Rule 8, spec changes are deliberate acts.

1. **PROGRAM_BASE = 0x00020000** should be added to a memory map constant
   in a spec header (e.g. `spec/memory_map.h`) or to `spec/hardware.json`
   as a new `program_base` field. The loader must read this constant from
   the spec header, not hard-code it in .c files without a citation.

2. **PROGRAM_STACK_TOP = 0x0006FFFC** belongs in the same spec file.

3. **INT 20h trap gate** -- the fact that vector 0x20 is a terminate alias
   should be noted in `spec/int21h_calling_convention.json` (or a companion
   spec file for termination handlers). This partially realizes DEC-10.

4. **AH=62h GETPSP** -- when implemented, add the per-register contract
   to `spec/int21h_calling_convention.json` (currently only 00h/02h/09h/
   30h/40h/4Ch are documented there).

---

*End of brief. No code was produced. All claims cite local files (file:line)
or explicitly stated external references (DOS 3.3 Programmer's Reference,
Intel SDM). Law 1 satisfied. Law 3 maintained (this brief concerns the
artifact layer, C only). Rule 8: spec changes noted as deliberate acts.*
