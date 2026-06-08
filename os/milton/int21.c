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
#include "sft.h"   /* JFT->SFT handle layer (beads initech-509.3); pulls psp.h */

/* The carry flag is bit 0 of EFLAGS (Intel SDM Vol 1 Sec 3.4.3). The whole
 * error-return contract rides on this single bit (ground-truth Sec 5.4). */
#define CF_BIT 0x1u

static int21_sink_fn g_sink = 0;
static int21_exit_fn g_exit = 0;

/* The current process's PSP (beads initech-509.3). Handle functions resolve a
 * handle through g_cur_psp->jft into the system SFT. NULL until the kernel binds
 * one at SYSINIT; a handle function with no bound PSP returns invalid-handle. */
static psp_t *g_cur_psp = 0;

void int21_set_sink(int21_sink_fn sink) { g_sink = sink; }
void int21_set_exit(int21_exit_fn fn)   { g_exit = fn; }
void int21_set_psp(struct psp *psp)     { g_cur_psp = (psp_t *)psp; }

/* ---- CF helpers: write ONLY bit 0 of the saved EFLAGS image. ---- */
static void cf_clear(int_frame_t *f) { f->eflags &= ~CF_BIT; }
static void cf_set(int_frame_t *f)   { f->eflags |=  CF_BIT; }

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

/* AH=09h DISPLAY STRING: EDX -> flat ptr to a '$'-terminated string -> CON.
 * The '$' (0x24) is the terminator and is NOT emitted (DOS 3.3 convention,
 * ground-truth Sec 6.2). Returns AL='$'. CF clear. */
static void do_puts(int_frame_t *f)
{
    const char *p = (const char *)(uintptr_t)f->edx;
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
    }
    set_al(f, 0x24u);
    cf_clear(f);
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

    /* AUX/PRN devices have no driver yet; FILE writes need FAT write (deferred
     * to initech-509.11). Both are recognized handles with no write backing ->
     * AX=0x0005 (access denied), CF=1 (Rule 2: never silently drop the bytes). */
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
    if (g_exit) {
        g_exit(code);
    }
    /* If no hook is bound (host default), just return; the kernel's hook never
     * returns. We deliberately do NOT touch CF -- 4Ch has no error path. */
}

/* INT 20h legacy terminate (vector 0x20; beads initech-509.5). Routes to the
 * SAME terminate path as 4Ch with exit code 0 (ground-truth Sec 2.1 / Sec 4.4).
 * A program doing `int 0x20` (or a near RET to PSP:0 = the CD 20 there) lands
 * here. With the loader's exit hook bound this does not return (the hook unwinds
 * to load_program); with no hook bound (kernel default before a load) it returns
 * and the stub irets. */
void int20_dispatch(int_frame_t *frame)
{
    do_terminate(frame, 0u);
}

/* ---- the dispatch spine ---- */
void int21_dispatch(int_frame_t *frame)
{
    uint8_t ah = (uint8_t)((frame->eax >> 8) & 0xFFu);

    switch (ah) {
        case 0x00:                       /* TERMINATE (alias for 4Ch AL=0) */
            do_terminate(frame, 0u);
            return;
        case 0x02:                       /* DISPLAY OUTPUT */
            do_putchar(frame);
            return;
        case 0x09:                       /* DISPLAY STRING */
            do_puts(frame);
            return;
        case 0x30:                       /* GET VERSION */
            do_getver(frame);
            return;
        case 0x40:                       /* WRITE TO FILE/DEVICE */
            do_write(frame);
            return;
        case 0x45:                       /* DUP (duplicate handle) */
            do_dup(frame);
            return;
        case 0x46:                       /* DUP2 (force-duplicate handle) */
            do_dup2(frame);
            return;
        case 0x4C:                       /* TERMINATE WITH RETURN CODE */
            do_terminate(frame, frame_al(frame));
            return;
        default:
            break;
    }

    /* Controlled scope (Rule 2 / ADR-0003 DEC-13). The AH is not one of the
     * implemented functions. Two distinct, fail-loud diagnostics: */
    if (ah_is_listed(ah)) {
        /* RECOGNIZED by the locked register but not yet implemented in this
         * subset (e.g. CON input 01h/06h/0Ah need the keyboard; file/SFT
         * functions need initech-509.3). A distinct diagnostic, NOT 'unknown'. */
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
