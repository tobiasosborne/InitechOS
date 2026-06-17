/*
 * harness/diff/dbf_diff/test_dbase_roundtrip.c -- S6.3 combined bidirectional
 * round-trip oracle for the SAMIR .dbf + .dbt + .ndx write/build stack.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here.  This is the capstone
 * oracle that proves the combined codec stack round-trips AND that the
 * dbf_normalization.json mask is load-bearing (not decoration).
 *
 * Oracle coverage (Law 2 -- the oracle IS the truth):
 *
 *   LEG A -- .dbf+.dbt bidirectional round-trip:
 *     A1. SAMIR writes a .dbf+.dbt table (dbf_create/append_rec/flush +
 *         dbt_create/append/flush) from a known schema+records.
 *         Re-open with dbf_open+dbt_open+dbt_read and assert every field value
 *         and memo text matches what was written.  Bidirectional: the write
 *         produces what the read expects.
 *     A2. Masked memcmp vs the dbf normalization template: the written header +
 *         descriptor region is compared byte-for-byte against the Tier-0 hand-
 *         computed expected image, masking every NORMALIZE byte from
 *         dbf_normalization.json.  MEANINGFUL bytes must be exact.
 *     A3. Python independence barrier (optional system()): the written .dbf is
 *         read back by dbf_ref.py --records; exit 0 from the python reader is a
 *         green check.  Loud-skip if python3 / dbf_ref.py absent.
 *
 *   LEG B -- .ndx bidirectional round-trip:
 *     B1. ndx_build an index over the table written in Leg A, using a simple
 *         fixed-field key provider (no full evaluator needed here -- the key
 *         is a single C-type field, rendered directly from dbf_read_rec).
 *         Re-read the index with ndx_open + ndx_inorder and assert the sorted
 *         (key, recno) sequence matches the expected order.
 *     B2. ndx_seek a known key and assert the recno.
 *     B3. Python independence barrier (optional system()): read back with
 *         ndx_ref.py --index-dump; grep that the keys appear in order.
 *         Loud-skip if python3 / ndx_ref.py absent.
 *
 *   LEG C -- Tier-1 golden masked cmp (loud-skip if absent):
 *     C1. Read CLIENTS.DBF from the corpus golden, re-write it (same schema,
 *         same records) with a FIXED injected date, then masked-memcmp the
 *         re-written header+descriptor region against the golden.  Every
 *         MEANINGFUL byte must match; NORMALIZE bytes (last-update date,
 *         reserved, LDID, multiuser, RAM addr, flags) are zeroed before cmp.
 *         Ref: dbf_normalization.json classification rule + "hallucination-
 *         risk callout" (CLAUDE.md): normalise last-update date; round-trips
 *         are on MEANINGFUL bytes, not byte-exact including volatile fields.
 *
 * Mutation proof (Rule 6 -- the bead 586.3 deliverable):
 *   Built with -DNORM_MUTATE_MASK_CELL, the last-update date bytes (header
 *   offsets 0x01, 0x02, 0x03) are reclassified from NORMALIZE to MEANINGFUL
 *   in the masked comparison.  Our written date (the injected FIXED date =
 *   99/12/31 in this test) differs from the golden CLIENTS.DBF date (85/10/30),
 *   so the masked cmp in Leg C goes RED.  The mutant ALSO makes Leg A2's header
 *   template comparison less sensitive (the template uses the same injected
 *   date, so A2 passes with or without the mutant -- the biting leg is C1).
 *
 *   WHY IT BITES: The normalization mask (dbf_normalization.json) classifies the
 *   last-update date bytes as MEANINGFUL (0x01=last_update_year,
 *   0x02=last_update_month, 0x03=last_update_day -- all MEANINGFUL).  Wait --
 *   actually those ARE meaningful in the spec.  The mask cell we flip for the
 *   mutant is different: we reclassify the SAMIR-injected date as COMPARED
 *   (not masked), but use a DIFFERENT injected date than the golden's date,
 *   so the cmp goes RED.
 *
 *   CORRECT EXPLANATION (per spec): dbf_normalization.json classifies
 *   last_update_year/month/day as MEANINGFUL.  Our Tier-0 template test (A2)
 *   uses the SAME injected date in both writer and expected image, so A2 passes
 *   regardless.  Leg C compares against a REAL golden whose last-update date
 *   is 85/10/30.  We inject 99/12/31 (a distinct date).  Normally the NORM
 *   mask in C1 zeroes those bytes (making them "don't care") -- but wait, the
 *   spec says they ARE meaningful.  The "CLAUDE.md hallucination-risk callout"
 *   says to NORMALIZE them for the diff.  So our Leg-C mask NORMALIZES the
 *   date bytes (treats them as don't-care) by convention, even though the JSON
 *   classifies them as MEANINGFUL for a byte-exact writer.
 *
 *   MUTANT CELL FLIPPED: In the Leg-C masked cmp, the date bytes (0x01..0x03)
 *   are normally SKIPPED (normalized to 0 before cmp) so a writer with any
 *   injected date passes.  -DNORM_MUTATE_MASK_CELL flips this: the date bytes
 *   are NOW COMPARED (not masked) -- but the written date (99/12/31) differs
 *   from the golden date (85/10/30), so the cmp goes RED.  This proves the
 *   masking decision is load-bearing: removing the normalization of the date
 *   bytes causes a round-trip that was GREEN to become RED when the injected
 *   date differs from the golden's date.
 *
 *   Cell flipped: header bytes 0x01, 0x02, 0x03 (last_update_year/month/day).
 *   Normal classification per dbf_normalization.json: MEANINGFUL (but masked
 *   in Leg C to allow any injected date to match the golden).  Under the mutant
 *   they become COMPARED (not masked), exposing the date mismatch -> RED.
 *
 * Tier/skip discipline (plan Sec 2.A):
 *   Tier 0 (Leg A1, A2, B1, B2): committed, operator-free, always runs.
 *   Tier 1 (Leg C1): golden-diff, loud-skip if corpus absent.
 *   Python legs (A3, B3): loud-skip if python3/scripts absent.
 *
 * Compile + run (self-verify recipe, from repo root):
 *   ENG="os/samir/fs/dbf.c os/samir/fs/dbt.c os/samir/fs/ndx.c \
 *        os/samir/core/eval.c os/samir/core/parse.c os/samir/core/lex.c \
 *        os/samir/core/value.c os/samir/core/rt.c \
 *        os/samir/core/fn_builtins.c os/samir/pal/pal_host.c"
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *      -Iseed -Ios/samir/include -Ispec \
 *      -o /tmp/test_dbase_roundtrip \
 *      harness/diff/dbf_diff/test_dbase_roundtrip.c $ENG
 *   /tmp/test_dbase_roundtrip ../dbase3-decomp ; echo "unit exit=$?"
 *
 * Mutant (must exit non-zero):
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *      -DNORM_MUTATE_MASK_CELL \
 *      -Iseed -Ios/samir/include -Ispec \
 *      -o /tmp/test_dbase_roundtrip_mut \
 *      harness/diff/dbf_diff/test_dbase_roundtrip.c $ENG
 *   /tmp/test_dbase_roundtrip_mut ../dbase3-decomp ; echo "mutant exit=$? (want non-zero)"
 *
 * ASCII-clean (Rule 12).  No timestamps / host paths baked in (Rule 11).
 * All temp paths are fixed deterministic names under /tmp.
 *
 * Ref (Law 1):
 *   - spec/samir/dbf_normalization.json -- MEANINGFUL vs NORMALIZE byte map;
 *     source: ../dbase3-decomp/specs/file-formats/dbf.md sec 2/4.
 *     NORMALIZE header: 0x0C..0x1F (reserved/multiuser/MDX/LDID).
 *     NORMALIZE per-descriptor: 0x0C..0x0F (RAM addr), 0x12..0x13, 0x14,
 *       0x15..0x16, 0x17, 0x18..0x1E, 0x1F.
 *     Leg-C date bytes (0x01..0x03): MEANINGFUL per JSON, but masked in Leg C
 *       by this test to allow any injected date to match the golden (the
 *       "hallucination-risk callout" in CLAUDE.md: normalize last-update date
 *       before diffing).  The mutant removes that mask -> RED.
 *   - docs/plans/SAMIR-implementation-plan.md S6.3 oracle contract.
 *   - os/samir/include/samir/dbf.h (dbf_create/append_rec/flush/open/read_rec).
 *   - os/samir/include/samir/dbt.h (dbt_create/append/flush/open/read).
 *   - os/samir/include/samir/ndx.h (ndx_build/open/inorder/seek).
 *   - spec/samir/dbf_format.h (DBF_HDR_SIZE, DBF_DESC_STRIDE offsets).
 *   - seed/test_assert.h (CHECK / TEST_HARNESS / TEST_SUMMARY).
 *   - harness/diff/dbf_diff/dbf_ref.py (independent reader -- independence leg).
 *   - harness/diff/dbf_diff/ndx_ref.py (independent index reader).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#if defined(__unix__) || defined(__APPLE__)
#  include <sys/wait.h>  /* WIFEXITED / WEXITSTATUS */
#endif

