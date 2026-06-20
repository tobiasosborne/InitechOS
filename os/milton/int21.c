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
#include "memory_map.h"  /* ENV_BLOCK -- the canonical populated-env sentinel (1i0x) */
#include "find_data.h"  /* find_data_t (43-byte DTA block, LOCKED; spec/) */
#include "irq.h"        /* in-IRQ depth + reentrancy guard (beads initech-xk2) */
#include "dos_messages.h" /* MSG_DOS_0001 controlled diagnostic (DEC-13; -Ibuild) */

/* The ANSI.SYS escape-sequence FSM (beads initech-p96i, consuming x3mh). ansi.c
 * is PURE + I/O-free + freestanding (the same shape as the other modules we
 * compile as a single TU), so we #include it DIRECTLY rather than link a
 * separate ansi.o. This keeps the KERNEL link and EVERY int21-linking host
 * oracle (test_int21, test_devwire, ...) building with ZERO Makefile change:
 * ansi.c contributes no separately-linked object, and no other TU in those
 * binaries also pulls ansi.c (test_ansi.c is its own binary). The FSM is driven
 * from con_putc (below) and its actions are applied to the live console through
 * the int21_ansi_console_t seam (int21.h) -- so int21.c still never includes
 * console.h and stays HOSTED-clean (Law 3). */
#include "ansi.c"

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

/* ---- ANSI.SYS CON wiring (beads initech-p96i; consumes the x3mh FSM) -------
 * When the ANSI gate is enabled, con_putc routes every CON output byte through
 * g_ansi_st (the escape-sequence FSM) and applies the resulting actions to the
 * live console through g_ansi_ops. g_ansi_inited guards a one-time ansi_init on
 * first use (so the FSM starts at GROUND with the default attribute).
 *
 * Cursor authority (the x3mh design fork, resolved here): the CONSOLE owns the
 * real cursor (it advances/wraps/scrolls on every put_char, which the pure FSM
 * cannot predict). The apply layer therefore keeps the console authoritative:
 * ESC[s save / ESC[6n DSR read the LIVE console cursor (via ops->get_cursor),
 * and ESC[u restore drives ops->set_cursor to the apply-layer save slot below.
 * The FSM's own row/col/saved_row/saved_col are NOT trusted for positioning --
 * only g_ansi_saved_{row,col} (read from the console at ESC[s) are. */
static ansi_state_t       g_ansi_st;
static int                g_ansi_inited = 0;
static int21_ansi_console_t g_ansi_ops  = { 0, 0, 0, 0, 0, 0, 0, 0 };
static int21_ansi_gate_fn g_ansi_gate   = 0;
/* The apply-layer cursor save slot (ESC[s/ESC[u); the console is the truth. */
static int                g_ansi_saved_row = 0;
static int                g_ansi_saved_col = 0;

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

/* The character-device chain seam (beads initech-6zd9; ADR-0003 DEC-09). The
 * resident device chain (devices.c: NUL/CON/AUX/PRN/CLOCK$) wired into the
 * AH=3Dh OPEN-by-name path and the AH=3Fh/40h device READ/WRITE routing. NULL
 * until the kernel binds it at SYSINIT (or the host oracle binds it). With no
 * chain bound, OPEN-by-name is disabled (a device NAME open falls through to the
 * FAT path) and no device-bound SFT slot can exist -- so the legacy CON/AUX/PRN
 * slots 0..3 are entirely unaffected (the CON output path is preserved exactly).
 * g_devio is a COPY of the io bundle (its lifetime is independent of the caller);
 * g_devio_bound gates it (an unbound bundle leaves every device callback NULL ->
 * the handler sets the device error bit, mapped to CF=1, never a fault). */
static device_header_t *g_devchain     = 0;
static devices_io_t     g_devio        = { 0 };
static int              g_devio_bound  = 0;

/* Forward declarations (beads initech-6zd9): do_write/do_read (defined ABOVE the
 * OPEN-by-name device routing block) reach the chain through these. The full
 * definitions live with the file-handle functions further down. */
static device_header_t *dev_open_lookup(const char *path);
static uint16_t dev_route_rw(device_header_t *dev, int is_write,
                             uint8_t *buf, uint32_t len, uint32_t *out_count);

/* The file backend (beads initech-509.5 read-side). NULL until the kernel binds
 * a FAT12-backed impl (or the host oracle binds a mock). The file functions
 * resolve FAT-specific work through it so int21.c stays host-testable. */
static const int21_file_backend_t *g_file = 0;

/* The ABSOLUTE-DISK block-device seam (INT 25h/26h; ADR-0003 DEC-15, beads
 * initech-4mq7). A COPY of the vtable the kernel binds from the mounted FAT12
 * volume's blockdev (vol->dev->read_sectors/write_sectors) -- int21.c never
 * includes fat12.h/blockdev.h or references the volume (Law 3). g_absdisk_bound
 * gates it: with NO seam bound INT 25h/26h fail loud (CF=1), never fault. The
 * copy keeps the seam alive independent of the caller's struct lifetime. */
static int21_absdisk_backend_t g_absdisk = { 0, 0, 0u };
static int                     g_absdisk_bound = 0;

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

/* The TERMINATION TYPE the last terminating process reported, retrievable as the
 * AH high byte of AH=4Dh GET RETURN CODE. DOS 3.3 types: 0 = normal (4Ch / INT
 * 20h / 00h), 1 = Ctrl-Break, 2 = critical error, 3 = TSR (terminated via AH=31h
 * KEEP). We produce 0 (normal) and 3 (KEEP); 1/2 have no termination path yet. Set
 * to 3 by do_keep and reset to 0 by do_exec on a normal child run, so a parent's
 * 4Dh reports AH=3 exactly when the child KEEP'd. Ref: DOS 3.3 PRM AH=4Dh (AH =
 * exit type); RBIL INT 21/AH=4Dh; RBIL INT 21/AH=31h (KEEP -> 4Dh AH=3). */
static uint8_t g_last_child_term_type = 0;

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

/* ---- the system-wide CTRL-BREAK flag (beads initech-er3h; ADR-0003 Amendment
 * DEC-16, OEA-ADR-0003-A4, RATIFIED 2026-06-15) -----------------------------
 * The single source of truth read by AH=33h AL=00h GET and written by AH=33h
 * AL=01h SET, by CONFIG.SYS BREAK= at SYSINIT, and by the BREAK shell built-in.
 * Boot default ON (1): DOS 3.3 defaults BREAK ON per the DOS 3.3 PRM (DEC-16
 * Sec 3.3; C-7 86Box-confirmation obligation -- editorial-erratum flippable).
 * 4tw's ^C check-point reads this via int21_get_break_flag (the CON-input
 * functions are always a check-point under Fork A; the ON-widening to every
 * INT 21h call is the C-6 forward obligation). This bead provides ONLY the flag
 * + the seam; it adds NO ^C detection. */
#if defined(INT21_MUTATE_BREAK_DEFAULT_OFF)
/* MUTANT M6 (Rule 6; make test-er3h-mutant only): boot the flag OFF instead of
 * ON, so the [er3h.1] boot-default GET assertion goes RED. NEVER in a real
 * build -- DEC-16 Sec 3.3 fixes the default ON. */
static uint8_t g_break_flag = 0u;
#else
static uint8_t g_break_flag = 1u;   /* DOS 3.3 boot default ON (DEC-16 3.3) */
#endif

/* SET the BREAK flag, NORMALIZING any non-zero to 1 (DEC-16 Sec 3.2 -- store the
 * boolean, not the raw byte, so a later GET returns exactly 0 or 1). The public
 * seam CONFIG.SYS SYSINIT and the BREAK built-in call. */
void int21_set_break_flag(uint8_t on)
{
    g_break_flag = (uint8_t)(on != 0u ? 1u : 0u);
}

/* GET the current BREAK flag (0 = OFF, 1 = ON). AH=33h AL=00h and the BREAK
 * built-in's bare-form report read this; 4tw's ^C check-point will too. */
