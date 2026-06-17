/*
 * harness/diff/dbf_diff/test_interp_use.c -- host oracle for S5.1: the work-area
 * model + USE/CLOSE/SELECT + the wa_resolve glue (interp/workarea).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Mirrors test_dbf_read.c:
 * the seed test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY), a host
 * PAL via pal_host_make (pal_host.c). A non-zero exit on any failed check keeps
 * `make test-interp-use` from false-greening (Law 2: the oracle is the truth).
 *
 * WHAT S5.1 IS: the Phase-5 convergence point that ties together the .dbf codec,
 * the .dbt memo, the .ndx index, and the expression evaluator. This oracle drives
 * a scripted USE/SELECT session and proves:
 *   1. USE TRAVEL into area 1 -> RECNO==1, nrec matches the codec, memo opened.
 *   2. A field of record 1 resolves THROUGH the evaluator (xb_interp_eval_str)
 *      via the work-area resolve hook -- the value matches dbf_read_rec directly.
 *   3. USE CLIENTS INDEX CNAMES into area 2 -> index attached, master order set.
 *   4. SELECT between areas by NUMBER and by ALIAS -> the resolve hook follows
 *      the selected area (THE headline property of the work-area model).
 *   5. A memo (M) field resolves through the .dbt (TRAVEL rec1 NOTES -> block 1).
 *   6. CLOSE frees the area (wa_is_open -> 0; RECNO -> 0).
 *
 * Two tiers (plan Sec 2.A):
 *   Tier 0 (committed, operator-free): a SYNTHETIC two-table session built from
 *     temp .dbf files (no goldens needed) -- proves USE/SELECT/RECNO/resolve and,
 *     critically, that resolve FOLLOWS SELECT. This is where the -DWA_MUTATE_SELECT
 *     mutant bites even with no corpus present.
 *   Tier 1 (golden-diff): the TRAVEL (memo) + CLIENTS (index) corpus session.
 *     Absent goldens -> a LOUD skip naming the path; Tier 0 still runs.
 *
 * Goldens base resolves from argv[1] (orchestrator passes the corpus path);
 * default "../dbase3-decomp".
 *
 * Mutation hook (Rule 6 / ARB rider (a)): built with -DWA_MUTATE_SELECT, wa_select
 * stores the 1-based area number instead of (area-1), so SELECT n selects area
 * n+1 and the resolve hook reads the WRONG area. The "resolve follows SELECT"
 * checks (Tier 0 AND Tier 1) go RED. Exactly one constant changes.
 *   (An alternative mutant, -DWA_MUTATE_RESOLVE_AREA, makes wa_resolve always read
 *    area 1; this oracle also bites it via the area-2 resolve checks.)
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - os/samir/include/samir/workarea.h (the 10-area model + wa_resolve glue).
 *   - os/samir/include/samir/interp.h   (xb_interp + xb_interp_eval_str).
 *   - os/samir/include/samir/eval.h     (xb_ctx.resolve hook contract).
 *   - os/samir/include/samir/dbf.h / dbt.h / ndx.h (the codecs S5.1 integrates).
 *   - docs/plans/SAMIR-implementation-plan.md S5.1 oracle ("scripted USE/SELECT").
 *   - Corpus ground truth (byte-verified 2026-06-17):
 *       TRAVEL.DBF nrec=49; rec1 FIRSTNAME=Claire LASTNAME=Buckman
 *         PAID=T NOTES=block:1 ; TRAVEL.DBT block 1 = 127-byte memo starting
 *         "\r\nPaid on 7/15/85".
 *       CLIENTS.DBF nrec=49; rec1 FIRSTNAME=Claire LASTNAME=Buckman.
 *       CNAMES.NDX key="LASTNAME + FIRSTNAME ", key_type=0 (char).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "test_assert.h"          /* seed/, on -Iseed */
#include "samir/workarea.h"       /* os/samir/include/, on -Ios/samir/include */
#include "samir/interp.h"
#include "samir/dbf.h"
#include "samir/dbt.h"
#include "samir/ndx.h"
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"