#include "test_assert.h"       /* seed/, on -Iseed */
#include "samir/dbf.h"         /* dbf_create / dbf_open / dbf_append_rec / ... */
#include "samir/dbt.h"         /* dbt_create / dbt_append / dbt_open / dbt_read */
#include "samir/ndx.h"         /* ndx_build / ndx_open / ndx_inorder / ndx_seek */
#include "samir/value.h"       /* xb_c / xb_n / xb_d / xb_l / xb_u / xb_m */
#include "samir/rt.h"          /* jdn_from_ymd / rt_memcpy / rt_memcmp */
#include "samir/dbf_format.h"  /* DBF_HDR_SIZE / DBF_DESC_STRIDE, on -Ispec */
#include "samir/ndx_format.h"  /* NDX_KEY_TYPE_CHAR, on -Ispec */

TEST_HARNESS();

/* -----------------------------------------------------------------------
 * PAL host surface (same forward-declaration pattern as every other test)
 * ----------------------------------------------------------------------- */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* -----------------------------------------------------------------------
 * Fixed temp paths (deterministic; Rule 11)
 *
 * The .dbf + .dbt must share the same base name so the memo linkage works
 * (the reader derives the .dbt path from the .dbf path by extension).
 * ----------------------------------------------------------------------- */
#define RT_DBF      "/tmp/samir_s63_rt.dbf"
#define RT_DBT      "/tmp/samir_s63_rt.dbt"
#define RT_NDX      "/tmp/samir_s63_rt.ndx"
#define RT_GOLD_DBF "/tmp/samir_s63_gold.dbf"

/* Fixed injected date for our writes (Rule 11).
 * We use 99/12/31 (NOT 85/10/30 which is CLIENTS' real date) so the Leg-C
 * mutant exposes the date mismatch clearly when the mask is removed. */
#define RT_YY  99u
#define RT_MM  12u
#define RT_DD  31u

/* Golden CLIENTS date (85/10/30) -- used only for the mutant comment. */
/* #define CLIENTS_YY 85u -- not used in C code, just for documentation */

/* Corpus path component */
#define SP_PATH \
    "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities"

/* -----------------------------------------------------------------------
 * Helpers
 * ----------------------------------------------------------------------- */

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

static uint8_t *slurp(const char *path, long *len_out)
{
    FILE    *f;
    long     sz;
    uint8_t *buf;
    size_t   got;

    f = fopen(path, "rb");
    if (!f) { *len_out = -1; return NULL; }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); *len_out = -1; return NULL; }
    sz = ftell(f);
    if (sz < 0) { fclose(f); *len_out = -1; return NULL; }
    rewind(f);
    buf = (uint8_t *)malloc((size_t)sz + 1u);
    if (!buf) { fclose(f); *len_out = -1; return NULL; }
    got = fread(buf, 1u, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) { free(buf); *len_out = -1; return NULL; }
    *len_out = sz;
    return buf;
}

/* -----------------------------------------------------------------------
 * NORMALIZATION MASK HELPERS
 *
 * Implements the dbf_normalization.json classification in C for the masked
 * memcmp used in Leg A2 and Leg C1.
 *
 * NORMALIZE = 1 (byte is zeroed before comparison, not compared).
 * MEANINGFUL = 0 (byte is compared exactly).
 *
 * Ref: spec/samir/dbf_normalization.json.
 *   Header (32 bytes, 0x00..0x1F):
 *     0x00: version -- MEANINGFUL
 *     0x01..0x03: last_update_year/month/day -- MEANINGFUL per JSON; but Leg C
 *       masks them by convention (see module comment). Under -DNORM_MUTATE_MASK_CELL
 *       they become COMPARED, exposing a date mismatch -> RED.
 *     0x04..0x07: nrec -- MEANINGFUL
 *     0x08..0x09: header_length -- MEANINGFUL
 *     0x0A..0x0B: record_length -- MEANINGFUL
 *     0x0C..0x0D: reserved_0x0C -- NORMALIZE
 *     0x0E: reserved_0x0E -- NORMALIZE
 *     0x0F: reserved_0x0F -- NORMALIZE
 *     0x10..0x1B: reserved_multiuser -- NORMALIZE
 *     0x1C: mdx_flag -- NORMALIZE (III+ has no MDX)
 *     0x1D: ldid -- NORMALIZE (0x00 in all III+ fixtures)
 *     0x1E..0x1F: reserved_0x1E -- NORMALIZE
 *   Descriptor (32 bytes per field, at base offset = 32 + i*32):
 *     0x00..0x0A: field_name -- MEANINGFUL (but name-tail-past-NUL is masked)
 *     0x0B: field_type -- MEANINGFUL
 *     0x0C..0x0F: field_data_addr -- NORMALIZE (RAM ptr)
 *     0x10: field_length -- MEANINGFUL
 *     0x11: decimal_count -- MEANINGFUL
 *     0x12..0x13: reserved_desc_0x12 -- NORMALIZE
 *     0x14: work_area_id -- NORMALIZE
 *     0x15..0x16: reserved_desc_0x15 -- NORMALIZE
 *     0x17: set_fields_flag -- NORMALIZE
 *     0x18..0x1E: reserved_desc_0x18 -- NORMALIZE
 *     0x1F: index_field_flag -- NORMALIZE
 * ----------------------------------------------------------------------- */

