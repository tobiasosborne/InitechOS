/*
 * harness/diff/dbf_diff/test_samir_pal_contract.c -- the PAL contract oracle (S0.1).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (TEST_HARNESS / CHECK / TEST_SUMMARY) -- a non-zero process exit on any
 * failed check (CLAUDE.md Law 2: the oracle is the truth, never false-green).
 *
 * This is the S0.1 gate: it proves the samir_pal vtable contract
 * (os/samir/include/samir/pal.h) is COMPLETE and LINKABLE. It links the
 * freestanding pal_null binding (os/samir/pal/pal_null.c), then calls EVERY
 * vtable slot exactly once through the contract and asserts each behaves as the
 * null binding documents (fail-loud negative on the fd/int slots; no-op / zero
 * on the void slots; NULL from alloc). If a slot were missing from the struct,
 * pal_null.c would not compile against pal.h and this test would not link -- so
 * a green run certifies the whole contract surface exists and is bound.
 *
 * It also smoke-checks the enums (mode flags / whence / pal_err) so the
 * documented INT 21h mappings have stable, distinct constants the bindings can
 * key on.
 *
 * Ref (Law 1): SAMIR-implementation-plan.md S0.1 + Sec 8.1 (the contract);
 *   ADR-0008 DEC-02 (the PAL is the ONLY OS surface; the terminal extension +
 *   seek=filesize idiom).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 */

#include <stdio.h>
#include <string.h>

#include "test_assert.h"   /* seed/, on -Iseed                       */
#include "samir/pal.h"     /* os/samir/include/, on -Ios/samir/include */

TEST_HARNESS();

/* The completeness proof lives in pal_null.c: a fully-bound null vtable. */
extern samir_pal_t samir_pal_null;

int main(void)
{
	samir_pal_t *p = &samir_pal_null;
	char    buf[16];
	uint8_t yy = 0xAA, mm = 0xAA, dd = 0xAA;

	/* ---- enum sanity: the documented mode / whence / error constants ---- */
	CHECK(PAL_RD == 0 && PAL_WR == 1 && PAL_RDWR == 2,
	      "open mode access bits RD/WR/RDWR == 0/1/2 (INT 21h AH=3Dh AL)");
	CHECK(PAL_CREATE == 4 && PAL_TRUNC == 8,
	      "open mode CREATE/TRUNC are distinct OR-able flags (4/8)");
	CHECK(PAL_SEEK_SET == 0 && PAL_SEEK_CUR == 1 && PAL_SEEK_END == 2,
	      "seek whence SET/CUR/END == 0/1/2 (INT 21h AH=42h AL); END = filesize");
	CHECK(PAL_OK == 0, "PAL_OK == 0 (success)");
	CHECK(PAL_ENOENT != PAL_EACCES && PAL_EACCES != PAL_EIO &&
	      PAL_EIO != PAL_ENOSPC && PAL_ENOENT != PAL_ENOSPC,
	      "pal_err codes are distinct (symbolic compare, never magnitude)");

	/* ---- every vtable slot is bound (non-NULL function pointer) ---- */
	CHECK(p->open && p->close && p->read && p->write && p->seek &&
	      p->remove && p->rename, "file-I/O slots are all bound");
	CHECK(p->conout && p->conin_line, "cooked-console slots are bound");
	CHECK(p->conin_char && p->gotoxy && p->set_attr,
	      "terminal-extension slots are bound (DEC-02 rev: @SAY/GET/READ)");
	CHECK(p->today, "clock slot is bound (injectable, AH=2Ah)");
	CHECK(p->alloc && p->reset, "arena slots are bound (AH=48h)");

	/* ---- call each slot once through the contract; assert null behaviour ---- */

	/* file-I/O: every slot fails loud with a negative (-pal_err). */
	CHECK(p->open(p, "X.DBF", PAL_RD) < 0,
	      "null open() fails loud (negative)");
	CHECK(p->close(p, 3) < 0,
	      "null close() fails loud (negative)");
	CHECK(p->read(p, 3, buf, (uint32_t)sizeof buf) < 0,
	      "null read() fails loud (negative)");
	CHECK(p->write(p, 3, buf, (uint32_t)sizeof buf) < 0,
	      "null write() fails loud (negative)");
	CHECK(p->seek(p, 3, 0, PAL_SEEK_END) < 0,
	      "null seek()/filesize fails loud (negative)");
	CHECK(p->remove(p, "X.DBF") < 0,
	      "null remove() fails loud (negative)");
	CHECK(p->rename(p, "A.DBF", "B.DBF") < 0,
	      "null rename() fails loud (negative)");

	/* cooked console: conout is a void no-op; conin_line signals EOF (<0). */
	p->conout(p, "hello", 5u);   /* must be callable + return cleanly */
	CHECK(p->conin_line(p, buf, (uint32_t)sizeof buf) < 0,
	      "null conin_line() returns EOF (negative)");

	/* terminal extension: conin_char EOF (<0); gotoxy/set_attr void no-ops. */
	CHECK(p->conin_char(p) < 0,
	      "null conin_char() returns EOF (negative)");
	p->gotoxy(p, 0u, 0u);        /* callable no-op */
	p->set_attr(p, 0x07u);       /* callable no-op */

	/* clock: null binding zeroes the packed date. */
	p->today(p, &yy, &mm, &dd);
	CHECK(yy == 0 && mm == 0 && dd == 0,
	      "null today() writes a zeroed packed date");

	/* arena: alloc has nothing to give (engine fails loud on NULL); reset no-op. */
	CHECK(p->alloc(p, 64u) == NULL,
	      "null alloc() returns NULL (arena exhausted by construction)");
	p->reset(p, NULL);           /* callable no-op */

	return TEST_SUMMARY("test_samir_pal_contract");
}
