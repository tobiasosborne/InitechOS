/*
 * os/samir/include/samir/pal.h -- SAMIR (InitechBase) Platform Abstraction Layer.
 *
 * THE ARTIFACT (CLAUDE.md Law 3): this is the contract the shipped, freestanding
 * SAMIR engine compiles against. It is the engine's ONLY OS surface -- engine
 * code (core/, fs/, cmd/, ui/) touches the operating system through this vtable
 * and NOTHING ELSE. The engine NEVER issues `int 0x21` directly. Two concrete
 * bindings implement it:
 *   - pal_host.c   (factory/oracle): libc stdio + an injectable/fixed clock.
 *   - pal_milton.c (artifact):       InitechDOS INT 21h handle API + CON + arena.
 * Consequence (the operator's portability requirement): an InitechDOS drift or
 * revert touches pal_milton.c and NOTHING else -- the engine + every host oracle
 * are insulated, and Phases 0-7 are fully host-developable/gradable.
 *
 * Freestanding-legal (CLAUDE.md Law 3 / DEC-04): no libc includes. Depends ONLY
 * on <stdint.h> -- the exact int-type convention the kernel uses (see
 * os/milton/blockdev.h). Do NOT introduce custom uintN typedefs here.
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md Sec 8.1 (this contract sketch) +
 *     step S0.1; the seek=filesize idiom + the terminal extension slots.
 *   - docs/adr/ADR-0008-SAMIR-InitechBase-Architecture.md DEC-02 (the PAL is the
 *     ONLY OS surface; the DEC-02 revision adding conin_char/gotoxy/set_attr and
 *     fixing file-size = seek(fd,0,PAL_SEEK_END)).
 *   - The Milton INT 21h handle API named in DEC-02
 *     (3Dh/3Eh/3Fh/40h/42h/3Ch/41h/56h, 2Ah, CON, 07h, 48h); the calling
 *     convention is recorded in spec/int21h_calling_convention.json.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths / nondeterminism (Rule 11).
 * Fail loud (Rule 2): every fd-returning / int-returning slot reports failure as
 * a negative value; a binding MUST NOT report a short/garbage transfer as success.
 */
#ifndef INITECH_SAMIR_PAL_H
#define INITECH_SAMIR_PAL_H

#include <stdint.h>

/*
 * A platform file handle. >= 0 is a valid open handle; a negative return is
 * -(pal_err) (e.g. -PAL_ENOENT). Backed by an InitechDOS file handle on the
 * artifact (INT 21h AH=3Dh/3Ch return the handle in AX) and by an index into a
 * libc FILE* table on the host.
 */
typedef int32_t pal_fd;

/*
 * open() mode flags (bitwise OR). Map onto the Milton open/create calls:
 *   PAL_RD / PAL_WR / PAL_RDWR    -> INT 21h AH=3Dh OPEN, AL access mode
 *                                    (0=read, 1=write, 2=read/write).
 *   PAL_CREATE                    -> INT 21h AH=3Ch CREAT (create/truncate to 0)
 *                                    when the file is absent or PAL_TRUNC is set.
 *   PAL_TRUNC                     -> truncate an existing file to length 0
 *                                    (AH=3Ch on Milton; O_TRUNC on the host).
 */
enum {
	PAL_RD     = 0,
	PAL_WR     = 1,
	PAL_RDWR   = 2,
	PAL_CREATE = 4,
	PAL_TRUNC  = 8
};

/*
 * seek() whence values. Map onto INT 21h AH=42h LSEEK AL sub-function:
 *   PAL_SEEK_SET -> AL=0 (from start)
 *   PAL_SEEK_CUR -> AL=1 (from current position)
 *   PAL_SEEK_END -> AL=2 (from end of file)
 * The file-SIZE primitive is seek(fd, 0, PAL_SEEK_END): it returns the new
 * absolute position, which at offset 0 from end IS the file length. There is no
 * separate "filesize" slot -- this is the documented idiom (ADR-0008 DEC-02 rev;
 * Milton do_lseek). dbf_open's truncation-invariant check (Rule 2) and FILE()
 * both depend on it.
 */
enum {
	PAL_SEEK_SET = 0,
	PAL_SEEK_CUR = 1,
	PAL_SEEK_END = 2
};

/*
 * PAL error codes. Returned negated from fd/int-returning slots (return
 * -PAL_ENOENT, etc.). Values chosen to read naturally against DOS/errno
 * conventions; the engine compares symbolically, never on the magnitude.
 */
typedef enum {
	PAL_OK     = 0,    /* success */
	PAL_ENOENT = 2,    /* no such file / path not found */
	PAL_EACCES = 5,    /* access denied / bad handle */
	PAL_ENOSPC = 28,   /* device full (no free space) */
	PAL_EIO    = 29    /* I/O error (short read/write, device fault) */
} pal_err;

/*
 * The PAL vtable. An engine routine receives a samir_pal_t* and calls only
 * through these slots. The first argument of every slot is the vtable instance
 * itself (so a binding can carry its own state behind it -- e.g. the host
 * FILE* table + injected clock, or the Milton arena base).
 */
typedef struct samir_pal samir_pal_t;

struct samir_pal {
	/* ---- byte file-I/O (Milton: INT 21h handle API) ---- */