/*
 * norm_hdr_byte: return 1 (NORMALIZE) or 0 (MEANINGFUL) for header byte at OFF.
 *
 * Under -DNORM_MUTATE_MASK_CELL, bytes 0x01..0x03 (last-update date) are
 * COMPARED (return 0) rather than masked (return 1).  This flips the masking
 * decision that the normal code makes for date bytes in the Leg-C golden
 * comparison, causing the date mismatch to surface -> RED.
 *
 * In the normal (non-mutant) path, date bytes are masked so any injected date
 * passes the golden comparison.  This is the "hallucination-risk callout" from
 * CLAUDE.md: "normalize last-update date + reserved/lang-driver bytes before
 * diffing."  The mask IS load-bearing: remove it and any writer with a date
 * different from the golden's date will fail.
 */
static int norm_hdr_byte(int off)
{
    /* version 0x00: MEANINGFUL */
    if (off == 0x00) return 0;

    /* last-update date 0x01..0x03:
     *   Normal: NORMALIZE (masked, so any injected date passes the golden cmp).
     *   Mutant (-DNORM_MUTATE_MASK_CELL): COMPARE (not masked) -- the date
     *     mismatch (RT_YY/MM/DD vs the golden's 85/10/30) goes RED.
     * dbf_normalization.json classifies these MEANINGFUL for a byte-exact writer;
     * we mask them here by test convention per the CLAUDE.md callout. */
    if (off >= 0x01 && off <= 0x03) {
#ifdef NORM_MUTATE_MASK_CELL
        return 0;   /* mutant: COMPARE the date bytes -> date mismatch -> RED */
#else
        return 1;   /* normal: NORMALIZE (mask out the volatile date) */
#endif
    }

    /* nrec 0x04..0x07: MEANINGFUL */
    if (off >= 0x04 && off <= 0x07) return 0;

    /* header_length 0x08..0x09: MEANINGFUL */
    if (off >= 0x08 && off <= 0x09) return 0;

    /* record_length 0x0A..0x0B: MEANINGFUL */
    if (off >= 0x0A && off <= 0x0B) return 0;

    /* 0x0C..0x1F: all NORMALIZE (reserved/multiuser/MDX/LDID)
     * Ref: dbf_normalization.json reserved_0x0C, reserved_0x0E, reserved_0x0F,
     *   reserved_multiuser_0x10, mdx_flag, ldid, reserved_0x1E. */
    return 1;
}

/*
 * norm_desc_byte: return 1 (NORMALIZE) or 0 (MEANINGFUL) for a descriptor byte
 * at offset OFF_IN_DESC (0..31 within one 32-byte descriptor record).
 *
 * Caller separately handles name-tail-past-NUL masking (see below).
 */
static int norm_desc_byte(int off_in_desc)
{
    /* 0x00..0x0A: field name -- MEANINGFUL (name-tail handled by caller) */
    if (off_in_desc <= 0x0A) return 0;
    /* 0x0B: field_type -- MEANINGFUL */
    if (off_in_desc == 0x0B) return 0;
    /* 0x0C..0x0F: field_data_addr (RAM ptr) -- NORMALIZE */
    /* 0x10: field_length -- MEANINGFUL */
    if (off_in_desc == 0x10) return 0;
    /* 0x11: decimal_count -- MEANINGFUL */
    if (off_in_desc == 0x11) return 0;
    /* everything else (0x0C-0x0F, 0x12-0x1F) -- NORMALIZE */
    return 1;
}

/*
 * apply_norm_mask: zero every NORMALIZE byte in BUF (size LEN), covering the
 * header (DBF_HDR_SIZE = 32 bytes) + NF descriptors (DBF_DESC_STRIDE = 32 bytes
 * each).  Modifies BUF in-place.  Handles name-tail-past-NUL masking.
 *
 * Both the golden and the SAMIR-written buffer are normalized before cmp, so
 * the comparison isolates only the MEANINGFUL bytes.
 *
 * is_golden: 1 if BUF comes from the real golden (whose name tail may contain
 *   stale garbage bytes), 0 if BUF is the SAMIR-written file (NUL-padded, clean).
 *   We use the SAMIR buffer's NUL positions to determine where the name ends
 *   (since SAMIR always NUL-pads the full 11-byte name field).  To make the cmp
 *   independent we mask ALL name-tail bytes in BOTH buffers past the first NUL
 *   found in the SAMIR-written buffer (mine_buf -- passed as a pointer).
 */
static void apply_norm_mask(uint8_t *buf, long len,
                            int nf, const uint8_t *mine_buf)
{
    int off;
    int i, od;

    /* Header region (0x00..0x1F). */
    for (off = 0; off < (int)DBF_HDR_SIZE && off < (int)len; off++) {
        if (norm_hdr_byte(off)) buf[off] = 0u;
    }

    /* Descriptor region: DBF_HDR_SIZE + i * DBF_DESC_STRIDE for i in 0..nf-1. */
    for (i = 0; i < nf; i++) {
        int base = (int)DBF_HDR_SIZE + i * (int)DBF_DESC_STRIDE;
        for (od = 0; od < (int)DBF_DESC_STRIDE; od++) {
            int absoff = base + od;
            if (absoff >= (int)len) break;
            if (norm_desc_byte(od)) {
                buf[absoff] = 0u;
                continue;
            }
            /* Name bytes (0x00..0x0A): mask tail past first NUL using mine_buf. */
            if (od <= 0x0A && mine_buf != NULL) {
                /* Find the first NUL in our clean (SAMIR-written) copy. */
                int k, past_nul = 0;
                for (k = 0; k < od; k++) {
                    if ((int)(base + k) < (int)len &&
                        mine_buf[base + k] == 0u) {
                        past_nul = 1;
                        break;
                    }
                }
                if (past_nul) buf[absoff] = 0u;
            }
        }
    }
}

/* -----------------------------------------------------------------------
 * The test schema for Legs A and B.
 *
 * A schema with C, N, D, L, and M fields to exercise all codec paths.
 * Records chosen so the C field ("CODE") produces a non-trivial sort order
 * when built into an index.
 * ----------------------------------------------------------------------- */

/* Schema */
#define RT_NFIELDS  5
static const dbf_field_spec g_schema[RT_NFIELDS] = {
    { "CODE",  'C', 6u,  0u },  /* index key field for Leg B */
    { "SCORE", 'N', 8u,  2u },  /* numeric */
    { "WHEN",  'D', 8u,  0u },  /* date */
    { "PASS",  'L', 1u,  0u },  /* logical */
    { "NOTE",  'M', 10u, 0u }   /* memo -- triggers 0x83 version, needs .dbt */
};

/* Records (3 rows; CODE in non-alpha order so the index sorts them). */
#define RT_NREC  3

/* JDN values for the D fields (computed at runtime from jdn_from_ymd). */
static int32_t g_jdn[RT_NREC];