TEST_HARNESS();

/* pal_host.c surface (not declared in a header; declare what we use). */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* ---- path helpers (same pattern as test_dbf_read.c) ---- */

static char *join(char *buf, size_t cap, const char *base, const char *rel)
{
    snprintf(buf, cap, "%s/%s", base, rel);
    return buf;
}

static int file_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

#define SP_PATH "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities"

/* =====================================================================
 * Tier 0: SYNTHETIC two-table session (no goldens needed).
 *
 * Build two minimal .dbf files in /tmp, USE them into areas 1 and 2, prove
 * USE/SELECT/RECNO/resolve, and -- critically -- that resolve FOLLOWS SELECT.
 * Table A (area 1): one field NAME C(4), one record "ALFA".
 * Table B (area 2): one field NAME C(4), one record "BETA".
 * Resolving "NAME" in area 1 -> "ALFA"; in area 2 -> "BETA".
 * ===================================================================== */

/* write a minimal 1-field-C(4), 1-record .dbf to `path` with field data `rec4`. */
static int write_min_dbf(const char *path, const char *rec4)
{
    FILE *f;
    uint8_t hdr[32], desc[32], tail[6];

    memset(hdr, 0, sizeof(hdr));
    memset(desc, 0, sizeof(desc));
    memset(tail, 0, sizeof(tail));

    hdr[0x00] = 0x03u;                       /* version: no memo */
    hdr[0x04] = 1; hdr[0x05] = 0; hdr[0x06] = 0; hdr[0x07] = 0; /* nrec=1 LE */
    hdr[0x08] = 65; hdr[0x09] = 0;           /* header_length = 32+32+1 = 65 */
    hdr[0x0A] = 5;  hdr[0x0B] = 0;           /* record_length = 1 + 4 = 5 */

    desc[0x00] = 'N'; desc[0x01] = 'A'; desc[0x02] = 'M'; desc[0x03] = 'E';
    desc[0x04] = 0;                           /* NUL-terminate "NAME" */
    desc[0x0B] = 'C';                         /* type C */
    desc[0x10] = 4;                           /* field_len = 4 */
    desc[0x11] = 0;                           /* dec_count = 0 */
    desc[0x14] = 0x01u;                       /* work-area id (+1 form) */

    tail[0] = 0x0Du;                          /* 0x0D terminator */
    tail[1] = 0x20u;                          /* delete flag: live */
    tail[2] = (uint8_t)rec4[0];
    tail[3] = (uint8_t)rec4[1];
    tail[4] = (uint8_t)rec4[2];
    tail[5] = (uint8_t)rec4[3];

    f = fopen(path, "wb");
    if (!f) return -1;
    fwrite(hdr, 1, 32, f);
    fwrite(desc, 1, 32, f);
    fwrite(tail, 1, 6, f);
    fclose(f);
    return 0;
}

/* resolve "NAME" via the interpreter and assert the first 4 bytes == want. */
static void check_name_resolves(xb_interp *ip, const char *label, const char *want)
{
    xb_val out;
    int ec = 0, rc;
    char msg[256];

    rc = xb_interp_eval_str(ip, "NAME", 4u, &out, &ec);
    snprintf(msg, sizeof(msg), "%s: eval \"NAME\" rc=%d ec=%d (expect 0)", label, rc, ec);
    CHECK(rc == INTERP_OK, msg);
    if (rc != INTERP_OK) return;

    snprintf(msg, sizeof(msg), "%s: NAME type C (got %d)", label, (int)out.t);
    CHECK(out.t == XB_C, msg);

    snprintf(msg, sizeof(msg), "%s: NAME == \"%s\" (len=%d)", label, want, (int)out.u.c.len);
    CHECK(out.t == XB_C && out.u.c.len == 4u && out.u.c.p != NULL &&
          memcmp(out.u.c.p, want, 4) == 0, msg);
}

