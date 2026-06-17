/*
 * os/samir/pal/pal_host.c -- SAMIR PAL host binding (libc stdio + injectable clock).
 *
 * FACTORY code (CLAUDE.md Law 3): this is the host/oracle binding; libc OK.
 * Engine code (core/, fs/, cmd/, ui/) compiles freestanding against pal.h only.
 * THIS FILE MUST NOT be linked into the shipped OS artifact.
 *
 * Implements every samir_pal vtable slot (pal.h) backed by:
 *   - File I/O: fopen/fread/fwrite/fseek/ftell/remove/rename over a fixed-size
 *     FILE* table indexed by pal_fd (slot 0..PAL_HOST_FD_MAX-1).
 *   - Console: fwrite to stdout (conout); fgets-style cooked read (conin_line);
 *     getchar/no-op stubs for the terminal-extension slots (S8.4).
 *   - Clock: the INJECTED cfg date (deterministic; Rule 11). NEVER wall clock.
 *   - Arena: bump allocator over a malloc'd backing buffer; reset(mark) unwinds
 *     to mark (or base if NULL).
 *
 * errno -> PAL_* mapping (bead initech-586.5.3 S0.1-FOLLOWUP):
 *   ENOENT, ENOFILE    -> PAL_ENOENT (2)
 *   EACCES, EPERM,
 *   EBADF, EINVAL      -> PAL_EACCES (5)
 *   ENOSPC, EDQUOT     -> PAL_ENOSPC (28)
 *   EIO, all others    -> PAL_EIO    (29)
 * The raw errno magnitude is NEVER returned through the contract.
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S0.2 + Sec 2.B (PAL portability)
 *   - os/samir/include/samir/pal.h (the full contract + doc-comments)
 *   - ADR-0008 DEC-02 (seek=filesize idiom; conin_char/gotoxy/set_attr stubs)
 *
 * ASCII-clean (Rule 12). No timestamps / host paths in artifacts (Rule 11).
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "samir/pal.h"

/* Maximum number of simultaneously open file handles. */
#define PAL_HOST_FD_MAX  16

/* Default arena size (bytes) if cfg specifies 0. */
#define PAL_HOST_HEAP_DEFAULT  (64u * 1024u)

/* ---- errno -> PAL_* mapping -------------------------------------------- */
/*
 * Map libc errno to a PAL error code. NEVER let the raw magnitude leak out.
 * Called after every libc I/O operation that sets errno on failure.
 */
static pal_err errno_to_pal(void)
{
    switch (errno) {
    case ENOENT:
#ifdef ENOFILE
    case ENOFILE:
#endif
        return PAL_ENOENT;

    case EACCES:
    case EPERM:
    case EBADF:
    case EINVAL:
        return PAL_EACCES;

    case ENOSPC:
#ifdef EDQUOT
    case EDQUOT:
#endif
        return PAL_ENOSPC;

    default:
        return PAL_EIO;
    }
}

/* ---- host binding state ------------------------------------------------- */

typedef struct {
    samir_pal_t  vtable;           /* MUST be first: the engine casts p->vtable */

    /* File table */
    FILE        *fds[PAL_HOST_FD_MAX];

    /* Injected (fixed) date; never the wall clock. */
    uint8_t      date_yy;
    uint8_t      date_mm;
    uint8_t      date_dd;

    /* Bump arena */
    uint8_t     *heap_base;        /* malloc'd backing buffer */
    uint8_t     *heap_ptr;         /* next free byte */
    uint8_t     *heap_end;         /* one past last byte */
} pal_host_state_t;

/* Recover the state block from the vtable pointer (works because vtable is first). */
static pal_host_state_t *state_of(samir_pal_t *p)
{
    return (pal_host_state_t *)p;
}

/* ---- file I/O slots ----------------------------------------------------- */

/*
 * Allocate the next free slot in the FILE* table.
 * Returns a pal_fd index >= 0 or -1 if the table is full.
 */
static int alloc_slot(pal_host_state_t *st)
{
    int i;
    for (i = 0; i < PAL_HOST_FD_MAX; i++) {
        if (st->fds[i] == NULL)
            return i;
    }
    return -1;
}

static pal_fd host_open(samir_pal_t *p, const char *name, int mode)
{
    pal_host_state_t *st = state_of(p);
    int slot;
    FILE *f;
    const char *fmode;
    int create  = (mode & PAL_CREATE) != 0;
    int trunc   = (mode & PAL_TRUNC)  != 0;
    int access  = mode & 3; /* PAL_RD=0, PAL_WR=1, PAL_RDWR=2 */

    slot = alloc_slot(st);
    if (slot < 0)
        return -(pal_fd)PAL_EACCES;   /* table full */

    /*
     * Choose the fopen mode string:
     *   - CREATE or TRUNC with write access: always create/truncate -> "w" / "w+b"
     *   - WR (no create/trunc): open existing for writing -> "r+b"
     *   - RDWR (no create/trunc): open existing r/w -> "r+b"
     *   - RD: open for reading only -> "rb"
     *
     * Rule: CREATE|TRUNC on a read-only open is treated as create-only (write
     * happens implicitly via the create; caller can then re-open RD).
     */
    if (create || trunc) {
        if (access == PAL_RD)
            fmode = "w+b";  /* create+read back in same handle */
        else if (access == PAL_WR)
            fmode = "wb";
        else
            fmode = "w+b";  /* RDWR | CREATE -> "w+b" */
    } else {
        if (access == PAL_RD)
            fmode = "rb";
        else if (access == PAL_WR)
            fmode = "r+b";
        else
            fmode = "r+b";  /* RDWR */
    }

    errno = 0;
    f = fopen(name, fmode);
    if (f == NULL)
        return -(pal_fd)errno_to_pal();

    st->fds[slot] = f;
    return (pal_fd)slot;
}