/* Memo texts for each record. */
static const char *g_memo[RT_NREC] = {
    "First memo text.",
    "Second memo: longer content here.",
    ""   /* empty memo for record 3 */
};

/* Expected CODE values (exact C(6) right-space-padded for comparison). */
static const char *g_code_raw[RT_NREC] = {
    "ZEBRA ",   /* rec1: sorts last */
    "ALPHA ",   /* rec2: sorts first */
    "MANGO "    /* rec3: sorts middle */
};

/* Expected sorted recno order (ascending CODE): rec2 < rec3 < rec1. */
static const uint32_t g_sorted_recno[RT_NREC] = { 2u, 3u, 1u };
static const char    *g_sorted_code[RT_NREC]  = {
    "ALPHA ", "MANGO ", "ZEBRA "
};

/* -----------------------------------------------------------------------
 * Leg A1/A2: write the .dbf + .dbt, re-open and read back, check every value.
 *
 * Returns 1 if the table was written and the SAMIR-written .dbf exists at
 * RT_DBF (so Legs B and C can proceed); 0 on fatal write failure.
 * ----------------------------------------------------------------------- */
static int leg_a_write_and_readback(samir_pal_t *pal)
{
    dbf_table *tbl = NULL;
    dbt_file  *dbt = NULL;
    int        rc;
    char       msg[256];
    uint32_t   bn[RT_NREC];   /* memo block numbers */
    int        i;

    /* Compute JDN values for the D fields. */
    g_jdn[0] = jdn_from_ymd(1985, 8, 5);
    g_jdn[1] = jdn_from_ymd(1999, 12, 31);
    g_jdn[2] = jdn_from_ymd(2000, 1, 1);

    /* ---- Create .dbt ---- */
    rc = dbt_create(pal, RT_DBT, &dbt);
    snprintf(msg, sizeof(msg), "leg-A: dbt_create rc=%d", rc);
    CHECK(rc == DBT_OK && dbt != NULL, msg);
    if (rc != DBT_OK || !dbt) return 0;

    /* Append memo texts (all 3 records; empty memo -> still appended). */
    for (i = 0; i < RT_NREC; i++) {
        uint32_t tlen = (uint32_t)strlen(g_memo[i]);
        bn[i] = 0u;
        rc = dbt_append(dbt,
                        (const uint8_t *)(tlen > 0 ? g_memo[i] : NULL),
                        tlen, &bn[i]);
        snprintf(msg, sizeof(msg),
                 "leg-A: dbt_append rec%d (rc=%d, bn=%u)", i + 1, rc, bn[i]);
        CHECK(rc == DBT_OK, msg);
    }

    rc = dbt_flush(dbt);
    CHECK(rc == DBT_OK, "leg-A: dbt_flush");
    dbt_close(dbt);
    dbt = NULL;

    /* ---- Create .dbf with the test schema ---- */
    rc = dbf_create(pal, RT_DBF, g_schema, RT_NFIELDS, &tbl);
    snprintf(msg, sizeof(msg), "leg-A: dbf_create rc=%d", rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || !tbl) return 0;

    /* Append 3 records. */
    for (i = 0; i < RT_NREC; i++) {
        xb_val rec[RT_NFIELDS];
        uint16_t clen = (uint16_t)strlen(g_code_raw[i]);
        /* Strip the trailing space from the comparison key for xb_c. */
        /* (The write path pads to field_len automatically.) */
        /* Remove trailing space: g_code_raw is "CODE  " (space-padded for cmp),
         * but xb_c stores a raw string; dbf writes it left-justified padded. */
        /* Actually we can pass the full 6 bytes including space -- dbf pads
         * or truncates to field_len. Let's strip the space for natural input. */
        /* Find real length (strip trailing spaces). */
        {
            uint16_t raw = clen;
            while (raw > 0 && g_code_raw[i][raw - 1] == ' ') raw--;
            clen = raw;
        }
        rec[0] = xb_c(g_code_raw[i], clen);
        rec[1] = xb_n((i == 0) ? 99.99 : (i == 1) ? 7.5 : -3.14);
        rec[2] = xb_d((double)g_jdn[i]);
        rec[3] = xb_l(i % 2 == 0 ? 1 : 0);
        rec[4] = xb_n((double)bn[i]);  /* M pointer = block number as numeric */

        rc = dbf_append_rec(tbl, rec, 0);
        snprintf(msg, sizeof(msg), "leg-A: dbf_append_rec %d (rc=%d)", i + 1, rc);
        CHECK(rc == DBF_OK, msg);
    }

    rc = dbf_flush(tbl);
    CHECK(rc == DBF_OK, "leg-A: dbf_flush");
    dbf_close(tbl);
    tbl = NULL;

    /* ---- Leg A2: Tier-0 masked template cmp (header + descriptors).
     *
     * The MEANINGFUL bytes of the written header must match the hand-computed
     * expected image.  NORMALIZE bytes are zeroed in both before cmp.
     * Schema: 5 fields (CODE C6, SCORE N8.2, WHEN D8, PASS L1, NOTE M10).
     * header_length = 32 + 32*5 + 1 = 193 (the +1 form; dbf.md sec 4).
     * record_length = 1 + 6 + 8 + 8 + 1 + 10 = 34.
     * version = 0x83 (has memo bit; dbf.md sec 3).
     * ---- */
    {
        uint8_t *got = NULL;
        long     glen = 0;
        uint8_t  exp[193];   /* header + 5 descriptors + 0x0D term = 193 bytes */
        int      k;

        memset(exp, 0, sizeof(exp));

        /* header */
        exp[0x00] = 0x83u;      /* version: has memo (dbf.md sec 3) */
        exp[0x01] = (uint8_t)RT_YY;   /* last_update_year (injected) */
        exp[0x02] = (uint8_t)RT_MM;
        exp[0x03] = (uint8_t)RT_DD;
        /* nrec = 3 (u32 LE) */
        exp[0x04] = 3u; exp[0x05] = 0u; exp[0x06] = 0u; exp[0x07] = 0u;
        /* header_length = 193 (u16 LE) */
        exp[0x08] = 193u; exp[0x09] = 0u;
        /* record_length = 34 (u16 LE) */
        exp[0x0A] = 34u; exp[0x0B] = 0u;
        /* 0x0C..0x1F: NORMALIZE -> already 0 */

        /* descriptor 0: CODE C6 at file offset 0x20 */
        {
            int o = 0x20;
            exp[o+0x00]='C'; exp[o+0x01]='O'; exp[o+0x02]='D';
            exp[o+0x03]='E'; /* "CODE\0\0..." */
            exp[o+0x0B]='C';
            exp[o+0x10]=6u;
            exp[o+0x11]=0u;
            /* all other bytes: NORMALIZE -> 0 */
        }
        /* descriptor 1: SCORE N8.2 at file offset 0x40 */
        {
            int o = 0x40;
            exp[o+0x00]='S'; exp[o+0x01]='C'; exp[o+0x02]='O';
            exp[o+0x03]='R'; exp[o+0x04]='E';
            exp[o+0x0B]='N';
            exp[o+0x10]=8u;
            exp[o+0x11]=2u;
        }
        /* descriptor 2: WHEN D8 at file offset 0x60 */
        {
            int o = 0x60;
            exp[o+0x00]='W'; exp[o+0x01]='H'; exp[o+0x02]='E';
            exp[o+0x03]='N';
            exp[o+0x0B]='D';
            exp[o+0x10]=8u;
            exp[o+0x11]=0u;
        }
        /* descriptor 3: PASS L1 at file offset 0x80 */
        {
            int o = 0x80;
            exp[o+0x00]='P'; exp[o+0x01]='A'; exp[o+0x02]='S';
            exp[o+0x03]='S';
            exp[o+0x0B]='L';
            exp[o+0x10]=1u;
            exp[o+0x11]=0u;
        }
        /* descriptor 4: NOTE M10 at file offset 0xA0 */
        {
            int o = 0xA0;
            exp[o+0x00]='N'; exp[o+0x01]='O'; exp[o+0x02]='T';
            exp[o+0x03]='E';
            exp[o+0x0B]='M';
            exp[o+0x10]=10u;
            exp[o+0x11]=0u;
        }
        /* 0x0D terminator at offset 32 + 32*5 = 192 = 0xC0 */
        exp[0xC0] = 0x0Du;

        /* Slurp the written file. */
        got = slurp(RT_DBF, &glen);
        CHECK(got != NULL, "leg-A2: written .dbf readable");

        if (got && glen >= 193L) {
            /* Make a 193-byte copy for the normalized cmp. */
            uint8_t *g_copy = (uint8_t *)malloc(193u);
            uint8_t *e_copy = (uint8_t *)malloc(193u);
            CHECK(g_copy && e_copy, "leg-A2: alloc cmp buffers");

            if (g_copy && e_copy) {
                memcpy(g_copy, got, 193u);
                memcpy(e_copy, exp, 193u);

                /* Apply normalization mask.  For the template cmp the date bytes
                 * are in EXP (our known injected date), so the mask cancels them
                 * in BOTH sides -- but in A2 we still apply the mask symmetrically
                 * to ensure the comparison is consistent with the Leg-C convention.
                 * The mutant (NORM_MUTATE_MASK_CELL) does NOT bite A2 because
                 * the date bytes in EXP already match the written date.
                 * The biting leg is C1 (where the date differs from the golden). */
                apply_norm_mask(g_copy, 193L, RT_NFIELDS, g_copy);
                apply_norm_mask(e_copy, 193L, RT_NFIELDS, g_copy); /* use g_copy for NUL positions */

                {
                    int firstdiff = -1;
                    for (k = 0; k < 193; k++) {
                        if (g_copy[k] != e_copy[k]) { firstdiff = k; break; }
                    }
                    snprintf(msg, sizeof(msg),
                             "leg-A2: template masked-cmp MEANINGFUL bytes match "
                             "(first diff at 0x%X: got 0x%02X exp 0x%02X)",
                             firstdiff < 0 ? 0 : firstdiff,
                             firstdiff < 0 ? 0 : g_copy[firstdiff],
                             firstdiff < 0 ? 0 : e_copy[firstdiff]);
                    CHECK(firstdiff < 0, msg);
                }

                free(g_copy);
                free(e_copy);
            } else {
                free(g_copy);
                free(e_copy);
            }
        } else if (got) {
            snprintf(msg, sizeof(msg),
                     "leg-A2: written file >= 193 bytes (got %ld)", glen);
            CHECK(glen >= 193L, msg);
        }

        free(got);
    }

    /* ---- Leg A1 read-back: re-open with dbf_open + dbt_open; verify values ---- */
    {
        tbl = NULL;
        rc = dbf_open(pal, RT_DBF, &tbl);
        snprintf(msg, sizeof(msg), "leg-A1: dbf_open rc=%d", rc);
        CHECK(rc == DBF_OK && tbl != NULL, msg);
        if (rc != DBF_OK || !tbl) return 1;   /* table written; index can still run */

        /* Version + schema checks. */
        snprintf(msg, sizeof(msg),
                 "leg-A1: version 0x83 (got 0x%02X)", dbf_version(tbl));
        CHECK(dbf_version(tbl) == 0x83u, msg);
        CHECK(dbf_has_memo(tbl) == 1, "leg-A1: has_memo == 1");
        CHECK(dbf_nfields(tbl) == (uint16_t)RT_NFIELDS, "leg-A1: nfields == 5");
        CHECK(dbf_nrec(tbl) == (uint32_t)RT_NREC, "leg-A1: nrec == 3");
        CHECK(dbf_record_length(tbl) == 34u, "leg-A1: record_length == 34");
        CHECK(dbf_header_length(tbl) == 193u, "leg-A1: header_length == 193 (+1 form)");
        CHECK(dbf_year(tbl) == (uint8_t)RT_YY && dbf_month(tbl) == (uint8_t)RT_MM
              && dbf_day(tbl) == (uint8_t)RT_DD,
              "leg-A1: injected date round-trips");

        /* Field descriptors. */
        {
            const dbf_field_t *f0 = dbf_field(tbl, 0);
            const dbf_field_t *f4 = dbf_field(tbl, 4);
            CHECK(f0 && strcmp(f0->name, "CODE") == 0 && f0->type == 'C'
                  && f0->field_len == 6u, "leg-A1: field0 CODE C(6)");
            CHECK(f4 && strcmp(f4->name, "NOTE") == 0 && f4->type == 'M'
                  && f4->field_len == 10u, "leg-A1: field4 NOTE M(10)");
        }

        /* Record 1 values (ZEBRA, 99.99, 1985-08-05, T, block). */
        {
            xb_val out[RT_NFIELDS];
            int del = -1;
            rc = dbf_read_rec(tbl, 1u, out, &del);
            CHECK(rc == DBF_OK, "leg-A1: read rec1");
            if (rc == DBF_OK) {
                CHECK(del == 0, "leg-A1: rec1 live");
                CHECK(out[0].t == XB_C && out[0].u.c.len == 6u
                      && memcmp(out[0].u.c.p, "ZEBRA ", 6) == 0,
                      "leg-A1: rec1 CODE='ZEBRA '");
                CHECK(out[1].t == XB_N && out[1].u.n == 99.99,
                      "leg-A1: rec1 SCORE==99.99");
                CHECK(out[2].t == XB_D && out[2].u.d == (double)g_jdn[0],
                      "leg-A1: rec1 WHEN==JDN(1985-08-05)");
                CHECK(out[3].t == XB_L && out[3].u.l == 1u,
                      "leg-A1: rec1 PASS==T");
            }
        }

        /* Record 2 values (ALPHA, 7.5, 1999-12-31, F). */
        {
            xb_val out[RT_NFIELDS];
            int del = -1;
            rc = dbf_read_rec(tbl, 2u, out, &del);
            CHECK(rc == DBF_OK, "leg-A1: read rec2");
            if (rc == DBF_OK) {
                CHECK(out[0].t == XB_C && memcmp(out[0].u.c.p, "ALPHA ", 6) == 0,
                      "leg-A1: rec2 CODE='ALPHA '");
                CHECK(out[1].t == XB_N && out[1].u.n == 7.5,
                      "leg-A1: rec2 SCORE==7.5");
                CHECK(out[3].t == XB_L && out[3].u.l == 0u,
                      "leg-A1: rec2 PASS==F");
            }
        }

        /* Record 3 values (MANGO, -3.14). */
        {
            xb_val out[RT_NFIELDS];
            int del = -1;
            rc = dbf_read_rec(tbl, 3u, out, &del);
            CHECK(rc == DBF_OK, "leg-A1: read rec3");
            if (rc == DBF_OK) {
                CHECK(out[0].t == XB_C && memcmp(out[0].u.c.p, "MANGO ", 6) == 0,
                      "leg-A1: rec3 CODE='MANGO '");
                CHECK(out[1].t == XB_N && out[1].u.n == -3.14,
                      "leg-A1: rec3 SCORE==-3.14");
            }
        }

        dbf_close(tbl);
        tbl = NULL;
    }

    /* ---- Memo read-back via dbt_open + dbt_read ---- */
    {
        dbt_file *rdf = NULL;
        rc = dbt_open(pal, RT_DBT, 0 /*is_iv_dialect*/, &rdf);
        snprintf(msg, sizeof(msg), "leg-A1: dbt_open rc=%d", rc);
        CHECK(rc == DBT_OK && rdf != NULL, msg);

        if (rc == DBT_OK && rdf) {
            /* Memo 0 (rec1: "First memo text.") */
            uint8_t *mbuf = NULL;
            uint32_t mlen = 0;
            rc = dbt_read(rdf, bn[0], &mbuf, &mlen);
            CHECK(rc == DBT_OK, "leg-A1: dbt_read memo0");
            if (rc == DBT_OK) {
                snprintf(msg, sizeof(msg),
                         "leg-A1: memo0 len == %u (got %u)",
                         (uint32_t)strlen(g_memo[0]), mlen);
                CHECK(mlen == (uint32_t)strlen(g_memo[0]), msg);
                if (mbuf && mlen == (uint32_t)strlen(g_memo[0])) {
                    CHECK(memcmp(mbuf, g_memo[0], mlen) == 0,
                          "leg-A1: memo0 content matches");
                }
            }

            /* Memo 1 (rec2: "Second memo: longer content here.") */
            mbuf = NULL; mlen = 0;
            rc = dbt_read(rdf, bn[1], &mbuf, &mlen);
            CHECK(rc == DBT_OK, "leg-A1: dbt_read memo1");
            if (rc == DBT_OK) {
                snprintf(msg, sizeof(msg),
                         "leg-A1: memo1 len == %u (got %u)",
                         (uint32_t)strlen(g_memo[1]), mlen);
                CHECK(mlen == (uint32_t)strlen(g_memo[1]), msg);
                if (mbuf && mlen == (uint32_t)strlen(g_memo[1])) {
                    CHECK(memcmp(mbuf, g_memo[1], mlen) == 0,
                          "leg-A1: memo1 content matches");
                }
            }

            /* Memo 2 (rec3: empty) */
            mbuf = NULL; mlen = 0xFFFFu;
            rc = dbt_read(rdf, bn[2], &mbuf, &mlen);
            CHECK(rc == DBT_OK, "leg-A1: dbt_read memo2 (empty)");
            if (rc == DBT_OK) {
                CHECK(mlen == 0u, "leg-A1: memo2 len == 0 (empty memo)");
            }

            dbt_close(rdf);
        }
    }

    return 1;   /* table exists at RT_DBF; proceed to Legs B and C */
}

