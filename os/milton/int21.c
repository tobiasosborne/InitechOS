/* int21.c -- InitechDOS INT 21h dispatcher (the `int 0x21` syscall spine).
 *
 * beads: initech-509.5. Gate ratification: initech-1f9. CONSOLE subset only
 *        (no filesystem); file-handle/SFT functions are deferred to
 *        initech-509.3.
 * Ref:   docs/research/internals-int21h-ground-truth.md Sec 5 (flat calling
 *        convention + the CF-in-EFLAGS return mechanism), Sec 6 (the console
 *        subset: 02h/09h/40h/30h/4Ch/00h); spec/int21h_calling_convention.json
 *        (the LOCKED ABI + per-function contract); spec/int21h_register.json
 *        (the controlled scope -- the recognized AH set; ADR-0003 DEC-04);
 *        spec/dos_messages.json (controlled diagnostics; ADR-0003 DEC-13).
 *        CLAUDE.md Law 1, Law 2, Law 3 (artifact = C), Rule 2 (fail loud +
 *        controlled scope), Rule 8, Rule 11, Rule 12.
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only. The
 * SAME TU compiles HOSTED for os/milton/test_int21.c. All "display" bytes go
 * through g_sink; terminate goes through g_exit -- so the dispatch logic never
 * calls the console/serial directly and is fully host-testable.
 */

#include "int21.h"
#include "sft.h"        /* JFT->SFT handle layer (beads initech-509.3); pulls psp.h */
#include "mcb.h"        /* MCB arena allocator behind AH=48h/49h/4Ah (initech-509.6) */
#include "dos_structs.h" /* exec_param_block_t (AH=4Bh EBX block; initech-456)      */
#include "find_data.h"  /* find_data_t (43-byte DTA block, LOCKED; spec/) */
#include "irq.h"        /* in-IRQ depth + reentrancy guard (beads initech-xk2) */
#include "dos_messages.h" /* MSG_DOS_0001 controlled diagnostic (DEC-13; -Ibuild) */

/* The carry flag is bit 0 of EFLAGS (Intel SDM Vol 1 Sec 3.4.3). The whole
 * error-return contract rides on this single bit (ground-truth Sec 5.4). */
#define CF_BIT 0x1u

/* The zero flag is bit 6 of EFLAGS (Intel SDM Vol 1 Sec 3.4.3.1). AH=06h DL=FF
 * and AH=0Bh report "no character available" via ZF=1 (DOS 3.3 PRM AH=06h);
 * we touch ONLY this bit, mirroring the CF helpers' single-bit discipline. */
#define ZF_BIT 0x40u

static int21_sink_fn g_sink = 0;
static int21_exit_fn g_exit = 0;

/* CON input source (beads initech-n62). g_conin_get is the BLOCKING read (never
 * -1; the kernel impl spins on hlt); g_conin_poll is the NON-blocking poll
 * (char 0..255, or -1 if none). NULL until the kernel binds the keyboard (or the
 * host oracle binds a mock) -- input functions then return EOF/no-char, NEVER
 * hang or fault (Rule 2). */
static int21_conin_fn     g_conin_get  = 0;
static int21_coninpoll_fn g_conin_poll = 0;

/* The current process's PSP (beads initech-509.3). Handle functions resolve a
 * handle through g_cur_psp->jft into the system SFT. NULL until the kernel binds
 * one at SYSINIT; a handle function with no bound PSP returns invalid-handle. */
static psp_t *g_cur_psp = 0;

/* The current working directory (beads initech-mzxa; ti8 Layer 2, READ side).
 * A RELATIVE path (no leading '\', no 'X:' prefix) resolves from here; an
 * ABSOLUTE or drive-prefixed path resolves from the root. `cwd_start_cluster`
 * is the CWD's first data cluster (0 == the fixed root). `cwd_path` is the
 * canonical root-relative path text AH=47h GET CURRENT DIR reports (NO leading
 * '\', NO drive). Until the CHDIR writer (AH=3Bh, the NEXT bead initech-u6wa)
 * lands, the CWD never leaves the root -- but ALL the read-side plumbing is here
 * now so u6wa only adds the 0x3Bh writer. Reset to root on each program launch
 * (loader.c int21_cwd_reset) and on do_terminate, and saved/restored around the
 * kernel-context PSP rebinds (kmain) so a child's CWD never leaks to the kernel.
 * The 64-byte cap (INT21_CWD_MAX, int21.h) mirrors the DOS AH=47h buffer. */
static char     g_cwd_path[INT21_CWD_MAX] = { 0 };  /* root-relative, no '\' / drive */
static uint16_t g_cwd_start_cluster       = 0u;     /* 0 == the fixed root            */

/* The CLOCK source (beads initech-yv9). GET/SET DATE+TIME reach the wall clock
 * through these; NULL until the kernel binds the RTC (or the host oracle binds a
 * fixed mock). With no clock bound, GET returns the DOS epoch and SET fails. */
static int21_clock_get_fn g_clock_get = 0;
static int21_clock_set_fn g_clock_set = 0;

/* The interrupt-vector table seam (beads initech-509.8). AH=25h SETVECT / AH=35h
 * GETVECT reach the live IDT through these so int21.c does not link idt.c and
 * stays host-testable. NULL until the kernel binds idt-backed callbacks at
 * SYSINIT (or the host oracle binds a mock vector table). With none bound,
 * SETVECT is a graceful no-op and GETVECT returns 0 -- never a fault (Rule 2). */
static int21_setvect_fn g_setvect = 0;
static int21_getvect_fn g_getvect = 0;

/* The file backend (beads initech-509.5 read-side). NULL until the kernel binds
 * a FAT12-backed impl (or the host oracle binds a mock). The file functions
 * resolve FAT-specific work through it so int21.c stays host-testable. */
static const int21_file_backend_t *g_file = 0;

/* The EXEC backend (beads initech-saw / AH=4Bh). NULL until the kernel binds the
 * FAT-sourced loader (load_program_from_fat) or the host oracle binds a mock.
 * AH=4Bh reaches the actual load+run through this seam so int21.c does not link
 * loader.c (keeping it host-testable). */
static int21_exec_fn g_exec = 0;

/* The most recent child exit code, retrievable via AH=4Dh GET-RETURN-CODE
 * (DOS 3.3 PRM AH=4Dh). Set by a successful AH=4Bh EXEC; reset is the caller's
 * concern (DOS clears it once read, but we keep the simpler "last value" model
 * this milestone -- the shell reads it once right after EXEC). */
static uint8_t g_last_child_rc = 0;

/* The MCB memory arena behind AH=48h/49h/4Ah (beads initech-509.6). g_arena
 * walks the kernel's program-memory region (PROGRAM_BASE..PROGRAM_ALLOC_END,
 * spec/memory_map.h); g_arena_base_linear is that region's flat base so a
 * returned data-paragraph index converts to a DOS segment as
 * (g_arena_base_linear >> 4) + data_para. g_arena_bound gates the seam: until
 * the kernel (or host oracle) binds an arena, 48h/49h/4Ah report insufficient
 * memory rather than fault (Rule 2). The arena is bound once at SYSINIT and
 * RE-INITIALIZED per program load (int21_mcb_reset) so each program starts
 * owning its whole window -- the authentic single-big-block. */
static mcb_arena_t g_arena;
static uint32_t    g_arena_base_linear = 0u;
static int         g_arena_bound       = 0;

/* The current Disk Transfer Area (flat ptr). FINDFIRST/FINDNEXT write the
 * 43-byte find_data_t here. Defaults to the current PSP's command-tail field
 * (PSP:0x80, DTA_DEFAULT_PSP_OFFSET) when zero -- the real-DOS default. AH=1Ah
 * SETDTA sets it; AH=2Fh GETDTA returns it (beads initech-509.5). */
static uint32_t g_dta = 0;

/* The single active FINDFIRST/FINDNEXT search state (one search at a time this
 * milestone -- no reentrant concurrent searches; fs-mount-sft-ground-truth.md
 * Sec 4.5). pattern is the 11-byte 8.3 search template ('?' = wildcard byte);
 * next_index is the backend dir-entry index to examine next; active gates
 * FINDNEXT (a FINDNEXT with no prior FINDFIRST returns no-more-files). */
typedef struct find_state {
    uint8_t  pattern[11];   /* 8.3 search template; '?' (0x3F) matches any byte */
    uint8_t  search_attr;   /* attribute mask from FINDFIRST ECX                */
    uint32_t next_index;    /* next backend dir-entry index to check            */
    uint16_t dir_start;     /* directory enumerated (0 == root; beads mzxa)     */
    uint8_t  active;        /* 1 once FINDFIRST has run                          */
} find_state_t;
static find_state_t g_find;

/* InDOS depth (beads initech-xk2; period-authentic DOS InDOS flag). Incremented
 * at int21_dispatch / int20_dispatch entry, decremented at exit, so it counts
 * the NESTING level of INT 21h calls in flight. Real DOS exposes the InDOS flag
 * (a byte) so a TSR/driver/ISR can poll it and DEFER its OWN INT 21h call while
 * InDOS != 0 (DOS INT 21h is not reentrant; DOS 3.3 internals). InitechDOS
 * mirrors that contract via dos_in_dos(): a future ISR/TSR/driver MUST check
 * dos_in_dos() before issuing an INT 21h call and defer if set. NOTHING checks
 * it yet (the IRQ handlers issue no syscalls -- the irq.c depth guard ENFORCES
 * that); this is the documented hook + the counter, nothing more. EXEC's
 * synchronous child syscalls legitimately nest, so this is a DEPTH counter, not
 * a 0/1 flag. Ref: irq.h; DOS 3.3 InDOS flag. */
static volatile uint32_t g_indos = 0u;

/* The InDOS depth (0 == no INT 21h call in flight). A future ISR/TSR/driver
 * polls this before issuing INT 21h and defers while it is non-zero (the real
 * DOS InDOS-flag contract). volatile read; not the IRQ guard (that is irq.h's
 * irq_depth() -- a different, stricter check that fails loud). */
int dos_in_dos(void)
{
    return g_indos != 0u ? 1 : 0;
}

#ifdef INT21_IRQTEST_SEAM
/* MUTANT-ONLY test seam (CLAUDE.md Rule 6; compiled ONLY into the mutant-A
 * irqstorm image via -DINT21_IRQTEST_SEAM). It lets the mutant PIT ISR
 * (pit.c -DPIT_MUTATE_SCRIBBLE_DOS) reach a DOS dispatcher global so we can PROVE
 * the storm oracle detects async shared-state corruption: bumping g_find.next_index
 * mid-enumeration makes FINDNEXT skip an entry, so the directory listing comes
 * back WRONG and test-int21-irqstorm goes RED. NEVER compiled into a real build
 * (the flag is set only by the mutant image rule). Touches a dispatcher global
 * EXACTLY as a forbidden ISR<->DOS sharing would. */
void int21_irqtest_bump_find(void)
{
    g_find.next_index++;
}
#endif

void int21_set_sink(int21_sink_fn sink) { g_sink = sink; }
void int21_set_exit(int21_exit_fn fn)   { g_exit = fn; }
void int21_set_psp(struct psp *psp)     { g_cur_psp = (psp_t *)psp; }
void int21_set_file_backend(const int21_file_backend_t *backend) { g_file = backend; }

/* ---- CWD seam (beads initech-mzxa; ti8 Layer 2, READ side) ----
 * Reset the current working directory to the root. The loader calls this on
 * each program launch (mirroring int21_mcb_reset) so a freshly loaded program
 * always starts at the root (the simplest authentic ti8 model: a child does not
 * inherit the parent's CWD -- real DOS gives a child its own COPY, but with no
 * CHDIR writer yet every CWD is the root regardless). do_terminate calls it too
 * so a child's CWD never lingers into kernel context. */
void int21_cwd_reset(void)
{
    g_cwd_start_cluster = 0u;
    g_cwd_path[0]       = '\0';
}

/* Snapshot / restore the CWD around a kernel-context PSP rebind (kmain restores
 * its own PSP after a child run; without saving the CWD a child's directory
 * would leak into kernel-context INT 21h). The opaque snapshot carries both the
 * start cluster and the path text. Mirrors the save/restore the kernel already
 * does for the exit hook + PSP. */
int21_cwd_snapshot_t int21_cwd_save(void)
{
    int21_cwd_snapshot_t s;
    s.start_cluster = g_cwd_start_cluster;
    for (uint32_t i = 0u; i < INT21_CWD_MAX; i++) {
        s.path[i] = g_cwd_path[i];
    }
    return s;
}

void int21_cwd_restore(const int21_cwd_snapshot_t *s)
{
    if (s == 0) {
        return;
    }
    g_cwd_start_cluster = s->start_cluster;
    for (uint32_t i = 0u; i < INT21_CWD_MAX; i++) {
        g_cwd_path[i] = s->path[i];
    }
}
void int21_set_exec_backend(int21_exec_fn fn) { g_exec = fn; }

/* ---- MCB arena seam (beads initech-509.6; AH=48h/49h/4Ah) ----
 * Bind the arena buffer + its flat base address. base==NULL or total_paras<2
 * (no room for a header + at least one data paragraph) leaves the arena UNBOUND
 * so the memory functions report insufficient-memory rather than fault (Rule 2).
 * Lays the initial single terminal FREE block (mcb_init); int21_mcb_reset then
 * (re)assigns the owner to the current PSP on each program load. */
void int21_set_mcb_arena(void *base, uint32_t total_paras, uint32_t base_linear)
{
    if (base == 0 || total_paras < 2u) {
        g_arena.base        = 0;
        g_arena.total_paras = 0u;
        g_arena_base_linear = 0u;
        g_arena_bound       = 0;
        return;
    }
    mcb_init(&g_arena, base, total_paras);
    g_arena_base_linear = base_linear;
    g_arena_bound       = 1;
}

/* The current PSP as a DOS owner id = its fake-paragraph segment (linear >> 4),
 * the SAME value AH=62h GET PSP returns in BX. Never MCB_OWNER_FREE(0) for a
 * live PSP (PROGRAM_BASE >> 4 != 0); with no PSP bound, 0 -- but the handlers
 * gate on g_cur_psp before allocating, so a 0 owner never reaches the arena. */
static uint16_t cur_psp_owner(void)
{
    if (g_cur_psp == 0) {
        return 0u;
    }
    return (uint16_t)(((uintptr_t)g_cur_psp >> 4) & 0xFFFFu);
}

int int21_mcb_reset(void)
{
    if (!g_arena_bound) {
        return 0;
    }
    /* Re-lay one terminal block over the whole arena, then hand the whole window
     * to the current program (the authentic single-big-block a .COM owns at
     * load). A program that wants a heap shrinks this block (4Ah) then allocs
     * from the freed tail (48h). If no PSP is bound yet, leave it FREE. */
    mcb_init(&g_arena, g_arena.base, g_arena.total_paras);
    uint16_t owner = cur_psp_owner();
    if (owner != MCB_OWNER_FREE) {
        (void)mcb_set_arena_owner(&g_arena, owner);
    }
    return 1;
}
void int21_set_conin(int21_conin_fn get, int21_coninpoll_fn poll)
{
    g_conin_get  = get;
    g_conin_poll = poll;
}
void int21_set_clock(int21_clock_get_fn get, int21_clock_set_fn set)
{
    g_clock_get = get;
    g_clock_set = set;
}
void int21_set_vectortable(int21_setvect_fn set, int21_getvect_fn get)
{
    g_setvect = set;
    g_getvect = get;
}

/* ---- CF helpers: write ONLY bit 0 of the saved EFLAGS image. ---- */
static void cf_clear(int_frame_t *f) { f->eflags &= ~CF_BIT; }
static void cf_set(int_frame_t *f)   { f->eflags |=  CF_BIT; }

/* ---- ZF helpers: write ONLY bit 6 of the saved EFLAGS image (06h/0Bh). ---- */
static void zf_clear(int_frame_t *f) { f->eflags &= ~ZF_BIT; }
static void zf_set(int_frame_t *f)   { f->eflags |=  ZF_BIT; }

/* ---- CON input source access (NULL-safe) ----
 * conin_get: BLOCKING read of one char (0..255). With no source bound it
 * returns 0 -- a graceful no-input result, NEVER a hang (Rule 2). conin_poll: a
 * non-blocking read; returns the char (0..255) or -1 when none / no source. */
