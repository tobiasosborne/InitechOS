/*
 * os/samir/pal/pal_null.c -- the null/stub PAL binding.
 *
 * THE ARTIFACT (CLAUDE.md Law 3): freestanding-legal. Includes ONLY samir/pal.h
 * (which pulls <stdint.h>). NO libc -- this is the proof that the samir_pal
 * contract (S0.1) is COMPLETE and LINKABLE without any platform support: a
 * binding that every slot fills, returning a PAL error (fail loud, Rule 2) or
 * doing nothing, depending only on the contract itself. The real bindings
 * (pal_host.c S0.2, pal_milton.c S8.1) replace it.
 *
 * Ref (Law 1): SAMIR-implementation-plan.md S0.1 oracle ("a pal_null impl
 * links; header is self-contained"); ADR-0008 Sec 5 (the contract is authored
 * on ratification).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 */
#include "samir/pal.h"

/* Every fd/int-returning slot fails loud with a PAL error; void slots no-op;
 * alloc has nothing to give. (void) the args to keep -Wextra/-Werror clean. */

static pal_fd  null_open  (samir_pal_t *p, const char *name, int mode)
{ (void)p; (void)name; (void)mode; return -(pal_fd)PAL_EACCES; }

static int     null_close (samir_pal_t *p, pal_fd fd)
{ (void)p; (void)fd; return -(int)PAL_EACCES; }

static int32_t null_read  (samir_pal_t *p, pal_fd fd, void *buf, uint32_t n)
{ (void)p; (void)fd; (void)buf; (void)n; return -(int32_t)PAL_EIO; }

static int32_t null_write (samir_pal_t *p, pal_fd fd, const void *buf, uint32_t n)
{ (void)p; (void)fd; (void)buf; (void)n; return -(int32_t)PAL_EIO; }

static int32_t null_seek  (samir_pal_t *p, pal_fd fd, int32_t off, int whence)
{ (void)p; (void)fd; (void)off; (void)whence; return -(int32_t)PAL_EIO; }

static int     null_remove(samir_pal_t *p, const char *name)
{ (void)p; (void)name; return -(int)PAL_ENOENT; }

static int     null_rename(samir_pal_t *p, const char *from, const char *to)
{ (void)p; (void)from; (void)to; return -(int)PAL_ENOENT; }

static void    null_conout(samir_pal_t *p, const char *s, uint32_t n)
{ (void)p; (void)s; (void)n; }

static int32_t null_conin_line(samir_pal_t *p, char *buf, uint32_t cap)
{ (void)p; (void)buf; (void)cap; return -1; }   /* EOF */

static int32_t null_conin_char(samir_pal_t *p)
{ (void)p; return -1; }                          /* EOF */

static void    null_gotoxy(samir_pal_t *p, uint8_t row, uint8_t col)
{ (void)p; (void)row; (void)col; }

static void    null_set_attr(samir_pal_t *p, uint8_t attr)
{ (void)p; (void)attr; }

static void    null_today(samir_pal_t *p, uint8_t *yy, uint8_t *mm, uint8_t *dd)
{ (void)p; if (yy) *yy = 0; if (mm) *mm = 0; if (dd) *dd = 0; }

static void   *null_alloc(samir_pal_t *p, uint32_t n)
{ (void)p; (void)n; return (void *)0; }

static void    null_reset(samir_pal_t *p, void *mark)
{ (void)p; (void)mark; }

/* The exported null vtable: every slot bound (the completeness proof). */
samir_pal_t samir_pal_null = {
	null_open,
	null_close,
	null_read,
	null_write,
	null_seek,
	null_remove,
	null_rename,
	null_conout,
	null_conin_line,
	null_conin_char,
	null_gotoxy,
	null_set_attr,
	null_today,
	null_alloc,
	null_reset
};