uint8_t int21_get_break_flag(void)
{
    return g_break_flag;
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

/* Bind the INT 25h/26h absolute-disk block-device seam (ADR-0003 DEC-15, beads
 * initech-4mq7). COPY the vtable so its lifetime is independent of the caller's
 * struct; NULL clears it -> every absolute I/O fails loud (CF=1). The kernel
 * binds it from the mounted volume's blockdev only on the mounted==1 path; the
 * host oracle binds a mock file-backed blockdev. */
void int21_set_blockdev(const int21_absdisk_backend_t *backend)
{
    if (backend == 0) {
        g_absdisk.read = 0;
        g_absdisk.write = 0;
        g_absdisk.total_sectors = 0u;
        g_absdisk_bound = 0;
        return;
    }
    g_absdisk = *backend;
    g_absdisk_bound = 1;
}

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

/* PROGRAM-DISJOINT ARENA BIND (beads initech-1q4u; ADR-0009 DEC-04). Bind the
 * heap arena to [arena_base_linear, arena_ceil_linear) and hand it to the current
 * PSP. The loader computes the window from the LOADED image so the heap is
 * provably disjoint from the program image+BSS / env / stack (spec/memory_map.h
 * ARENA DISJOINTNESS INVARIANT). This SUPERSEDES the whole-window int21_mcb_reset
 * bind that overlaid the running program.
 *
 * Fail loud (Rule 2): a degenerate window (ceil <= base, or fewer than 2
 * paragraphs) leaves the arena UNBOUND (int21_set_mcb_arena's own < 2 guard) so a
 * 48h ALLOC returns insufficient memory rather than a corrupting overlap. We also
 * reject a non-paragraph-aligned base here (the caller must round up) -- a mis-
 * aligned base would shift every reported DOS segment. */
int int21_mcb_bind_program(uint32_t arena_base_linear, uint32_t arena_ceil_linear)
{
    /* Degenerate / inverted window, or a base not paragraph-aligned: unbind. */
    if (arena_ceil_linear <= arena_base_linear ||
        (arena_base_linear & 0xFu) != 0u) {
        int21_set_mcb_arena(0, 0u, 0u);   /* UNBOUND -> 48h reports insufficient */
        return 0;
    }

    uint32_t total_paras = (arena_ceil_linear - arena_base_linear) / 16u;
    /* < 2 paragraphs (no room for a header + one data paragraph) -> int21_set_mcb_
     * arena leaves it unbound; mirror that as the "not bound" return. */
    if (total_paras < 2u) {
        int21_set_mcb_arena(0, 0u, 0u);
        return 0;
    }

    /* Bind the buffer over the computed disjoint window and leave it as ONE FREE
     * terminal block (int21_set_mcb_arena's mcb_init lays exactly that). The arena
     * is the DISJOINT free region ABOVE the program image+BSS (DEC-04): unlike the
     * old overlay model -- where the program OWNED its whole window and had to
     * AH=4Ah SETBLOCK-shrink before AH=48h could carve a tail -- the disjoint arena
     * is NOT the program's image block, so there is nothing for the program to
     * shrink. It must be left FREE so the program's first AH=48h allocates straight
     * from it (the allocation is then stamped with the program's PSP owner by
     * mcb_alloc). Handing the whole arena to the PSP up front (int21_mcb_reset)
     * would leave zero FREE space, and a heap-using program (SAMIR) that -- per its
     * documented contract -- does NOT SETBLOCK first would get AH=48h "insufficient
     * memory" and panic. So we deliberately do NOT call int21_mcb_reset here.
     * Ref: ADR-0009 DEC-04 ("the arena's FREE region"); pal_milton.c pal_milton_make
     * ("AH=48h draws straight from it; no AH=4Ah SETBLOCK first"); the S8.2 in-emu
     * gate (bead hdlb) forces this real allocation. */
    int21_set_mcb_arena((void *)(uintptr_t)arena_base_linear, total_paras,
                        arena_base_linear);
    return g_arena_bound;   /* 1 if int21_set_mcb_arena bound it (>= 2 paras), else 0 */
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

/* Bind the character-device chain + its I/O bundle (beads initech-6zd9). The io
 * bundle is COPIED so the caller need not keep it alive; a NULL io clears the
 * bundle (every device callback then reads as "not present"). The chain head may
 * be NULL to disable OPEN-by-name. The kernel binds devices_head() + an io bundle
 * routed to the real console/keyboard/RTC at SYSINIT; the host oracle binds a
 * mock chain + stub callbacks. */
void int21_set_devices(device_header_t *chain, const devices_io_t *io)
{
    g_devchain = chain;
    if (io == 0) {
        /* Clear the bundle: a zeroed devices_io_t has every callback NULL, which
         * the devices.c handlers treat as "device not present" -> error bit. */
        for (uint32_t i = 0u; i < (uint32_t)sizeof(g_devio); i++) {
            ((uint8_t *)&g_devio)[i] = 0u;
        }
        g_devio_bound = 0;
        return;
    }
    g_devio = *io;          /* struct copy -- independent of the caller's lifetime */
    g_devio_bound = 1;
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

/* ---- ANSI.SYS bind points + the apply layer (beads initech-p96i) ---------- */

void int21_set_ansi_console(const int21_ansi_console_t *ops)
{
    if (ops == 0) {
        /* Clear: a zeroed table has every callback NULL -> each action a no-op,
         * and ansi_apply's PUT_CHAR fallback (raw sink) still renders text. */
        for (uint32_t i = 0u; i < (uint32_t)sizeof(g_ansi_ops); i++) {
            ((uint8_t *)&g_ansi_ops)[i] = 0u;
        }
        return;
    }
    g_ansi_ops = *ops;   /* struct copy -- independent of the caller's lifetime */
}

void int21_set_ansi_gate(int21_ansi_gate_fn gate)
{
    g_ansi_gate = gate;
}

/* Is the ANSI escape interpreter active for the CON output path?  EXTRACTED so
 * the host oracle can test the gate decision directly (Law 2). True iff the
 * gate is bound AND returns non-zero (CONFIG.SYS loaded DEVICE=ANSI.SYS). When
 * UNBOUND (the default) this is false, so con_putc takes the raw-sink path that
 * is byte-for-byte identical to before this bead. */
static int con_ansi_active(void)
{
#ifdef ANSIWIRE_MUTATE_IGNORE_GATE
    /* MUTANT (Rule 6; make test-ansi-wire-mutant): IGNORE the gate and always
     * feed the FSM. The "ANSI OFF renders escape bytes literally" oracle then
     * goes RED (the FSM swallows the escape bytes instead of passing them to the
     * sink). One-branch RUNTIME perturbation; compiles clean under -Werror.
     * NEVER in a real build -- without DEVICE=ANSI.SYS, DOS prints bytes raw. */
    return 1;
#else
    return (g_ansi_gate != 0) && (g_ansi_gate() != 0);
#endif
}

/* Apply one ANSI FSM action to the live console (through g_ansi_ops). This is
 * the seam between the PURE FSM and the console driver. `ctx` is unused (the
 * console handle travels inside g_ansi_ops.ctx); the signature matches
 * ansi_action_cb_t. Ref: MS-DOS 3.3 Tech Ref Ch 4 "ANSI.SYS" effect table. */
static void ansi_apply(const ansi_action_t *act, void *ctx)
{
    (void)ctx;
    switch (act->kind) {

    case ANSI_ACT_PUT_CHAR:
        /* Draw the glyph with the action's attribute, then advance (the console
         * op also mirrors to serial -- see kmain's binding). If no console op is
         * bound (host default), fall back to the raw sink so plain text still
         * renders and serial still sees the byte. The FSM only emits PUT_CHAR for
         * non-escape bytes, so with NO escapes in the stream this is identical to
         * the old con_putc -> g_sink path (CLAUDE.md Law 4). */
        if (g_ansi_ops.put_char) {
            g_ansi_ops.put_char(g_ansi_ops.ctx, act->ch, act->attr);
        } else if (g_sink) {
            g_sink((char)act->ch);
        }
        break;

    case ANSI_ACT_MOVE_CURSOR:
        if (g_ansi_ops.set_cursor) {
            g_ansi_ops.set_cursor(g_ansi_ops.ctx, act->row, act->col);
        }
        break;

    case ANSI_ACT_CURSOR_REL:
        if (g_ansi_ops.cursor_rel) {
            g_ansi_ops.cursor_rel(g_ansi_ops.ctx, act->delta_row, act->delta_col);
        }
        break;

    case ANSI_ACT_SAVE_CURSOR:
        /* Cursor authority: read the LIVE console cursor (not the FSM's, which is
         * stale after plain text advanced it) into the apply-layer save slot. */
        if (g_ansi_ops.get_cursor) {
            g_ansi_ops.get_cursor(g_ansi_ops.ctx, &g_ansi_saved_row, &g_ansi_saved_col);
        }
        break;

    case ANSI_ACT_RESTORE_CURSOR:
        /* Drive the console back to the saved slot (NOT the FSM's saved value). */
        if (g_ansi_ops.set_cursor) {
            g_ansi_ops.set_cursor(g_ansi_ops.ctx, g_ansi_saved_row, g_ansi_saved_col);
        }
        break;

    case ANSI_ACT_ERASE_DISPLAY:
        if (g_ansi_ops.erase_display) {
            g_ansi_ops.erase_display(g_ansi_ops.ctx, act->erase_mode);
        }
        break;

    case ANSI_ACT_ERASE_LINE:
        if (g_ansi_ops.erase_line) {
            g_ansi_ops.erase_line(g_ansi_ops.ctx, act->erase_mode);
        }
        break;

    case ANSI_ACT_SET_ATTR:
        if (g_ansi_ops.set_attr) {
            g_ansi_ops.set_attr(g_ansi_ops.ctx, act->attr);
        }
        break;

    case ANSI_ACT_DEVICE_STATUS:
        /* ESC[6n Device Status Report. The response (ESC[<row+1>;<col+1>R) must
         * be injected into the CON INPUT queue so a program reading stdin sees
         * it. There is no keyboard-INJECT seam yet (g_conin_* are READ-only), so
         * this is a DEFERRED follow-up: we read the live cursor (proving the data
         * path) but drop the response. Wiring it needs a kbd-inject seam bound by
         * the kernel. Ref: MS-DOS 3.3 Tech Ref Ch 4 "DSR" / RBIL ANSI.SYS 6n. */
        /* (deferred -- see bd follow-up) */
        break;

    case ANSI_ACT_BELL:
        /* BEL: no PC speaker driver wired into CON yet; drop (the byte was
         * consumed). A serial/speaker tap is a deferred nicety, not a blocker. */
        break;

    case ANSI_ACT_KEY_REMAP:
        /* ESC[...p keyboard reassignment: deferred (no key-remap table yet). */
        break;

    case ANSI_ACT_NONE:
    default:
        break;
    }
}

/* ---- the CON sink fan-out (NULL-safe) ---- */
static void con_putc(char c)
{
    if (con_ansi_active()) {
        /* ANSI.SYS loaded: route the byte through the escape-sequence FSM, whose
         * actions are applied to the live console (and serial, via the put_char
         * op) by ansi_apply. Plain (non-escape) bytes become a single PUT_CHAR
         * action -> the SAME console_putc + serial path as the raw sink, so a
         * stream WITHOUT escapes renders byte-for-byte identically (Law 4). */
        if (!g_ansi_inited) {
            /* One-time init at first use. Dimensions match the canonical 80x25
             * console (console.h CONSOLE_COLS/ROWS); the FSM uses them only to
             * clamp absolute moves, and the console clamps again defensively. */
            ansi_init(&g_ansi_st, 25, 80);
            g_ansi_inited = 1;
        }
        ansi_feed(&g_ansi_st, (uint8_t)c, ansi_apply, 0);
        return;
    }
    /* ANSI not loaded: the original path, untouched (byte-identical to before
     * this bead). con_ansi_active() is false whenever the gate is unbound. */
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
static uint16_t frame_dx(const int_frame_t *f) { return (uint16_t)(f->edx & 0xFFFFu); }
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
        case 0x33:                       /* BREAK (Get/Set CTRL-BREAK; DEC-16) */
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

/* The CON-input ^C (0x03) check-point (beads initech-4tw; ADR-0003 Amendment
 * DEC-16, OEA-ADR-0003-A4, RATIFIED 2026-06-15, Sec 3.3 Fork A + Sec 7.2).
 *
 * Returns 1 and INVOKES the INT 23h break vector if `c` is Ctrl-C (0x03); the
 * caller must then NOT deliver 0x03 as an ordinary char. Returns 0 for any other
 * byte. Driven by the ^C-checking CON calls AH=01h / AH=08h / AH=0Ah ONLY -- the
 * DIRECT calls AH=07h (no Ctrl-C) and AH=06h are the documented exceptions and do
 * NOT call this (DOS 3.3 PRM AH=07h "no Ctrl-C"; int21.c do_conin_raw).
 *
 * FORK A GATING (DEC-16 Sec 3.3, the binding consumer obligation): under the
 * ratified Fork A the CON character-input family is ALWAYS a ^C check-point --
 * when BREAK is OFF, CON-I/O is the *only* check-point; when ON, it is one of
 * many (the ON-widening to every INT 21h call is the C-6 forward obligation, NOT
 * landed here). So the CON ^C detection fires whether g_break_flag is 0 or 1. We
 * still READ g_break_flag (per DEC-16 Sec 3.3 (ii) "read g_break_flag rather than
 * hard-coding") so the flag is a live input and the seam is ready for C-6: the
 * `(g_break_flag, ON or OFF)` -> CON-checks-^C truth is the Fork-A row. When the
 * non-CON every-call check-points are added (C-6) THEY are the ones gated to fire
 * only when g_break_flag is ON; the CON path stays unconditional here.
 *
 * int23_dispatch is the existing default break handler (DEC-10 / initech-509.8);
 * its default action terminates the current program (the DOS Ctrl-Break abort).
 * Ref: DOS 3.3 PRM AH=01h/08h/0Ah, INT 23h; ADR-0003 Amendment DEC-16 Sec 3.3. */
static int conin_check_ctrlc(int c, int_frame_t *f)
{
#ifdef INT21_MUTATE_NO_CTRLC_CHECK
    /* MUTANT M5 (Rule 6; make test-4tw-mutant only): drop the ^C check so 0x03 is
     * delivered as a raw char and INT 23h is NEVER invoked -> the [4tw.*] oracle
     * goes RED. NEVER define in a real build -- DEC-16 Sec 3.3 mandates the CON
     * check-point. */
    (void)c; (void)f;
    return 0;
#else
    /* Read the BREAK flag (live input / C-6-ready seam; see header). Under Fork A
     * the CON family check-points whether it is ON or OFF, so the value does not
     * suppress the CON detection -- it is consulted, not a gate, on this path. */
    uint8_t brk = int21_get_break_flag();
    (void)brk;
    if ((c & 0xFF) == 0x03) {            /* ^C (Ctrl-C) */
        int23_dispatch(f);              /* the DOS Ctrl-Break abort (DEC-10) */
        return 1;
    }
    return 0;
#endif
}

/* AH=01h CHARACTER INPUT WITH ECHO: block for one char, echo it to CON, AL=char.
 * (Ctrl-C check deferred -- see header note.) CF is not part of this function's
 * contract; success leaves it clear. Ref: DOS 3.3 PRM AH=01h.
 *
 * 4tw HOOK (beads initech-4tw; ADR-0003 Amendment DEC-16 Sec 3.3 Fork A): this
 * is a CON-input ^C check-point. 4tw lands the 0x03 -> INT 23h detection HERE
 * (and in do_conin_noecho / do_buffered_input), reading the BREAK state via
 * int21_get_break_flag() (the seam this bead, initech-er3h, provides). Under
 * Fork A the CON family is ALWAYS a check-point (BREAK ON or OFF); the
 * ON-widening to every INT 21h call is the C-6 forward obligation. This bead
 * adds NO ^C logic -- only the flag + the seam. */
static void do_conin_echo(int_frame_t *f)
{
    int c = conin_get_pb();
    /* ^C check-point (beads initech-4tw; DEC-16 Sec 3.3 Fork A): a 0x03 routes to
     * the INT 23h break handler instead of being delivered/echoed as a char. The
     * CON family is ALWAYS a check-point under Fork A (BREAK ON or OFF). If the
     * default break action returns (host: no exit hook), we still do NOT deliver
     * 0x03 -- there is nothing to return after a break. */
    if (conin_check_ctrlc(c, f)) {
        return;
    }
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

/* AH=08h CHARACTER INPUT, NO ECHO (WITH Ctrl-C check): block for one char,
 * AL=char, no echo. UNLIKE 07h, 08h IS a ^C check-point (beads initech-4tw;
 * DEC-16 Sec 3.3 Fork A): a 0x03 routes to the INT 23h break handler rather than
 * being delivered in AL. Ref: DOS 3.3 PRM AH=08h ("with Ctrl-C check"). */
static void do_conin_noecho(int_frame_t *f)
{
    int c = conin_get_pb();
    if (conin_check_ctrlc(c, f)) {
        return;
    }
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

/* SHARED COOKED LINE EDITOR. Reads one line of cooked CON input into `out`
 * (capacity `cap` CHARS -- the line text only, NOT the terminating CR/LF),
 * echoing each char as DOS does: ordinary chars echo; BACKSPACE (0x08) erases
 * the last buffered char visually ("\b \b") or is ignored when empty; CR (0x0D)
 * terminates the line. On termination the CR+LF pair is echoed (the DOS Enter
 * convention) and the stored char count is returned. Once `out` is full
 * (count == cap) a further non-CR char is dropped with a BEL -- the line is NOT
 * overflowed (Rule 2). A stray LF is an ordinary stored char: the kbd decodes
 * Enter to CR directly, so CR is the ONLY terminator (initech-62m, Rule 3).
 *
 * This is the editor BOTH AH=0Ah (do_buffered_input) and AH=3Fh-on-CON
 * (do_read's CON device leg) drive -- they differ only in how they LAY OUT the
 * returned line (the buf[0]/buf[1]/CR format vs. line+CR+LF inclusive), not in
 * the editing. Factoring it here keeps the two callers in lockstep and shrinks
 * the later Ctrl-C surface (beads initech-4tw hooks the read inside here).
 *
 * ^C check-point (beads initech-4tw; DEC-16 Sec 3.3 Fork A): AH=0Ah / AH=3Fh-on-
 * CON are ^C-checking calls. When 0x03 is read mid-line the editor invokes INT
 * 23h (conin_check_ctrlc), sets *broke=1, and returns -- aborting the line. In
 * the kernel int23's default action terminates and never returns; *broke lets the
 * host caller (and any non-terminating future break handler) skip the normal line
 * layout. `f` is the trap frame threaded through for int23_dispatch. `broke` may
 * be NULL only if the caller cannot reach a ^C (none today; both callers pass it).
 * Ref: DOS 3.3 PRM AH=0Ah; Microsoft KB Q113058 (AH=3Fh CON read);
 * os/milton/kbd.c SC1_NORMAL[0x1C] == '\r'. */
static uint8_t conin_cooked_line(uint8_t *out, uint8_t cap, int_frame_t *f, int *broke)
{
    uint8_t count = 0;          /* chars stored so far (excl. CR/LF) */

    if (broke) {
        *broke = 0;
    }

    /* Rule 2 (host-safe, NEVER hang): with no CON input source bound and no
     * pending pushback, a cooked read yields an empty line (EOF) instead of
     * spinning forever on conin_get()'s no-source 0 sentinel (which is neither
     * CR nor BACKSPACE, so the loop below would store it / BEL endlessly). The
     * kernel binds the keyboard at SYSINIT before any program runs, so this
     * guard is never taken on a real boot; it only protects host oracles that
     * link int21.c without a conin source (e.g. test_fileio reaching a CON read
     * via a mutated CREATNEW path -- initech-x8fs integration). */
    if (!g_conin_get && g_conin_pushback < 0) {
        return 0;
    }

    for (;;) {
        int ci = conin_get_pb();
        uint8_t c = (uint8_t)ci;

        /* ^C check-point: 0x03 routes to INT 23h and aborts the line (DEC-16 Sec
         * 3.3 Fork A; beads initech-4tw). Checked BEFORE CR/BACKSPACE/store so a
         * Ctrl-C never lands in the line buffer. */
        if (conin_check_ctrlc(c, f)) {
            if (broke) {
                *broke = 1;
            }
            return count;
        }

        if (c == 0x0Du) {                       /* CR: terminate the line */
            con_putc('\r');                     /* echo CR + LF (DOS convention) */
            con_putc('\n');
            return count;
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

        /* Ordinary char. Room for it only if count < cap. */
        if (count < cap) {
            out[count] = c;
            count++;
            con_putc((char)c);                  /* echo */
        } else {
            /* Buffer full: ignore further non-CR chars, beep (BEL). Rule 2: do
             * NOT overflow the caller's buffer. */
            con_putc('\a');
        }
    }
}

/* AH=0Ah BUFFERED INPUT: EDX -> a buffer where byte0 (caller-set) is the maximum
 * length INCLUDING the terminating CR, byte1 is the count WE write (chars read,
 * NOT counting the CR), and byte2.. are the chars, terminated by a CR (0x0D)
 * which IS stored but NOT counted. The cooked editing (echo, BACKSPACE, CR
 * terminate, BEL-on-full) is the shared conin_cooked_line editor; this routine
 * only applies the 0Ah LAYOUT: chars at buf[2..], length at buf[1], CR after the
 * chars. The char capacity is max-1 (reserve one slot for the CR).
 * Ref: DOS 3.3 PRM AH=0Ah. */
static void do_buffered_input(int_frame_t *f)
{
    uint8_t *buf = (uint8_t *)(uintptr_t)f->edx;
    if (buf == 0) {
        cf_clear(f);            /* nothing we can do; fail safe (no fault) */
        return;
    }

    uint8_t max = buf[0];       /* max length incl. the CR (caller-set) */
    /* Char capacity = max-1 (reserve a slot for the CR); a max of 0 or 1 leaves
     * no room for any char (max==1 -> room only for the CR; max==0 is degenerate
     * -- the editor still reads to the CR but stores nothing). */
    uint8_t cap = (max >= 1u) ? (uint8_t)(max - 1u) : 0u;
    int broke = 0;
    uint8_t count = conin_cooked_line(&buf[2], cap, f, &broke);

    /* ^C aborted the line (beads initech-4tw; DEC-16 Sec 3.3 Fork A): INT 23h has
     * already fired. In the kernel int23's default action terminated and never
     * returned here; if a (future, non-terminating) break handler DID return, the
     * 0Ah buffer is left as-is and we do NOT lay out a CR/count for an aborted
     * line. Return clean (CF clear). */
    if (broke) {
        cf_clear(f);
        return;
    }

    /* Store the CR after the chars if there is room (real DOS always stores it at
     * buf[2+count]); the CR is NOT counted in buf[1]. `count` is capped at max-1
     * by the editor's capacity, so `count < max` holds for any max>=1. The old
     * (uint8_t)(2u+count) < (uint8_t)(2u+max) form WRAPPED when max>=254 and
     * wrongly dropped the terminator on a full-size (buf[0]=255) buffer. */
#ifdef INT21_MUTATE_BUFINPUT_CR_WRAP
    /* MUTANT (Rule 6; make test-conin-mutant only): restore the old
     * uint8_t-wrapping guard so a max=255 buffer drops its CR -- the full-size-
     * buffer oracle must go RED. NEVER define in a real build. */
    if ((uint8_t)(2u + count) < (uint8_t)(2u + max)) {
#else
    if (count < max) {
#endif
        buf[2u + count] = 0x0Du;
    }
#ifdef INT21_MUTATE_BUFINPUT_COUNT_CR
    /* MUTANT (Rule 6; make test-conin-mutant only): count the CR in the length
     * byte (count+1 instead of count), so the 0Ah oracle -- which asserts buf[1]
     * == chars read NOT counting the CR -- goes RED. NEVER define in a real build. */
    buf[1] = (uint8_t)(count + 1u);
#else
    buf[1] = count;
#endif
    cf_clear(f);
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

/* ---- AUX / PRN legacy character-I/O (AH=03h/04h/05h; beads initech-40oq) ---
 * AH=03h AUX INPUT:    AL = byte read from AUX (COM1). CF clear.
 * AH=04h AUX OUTPUT:   DL = byte sent to AUX (COM1). CF clear.
 * AH=05h PRINT OUTPUT: DL = byte sent to PRN (LPT1). CF clear.
 *
 * These three functions are the character-I/O entry points for the AUX (serial
 * COM1) and PRN (parallel LPT1) devices. They route through the g_devio bundle
 * (int21_set_devices / dev_build_io) exactly as the device-chain read/write
 * path does for OPEN-by-name AUX/PRN handles (reuse the same seam; no new
 * I/O path needed). If the seam is not bound (g_devio_bound == 0, the host-
 * oracle default), the call returns 0x00 for reads and discards writes -- silent
 * no-data / no-op is the correct host-safe behaviour (the equivalent of an
 * unbound null device). CF is clear in all cases: DOS 3.3 PRM AH=03h/04h/05h
 * define no error path. Ref: DOS 3.3 Programmer's Reference Manual AH=03h/04h/05h.
 *
 * CERT_MUTATE_DROP_GETVER (beads initech-40oq mutant): that guard wraps the
 * AH=30h case, not these. These handlers are unconditional.
 *
 * NOTE: adding these dispatch stubs is what closes the 40oq cert (the only
 * remaining not-yet-impl AHs in the Appendix-A Core+Resident scope after
 * Waves 1-4 + the per-driver beads). The device seam (g_devio) already exists
 * (initech-509.7, closed). */
static void do_aux_input(int_frame_t *f)
{
    uint8_t c = 0u;
    if (g_devio_bound && g_devio.aux_read) {
        /* One-byte blocking read from AUX through the kernel-bound seam.
         * The seam returns a count; a return <= 0 means no data -> deliver 0
         * (no error path in DOS AH=03h). */
        (void)g_devio.aux_read(&c, 1, g_devio.aux_ctx);
    }
    set_al(f, c);
    cf_clear(f);
}

static void do_aux_output(int_frame_t *f)
{
    uint8_t c = frame_dl(f);
    if (g_devio_bound && g_devio.aux_write) {
        (void)g_devio.aux_write(&c, 1, g_devio.aux_ctx);
    }
    cf_clear(f);
}

static void do_prn_output(int_frame_t *f)
{
    uint8_t c = frame_dl(f);
    if (g_devio_bound && g_devio.prn_write) {
        (void)g_devio.prn_write(&c, 1, g_devio.prn_ctx);
    }
    cf_clear(f);
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
     * regardless of the slot's nominal mode (DOS treats CON as writable). This is
     * the LEGACY predefined-CON fast path (slots 0..3, device == NULL) -- the most
     * load-bearing output path in the OS (the shell prompt, every diagnostic). It
     * is PRESERVED BYTE-FOR-BYTE: an OPEN-by-name CON slot (device != NULL) does
     * NOT match here (its dev_id is not SFT_DEV_CON) and takes the chain route
     * below instead, which routes con_write back to this same console sink. */
    if (e->kind == SFT_KIND_DEVICE && e->dev_id == SFT_DEV_CON) {
        for (uint32_t i = 0; i < count; i++) {
            con_putc(buf[i]);
        }
        f->eax = count;   /* full EAX = bytes written */
        cf_clear(f);
        return;
    }

    /* OPEN-by-name device write (beads initech-6zd9): a SFT slot bound to a
     * resident device (CON/NUL/PRN/AUX/CLOCK$ via AH=3Dh) routes through the
     * devices.c chain. NUL discards + succeeds; CON/PRN/AUX reach their sinks;
     * CLOCK$ accepts a 6-byte record. A device the io bundle cannot serve ->
     * access-denied (Rule 2: never a silent drop). */
    if (e->kind == SFT_KIND_DEVICE && e->device != 0) {
        uint32_t wrote = 0u;
        uint16_t derr = dev_route_rw((device_header_t *)e->device, 1 /* write */,
                                     (uint8_t *)(uintptr_t)buf, count, &wrote);
        if (derr != 0u) {
            set_ax(f, derr);
            cf_set(f);
            return;
        }
        f->eax = wrote;   /* EAX = bytes the device consumed */
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
        uint16_t err = g_file->write_at(e->dir_start, e->root_slot,
                                        e->file_offset,
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

/* Resolve EBX through the current process's JFT into the system SFT exactly the
 * way do_write/do_dup/do_ioctl(AL=00) do. Returns the SFT entry, or NULL for a
 * bad / closed / out-of-range handle. Shared by every handle-based IOCTL minor
 * so the invalid-handle path (Rule 2) is uniform. */
static sft_entry_t *ioctl_sft_from_ebx(int_frame_t *f)
{
    uint32_t handle = f->ebx;
    return (handle <= 0xFFu) ? sft_from_handle(g_cur_psp, (uint8_t)handle) : 0;
}

/* AH=44h IOCTL. AL selects the minor function. This landing implements the
 * handle-based char/file minors that need no FAT/block/ATA access -- purely
 * SFT/device-flag queries:
 *
 *   AL=00h Get Device Information   -> DX = device-information word
 *   AL=01h Set Device Information   -> char-device-only, DH must be 0 (no-op set)
 *   AL=06h Get Input Status         -> AL = 0xFF ready/not-EOF, 0x00 not-ready/EOF
 *   AL=07h Get Output Status        -> AL = 0xFF ready, 0x00 not-ready
 *   AL=08h Is Block Device Changeable -> block-device-only (we have none) -> 0x0001
 *
 * Every minor resolves EBX through ioctl_sft_from_ebx (a bad/closed/out-of-range
 * handle -> CF=1, AX=0x0006 INVALID_HANDLE, registers otherwise untouched, Rule 2).
 * Any other AL -> CF=1, AX=0x0001 INVALID_FUNCTION.
 *
 * Ground truth: DOS 3.3 Programmer's Reference Manual INT 21h Fn 44h; RBIL
 * INT 21/AX=4400h..4408h (HelpPC int-21-44-{0,1,6,7,8}). Bit names in int21.h
 * (INT21_DEVINFO_*); spec/int21h_calling_convention.json AH=44h. */
static void do_ioctl(int_frame_t *f)
{
    uint8_t al = frame_al(f);

    /* --- AL=00h Get Device Information (beads initech-ro6c) ------------------
     * A CHARACTER device (kind==SFT_KIND_DEVICE) -> DX = INT21_DEVINFO_CON
     * (0x80D3, the PRM CON word; our only modeled char device is CON; bit15
     * ISDEV set). A disk FILE (kind==SFT_KIND_FILE) -> DX = INT21_DEVINFO_FILE:
     * bit15 clear, bits 0-5 = drive number (0 == A:), bit6 = 1 (PRM "not
     * written" default; no per-handle dirty bit in sft_entry_t yet). */
    if (al == 0x00u) {
        sft_entry_t *e = ioctl_sft_from_ebx(f);
        if (e == 0) {
#ifdef INT21_MUTATE_IOCTL_BADHANDLE_OK
            /* MUTANT (Rule 6): clear CF on a bad handle instead of failing ->
             * the invalid-handle assertion goes RED. */
            cf_clear(f);
            return;
#else
            set_ax(f, INT21_ERR_INVALID_HANDLE);
            cf_set(f);
            return;
#endif
        }

        /* Character device (our only one is CON) vs disk file: the bit15
         * (ISDEV) fork. DX carries the locked device-info word for the kind. */
#ifdef INT21_MUTATE_IOCTL_CON_WRONG
        /* MUTANT (Rule 6): emit the wrong CON word (drop a bit) -> the CON
         * full-word assertion goes RED. */
        uint16_t con_word = (uint16_t)(INT21_DEVINFO_CON & ~INT21_DEVINFO_STDIN);
#else
        uint16_t con_word = (uint16_t)INT21_DEVINFO_CON;
#endif
#ifdef INT21_MUTATE_IOCTL_ISDEV_INVERT
        /* MUTANT (Rule 6): invert the device-vs-file fork (a device gets the
         * FILE word, a file gets the CON word) -> BOTH word assertions go RED. */
        if (e->kind == SFT_KIND_DEVICE) {
            set_dx(f, (uint16_t)INT21_DEVINFO_FILE);
        } else {
            set_dx(f, con_word);
        }
#else
        if (e->kind == SFT_KIND_DEVICE) {
            set_dx(f, con_word);
        } else {
            set_dx(f, (uint16_t)INT21_DEVINFO_FILE);
        }
#endif
        cf_clear(f);
        return;
    }

    /* --- AL=01h Set Device Information (RBIL/HelpPC INT 21,44,1) -------------
     * Inputs: BX=handle, DH MUST be 0, DL=device-data low byte. CHARACTER-DEVICE
     * ONLY (the device-info word's bit15 ISDEV must be set on the handle being
     * set). On a disk FILE handle, or DH!=0, real DOS rejects with invalid
     * function. Our SFT carries NO writable per-handle device-info word and CON's
     * word is LOCKED (0x80D3), so we cannot honor an arbitrary new word: we model
     * the documented thin answer -- validate (char device + DH==0), accept the
     * write as a NO-OP (the only settable bits real DOS exposes here are the
     * cooked/raw + EOF-on-input flags, which our CON does not vary), and report
     * success with DX = the (unchanged) device-info word so a caller that read
     * back sees a consistent value. A FILE handle or DH!=0 -> AX=0x0001, CF=1. */
    if (al == 0x01u) {
        sft_entry_t *e = ioctl_sft_from_ebx(f);
        if (e == 0) {
            set_ax(f, INT21_ERR_INVALID_HANDLE);
            cf_set(f);
            return;
        }
        /* DH must be zero (RBIL: "DH = must be zero"); a non-char handle has no
         * device-info word to set -> invalid function (Rule 2: reject loudly). */
#ifdef INT21_MUTATE_IOCTL_SETINFO_NOGUARD
        /* MUTANT (Rule 6): drop the (char-device && DH==0) guard so a FILE
         * handle / nonzero DH wrongly succeeds -> the reject assertions go RED. */
        if (0) {
#else
        if (e->kind != SFT_KIND_DEVICE || frame_dh(f) != 0x00u) {
#endif
            set_ax(f, INT21_ERR_INVALID_FUNCTION);
            cf_set(f);
            return;
        }
        /* No-op set on CON: nothing in our model is mutable. Echo the locked CON
         * device-info word back in DX and succeed (CF clear). */
        set_dx(f, (uint16_t)INT21_DEVINFO_CON);
        cf_clear(f);
        return;
    }

    /* --- AL=06h Get Input Status (RBIL/HelpPC INT 21,44,6) ------------------
     * Inputs: BX=handle. Output AL: char device -> 0xFF ready / 0x00 not-ready;
     * FILE -> 0xFF if NOT at EOF / 0x00 if at EOF (read position >= size).
     * Our CON is the live keyboard -> ALWAYS ready (0xFF). For a FILE we have the
     * exact per-handle position (file_offset) and size (dir_entry.file_size), so
     * we report the faithful EOF answer. */
    if (al == 0x06u) {
        sft_entry_t *e = ioctl_sft_from_ebx(f);
        if (e == 0) {
            set_ax(f, INT21_ERR_INVALID_HANDLE);
            cf_set(f);
            return;
        }
        uint8_t status;
        if (e->kind == SFT_KIND_DEVICE) {
            status = 0xFFu;                 /* CON keyboard: always ready */
        } else {
#ifdef INT21_MUTATE_IOCTL_INSTATUS_EOF_FLIP
            /* MUTANT (Rule 6): invert the file EOF test -> the at-EOF and
             * not-at-EOF input-status assertions go RED. */
            status = (e->file_offset >= e->dir_entry.file_size) ? 0xFFu : 0x00u;
#else
            /* RBIL: AL=0x00 if EOF (files); 0xFF if not at EOF. */
            status = (e->file_offset >= e->dir_entry.file_size) ? 0x00u : 0xFFu;
#endif
        }
        set_al(f, status);
        cf_clear(f);
        return;
    }

    /* --- AL=07h Get Output Status (RBIL/HelpPC INT 21,44,7) -----------------
     * Inputs: BX=handle. Output AL: char device -> 0xFF ready / 0x00 not-ready;
     * FILE -> ALWAYS 0xFF (a disk file is always ready for output, RBIL). Our
     * CON screen is always ready -> 0xFF for both kinds. */
    if (al == 0x07u) {
        sft_entry_t *e = ioctl_sft_from_ebx(f);
        if (e == 0) {
            set_ax(f, INT21_ERR_INVALID_HANDLE);
            cf_set(f);
            return;
        }
#ifdef INT21_MUTATE_IOCTL_OUTSTATUS_NOTREADY
        /* MUTANT (Rule 6): report not-ready (0x00) -> the output-ready
         * assertions go RED. */
        set_al(f, 0x00u);
#else
        set_al(f, 0xFFu);               /* CON screen / disk file: always ready */
#endif
        cf_clear(f);
        return;
    }

    /* --- AL=08h Is Block Device Changeable (RBIL/HelpPC INT 21,44,8) --------
     * Real DOS: BL=drive number, AX=0 removable / 1 fixed; it is BLOCK-DEVICE
     * ONLY and returns invalid function for a non-block (character) handle. Our
     * SFT models exactly two handle kinds -- CHARACTER devices (CON) and open
     * FILES on the mounted volume -- and exposes NO block-device handle. So AL=08
     * is never applicable here: every handle kind -> AX=0x0001 (invalid
     * function), CF=1. (When a block-device handle model lands, this minor gains
     * a real removable/fixed answer; tracked separately.) */
    if (al == 0x08u) {
        sft_entry_t *e = ioctl_sft_from_ebx(f);
        if (e == 0) {
            set_ax(f, INT21_ERR_INVALID_HANDLE);
            cf_set(f);
            return;
        }
#ifdef INT21_MUTATE_IOCTL_CHANGEABLE_OK
        /* MUTANT (Rule 6): wrongly answer "removable" (AX=0, CF clear) for our
         * non-block handles -> the invalid-function assertion goes RED. */
        set_ax(f, 0x0000u);
        cf_clear(f);
        return;
#else
        set_ax(f, INT21_ERR_INVALID_FUNCTION);
        cf_set(f);
        return;
#endif
    }

    /* Any other AL minor (02/03/04/05/09/0A/...) is not modeled -> invalid
     * function, CF=1 (Rule 2: recognized AH, unimplemented minor fails loud). */
#ifdef INT21_MUTATE_IOCTL_MINOR_OK
    /* MUTANT (Rule 6): treat an unmodeled minor as success (CF=0) instead of
     * 0x0001 -> the unmodeled-minor assertion goes RED. */
    cf_clear(f);
#else
    set_ax(f, INT21_ERR_INVALID_FUNCTION);
    cf_set(f);
#endif
}

/* ---- file-handle functions (beads initech-509.5 read-side) ---------------- *
 * All resolve a process handle through g_cur_psp->jft into g_sft (the SFT), and
 * reach the mounted volume through g_file (the backend vtable). DEC-04a ABI:
 * AH=func, EBX=handle, ECX=count, EDX=flat ptr, EAX=return, CF=error.
 * Ref: docs/research/fs-mount-sft-ground-truth.md Sec 4; DOS 3.3 PRM AH=3Dh/3Eh/
 * 3Fh/42h/4Eh/4Fh/1Ah/2Fh. */

/* Reject a path a ROOT-ONLY fallback cannot support: a subdirectory separator
 * '\' or a drive-letter ':'. Since beads initech-zs24 EVERY path-taking dispatch
 * site (OPEN/CREAT/UNLINK/FINDFIRST + EXEC) resolves subdir paths through
 * resolve_dir_path() below; this helper survives ONLY as resolve_dir_path's
 * root-only fallback (no backend resolve bound -- the host oracle / a read-only
 * mount) and as the shared overlength-path runaway guard.
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

/* ---- internal devices_io_t adapters for CON + CLOCK$ (beads initech-6zd9) --- *
 * The device chain's CON and CLOCK$ legs are wired to int21.c's OWN existing
 * seams -- NOT to a separately-bound kernel bundle -- for two reasons:
 *   1. CON OUTPUT IDENTITY (the load-bearing constraint): a device-chain CON
 *      write goes through the SAME con_putc() the legacy do_write CON fast path
 *      uses, so OPEN-by-name "CON" output is provably the same byte stream as
 *      handle-1 output. There is exactly one CON output path.
 *   2. CLOCK$ reuses the existing clock seam (g_clock_get/set), already bound to
 *      the RTC by the kernel and to a fixed mock by the host oracle.
 * The kernel-bound g_devio bundle is consulted ONLY for PRN + AUX (the two
 * sinks int21.c has no other path to). So a device READ/WRITE merges: CON/CLOCK$
 * from the internal adapters, PRN/AUX from g_devio. */

/* CON write adapter: route device-chain CON output through con_putc (== the
 * legacy CON path). Always "succeeds" for the count requested (con_putc is the
 * synchronous sink; a NULL sink silently discards, exactly as AH=02h/40h do). */
static int dev_con_write_adapter(const uint8_t *b, int n, void *ctx)
{
    (void)ctx;
    for (int i = 0; i < n; i++) {
        con_putc((char)b[i]);
    }
    return n;
}

/* CON read adapter: deliver raw keyboard bytes through the existing CON input
 * source (conin_get_pb honors the 0Bh pushback). A device-chain CON read is RAW
 * (no cooked editing -- the cooked line editor is the legacy predefined-CON-slot
 * behavior; an OPEN-by-name CON read is the raw character stream). Fills up to
 * `n` bytes, blocking for the first (conin_get never hangs the host: an unbound
 * source returns 0). */
static int conin_get_pb(void);   /* fwd: defined with the CON-input subset below */
static int dev_con_read_adapter(uint8_t *b, int n, void *ctx)
{
    (void)ctx;
    if (n <= 0) {
        return 0;
    }
    b[0] = (uint8_t)conin_get_pb();
    return 1;                    /* one char per call (DOS char-device semantics) */
}

/* CON peek adapter: non-destructive one-byte poll through the existing CON poll
 * (conin_poll_pb). -1 == no data yet. */
static int conin_poll_pb(void);  /* fwd: defined with the CON-input subset below */
static int dev_con_peek_adapter(void *ctx)
{
    (void)ctx;
    return conin_poll_pb();
}

/* Days from 1 Jan 1980 to y/mo/d (the CLOCK$ record's offset-0 WORD). Pure,
 * deterministic Gregorian day count (Rule 11). Ref: MS-DOS 3.3 Tech Ref Ch. 4
 * (CLOCK$ record: WORD days-since-1980). Valid for 1980-01-01 onward; a date at
 * or before the epoch yields 0 (the epoch sentinel). */
static uint16_t dev_days_since_1980(uint16_t y, uint8_t mo, uint8_t d)
{
    static const uint16_t cum[12] = {   /* days before month m (non-leap) */
        0u, 31u, 59u, 90u, 120u, 151u, 181u, 212u, 243u, 273u, 304u, 334u
    };
    if (y < 1980u || mo < 1u || mo > 12u || d < 1u) {
        return 0u;
    }
    uint32_t days = 0u;
    for (uint16_t yy = 1980u; yy < y; yy++) {
        int leap = ((yy % 4u) == 0u && ((yy % 100u) != 0u || (yy % 400u) == 0u));
        days += leap ? 366u : 365u;
    }
    days += cum[mo - 1u];
    if (mo > 2u && ((y % 4u) == 0u && ((y % 100u) != 0u || (y % 400u) == 0u))) {
        days += 1u;              /* this year's leap day, already past in m > Feb */
    }
    days += (uint32_t)(d - 1u);
    return (days > 0xFFFFu) ? (uint16_t)0xFFFFu : (uint16_t)days;
}

/* CLOCK$ read adapter: build the 6-byte date/time record from the existing clock
 * seam (g_clock_get). With no clock bound, g_clock_get is NULL -> fail (the
 * devices.c handler then sets the error bit). Ref: MS-DOS 3.3 Tech Ref Ch. 4. */
static int dev_clk_read_adapter(dev_clock_rec_t *rec, void *ctx)
{
    (void)ctx;
    if (g_clock_get == 0) {
        return 0;
    }
    uint16_t y; uint8_t mo, d, h, mi, s, dow;
    if (!g_clock_get(&y, &mo, &d, &h, &mi, &s, &dow)) {
        return 0;
    }
    rec->days_since_1980 = dev_days_since_1980(y, mo, d);
    rec->minutes    = mi;
    rec->hours      = h;
    rec->hundredths = 0u;        /* the clock seam has no sub-second field */
    rec->seconds    = s;
    return 1;
}

/* CLOCK$ write adapter: program the clock seam (g_clock_set) from a 6-byte
 * record. The record carries days-since-1980 (which the clock seam, taking
 * y/mo/d, cannot consume without a reverse day->date walk) -- the kernel clock
 * seam SET takes a full date, so a CLOCK$ WRITE setting only the time fields is
 * the practical case. We set the TIME (hours/minutes/seconds) via the seam's
 * which-mask; the date half is accepted but a days->y/m/d reverse walk is a
 * flagged follow-up (bead to file). With no clock bound, g_clock_set is NULL ->
 * fail (the handler sets the error bit). */
static int dev_clk_write_adapter(const dev_clock_rec_t *rec, void *ctx)
{
    (void)ctx;
    if (g_clock_set == 0) {
        return 0;
    }
    /* Set only the TIME fields (the date reverse-walk is deferred); the seam's
     * which-mask leaves the date untouched. year/month/day are passed as the
     * current-epoch placeholders the seam ignores for a TIME-only SET. */
    return g_clock_set(1980u, 1u, 1u,
                       rec->hours, rec->minutes, rec->seconds,
                       INT21_CLOCK_SET_TIME);
}

/* Build the merged device io bundle for one request: CON + CLOCK$ from the
 * internal adapters (CON output identity; clock-seam reuse), PRN + AUX from the
 * kernel-bound g_devio (the two sinks int21.c has no other path to). */
static devices_io_t dev_build_io(void)
{
    devices_io_t io;
    for (uint32_t i = 0u; i < (uint32_t)sizeof(io); i++) {
        ((uint8_t *)&io)[i] = 0u;
    }
    /* CON: always int21.c's own seams (the load-bearing identity). */
    io.con_write = dev_con_write_adapter;
    io.con_read  = dev_con_read_adapter;
    io.con_peek  = dev_con_peek_adapter;
    io.con_ctx   = 0;
    /* CLOCK$: the existing clock seam. */
    io.clk_read  = dev_clk_read_adapter;
    io.clk_write = dev_clk_write_adapter;
    io.clk_ctx   = 0;
    /* PRN + AUX: the kernel-bound bundle (NULL when unbound -> handler errors). */
    if (g_devio_bound) {
        io.prn_write = g_devio.prn_write;
        io.prn_ctx   = g_devio.prn_ctx;
        io.aux_write = g_devio.aux_write;
        io.aux_read  = g_devio.aux_read;
        io.aux_peek  = g_devio.aux_peek;
        io.aux_ctx   = g_devio.aux_ctx;
    }
    return io;
}

/* ---- AH=3Dh OPEN-by-name device routing (beads initech-6zd9) --------------- *
 * DOS resolves an OPEN (AH=3Dh) of a DEVICE NAME -- "CON", "NUL", "PRN", "AUX",
 * "CLOCK$" -- by walking the resident device chain BY NAME *before* it ever
 * touches the directory; a match yields a character-device handle, never a file.
 * The name match ignores a leading path/drive and any extension (so "A:\NUL",
 * "NUL", and "NUL.TXT" all open the NUL device). Ref (Law 1): Ralf Brown's
 * Interrupt List INT 21/AH=3Dh ("DOS first checks for a character device of the
 * specified name"); MS-DOS 3.3 Technical Reference Ch. 4 (the 8-byte, space-
 * padded device names; the base name is matched, the extension is ignored).
 *
 * dev_name_from_path: extract the bare base name from an ASCIIZ path and write it
 * UPPER-CASED + space-padded into out8[DEV_NAME_LEN] (exactly the 8-byte form
 * devices_find() compares). Skips a leading "X:" drive and any "...\\" / "/" path
 * components (so the LAST component is the candidate name), then copies up to the
 * first '.' (an extension is not part of the device name). Returns 1 if a non-
 * empty base name was produced, 0 otherwise (caller then is not a device open). */
static int dev_name_from_path(const char *path, char out8[DEV_NAME_LEN])
{
    uint32_t i;
    for (i = 0u; i < (uint32_t)DEV_NAME_LEN; i++) {
        out8[i] = ' ';                    /* space-pad to 8 (DOS device-name form) */
    }
    if (path == 0 || *path == '\0') {
        return 0;
    }

    /* Skip a leading "X:" drive prefix (one volume this milestone; the letter is
     * irrelevant to a device-name match). */
    const char *p = path;
    if (p[0] != '\0' && p[1] == ':') {
        p += 2;
    }

    /* Advance past the LAST path separator so only the final component remains
     * (DOS device names live in every directory; "A:\\SUB\\NUL" still opens NUL).
     * Bounded by INT21_PATH_SCAN_MAX so a malformed/unterminated pointer can never
     * run away (Rule 2; the same runaway bound the file path uses). */
    const char *base = p;
    uint32_t scanned = 0u;
    for (const char *q = p; *q; q++) {
        if (*q == '\\' || *q == '/') {
            base = q + 1;
        }
        if (++scanned >= INT21_PATH_SCAN_MAX) {
            return 0;                     /* runaway -> not a device name */
        }
    }

    /* Copy the base name up to the first '.' (extension excluded) or 8 chars,
     * upper-casing as we go. An empty base ("A:\\" or a trailing separator) is
     * not a device name. */
    uint32_t n = 0u;
    for (const char *q = base; *q && *q != '.' && n < (uint32_t)DEV_NAME_LEN; q++) {
        char c = *q;
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 32);           /* upper-case (devices_find is exact) */
        }
        out8[n++] = c;
    }
    return (n > 0u) ? 1 : 0;
}

/* dev_open_lookup: if OPEN-by-name is enabled (a device chain is bound) AND
 * `path` names a resident device, return its header; else NULL (the caller then
 * takes the FAT file path). NULL chain -> OPEN-by-name disabled (pre-6zd9). */
static device_header_t *dev_open_lookup(const char *path)
{
#ifdef INT21_MUTATE_OPEN_NO_DEVICE
    /* MUTANT (Rule 6; make test-devwire-mutant only): pretend NO path is ever a
     * device, so an OPEN of "NUL"/"CON"/"PRN"/"CLOCK$" falls through to the FAT
     * path -> with no such FILE it returns file-not-found and the device-OPEN
     * oracle goes RED. A one-branch RUNTIME perturbation that compiles under
     * -Werror. NEVER define in a real build. The (void) refs keep
     * -Werror=unused-function quiet (dev_name_from_path is otherwise unreached). */
    (void)path;
    (void)dev_name_from_path;
    return 0;
#else
    if (g_devchain == 0) {
        return 0;                         /* chain unbound -> OPEN-by-name disabled */
    }
    char name8[DEV_NAME_LEN];
    if (!dev_name_from_path(path, name8)) {
        return 0;
    }
    return devices_find(name8);           /* NULL if the name is not a device */
#endif
}

/* dev_route_rw: drive one READ (DEVCMD_READ) or WRITE (DEVCMD_WRITE) request
 * through devices_request() for an SFT slot bound to a resident device (beads
 * initech-6zd9). Builds a device_request_t over the caller's [buf,len) and the
 * bound io bundle, then maps the result back to the syscall contract:
 *   - success: *out_count = the bytes the device transferred, returns 0;
 *   - device error (DEVST_ERROR set): returns a DOS error code (CF=1 at the
 *     call site). A WRITE that the device could not perform (no sink bound, or a
 *     write-only device read) maps to ACCESS_DENIED; a READ fault maps the same
 *     so a program sees a clean DOS error rather than a fault (Rule 2).
 * is_write selects the command. */
static uint16_t dev_route_rw(device_header_t *dev, int is_write,
                             uint8_t *buf, uint32_t len, uint32_t *out_count)
{
    device_request_t pkt;
    for (uint32_t i = 0u; i < (uint32_t)sizeof(pkt); i++) {
        ((uint8_t *)&pkt)[i] = 0u;        /* zero the packet (reserved + status) */
    }
    pkt.length        = (uint8_t)DEV_REQ_HDR_LEN;
    pkt.command       = is_write ? (uint8_t)DEVCMD_WRITE : (uint8_t)DEVCMD_READ;
    pkt.data.rw.buffer = buf;
    /* The DOS device count field is 16-bit; a single INT 21h device READ/WRITE in
     * this model transfers at most 0xFFFF bytes (a larger request is clamped --
     * the device serves what it can, the syscall reports the actual count). */
    pkt.data.rw.count = (len > 0xFFFFu) ? (uint16_t)0xFFFFu : (uint16_t)len;

    devices_io_t io = dev_build_io();     /* CON/CLOCK$ internal, PRN/AUX bound */
    devices_request(dev, &pkt, &io);

    if (pkt.status & DEVST_ERROR) {
        /* A device that cannot satisfy the request (no sink bound, write-only
         * device read, etc.) -> a clean DOS error, never a fault. */
        *out_count = 0u;
        return INT21_ERR_ACCESS_DENIED;   /* 0x0005 */
    }
    *out_count = (uint32_t)pkt.data.rw.count;
    return 0u;
}

/* AH=3Dh OPEN: EDX = flat ptr to ASCIIZ path, AL = mode (0=r,1=w,2=rdwr).
 * DEVICE-by-name (beads initech-6zd9): if `path` names a resident character
 * device (CON/NUL/PRN/AUX/CLOCK$), bind a SFT_KIND_DEVICE slot to that device
 * header and return the handle -- the device open NEVER touches the FAT (DOS
 * checks the device chain before the directory; Ralf Brown INT 21/AH=3Dh).
 * Otherwise: a '\SUB\FILE'-qualified path is resolved to its containing
 * directory's start cluster via the backend resolve seam (beads initech-mzxa);
 * a missing/non-dir component or an overlength path -> CF=1, AX=0x0003 (path not
 * found). On success: allocate an SFT FILE slot + a JFT slot, LOCATE the file in
 * the resolved directory (no whole-file read -- positioned per-handle I/O, beads
 * initech-0qh), store its dir_entry + root_slot + mode in the SFT with
 * file_offset=0, return EAX = handle (JFT index), CF clear. Any number of files
 * may be open concurrently (each its own SFT slot). Errors: AX=0x0002 (not
 * found), 0x0003 (path), 0x0004 (no free SFT/JFT slot). Ref: brief Sec 4.1;
 * DOS 3.3 PRM AH=3Dh; RBIL INT 21/AH=3Dh (device-name precedence). */
static void do_open(int_frame_t *f)
{
    const char *path = (const char *)(uintptr_t)f->edx;
    uint8_t mode = frame_al(f);

    if (path == 0 || *path == '\0') {
        set_ax(f, INT21_ERR_FILE_NOT_FOUND);
        cf_set(f);
        return;
    }

    /* DEVICE-by-name FIRST (DOS precedence): a device open binds a device-kind
     * SFT slot and returns immediately, bypassing the FAT path entirely. Only the
     * JFT slot is needed (no FAT, no per-handle file state). */
    device_header_t *dev = dev_open_lookup(path);
    if (dev != 0) {
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
        sft_entry_t *e = &g_sft[sft_idx];
        e->kind        = SFT_KIND_DEVICE;
        /* dev_id is the legacy predefined-device tag (CON/AUX/PRN); an OPEN-by-name
         * slot is identified by its `device` chain pointer, not dev_id. Leave dev_id
         * at a non-CON value so this slot never matches the legacy CON fast path in
         * do_write/do_read -- the (device != NULL) chain route owns it. */
        e->open_mode   = mode;
        e->dev_id      = (uint8_t)SFT_DEV_AUX;  /* != SFT_DEV_CON: not the CON fast path */
        e->ref_count   = 1u;
        e->device      = dev;                   /* the chain route selector */
        e->file_offset = 0u;
        e->root_slot   = 0u;
        e->dir_start   = 0u;
        g_cur_psp->jft[handle] = sft_idx;

        f->eax = (f->eax & 0xFFFF0000u) | (uint32_t)handle;  /* EAX (AX) = handle */
        cf_clear(f);
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
    e->dir_start   = dir_start;   /* containing dir for subdir write-back (zs24) */
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
    e->root_slot   = root_slot;        /* dir-entry slot for positioned write-back */
    e->dir_start   = dir_start;        /* containing dir (0==root; subdir zs24) */
    g_cur_psp->jft[handle] = sft_idx;

    f->eax = (f->eax & 0xFFFF0000u) | (uint32_t)handle;
    cf_clear(f);
}

/* AH=5Bh CREATNEW: identical to AH=3Ch CREAT (create a zero-length file, claim
 * an SFT FILE slot + JFT slot in WRITE mode, return EAX = handle, CF clear) WITH
 * ONE added precondition -- the target must NOT already exist. If it does, FAIL
 * with CF=1, AX=0x0050 (ERROR_FILE_EXISTS) and commit nothing (no truncate, no
 * slot churn). CX (attribute) is UNUSED here, exactly as in do_creat -- attribute
 * semantics are a separate, larger bead, NOT this one. The existence check reuses
 * the backend's pure-locate open() member (NO SFT mutation, NO whole-file read --
 * int21.h backend contract: out_entry/out_slot written only on success, returns
 * 0 found / 0x0002 not-found) and runs AFTER resolve_dir_path succeeds and BEFORE
 * sft_alloc/jft_alloc so a rejection touches nothing. Shares the resolve_dir_path
 * seam so '\SUB\FILE' subdir targets work for free (beads initech-mzxa/zs24). The
 * AH=59h dispatch wrapper auto-notes CF+AX, so no explicit int21_note_error here.
 * A NULL/empty path -> 0x0002 (file-not-found), the SAME code do_creat/do_open
 * return for an empty EDX (spec/int21h_calling_convention.json AH=5Bh: "0x0002
 * NULL/empty path"; RBIL INT 21h/AH=5Bh -- an empty ASCIIZ name has no entry to
 * create, so it is a name lookup that finds nothing). A backend binding create()
 * but NOT open() is rejected 0x0005 (no existence-probe -> cannot honor the
 * fail-if-exists contract; beads initech-glsw, Rule 2 fail-loud).
 * Ref: DOS 3.3 PRM AH=5Bh (file-exists = error 50h); ADR-0003 Appendix A line
 * "5Bh CREATNEW -- Create file, fail if existing"; beads initech-kji0/glsw. */
static void do_creatnew(int_frame_t *f)
{
    const char *path = (const char *)(uintptr_t)f->edx;

    if (path == 0 || *path == '\0') {
        set_ax(f, INT21_ERR_FILE_NOT_FOUND);
        cf_set(f);
        return;
    }
    /* Resolve the CONTAINING directory (the file may not exist yet, so the
     * resolve targets the parent chain, not the leaf). A missing/non-dir parent
     * component or an overlength path -> 0x0003. */
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

#ifndef INT21_MUTATE_CREATNEW_NO_GUARD
    /* CREATNEW precondition: the target must not already exist. Probe via the
     * pure-locate open() member. open() returns 0 when the leaf is found in
     * `dir_start`; that means EXISTS -> fail with 0x0050, commit nothing. A
     * 0x0002 (not found) or any other non-zero locate result means "does not
     * exist" -> fall through to the CREAT path. (Rule 6 mutants: NO_GUARD drops
     * this block so CREATNEW==CREAT; GUARD_INVERT flips the sense; WRONG_CONST
     * returns the wrong AX.)
     *
     * RULE 2 / beads initech-glsw: a backend that binds create() but NOT open()
     * has NO existence-probe, so the precondition CANNOT be honored -- proceeding
     * to create() would TRUNCATE an existing file with CF clear, the EXACT
     * INVERSE of the CREATNEW contract. Refuse loud rather than silently skip the
     * guard. This is SYMMETRIC with do_open, which treats a NULL open() member as
     * a hard error (file-not-found there) instead of a silent success. Unreachable
     * with the shipped fat12 backend (fileio_fat binds both fat_open + fat_create),
     * so it is a defensive invariant guard: hard-error 0x0005 (access denied --
     * the same "the volume cannot serve this write safely" code do_creat returns
     * when create() itself is NULL), commit nothing. NO_GUARD removes BOTH the
     * existence probe and this guard so CREATNEW degenerates to CREAT (the mutant
     * the NO_GUARD oracle bites). */
    if (g_file->open == 0) {
        set_ax(f, INT21_ERR_ACCESS_DENIED);   /* 0x0005: cannot verify non-existence */
        cf_set(f);
        return;
    }
    {
        dir_entry_t probe_de;
        uint32_t    probe_slot = 0u;
        uint16_t    found = g_file->open(leaf, dir_start, &probe_de, &probe_slot);
#ifdef INT21_MUTATE_CREATNEW_GUARD_INVERT
        if (found != 0u) {               /* MUTANT: treat NOT-found as exists */
#else
        if (found == 0u) {               /* file located -> already exists */
#endif
#ifdef INT21_MUTATE_CREATNEW_WRONG_CONST
            set_ax(f, INT21_ERR_ACCESS_DENIED);   /* MUTANT: wrong 0x0005 */
#else
            set_ax(f, INT21_ERR_FILE_EXISTS);     /* 0x0050 ERROR_FILE_EXISTS */
#endif
            cf_set(f);
            return;
        }
    }
#endif /* INT21_MUTATE_CREATNEW_NO_GUARD */

    /* From here on: identical to do_creat. Secure the table slots BEFORE creating
     * so a slot-exhaustion failure does not commit anything (mirrors do_open). */
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
    e->root_slot   = root_slot;        /* dir-entry slot for positioned write-back */
    e->dir_start   = dir_start;        /* containing dir (0==root; subdir zs24) */
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

/* AH=56h RENAME (SAME-directory dir-entry rename; beads initech-gnrc): the FIRST
 * INT 21h handler that reads a SECOND flat pointer.
 *   IN : AH=0x56; EDX = flat ptr to the OLD ASCIIZ 8.3 path; EDI = flat ptr to
 *        the NEW ASCIIZ 8.3 path. In the locked flat 32-bit ABI EVERY pointer is
 *        a flat 32-bit LINEAR address (spec/int21h_calling_convention.json: abi.
 *        primary_pointer = EDX; pointer args are flat linear addresses). Real DOS
 *        passes the new name in ES:DI; on this 32-bit protected-flat kernel the
 *        segment is collapsed and EDI carries the flat linear address directly --
 *        the SAME convention EDX already uses for the primary pointer. int_frame_t
 *        carries both edx and edi (os/milton/idt.h).
 *   OUT: success -> CF=0 (AX left untouched, the do_unlink/do_mkdir pattern).
 * ERRORS (CF=1):
 *   - NULL/empty EDX or EDI                     -> AX=0x0002 (file not found)
 *   - bad/overlong path component (either side) -> AX=0x0003 (from resolve_dir_path)
 *   - source not found                          -> AX=0x0002
 *   - dest name already exists in the target dir, OR the source is a directory/
 *     volume-label, OR no write backend         -> AX=0x0005 (access denied)
 *   - old_dir != new_dir (cross-directory pair) -> AX=0x0011 (NOT_SAME_DEVICE).
 *     SAME-directory rename ONLY this milestone (operator decision 2026-06-15,
 *     DOS-faithful); cross-dir MOVE is deferred to beads initech-ycb3.
 * No int21_note_error() here: the AH=59h auto-note at the dispatch choke point
 * captures CF + AX. Ref (Law 1): DOS 3.3 PRM AH=56h; spec/int21h_calling_
 * convention.json (AH=56h: in EDX old / EDI new); ADR-0003 Appendix A. */
static void do_rename(int_frame_t *f)
{
    const char *old_path = (const char *)(uintptr_t)f->edx;
    const char *new_path = (const char *)(uintptr_t)f->edi;  /* the SECOND flat ptr */

    if (old_path == 0 || *old_path == '\0' ||
        new_path == 0 || *new_path == '\0') {
        set_ax(f, INT21_ERR_FILE_NOT_FOUND);
        cf_set(f);
        return;
    }

    /* Resolve BOTH paths to their containing directory + bare leaf. A bad/overlong
     * component on either side -> 0x0003 (path not found, via resolve_dir_path). */
    const char *old_leaf      = old_path;
    uint16_t    old_dir_start = 0u;
    uint16_t    operr = resolve_dir_path(old_path, &old_leaf, &old_dir_start);
    if (operr != 0u) {
        set_ax(f, operr);                /* 0x0003 PATH_NOT_FOUND */
        cf_set(f);
        return;
    }
    const char *new_leaf      = new_path;
    uint16_t    new_dir_start = 0u;
    uint16_t    nperr = resolve_dir_path(new_path, &new_leaf, &new_dir_start);
    if (nperr != 0u) {
        set_ax(f, nperr);                /* 0x0003 PATH_NOT_FOUND */
        cf_set(f);
        return;
    }

    /* A cross-DIRECTORY pair is a MOVE, not a rename -> NOT_SAME_DEVICE. Same-dir
     * rename ONLY this milestone (cross-dir MOVE deferred to beads initech-ycb3). */
#ifndef INT21_MUTATE_RENAME_NO_SAMEDIR_GUARD
    if (old_dir_start != new_dir_start) {
        set_ax(f, INT21_ERR_NOT_SAME_DEVICE);
        cf_set(f);
        return;
    }
#else
    /* MUTANT 4 (Rule 6; make test-gnrc-mutant only): DROP the old_dir==new_dir
     * guard, so a cross-directory EDX/EDI pair wrongly proceeds to the backend
     * rename (acting as if same-dir) instead of returning 0x0011. The
     * not-same-device leg goes RED. NEVER in a real build. */
#endif

    if (g_file == 0 || g_file->rename == 0) {
        /* No write backing (read-only volume / no volume) -> access denied
         * (Rule 2: never a silent success). */
        set_ax(f, INT21_ERR_ACCESS_DENIED);
        cf_set(f);
        return;
    }

    uint16_t err = g_file->rename(old_leaf, old_dir_start, new_leaf, new_dir_start);
    if (err != 0) {
#ifndef INT21_MUTATE_RENAME_NOTFOUND_PATH
        set_ax(f, err);                  /* 0x0002 not found / 0x0005 dest/dir/write */
#else
        /* MUTANT 5 (Rule 6; make test-gnrc-mutant only): map a backend source-
         * not-found (0x0002) to 0x0003 (path not found), corrupting the DOS
         * register contract. The register-contract AX assertion goes RED. NEVER in
         * a real build. */
        set_ax(f, (err == INT21_ERR_FILE_NOT_FOUND) ? INT21_ERR_PATH_NOT_FOUND : err);
#endif
        cf_set(f);
        return;
    }
    cf_clear(f);
}

/* AH=43h CHMOD (GET/SET FILE ATTRIBUTES): a PATH-BASED call (not a handle call).
 *   IN : AL=00 GET / AL=01 SET; EDX = flat ptr to an ASCIIZ 8.3 path
 *        ('\SUB\FILE'-qualified OK via resolve_dir_path). SET: CX = the new
 *        attribute byte (RO 0x01 / Hidden 0x02 / System 0x04 / Archive 0x20).
 *   OUT: GET success -> CF=0, CX = the dir entry's attribute byte (CL=attr,
 *        CH=0). A GET on a DIRECTORY succeeds with CX=0x10 (and a volume-label
 *        with CX=0x08): GET is a pure read, and CX=0x10 is the faithful DOS
 *        answer + the canonical "does this directory exist" idiom (RBIL AX=4300h
 *        has NO directory exclusion). SET success -> CF=0.
 * ERRORS (CF=1):
 *   - AL not in {0,1}                          -> AX=0x0001 (invalid function)
 *   - NULL / empty path                        -> AX=0x0002 (file not found)
 *   - bad/overlong path component              -> AX=0x0003 (path not found; from
 *                                                 resolve_dir_path)
 *   - file not found                           -> AX=0x0002
 *   - a SET targeting a DIRECTORY or VOLUME-LABEL entry, OR a SET CX that sets
 *     the Directory(0x10)/VolLabel(0x08) bit, OR no write backend -> AX=0x0005
 *     (access denied -- the DOS-faithful SET reject set; Rule 2 fail loud, never
 *     silently corrupt a dirent's type bits). This reject is SET-ONLY; a GET on
 *     a directory/volume-label SUCCEEDS (operator-sanctioned 2026-06-15;
 *     initech-5o6o records the SET reject as the intentional fail-loud deviation).
 * The SET reject for a re-typing CX is enforced HERE at the dispatch edge (before
 * the backend is even consulted) AND again in the fat12 primitive (defense in
 * depth). No int21_note_error(): the AH=59h auto-note at the dispatch choke point
 * captures CF + AX. Ref: RBIL INT 21h/AX=4300h; DOS 3.3 PRM INT 21h Function 43h
 * (Get/Set File Attributes); spec/dos_structs.h (attribute 0x0B; DIR_ATTR_*);
 * CLAUDE.md Law 1 (cite), Rule 2 (fail loud), Rule 11 (mtime/mdate untouched on
 * SET). */
static void do_chmod(int_frame_t *f)
{
    const char *path = (const char *)(uintptr_t)f->edx;
    uint8_t     al   = frame_al(f);

    /* AL must select GET (0) or SET (1); reject anything else BEFORE the path is
     * touched (DOS validates the subfunction first). */
#ifndef INT21_MUTATE_CHMOD_NO_AL_REJECT
    if (al != 0x00u && al != 0x01u) {
        set_ax(f, INT21_ERR_INVALID_FUNCTION);
        cf_set(f);
        return;
    }
#else
    /* MUTANT 1 (Rule 6; make test-b53d-mutant only): SKIP the AL validation, so a
     * bad AL (e.g. AL=2) is NO LONGER rejected -- it falls through to the GET path
     * (al==2 != 0x01 takes the GET branch), returning CF=0 + a CX attr instead of
     * the required CF=1/AX=0x0001. The bad-AL host contract goes RED. NEVER in a
     * real build. */
#endif

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
        cf_set(f);
        return;
    }
    if (g_file == 0 || g_file->chmod == 0) {
        /* No attribute backend (read-only volume / no volume) -> access denied
         * (Rule 2: never a silent success that drops the attribute). */
        set_ax(f, INT21_ERR_ACCESS_DENIED);
        cf_set(f);
        return;
    }

    if (al == 0x01u) {
        /* SET: a CX that re-types the entry (Directory/VolLabel) is forbidden ->
         * access denied (Rule 2 fail-loud). NOTE: real DOS 3.3 (RBIL 4301h) MASKS
         * the Directory/VolLabel CX bits and SUCCEEDS a SET on a directory; we
         * REJECT with 0x0005 instead. This is an INTENTIONAL, operator-approved
         * (2026-06-15) Law-4 fidelity deviation, registered in beads initech-5o6o
         * and ADR-0003 Amendment DEC-14 sub-decision DEC-14.2 (sec.2). Revisit
         * only if a real DOS consumer needs DOS's mask-and-succeed. */
        uint16_t cx = frame_cx(f);
#ifndef INT21_MUTATE_CHMOD_NO_CX_REJECT
        if ((cx & (uint16_t)(DIR_ATTR_DIRECTORY | DIR_ATTR_VOLLABEL)) != 0u) {
            set_ax(f, INT21_ERR_ACCESS_DENIED);
            cf_set(f);
            return;
        }
#endif
        uint8_t  attr = (uint8_t)(cx & 0xFFu);
        uint16_t err = g_file->chmod(leaf, dir_start, 1, &attr);
        if (err != 0) {
            set_ax(f, err);              /* 0x0002 not found / 0x0005 access */
            cf_set(f);
            return;
        }
        cf_clear(f);
        return;
    }

    /* GET (AL=00): return the attribute byte in CX (CL=attr, CH=0). */
    uint8_t attr = 0u;
    uint16_t err = g_file->chmod(leaf, dir_start, 0, &attr);
    if (err != 0) {
        set_ax(f, err);                  /* 0x0002 not found / 0x0005 dir/vollabel */
        cf_set(f);
        return;
    }
#ifndef INT21_MUTATE_CHMOD_GET_ZERO
    set_cx(f, (uint16_t)attr);
#else
    /* MUTANT 2 (Rule 6; make test-b53d-mutant only): return a constant 0x00 in CX
     * instead of the real attribute byte. The GET-returns-CX host assertion goes
     * RED. The (void) keeps -Werror=unused quiet. NEVER in a real build. */
    (void)attr;
    set_cx(f, 0x0000u);
#endif
    cf_clear(f);
}

/* AH=39h MKDIR (CREATE DIRECTORY): EDX = flat ptr to an ASCIIZ path. Resolve the
 * path to its CONTAINING directory (the new dir does not exist yet) via the
 * resolve seam and create the bare leaf there through the backend mkdir member.
 * The CONTAINING directory may be the root OR a subdirectory (nested MD
 * '\SUB\NEWDIR'): resolve_dir_path returns the parent's dir_start and the
 * backend forwards it to the now parent-aware fat12_mkdir (beads initech-m0bp).
 * CF clear on success; CF=1, AX=0x0003 (bad path component / no backend) /
 * 0x0005 (name already exists, dir full, full volume, read-only). The AH=59h
 * dispatch wrapper auto-notes CF+AX so no int21_note_error here. Ref: DOS 3.3
 * PRM AH=39h; PRD Sec 6.5 (DOS path model); beads initech-u6wa / initech-m0bp. */
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
        set_ax(f, err);                  /* 0x0005 exists/dir-full / 0x0003 bad path */
        cf_set(f);
        return;
    }
    cf_clear(f);
}

/* AH=3Ah RMDIR (REMOVE DIRECTORY): EDX = flat ptr to an ASCIIZ path. Resolve the
 * FULL path to the TARGET directory (resolve_dir seam) for the two dispatcher-
 * level guards, then resolve its CONTAINING directory + bare leaf (resolve seam)
 * and remove it through the backend rmdir member (which verifies the directory
 * is empty). The CONTAINING directory may be the root OR a subdirectory (nested
 * RD '\SUB\NEWDIR'): resolve_dir_path returns the parent's dir_start and the
 * backend forwards it to the now parent-aware fat12_rmdir (beads initech-m0bp).
 * Dispatcher-level guards (it owns the CWD state):
 *   - RMDIR of the ROOT ('\' / '' / a path resolving to the root) -> reject with
 *     0x0010 (DOS forbids removing the root, like the current dir);
 *   - RMDIR whose TARGET start_cluster == the current g_cwd_start_cluster ->
 *     0x0010 (DOS "attempt to remove the current directory").
 * Backend errors: 0x0003 (missing dir / not a directory), 0x0005 (the directory
 * is not empty). CF clear on success. The AH=59h dispatch wrapper auto-notes
 * CF+AX. Ref: DOS 3.3 PRM AH=3Ah; beads initech-u6wa / initech-m0bp. */
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
 * clear. A read at/after EOF returns 0 bytes cleanly. A CON device read delivers
 * COOKED line input (beads initech-x8fs; see below). AUX/PRN read has no driver
 * -> 0 bytes. Bad handle -> CF=1, AX=0x0006. Ref: brief Sec 4.2; DOS 3.3 PRM
 * AH=3Fh; Microsoft KB Q113058 (AH=3Fh CON read). */

/* Internal CON cooked-line staging size. Real DOS reads a console handle through
 * a 128-byte internal line buffer (the historical command-line template limit);
 * we stage the line there, then copy it (plus CR+LF) into the caller's buffer. */
#define INT21_CON_LINE_MAX 128u

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
        /* OPEN-by-name device read (beads initech-6zd9): a SFT slot bound to a
         * resident device (device != NULL) routes through the devices.c chain --
         * NUL returns EOF (0 bytes), CLOCK$ returns the 6-byte date/time record,
         * an OPEN-by-name CON returns raw keyboard bytes, PRN read is an error.
         * The predefined CON slots 0..3 (device == NULL, dev_id == SFT_DEV_CON)
         * are NOT matched here -- they fall through to the cooked-line editor
         * below, preserving the existing handle-1/etc. read behavior exactly. */
        if (e->device != 0) {
            if (count == 0u) {
                f->eax = 0u;             /* zero-count read: 0 bytes, no device I/O */
                cf_clear(f);
                return;
            }
            /* The device may WRITE into the caller's buffer (CLOCK$/CON read);
             * validate it first (ADR-0003 DEC-14 / initech-tzq). */
            if (!user_buf_ok(f->edx, count)) {
                set_ax(f, INT21_ERR_INVALID_MEMORY);
                cf_set(f);
                return;
            }
            uint32_t got = 0u;
            uint16_t derr = dev_route_rw((device_header_t *)e->device, 0 /* read */,
                                         buf, count, &got);
            if (derr != 0u) {
                set_ax(f, derr);          /* PRN read / unserviceable -> 0x0005 */
                cf_set(f);
                return;
            }
            f->eax = got;                 /* bytes the device delivered (0 = EOF) */
            cf_clear(f);
            return;
        }
#if !defined(INT21_MUTATE_CONHANDLE_NOCOOKED)
        /* CON cooked line-read (beads initech-x8fs). Reading a console handle in
         * the default (cooked/ASCII) mode delivers a full line of edited keyboard
         * input through the SAME cooked editor as AH=0Ah, but the HANDLE-read
         * contract differs: the bytes returned are the line FOLLOWED BY CR (0x0D)
         * AND LF (0x0A), and the returned count (EAX) INCLUDES the CR and LF.
         * (AH=0Ah, by contrast, stores only the CR and excludes it from buf[1].)
         * Ground truth: Microsoft KB Q113058 -- typing "abc"+Enter into a 3Fh read
         * yields buffer "abc\r\n" and AX=5; the line is terminated by CR and the
         * CR/LF pair is returned to the caller as data. AUX/PRN have no driver
         * (handled by the EOF fall-through below). Ctrl-C routes to INT 23h via the
         * shared editor's ^C check-point (beads initech-4tw; DEC-16 Sec 3.3). */
        if (e->dev_id == SFT_DEV_CON) {
            /* A zero-count read returns 0 bytes WITHOUT consuming a line of input
             * (the DOS zero-count contract) -- do not block the keyboard for a
             * read that cannot store anything. */
            if (count == 0u) {
                f->eax = 0u;
                cf_clear(f);
                return;
            }
            /* Validate the destination before the editor writes the copied line
             * into it (ADR-0003 DEC-14 / initech-tzq). */
            if (!user_buf_ok(f->edx, count)) {
                set_ax(f, INT21_ERR_INVALID_MEMORY);
                cf_set(f);
                return;
            }

            uint8_t line[INT21_CON_LINE_MAX];
            int broke = 0;
            uint8_t n = conin_cooked_line(line, (uint8_t)INT21_CON_LINE_MAX, f, &broke);

            /* ^C aborted the line (beads initech-4tw; DEC-16 Sec 3.3): INT 23h has
             * fired. In the kernel int23's default action terminated and never
             * returned here. If a non-terminating break handler returned, the
             * handle read yields EOF (0 bytes), CF clear -- no partial cooked line
             * with a ^C buried in it. */
            if (broke) {
                f->eax = 0u;
                cf_clear(f);
                return;
            }

            /* Emit the cooked line + CR + LF into the caller's buffer, capped at
             * `count` bytes. The returned EAX is the number actually copied
             * (line chars + CR + LF when room; clamped to count otherwise). The
             * remainder-buffering DOS does when count < line length is a separate
             * follow-up -- the common shell/redirect case has count >= line+2. */
            uint32_t took = 0u;
            for (uint8_t i = 0u; i < n && took < count; i++) {
                buf[took++] = line[i];
            }
            if (took < count) { buf[took++] = 0x0Du; }   /* CR */
            if (took < count) { buf[took++] = 0x0Au; }   /* LF */
            f->eax = took;
            cf_clear(f);
            return;
        }
#endif
        /* AUX/PRN have no driver (and, under the no-cooked mutant, CON too):
         * return 0 bytes read (EOF) WITHOUT hanging -- success with EAX=0
         * (brief Sec 4.2). */
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

/* AH=57h GET/SET FILE DATE AND TIME (by handle; beads initech-qekc).
 *   AL=00h GET: return the open handle's packed modification time/date.
 *               CX = dir_entry.mtime, DX = dir_entry.mdate, CF=0.
 *   AL=01h SET: write the caller-supplied CX/DX stamps to the handle's dir entry
 *               and FLUSH IMMEDIATELY (parity with the per-call write-commit
 *               model: each WRITE commits, so AH=57h SET commits here -- NOT
 *               deferred to CLOSE). CF=0 on success.
 *
 * EBX = file handle (a JFT index, resolved through the current process's JFT into
 * the system SFT exactly the way do_write/do_lseek do -- sft_from_handle). The
 * packed CX time (h/m/s2) and DX date (y-1980/m/d) are the SAME packed words
 * dir_entry_t.mtime(0x16)/.mdate(0x18) store on disk, so SET copies CX->mtime,
 * DX->mdate VERBATIM (no encode/decode) and so does GET in reverse.
 *
 * ERRORS (CF=1):
 *   - bad / closed / out-of-range handle           -> AX=0x0006 (INVALID_HANDLE)
 *   - AL not in {0,1}                               -> AX=0x0001 (INVALID_FUNCTION)
 *   - a DEVICE handle (CON/AUX/PRN -- no dir entry) -> AX=0x0001 (a device has no
 *     filesystem date/time; documented as invalid-function, distinct from a bad
 *     handle which IS resolvable just not a file)
 *   - SET with no write backend (set_time == NULL) or a backend write failure
 *                                                   -> AX=0x0005 (ACCESS_DENIED)
 * No int21_note_error() call: the AH=59h auto-note at the dispatch choke point
 * captures CF + AX. Ref: DOS 3.3 PRM AH=57h; spec/dos_structs.h (dir_entry_t
 * mtime 0x16 / mdate 0x18); CLAUDE.md Law 1 (cite), Rule 2 (fail loud). */
static void do_filetime(int_frame_t *f)
{
    uint32_t handle = f->ebx;
    uint8_t  al     = frame_al(f);

    /* AL must select GET (0) or SET (1); anything else is invalid-function and is
     * rejected BEFORE the handle is touched (DOS validates the subfunction). */
#ifndef INT21_MUTATE_FILETIME_NO_AL_REJECT
    if (al != 0x00u && al != 0x01u) {
        set_ax(f, INT21_ERR_INVALID_FUNCTION);
        cf_set(f);
        return;
    }
#else
    /* MUTANT 4 (Rule 6; make test-qekc-mutant only): SKIP the AL validation, so a
     * bad AL (e.g. AL=2) is NO LONGER rejected -- it falls through to the GET/SET
     * body (AL=2 != 0x00 takes the SET path on a write-capable backend, returning
     * CF=0 instead of the required CF=1/AX=0x0001). The bad-AL host contract goes
     * RED (the assertion demands CF=1 + invalid-function). NEVER in a real build. */
#endif

    sft_entry_t *e = (handle <= 0xFFu)
                       ? sft_from_handle(g_cur_psp, (uint8_t)handle)
                       : 0;
    if (e == 0) {
        /* No such open handle (out of range / closed / no process). Rule 2. */
        set_ax(f, INT21_ERR_INVALID_HANDLE);
        cf_set(f);
        return;
    }

    /* Only a disk FILE carries a directory date/time. A character device
     * (CON/AUX/PRN) is resolvable but has no dir entry -> invalid-function
     * (documented; distinct from a bad handle, which is not resolvable). */
    if (e->kind != SFT_KIND_FILE) {
        set_ax(f, INT21_ERR_INVALID_FUNCTION);
        cf_set(f);
        return;
    }

    if (al == 0x00u) {
        /* GET: read the SFT's in-memory dir_entry copy (the kernel keeps it in
         * sync via OPEN/CREAT + the positioned-write refresh). No backend seam.
         * CX = packed mtime, DX = packed mdate (DOS 3.3 PRM AH=57h AL=00). */
#ifndef INT21_MUTATE_FILETIME_GET_SWAP
        set_cx(f, e->dir_entry.mtime);
        set_dx(f, e->dir_entry.mdate);
#else
        /* MUTANT 2 (Rule 6; make test-qekc-mutant only): SWAP the registers --
         * return mdate in CX and mtime in DX. The host CX/DX GET contract goes
         * RED (CX != seeded mtime). NEVER define in a real build. */
        set_cx(f, e->dir_entry.mdate);
        set_dx(f, e->dir_entry.mtime);
#endif
        cf_clear(f);
        return;
    }

    /* SET: copy CX->mtime, DX->mdate VERBATIM into the SFT copy, then FLUSH via
     * the backend (immediate commit -- parity with write_at). A read-only backend
     * (set_time == NULL) or a write failure -> access denied (Rule 2: never a
     * silent no-op). */
    uint16_t new_mtime = frame_cx(f);
    uint16_t new_mdate = frame_dx(f);
    if (g_file == 0 || g_file->set_time == 0) {
#ifndef INT21_MUTATE_FILETIME_RO_OK
        set_ax(f, INT21_ERR_ACCESS_DENIED);
        cf_set(f);
        return;
#else
        /* MUTANT 5 (Rule 6; make test-qekc-mutant only): a SET with NO write
         * backend (set_time == NULL) returns CF=0 (silent success) instead of
         * CF=1/AX=0x0005. The read-only-SET host assertion goes RED. The (void)
         * keeps -Werror quiet for the unused new_* below. NEVER in a real build. */
        (void)new_mtime; (void)new_mdate;
        cf_clear(f);
        return;
#endif
    }
    uint16_t err = g_file->set_time(e->dir_start, e->root_slot,
                                    new_mtime, new_mdate);
    if (err != 0) {
        set_ax(f, err);
        cf_set(f);
        return;
    }
    e->dir_entry.mtime = new_mtime;   /* refresh the in-memory copy (GET parity) */
    e->dir_entry.mdate = new_mdate;
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
#ifdef INT21_MUTATE_WILD_STAR_BLEED
            /* MUTANT (Rule 6; make test-80k-mutant only): a '*' fills from the
             * current cursor to the END OF THE 11-BYTE TEMPLATE, bleeding a
             * name-field '*' into the ext field. "*.TXT" then becomes all '?',
             * so the "ext must stay TXT" rows MUST go RED. NEVER in a real build. */
            for (; base + pos < 11; pos++) {
                out[base + pos] = '?';
            }
#else
            /* Fill the rest of THIS field with '?' (match any). */
            for (; pos < cap; pos++) {
                out[base + pos] = '?';
            }
#endif
            continue;
        }
        if (pos < cap) {
#ifndef INT21_MUTATE_WILD_NO_UPCASE
            /* Upper-case ASCII a-z. */
            if (c >= 'a' && c <= 'z') {
                c = (char)(c - 'a' + 'A');
            }
#else
            /* MUTANT (Rule 6; make test-80k-mutant only): drop the upper-case
             * fold, so a lower-case spec stores lower-case bytes that never
             * match the upper-cased on-disk name. The lower-case-spec row MUST
             * go RED. NEVER define in a real build. */
#endif
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
#if defined(INT21_MUTATE_WILD_MATCH_EXACT) || defined(INT21_MUTATE_WILD_QMARK_LITERAL)
        /* MUTANT (Rule 6; make test-80k-mutant only): both perturbations drop
         * the '?' wildcard arm so the template '?' is compared as a literal
         * byte (exact-only matching). EXACT and QMARK_LITERAL collapse to the
         * same defect at the match site -- every '*'/'?' match row MUST go RED.
         * (QMARK_LITERAL is the build-side framing of "'?' is not special";
         * the observable effect is identical here.) NEVER in a real build. */
#else
        if (pat[i] == '?') {
            continue;
        }
#endif
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

#ifdef INT21_WILDCARD_TESTSEAM
/* TEST-ONLY seam (CLAUDE.md Rule 6 idiom; compiled ONLY into the wildcard host
 * oracle via -DINT21_WILDCARD_TESTSEAM, beads initech-80k). The 8.3 FCB-style
 * matcher (build_pattern + pattern_match) is the load-bearing math behind
 * DIR/FINDFIRST wildcards, so it gets a direct table-driven oracle that drives
 * the SAME two static functions the dispatcher uses -- no second matcher to
 * drift. Pure functions, no global state; never compiled into a real kernel. */
void int21_wildcard_build(const char *spec, uint8_t out[11])
{
    build_pattern(spec, out);
}
int int21_wildcard_match(const uint8_t pat[11], const uint8_t name[11])
{
    return pattern_match(pat, name);
}
#endif /* INT21_WILDCARD_TESTSEAM */

/* AH=4Bh EXEC (LOAD AND EXECUTE): AL = subfunction, EDX = flat ptr to the ASCIIZ
 * program path (root-dir 8.3 only this milestone). AL=00h is load-and-execute (a
 * child program); AL=03h (load overlay) + other subfunctions are out of scope ->
 * invalid function (CF=1, AX=0x0001). The child runs to completion via the saw
 * path (load_program_from_fat); when it terminates (4Ch / INT 20h) the loader
 * regains control and EXEC returns to the caller with CF clear. The child's exit
 * code is stashed for AH=4Dh GET-RETURN-CODE.
 *
 * Path rules: an empty/NULL path -> file-not-found. SUBDIR EXEC (beads
 * initech-zs24, Landing 2): the EDX path is resolved to a (containing-directory
 * `dir_start`, bare leaf) pair through the SAME resolve seam OPEN/CREAT/UNLINK
 * use (resolve_dir_path -> the file backend resolve()), so EXEC honors absolute,
 * CWD-relative, and '\SUB\FILE' paths IDENTICALLY to OPEN -- no second path
 * grammar (drive-letter handling, the runaway guard, and the missing/non-dir
 * component -> 0x0003 contract all come from resolve_dir_path). The loader then
 * locates the leaf in `dir_start` and runs it (the root case, dir_start==0, takes
 * the byte-identical historical find -- loader.c). Earlier milestones (mzxa
 * decision 3) kept EXEC root-only because threading the containing-directory
 * cluster into the loader was deemed riskier than the read-side resolve; this
 * landing does exactly that threading.
 *
 * REENTRANCY (Rule 2 / stop condition): the loader's return-to-loader context
 * (loader.c g_loader_ctx) is single-level -- a nested EXEC (EXEC issued from
 * inside an already-running loaded program) would clobber it. load_program_from_fat
 * guards this and returns INSUFFICIENT_MEM; we surface that as CF=1, AX=0x0008.
 * EXEC from KERNEL/shell context (the common case) is fully supported.
 * Ref: DOS 3.3 PRM AH=4Bh; ADR-0003 DEC-08 (flat .COM); beads initech-saw/zs24. */
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
#ifdef INT21_MUTATE_EXEC_ROOTREJECT
    /* MUTANT (CLAUDE.md Rule 6; make test-zs24-exec-mutant only): restore the
     * pre-zs24 root-only gate -- reject any subdir/drive path BEFORE resolving, so
     * a subdir EXEC returns 0x0003 and the program never runs (its serial marker
     * is ABSENT). The subdir-EXEC oracle MUST go RED. NEVER define in a real build. */
    if (path_has_subdir_or_drive(path)) {
        set_ax(f, INT21_ERR_PATH_NOT_FOUND);
        cf_set(f);
        return;
    }
#endif
    /* Resolve the EDX path to (containing-dir start cluster, bare 8.3 leaf) the
     * SAME way do_open does (beads initech-zs24): absolute / CWD-relative /
     * '\SUB\FILE', a leading 'X:' drive prefix stripped, a missing/non-dir
     * component or an overlength runaway -> 0x0003 PATH_NOT_FOUND. No backend
     * resolve bound (host EXEC oracle) falls back to root-only inside
     * resolve_dir_path, byte-identical to the pre-zs24 path_has_subdir_or_drive
     * gate for a bare name. */
    const char *leaf      = path;
    uint16_t    dir_start = 0u;
    uint16_t    perr = resolve_dir_path(path, &leaf, &dir_start);
    if (perr != 0u) {
        set_ax(f, perr);                 /* 0x0003 PATH_NOT_FOUND */
        int21_note_error(perr);          /* AH=59h GET EXTENDED ERROR */
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
     * its length, and psp_build re-frames it (count byte + CR) at PSP:80h.
     *
     * EXEC env inheritance (beads initech-1i0x Tranche E inc 3): the SAME param
     * block carries env_block at offset 0 (a FLAT linear ptr; 0 = inherit the
     * parent's / inherit-empty, per dos_structs.h). We thread it to the EXEC
     * backend unchanged -- the loader decides whether to synthesize the empty
     * block (env_block==0) or honor the caller's populated block (the shell
     * serializes its master env into ENV_BLOCK and passes env_block==ENV_BLOCK).
     * An absent/unreadable EBX block degrades to env_block=0 (inherit-empty),
     * exactly like the no-arg tail degrade above (Rule 2 -- never fault on a
     * legacy caller's stale EBX). */
    const char *tail_text = 0;
    uint32_t    tail_len  = 0;
    uint32_t    env_block = 0;
    {
        uint32_t pb = f->ebx;
        if (pb != 0 && user_buf_ok(pb, (uint32_t)sizeof(exec_param_block_t))) {
            const exec_param_block_t *blk = (const exec_param_block_t *)(uintptr_t)pb;
            /* Sanitize the caller's env_block exactly as the cmd_tail below is guarded
             * (Rule 2 -- never fault on a legacy/stale EBX, this function's stated
             * intent): loader_decide_env honors ONLY {0, ENV_BLOCK}, so any OTHER value
             * -- a caller that left EBX pointing at a block whose env_block field is
             * stale garbage, or a real-DOS program passing its own env segment we do not
             * yet copy -- is degraded to inherit-empty instead of failing the EXEC
             * (loader_decide_env's BAD_ENV mapped to BAD_FORMAT at the kmain seam). The
             * shell's populated-env path (env_block == ENV_BLOCK) is preserved. */
            env_block = blk->env_block;
            if (env_block != 0u && env_block != (uint32_t)ENV_BLOCK) {
                env_block = 0u;
            }
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

    /* Default this child's termination type to 0 (normal) BEFORE it runs. A child
     * that ends via AH=31h KEEP overwrites it to 3 from inside its own do_keep
     * (which executes synchronously within g_exec, before control unwinds back
     * here); a child that ends normally leaves it 0. Resetting here -- not after --
     * is what keeps a normal child run from inheriting a PRIOR KEEP child's type
     * (beads initech-bo40; DOS 3.3 4Dh reports the type of the child JUST run). */
    g_last_child_term_type = 0u;

    uint32_t indos_snapshot = g_indos;
    uint8_t rc = 0;
    uint16_t err = g_exec(leaf, dir_start, tail_text, tail_len, env_block, &rc);
    g_indos = indos_snapshot;
    if (err != 0) {
        /* Load/run failed (not found / nested / too big). Fail loud (Rule 2). */
        set_ax(f, err);
        cf_set(f);
        return;
    }

    /* The child ran and returned. Stash its exit code for AH=4Dh; CF clear. DOS
     * leaves the child's rc retrievable via 4Dh, not in AX of the 4Bh return. The
     * termination type was defaulted to 0 above and overwritten to 3 by the child's
     * own do_keep iff it ended via AH=31h KEEP -- so it already reflects this child
     * (beads initech-bo40); no action needed here. */
    g_last_child_rc = rc;
    cf_clear(f);
}

/* AH=4Dh GET RETURN CODE (WAIT): return the exit code of the last child run via
 * AH=4Bh EXEC. AL = the exit code; AH = the termination TYPE (0 = normal exit via
 * 4Ch / INT 20h / 00h; 3 = terminated by AH=31h KEEP -- the TSR case, beads
 * initech-bo40). Ctrl-Break(1) / critical-error(2) have no termination path yet.
 * DOS "consumes" the value (a second 4Dh reads 0), which we model by clearing the
 * stash after read. CF clear. Ref: DOS 3.3 PRM AH=4Dh; RBIL INT 21/AH=4Dh. */
static void do_get_return_code(int_frame_t *f)
{
#ifdef INT21_MUTATE_RETCODE_ZERO
    /* MUTANT (Rule 6; make test-exec-mutant only): always report rc=0 regardless
     * of the child's actual exit code, so the 4Dh oracle (which EXECs a program
     * that exits rc=7 and asserts AL==7) goes RED. NEVER define in a real build. */
    f->eax = (f->eax & 0xFFFF0000u);          /* AL=0, AH=0 */
#else
    /* AL = exit code; AH = termination type (3 == KEEP/TSR, else 0 normal). */
    f->eax = (f->eax & 0xFFFF0000u)
           | ((uint32_t)g_last_child_term_type << 8)
           | (uint32_t)g_last_child_rc;
#endif
    g_last_child_rc        = 0u;   /* consumed (DOS clears after the read) */
    g_last_child_term_type = 0u;   /* consumed alongside the code          */
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

/* AH=31h KEEP (Terminate and Stay Resident): AL = exit/return code, DX = the
 * number of paragraphs to keep RESIDENT, measured from the program's PSP segment.
 * Like terminate (return control to the parent / the EXEC'r), BUT the program's
 * memory block is NOT freed: it is shrunk to DX paragraphs and re-homed to the
 * DOS system arena (MCB_OWNER_SYSTEM) so a later AH=48h ALLOC never reuses it and
 * the terminate-time owner reclaim (do_terminate -> mcb_free_owner) SKIPS it. The
 * parent retrieves AL via AH=4Dh, which then reports AH=3 (terminated by KEEP).
 *
 * FLAT-MODE PARAGRAPH MAPPING (Law 1; DOS 3.3 PRM / RBIL INT 21/AH=31h, adapted):
 * real DOS DX counts paragraphs from the PSP segment, and the PSP is the first 16
 * paragraphs (256 bytes) of the program's block. In our MCB model the program's
 * single-big-block (mcb_set_arena_owner / int21_mcb_reset) has its DATA region
 * starting at data paragraph 1 (the 1-paragraph MCB header sits at data_para 0),
 * and the PSP occupies the FIRST data paragraph of that region (PROGRAM_BASE). So
 * DX paragraphs "from the PSP" == DX DATA paragraphs of the block -- a 1:1 map we
 * pass straight to mcb_keep_resident (which clamps a too-large DX to the block's
 * own size; DOS keeps at most the program's own block).
 *
 * Fail loud (Rule 2): if the current PSP owns no arena block (e.g. the disjoint-
 * arena bind that leaves the window FREE -- the program never claimed a block to
 * keep), mcb_keep_resident returns BAD_BLOCK; KEEP then terminates normally
 * (there is nothing to keep) rather than corrupting the chain. KEEP is otherwise
 * EXACTLY terminate: it routes through the SAME do_terminate return path (handle
 * close, owner reclaim of whatever is STILL owned by the PSP, CWD reset, exit
 * hook), differing only in that the kept block is already re-homed resident so
 * the reclaim leaves it standing. Ref: DOS 3.3 PRM AH=31h; ADR-0003 (the arena
 * model); spec/int21h_register.json (31h KEEP, Resident). */
static void do_keep(int_frame_t *f)
{
    uint8_t  code = frame_al(f);
    uint16_t keep_paras = frame_dx(f);

    /* Mark the program's block resident BEFORE the terminate reclaim runs. With
     * an arena + a live PSP bound, re-home the PSP's block to MCB_OWNER_SYSTEM at
     * DX paragraphs; BAD_BLOCK (the PSP owns nothing to keep) degrades to a normal
     * terminate (nothing kept), never a fault. */
    if (g_arena_bound && g_cur_psp != 0) {
        (void)mcb_keep_resident(&g_arena, cur_psp_owner(), (uint32_t)keep_paras);
    }

    /* Record this process's exit status for the parent's AH=4Dh: AL = the exit
     * code, AH = termination type 3 (KEEP/TSR). In the kernel build the loader
     * regains control after the exit hook and do_exec re-stashes the SAME code
     * (g_last_child_rc) from the loader's captured rc; recording it here makes the
     * status retrievable on the DIRECT-dispatch path too (and is byte-consistent
     * with the EXEC path -- both store the identical code). The type is set to 3
     * here and defaults back to 0 on the next EXEC of a normal child (do_exec). */
    g_last_child_rc        = code;
    g_last_child_term_type = 3u;

    /* Terminate-to-parent: the SAME control transfer as 4Ch. do_terminate's
     * mcb_free_owner reclaim now finds nothing owned by the PSP (the kept block is
     * MCB_OWNER_SYSTEM), so the resident block survives. */
    do_terminate(f, code);
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

/* AH=33h GET/SET CTRL-BREAK STATE (beads initech-er3h; ADR-0003 Amendment
 * DEC-16, OEA-ADR-0003-A4, RATIFIED 2026-06-15). AL selects the sub-function;
 * DL carries the 1-byte flag value in/out. Register-role caution (DEC-16 Sec
 * 3.2 anti-transposition): AL is the sub-selector and DL is the value -- EDX is
 * NOT a pointer here, unlike most DEC-04a functions.
 *
 *   AL=00h GET: DL = current g_break_flag (0=OFF, 1=ON); CF=0. (DEC-16 3.2.)
 *   AL=01h SET: g_break_flag = (DL != 0) -- NORMALIZED, so a later GET returns
 *               exactly 0/1 (never a raw DL like 0xFF); AL = the normalized
 *               value; CF=0. (DEC-16 3.2 DL-normalization-on-SET.)
 *   AL=other  : out of scope (the DOS-5 AL=05h/06h/07h sub-functions are
 *               post-3.3 and fenced off) -> CF=1, AX=0x0001 INVALID_FUNCTION +
 *               a grep-able serial diagnostic; NEVER a silent no-op, NEVER
 *               misread as get/set (DEC-16 3.2 error contract; Rule 2 fail-loud).
 * AL=00h/01h never set CF (a pure state read/write, no failure mode). Ref:
 * DOS 3.3 PRM Function 33h; RBIL INT 21h/AH=33h; spec/int21h_register.json (the
 * AH=33h BREAK row) + spec/int21h_calling_convention.json (the AH=33h stanza). */
static void do_break(int_frame_t *f)
{
    uint8_t al = frame_al(f);

    if (al == 0x00u) {                  /* GET */
        /* DL = current flag (0/1); preserve DH + the upper EDX (write only DL). */
        f->edx = (f->edx & 0xFFFFFF00u) | (uint32_t)g_break_flag;
        cf_clear(f);
        return;
    }
    if (al == 0x01u) {                  /* SET */
        uint8_t dl = frame_dl(f);
#if defined(INT21_MUTATE_BREAK_DL_RAW)
        /* MUTANT M2 (Rule 6; make test-er3h-mutant only): store DL VERBATIM
         * instead of the normalized boolean, so SET(DL=0xFF) then GET returns
         * 0xFF (not 1) -- the DL-normalization assertions [er3h.3]/[er3h.4] go
         * RED. NEVER in a real build (DEC-16 3.2 fixes normalize-on-write). */
        g_break_flag = dl;
#else
        g_break_flag = (uint8_t)(dl != 0u ? 1u : 0u);   /* normalize (DEC-16 3.2) */
#endif
#if defined(INT21_MUTATE_BREAK_SET_WRITES_AL)
        /* MUTANT M7 (Rule 6; make test-er3h-mutant only): wrongly write an OUTPUT
         * register on SET. DEC-16 3.2 (lines 241-242): for AL=01h "no register
         * other than the saved flag state changes" -- SET returns nothing in AL.
         * Writing AL makes the [er3h.3]/[er3h.4] AL-unchanged assertions go RED.
         * NEVER in a real build. */
        set_al(f, g_break_flag);
#endif
        /* DEC-16 3.2: SET updates ONLY the flag + clears CF; it writes NO output
         * register (AL/DL are left exactly as the caller passed them). */
        cf_clear(f);
        return;
    }

    /* Out-of-scope AL (the DOS-5 sub-functions, or any AL not 00h/01h) -- fail
     * loud (DEC-16 3.2 / Rule 2). Distinct, grep-able serial diagnostic. */
    con_puts("INT21 not-yet-impl AH=33 AL=");
    con_hex2(al);
    con_putc('\n');
    set_ax(f, INT21_ERR_INVALID_FUNCTION);
    cf_set(f);
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
 * RETURNS the action to the failed caller (DOS contract). The disk layer now
 * raises this on a hard sector-I/O failure through int21_run_critical_error
 * (beads initech-mvg) -- a `int $0x24` software-INT or that helper both land
 * here -- and honors the returned AL (Retry re-issues, Abort terminates the
 * program, Fail propagates the error). */
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
        case 0x03:                       /* AUX INPUT (COM1 -> AL; beads initech-40oq) */
            do_aux_input(frame);
            return;
        case 0x04:                       /* AUX OUTPUT (DL -> COM1; beads initech-40oq) */
            do_aux_output(frame);
            return;
        case 0x05:                       /* PRINT OUTPUT (DL -> LPT1; beads initech-40oq) */
            do_prn_output(frame);
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
#ifdef CERT_MUTATE_DROP_GETVER
            /* MUTANT (Rule 6; make test-40oq-mutant only): drop the AH=30h
             * dispatch so it falls to the not-yet-impl arm and the test_40oq
             * dynamic safe-set check sees "not-yet-impl AH=30" -> RED.
             * The (void) reference keeps -Werror=unused-function quiet so the
             * mutant still COMPILES + RUNS. NEVER define in a real build. */
            (void)do_getver;
            break;
#else
            do_getver(frame);
            return;
#endif
        case 0x33:                       /* GET/SET CTRL-BREAK STATE (DEC-16) */
#ifdef INT21_MUTATE_BREAK_NO_DISPATCH
            /* MUTANT M1/M3 base (Rule 6; make test-er3h-mutant only): do NOT
             * dispatch 0x33; fall through to the listed-but-not-yet-impl path
             * (CF=1, AX=0x0001) so every AH=33h GET/SET oracle goes RED. The
             * (void) ref keeps -Werror=unused-function quiet so the mutant still
             * BUILDS + RUNS. NEVER in a real build. */
            (void)do_break;
            break;
#else
            do_break(frame);
            return;
#endif
        case 0x3C:                       /* CREAT (create/truncate file) */
            do_creat(frame);
            return;
        case 0x5B:                       /* CREATNEW (create, fail if exists) */
#ifdef INT21_MUTATE_CREATNEW_NO_DISPATCH
            /* MUTANT (Rule 6): do NOT dispatch 0x5B; fall through to the
             * not-yet-impl path (CF=1, AX=0x0001). The (void) ref keeps
             * -Werror=unused-function quiet so the mutant still BUILDS + RUNS. */
            (void)do_creatnew;
            break;
#else
            do_creatnew(frame);
            return;
#endif
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
        case 0x56:                       /* RENAME (same-directory dir-entry rename) */
#ifdef INT21_MUTATE_RENAME_NO_DISPATCH
            /* MUTANT (Rule 6): do NOT dispatch 0x56; fall through to the not-yet-
             * impl path (CF=1, AX=0x0001). The (void) ref keeps -Werror=unused-
             * function quiet so the mutant still BUILDS + RUNS. NEVER in a real
             * build. */
            (void)do_rename;
            break;
#else
            do_rename(frame);
            return;
#endif
        case 0x43:                       /* CHMOD (get/set file attributes) */
#ifdef INT21_MUTATE_CHMOD_NO_DISPATCH
            /* MUTANT 6 (Rule 6; make test-b53d-mutant only): do NOT dispatch
             * 0x43; fall through to the not-yet-impl path so CHMOD returns
             * CF=1/AX=0x0001 and every chmod oracle goes RED. The (void) ref
             * keeps -Werror=unused-function quiet so the mutant still BUILDS +
             * RUNS. NEVER in a real build. */
            (void)do_chmod;
            break;
#else
            do_chmod(frame);
            return;
#endif
        case 0x42:                       /* LSEEK (move file pointer) */
            do_lseek(frame);
            return;
        case 0x57:                       /* GET/SET FILE DATE+TIME (by handle) */
#ifdef INT21_MUTATE_FILETIME_NO_DISPATCH
            /* MUTANT (Rule 6): do NOT dispatch 0x57; fall through to the
             * not-yet-impl path. The (void) ref keeps -Werror=unused-function
             * quiet so the mutant still BUILDS + RUNS. NEVER in a real build. */
            (void)do_filetime;
            break;
#else
            do_filetime(frame);
            return;
#endif
        case 0x44:                       /* IOCTL (AL=00 get device info) */
#ifdef INT21_MUTATE_IOCTL_NO_DISPATCH
            /* MUTANT (Rule 6): do NOT dispatch 0x44; fall through to the
             * not-yet-impl path (CF=1, AX=0x0001). The (void) ref keeps
             * -Werror=unused-function quiet so the mutant still BUILDS + RUNS. */
            (void)do_ioctl;
            break;
#else
            do_ioctl(frame);
            return;
#endif
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
        case 0x31:                       /* KEEP (Terminate and Stay Resident) */
            do_keep(frame);
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

/* ---- CRITICAL-ERROR CHOKE POINT (beads initech-mvg) -----------------------
 * THE single in-kernel entry the disk layer (ata.c's critical-error wrapper
 * blockdev) calls when a hard sector I/O fails. It RAISES INT 24h by running the
 * real handler over a synthesized frame -- so the SAME MSG-DOS-0001 prompt +
 * A/R/F read + crit_error_action mapping the `int $0x24` software-interrupt path
 * uses is exercised here (Law 2: one code path, one oracle) -- then maps the
 * returned AL to the disk layer's decision:
 *
 *   AL=0 Retry -> return INT21_CRIT_RETRY : the wrapper RE-ISSUES the sector op.
 *   AL=1 Abort -> TERMINATE the current program through the REAL do_terminate
 *                 path (handle/memory/CWD reclaim + the bound exit hook) with the
 *                 DOS abort code 0x23 ('#', "terminated by INT 24h" -- never a
 *                 new teardown, Rule 3), then -- if the exit hook returns (the
 *                 host-oracle default; the kernel hook does NOT return, it
 *                 long-jumps to the loader) -- return INT21_CRIT_FAIL so a stray
 *                 caller still propagates an error rather than continuing as if
 *                 the I/O succeeded (Rule 2 fail-loud).
 *   AL=2 Fail  -> return INT21_CRIT_FAIL  : the wrapper propagates the error
 *                 (CF=1 / the DOS error code) up the syscall (current behavior).
 *   Ignore (AL=3) is DEFERRED (the int24 handler never returns it -- the A/R/F
 *                 prompt has no Ignore key this milestone); were it ever returned
 *                 it is treated as Fail (the bead's defer-Ignore-as-Fail note).
 *
 * `op_is_write` (0 read / 1 write) is accepted for a future per-direction
 * diagnostic + matches the DOS INT 24h AH bit0 (read/write) semantics; it does
 * not change the A/R/F contract here. The frame is local (NOT the interrupted
 * caller's): int24_dispatch only reads/writes AL+CF on it, so a synthetic frame
 * is sufficient and keeps the disk layer ignorant of int_frame_t. Ref: DOS 3.3
 * INT 24h (RBIL INT 24h: AL return 0=Ignore/1=Retry/2=Abort/3=Fail historically
 * varies; we use the crit_error_action A/R/F mapping pinned in int21.h); ADR-0003
 * DEC-10. */
int int21_run_critical_error(int op_is_write)
{
    int_frame_t f;
    uint8_t     action;
    uint32_t    i;

    (void)op_is_write;   /* reserved for a future per-direction diagnostic */

    /* A zeroed frame with CF preset so we can prove int24 CLEARS it (mirrors the
     * host oracle's fresh_frame; the only fields int24 touches are AL + CF). */
    for (i = 0u; i < sizeof(f); i++) {
        ((uint8_t *)&f)[i] = 0u;
    }
    f.eflags = 0x00000202u | CF_BIT;   /* IF + reserved bit1; CF preset */

#if defined(MVG_MUTATE_NO_RAISE)
    /* MUTANT (Rule 6; make test-int24-wired-mutant only): return Fail WITHOUT
     * raising INT 24h -- the regression this bead fixes (a disk error never
     * presents the operator prompt). MSG-DOS-0001 is never emitted and Retry
     * never re-issues, so scenarios [A]/[B]/[C]/[D] go RED. NEVER in a real
     * build. The (void) refs keep -Werror=unused quiet so the mutant still
     * BUILDS + RUNS. */
    (void)f; (void)action;
    return INT21_CRIT_FAIL;
#else
    int24_dispatch(&f);                /* present MSG-DOS-0001, read A/R/F -> AL */
    action = (uint8_t)(f.eax & 0xFFu);
#endif

    switch (action) {
        case 0u:                       /* Retry */
            return INT21_CRIT_RETRY;
        case 1u:                       /* Abort */
            /* Terminate the current program through the REAL terminate path with
             * the DOS "terminated by INT 24h" exit code 0x23. do_terminate runs
             * the handle/memory/CWD reclaim then the bound exit hook (which does
             * not return in the kernel build). If it DOES return (host default),
             * fall through to FAIL so we never report a fake success (Rule 2). */
            do_terminate(&f, 0x23u);
            return INT21_CRIT_FAIL;
        case 2u:                       /* Fail */
        default:                       /* Ignore (3) deferred -> treat as Fail */
            return INT21_CRIT_FAIL;
    }
}

/* ==== INT 25h / 26h ABSOLUTE DISK READ/WRITE (ADR-0003 DEC-15) ==============
 * beads initech-4mq7. Ref: spec/absdisk_int2526.json (the LOCKED contract);
 * docs/adr/ADR-0003-AMENDMENT-DEC-15-INT25h-26h-Absolute-Disk-Vectors.md.
 * Two NEW separate software-interrupt vectors over the bound g_absdisk block-
 * device seam -- NOT INT 21h AH functions (AH=25h SETVECT / AH=35h GETVECT are
 * distinct, above). The error AX uses the SEPARATE absolute-disk hardware-error
 * space (ABSDISK_AL_x / ABSDISK_AH_x, int21.h), NOT INT21_ERR_x.
 *
 * Helper: write AX = (ah<<8 | al) + set CF (the DOS failure return). Writes only
 * the low 16 bits of EAX (set_ax) + CF -- the rest of the frame (EBX/ECX/EDX/
 * ESI/EDI/EBP, the DEC-04a benign superset) is preserved by the stub's popad. */
static void absdisk_fail(int_frame_t *f, uint8_t al, uint8_t ah)
{
#if defined(ABSDISK_MUTATE_WRONG_ERRCLASS)
    /* MUTANT M5 (Rule 6): return an INT 21h INT21_ERR_* extended-error code
     * (the WRONG code space) instead of the absolute-disk AL/AH hardware pair.
     * The error-AX assertions [8] check the exact ABSDISK_AL_x / ABSDISK_AH_x
     * values, so they go RED -- proving the test asserts the locked spec-data
     * contract (the two code spaces are NOT conflated; DEC-15.2 / DEC-15.4). */
    (void)al; (void)ah;
    set_ax(f, INT21_ERR_INVALID_FUNCTION);   /* 0x0001 -- a DIFFERENT space */
    cf_set(f);
#else
    set_ax(f, (uint16_t)(((uint16_t)ah << 8) | (uint16_t)al));
    cf_set(f);
#endif
}

/* The shared decode + validation + I/O route for INT 25h (op_write==0) and INT
 * 26h (op_write==1). The ORDER is fail-loud, BEFORE any I/O (DEC-15.2):
 *   1. reject CX=0xFFFF packet sentinel (never a literal 65535-count; DEC-15.5);
 *   2. validate AL==0 (the single mounted volume; else invalid-drive);
 *   3. no seam bound -> fail loud (never fault, Rule 2);
 *   4. CX==0 -> no-op success CF=0 (DOS contract; touch no device);
 *   5. user_buf_ok(EBX, CX*512), guarding CX*512 against 32-bit wrap (DEC-14);
 *   6. bounds-check DX>=total OR DX+CX>total OR DX+CX wraps -> sector-not-found
 *      (BEFORE the seam call -- a raw LBA past EOF fails loud, never a short
 *      read);
 *   7. INT 26h with a read-only backend (write==NULL) -> write-protect;
 *   8. route to the seam; a negative return -> general failure;
 *   9. success -> CF=0.
 * AL=drive, CX=ECX count, DX=EDX start LBA, EBX=flat buffer (the DEC-15
 * register-role SWAP -- EBX is the buffer, DX/EDX the start sector). */
static void absdisk_body(int_frame_t *frame, int op_write)
{
#if defined(ABSDISK_MUTATE_REG_TRANSPOSE)
    /* MUTANT M3 (Rule 6; make test-absdisk-mutant only): read the start sector
     * from CX and the count from DX -- the DX/CX register transposition the
     * DEC-15 register layout forbids. The round-trip writes/reads the WRONG LBA
     * (and count), so the round-trip + cross-check oracle goes RED. */
    uint8_t  drive = (uint8_t)(frame->eax & 0xFFu);            /* AL          */
    uint32_t count = frame->edx & 0xFFFFu;                     /* (DX) wrong  */
    uint32_t lba   = frame->ecx & 0xFFFFu;                     /* (CX) wrong  */
#else
    uint8_t  drive = (uint8_t)(frame->eax & 0xFFu);            /* AL          */
    uint32_t count = frame->ecx & 0xFFFFu;                     /* CX          */
    uint32_t lba   = frame->edx & 0xFFFFu;                     /* DX          */
#endif
    uint32_t buf   = frame->ebx;                               /* EBX         */
    uint32_t bytes;
    int      rc;

#if defined(ABSDISK_MUTATE_OFF_BY_ONE_LBA)
    /* MUTANT M1 (Rule 6): add a spurious +1 to the LBA -- an absolute call must
     * NOT offset the LBA. The write lands at scratch+1 and the read at scratch+1
     * (or the read-back at scratch sees stale), so the round-trip [5] AND the
     * independent file cross-check [6] go RED. */
    lba = lba + 1u;
#endif

    /* [1] DOS4+ packet sentinel: reject CF=1 + a grep-able serial diagnostic
     * (the controlled-scope not-yet-impl pattern). NEVER a literal 65535 count
     * read off the end of the volume (DEC-15.5). Surface it as sector-not-found
     * so a caller sees a coherent CF/AX, and emit the marker.
     *
     * MUTANT M8 (Rule 6; ABSDISK_MUTATE_PACKET_LITERAL): suppress this reject so
     * CX=0xFFFF is treated as a LITERAL 65535-sector count -- it then falls into
     * the bounds-check [6] (DX+65535 > total) / off-the-end read, so the packet-
     * rejection oracle leg goes RED (it expected the loud reject). */
#if !defined(ABSDISK_MUTATE_PACKET_LITERAL)
    if ((frame->ecx & 0xFFFFu) == ABSDISK_PACKET_SENTINEL) {
        con_puts("ABSDISK-PACKET-REJECT\n");   /* DOS4+ CX=0xFFFF, out of scope */
        absdisk_fail(frame, ABSDISK_AL_SECTOR_NOT_FOUND,
                     ABSDISK_AH_SECTOR_NOT_FOUND);
        return;
    }
#endif

    /* [2] Drive: AL is ZERO-BASED EXPLICIT (0=A:). This single-volume milestone
     * mounts one volume, so the only valid drive is 0; anything else fails loud
     * with invalid-drive (never a silent success, never a fault -- DEC-15.2). */
    if (drive != 0u) {
        absdisk_fail(frame, ABSDISK_AL_INVALID_DRIVE, ABSDISK_AH_INVALID_DRIVE);
        return;
    }

    /* [3] No seam bound -> fail loud, never fault (Rule 2 / DEC-15 C-4). Map to
     * general failure (the device is unreachable). */
    if (!g_absdisk_bound) {
        absdisk_fail(frame, ABSDISK_AL_GENERAL_FAILURE,
                     ABSDISK_AH_GENERAL_FAILURE);
        return;
    }

    /* [4] Zero count: no-op success, touch no device (DOS contract). */
    if (count == 0u) {
        cf_clear(frame);
        return;
    }

    /* [5] Validate the transfer buffer span [EBX, EBX + CX*512) BEFORE any I/O
     * (DEC-14). Guard CX*512 against a 32-bit wrap of the byte span: CX is at
     * most 0xFFFE here (0xFFFF rejected at [1]), so CX*512 fits in 32 bits, but
     * keep the multiply explicit and let user_buf_ok catch ptr+span wrap. */
    bytes = count * 512u;
    if (!user_buf_ok(buf, bytes)) {
        absdisk_fail(frame, ABSDISK_AL_INVALID_DRIVE, ABSDISK_AH_INVALID_DRIVE);
        return;
    }

    /* [6] Bounds-check against the mounted geometry BEFORE the seam call: a raw
     * LBA at/past EOF must fail loud (never a short/garbage read). DX+CX wrap is
     * a count overflow. All three -> sector-not-found (DEC-15.2). */
#if defined(ABSDISK_MUTATE_NO_BOUNDS)
    /* MUTANT M4 (Rule 6): drop the bounds check entirely -- DX>=total then falls
     * through to the seam (a real backend short-reads / off-the-end), so the
     * out-of-range / overflow error-path oracle legs [8] go RED (they expected
     * CF=1 + sector-not-found, not a CF=0 pass-through). */
    if (0) {
#else
    if (lba >= g_absdisk.total_sectors ||
        (lba + count) < lba ||                 /* DX+CX 32-bit wrap            */
        (lba + count) > g_absdisk.total_sectors) {
#endif
        absdisk_fail(frame, ABSDISK_AL_SECTOR_NOT_FOUND,
                     ABSDISK_AH_SECTOR_NOT_FOUND);
        return;
    }

    if (op_write) {
        /* [7] Read-only backend (write member NULL) -> write-protect (DEC-15.2;
         * ties to MSG-DOS-0008). */
        if (g_absdisk.write == 0) {
            absdisk_fail(frame, ABSDISK_AL_WRITE_PROTECT,
                         ABSDISK_AH_WRITE_PROTECT);
            return;
        }
#if defined(ABSDISK_MUTATE_WRITE_NOOP)
        /* MUTANT M2 (Rule 6): stub-drop the WRITE -- claim success (rc=0) but do
         * no I/O. The read-back [5] sees stale bytes and the independent file
         * cross-check [6] disagrees, so both go RED. */
        rc = 0;
#elif defined(ABSDISK_MUTATE_WRITE_NEIGHBOR)
        /* MUTANT M6 (Rule 6): write count+1 sectors when count was asked -- it
         * scribbles the ADJACENT sector. The non-corruption snapshot [7] of the
         * neighbor (and the boot/FAT/root snapshot if scratch-1 is in range) goes
         * RED. */
        rc = g_absdisk.write(lba, count + 1u, (const void *)(uintptr_t)buf);
#else
        rc = g_absdisk.write(lba, count, (const void *)(uintptr_t)buf);
#endif
    } else {
        if (g_absdisk.read == 0) {
            absdisk_fail(frame, ABSDISK_AL_GENERAL_FAILURE,
                         ABSDISK_AH_GENERAL_FAILURE);
            return;
        }
        rc = g_absdisk.read(lba, count, (void *)(uintptr_t)buf);
    }

    /* [8] A negative seam return -> general failure (the single honest mapping;
     * the blockdev seam has no finer hardware-error granularity -- DEC-15.2 /
     * Consequence C-5). */
    if (rc < 0) {
        absdisk_fail(frame, ABSDISK_AL_GENERAL_FAILURE,
                     ABSDISK_AH_GENERAL_FAILURE);
        return;
    }

    /* [9] Success: CF=0. */
    cf_clear(frame);

#if defined(ABSDISK_MUTATE_STACK_WART)
    /* MUTANT M7 (Rule 6): MODEL the leftover-FLAGS-on-stack wart DEC-15.3
     * deliberately OMITS. The real wart leaves a stray FLAGS word on the caller
     * stack so the IRETD frame is DESYNCHRONIZED -- the saved EFLAGS the caller
     * pops is wrong beyond the documented CF. We model that desync by scribbling
     * the EFLAGS frame OUTSIDE bit 0 (the dispatcher's only sanctioned EFLAGS
     * output). The oracle's frame-balance assertion verifies the dispatcher
     * touched ONLY EFLAGS bit 0 (CF) -- every other bit preserved byte-identical,
     * the uniform-IRETD/balanced-frame guarantee -- so this mutant goes RED.
     * NEVER in a real build: the real stub returns a balanced frame. */
    frame->eflags ^= 0x00000800u;   /* flip a non-CF bit (the desync) */
#endif
}

static void int25_dispatch_body(int_frame_t *frame)
{
    absdisk_body(frame, 0);   /* INT 25h = absolute READ */
}

static void int26_dispatch_body(int_frame_t *frame)
{
    absdisk_body(frame, 1);   /* INT 26h = absolute WRITE */
}

/* INT 25h/26h share the SAME reentrancy guard + InDOS bracket as INT 21h/24h
 * (DEC-15.1, Consequence C-7): they share the FAT/sector scratch with INT 21h,
 * so an ISR (or a driver it calls) issuing `int 0x25`/`int 0x26` would corrupt
 * an interrupted syscall -- the guard fails loud first (Rule 2). The bodies
 * RETURN a result + CF (do_terminate is not on their path), so each g_indos--
 * runs. MIRRORS int24_dispatch exactly. */
void int25_dispatch(int_frame_t *frame)
{
    if (irq_depth() != 0u) {
        dos_reentry_panic();   /* never returns */
    }
    g_indos++;
    int25_dispatch_body(frame);
    g_indos--;
}

void int26_dispatch(int_frame_t *frame)
{
    if (irq_depth() != 0u) {
        dos_reentry_panic();   /* never returns */
    }
    g_indos++;
    int26_dispatch_body(frame);
    g_indos--;
}