static int host_close(samir_pal_t *p, pal_fd fd)
{
    pal_host_state_t *st = state_of(p);
    FILE *f;

    if (fd < 0 || fd >= PAL_HOST_FD_MAX || st->fds[fd] == NULL)
        return -(int)PAL_EACCES;

    f = st->fds[fd];
    st->fds[fd] = NULL;
    errno = 0;
    if (fclose(f) != 0)
        return -(int)errno_to_pal();
    return 0;
}

static int32_t host_read(samir_pal_t *p, pal_fd fd, void *buf, uint32_t n)
{
    pal_host_state_t *st = state_of(p);
    size_t got;

    if (fd < 0 || fd >= PAL_HOST_FD_MAX || st->fds[fd] == NULL)
        return -(int32_t)PAL_EACCES;
    if (n == 0)
        return 0;

    errno = 0;
    got = fread(buf, 1, (size_t)n, st->fds[fd]);
    /* got == 0 with no error means EOF; return 0 (not a short read error). */
    if (got == 0) {
        if (ferror(st->fds[fd]))
            return -(int32_t)PAL_EIO;
        return 0;   /* EOF */
    }
    /* Short read: return the real count, not an error.
     * Rule 2 / S0.1-FOLLOWUP: NEVER report short as full. */
    return (int32_t)got;
}

static int32_t host_write(samir_pal_t *p, pal_fd fd, const void *buf, uint32_t n)
{
    pal_host_state_t *st = state_of(p);
    size_t written;

    if (fd < 0 || fd >= PAL_HOST_FD_MAX || st->fds[fd] == NULL)
        return -(int32_t)PAL_EACCES;
    if (n == 0)
        return 0;

    errno = 0;
    written = fwrite(buf, 1, (size_t)n, st->fds[fd]);
    if (written == 0)
        return -(int32_t)errno_to_pal();
    /* Short write: return real count; engine treats < n as PAL_ENOSPC. */
    return (int32_t)written;
}

static int32_t host_seek(samir_pal_t *p, pal_fd fd, int32_t off, int whence)
{
    pal_host_state_t *st = state_of(p);
    int libc_whence;
    long pos;

    if (fd < 0 || fd >= PAL_HOST_FD_MAX || st->fds[fd] == NULL)
        return -(int32_t)PAL_EACCES;

    switch (whence) {
    case PAL_SEEK_SET: libc_whence = SEEK_SET; break;
    case PAL_SEEK_CUR: libc_whence = SEEK_CUR; break;
    case PAL_SEEK_END: libc_whence = SEEK_END; break;
    default:
        return -(int32_t)PAL_EACCES;
    }

    errno = 0;
    if (fseek(st->fds[fd], (long)off, libc_whence) != 0)
        return -(int32_t)errno_to_pal();

    errno = 0;
    pos = ftell(st->fds[fd]);
    if (pos < 0)
        return -(int32_t)errno_to_pal();

    /* seek(fd, 0, PAL_SEEK_END) -> returns file size (ADR-0008 DEC-02). */
    return (int32_t)pos;
}

static int host_remove(samir_pal_t *p, const char *name)
{
    (void)p;
    errno = 0;
    if (remove(name) != 0)
        return -(int)errno_to_pal();
    return 0;
}

static int host_rename(samir_pal_t *p, const char *from, const char *to)
{
    (void)p;
    errno = 0;
    if (rename(from, to) != 0)
        return -(int)errno_to_pal();
    return 0;
}

/* ---- cooked console slots ----------------------------------------------- */

static void host_conout(samir_pal_t *p, const char *s, uint32_t n)
{
    (void)p;
    fwrite(s, 1, (size_t)n, stdout);
}

static int32_t host_conin_line(samir_pal_t *p, char *buf, uint32_t cap)
{
    (void)p;
    if (cap == 0)
        return -(int32_t)PAL_EACCES;

    if (fgets(buf, (int)cap, stdin) == NULL)
        return -1;   /* EOF or error */

    /* Strip trailing newline (fgets-style: leave buf NUL-terminated). */
    {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
            len--;
        }
        return (int32_t)len;
    }
}

/* ---- terminal extension stubs (S8.4) ------------------------------------ */