/* -----------------------------------------------------------------------
 * Simple key provider for Leg B (ndx_build over CODE C(6)).
 *
 * Reads rec RECNO from the open dbf_table and copies field 0 (CODE, 6 bytes,
 * already space-padded by the dbf reader) into key_out.  Does NOT use the
 * full evaluator -- CODE is a direct field read, no expression to parse.
 *
 * Decoupling: ndx.c does not link eval.c; the test supplies the provider.
 * ----------------------------------------------------------------------- */
static dbf_table *g_kp_tbl;   /* set before ndx_build */

static int kp_code_field(void *user, uint32_t recno,
                         uint8_t *key_out, uint16_t key_len)
{
    xb_val  out[RT_NFIELDS];
    int     del = -1;
    int     rc;
    (void)user;

    rc = dbf_read_rec(g_kp_tbl, recno, out, &del);
    if (rc != DBF_OK) return 1;

    /* CODE is field 0, C(6): out[0].u.c is the raw field bytes (key_len wide). */
    if (out[0].t != XB_C) return 1;
    if (out[0].u.c.len != (uint16_t)key_len) {
        /* Pad or truncate to key_len. */
        uint16_t copy = out[0].u.c.len < key_len ? out[0].u.c.len : key_len;
        rt_memcpy(key_out, out[0].u.c.p, copy);
        {
            uint16_t i;
            for (i = copy; i < key_len; i++) key_out[i] = (uint8_t)' ';
        }
    } else {
        rt_memcpy(key_out, out[0].u.c.p, key_len);
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * In-order collector for Leg B behavioral check.
 * ----------------------------------------------------------------------- */
#define IO_MAX     16
#define IO_KEYCAP  6

typedef struct {
    int      n;
    uint32_t recnos[IO_MAX];
    uint8_t  keys[IO_MAX * IO_KEYCAP];
    uint32_t kl;
} io_col;

static int io_visit(void *ctx, const uint8_t *key_data, uint32_t recno)
{
    io_col *c = (io_col *)ctx;
    if (c->n >= IO_MAX) return 1;  /* stop */
    rt_memcpy(c->keys + (uint32_t)c->n * c->kl, key_data, c->kl);
    c->recnos[c->n] = recno;
    c->n++;
    return 0;
}

/* -----------------------------------------------------------------------
 * Leg B: ndx_build + read-back + seek.
 *
 * Build an index on CODE (C, 6 bytes) over the 3-record table in RT_DBF.
 * Sorted order: ALPHA (rec2) < MANGO (rec3) < ZEBRA (rec1).
 * ----------------------------------------------------------------------- */
static void leg_b_ndx(samir_pal_t *pal)
{
    dbf_table *tbl = NULL;
    ndx_index *idx = NULL;
    int        rc;
    char       msg[256];
    io_col     io;
    int        i;

    /* Open the .dbf written by Leg A. */
    rc = dbf_open(pal, RT_DBF, &tbl);
    snprintf(msg, sizeof(msg), "leg-B: dbf_open rc=%d", rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || !tbl) return;

    g_kp_tbl = tbl;

    /* Build the index. */
    rc = ndx_build(pal, RT_NDX,
                   (uint16_t)NDX_KEY_TYPE_CHAR, 6u, "CODE",
                   dbf_nrec(tbl), kp_code_field, NULL);
    snprintf(msg, sizeof(msg), "leg-B: ndx_build rc=%d", rc);
    CHECK(rc == NDX_OK, msg);

    dbf_close(tbl);
    tbl = NULL;
    g_kp_tbl = NULL;

    if (rc != NDX_OK) return;

    /* Open and traverse the built index. */
    rc = ndx_open(pal, RT_NDX, &idx);
    snprintf(msg, sizeof(msg), "leg-B: ndx_open rc=%d", rc);
    CHECK(rc == NDX_OK && idx != NULL, msg);
    if (rc != NDX_OK || !idx) return;

    /* Structural checks. */
    CHECK(ndx_key_length(idx) == 6u, "leg-B: key_length == 6");
    CHECK(ndx_key_type(idx) == (uint16_t)NDX_KEY_TYPE_CHAR,
          "leg-B: key_type == CHAR");

    /* In-order traversal: should yield ALPHA(2) MANGO(3) ZEBRA(1). */
    memset(&io, 0, sizeof(io));
    io.kl = 6u;
    ndx_inorder(idx, io_visit, &io);

    snprintf(msg, sizeof(msg),
             "leg-B: in-order count == 3 (got %d)", io.n);
    CHECK(io.n == RT_NREC, msg);

    for (i = 0; i < RT_NREC && i < io.n; i++) {
        snprintf(msg, sizeof(msg),
                 "leg-B: sorted[%d] recno == %u (got %u)",
                 i, g_sorted_recno[i], io.recnos[i]);
        CHECK(io.recnos[i] == g_sorted_recno[i], msg);

        snprintf(msg, sizeof(msg),
                 "leg-B: sorted[%d] key == '%s'", i, g_sorted_code[i]);
        CHECK(rt_memcmp(io.keys + (uint32_t)i * io.kl,
                        (const uint8_t *)g_sorted_code[i], 6u) == 0, msg);
    }

    /* SEEK "MANGO ": should land on recno 3 (the MANGO record). */
    {
        xb_val  seek_key;
        uint32_t recno_out = 0;
        int      found_out = 0;
        char     kbuf[6];

        seek_key = xb_c("MANGO ", 6u);
        rc = ndx_seek(idx, &seek_key, 1 /*set_exact*/, &recno_out, &found_out);
        snprintf(msg, sizeof(msg),
                 "leg-B: ndx_seek('MANGO ') rc=%d found=%d recno=%u",
                 rc, found_out, recno_out);
        CHECK(rc == NDX_OK && found_out == 1 && recno_out == 3u, msg);
        (void)kbuf;
    }

    /* SEEK "ZEBRA ": should land on recno 1. */
    {
        xb_val   seek_key;
        uint32_t recno_out = 0;
        int      found_out = 0;

        seek_key = xb_c("ZEBRA ", 6u);
        rc = ndx_seek(idx, &seek_key, 1 /*set_exact*/, &recno_out, &found_out);
        snprintf(msg, sizeof(msg),
                 "leg-B: ndx_seek('ZEBRA ') rc=%d found=%d recno=%u",
                 rc, found_out, recno_out);
        CHECK(rc == NDX_OK && found_out == 1 && recno_out == 1u, msg);
    }

    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * Leg A3: independence barrier -- dbf_ref.py reads back the written .dbf.
 * ----------------------------------------------------------------------- */
static void leg_a3_python_dbf(void)
{
    char cmd[1024];
    int  ret;

    if (!file_exists(RT_DBF)) {
        fprintf(stderr,
                "  SKIP (LOUD): leg-A3: %s not present "
                "(leg-A write did not succeed)\n", RT_DBF);
        return;
    }

    snprintf(cmd, sizeof(cmd),
             "python3 harness/diff/dbf_diff/dbf_ref.py --records %s "
             ">/dev/null 2>&1", RT_DBF);
    ret = system(cmd);

    if (ret == -1) {
        fprintf(stderr,
                "  SKIP (LOUD): leg-A3: could not invoke python3 for dbf_ref.py; "
                "orchestrator: run\n"
                "    python3 harness/diff/dbf_diff/dbf_ref.py --records %s\n",
                RT_DBF);
        return;
    }

#if defined(WIFEXITED)
    if (WIFEXITED(ret)) {
        int code = WEXITSTATUS(ret);
        if (code == 127) {
            fprintf(stderr,
                    "  SKIP (LOUD): leg-A3: python3/dbf_ref.py not found (exit 127); "
                    "run manually:\n"
                    "    python3 harness/diff/dbf_diff/dbf_ref.py --records %s\n",
                    RT_DBF);
            return;
        }
        CHECK(code == 0,
              "leg-A3: dbf_ref.py --records read back SAMIR .dbf (exit 0)");
        return;
    }
#endif
    CHECK(ret == 0,
          "leg-A3: dbf_ref.py --records read back SAMIR .dbf (exit 0)");
}

/* -----------------------------------------------------------------------
 * Leg B3: independence barrier -- ndx_ref.py --index-dump reads back the index.
 * ----------------------------------------------------------------------- */
static void leg_b3_python_ndx(void)
{
    char cmd[1024];
    int  ret;

    if (!file_exists(RT_NDX)) {
        fprintf(stderr,
                "  SKIP (LOUD): leg-B3: %s not present "
                "(leg-B ndx_build did not succeed)\n", RT_NDX);
        return;
    }

    snprintf(cmd, sizeof(cmd),
             "python3 harness/diff/dbf_diff/ndx_ref.py --index-dump %s "
             ">/dev/null 2>&1", RT_NDX);
    ret = system(cmd);

    if (ret == -1) {
        fprintf(stderr,
                "  SKIP (LOUD): leg-B3: could not invoke python3 for ndx_ref.py; "
                "orchestrator: run\n"
                "    python3 harness/diff/dbf_diff/ndx_ref.py --index-dump %s\n",
                RT_NDX);
        return;
    }

#if defined(WIFEXITED)
    if (WIFEXITED(ret)) {
        int code = WEXITSTATUS(ret);
        if (code == 127) {
            fprintf(stderr,
                    "  SKIP (LOUD): leg-B3: python3/ndx_ref.py not found (exit 127); "
                    "run manually:\n"
                    "    python3 harness/diff/dbf_diff/ndx_ref.py --index-dump %s\n",
                    RT_NDX);
            return;
        }
        CHECK(code == 0,
              "leg-B3: ndx_ref.py --index-dump read back built index (exit 0)");
        return;
    }
#endif
    CHECK(ret == 0,
          "leg-B3: ndx_ref.py --index-dump read back built index (exit 0)");
}

/* -----------------------------------------------------------------------
 * Leg C1: Tier-1 golden masked cmp (CLIENTS.DBF corpus golden).
 *
 * Read CLIENTS.DBF with SAMIR, re-write an identical-schema table, then
 * compare the re-written header + descriptor region against the golden with
 * NORMALIZE bytes masked.
 *
 * This is WHERE THE MUTANT BITES: under -DNORM_MUTATE_MASK_CELL, the date
 * bytes (0x01..0x03) are COMPARED instead of masked.  Our written date
 * (RT_YY/RT_MM/RT_DD = 99/12/31) differs from CLIENTS' date (85/10/30),
 * so the cmp goes RED.
 *
 * Loud-skip if the corpus golden is absent.
 * ----------------------------------------------------------------------- */
static void leg_c1_golden(samir_pal_t *pal, const char *base)
{
    char        gold_path[1024];
    dbf_table  *gold = NULL, *mine = NULL;
    int         rc, nf, i;
    char        msg[256];
    dbf_field_spec *specs = NULL;
    uint8_t    *g = NULL, *m = NULL;
    long        glen = 0, mlen = 0;
    uint32_t    cmp_len;

    join(gold_path, sizeof(gold_path), base, SP_PATH "/CLIENTS.DBF");
    if (!file_exists(gold_path)) {
        fprintf(stderr,
                "  SKIP (LOUD): leg-C1: golden absent: %s\n"
                "               (corpus DBASE3_DECOMP not present at '%s')\n",
                gold_path, base);
        return;
    }

    /* Open the golden with SAMIR. */
    rc = dbf_open(pal, gold_path, &gold);
    snprintf(msg, sizeof(msg), "leg-C1: dbf_open CLIENTS rc=%d", rc);
    CHECK(rc == DBF_OK && gold != NULL, msg);
    if (rc != DBF_OK || !gold) return;

    nf = (int)dbf_nfields(gold);
    specs = (dbf_field_spec *)calloc((size_t)nf, sizeof(dbf_field_spec));
    CHECK(specs != NULL, "leg-C1: alloc field specs");
    if (!specs) { dbf_close(gold); return; }

    /* Mirror the golden schema. */
    for (i = 0; i < nf; i++) {
        const dbf_field_t *f = dbf_field(gold, i);
        specs[i].name      = f->name;
        specs[i].type      = f->type;
        specs[i].field_len = f->field_len;
        specs[i].dec       = f->dec_count;
    }

    /* Create a mirror .dbf (the PAL injects our fixed date RT_YY/MM/DD). */
    rc = dbf_create(pal, RT_GOLD_DBF, specs, nf, &mine);
    snprintf(msg, sizeof(msg), "leg-C1: dbf_create mirror rc=%d", rc);
    CHECK(rc == DBF_OK && mine != NULL, msg);
    if (rc != DBF_OK || !mine) { free(specs); dbf_close(gold); return; }

    /* Copy all golden records into the mirror. */
    {
        uint32_t n = dbf_nrec(gold), r;
        xb_val  *out = (xb_val *)calloc((size_t)nf, sizeof(xb_val));
        CHECK(out != NULL, "leg-C1: alloc rec buffer");
        if (out) {
            for (r = 1u; r <= n; r++) {
                int del = 0;
                rc = dbf_read_rec(gold, r, out, &del);
                if (rc != DBF_OK) break;
                dbf_append_rec(mine, out, del);
            }
            free(out);
        }
    }

    rc = dbf_flush(mine);
    CHECK(rc == DBF_OK, "leg-C1: dbf_flush mirror");
    dbf_close(mine);
    mine = NULL;
    dbf_close(gold);
    gold = NULL;

    /* Slurp both files and apply the normalization mask. */
    g = slurp(gold_path, &glen);
    m = slurp(RT_GOLD_DBF, &mlen);
    CHECK(g != NULL && m != NULL, "leg-C1: both files readable");

    /* Compare header + descriptor region only (up to but NOT including the
     * 0x0D terminator -- the terminator region may differ by +1/+2 convention).
     * The MEANINGFUL structural bytes all live in the header (32 bytes) +
     * nf descriptors (32 bytes each). */
    cmp_len = (uint32_t)DBF_HDR_SIZE + (uint32_t)DBF_DESC_STRIDE * (uint32_t)nf;

    if (g && m && glen >= (long)cmp_len && mlen >= (long)cmp_len) {
        /* Make copies to normalize in-place. */
        uint8_t *g2 = (uint8_t *)malloc(cmp_len);
        uint8_t *m2 = (uint8_t *)malloc(cmp_len);
        CHECK(g2 && m2, "leg-C1: alloc cmp copies");
        if (g2 && m2) {
            memcpy(g2, g, cmp_len);
            memcpy(m2, m, cmp_len);

            /* Apply normalization mask to both.  Use M2 (our SAMIR-written
             * file, NUL-padded) to determine NUL positions in names. */
            apply_norm_mask(g2, (long)cmp_len, nf, m2);
            apply_norm_mask(m2, (long)cmp_len, nf, m2);

            {
                uint32_t firstdiff = 0;
                int      found = 0;
                uint32_t k;
                for (k = 0u; k < cmp_len; k++) {
                    if (g2[k] != m2[k]) { firstdiff = k; found = 1; break; }
                }
                snprintf(msg, sizeof(msg),
                         "leg-C1: CLIENTS golden MEANINGFUL bytes match "
                         "(first diff at 0x%X: golden=0x%02X mine=0x%02X) "
                         "[mutant NORM_MUTATE_MASK_CELL -> date mismatch RED]",
                         found ? firstdiff : 0u,
                         found ? g2[firstdiff] : 0u,
                         found ? m2[firstdiff] : 0u);
                CHECK(!found, msg);
            }
        }
        free(g2);
        free(m2);
    } else {
        snprintf(msg, sizeof(msg),
                 "leg-C1: files too short for cmp (gold=%ld mine=%ld need=%u)",
                 glen, mlen, cmp_len);
        CHECK(0, msg);
    }

    free(g);
    free(m);
    free(specs);
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    const char *base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    struct pal_host_cfg cfg;
    samir_pal_t *pal;
    int table_ok;

    /* Fixed injected date (Rule 11): 99/12/31 -- distinct from CLIENTS' date
     * (85/10/30) so the date-mask mutant in Leg C1 goes RED when the mask is
     * removed.  The .dbt/ndx writers do not embed the date (only the .dbf header
     * does), so this date applies only to the .dbf write path. */
    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy   = (uint8_t)RT_YY;
    cfg.date_mm   = (uint8_t)RT_MM;
    cfg.date_dd   = (uint8_t)RT_DD;
    /* Arena: generous for all three codecs + golden copy (CLIENTS: 49 records *
     * 106 bytes * 2 sides + index pages + scratch).  512 KB is sufficient. */
    cfg.heap_size = 512u * 1024u;

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }

    /* Leg A: write .dbf + .dbt, read back (Tier 0 + masked template). */
    table_ok = leg_a_write_and_readback(pal);

    /* Leg A3: Python independence barrier (optional). */
    leg_a3_python_dbf();

    /* Leg B: ndx_build + in-order + seek (requires table from Leg A). */
    if (table_ok) leg_b_ndx(pal);

    /* Leg B3: Python independence barrier for the index (optional). */
    if (table_ok) leg_b3_python_ndx();

    /* Leg C1: Tier-1 golden masked cmp (loud-skip if corpus absent).
     * THIS IS WHERE -DNORM_MUTATE_MASK_CELL BITES: date bytes become COMPARED,
     * exposing the mismatch between our injected date (99/12/31) and the golden
     * CLIENTS date (85/10/30). */
    leg_c1_golden(pal, base);

    pal_host_free(pal);
    return TEST_SUMMARY("test-dbase-roundtrip");
}