static int conin_get(void)
{
    if (g_conin_get) {
        return g_conin_get() & 0xFF;
    }
    return 0;   /* no source bound -> no input (host-safe default) */
}

static int conin_poll(void)
{
    if (g_conin_poll) {
        return g_conin_poll();
    }
    return -1;  /* no source bound -> no char */
}

/* ---- the CON sink fan-out (NULL-safe) ---- */
static void con_putc(char c)
{
    if (g_sink) {
        g_sink(c);
    }
}

static void con_puts(const char *s)
{
    while (*s) {
        con_putc(*s++);
    }
}

/* Emit `v` as two ASCII hex nibbles (uppercase). Deterministic, ASCII, no libc
 * (Rule 11/12). Used by the controlled-scope diagnostics ("AH=NN"). */
static void con_hex2(uint8_t v)
{
    static const char H[] = "0123456789ABCDEF";
    con_putc(H[(v >> 4) & 0xFu]);
    con_putc(H[v & 0xFu]);
}

/* ---- low/high-byte accessors on the saved EAX ---- */
static uint8_t  frame_al(const int_frame_t *f) { return (uint8_t)(f->eax & 0xFFu); }
static uint8_t  frame_dl(const int_frame_t *f) { return (uint8_t)(f->edx & 0xFFu); }

/* Set AL (low byte) without disturbing the rest of EAX. */
static void set_al(int_frame_t *f, uint8_t al)
{
    f->eax = (f->eax & 0xFFFFFF00u) | (uint32_t)al;
}

/* Set the low 16 bits (AX); used for error codes (DOS returns AX=error). */
static void set_ax(int_frame_t *f, uint16_t ax)
{
    f->eax = (f->eax & 0xFFFF0000u) | (uint32_t)ax;
}

/* 16-bit (CX/DX/BX) writers + the high/low byte readers the date/time/space/PSP
 * functions need (AH=2Ah-2Dh/36h/62h). Each writes ONLY the low 16 bits of the
 * register so the rest of the dword (and the upper EAX) is left intact, matching
 * the real-DOS 16-bit register contract on a 32-bit flat frame. */
static void set_cx(int_frame_t *f, uint16_t cx)
{
    f->ecx = (f->ecx & 0xFFFF0000u) | (uint32_t)cx;
}
static void set_dx(int_frame_t *f, uint16_t dx)
{
    f->edx = (f->edx & 0xFFFF0000u) | (uint32_t)dx;
}
static void set_bx(int_frame_t *f, uint16_t bx)
{
    f->ebx = (f->ebx & 0xFFFF0000u) | (uint32_t)bx;
}
static uint16_t frame_cx(const int_frame_t *f) { return (uint16_t)(f->ecx & 0xFFFFu); }
static uint8_t  frame_dh(const int_frame_t *f) { return (uint8_t)((f->edx >> 8) & 0xFFu); }
static uint8_t  frame_ch(const int_frame_t *f) { return (uint8_t)((f->ecx >> 8) & 0xFFu); }
static uint8_t  frame_cl(const int_frame_t *f) { return (uint8_t)(f->ecx & 0xFFu); }

/* ---- controlled scope: is this AH sanctioned by spec/int21h_register.json? --
 * The locked register groups several functions into ranges (e.g. "01h-0Ch CON
 * I/O", "0Fh-24h FCB ops"). We recognize the EXACT set of single AHs + ranges
 * the register lists. An AH NOT here is UNKNOWN (-> "unknown" diagnostic). An AH
 * here that this subset has not yet implemented is RECOGNIZED-but-deferred (->
 * "not-yet-impl" diagnostic). Source: spec/int21h_register.json (verbatim). */
static int ah_is_listed(uint8_t ah)
{
    /* Single-AH entries from the register. */
    switch (ah) {
        case 0x00: /* TERMINATE */
        case 0x0E: case 0x19:            /* SELDISK / GETDISK */
        case 0x1A: case 0x2F:            /* SETDTA / GETDTA */
        case 0x25: case 0x35:            /* SETVECT / GETVECT */
        case 0x30:                       /* GETVER */
        case 0x31:                       /* KEEP (TSR) */
        case 0x36:                       /* GETSPACE */
        case 0x39: case 0x3A: case 0x3B: /* MKDIR / RMDIR / CHDIR */
        case 0x3C:                       /* CREAT */
        case 0x3D:                       /* OPEN */
        case 0x3E:                       /* CLOSE */
        case 0x3F:                       /* READ */
        case 0x40:                       /* WRITE */
        case 0x41:                       /* UNLINK */
        case 0x42:                       /* LSEEK */
        case 0x43:                       /* CHMOD */
        case 0x44:                       /* IOCTL */
        case 0x45: case 0x46:            /* DUP / DUP2 */
        case 0x47:                       /* GETCWD */
        case 0x48: case 0x49: case 0x4A: /* ALLOC / FREE / SETBLOCK */
        case 0x4B:                       /* EXEC */
        case 0x4C:                       /* EXIT */
        case 0x4D:                       /* WAIT */
        case 0x4E: case 0x4F:            /* FINDFIRST / FINDNEXT */
        case 0x56:                       /* RENAME */
        case 0x57:                       /* FILETIME */
        case 0x59:                       /* GETERR */
        case 0x5B:                       /* CREATNEW */
        case 0x62:                       /* GETPSP */
            return 1;
        default:
            break;
    }
    /* Range entries from the register. */
    if (ah >= 0x01 && ah <= 0x0C) return 1; /* CON I/O */
    if (ah >= 0x2A && ah <= 0x2D) return 1; /* DATE / TIME */
    if (ah >= 0x0F && ah <= 0x24) return 1; /* FCB ops (Legacy) */
    return 0;
}

/* ---- the console-output subset implementations ---- */

/* AH=02h DISPLAY OUTPUT: DL = char -> CON. DOS returns AL = last char. CF clear. */
static void do_putchar(int_frame_t *f)
{
    uint8_t c = frame_dl(f);
    con_putc((char)c);
    set_al(f, c);   /* DOS convention: AL = the character written */
    cf_clear(f);
}

/* Upper bound on the AH=09h '$'-scan (bcg.5). A near pointer cannot span more
 * than one 64 KiB segment in period DOS, so a string with no '$' within this
 * many bytes is malformed; stop rather than walk off into memory (Rule 2). */
#define INT21_PUTS_SCAN_MAX 65536u

/* AH=09h DISPLAY STRING: EDX -> flat ptr to a '$'-terminated string -> CON.
 * The '$' (0x24) is the terminator and is NOT emitted (DOS 3.3 convention,
 * ground-truth Sec 6.2). Returns AL='$'. CF clear. */
static void do_puts(int_frame_t *f)
{
    const char *p = (const char *)(uintptr_t)f->edx;
    uint32_t scanned = 0u;

    /* Guard the EDX walk (bcg.5; the DEC-14 fail-loud class for buffer-taking
     * calls): a NULL pointer must not dereference address 0, and an unterminated
     * string must not run away. NULL -> emit nothing and return AL='$'. */
    if (p == 0) {
        set_al(f, 0x24u);
        cf_clear(f);
        return;
    }

    for (;;) {
        char c = *p++;
        if (c == '$') {
#ifdef INT21_MUTATE_PUTS_EMIT_DOLLAR
            /* MUTANT (Rule 6; make test-int21-mutant only): emit the '$' too
             * before breaking, so the PUTS oracle (which expects "HELLO", not
             * "HELLO$") goes RED. NEVER define in a real build. */
            con_putc('$');
#endif
            break;
        }
        con_putc(c);
        if (++scanned >= INT21_PUTS_SCAN_MAX) {
            break;   /* unterminated string -> stop before walking off memory */
        }
    }
    set_al(f, 0x24u);
    cf_clear(f);
}

/* ---- the CON-input subset (beads initech-n62) -----------------------------
 * All read through the CON input source (conin_get blocking / conin_poll non-
 * blocking) and echo through the CON sink, mirroring DOS 3.3 semantics. Ctrl-C
 * (^C, 0x03) handling -- the SET BREAK / INT 23h check that 01h/08h/0Ah perform
 * in real DOS -- is DEFERRED (beads to file): there is no break handler or
 * process model yet, so ^C is delivered as an ordinary character this subset.
 * Ref: DOS 3.3 Programmer's Reference Manual AH=01h/06h/07h/08h/0Ah/0Bh/0Ch.
 *
 * THE 0Bh PUSHBACK: AH=0Bh GET INPUT STATUS must report whether a char is ready
 * WITHOUT consuming it -- but the only non-blocking primitive (conin_poll)
 * consumes. So a peeked char is held in g_conin_pushback (a single-slot lookahead)
 * and the read paths (conin_get_pb / conin_poll_pb) drain it FIRST, so a 0Bh
 * status check followed by 01h/07h/08h/0Ah still sees that keystroke. */
static int g_conin_pushback = -1;   /* a single peeked-but-unconsumed char */

/* Blocking get honoring the 0Bh pushback. */
static int conin_get_pb(void)
{
    if (g_conin_pushback >= 0) {
        int c = g_conin_pushback;
        g_conin_pushback = -1;
        return c & 0xFF;
    }
    return conin_get();
}

/* Non-blocking poll honoring the 0Bh pushback (06h-input / 0Bh / 0Ch drain). */
static int conin_poll_pb(void)
{
    if (g_conin_pushback >= 0) {
        int c = g_conin_pushback;
        g_conin_pushback = -1;
        return c & 0xFF;
    }
    return conin_poll();
}

/* AH=01h CHARACTER INPUT WITH ECHO: block for one char, echo it to CON, AL=char.
 * (Ctrl-C check deferred -- see header note.) CF is not part of this function's
 * contract; success leaves it clear. Ref: DOS 3.3 PRM AH=01h. */
static void do_conin_echo(int_frame_t *f)
{
    int c = conin_get_pb();
#ifndef INT21_MUTATE_CONIN_NO_ECHO
    con_putc((char)c);          /* echo */
#else
    /* MUTANT (Rule 6; make test-conin-mutant only): drop the echo, so the 01h
     * oracle -- which asserts the char appears on the CON sink -- goes RED.
     * NEVER define in a real build. */
#endif
    set_al(f, (uint8_t)c);
    cf_clear(f);
}

/* AH=07h DIRECT CHARACTER INPUT, NO ECHO, NO Ctrl-C: block for one char, AL=char,
 * no echo. Ref: DOS 3.3 PRM AH=07h. */
static void do_conin_raw(int_frame_t *f)
{
    int c = conin_get_pb();
    set_al(f, (uint8_t)c);
    cf_clear(f);
}

/* AH=08h CHARACTER INPUT, NO ECHO (with Ctrl-C check): block for one char,
 * AL=char, no echo. The ^C check is deferred (header note); functionally
 * identical to 07h this subset. Ref: DOS 3.3 PRM AH=08h. */
static void do_conin_noecho(int_frame_t *f)
{
    int c = conin_get_pb();
    set_al(f, (uint8_t)c);
    cf_clear(f);
}

/* AH=06h DIRECT CONSOLE I/O (dual): DL=0xFF -> INPUT (non-blocking, no echo);
 * DL!=0xFF -> OUTPUT DL to CON. On input: if a char is ready, ZF=0 (clear) and
 * AL=char; if none, ZF=1 (set) and AL=0, with NO wait and NO echo. On output,
 * the char goes to CON and AL=DL (DOS leaves ZF undefined on the output leg; we
 * clear it for determinism). Ref: DOS 3.3 PRM AH=06h. */
static void do_direct_conio(int_frame_t *f)
{
    uint8_t dl = frame_dl(f);
    if (dl == 0xFFu) {
        /* INPUT direction: non-blocking poll (honor any 0Bh pushback). */
        int c = conin_poll_pb();
        if (c < 0) {
            set_al(f, 0u);
            zf_set(f);          /* no character available */
        } else {
            set_al(f, (uint8_t)c);
            zf_clear(f);        /* a character was returned in AL */
        }
        cf_clear(f);
    } else {
        /* OUTPUT direction: emit DL to CON (this is the 06h output leg; the
         * dual of the input above). */
        con_putc((char)dl);
        set_al(f, dl);
        zf_clear(f);
        cf_clear(f);
    }
}

/* AH=0Bh GET INPUT STATUS: AL=0xFF if a character is available, AL=0x00 if not.
 * No wait, no consume -- a peeked char is parked in the pushback so a following
 * read still sees it. Ref: DOS 3.3 PRM AH=0Bh. */
static void do_input_status(int_frame_t *f)
{
    int avail;
    if (g_conin_pushback >= 0) {
        avail = 1;                 /* already have one queued */
    } else {
        int c = conin_poll();
        if (c >= 0) {
            g_conin_pushback = c & 0xFF;  /* park it -- do NOT consume */
            avail = 1;
        } else {
            avail = 0;
        }
    }
    set_al(f, avail ? 0xFFu : 0x00u);
    cf_clear(f);
}

/* AH=0Ah BUFFERED INPUT: EDX -> a buffer where byte0 (caller-set) is the maximum
 * length INCLUDING the terminating CR, byte1 is the count WE write (chars read,
 * NOT counting the CR), and byte2.. are the chars, terminated by a CR (0x0D)
 * which IS stored but NOT counted. We loop blocking-reads, echoing each char;
 * BACKSPACE (0x08) erases the last buffered char (emit "\b \b") or is ignored
 * when empty; CR (0x0D) stops the loop (stored + echoed, then a LF). When the
 * buffer is full (count == max-1, leaving room only for the CR) further non-CR
 * chars are ignored (we emit a BEL, do NOT overflow -- Rule 2).
 * Ref: DOS 3.3 PRM AH=0Ah. */