static int32_t host_conin_char(samir_pal_t *p)
{
    int c;
    (void)p;
    c = getchar();
    return (c == EOF) ? -1 : (int32_t)c;
}

static void host_gotoxy(samir_pal_t *p, uint8_t row, uint8_t col)
{
    /* Host stub: no-op. Exercised fully at S8.4. */
    (void)p; (void)row; (void)col;
}

static void host_set_attr(samir_pal_t *p, uint8_t attr)
{
    /* Host stub: no-op. Exercised fully at S8.4. */
    (void)p; (void)attr;
}

/* ---- clock slot (injectable; NEVER wall clock) -------------------------- */

static void host_today(samir_pal_t *p, uint8_t *yy, uint8_t *mm, uint8_t *dd)
{
    pal_host_state_t *st = state_of(p);
    if (yy) *yy = st->date_yy;
    if (mm) *mm = st->date_mm;
    if (dd) *dd = st->date_dd;
}

/* ---- arena slots -------------------------------------------------------- */

static void *host_alloc(samir_pal_t *p, uint32_t n)
{
    pal_host_state_t *st = state_of(p);
    void *ptr;
    uint32_t avail;

    if (n == 0)
        return st->heap_ptr;   /* zero alloc returns current mark */

    /* Align to 4 bytes for safety. */
    n = (n + 3u) & ~3u;

    avail = (uint32_t)(st->heap_end - st->heap_ptr);
    if (n > avail)
        return NULL;   /* exhausted; engine fails loud on NULL */

    ptr = st->heap_ptr;
    st->heap_ptr += n;
    return ptr;
}

static void host_reset(samir_pal_t *p, void *mark)
{
    pal_host_state_t *st = state_of(p);
    uint8_t *m = (uint8_t *)mark;

    if (m == NULL) {
        /* Reset to base: free everything. */
        st->heap_ptr = st->heap_base;
    } else if (m >= st->heap_base && m <= st->heap_ptr) {
        /* Unwind to a previously returned pointer. */
        st->heap_ptr = m;
    }
    /* Ignore out-of-range marks (fail loud would require pal panics; Rule 2
     * is expressed here by silently ignoring rather than corrupting state). */
}

/* ---- constructor / destructor ------------------------------------------ */

/*
 * Configuration for pal_host_make.
 *   date_yy / _mm / _dd : the INJECTED date returned by today() (Rule 11).
 *                         Must be 1-based MM/DD; YY is two-digit 00-99.
 *   heap_size            : arena backing buffer size in bytes; 0 = default.
 */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};

/*
 * pal_host_make: allocate and initialize a host PAL binding.
 * Returns a pointer to the samir_pal_t vtable (the first field of the state
 * block), or NULL on allocation failure. Caller owns the object and must
 * call pal_host_free() when done.
 */
samir_pal_t *pal_host_make(struct pal_host_cfg cfg)
{
    pal_host_state_t *st;
    uint32_t hsz;

    st = (pal_host_state_t *)calloc(1, sizeof(pal_host_state_t));
    if (st == NULL)
        return NULL;

    /* Wire the vtable. */
    st->vtable.open       = host_open;
    st->vtable.close      = host_close;
    st->vtable.read       = host_read;
    st->vtable.write      = host_write;
    st->vtable.seek       = host_seek;
    st->vtable.remove     = host_remove;
    st->vtable.rename     = host_rename;
    st->vtable.conout     = host_conout;
    st->vtable.conin_line = host_conin_line;
    st->vtable.conin_char = host_conin_char;
    st->vtable.gotoxy     = host_gotoxy;
    st->vtable.set_attr   = host_set_attr;
    st->vtable.today      = host_today;
    st->vtable.alloc      = host_alloc;
    st->vtable.reset      = host_reset;

    /* Inject the fixed clock (deterministic; Rule 11). */
    st->date_yy = cfg.date_yy;
    st->date_mm = cfg.date_mm;
    st->date_dd = cfg.date_dd;

    /* Allocate the arena backing buffer. */
    hsz = (cfg.heap_size > 0) ? cfg.heap_size : PAL_HOST_HEAP_DEFAULT;
    st->heap_base = (uint8_t *)malloc((size_t)hsz);
    if (st->heap_base == NULL) {
        free(st);
        return NULL;
    }
    st->heap_ptr = st->heap_base;
    st->heap_end = st->heap_base + hsz;

    /* FILE* table already zeroed by calloc. */
    return &st->vtable;
}

/*
 * pal_host_free: close all open handles, free the arena, free the state block.
 */
void pal_host_free(samir_pal_t *p)
{
    pal_host_state_t *st;
    int i;

    if (p == NULL)
        return;

    st = state_of(p);

    /* Close any handles left open (be tidy; Rule 2). */
    for (i = 0; i < PAL_HOST_FD_MAX; i++) {
        if (st->fds[i] != NULL) {
            fclose(st->fds[i]);
            st->fds[i] = NULL;
        }
    }

    free(st->heap_base);
    st->heap_base = NULL;
    st->heap_ptr  = NULL;
    st->heap_end  = NULL;

    free(st);
}