static void test_synthetic_select(samir_pal_t *pal)
{
    const char *pa = "/tmp/test_interp_use_a.dbf";
    const char *pb = "/tmp/test_interp_use_b.dbf";
    xb_interp *ip;
    wa_env *env;
    int rc;
    char msg[256];

    CHECK(write_min_dbf(pa, "ALFA") == 0, "synthetic: write table A");
    CHECK(write_min_dbf(pb, "BETA") == 0, "synthetic: write table B");

    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "synthetic: xb_interp_make");
    if (!ip) return;
    env = xb_interp_env(ip);
    CHECK(env != NULL, "synthetic: env present");

    /* default selected area = 1 (dBASE starts in area 1) */
    snprintf(msg, sizeof(msg), "synthetic: default selected==1 (got %d)", wa_selected(env));
    CHECK(wa_selected(env) == 1, msg);

    /* --- USE A into area 1 --- */
    rc = wa_set_open(env, 1, pa, NULL, NULL);
    snprintf(msg, sizeof(msg), "synthetic: USE A area1 rc=%d", rc);
    CHECK(rc == WA_OK, msg);

    /* RECNO=1 on USE (the headline contract); nrec==1; alias derived "A" or base. */
    snprintf(msg, sizeof(msg), "synthetic: area1 RECNO==1 (got %u)", wa_recno(env, 1));
    CHECK(wa_recno(env, 1) == 1u, msg);
    snprintf(msg, sizeof(msg), "synthetic: area1 nrec==1 (got %u)", wa_nrec(env, 1));
    CHECK(wa_nrec(env, 1) == 1u, msg);
    snprintf(msg, sizeof(msg), "synthetic: area1 open (got %d)", wa_is_open(env, 1));
    CHECK(wa_is_open(env, 1) == 1, msg);

    /* --- USE B into area 2 --- */
    rc = wa_set_open(env, 2, pb, "BTAB", NULL);
    snprintf(msg, sizeof(msg), "synthetic: USE B area2 ALIAS BTAB rc=%d", rc);
    CHECK(rc == WA_OK, msg);
    snprintf(msg, sizeof(msg), "synthetic: area2 alias==BTAB (got \"%s\")", wa_alias(env, 2));
    CHECK(strcmp(wa_alias(env, 2), "BTAB") == 0, msg);

    /* --- SELECT 1 -> resolve "NAME" must be "ALFA" --- */
    rc = wa_select(env, 1);
    CHECK(rc == WA_OK, "synthetic: SELECT 1");
    snprintf(msg, sizeof(msg), "synthetic: selected==1 after SELECT 1 (got %d)", wa_selected(env));
    CHECK(wa_selected(env) == 1, msg);
    check_name_resolves(ip, "synthetic SELECT 1", "ALFA");

    /* --- SELECT 2 by NUMBER -> resolve "NAME" must follow to "BETA" ---
     * THIS is the property the -DWA_MUTATE_SELECT mutant breaks. */
    rc = wa_select(env, 2);
    CHECK(rc == WA_OK, "synthetic: SELECT 2");
    check_name_resolves(ip, "synthetic SELECT 2 (by number)", "BETA");

    /* --- SELECT back by ALIAS "A" letter (area 1) -> "ALFA" --- */
    rc = wa_select_alias(env, "A");
    snprintf(msg, sizeof(msg), "synthetic: SELECT A (letter->area1) rc=%d", rc);
    CHECK(rc == WA_OK, msg);
    check_name_resolves(ip, "synthetic SELECT A (alias letter)", "ALFA");

    /* --- SELECT by explicit ALIAS "BTAB" (area 2) -> "BETA" --- */
    rc = wa_select_alias(env, "btab");   /* case-insensitive */
    snprintf(msg, sizeof(msg), "synthetic: SELECT btab (alias)->area2 rc=%d", rc);
    CHECK(rc == WA_OK, msg);
    snprintf(msg, sizeof(msg), "synthetic: selected==2 after alias BTAB (got %d)", wa_selected(env));
    CHECK(wa_selected(env) == 2, msg);
    check_name_resolves(ip, "synthetic SELECT btab (alias name)", "BETA");

    /* --- a bad alias fails loud --- */
    rc = wa_select_alias(env, "NOPE");
    snprintf(msg, sizeof(msg), "synthetic: SELECT NOPE -> -WA_ERR_NO_ALIAS (got %d)", rc);
    CHECK(rc == -WA_ERR_NO_ALIAS, msg);

    /* --- CLOSE area 1 frees it --- */
    rc = wa_close(env, 1);
    CHECK(rc == WA_OK, "synthetic: CLOSE area 1");
    snprintf(msg, sizeof(msg), "synthetic: area1 closed (open=%d)", wa_is_open(env, 1));
    CHECK(wa_is_open(env, 1) == 0, msg);
    snprintf(msg, sizeof(msg), "synthetic: area1 RECNO==0 after close (got %u)", wa_recno(env, 1));
    CHECK(wa_recno(env, 1) == 0u, msg);

    /* --- USE into the closed area 1 again succeeds; into the OCCUPIED area 2 fails --- */
    rc = wa_set_open(env, 1, pa, NULL, NULL);
    snprintf(msg, sizeof(msg), "synthetic: re-USE A into freed area1 rc=%d", rc);
    CHECK(rc == WA_OK, msg);
    rc = wa_set_open(env, 2, pa, NULL, NULL);
    snprintf(msg, sizeof(msg), "synthetic: USE into occupied area2 -> -WA_ERR_OCCUPIED (got %d)", rc);
    CHECK(rc == -WA_ERR_OCCUPIED, msg);

    /* --- area-range fail-loud --- */
    rc = wa_set_open(env, 11, pa, NULL, NULL);
    snprintf(msg, sizeof(msg), "synthetic: USE area 11 -> -WA_ERR_BAD_AREA (got %d)", rc);
    CHECK(rc == -WA_ERR_BAD_AREA, msg);

    xb_interp_free(ip);
    remove(pa);
    remove(pb);
}