	/* open: open or create `name` per `mode` (PAL_RD/WR/RDWR | CREATE | TRUNC).
	 * Returns a pal_fd >= 0, or -(pal_err) on failure.
	 * Milton: AH=3Dh OPEN (existing) / AH=3Ch CREAT (create/truncate). */
	pal_fd  (*open)  (samir_pal_t *, const char *name, int mode);

	/* close: release an open handle. Returns 0 (PAL_OK) or -(pal_err).
	 * Milton: AH=3Eh CLOSE. */
	int     (*close) (samir_pal_t *, pal_fd fd);

	/* read: read up to `n` bytes into `buf`. Returns the byte count actually
	 * read (>=0; 0 = EOF), or -(pal_err). A binding MUST NOT report a short
	 * read as a full one (Rule 2). Milton: AH=3Fh READ. */
	int32_t (*read)  (samir_pal_t *, pal_fd fd, void *buf, uint32_t n);

	/* write: write `n` bytes from `buf`. Returns the byte count actually
	 * written (>=0), or -(pal_err); a short write (device full) returns < n
	 * and the engine must treat it as PAL_ENOSPC. Milton: AH=40h WRITE. */
	int32_t (*write) (samir_pal_t *, pal_fd fd, const void *buf, uint32_t n);

	/* seek: reposition `fd` to `off` relative to `whence`. Returns the new
	 * absolute position (>=0), or -(pal_err). seek(fd,0,PAL_SEEK_END) IS the
	 * file-size primitive (see PAL_SEEK_END above). Milton: AH=42h LSEEK. */
	int32_t (*seek)  (samir_pal_t *, pal_fd fd, int32_t off, int whence);

	/* remove: delete `name`. Returns 0 or -(pal_err). Milton: AH=41h UNLINK. */
	int     (*remove)(samir_pal_t *, const char *name);

	/* rename: rename `from` to `to`, SAME DIRECTORY ONLY (matches dBASE usage;
	 * ADR-0008 DEC-02 known constraint). Returns 0 or -(pal_err).
	 * Milton: AH=56h RENAME. */
	int     (*rename)(samir_pal_t *, const char *from, const char *to);

	/* ---- cooked console (the dot-prompt REPL + LIST/DISPLAY) ---- */

	/* conout: write `n` bytes of `s` to the console (cooked output, no return
	 * code -- console writes do not fail in-universe). Milton: CON via AH=40h
	 * on handle 1 (stdout). */
	void    (*conout)(samir_pal_t *, const char *s, uint32_t n);

	/* conin_line: read one cooked (line-edited, echoed) input line into `buf`
	 * of capacity `cap`. Returns the line length (>=0, no trailing newline), or
	 * < 0 on EOF/error. Milton: cooked line read on CON (handle 0, AH=3Fh). */
	int32_t (*conin_line)(samir_pal_t *, char *buf, uint32_t cap);

	/* ---- terminal extension: @SAY/GET/READ + WAIT/INKEY (S8.4) ----
	 * Part of the RATIFIED contract (ADR-0008 DEC-02 revision) so that ui/
	 * (say_get.c / read.c / browse.c) does NOT re-open pal.h when S8.4 lands.
	 * The Phase-0 host oracles exercise only the file / cooked-console / clock /
	 * arena core above; these three are stubbed until the form layer. */

	/* conin_char: read a single raw keypress, NO echo. Returns the key (>=0),
	 * or < 0 on EOF/error. Milton: AH=07h direct console input without echo
	 * (do_conin_raw, os/milton/int21.c). */
	int32_t (*conin_char)(samir_pal_t *);

	/* gotoxy: position the text cursor at (row, col), 0-based. This is an OS
	 * console primitive, NOT an INT 21h call -- on the artifact it is backed by
	 * the LFB/FLAIR text console, not DOS. */
	void    (*gotoxy)(samir_pal_t *, uint8_t row, uint8_t col);

	/* set_attr: set the text attribute (foreground/background) for subsequent
	 * conout writes; clear-to-end-of-line is expressed via conout. Console
	 * primitive, NOT INT 21h. */
	void    (*set_attr)(samir_pal_t *, uint8_t attr);

	/* ---- clock: INJECTABLE for reproducibility (Rule 11) ---- */

	/* today: return the current date as packed two-digit YY (00-99 = 1900s/2000s
	 * per CENTURY handling above the PAL), 1-based MM, 1-based DD. INJECTABLE so
	 * goldens with known dates bite deterministically; on the artifact it is
	 * Milton INT 21h AH=2Ah GET DATE, on the host a fixed/configured date. */
	void    (*today) (samir_pal_t *, uint8_t *yy, uint8_t *mm, uint8_t *dd);

	/* ---- memory: fixed bump arena (no malloc on Milton) ---- */

	/* alloc: bump-allocate `n` bytes from the arena. Returns a pointer, or NULL
	 * on exhaustion (the engine fails loud on NULL). On the artifact the arena
	 * is one large INT 21h AH=48h ALLOCATE block obtained at startup
	 * (segment<<4 -> flat); on the host it is a fixed-size backing buffer. */
	void   *(*alloc) (samir_pal_t *, uint32_t n);

	/* reset: unwind the bump arena to a previously returned `mark` (a pointer
	 * from a prior alloc, or NULL to reset to the base). Frees everything
	 * allocated after `mark`. No per-object free exists. */
	void    (*reset) (samir_pal_t *, void *mark);
};

#endif /* INITECH_SAMIR_PAL_H */