static void do_buffered_input(int_frame_t *f)
{
    uint8_t *buf = (uint8_t *)(uintptr_t)f->edx;
    if (buf == 0) {
        cf_clear(f);            /* nothing we can do; fail safe (no fault) */
        return;
    }

    uint8_t max = buf[0];       /* max length incl. the CR (caller-set) */
    uint8_t count = 0;          /* chars stored so far (excl. CR) */

    /* A max of 0 is degenerate (no room even for the CR); store nothing. Real
     * DOS still reads until CR but cannot store; we mirror "no room" by reading
     * to the CR and storing none. max==1 leaves room only for the CR. */
    for (;;) {
        int ci = conin_get_pb();
        uint8_t c = (uint8_t)ci;

        /* Line terminator: the DOS Enter key is CR (0x0D). The PS/2 driver
         * (kbd.c) decodes the Enter scancode (0x1C) to CR (0x0D) directly -- the
         * DOS/BIOS INT 16h convention -- so the ONLY terminator here is CR. The
         * former "also accept LF and normalize to CR" path was a bandaid for the
         * kbd emitting LF; it is retired now that the root cause is fixed
         * (initech-62m, Rule 3). A stray LF is just an ordinary stored char.
         * Ref: DOS 3.3 PRM AH=0Ah; os/milton/kbd.c SC1_NORMAL[0x1C] == '\r'. */
        if (c == 0x0Du) {                       /* CR: terminate the line */
            /* Store the CR if there is room (real DOS always stores it at
             * buf[2+count]); the CR is NOT counted in buf[1]. */
            /* Room for the CR? `count` is already capped at max-1 by the
             * ordinary-char path, so this holds for any max>=1. The previous
             * (uint8_t)(2u+count) < (uint8_t)(2u+max) form WRAPPED when max>=254
             * (2u+max overflows uint8_t) and wrongly dropped the terminator on a
             * full-size buffer -- buf[0]=255 is a legal caller request. */
#ifdef INT21_MUTATE_BUFINPUT_CR_WRAP
            /* MUTANT (Rule 6; make test-conin-mutant only): restore the old
             * uint8_t-wrapping guard so a max=255 buffer drops its CR -- the
             * full-size-buffer oracle must go RED. NEVER define in a real build. */
            if ((uint8_t)(2u + count) < (uint8_t)(2u + max)) {
#else
            if (count < max) {
#endif
                buf[2u + count] = 0x0Du;
            }
#ifdef INT21_MUTATE_BUFINPUT_COUNT_CR
            /* MUTANT (Rule 6; make test-conin-mutant only): count the CR in the
             * length byte (count+1 instead of count), so the 0Ah oracle -- which
             * asserts buf[1] == chars read NOT counting the CR -- goes RED.
             * NEVER define in a real build. */
            buf[1] = (uint8_t)(count + 1u);
#else
            buf[1] = count;
#endif
            con_putc('\r');                     /* echo CR + LF (DOS convention) */
            con_putc('\n');
            cf_clear(f);
            return;
        }

        if (c == 0x08u) {                       /* BACKSPACE: erase one */
            if (count > 0u) {
                count--;
                con_putc('\b');                 /* erase visually: back, space, back */
                con_putc(' ');
                con_putc('\b');
            }
            /* empty -> ignore (DOS does not back past the prompt) */
            continue;
        }

        /* Ordinary char. Room for it only if count < max-1 (reserve 1 for CR). */
        if (max >= 1u && count < (uint8_t)(max - 1u)) {
            buf[2u + count] = c;
            count++;
            con_putc((char)c);                  /* echo */
        } else {
            /* Buffer full: ignore further non-CR chars, beep (BEL). Rule 2: do
             * NOT overflow the caller's buffer. */
            con_putc('\a');
        }
    }
}

/* AH=0Ch FLUSH KEYBOARD BUFFER then invoke an input function: AL on entry names
 * the input function to chain (01h/06h/07h/08h/0Ah). Drain any pending input
 * (so a stale keystroke cannot satisfy the chained read), then dispatch. If AL
 * is not one of the chainable functions, just flush and return (CF clear).
 * Ref: DOS 3.3 PRM AH=0Ch. */
static void do_flush_then_input(int_frame_t *f)
{
    /* Flush: drain the pushback + every char the poll can yield right now. */
    g_conin_pushback = -1;
    while (conin_poll() >= 0) {
        /* discard */
    }

    uint8_t sub = frame_al(f);
    /* Re-dispatch by replacing AH with the chained function and clearing AL's
     * role (the sub-functions read AH only, plus 06h reads DL / 0Ah reads EDX,
     * which the caller already set up). We call the implementations directly. */
    switch (sub) {
        case 0x01: do_conin_echo(f);      return;
        case 0x06: do_direct_conio(f);    return;
        case 0x07: do_conin_raw(f);       return;
        case 0x08: do_conin_noecho(f);    return;
        case 0x0A: do_buffered_input(f);  return;
        default:
            /* Not a chainable input function: flush-only, success. */
            cf_clear(f);
            return;
    }
}

/* Validate a user-supplied INT 21h buffer [ptr, ptr+count) BEFORE any access.
 * Returns 1 if safe, 0 if the call must fail loud (caller sets CF=1,
 * AX=0x0009). A bad caller pointer -- NULL, or a count that wraps the 32-bit
 * flat space -- is rejected rather than dereferenced/scribbled (Rule 2;
 * ADR-0003 DEC-14 / beads initech-tzq). A zero count never touches memory and
 * always passes (the DOS contract). Scope is NULL + 32-bit wrap: both are
 * meaningful on the flat target AND exercisable by the host unit tests (whose
 * buffers live at host addresses, so a fixed arena-ceiling check would reject
 * valid host buffers -- target-only, out of scope per DEC-14.1). */
static int user_buf_ok(uint32_t ptr, uint32_t count)
{
#ifdef INT21_MUTATE_NO_PTR_GUARD
    /* MUTANT (Rule 6; make test-int21-edge-mutant only): disable the guard so a
     * NULL/wrapping buffer is dereferenced -- a NULL read of a non-empty file
     * then SIGSEGVs (the exact fault the guard prevents) and the oracle goes
     * RED. NEVER define in a real build. */
    (void)ptr; (void)count;
    return 1;
#else
    if (count == 0u)       return 1;   /* no memory access */
    if (ptr == 0u)         return 0;   /* NULL buffer */
    if (ptr + count < ptr) return 0;   /* 32-bit wrap (count overflow) */
    return 1;
#endif
}

/* AH=40h WRITE TO FILE/DEVICE: EBX=handle, ECX=count, EDX=flat ptr. The handle
 * is resolved through the current process's JFT into the system SFT (beads
 * initech-509.3): a CON device entry writes to the console (so handles 1/2 --
 * and anything DUP2'd onto the CON-write slot -- still go to CON). Writing to
 * AUX/PRN (no driver yet) or to a FILE (FAT write deferred to 509.11) returns
 * CF=1, AX=0x0005 (access denied). An out-of-range/closed handle -> CF=1,
 * AX=0x0006 (invalid handle). Success: EAX = bytes written, CF clear. */
static void do_write(int_frame_t *f)
{
    uint32_t handle = f->ebx;
    uint32_t count  = f->ecx;
    const char *buf = (const char *)(uintptr_t)f->edx;

    sft_entry_t *e = (handle <= 0xFFu)
                       ? sft_from_handle(g_cur_psp, (uint8_t)handle)
                       : 0;
    if (e == 0) {
        /* No such open handle (out of range / closed / no process). Rule 2. */
        set_ax(f, INT21_ERR_INVALID_HANDLE);
        cf_set(f);
        return;
    }

    /* Validate the source buffer before either the CON fan-out or the FILE
     * backend dereferences it (ADR-0003 DEC-14 / initech-tzq). */
    if (!user_buf_ok(f->edx, count)) {
        set_ax(f, INT21_ERR_INVALID_MEMORY);
        cf_set(f);
        return;
    }

    /* CON device write: fan the bytes to the console sink. CON is the screen
     * regardless of the slot's nominal mode (DOS treats CON as writable). */
    if (e->kind == SFT_KIND_DEVICE && e->dev_id == SFT_DEV_CON) {
        for (uint32_t i = 0; i < count; i++) {
            con_putc(buf[i]);
        }
        f->eax = count;   /* full EAX = bytes written */
        cf_clear(f);
        return;
    }

    /* FILE write (beads initech-0qh): POSITIONED write of the bytes at the
     * per-handle file_offset over the cluster chain (backend write_at via
     * fat12_write_partial -- overwrite/extend/zero-fill-hole, committed to disk
     * per call). The handle must have been opened for write (CREAT). A read-mode
     * FILE handle or a missing write backend -> access denied (Rule 2). The
     * backend returns the UPDATED dir entry so we refresh the SFT copy's size +
     * start_cluster, then advance the position. */
    if (e->kind == SFT_KIND_FILE) {
        /* AH=3Dh AL=2 (RDWR) must permit writing, not only AL=1 (WRITE) per the
         * DOS 3.3 PRM (bcg.1). A read-only (AL=0) handle or a missing write
         * backend -> access denied (Rule 2). */
        int writable = (e->open_mode == SFT_MODE_WRITE ||
                        e->open_mode == SFT_MODE_RDWR);
        if (!writable || g_file == 0 || g_file->write_at == 0) {
            set_ax(f, INT21_ERR_ACCESS_DENIED);
            cf_set(f);
            return;
        }
        uint32_t written = 0u;
        dir_entry_t updated = e->dir_entry;
        uint16_t err = g_file->write_at(e->root_slot, e->file_offset,
                                        (const uint8_t *)buf, count,
                                        &written, &updated);
        if (err != 0) {
            set_ax(f, err);
            cf_set(f);
            return;
        }
        e->dir_entry    = updated;        /* refresh size + start_cluster */
        e->file_offset += written;        /* advance the per-handle position */
        f->eax = written;                 /* EAX = bytes written */
        cf_clear(f);
        return;
    }

    /* AUX/PRN devices have no driver yet -> AX=0x0005 (access denied), CF=1
     * (Rule 2: never silently drop the bytes). */
    set_ax(f, INT21_ERR_ACCESS_DENIED);
    cf_set(f);
}

/* AH=45h DUP: duplicate handle EBX into the lowest free JFT slot; the new
 * handle aliases the same SFT entry. Returns EAX = new handle, CF clear; or
 * CF=1, AX=error (0x0006 invalid src, 0x0004 too many open). Ref: DOS 3.3
 * Programmer's Reference Manual AH=45h; fs-mount-sft-ground-truth.md Sec 3.4. */
static void do_dup(int_frame_t *f)
{
    uint8_t src = (uint8_t)(f->ebx & 0xFFu);
    uint8_t newh = 0;
    uint16_t rc = sft_dup(g_cur_psp, src, &newh);
    if (rc != SFT_OK) {
        set_ax(f, rc);
        cf_set(f);
        return;
    }
    f->eax = (f->eax & 0xFFFFFF00u) | (uint32_t)newh; /* EAX (AL) = new handle */
    cf_clear(f);
}

/* AH=46h DUP2: force handle ECX to alias handle EBX (the I/O-redirection
 * primitive: DUP2(EBX=file, ECX=1) repoints stdout at the file). Returns CF
 * clear on success; CF=1, AX=0x0006 on a bad src/dst. Ref: DOS 3.3 Programmer's
 * Reference Manual AH=46h; fs-mount-sft-ground-truth.md Sec 3.4. */
static void do_dup2(int_frame_t *f)
{
    uint8_t src = (uint8_t)(f->ebx & 0xFFu);
    uint8_t dst = (uint8_t)(f->ecx & 0xFFu);
    uint16_t rc = sft_dup2(g_cur_psp, src, dst);
    if (rc != SFT_OK) {
        set_ax(f, rc);
        cf_set(f);
        return;
    }
    cf_clear(f);
}

/* ---- file-handle functions (beads initech-509.5 read-side) ---------------- *
 * All resolve a process handle through g_cur_psp->jft into g_sft (the SFT), and
 * reach the mounted volume through g_file (the backend vtable). DEC-04a ABI:
 * AH=func, EBX=handle, ECX=count, EDX=flat ptr, EAX=return, CF=error.
 * Ref: docs/research/fs-mount-sft-ground-truth.md Sec 4; DOS 3.3 PRM AH=3Dh/3Eh/
 * 3Fh/42h/4Eh/4Fh/1Ah/2Fh. */

/* Reject a path the milestone cannot support at a ROOT-ONLY site: a subdirectory
 * separator '\' or a drive-letter ':' (do_exec is still root-only -- the loader
 * load path reads the located program's own start_cluster but cannot yet locate
 * a program inside a subdir; beads initech-mzxa decision 3, follow-up initech-zs24).
 * The file/find sites (OPEN/CREAT/UNLINK/FINDFIRST) NO LONGER call this -- they
 * resolve subdir paths via resolve_dir_path() below -- but the helper stays as
 * do_exec's root-only gate AND as the overlength-path runaway guard.
 * Returns 1 if the path is rejectable (caller sets CF=1, AX=0x0003). */
/* A runaway guard, NOT a DOS path-length policy: bounds the scan of a possibly
 * malformed / unterminated ASCIIZ pointer so it can never walk the kernel off
 * into memory (Rule 2 -- fail loud, do not hang). Well above DOS's 64-byte path
 * maximum, so it never rejects a legal path; an overlength scan (no NUL within
 * the bound) is treated as rejectable, same as an illegal path character. */
#define INT21_PATH_SCAN_MAX 128u
static int path_has_subdir_or_drive(const char *p)
{
    uint32_t n = 0u;
    (void)n;
    for (; *p; p++) {
        if (*p == '\\' || *p == ':') {
            return 1;
        }
#ifndef INT21_MUTATE_PATHSCAN_NOBOUND
        /* MUTANT (Rule 6; make test-exec-mutant only) removes this bound so an
         * overlength path is NOT rejected and reaches the backend -- the
         * overlength oracle must go RED. NEVER define in a real build. */
        if (++n >= INT21_PATH_SCAN_MAX) {
            return 1;           /* no terminator within bound -> reject */
        }
#endif
    }
    return 0;
}

/* Bound-check an ASCIIZ path for the runaway guard ONLY (no '\' / ':' rejection):
 * a malformed / unterminated pointer with no NUL within INT21_PATH_SCAN_MAX is
 * rejectable so a resolve never walks the kernel off into memory (Rule 2 -- the
 * SAME fail-loud bound path_has_subdir_or_drive enforces, preserved for the
 * resolving file/find sites). Returns 1 if overlength (caller -> CF=1, AX=0x0003).
 * The INT21_MUTATE_PATHSCAN_NOBOUND seam removes the bound so the overlength
 * oracle goes RED (shared with the do_exec gate above). */
static int path_overlength(const char *p)
{
    uint32_t n = 0u;
    (void)n;
    for (; *p; p++) {
#ifndef INT21_MUTATE_PATHSCAN_NOBOUND
        if (++n >= INT21_PATH_SCAN_MAX) {
            return 1;           /* no terminator within bound -> reject */
        }
#endif
    }
    return 0;
}

/* ---- the path -> containing-directory resolve seam (beads initech-mzxa) ---- *
 * Resolve `path` (a possibly '\SUB\FILE'-qualified 8.3 path) to its CONTAINING
 * directory's first data cluster (*out_dir_start, 0 == root) and the bare final
 * 8.3 component (*out_leaf, a pointer INTO `path`). Returns 0 on success, or
 * INT21_ERR_PATH_NOT_FOUND (0x0003) when a non-final component is missing/not a
 * directory, or the path is overlength, or no backend resolve is bound and the
 * path is not a bare root-relative name. int21.c never includes fat12.h: the
 * backend resolve member (fileio_fat.c -> fat12_resolve_path / fat12_read_dir)
 * owns the cluster math; int21.c owns only the leading-drive strip + the wiring.
 *
 * A leading 'X:' drive prefix is stripped HERE (the drive selects the volume --
 * a DOS-API concern; this milestone has one volume so the letter is ignored).
 * The INT21_MUTATE_RESOLVE_NODRIVE seam skips that strip so an 'A:'-prefixed
 * path reaches the backend resolve with the 'A:' intact -> it cannot parse ->
 * 0x0003 (the "leading A: is stripped + succeeds" oracle goes RED). The
 * INT21_MUTATE_RESOLVE_NOTROOT seam forces *out_dir_start back to 0 (root) after
 * a successful resolve so a '\SUB\FILE' wrongly looks in the root -> not found
 * (the subdir oracle goes RED). NEVER define either in a real build (Rule 6). */
static uint16_t resolve_dir_path(const char *path, const char **out_leaf,
                                 uint16_t *out_dir_start)
{
    if (path_overlength(path)) {
        return INT21_ERR_PATH_NOT_FOUND;     /* runaway / unterminated pointer */
    }

    /* Strip a leading drive prefix ('X:') -- one volume this milestone, so the
     * letter is ignored; only the path after ':' matters. */
    const char *p = path;
#ifndef INT21_MUTATE_RESOLVE_NODRIVE
    if (p[0] != '\0' && p[1] == ':') {
        p += 2;                              /* skip "X:" */
    }
#endif

    if (g_file == 0 || g_file->resolve == 0) {
        /* No backend resolve bound: root-only fallback (the pre-mzxa behavior).
         * A bare root-relative name resolves to the root; any remaining '\' or
         * ':' is unsupported -> path-not-found. */
        if (path_has_subdir_or_drive(p)) {
            return INT21_ERR_PATH_NOT_FOUND;
        }
        *out_leaf      = p;
        *out_dir_start = 0u;
        return 0u;
    }

    const char *leaf       = p;
    uint16_t    dir_start  = 0u;
    uint16_t    err = g_file->resolve(p, g_cwd_start_cluster, &leaf, &dir_start);
    if (err != 0u) {
        return INT21_ERR_PATH_NOT_FOUND;     /* missing/non-dir component (0x0003) */
    }
#ifdef INT21_MUTATE_RESOLVE_NOTROOT
    dir_start = 0u;                          /* MUTANT: always look in the root */
#endif
    *out_leaf      = leaf;
    *out_dir_start = dir_start;
    return 0u;
}

/* AH=3Dh OPEN: EDX = flat ptr to ASCIIZ path, AL = mode (0=r,1=w,2=rdwr).
 * A '\SUB\FILE'-qualified path is resolved to its containing directory's start
 * cluster via the backend resolve seam (beads initech-mzxa); a missing/non-dir
 * component or an overlength path -> CF=1, AX=0x0003 (path not found). On
 * success: allocate an SFT FILE slot + a JFT slot, LOCATE the file in the
 * resolved directory (no whole-file read -- positioned per-handle I/O, beads
 * initech-0qh), store its dir_entry + root_slot + mode in the SFT with
 * file_offset=0, return EAX = handle (JFT index), CF clear. Any number of files
 * may be open concurrently (each its own SFT slot). Errors: AX=0x0002 (not
 * found), 0x0003 (path), 0x0004 (no free SFT/JFT slot). Ref: brief Sec 4.1;
 * DOS 3.3 PRM AH=3Dh. */
static void do_open(int_frame_t *f)
{
    const char *path = (const char *)(uintptr_t)f->edx;
    uint8_t mode = frame_al(f);

    if (path == 0 || *path == '\0') {
        set_ax(f, INT21_ERR_FILE_NOT_FOUND);
        cf_set(f);
        return;
    }
    const char *leaf      = path;
    uint16_t    dir_start = 0u;
    uint16_t    perr = resolve_dir_path(path, &leaf, &dir_start);
    if (perr != 0u) {
        set_ax(f, perr);                 /* 0x0003 PATH_NOT_FOUND */
        int21_note_error(perr);          /* AH=59h GET EXTENDED ERROR */
        cf_set(f);
        return;
    }
    if (g_file == 0 || g_file->open == 0) {
        /* No mounted volume bound -> behave as file-not-found (Rule 2: never a
         * silent success). */
        set_ax(f, INT21_ERR_FILE_NOT_FOUND);
        cf_set(f);
        return;
    }

    /* Allocate the table slots BEFORE locating so a slot-exhaustion failure does
     * not commit anything; the backend's open() merely locates (no buffer). */
    uint8_t sft_idx = sft_alloc();
    if (sft_idx >= (uint8_t)SFT_MAX_ENTRIES) {
        set_ax(f, INT21_ERR_TOO_MANY_OPEN);
        cf_set(f);
        return;
    }
    uint8_t handle = jft_alloc(g_cur_psp);
    if (handle == JFT_CLOSED) {
        set_ax(f, INT21_ERR_TOO_MANY_OPEN);
        cf_set(f);
        return;
    }

    dir_entry_t  de;
    uint32_t     root_slot = 0u;
    uint16_t err = g_file->open(leaf, dir_start, &de, &root_slot);
    if (err != 0) {
        /* Backend rejected (not found). No slot was committed (we only read
         * sft_alloc/jft_alloc, never wrote them), so nothing to roll back. */
        set_ax(f, err);
        int21_note_error(err);   /* AH=59h GET EXTENDED ERROR can report it */
        cf_set(f);
        return;
    }

    /* Commit the SFT FILE entry + the JFT mapping. The SFT slot is the complete
     * per-handle state: its own position + dir_entry copy + root_slot. */
    sft_entry_t *e = &g_sft[sft_idx];
    e->kind        = SFT_KIND_FILE;
    e->open_mode   = mode;
    e->dev_id      = 0;
    e->ref_count   = 1u;
    e->dir_entry   = de;          /* struct copy of the 32-byte FAT dir entry */
    e->file_offset = 0u;
    e->root_slot   = root_slot;
    g_cur_psp->jft[handle] = sft_idx;

    f->eax = (f->eax & 0xFFFF0000u) | (uint32_t)handle;  /* EAX (AX) = handle */
    cf_clear(f);
}

/* AH=3Ch CREAT: EDX = flat ptr to ASCIIZ 8.3 path, CX = attribute. Create or
 * TRUNCATE the file (size 0 on disk), claim an SFT FILE slot + a JFT slot in
 * WRITE mode, store its dir_entry + root_slot + file_offset=0, and return EAX =
 * handle, CF clear. Subsequent AH=40h WRITEs commit positioned to disk per call
 * (no deferred flush; beads initech-0qh). Root-dir 8.3 names only; '\' / ':' ->
 * CF=1, AX=0x0003. No write backend / read-only volume -> CF=1, AX=0x0005
 * (access denied). Dir full -> CF=1, AX=0x0004.
 * Ref: DOS 3.3 PRM AH=3Ch; beads initech-0qh. */
static void do_creat(int_frame_t *f)
{
    const char *path = (const char *)(uintptr_t)f->edx;

    if (path == 0 || *path == '\0') {
        set_ax(f, INT21_ERR_FILE_NOT_FOUND);
        cf_set(f);
        return;
    }
    /* Resolve the CONTAINING directory (the file itself does not exist yet, so
     * the resolve targets the parent chain, not the leaf). A missing/non-dir
     * parent component or an overlength path -> 0x0003. */
    const char *leaf      = path;
    uint16_t    dir_start = 0u;
    uint16_t    perr = resolve_dir_path(path, &leaf, &dir_start);
    if (perr != 0u) {
        set_ax(f, perr);                 /* 0x0003 PATH_NOT_FOUND */
        int21_note_error(perr);          /* AH=59h GET EXTENDED ERROR */
        cf_set(f);
        return;
    }
    if (g_file == 0 || g_file->create == 0) {
        /* No write backing (read-only volume / no volume) -> access denied
         * (Rule 2: never a silent success that drops the file). */
        set_ax(f, INT21_ERR_ACCESS_DENIED);
        cf_set(f);
        return;
    }

    /* Secure the table slots BEFORE creating so a slot-exhaustion failure does
     * not commit anything (mirrors do_open). */
    uint8_t sft_idx = sft_alloc();
    if (sft_idx >= (uint8_t)SFT_MAX_ENTRIES) {
        set_ax(f, INT21_ERR_TOO_MANY_OPEN);
        cf_set(f);
        return;
    }
    uint8_t handle = jft_alloc(g_cur_psp);
    if (handle == JFT_CLOSED) {
        set_ax(f, INT21_ERR_TOO_MANY_OPEN);
        cf_set(f);
        return;
    }

    dir_entry_t de;
    uint32_t    root_slot = 0u;
    uint16_t err = g_file->create(leaf, dir_start, &de, &root_slot);
    if (err != 0) {
        set_ax(f, err);
        cf_set(f);
        return;
    }

    sft_entry_t *e = &g_sft[sft_idx];
    e->kind        = SFT_KIND_FILE;
    e->open_mode   = SFT_MODE_WRITE;   /* a write handle (positioned writes) */
    e->dev_id      = 0;
    e->ref_count   = 1u;
    e->dir_entry   = de;
    e->file_offset = 0u;
    e->root_slot   = root_slot;        /* root-dir slot for positioned write-back */
    g_cur_psp->jft[handle] = sft_idx;

    f->eax = (f->eax & 0xFFFF0000u) | (uint32_t)handle;
    cf_clear(f);
}

/* AH=41h UNLINK (DELETE FILE): EDX = flat ptr to ASCIIZ path. Resolve a
 * '\SUB\FILE'-qualified path to its containing directory (beads initech-mzxa)
 * and delete the file there (mark deleted + free its chain) via the backend.
 * CF clear on success; CF=1, AX=0x0002 (not found) / 0x0003 (path) / 0x0005
 * (error / no backend). Ref: DOS 3.3 PRM AH=41h; beads initech-509.11. */
static void do_unlink(int_frame_t *f)
{
    const char *path = (const char *)(uintptr_t)f->edx;

    if (path == 0 || *path == '\0') {
        set_ax(f, INT21_ERR_FILE_NOT_FOUND);
        cf_set(f);
        return;
    }
    const char *leaf      = path;
    uint16_t    dir_start = 0u;
    uint16_t    perr = resolve_dir_path(path, &leaf, &dir_start);
    if (perr != 0u) {
        set_ax(f, perr);                 /* 0x0003 PATH_NOT_FOUND */
        int21_note_error(perr);          /* AH=59h GET EXTENDED ERROR */
        cf_set(f);
        return;
    }
    if (g_file == 0 || g_file->unlink == 0) {
        set_ax(f, INT21_ERR_ACCESS_DENIED);
        cf_set(f);
        return;
    }

    uint16_t err = g_file->unlink(leaf, dir_start);
    if (err != 0) {
        set_ax(f, err);
        cf_set(f);
        return;
    }
    cf_clear(f);
}

/* AH=39h MKDIR (CREATE DIRECTORY): EDX = flat ptr to an ASCIIZ path. Resolve the
 * path to its CONTAINING directory (the new dir does not exist yet) via the
 * resolve seam and create the bare leaf there through the backend mkdir member.
 * ROOT-PARENT ONLY this landing (a non-root parent -> the backend's dir_start!=0
 * guard returns 0x0003; nested MD is beads initech-zs24). CF clear on success;
 * CF=1, AX=0x0003 (bad path component / no backend) / 0x0005 (name already
 * exists, dir full, full volume, read-only). The AH=59h dispatch wrapper
 * auto-notes CF+AX so no int21_note_error here. Ref: DOS 3.3 PRM AH=39h; PRD
 * Sec 6.5 (DOS path model); beads initech-u6wa. */
static void do_mkdir(int_frame_t *f)
{
    const char *path = (const char *)(uintptr_t)f->edx;

    if (path == 0 || *path == '\0') {
        set_ax(f, INT21_ERR_PATH_NOT_FOUND);
        cf_set(f);
        return;
    }
    /* Resolve the CONTAINING directory + bare leaf (the new dir name). A
     * missing/non-dir parent component or an overlength path -> 0x0003. */
    const char *leaf      = path;
    uint16_t    dir_start = 0u;
    uint16_t    perr = resolve_dir_path(path, &leaf, &dir_start);
    if (perr != 0u) {
        set_ax(f, perr);                 /* 0x0003 PATH_NOT_FOUND */
        cf_set(f);
        return;
    }
    if (g_file == 0 || g_file->mkdir == 0) {
        /* No write backing (read-only volume / no volume) -> access denied
         * (Rule 2: never a silent success that drops the directory). */
        set_ax(f, INT21_ERR_ACCESS_DENIED);
        cf_set(f);
        return;
    }

    uint16_t err = g_file->mkdir(leaf, dir_start);
    if (err != 0) {
        set_ax(f, err);                  /* 0x0005 exists / 0x0003 non-root parent */
        cf_set(f);
        return;
    }
    cf_clear(f);
}

/* AH=3Ah RMDIR (REMOVE DIRECTORY): EDX = flat ptr to an ASCIIZ path. Resolve the
 * FULL path to the TARGET directory (resolve_dir seam) for the two dispatcher-
 * level guards, then resolve its CONTAINING directory + bare leaf (resolve seam)
 * and remove it through the backend rmdir member (which verifies the directory
 * is empty). ROOT-PARENT ONLY this landing (a non-root parent -> the backend's
 * dir_start!=0 guard returns 0x0003; nested RD is beads initech-zs24).
 * Dispatcher-level guards (it owns the CWD state):
 *   - RMDIR of the ROOT ('\' / '' / a path resolving to the root) -> reject with
 *     0x0010 (DOS forbids removing the root, like the current dir);
 *   - RMDIR whose TARGET start_cluster == the current g_cwd_start_cluster ->
 *     0x0010 (DOS "attempt to remove the current directory").
 * Backend errors: 0x0003 (missing dir / not a directory / non-root parent),
 * 0x0005 (the directory is not empty). CF clear on success. The AH=59h dispatch
 * wrapper auto-notes CF+AX. Ref: DOS 3.3 PRM AH=3Ah; beads initech-u6wa. */
static void do_rmdir(int_frame_t *f)
{
    const char *path = (const char *)(uintptr_t)f->edx;

    if (path == 0 || *path == '\0') {
        /* An empty path names no directory (the root cannot be removed). */
        set_ax(f, INT21_ERR_CURRENT_DIR);
        cf_set(f);
        return;
    }
    /* A bare root '\' is the root directory -- never removable. */
    if (path[0] == '\\' && path[1] == '\0') {
        set_ax(f, INT21_ERR_CURRENT_DIR);
        cf_set(f);
        return;
    }

    if (path_overlength(path)) {
        set_ax(f, INT21_ERR_PATH_NOT_FOUND);     /* runaway / unterminated ptr */
        cf_set(f);
        return;
    }

    if (g_file == 0 || g_file->rmdir == 0) {
        set_ax(f, INT21_ERR_ACCESS_DENIED);
        cf_set(f);
        return;
    }

    /* Resolve the FULL path to the TARGET directory's own start cluster (0 ==
     * root) for the root-reject + current-dir guards. resolve_dir validates the
     * final component IS a directory; a missing dir / a path into a file ->
     * 0x0003. With no resolve_dir bound, fall through to the backend resolve. */
    if (g_file->resolve_dir != 0) {
        uint16_t target_start = 0xFFFFu;
        char     canon[INT21_CWD_MAX];
        uint16_t rerr = g_file->resolve_dir(path, g_cwd_start_cluster,
                                            &target_start, canon,
                                            (uint32_t)sizeof(canon));
        if (rerr != 0u) {
            set_ax(f, INT21_ERR_PATH_NOT_FOUND); /* missing dir / not a dir */
            cf_set(f);
            return;
        }
        if (target_start == 0u) {
            /* The path resolved to the ROOT (e.g. '\SUB\..') -- not removable. */
            set_ax(f, INT21_ERR_CURRENT_DIR);
            cf_set(f);
            return;
        }
        if (target_start == g_cwd_start_cluster) {
            /* DOS forbids removing the directory you are currently in. */
            set_ax(f, INT21_ERR_CURRENT_DIR);
            cf_set(f);
            return;
        }
    }

    /* Resolve the CONTAINING directory + bare leaf for the backend remove. */
    const char *leaf      = path;
    uint16_t    dir_start = 0u;
    uint16_t    perr = resolve_dir_path(path, &leaf, &dir_start);
    if (perr != 0u) {
        set_ax(f, perr);                 /* 0x0003 PATH_NOT_FOUND */
        cf_set(f);
        return;
    }

    uint16_t err = g_file->rmdir(leaf, dir_start);
    if (err != 0) {
        set_ax(f, err);                  /* 0x0005 non-empty / 0x0003 non-root */
        cf_set(f);
        return;
    }
    cf_clear(f);
}

/* AH=3Fh READ: EBX=handle, ECX=count, EDX=flat ptr to buffer. POSITIONED read
 * of up to `count` bytes from the per-handle file_offset over the cluster chain
 * (backend read_at via fat12_read_partial -- no whole-file buffer; beads
 * initech-0qh); advance file_offset by the bytes read; EAX = bytes read, CF
 * clear. A read at/after EOF returns 0 bytes cleanly. A CON device read
 * (keyboard input) is deferred (beads initech-n62) -> 0 bytes (EOF), NEVER a
 * hang. AUX/PRN read has no driver -> 0 bytes. Bad handle -> CF=1, AX=0x0006.
 * Ref: brief Sec 4.2; DOS 3.3 PRM AH=3Fh. */
static void do_read(int_frame_t *f)
{
    uint32_t handle = f->ebx;
    uint32_t count  = f->ecx;
    uint8_t *buf    = (uint8_t *)(uintptr_t)f->edx;

    sft_entry_t *e = (handle <= 0xFFu)
                       ? sft_from_handle(g_cur_psp, (uint8_t)handle)
                       : 0;
    if (e == 0) {
        set_ax(f, INT21_ERR_INVALID_HANDLE);
        cf_set(f);
        return;
    }

    if (e->kind == SFT_KIND_DEVICE) {
        /* CON keyboard input is deferred (beads initech-n62); AUX/PRN have no
         * driver. Return 0 bytes read (EOF) WITHOUT hanging -- a clear deferred
         * path, success with EAX=0 (brief Sec 4.2). */
        f->eax = 0u;
        cf_clear(f);
        return;
    }

    /* Validate the destination buffer before the backend writes into it
     * (ADR-0003 DEC-14 / initech-tzq). The DEVICE leg above never touches the
     * buffer (returns EOF), so the guard belongs on the FILE path. */
    if (!user_buf_ok(f->edx, count)) {
        set_ax(f, INT21_ERR_INVALID_MEMORY);
        cf_set(f);
        return;
    }

    /* FILE: positioned read over the cluster chain (no whole-file buffer). The
     * backend serves min(count, file_size - file_offset) bytes from the chain. */
    uint32_t take = 0u;
    if (g_file != 0 && g_file->read_at != 0) {
        uint16_t err = g_file->read_at(&e->dir_entry, e->file_offset,
                                       buf, count, &take);
        if (err != 0) {
            set_ax(f, err);
            cf_set(f);
            return;
        }
    }
#ifdef INT21_MUTATE_READ_IGNORE_OFFSET
    /* MUTANT (Rule 6; make test-fileio-mutant only): advance the offset by ZERO
     * so a second READ re-reads from the start instead of continuing. The READ
     * oracle (which reads in two chunks and concatenates) goes RED. NEVER define
     * in a real build. */
#else
    e->file_offset += take;
#endif
    f->eax = take;
    cf_clear(f);
}

/* AH=3Eh CLOSE: EBX=handle. Decrement the SFT ref; on the last reference free
 * the slot; JFT[EBX] = 0xFF. With positioned per-call writes (beads initech-0qh)
 * the bytes are already on disk, so close holds no flush -- it calls the
 * backend's close(slot) hook (a no-op kept for symmetry / a future deferred
 * write model) and frees the slot. Closing a predefined handle 0-4 is a no-op
 * success (as real DOS). CF clear; bad handle -> CF=1, AX=0x0006.
 * Ref: brief Sec 4.3; DOS 3.3 PRM AH=3Eh. */
static void do_close(int_frame_t *f)
{
    uint32_t handle = f->ebx;

    if (handle > 0xFFu) {
        set_ax(f, INT21_ERR_INVALID_HANDLE);
        cf_set(f);
        return;
    }

    /* Predefined standard handles 0-4: no-op success (real DOS does not actually
     * close stdin/stdout/stderr/aux/prn on a 3Eh -- the device stays live). */
    if (handle <= 4u) {
        cf_clear(f);
        return;
    }

    sft_entry_t *e = sft_from_handle(g_cur_psp, (uint8_t)handle);
    if (e == 0) {
        set_ax(f, INT21_ERR_INVALID_HANDLE);
        cf_set(f);
        return;
    }

    uint8_t  sft_idx   = g_cur_psp->jft[handle];
    int      was_file  = (e->kind == SFT_KIND_FILE);
    uint32_t root_slot = e->root_slot;

#ifdef INT21_MUTATE_CLOSE_NO_REFGUARD
    /* MUTANT (Rule 6; make test-int21-edge-mutant only): drop the ref_count>0
     * guard so a CLOSE of a slot whose ref_count is already 0 UNDERFLOWS the
     * uint16 ref_count to 0xFFFF -- the exact double-close corruption flagged by
     * beads initech-00x. The double-close-underflow oracle (which asserts the
     * ref_count never wraps) goes RED. NEVER define in a real build. */
    e->ref_count--;
#else
    if (e->ref_count > 0u) {
        e->ref_count--;
    }
#endif
    if (e->ref_count == 0u) {
        /* Last reference: finalize the handle via the backend (no-op with
         * per-call positioned writes; the hook is for symmetry / future
         * deferred buffering), then free the SFT slot. */
        if (was_file && g_file != 0 && g_file->close != 0) {
            g_file->close(root_slot);
        }
        for (uint32_t i = 0; i < (uint32_t)sizeof(g_sft[sft_idx]); i++) {
            ((uint8_t *)&g_sft[sft_idx])[i] = 0;  /* -> SFT_KIND_FREE */
        }
    }
    g_cur_psp->jft[handle] = JFT_CLOSED;
    cf_clear(f);
}

/* AH=42h LSEEK: EBX=handle, AL=whence (0=start,1=cur,2=end), ECX:EDX offset
 * (ECX high 16 bits = 0 this milestone). New absolute offset -> EAX. Seeking
 * past EOF is allowed (a subsequent READ then returns 0). CF clear; bad handle
 * -> CF=1, AX=0x0006; bad whence -> CF=1, AX=0x0001.
 * Ref: brief Sec 4.4; DOS 3.3 PRM AH=42h. */
static void do_lseek(int_frame_t *f)
{
    uint32_t handle = f->ebx;
    uint8_t  whence = frame_al(f);
    uint32_t off    = f->edx;   /* low 32 bits; ECX high half is 0 this milestone */

    sft_entry_t *e = (handle <= 0xFFu)
                       ? sft_from_handle(g_cur_psp, (uint8_t)handle)
                       : 0;
    if (e == 0) {
        set_ax(f, INT21_ERR_INVALID_HANDLE);
        cf_set(f);
        return;
    }

    uint32_t base;
    switch (whence) {
        case 0: base = 0u; break;                       /* from start            */
        case 1: base = e->file_offset; break;           /* from current position */
        case 2: base = e->dir_entry.file_size; break;   /* from end              */
        default:
            set_ax(f, INT21_ERR_INVALID_FUNCTION);
            cf_set(f);
            return;
    }
#ifdef INT21_MUTATE_LSEEK_WHENCE
    /* MUTANT (Rule 6; make test-fileio-mutant only): whence 2 (from end) uses 0
     * as the base instead of file_size, so SEEK_END lands at the wrong offset.
     * The LSEEK oracle goes RED. NEVER define in a real build. */
    base = 0u;
#endif
    e->file_offset = base + off;   /* past-EOF allowed (DOS); READ then returns 0 */
    f->eax = e->file_offset;
    cf_clear(f);
}

/* AH=1Ah SETDTA: EDX = flat ptr to the new Disk Transfer Area. CF clear (DOS
 * 1Ah has no error path). Ref: DOS 3.3 PRM AH=1Ah. */
static void do_setdta(int_frame_t *f)
{
    g_dta = f->edx;
    cf_clear(f);
}

/* AH=2Fh GETDTA: returns the current DTA flat ptr. Real DOS returns it in
 * ES:BX; in the flat ABI we return it in EBX (DEC-04a flat-ptr convention). If
 * no DTA was set, the default is the current PSP:0x80 (the command-tail field).
 * CF clear. Ref: DOS 3.3 PRM AH=2Fh; spec/find_data.h DTA_DEFAULT_PSP_OFFSET. */
static void do_getdta(int_frame_t *f)
{
    uint32_t dta = g_dta;
    if (dta == 0 && g_cur_psp != 0) {
        dta = (uint32_t)(uintptr_t)&g_cur_psp->cmd_tail[0];
    }
    f->ebx = dta;
    cf_clear(f);
}

/* The current DTA flat ptr, resolving the PSP:0x80 default. Returns 0 only when
 * neither a DTA nor a PSP is bound (a FINDFIRST then fails loud, Rule 2). */
static uint32_t current_dta(void)
{
    if (g_dta != 0) {
        return g_dta;
    }
    if (g_cur_psp != 0) {
        return (uint32_t)(uintptr_t)&g_cur_psp->cmd_tail[0];
    }
    return 0;
}

/* Build the 11-byte 8.3 search template from an ASCIIZ file spec. '*' expands to
 * the remaining '?' wildcards in its field (name 0..7, ext 0..2); a bare "*.*"
 * thus becomes all-'?' (match all). Other chars are upper-cased and copied;
 * short fields are space-padded. This is the milestone subset (brief Sec 4.5):
 * "*.*" (match all) and exact 8.3 match -- NOT the full DOS wildcard engine. */
static void build_pattern(const char *spec, uint8_t out[11])
{
    for (int i = 0; i < 11; i++) {
        out[i] = ' ';
    }

    int field = 0;                 /* 0 = name (0..7), 1 = ext (8..10) */
    int pos   = 0;
    const char *p = spec;

    while (*p) {
        char c = *p++;
        if (c == '.') {
            field = 1;
            pos   = 0;
            continue;
        }
        int base = (field == 0) ? 0 : 8;
        int cap  = (field == 0) ? 8 : 3;
        if (c == '*') {
            /* Fill the rest of THIS field with '?' (match any). */
            for (; pos < cap; pos++) {
                out[base + pos] = '?';
            }
            continue;
        }
        if (pos < cap) {
            /* Upper-case ASCII a-z. */
            if (c >= 'a' && c <= 'z') {
                c = (char)(c - 'a' + 'A');
            }
            out[base + pos] = (uint8_t)c;
            pos++;
        }
    }
}

/* Render a dir entry's raw 8.3 name fields into an 11-byte (no-dot) template for
 * comparison against the search pattern. The on-disk filename[8]/extension[3]
 * are already space-padded upper-case (spec/dos_structs.h). */
static void entry_template(const dir_entry_t *e, uint8_t out[11])
{
    for (int i = 0; i < 8; i++) {
        out[i] = e->filename[i];
    }
    for (int i = 0; i < 3; i++) {
        out[8 + i] = e->extension[i];
    }
}

/* Match an 11-byte name template against a search pattern: a '?' in the pattern
 * matches any byte; otherwise the bytes must be equal. */
static int pattern_match(const uint8_t pat[11], const uint8_t name[11])
{
    for (int i = 0; i < 11; i++) {
        if (pat[i] == '?') {
            continue;
        }
        if (pat[i] != name[i]) {
            return 0;
        }
    }
    return 1;
}

/* Format a dir entry's 8.3 name as "NAME.EXT\0" (or "NAME\0" when the extension
 * is all spaces) into out[13]. Mirrors fat12_format_83 but kept local so int21.c
 * does not link fat12.c into the host oracle (Law 3 / host-testability). */
static void format_83(const dir_entry_t *e, char out[13])
{
    int n = 0;
    for (int i = 0; i < 8 && e->filename[i] != ' '; i++) {
        out[n++] = (char)e->filename[i];
    }
    if (e->extension[0] != ' ') {
        out[n++] = '.';
        for (int i = 0; i < 3 && e->extension[i] != ' '; i++) {
            out[n++] = (char)e->extension[i];
        }
    }
    out[n] = '\0';
}

/* Write the 43-byte find_data_t for a matched entry into the current DTA, then
 * record the search position for FINDNEXT. */
static uint16_t emit_find_data(const dir_entry_t *e)
{
    uint32_t dta = current_dta();
    if (dta == 0) {
        /* No DTA and no PSP -> we cannot honor the find contract. Fail loud
         * rather than write through a NULL (Rule 2). */
        __builtin_trap();
        return INT21_ERR_INVALID_MEMORY;   /* not reached */
    }
    /* A wild (non-NULL but count-overflowing) DTA must not be scribbled with the
     * 43-byte find_data_t -- fail loud and let FINDFIRST/NEXT return the error
     * (ADR-0003 DEC-14 / initech-tzq). The NULL-DTA -> PSP:0x80 fallback in
     * current_dta() is preserved above. */
    if (!user_buf_ok(dta, (uint32_t)sizeof(find_data_t))) {
        return INT21_ERR_INVALID_MEMORY;
    }
    find_data_t *fd = (find_data_t *)(uintptr_t)dta;

    /* Write all 43 bytes deterministically (Rule 11): the 0x00..0x14 internal
     * region first (drive/template/search-attr + reserved resume state), then
     * the found-entry fields at their real-DOS offsets 0x15.. (dww). */
    fd->drive_attr = 0u;
    for (int i = 0; i < 11; i++) {
        fd->pattern[i] = g_find.pattern[i];
    }
    fd->search_attr    = g_find.search_attr;
    fd->dir_entry      = 0u;
    fd->parent_cluster = 0u;
    for (int i = 0; i < 4; i++) {
        fd->reserved1[i] = 0u;
    }
    fd->attr  = e->attribute;
    fd->ftime = e->mtime;
    fd->fdate = e->mdate;
    fd->fsize = e->file_size;
    format_83(e, fd->fname);
    return 0u;
}

/* Scan from g_find.next_index for the next surviving entry matching the search
 * pattern + attribute mask, write it to the DTA, advance next_index past it, and
 * return 0 (CF clear). When none remain return AX=0x0012 (no more files). The
 * attribute filter: volume-label (bit 3) and directory (bit 4) entries are
 * skipped UNLESS the search mask requests that class (brief Sec 4.5). */
static uint16_t find_scan(int_frame_t *f)
{
    if (g_file == 0 || g_file->dir_entry == 0) {
        return INT21_ERR_NO_MORE_FILES;   /* no volume -> empty directory */
    }

    for (;;) {
        dir_entry_t de;
        int found = 0;
        uint16_t rc = g_file->dir_entry(g_find.next_index, g_find.dir_start,
                                        &de, &found);
        if (rc != 0) {
            return rc;                     /* backend read error */
        }
        if (!found) {
            return INT21_ERR_NO_MORE_FILES;  /* end of directory */
        }
        g_find.next_index++;

        /* Attribute filter: skip vollabel/dir entries unless requested. */
        uint8_t special = (uint8_t)(de.attribute &
                                    (DIR_ATTR_VOLLABEL | DIR_ATTR_DIRECTORY));
        if (special != 0 && (special & g_find.search_attr) == 0) {
            continue;
        }

        uint8_t name[11];
        entry_template(&de, name);
        if (!pattern_match(g_find.pattern, name)) {
            continue;
        }

        uint16_t werr = emit_find_data(&de);
        if (werr != 0u) {
            cf_set(f);
            return werr;       /* wild DTA -- fail loud (DEC-14) */
        }
        cf_clear(f);
        return 0u;
    }
}

/* AH=4Eh FINDFIRST: EDX = flat ptr to ASCIIZ file spec, ECX = attribute mask.
 * A '\SUB\*.TXT'-qualified spec is resolved to its containing directory (beads
 * initech-mzxa): the directory portion -> the start cluster FINDNEXT enumerates,
 * and the final component -> the 8.3 search template. A missing/non-dir
 * component -> CF=1, AX=0x0003 (path not found). Then build the search template,
 * reset the search position, and emit the first match into the current DTA. CF
 * clear on a hit; CF=1, AX=0x0012 (no more files) / 0x0002 when nothing matches.
 * Ref: brief Sec 4.5; DOS 3.3 PRM AH=4Eh. */
static void do_findfirst(int_frame_t *f)
{
    const char *spec = (const char *)(uintptr_t)f->edx;
    if (spec == 0 || *spec == '\0') {
        set_ax(f, INT21_ERR_NO_MORE_FILES);
        cf_set(f);
        return;
    }
    const char *leaf      = spec;
    uint16_t    dir_start = 0u;
    uint16_t    perr = resolve_dir_path(spec, &leaf, &dir_start);
    if (perr != 0u) {
        set_ax(f, perr);                 /* 0x0003 PATH_NOT_FOUND */
        int21_note_error(perr);          /* AH=59h GET EXTENDED ERROR */
        cf_set(f);
        return;
    }

    build_pattern(leaf, g_find.pattern);
    g_find.search_attr = (uint8_t)(f->ecx & 0xFFu);
    g_find.next_index  = 0u;
    g_find.dir_start   = dir_start;
    g_find.active      = 1u;

    uint16_t rc = find_scan(f);
    if (rc != 0) {
        /* FINDFIRST with no match returns file-not-found in real DOS (0x0002),
         * but DOS programs accept 0x0012 too; we use no-more-files for an empty
         * result and the backend's error otherwise. */
        set_ax(f, rc);
        cf_set(f);
    }
}

/* AH=4Fh FINDNEXT: no args; continue the active search from FINDFIRST. CF clear
 * on a hit; CF=1, AX=0x0012 when exhausted, or 0x0012 if no FINDFIRST ran.
 * Ref: brief Sec 4.5; DOS 3.3 PRM AH=4Fh. */
static void do_findnext(int_frame_t *f)
{
    if (!g_find.active) {
        set_ax(f, INT21_ERR_NO_MORE_FILES);
        cf_set(f);
        return;
    }
    uint16_t rc = find_scan(f);
    if (rc != 0) {
        set_ax(f, rc);
        cf_set(f);
    }
}

/* AH=4Bh EXEC (LOAD AND EXECUTE): AL = subfunction, EDX = flat ptr to the ASCIIZ
 * program path (root-dir 8.3 only this milestone). AL=00h is load-and-execute (a
 * child program); AL=03h (load overlay) + other subfunctions are out of scope ->
 * invalid function (CF=1, AX=0x0001). The child runs to completion via the saw
 * path (load_program_from_fat); when it terminates (4Ch / INT 20h) the loader
 * regains control and EXEC returns to the caller with CF clear. The child's exit
 * code is stashed for AH=4Dh GET-RETURN-CODE.
 *
 * Path rules: a '\' or ':' -> CF=1, AX=0x0003 (path not found); an empty/NULL
 * path -> file-not-found. EXEC stays ROOT-ONLY this milestone (beads
 * initech-mzxa decision 3): unlike OPEN/CREAT/UNLINK/FINDFIRST -- which resolve
 * a subdir path through the read-side resolve seam -- the EXEC source feeds the
 * loader's program-load path (load_program_from_fat -> load_program), and that
 * path locates the .COM by a ROOT-dir 8.3 name (loader.c). Threading a resolved
 * containing-directory cluster into the loader is a RISKIER loader-internal
 * change than the clean read-side resolve, so per decision 3 do_exec keeps
 * rejecting subdir paths and a FOLLOW-UP bead (initech-zs24) tracks subdir EXEC.
 *
 * REENTRANCY (Rule 2 / stop condition): the loader's return-to-loader context
 * (loader.c g_loader_ctx) is single-level -- a nested EXEC (EXEC issued from
 * inside an already-running loaded program) would clobber it. load_program_from_fat
 * guards this and returns INSUFFICIENT_MEM; we surface that as CF=1, AX=0x0008.
 * EXEC from KERNEL/shell context (the common case) is fully supported.
 * Ref: DOS 3.3 PRM AH=4Bh; ADR-0003 DEC-08 (flat .COM); beads initech-saw. */
static void do_exec(int_frame_t *f)
{
    uint8_t sub = frame_al(f);
    if (sub != 0x00u) {
        /* AL=03h load-overlay + any other subfunction: out of scope (Rule 2). */
        set_ax(f, INT21_ERR_INVALID_FUNCTION);
        cf_set(f);
        return;
    }

    const char *path = (const char *)(uintptr_t)f->edx;
    if (path == 0 || *path == '\0') {
        set_ax(f, INT21_ERR_FILE_NOT_FOUND);
        cf_set(f);
        return;
    }
    if (path_has_subdir_or_drive(path)) {
        set_ax(f, INT21_ERR_PATH_NOT_FOUND);
        cf_set(f);
        return;
    }
    if (g_exec == 0) {
        /* No loader bound -> behave as file-not-found (Rule 2: never a silent
         * success; an EXEC with no backing cannot have run a program). */
        set_ax(f, INT21_ERR_FILE_NOT_FOUND);
        cf_set(f);
        return;
    }

    /* InDOS balance across a SYNCHRONOUS child run (beads initech-xk2). The child
     * runs INSIDE g_exec and itself issues INT 21h calls (legitimate nesting,
     * irq_depth()==0 throughout -- NOT the IRQ reentry the guard catches). When
     * the child terminates (4Ch / INT 20h) the loader's exit hook does a
     * NON-returning longjmp back into load_program (loader.c loader_exit_hook),
     * DISCARDING the child's syscall stack -- so that terminating syscall's
     * int21_dispatch wrapper never runs its matching g_indos-- (its `++` is
     * stranded). Snapshot g_indos here and restore it after the child returns so
     * the InDOS depth is exactly the caller's level again, never drifting upward
     * across an EXEC chain (Rule 11). In the host oracle the mock g_exec simply
     * returns, so this snapshot/restore is a harmless no-op. */
    /* Command tail (initech-456): AH=4Bh takes an EXEC parameter block in EBX
     * (exec_param_block_t, the flat-32 analog of real DOS ES:BX). Extract the
     * command-tail TEXT+len for the child PSP:80h; the env/FCB fields are
     * reserved. Every level is DEC-14 user-pointer validated; an absent (EBX=0)
     * or unreadable block degrades to a no-argument launch (count=0) rather than
     * faulting on a legacy caller's stale EBX (Rule 2). The tail at cmd_tail is a
     * DOS-format {count, text, CR}; we hand the loader the TEXT (cmd_tail+1) and
     * its length, and psp_build re-frames it (count byte + CR) at PSP:80h. */
    const char *tail_text = 0;
    uint32_t    tail_len  = 0;
    {
        uint32_t pb = f->ebx;
        if (pb != 0 && user_buf_ok(pb, (uint32_t)sizeof(exec_param_block_t))) {
            const exec_param_block_t *blk = (const exec_param_block_t *)(uintptr_t)pb;
            uint32_t tp = blk->cmd_tail;
            if (tp != 0 && user_buf_ok(tp, 1u)) {
                uint8_t count = ((const uint8_t *)(uintptr_t)tp)[0];
                /* count text bytes + the leading count byte + the trailing CR */
                if (user_buf_ok(tp, (uint32_t)count + 2u)) {
                    tail_text = (const char *)(uintptr_t)(tp + 1u);
                    tail_len  = count;
                }
            }
        }
    }

    uint32_t indos_snapshot = g_indos;
    uint8_t rc = 0;
    uint16_t err = g_exec(path, tail_text, tail_len, &rc);
    g_indos = indos_snapshot;
    if (err != 0) {
        /* Load/run failed (not found / nested / too big). Fail loud (Rule 2). */
        set_ax(f, err);
        cf_set(f);
        return;
    }

    /* The child ran and returned. Stash its exit code for AH=4Dh; CF clear. DOS
     * leaves the child's rc retrievable via 4Dh, not in AX of the 4Bh return. */
    g_last_child_rc = rc;
    cf_clear(f);
}

/* AH=4Dh GET RETURN CODE (WAIT): return the exit code of the last child run via
 * AH=4Bh EXEC. AL = the exit code; AH = the termination type (0 = normal exit,
 * the only kind this milestone -- no Ctrl-C/critical-error/TSR termination). DOS
 * "consumes" the value (a second 4Dh reads 0), which we model by clearing the
 * stash after read. CF clear. Ref: DOS 3.3 PRM AH=4Dh. */
static void do_get_return_code(int_frame_t *f)
{
#ifdef INT21_MUTATE_RETCODE_ZERO
    /* MUTANT (Rule 6; make test-exec-mutant only): always report rc=0 regardless
     * of the child's actual exit code, so the 4Dh oracle (which EXECs a program
     * that exits rc=7 and asserts AL==7) goes RED. NEVER define in a real build. */
    f->eax = (f->eax & 0xFFFF0000u);          /* AL=0, AH=0 */
#else
    f->eax = (f->eax & 0xFFFF0000u) | (uint32_t)g_last_child_rc; /* AL = exit code */
    /* AH = 0 (normal termination); high byte of AX is the termination type. */
#endif
    g_last_child_rc = 0u;   /* consumed (DOS clears after the read) */
    cf_clear(f);
}

/* ===========================================================================
 * MEMORY ARENA: AH=48h ALLOC / AH=49h FREE / AH=4Ah SETBLOCK (initech-509.6).
 *
 * The handlers convert at the syscall edge between a DOS SEGMENT (paragraph
 * address, the value GET PSP/alloc_end_seg use) and the arena-relative DATA
 * PARAGRAPH index the pure allocator (mcb.c) speaks, then call into g_arena.
 *
 *   segment(data_para) = (g_arena_base_linear >> 4) + data_para
 *   data_para(segment) = segment - (g_arena_base_linear >> 4)
 *
 * FLAT-MODE REGISTER ABI (Law 1; DOS 3.3 PRM AH=48h/49h/4Ah, adapted): real DOS
 * passes the block SEGMENT in ES. In this 32-bit PROTECTED-flat kernel ES is a
 * GDT SELECTOR (the trap stub captured the caller's ES, which for a flat program
 * is DATA_SEL=0x10 -- loading a DOS fake-paragraph into ES would #GP), so a DOS
 * segment cannot ride ES. We carry it in a GP register, mirroring AH=62h GET PSP
 * (segment -> BX) and the loader (PSP pointer -> EBX):
 *   48h ALLOC   : BX = paragraphs wanted      -> AX = segment (CF=0);
 *                 on fail AX=0008h, BX = largest free paras (CF=1).
 *   49h FREE    : BX = segment to free        -> CF=0; on fail AX=err (CF=1).
 *   4Ah SETBLOCK: BX = segment, CX = new paras-> CF=0; on fail AX=0008h,
 *                 BX = max paras this block could reach (CF=1).
 * (4Ah uses CX for the new size because BX already carries the segment; real DOS
 * has ES free for the segment, we do not.) This is NOT a change to a locked spec
 * (spec/int21h_calling_convention.json does not yet list 48/49/4A); it is the
 * flat adaptation documented here + in the bead.
 *
 * The MCB error codes (mcb.h) map 1:1 onto DOS INT 21h errors:
 *   MCB_ERR_INSUFFICIENT 0x0008  -> AX=0x0008  (insufficient memory)
 *   MCB_ERR_DESTROYED    0x0007  -> AX=0x0007  (MCB chain destroyed)
 *   MCB_ERR_BAD_BLOCK    0x0009  -> AX=0x0009  (invalid memory block address)
 * so the handler returns the raw mcb status as AX on failure.
 * Ref: spec/memory_map.h (the arena region), mcb.h/mcb.c (the allocator),
 *      DOS 3.3 PRM. ===========================================================*/

/* DOS-segment base of the arena (paragraphs). data_para 0's segment + 1 == the
 * first data paragraph's segment, etc. */
static uint32_t arena_seg_base(void)
{
    return (g_arena_base_linear >> 4) & 0xFFFFu;
}

/* AH=48h ALLOCATE MEMORY: BX = paragraphs requested. On success AX = the DOS
 * segment of the allocated block, CF=0. On failure CF=1, AX=0x0008, BX = the
 * largest free block (paragraphs) so the caller can retry smaller (DOS contract).
 * Owner = the current PSP (a process must exist to own memory). */
static void do_alloc(int_frame_t *f)
{
    uint16_t want = (uint16_t)(f->ebx & 0xFFFFu);

    /* No arena bound, or no owning process -> nothing can be allocated. Report
     * insufficient memory with largest=0 (Rule 2: never fault, never silent). */
    if (!g_arena_bound || g_cur_psp == 0) {
        set_ax(f, INT21_ERR_INSUFFICIENT_MEM);
        set_bx(f, 0u);
        cf_set(f);
        return;
    }

    uint32_t data_para = 0u, largest = 0u;
    uint16_t rc = mcb_alloc(&g_arena, cur_psp_owner(), (uint32_t)want,
                            &data_para, &largest);
    if (rc != MCB_OK) {
        set_ax(f, rc);                  /* 0x0008 insufficient / 0x0007 destroyed */
        set_bx(f, (uint16_t)(largest & 0xFFFFu));
        cf_set(f);
        return;
    }
#ifdef INT21_MUTATE_ALLOC_NO_SEGBASE
    /* MUTANT (Rule 6; make test-mcb-int21-mutant only): drop the arena segment
     * base from the conversion, returning a bare data-paragraph index as the
     * "segment". A subsequent 49h FREE of that wrong segment then maps to a
     * different data_para and is rejected (or, with FREE using the same broken
     * map, the alloc/free round-trip oracle's exact-segment assertion goes RED).
     * NEVER define in a real build. */
    set_ax(f, (uint16_t)(data_para & 0xFFFFu));
#else
    set_ax(f, (uint16_t)((arena_seg_base() + data_para) & 0xFFFFu));
#endif
    cf_clear(f);
}

/* AH=49h FREE ALLOCATED MEMORY: BX = the DOS segment of the block to free. CF=0
 * on success; CF=1, AX=err on a bad block / owner mismatch / corrupt chain. Only
 * the owning PSP may free its block (MCB owner check). */
static void do_free(int_frame_t *f)
{
    uint16_t seg = (uint16_t)(f->ebx & 0xFFFFu);

    if (!g_arena_bound || g_cur_psp == 0) {
        set_ax(f, INT21_ERR_INVALID_MEMORY);   /* 0x0009 invalid block address */
        cf_set(f);
        return;
    }
    uint32_t base = arena_seg_base();
    if ((uint32_t)seg < base) {
        set_ax(f, INT21_ERR_INVALID_MEMORY);   /* segment below the arena */
        cf_set(f);
        return;
    }
    uint32_t data_para = (uint32_t)seg - base;
    uint16_t rc = mcb_free(&g_arena, data_para, cur_psp_owner());
    if (rc != MCB_OK) {
        set_ax(f, rc);                          /* 0x0009 bad block / 0x0007 */
        cf_set(f);
        return;
    }
    cf_clear(f);
}

/* AH=4Ah SETBLOCK (resize): BX = the DOS segment, CX = new paragraph count. CF=0
 * on success; on failure CF=1, AX=0x0008, BX = the largest size this block could
 * grow to (DOS contract). Only the owning PSP may resize its block. */
static void do_setblock(int_frame_t *f)
{
    uint16_t seg  = (uint16_t)(f->ebx & 0xFFFFu);
    uint16_t want = frame_cx(f);

    if (!g_arena_bound || g_cur_psp == 0) {
        set_ax(f, INT21_ERR_INSUFFICIENT_MEM);
        set_bx(f, 0u);
        cf_set(f);
        return;
    }
    uint32_t base = arena_seg_base();
    if ((uint32_t)seg < base) {
        set_ax(f, INT21_ERR_INVALID_MEMORY);
        cf_set(f);
        return;
    }
    uint32_t data_para = (uint32_t)seg - base;
    uint32_t largest   = 0u;
    uint16_t rc = mcb_setblock(&g_arena, data_para, (uint32_t)want,
                               cur_psp_owner(), &largest);
    if (rc != MCB_OK) {
        set_ax(f, rc);                          /* 0x0008 / 0x0009 / 0x0007 */
        if (rc == MCB_ERR_INSUFFICIENT) {
            set_bx(f, (uint16_t)(largest & 0xFFFFu));
        }
        cf_set(f);
        return;
    }
    cf_clear(f);
}

/* AH=30h GET VERSION: AL=major(3), AH=minor(30=0x1E), BH=0 (OEM). CF clear.
 * Version 3.30 (ADR-0003 DEC-12 / spec/dos_banner.txt). */
static void do_getver(int_frame_t *f)
{
    f->eax = (f->eax & 0xFFFF0000u)
           | ((uint32_t)INT21_VER_MINOR << 8)   /* AH = minor */
           | (uint32_t)INT21_VER_MAJOR;         /* AL = major */
    f->ebx &= 0xFFFF00FFu;                      /* BH (bits 15:8) = 0 (OEM) */
    cf_clear(f);
}

/* AH=4Ch / AH=00h TERMINATE: route AL through the terminate hook (the kernel
 * emits the exit line + cli;hlt; the host oracle records the code). There is no
 * process model yet, so terminate == stop (ground-truth Sec 6.4/6.5). */
static void do_terminate(int_frame_t *f, uint8_t code)
{
    (void)f;

    /* Reclaim the exiting process's open FILE handles BEFORE control leaves
     * (beads initech-6hk; epic initech-6qy). Real DOS closes all a process's
     * handles on terminate; without this a child that OPENs files and exits
     * (4Ch / 00h / INT 20h) WITHOUT closing them leaks SFT slots, and an EXEC
     * chain exhausts the 16 file slots so a later OPEN fails. g_cur_psp is the
     * CURRENT process: at child-exit time the loader has bound the child's PSP
     * (loader.c: int21_set_psp(plan.psp_addr) before the JMP) and only restores
     * the kernel PSP AFTER g_exit unwinds -- so g_cur_psp is exactly the child
     * whose handles must be freed. The resident device slots 0..3 are preserved
     * (sft_close_process touches only FILE-kind entries). Idempotent + fail-loud
     * on a corrupt JFT (Rule 2). MUST run BEFORE g_exit (which does not return in
     * the kernel build -- it long-jumps back to the loader). */
    sft_close_process(g_cur_psp);

    /* Reclaim the exiting process's MEMORY (beads initech-509.6), symmetric with
     * the JFT handle close above: real DOS frees ALL arena blocks owned by a
     * terminating PSP on exit. Without this, a child that ALLOCs and exits
     * without FREEing leaks its blocks -- and the next program (or kernel-context
     * code) cannot reclaim the window. Only the terminating PSP's blocks are
     * freed (owner-scoped); the kernel restores its own PSP + may re-init the
     * arena afterward. Skipped if no PSP/arena is bound (host default). */
    if (g_arena_bound && g_cur_psp != 0) {
        (void)mcb_free_owner(&g_arena, cur_psp_owner());
    }

    /* Reset the CWD to the root on terminate (beads initech-mzxa; ti8 Layer 2):
     * the exiting process's working directory must not linger into the next
     * program or kernel-context INT 21h. Symmetric with the handle/memory
     * reclaim above. With no CHDIR writer yet this is always already root, but
     * the plumbing is established so initech-u6wa (AH=3Bh) needs no exit change. */
    int21_cwd_reset();

    if (g_exit) {
        g_exit(code);
    }
    /* If no hook is bound (host default), just return; the kernel's hook never
     * returns. We deliberately do NOT touch CF -- 4Ch has no error path. */
}

/* ===========================================================================
 * Cheap resident query functions (beads initech-yv9): SELDISK/GETDISK,
 * DATE/TIME, GET DISK FREE SPACE, GET CWD, GET EXTENDED ERROR, GET PSP. All are
 * single-drive (A: only) / root-only this milestone (no subdirs -- ti8).
 * Ref (Law 1): DOS 3.3 Programmer's Reference Manual, the named function pages;
 *   spec/int21h_register.json (all are in the locked register). The DATE/TIME
 *   functions reach the wall clock through the CLOCK seam (g_clock_get/set ->
 *   the MC146818 RTC, os/milton/rtc.c).
 * ===========================================================================*/

/* ---- the last DOS error (AH=59h GET EXTENDED ERROR; beads initech-yv9) ----
 * Real DOS keeps the most recent INT 21h error so a program can query its
 * CLASS / suggested ACTION / LOCUS after a failed call. This milestone tracks
 * the raw error CODE (AX) set by the failing functions; the class/action/locus
 * are derived from a small table for the codes we actually return. */
static uint16_t g_last_error = 0u;   /* 0 = no error */

void int21_note_error(uint16_t code) { g_last_error = code; }

/* AH=0Eh SELECT DISK: DL = drive (0=A:). Returns AL = number of logical drives.
 * Only A: exists this milestone -> AL=1. DOS has no error path here (an invalid
 * DL is ignored; the default drive is unchanged since only A: exists). Ref: DOS
 * 3.3 PRM AH=0Eh. */
static void do_seldisk(int_frame_t *f)
{
    (void)frame_dl(f);          /* requested drive: ignored (only A: exists) */
    set_al(f, 1u);              /* number of logical drives = 1 (A:)         */
    cf_clear(f);
}

/* AH=19h GET CURRENT DISK: returns AL = current drive (0=A:). Always A: this
 * milestone. No error path. Ref: DOS 3.3 PRM AH=19h. */
static void do_getdisk(int_frame_t *f)
{
    set_al(f, 0u);              /* current drive = A: (0)                    */
    cf_clear(f);
}

/* AH=2Ah GET DATE: CX=year(full), DH=month(1-12), DL=day(1-31), AL=day-of-week
 * (0=Sun). Sourced from the clock seam (RTC). With no clock bound, returns the
 * DOS file-time epoch (1980-01-01, a Tuesday). No error path (DOS 2Ah always
 * succeeds). Ref: DOS 3.3 PRM AH=2Ah. */
static void do_getdate(int_frame_t *f)
{
    uint16_t year = 1980u;
    uint8_t  mon = 1u, day = 1u, hh = 0u, mi = 0u, ss = 0u, dow = 2u;
    if (g_clock_get) {
        (void)g_clock_get(&year, &mon, &day, &hh, &mi, &ss, &dow);
    }
    set_cx(f, year);
    set_dx(f, (uint16_t)(((uint16_t)mon << 8) | (uint16_t)day));
    set_al(f, dow);
    cf_clear(f);
}

/* AH=2Bh SET DATE: CX=year(full), DH=month, DL=day. AL=0 success / 0xFF invalid.
 * Sets ONLY the date portion of the RTC (the time is preserved). Validates
 * ranges via the clock seam's set (rtc_encode range-checks). Ref: DOS 3.3 PRM
 * AH=2Bh. */
static void do_setdate(int_frame_t *f)
{
    uint16_t year = frame_cx(f);
    uint8_t  mon  = frame_dh(f);
    uint8_t  day  = frame_dl(f);
    int ok = 0;
    if (g_clock_set) {
        ok = g_clock_set(year, mon, day, 0u, 0u, 0u, INT21_CLOCK_SET_DATE);
    }
    set_al(f, ok ? 0x00u : 0xFFu);
    cf_clear(f);                /* AL carries the status, not CF (DOS 2Bh)   */
}

/* AH=2Ch GET TIME: CH=hour(0-23), CL=min, DH=sec, DL=centiseconds. The RTC has
 * 1-second resolution, so DL (centiseconds) is always 0 (documented). Sourced
 * from the clock seam. No error path. Ref: DOS 3.3 PRM AH=2Ch. */
static void do_gettime(int_frame_t *f)
{
    uint16_t year = 1980u;
    uint8_t  mon = 1u, day = 1u, hh = 0u, mi = 0u, ss = 0u, dow = 0u;
    if (g_clock_get) {
        (void)g_clock_get(&year, &mon, &day, &hh, &mi, &ss, &dow);
    }
    set_cx(f, (uint16_t)(((uint16_t)hh << 8) | (uint16_t)mi));
    set_dx(f, (uint16_t)(((uint16_t)ss << 8) | 0u));   /* DL=0: 1s resolution */
    cf_clear(f);
}

/* AH=2Dh SET TIME: CH=hour, CL=min, DH=sec, DL=centiseconds(ignored). AL=0/0xFF.
 * Sets ONLY the time portion of the RTC (the date is preserved). Ref: DOS 3.3
 * PRM AH=2Dh. */
static void do_settime(int_frame_t *f)
{
    uint8_t hh = frame_ch(f);
    uint8_t mi = frame_cl(f);
    uint8_t ss = frame_dh(f);
    /* DL (centiseconds) is below our 1s resolution; ignored. */
    int ok = 0;
    if (g_clock_set) {
        /* Year/month/day are required by the seam's validity check but the SET
         * TIME mask ignores them; pass a known-valid placeholder date so the
         * encode's range check passes (it validates the whole struct). */
        ok = g_clock_set(1980u, 1u, 1u, hh, mi, ss, INT21_CLOCK_SET_TIME);
    }
    set_al(f, ok ? 0x00u : 0xFFu);
    cf_clear(f);
}

/* AH=36h GET DISK FREE SPACE: DL=drive (0=default, 1=A:). On success:
 *   AX = sectors per cluster, BX = free clusters, CX = bytes per sector,
 *   DX = total clusters. Invalid drive (not 0/1, or no volume) => AX=0xFFFF.
 * Computed from the mounted FAT12 volume via the file backend's freespace hook
 * (which counts free clusters in the cached FAT). Ref: DOS 3.3 PRM AH=36h. */
static void do_getspace(int_frame_t *f)
{
    uint8_t drive = frame_dl(f);
    if (drive > 1u || g_file == 0 || g_file->freespace == 0) {
        set_ax(f, 0xFFFFu);     /* invalid drive / no volume                 */
        cf_clear(f);            /* 36h has no CF error path; AX=FFFF signals  */
        return;
    }
    uint16_t spc = 0, bps = 0, total = 0, freec = 0;
    if (g_file->freespace(&spc, &bps, &total, &freec) != 0) {
        set_ax(f, 0xFFFFu);
        cf_clear(f);
        return;
    }
    set_ax(f, spc);
    set_bx(f, freec);
    set_cx(f, bps);
    set_dx(f, total);
    cf_clear(f);
}

/* AH=47h GET CURRENT DIR: DL=drive (0=default,1=A:); DS:SI -> 64-byte buffer.
 * Fills the buffer with the CWD path RELATIVE to the root (NO leading '\', NO
 * drive) -- the canonical g_cwd_path (beads initech-mzxa; ti8 Layer 2). Until
 * the CHDIR writer (AH=3Bh, initech-u6wa) lands, g_cwd_path is always the root
 * (an empty string -> a single NUL), but this reads the live g_cwd state so
 * u6wa needs no change here. CF clear on success; AX=0x000F (invalid drive) for
 * a bad drive. Ref: DOS 3.3 PRM AH=47h. */
static void do_getcwd(int_frame_t *f)
{
    uint8_t drive = frame_dl(f);
    if (drive > 1u) {
        set_ax(f, 0x000Fu);     /* invalid drive */
        int21_note_error(0x000Fu);
        cf_set(f);
        return;
    }
    /* ESI = flat ptr to the caller's 64-byte buffer (DS:SI in real DOS). Copy the
     * canonical root-relative CWD (no leading '\', no drive), NUL-terminated and
     * bounded by the 64-byte DOS buffer (Rule 2 -- never overrun). */
    char *buf = (char *)(uintptr_t)f->esi;
    if (buf != 0) {
        uint32_t i = 0u;
        for (; i < INT21_CWD_MAX - 1u && g_cwd_path[i] != '\0'; i++) {
            buf[i] = g_cwd_path[i];
        }
        buf[i] = '\0';
    }
    cf_clear(f);
}

/* AH=3Bh CHDIR (CHANGE CURRENT DIRECTORY): EDX = flat ptr to an ASCIIZ path.
 * Resolve the FULL path to a DIRECTORY via the backend resolve_dir seam (beads
 * initech-u6wa) and, on success, make it the current directory: update
 * g_cwd_start_cluster (the resolved dir's own first data cluster; 0 == root) and
 * g_cwd_path (the backend's canonical root-relative text). A RELATIVE path
 * resolves from the current g_cwd_start_cluster; an ABSOLUTE / drive-prefixed one
 * from the root. An empty path, '\' (root), and '.' are a no-op success (stay).
 * Errors -> CF=1, AX=0x0003 (path not found) for a missing dir OR a CHDIR into a
 * FILE; the AH=59h dispatch wrapper auto-notes CF+AX so no int21_note_error here.
 * The canon is copied under a HARD INT21_CWD_MAX-1 bound: an overlength canon
 * fails loud (0x0003) -- it is NEVER truncated into a wrong CWD (Rule 2). Ref:
 * DOS 3.3 PRM AH=3Bh; PRD Sec 6.5 (DOS path model). */
static void do_chdir(int_frame_t *f)
{
    const char *path = (const char *)(uintptr_t)f->edx;

    if (path == 0) {
        set_ax(f, INT21_ERR_PATH_NOT_FOUND);
        cf_set(f);
        return;
    }

    /* Empty / root '\' / '.' -> stay in the current directory (success). DOS
     * 'CD' with no movement leaves the CWD unchanged. */
    if (path[0] == '\0' ||
        (path[0] == '\\' && path[1] == '\0') ||
        (path[0] == '.'  && path[1] == '\0')) {
        cf_clear(f);
        return;
    }

    if (path_overlength(path)) {
        set_ax(f, INT21_ERR_PATH_NOT_FOUND);     /* runaway / unterminated ptr */
        cf_set(f);
        return;
    }

    if (g_file == 0 || g_file->resolve_dir == 0) {
        /* No backend resolve_dir bound -> root-only: any non-empty/non-root path
         * is unsupported (the pre-u6wa behavior). */
        set_ax(f, INT21_ERR_PATH_NOT_FOUND);
        cf_set(f);
        return;
    }

    uint16_t dir_start = 0u;
    char     canon[INT21_CWD_MAX];
    uint16_t err = g_file->resolve_dir(path, g_cwd_start_cluster, &dir_start,
                                       canon, (uint32_t)sizeof(canon));
    if (err != 0u) {
        set_ax(f, INT21_ERR_PATH_NOT_FOUND);     /* missing dir / not a dir */
        cf_set(f);
        return;
    }

    /* Enforce the INT21_CWD_MAX-1 bound on the WRITE into g_cwd_path: an
     * overlength canon fails loud rather than silently truncating into a wrong
     * CWD (Rule 2). The backend already bounds its write to sizeof(canon) ==
     * INT21_CWD_MAX, so a NUL within bound is guaranteed; the explicit length
     * re-check is belt-and-suspenders. */
    uint32_t clen = 0u;
    while (canon[clen] != '\0') {
        if (clen >= INT21_CWD_MAX - 1u) {
            set_ax(f, INT21_ERR_PATH_NOT_FOUND); /* overlength -> fail loud */
            cf_set(f);
            return;
        }
        clen++;
    }

    /* Commit the new CWD. */
    g_cwd_start_cluster = dir_start;
    {
        uint32_t i = 0u;
        for (; i < clen; i++) {
            g_cwd_path[i] = canon[i];
        }
        g_cwd_path[i] = '\0';
    }
    cf_clear(f);
}

/* AH=59h GET EXTENDED ERROR: returns the most recent error as a (class, action,
 * locus) triple. AX = the error code (0 if none); BH = class, BL = suggested
 * action, CH = locus. We map the small set of codes we actually return; an
 * unknown/zero code reports "no error". DOS clears CL/the high CX byte. Ref: DOS
 * 3.3 PRM AH=59h (the extended-error tables). */
static void do_geterr(int_frame_t *f)
{
    uint16_t code = g_last_error;
    uint8_t cls, act, locus;
    if (code == 0u) {
        cls = 0u; act = 0u; locus = 0u;                 /* no error          */
    } else if (code == INT21_ERR_FILE_NOT_FOUND ||
               code == INT21_ERR_PATH_NOT_FOUND) {
        cls = 0x08u;   /* NOT FOUND        */
        act = 0x03u;   /* USER (prompt/retry) */
        locus = 0x02u; /* BLOCK DEVICE (disk) */
    } else if (code == INT21_ERR_ACCESS_DENIED) {
        cls = 0x0Bu;   /* MEDIA / access   */
        act = 0x04u;   /* ABORT            */
        locus = 0x02u;
    } else if (code == INT21_ERR_CURRENT_DIR) {
        /* RMDIR of the current/root directory (beads initech-u6wa). Same access
         * family + abort action as access-denied (the operation cannot proceed),
         * on the block device (the disk directory). */
        cls = 0x0Bu;   /* MEDIA / access   */
        act = 0x04u;   /* ABORT            */
        locus = 0x02u; /* BLOCK DEVICE (disk) */
    } else if (code == INT21_ERR_INVALID_HANDLE ||
               code == INT21_ERR_TOO_MANY_OPEN) {
        cls = 0x09u;   /* BAD FORMAT / resource */
        act = 0x04u;   /* ABORT            */
        locus = 0x01u; /* UNKNOWN          */
    } else {
        cls = 0x0Du;   /* UNKNOWN error class */
        act = 0x05u;   /* IGNORE           */
        locus = 0x01u;
    }
    set_ax(f, code);
    /* BH = class, BL = action (keep the rest of EBX intact). */
    f->ebx = (f->ebx & 0xFFFF0000u) | ((uint32_t)cls << 8) | (uint32_t)act;
    /* CH = locus; CL = 0 (DOS zeroes it). */
    set_cx(f, (uint16_t)((uint16_t)locus << 8));
    cf_clear(f);                /* 59h itself never fails                    */
}

/* AH=62h GET PSP: returns BX = the current PSP "segment" (paragraph). The flat
 * model stores segments as (linear >> 4) fake paragraphs (psp.h Option B), so BX
 * = (flat PSP address >> 4). No error path. Ref: DOS 3.3 PRM AH=62h. */
static void do_getpsp(int_frame_t *f)
{
    uint16_t psp_seg = 0u;
    if (g_cur_psp != 0) {
        psp_seg = (uint16_t)(((uintptr_t)g_cur_psp >> 4) & 0xFFFFu);
    }
    set_bx(f, psp_seg);
    cf_clear(f);
}

/* ===========================================================================
 * SETVECT / GETVECT (AH=25h / AH=35h; beads initech-509.8, DEC-10).
 * The flat ABI (spec/int21h_calling_convention.json): "All pointer arguments
 * are FLAT 32-bit LINEAR addresses -- no segment:offset"; the function selector
 * is AH and the primary pointer rides EDX. Both functions are in the locked
 * register (spec/int21h_register.json: "25h / 35h SETVECT / GETVECT", Core).
 * On this protected-flat kernel the "interrupt vector" is the IDT gate offset,
 * reached through the vector-table seam (g_setvect / g_getvect) so int21.c stays
 * host-testable. Ref (Law 1): DOS 3.3 PRM AH=25h/35h; the two locked specs.
 * ===========================================================================*/

/* AH=25h SET INTERRUPT VECTOR: AL = vector number, EDX = flat handler address
 * (the DOS DS:DX handler pointer, here a flat 32-bit linear address per the flat
 * ABI). Installs the handler via the seam, which writes the IDT gate offset for
 * vector AL keeping the kernel code selector + 0x8F TRAP type (idt_set_gate ->
 * idt_install_trap). DOS 25h has no error path; CF clear. */
static void do_setvect(int_frame_t *f)
{
    uint8_t  vec     = frame_al(f);
    uint32_t handler = f->edx;          /* flat 32-bit linear handler address */
    if (g_setvect) {
        g_setvect(vec, handler);
    }
    cf_clear(f);                        /* DOS 25h never sets CF */
}

/* AH=35h GET INTERRUPT VECTOR: AL = vector number; returns the handler pointer
 * in ES:BX. On the flat model ES=0 and EBX = the flat handler offset (matching
 * how other functions return a pointer with a zero selector). DOS 35h has no
 * error path; CF clear. */
static void do_getvect(int_frame_t *f)
{
    uint8_t  vec     = frame_al(f);
    uint32_t handler = g_getvect ? g_getvect(vec) : 0u;
#ifdef GETVECT_MUTATE_AX
    /* MUTANT (Rule 6; make test-int24-mutant only): return the handler in EAX
     * instead of EBX -- the WRONG register. DOS 35h returns the vector in ES:BX
     * (EBX in the flat model); a caller reading EBX gets nothing. The [4b] GETVECT
     * oracle (which asserts EBX == the handler) goes RED. NEVER in a real build. */
    f->eax = handler;
    f->ebx = 0u;
#else
    f->ebx = handler;                   /* EBX = flat handler offset (BX low 16) */
#endif
    f->es  = 0u;                        /* ES = 0 in the flat model */
    cf_clear(f);                        /* DOS 35h never sets CF */
}

/* INT 20h legacy terminate (vector 0x20; beads initech-509.5). Routes to the
 * SAME terminate path as 4Ch with exit code 0 (ground-truth Sec 2.1 / Sec 4.4).
 * A program doing `int 0x20` (or a near RET to PSP:0 = the CD 20 there) lands
 * here. With the loader's exit hook bound this does not return (the hook unwinds
 * to load_program); with no hook bound (kernel default before a load) it returns
 * and the stub irets. */
static void int20_dispatch_body(int_frame_t *frame)
{
    do_terminate(frame, 0u);
}

/* ===========================================================================
 * INT 22h / 23h / 24h DOS handlers (beads initech-509.8, ADR-0003 DEC-10).
 * Ref (Law 1): docs/adr/ADR-0003-InitechDOS-Base-OS-Personality.md Sec 5.10
 *   (DEC-10 -- handlers for 24h/23h/22h; 24h presents MSG-DOS-0001 + processes
 *   the operator A/R/F response); App C (MSG-DOS-0001 = "Abort, Retry, Fail?").
 *   The PSP vector save/restore (PSP 0Ah-15h) is a SEPARATE step (loader.c) --
 *   these handlers + their installed IDT gates are what that step reads.
 * ===========================================================================*/

/* PURE A/R/F decision (host-testable seam; no asm/IO). Upcases `ch` then maps to
 * the DOS INT 24h AL action: 'R'->0 Retry, 'A'->1 Abort, 'F'->2 Fail. Any other
 * key returns -1, meaning "re-prompt" -- int24_dispatch loops, re-presenting
 * MSG-DOS-0001 and re-reading until a valid key arrives (deterministic, never a
 * silent default). Ignore=3 is deferred (header note). Ref: DOS 3.3 PRM INT 24h;
 * spec/dos_messages.json MSG-DOS-0001. */
int crit_error_action(int ch)
{
    int c = ch;
    if (c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';          /* upcase (ASCII; Rule 12) */
    }
    switch (c) {
#ifdef CRIT_MUTATE_AF_SWAP
        /* MUTANT (Rule 6; make test-int24-mutant only): SWAP Abort<->Fail so the
         * A/R/F mapping is wrong. The crit_error_action mapping test + the int24
         * AL=1-for-'A' test go RED. NEVER define in a real build. */
        case 'R': return 0;         /* Retry */
        case 'A': return 2;         /* (mutant) Fail instead of Abort */
        case 'F': return 1;         /* (mutant) Abort instead of Fail */
#else
        case 'R': return 0;         /* Retry */
        case 'A': return 1;         /* Abort */
        case 'F': return 2;         /* Fail  */
#endif
        default:  return -1;        /* invalid -> re-prompt */
    }
}

/* INT 22h TERMINATE (vector 0x22). The DOS terminate-return address; in the
 * single-level model the default handler terminates the current program with
 * code 0 -- the SAME path as INT 20h / 4Ch AL=0. Normally non-returning. */
static void int22_dispatch_body(int_frame_t *frame)
{
    do_terminate(frame, 0u);
}

/* INT 23h CONTROL-BREAK (vector 0x23). The DOS default control-break action
 * ABORTS the program, so route to the SAME terminate path as 22h (the break-
 * abort). No keyboard ^C wiring here (beads initech-4tw); this is the default
 * handler the loader points the PSP 0Eh-0Fh vector at. A grep-able marker rides
 * the CON sink (the kernel routes the sink to console + serial; mirrors the
 * controlled-scope diagnostics) so a break-abort is visible in a serial log. */
static void int23_dispatch_body(int_frame_t *frame)
{
    con_puts("INT23-BREAK\n");
    do_terminate(frame, 0u);
}

/* INT 24h CRITICAL ERROR (vector 0x24). Present MSG-DOS-0001 to CON, read ONE
 * operator key (blocking, honoring the 0Bh pushback), and decide the action via
 * crit_error_action. On an invalid key, re-present + re-read (the -1 re-prompt
 * loop). Write the action code into AL and clear CF; do NOT terminate -- 24h
 * RETURNS the action to the failed caller (DOS contract). Nothing calls 24h yet
 * (no driver raises a critical error this milestone), so it just returns. */
static void int24_dispatch_body(int_frame_t *frame)
{
    int action;
    for (;;) {
        int ch;
        con_puts(MSG_DOS_0001);     /* "Abort, Retry, Fail?" (controlled; DEC-13) */
        ch = conin_get_pb();        /* blocking single-char read (respects 0Bh) */
        action = crit_error_action(ch);
#ifdef INT24_MUTATE_NO_REPROMPT
        /* MUTANT (Rule 6; make test-int24-mutant only): accept the FIRST key
         * unconditionally -- no re-prompt loop. An invalid key yields the -1
         * "re-prompt" sentinel straight into AL (and MSG-DOS-0001 prints only
         * once), so the re-prompt oracle [2b] goes RED. NEVER in a real build. */
        break;
#else
        if (action >= 0) {
            break;                  /* valid A/R/F -> done */
        }
        con_putc('\n');             /* invalid key -> newline + re-prompt */
#endif
    }
    set_al(frame, (uint8_t)action); /* AL = 0 Retry / 1 Abort / 2 Fail */
    cf_clear(frame);                /* 24h returns the action, not an error */
}

/* ---- the dispatch spine ---- */
static void int21_dispatch_body(int_frame_t *frame)
{
    uint8_t ah = (uint8_t)((frame->eax >> 8) & 0xFFu);

    switch (ah) {
        case 0x00:                       /* TERMINATE (alias for 4Ch AL=0) */
            do_terminate(frame, 0u);
            return;
        case 0x01:                       /* CHARACTER INPUT WITH ECHO */
            do_conin_echo(frame);
            return;
        case 0x02:                       /* DISPLAY OUTPUT */
            do_putchar(frame);
            return;
        case 0x06:                       /* DIRECT CONSOLE I/O (in if DL=FF) */
            do_direct_conio(frame);
            return;
        case 0x07:                       /* DIRECT CHAR INPUT, no echo, no ^C */
            do_conin_raw(frame);
            return;
        case 0x08:                       /* CHAR INPUT, no echo (^C deferred) */
            do_conin_noecho(frame);
            return;
        case 0x0A:                       /* BUFFERED INPUT */
            do_buffered_input(frame);
            return;
        case 0x0B:                       /* GET INPUT STATUS */
            do_input_status(frame);
            return;
        case 0x0C:                       /* FLUSH KB BUFFER + invoke input */
            do_flush_then_input(frame);
            return;
        case 0x09:                       /* DISPLAY STRING */
            do_puts(frame);
            return;
        case 0x1A:                       /* SET DTA */
            do_setdta(frame);
            return;
        case 0x25:                       /* SET INTERRUPT VECTOR */
            do_setvect(frame);
            return;
        case 0x35:                       /* GET INTERRUPT VECTOR */
            do_getvect(frame);
            return;
        case 0x2F:                       /* GET DTA */
            do_getdta(frame);
            return;
        case 0x30:                       /* GET VERSION */
            do_getver(frame);
            return;
        case 0x3C:                       /* CREAT (create/truncate file) */
            do_creat(frame);
            return;
        case 0x3D:                       /* OPEN (handle) */
            do_open(frame);
            return;
        case 0x3E:                       /* CLOSE (handle) */
            do_close(frame);
            return;
        case 0x3F:                       /* READ (handle) */
            do_read(frame);
            return;
        case 0x40:                       /* WRITE TO FILE/DEVICE */
            do_write(frame);
            return;
        case 0x39:                       /* MKDIR (create directory) */
            do_mkdir(frame);
            return;
        case 0x3A:                       /* RMDIR (remove directory) */
            do_rmdir(frame);
            return;
        case 0x41:                       /* UNLINK (delete file) */
            do_unlink(frame);
            return;
        case 0x42:                       /* LSEEK (move file pointer) */
            do_lseek(frame);
            return;
        case 0x45:                       /* DUP (duplicate handle) */
            do_dup(frame);
            return;
        case 0x46:                       /* DUP2 (force-duplicate handle) */
            do_dup2(frame);
            return;
        case 0x48:                       /* ALLOCATE MEMORY (MCB arena) */
            do_alloc(frame);
            return;
        case 0x49:                       /* FREE ALLOCATED MEMORY */
            do_free(frame);
            return;
        case 0x4A:                       /* SETBLOCK (resize allocation) */
            do_setblock(frame);
            return;
        case 0x4E:                       /* FINDFIRST */
            do_findfirst(frame);
            return;
        case 0x4F:                       /* FINDNEXT */
            do_findnext(frame);
            return;
        case 0x4B:                       /* EXEC (load and execute) */
            do_exec(frame);
            return;
        case 0x4C:                       /* TERMINATE WITH RETURN CODE */
            do_terminate(frame, frame_al(frame));
            return;
        case 0x4D:                       /* GET RETURN CODE (of last EXEC child) */
            do_get_return_code(frame);
            return;
        case 0x0E:                       /* SELECT DISK */
            do_seldisk(frame);
            return;
        case 0x19:                       /* GET CURRENT DISK */
            do_getdisk(frame);
            return;
        case 0x2A:                       /* GET DATE */
            do_getdate(frame);
            return;
        case 0x2B:                       /* SET DATE */
            do_setdate(frame);
            return;
        case 0x2C:                       /* GET TIME */
            do_gettime(frame);
            return;
        case 0x2D:                       /* SET TIME */
            do_settime(frame);
            return;
        case 0x36:                       /* GET DISK FREE SPACE */
            do_getspace(frame);
            return;
        case 0x3B:                       /* CHDIR (change current directory) */
            do_chdir(frame);
            return;
        case 0x47:                       /* GET CURRENT DIR */
            do_getcwd(frame);
            return;
        case 0x59:                       /* GET EXTENDED ERROR */
            do_geterr(frame);
            return;
        case 0x62:                       /* GET PSP */
            do_getpsp(frame);
            return;
        default:
            break;
    }

    /* Controlled scope (Rule 2 / ADR-0003 DEC-13). The AH is not one of the
     * implemented functions. Two distinct, fail-loud diagnostics: */
    if (ah_is_listed(ah)) {
        /* RECOGNIZED by the locked register but not yet implemented in this
         * subset (e.g. FCB ops 0Fh-24h, DATE/TIME 2Ah-2Dh). CON input
         * 01h/06h/07h/08h/0Ah/0Bh/0Ch are now real (beads initech-n62). A
         * distinct diagnostic, NOT 'unknown'. */
        con_puts("INT21 not-yet-impl AH=");
        con_hex2(ah);
        con_putc('\n');
    } else {
#ifdef INT21_MUTATE_UNLISTED_NOOP
        /* MUTANT (Rule 6; make test-int21-mutant only): the unlisted-AH path is
         * a SILENT no-op -- no diagnostic, no CF. The controlled-scope oracle
         * must go RED. NEVER define in a real build. */
        return;
#else
        /* NOT in the locked register at all -> invalid function. Emit the
         * grep-able serial diagnostic (the controlled MSG-DOS-0002 "Bad command
         * or file name" is the closest console message; here we use the clear,
         * specific form per spec/int21h_calling_convention.json). */
        con_puts("INT21 unknown AH=");
        con_hex2(ah);
        con_putc('\n');
#endif
    }

    /* Both controlled-scope paths: CF=1, AX=0x0001 (invalid function). */
    set_ax(frame, INT21_ERR_INVALID_FUNCTION);
    cf_set(frame);
}

/* ---- reentrancy guard + InDOS bracket (beads initech-xk2) ------------------
 * The trap stubs (int21_entry / int20_entry, isr.asm) call these wrappers, NOT
 * the bodies directly. INT 21h is a 0x8F TRAP gate (IF stays set), so an IRQ
 * (PIT IRQ0 / keyboard IRQ1) can be delivered WHILE a syscall is in flight. That
 * is safe today only because the IRQ handlers touch ZERO dispatcher state and
 * never call DOS. The guard ENFORCES that invariant (Rule 2 / Law 2):
 *
 *   - irq_depth() != 0 at entry  =>  an ISR (or a driver it called) issued this
 *     INT 21h -- the FORBIDDEN reentry. Fail loud (dos_reentry_panic) rather than
 *     let it corrupt the interrupted syscall's frame or shared globals (g_dta,
 *     the FINDFIRST search state, g_cur_psp, the FAT cluster scratch). This does
 *     NOT false-fire on EXEC's synchronous child syscalls: a child runs in TASK
 *     context (irq_depth() == 0), only g_indos nests -- which is allowed.
 *
 *   - g_indos brackets the call so dos_in_dos() reports a syscall in flight (the
 *     period-authentic InDOS flag; the documented defer hook for a future driver).
 *
 * The guard is the FIRST thing each wrapper does (before any state is touched),
 * so a reentry is caught before it can corrupt anything. */
void int21_dispatch(int_frame_t *frame)
{
    if (irq_depth() != 0u) {
        dos_reentry_panic();   /* never returns (routes to the panic path) */
    }
    g_indos++;
    int21_dispatch_body(frame);
    /* AH=59h GET EXTENDED ERROR reports the most-recent INT 21h error. Capture
     * it at this single dispatch choke point: every handler that fails returns
     * CF=1 with its INT21_ERR_* code in AX (the universal DOS error convention),
     * so one capture here covers every current and future handler -- closing the
     * bcg.2 gap where read/write/close/lseek/dup/creat/unlink/exec/findfirst/
     * findnext set CF+AX but never called int21_note_error. do_geterr (AH=59h)
     * returns CF=0, so it never clobbers the value it just reported; a
     * successful call (CF=0) leaves the last error intact, as DOS does. */
    if (frame->eflags & CF_BIT) {
        int21_note_error((uint16_t)(frame->eax & 0xFFFFu));
    }
    g_indos--;
}

void int20_dispatch(int_frame_t *frame)
{
    if (irq_depth() != 0u) {
        dos_reentry_panic();   /* never returns */
    }
    g_indos++;
    int20_dispatch_body(frame);
    g_indos--;
}

/* INT 22h/23h/24h share the SAME reentrancy guard + InDOS bracket as INT 21h/20h
 * (beads initech-509.8 / initech-xk2). An ISR must never raise these (it would
 * corrupt an interrupted syscall's frame/globals); the guard fails loud if one
 * does. 22h/23h normally do not return (terminate); 24h returns the A/R/F action
 * -- its g_indos-- runs because do_terminate is not on its path. */
void int22_dispatch(int_frame_t *frame)
{
    if (irq_depth() != 0u) {
        dos_reentry_panic();   /* never returns */
    }
    g_indos++;
    int22_dispatch_body(frame);
    g_indos--;
}

void int23_dispatch(int_frame_t *frame)
{
    if (irq_depth() != 0u) {
        dos_reentry_panic();   /* never returns */
    }
    g_indos++;
    int23_dispatch_body(frame);
    g_indos--;
}

void int24_dispatch(int_frame_t *frame)
{
    if (irq_depth() != 0u) {
        dos_reentry_panic();   /* never returns */
    }
    g_indos++;
    int24_dispatch_body(frame);
    g_indos--;
}