/* =====================================================================
 * Tier 1: corpus session -- TRAVEL (memo) area 1, CLIENTS + CNAMES area 2.
 * ===================================================================== */

static void test_corpus_session(samir_pal_t *pal, const char *base)
{
    char trav[1024], clients[1024], cnames[1024];
    xb_interp *ip;
    wa_env *env;
    wa_index_list il;
    int rc, ec = 0;
    char msg[256];
    dbf_table *t;
    xb_val ref[16];
    int deleted = 0;
    xb_val out;

    join(trav, sizeof(trav), base, SP_PATH "/TRAVEL.DBF");
    join(clients, sizeof(clients), base, SP_PATH "/CLIENTS.DBF");
    join(cnames, sizeof(cnames), base, SP_PATH "/CNAMES.NDX");

    if (!file_exists(trav) || !file_exists(clients) || !file_exists(cnames)) {
        fprintf(stderr,
                "  SKIP (LOUD): corpus golden(s) absent under base '%s'\n"
                "               need: %s , %s , %s\n"
                "               (pass the corpus base as argv[1]; Tier-0 synthetic still ran)\n",
                base, trav, clients, cnames);
        return;
    }

    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "corpus: xb_interp_make");
    if (!ip) return;
    env = xb_interp_env(ip);

    /* --- USE TRAVEL into area 1 (memo table) --- */
    rc = wa_set_open(env, 1, trav, NULL, NULL);
    snprintf(msg, sizeof(msg), "corpus: USE TRAVEL area1 rc=%d", rc);
    CHECK(rc == WA_OK, msg);
    if (rc != WA_OK) { xb_interp_free(ip); return; }

    /* RECNO==1, nrec matches the codec, memo handle opened. */
    snprintf(msg, sizeof(msg), "corpus: TRAVEL RECNO==1 (got %u)", wa_recno(env, 1));
    CHECK(wa_recno(env, 1) == 1u, msg);

    t = wa_table(env, 1);
    CHECK(t != NULL, "corpus: TRAVEL table handle non-NULL");
    snprintf(msg, sizeof(msg), "corpus: TRAVEL nrec matches codec (wa=%u dbf=%u)",
             wa_nrec(env, 1), t ? dbf_nrec(t) : 0u);
    CHECK(t != NULL && wa_nrec(env, 1) == dbf_nrec(t), msg);

    snprintf(msg, sizeof(msg), "corpus: TRAVEL memo (.dbt) opened (got %p)", (void *)wa_memo(env, 1));
    CHECK(wa_memo(env, 1) != NULL, msg);   /* TRAVEL has a memo field + TRAVEL.DBT */

    /* alias derived from the file base name = "TRAVEL". */
    snprintf(msg, sizeof(msg), "corpus: TRAVEL alias==\"TRAVEL\" (got \"%s\")", wa_alias(env, 1));
    CHECK(strcmp(wa_alias(env, 1), "TRAVEL") == 0, msg);

    /* --- resolve a field of record 1 via the evaluator vs dbf_read_rec --- */
    /* TRAVEL field[1]=LASTNAME C(20). dbf_read_rec gives "Buckman"+spaces. */
    if (t) {
        rc = dbf_read_rec(t, 1u, ref, &deleted);
        CHECK(rc == DBF_OK, "corpus: dbf_read_rec(TRAVEL,1) for reference");
    }
    wa_select(env, 1);
    rc = xb_interp_eval_str(ip, "LASTNAME", 8u, &out, &ec);
    snprintf(msg, sizeof(msg), "corpus: eval LASTNAME rc=%d ec=%d", rc, ec);
    CHECK(rc == INTERP_OK, msg);
    if (rc == INTERP_OK) {
        snprintf(msg, sizeof(msg), "corpus: LASTNAME resolves type C + matches dbf_read_rec");
        CHECK(out.t == XB_C && out.u.c.len == ref[1].u.c.len &&
              memcmp(out.u.c.p, ref[1].u.c.p, out.u.c.len) == 0, msg);
        /* spot-check the trimmed value == "Buckman" */
        snprintf(msg, sizeof(msg), "corpus: LASTNAME starts \"Buckman\"");
        CHECK(out.t == XB_C && out.u.c.len >= 7u && memcmp(out.u.c.p, "Buckman", 7) == 0, msg);
    }

    /* a small expression that references a field: "LASTNAME + FIRSTNAME". */
    rc = xb_interp_eval_str(ip, "LASTNAME + FIRSTNAME", 20u, &out, &ec);
    snprintf(msg, sizeof(msg), "corpus: eval LASTNAME+FIRSTNAME rc=%d ec=%d", rc, ec);
    CHECK(rc == INTERP_OK, msg);
    if (rc == INTERP_OK) {
        /* 20+20 = 40 bytes; first 7 = "Buckman"; bytes 20.. = "Claire". */
        snprintf(msg, sizeof(msg), "corpus: concat len==40 (got %d)", (int)out.u.c.len);
        CHECK(out.t == XB_C && out.u.c.len == 40u, msg);
        if (out.t == XB_C && out.u.c.len == 40u) {
            CHECK(memcmp(out.u.c.p, "Buckman", 7) == 0, "corpus: concat[0..6]=\"Buckman\"");
            CHECK(memcmp(out.u.c.p + 20, "Claire", 6) == 0, "corpus: concat[20..25]=\"Claire\"");
        }
    }

    /* --- memo (M) field resolves THROUGH the .dbt: TRAVEL rec1 NOTES -> block 1 --- */
    rc = xb_interp_eval_str(ip, "NOTES", 5u, &out, &ec);
    snprintf(msg, sizeof(msg), "corpus: eval NOTES (memo) rc=%d ec=%d", rc, ec);
    CHECK(rc == INTERP_OK, msg);
    if (rc == INTERP_OK) {
        snprintf(msg, sizeof(msg), "corpus: NOTES type M (got %d)", (int)out.t);
        CHECK(out.t == XB_M, msg);
        /* block-1 memo is 127 bytes starting "\r\nPaid on 7/15/85". */
        snprintf(msg, sizeof(msg), "corpus: NOTES memo len==127 (got %d)", (int)out.u.c.len);
        CHECK(out.t == XB_M && out.u.c.len == 127u, msg);
        if (out.t == XB_M && out.u.c.len >= 17u) {
            CHECK(memcmp(out.u.c.p, "\r\nPaid on 7/15/85", 17) == 0,
                  "corpus: NOTES memo starts \"\\r\\nPaid on 7/15/85\"");
        }
    }

    /* --- USE CLIENTS INDEX CNAMES into area 2 -> index attached, master set --- */
    il.names[0] = cnames;
    il.count = 1;
    rc = wa_set_open(env, 2, clients, NULL, &il);
    snprintf(msg, sizeof(msg), "corpus: USE CLIENTS INDEX CNAMES area2 rc=%d", rc);
    CHECK(rc == WA_OK, msg);
    if (rc == WA_OK) {
        snprintf(msg, sizeof(msg), "corpus: area2 index_count==1 (got %d)", wa_index_count(env, 2));
        CHECK(wa_index_count(env, 2) == 1, msg);
        snprintf(msg, sizeof(msg), "corpus: area2 master order==1 (got %d)", wa_master_order(env, 2));
        CHECK(wa_master_order(env, 2) == 1, msg);
        snprintf(msg, sizeof(msg), "corpus: area2 index[0] handle non-NULL");
        CHECK(wa_index(env, 2, 0) != NULL, msg);
        /* the attached index's key expression is verbatim. */
        if (wa_index(env, 2, 0)) {
            const char *kx = ndx_key_expr(wa_index(env, 2, 0));
            snprintf(msg, sizeof(msg), "corpus: CNAMES key_expr==\"LASTNAME + FIRSTNAME \" (got \"%s\")", kx);
            CHECK(kx && strcmp(kx, "LASTNAME + FIRSTNAME ") == 0, msg);
        }
        /* SET ORDER to 0 (natural) then back to 1 -- master order tracking. */
        rc = wa_set_order(env, 2, 0);
        CHECK(rc == WA_OK && wa_master_order(env, 2) == 0, "corpus: SET ORDER 0 (natural)");
        rc = wa_set_order(env, 2, 1);
        CHECK(rc == WA_OK && wa_master_order(env, 2) == 1, "corpus: SET ORDER 1 (master)");
        /* out-of-range order fails loud. */
        rc = wa_set_order(env, 2, 2);
        snprintf(msg, sizeof(msg), "corpus: SET ORDER 2 (>nidx) -> -WA_ERR_BAD_ORDER (got %d)", rc);
        CHECK(rc == -WA_ERR_BAD_ORDER, msg);
    }

    /* --- SELECT between the two areas: resolve must follow the SELECTED area --- */
    /* Area 1 = TRAVEL (field LASTNAME), Area 2 = CLIENTS (also has LASTNAME).
     * Both rec1 LASTNAME == "Buckman" -- to make the SELECT bite distinguishable,
     * use a CLIENTS-only field (CITY) vs a TRAVEL-only field (TRAVELCODE). */
    wa_select(env, 1);
    rc = xb_interp_eval_str(ip, "TRAVELCODE", 10u, &out, &ec);   /* TRAVEL-only field */
    snprintf(msg, sizeof(msg), "corpus: area1 selected -> TRAVELCODE resolves rc=%d ec=%d", rc, ec);
    CHECK(rc == INTERP_OK && out.t == XB_C, msg);
    /* CITY is NOT a TRAVEL field -> unbound while area 1 selected. */
    rc = xb_interp_eval_str(ip, "CITY", 4u, &out, &ec);
    snprintf(msg, sizeof(msg), "corpus: area1 selected -> CITY unbound (rc=%d ec=%d)", rc, ec);
    CHECK(rc == -INTERP_ERR_EVAL && ec == XBEE_UNBOUND, msg);

    wa_select(env, 2);
    rc = xb_interp_eval_str(ip, "CITY", 4u, &out, &ec);          /* CLIENTS-only field */
    snprintf(msg, sizeof(msg), "corpus: area2 selected -> CITY resolves rc=%d ec=%d", rc, ec);
    CHECK(rc == INTERP_OK && out.t == XB_C, msg);
    if (rc == INTERP_OK && out.t == XB_C) {
        /* CLIENTS rec1 CITY = "Oxnard". */
        CHECK(out.u.c.len >= 6u && memcmp(out.u.c.p, "Oxnard", 6) == 0,
              "corpus: area2 CITY starts \"Oxnard\"");
    }
    /* TRAVELCODE is NOT a CLIENTS field -> unbound while area 2 selected. */
    rc = xb_interp_eval_str(ip, "TRAVELCODE", 10u, &out, &ec);
    snprintf(msg, sizeof(msg), "corpus: area2 selected -> TRAVELCODE unbound (rc=%d ec=%d)", rc, ec);
    CHECK(rc == -INTERP_ERR_EVAL && ec == XBEE_UNBOUND, msg);

    /* --- SELECT by ALIAS "TRAVEL" -> back to area 1; resolve follows --- */
    rc = wa_select_alias(env, "TRAVEL");
    snprintf(msg, sizeof(msg), "corpus: SELECT TRAVEL (alias) rc=%d selected=%d", rc, wa_selected(env));
    CHECK(rc == WA_OK && wa_selected(env) == 1, msg);
    rc = xb_interp_eval_str(ip, "TRAVELCODE", 10u, &out, &ec);
    CHECK(rc == INTERP_OK && out.t == XB_C, "corpus: after alias TRAVEL, TRAVELCODE resolves");

    /* --- CLOSE area 2 frees it (handles + RECNO) --- */
    rc = wa_close(env, 2);
    CHECK(rc == WA_OK, "corpus: CLOSE area 2");
    snprintf(msg, sizeof(msg), "corpus: area2 closed (open=%d table=%p)",
             wa_is_open(env, 2), (void *)wa_table(env, 2));
    CHECK(wa_is_open(env, 2) == 0 && wa_table(env, 2) == NULL, msg);
    snprintf(msg, sizeof(msg), "corpus: area2 RECNO==0 + index_count==0 after close (%u/%d)",
             wa_recno(env, 2), wa_index_count(env, 2));
    CHECK(wa_recno(env, 2) == 0u && wa_index_count(env, 2) == 0, msg);

    xb_interp_free(ip);
}

/* =====================================================================
 * main
 * ===================================================================== */

int main(int argc, char **argv)
{
    const char *base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    struct pal_host_cfg cfg;
    samir_pal_t *pal;
    char path[1024];

    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy   = 99;     /* injected fixed date (Rule 11) */
    cfg.date_mm   = 12;
    cfg.date_dd   = 31;
    cfg.heap_size = 1024u * 1024u;   /* 1 MiB: two corpus tables + caches + interp */

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }

    /* Tier 0: synthetic two-table SELECT/resolve session (always runs). */
    test_synthetic_select(pal);

    /* Tier 1: corpus TRAVEL (memo) + CLIENTS (index) session. */
    join(path, sizeof(path), base, SP_PATH "/TRAVEL.DBF");
    if (!file_exists(path)) {
        fprintf(stderr,
                "  SKIP (LOUD): no corpus goldens under base '%s'\n"
                "               expected e.g. %s/%s/TRAVEL.DBF\n"
                "               (pass the corpus base as argv[1]; Tier-0 synthetic ran;\n"
                "                the -DWA_MUTATE_SELECT mutant still bites Tier 0)\n",
                base, base, SP_PATH);
    }
    test_corpus_session(pal, base);

    pal_host_free(pal);
    return TEST_SUMMARY("test-interp-use");
}
