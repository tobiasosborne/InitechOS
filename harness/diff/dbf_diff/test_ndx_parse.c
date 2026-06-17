/*
 * harness/diff/dbf_diff/test_ndx_parse.c -- host oracle for ndx_open + ndx_read_node
 *                                            (step S4.1).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc is OK here. Mirrors test_dbf_header.c:
 * seed test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY), a host PAL
 * via pal_host_make (pal_host.c). Non-zero exit on any failed check keeps the
 * gate from false-greening (Law 2: the oracle is the truth).
 *
 * Two tiers (plan Sec 2.A):
 *   Tier 0 (committed, operator-free): expected header values transcribed from
 *     ndx.md sec 2.2 "Header validated bytes (CNAMES.NDX)" and the "Verification"
 *     section; each value is cited to the ndx.md source. Opens the REAL golden
 *     through the PAL. Runs even if the golden is absent (loud-skip per file).
 *   Tier 1 (golden-diff): all 11 corpus .ndx files; full header field check +
 *     node entry_count + first-entry structure (child_page, dbf_recno, key_data
 *     prefix). Loud-skip if the file is absent.
 *
 * Mutation (Rule 6): built with -DNDX_MUTATE_SUBLAYOUT, the "clicketyclick wrong
 * 18-23 sublayout" perturbation in ndx.c swaps child_page/dbf_recno offsets and
 * shifts key_data +4; every leaf entry then has wrong recno and wrong key bytes.
 * The Tier-1 checks on TOURDATE (30 leaf entries, verified key bytes) go RED,
 * and the CUSTOMER recno check goes RED. Exit code non-zero -> mutation gate
 * passes (RED is correct when -DNDX_MUTATE_SUBLAYOUT is active).
 *
 * Goldens base is argv[1] (default "../dbase3-decomp"). Goldens path:
 *   <base>/goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/
 *
 * Compile + run (self-grade; NOT make):
 *   gcc -std=c11 -Wall -Wextra -Werror -Iseed -Ios/samir/include -Ispec \
 *     os/samir/fs/ndx.c os/samir/core/rt.c os/samir/pal/pal_host.c \
 *     harness/diff/dbf_diff/test_ndx_parse.c -o /tmp/test_ndx_parse \
 *     && /tmp/test_ndx_parse ../dbase3-decomp
 *
 * Mutant (must exit non-zero):
 *   gcc -std=c11 -Wall -Wextra -Werror -DNDX_MUTATE_SUBLAYOUT \
 *     -Iseed -Ios/samir/include -Ispec \
 *     os/samir/fs/ndx.c os/samir/core/rt.c os/samir/pal/pal_host.c \
 *     harness/diff/dbf_diff/test_ndx_parse.c -o /tmp/test_ndx_mutant \
 *     && /tmp/test_ndx_mutant ../dbase3-decomp; echo "exit $?"
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/ndx.md  ss2 (header table + casing),
 *       ss2.1 (key_expr verbatim), ss2.2 (CNAMES worked byte dump),
 *       ss3 (node 2+2 header), ss3.1 (group layout), ss3.2 (trailing child),
 *       ss4.2 (TOURDATE JDN bytes), Verification section.
 *   - spec/samir/ndx_format.h (LOCKED offset constants).
 *   - docs/plans/SAMIR-implementation-plan.md S4.1 oracle contract.
 *   - os/samir/include/samir/ndx.h (API under test).
 *   - seed/test_assert.h (harness idiom).
 */

#include <stdio.h>
#include <string.h>

#include "test_assert.h"       /* seed/, on -Iseed */
#include "samir/ndx.h"         /* os/samir/include/, on -Ios/samir/include */

TEST_HARNESS();

/* pal_host.c surface (declared here; not in a header). */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* -----------------------------------------------------------------------
 * Tier-0 header manifest (one row per golden)
 * Values transcribed from ndx.md sec 2.2 (CNAMES worked byte dump) and the
 * Verification section. Each value is cited below.
 * ----------------------------------------------------------------------- */
typedef struct {
    const char *relpath;      /* path relative to corpus base */
    const char *label;        /* short name for messages */
    uint32_t    root_page;    /* ndx.md ss2 table, Verification */
    uint32_t    total_pages;  /* ndx.md ss2 table, Verification */
    uint16_t    key_length;   /* ndx.md ss2 table, Verification */
    uint16_t    keys_per_page;/* ndx.md ss2 table, Verification */
    uint16_t    key_type;     /* ndx.md ss2 table, Verification */
    uint16_t    group_length; /* ndx.md ss2 formula verified, Verification */
    uint16_t    unique_flag;  /* ndx.md Verification: all 11 = 0 */
    const char *key_expr;     /* ndx.md ss2.1 (verbatim), Verification */
} ndx_expect;

/*
 * Tier-0 manifest for 5 representative goldens.
 * All values sourced from ndx.md Verification section (byte-verified 2026-06-16).
 *
 * CNAMES.NDX: ndx.md ss2.2 (worked byte dump).
 *   root=6, total=7, KL=40, KPP=10, type=0, grp=48, unique=0
 *   key_expr="LASTNAME + FIRSTNAME " [ndx.md ss2.1 + Verification casing check]
 *
 * TOURDATE.NDX: ndx.md ss4.2 (date key example) + Verification.
 *   root=1, total=2, KL=8, KPP=31, type=1, grp=16, unique=0
 *   key_expr="DEPARTURE " [ndx.md Verification "casing verbatim"]
 *
 * ZIPCODE.NDX: ndx.md Verification ("KL 5->grp 16/kpp 31").
 *   root=3, total=4, KL=5, KPP=31, type=0, grp=16, unique=0
 *   key_expr="ZIPCODE "
 *
 * CUSTOMER.NDX: ndx.md Verification ("KL 28->grp 36/kpp 14").
 *   root=1, total=2, KL=28, KPP=14, type=0, grp=36, unique=0
 *   key_expr="CDES_CITY+CNAME " [ndx.md Verification]
 *
 * CHKNO.NDX: ndx.md ss2 (numeric key_type=1, KL=8); Verification.
 *   root=1, total=2, KL=8, KPP=31, type=1, grp=16, unique=0
 *   key_expr="Chkno " [ndx.md Verification: "mixed case; the DBF field is CHKNO"]
 */
#define PRISTINE_REL "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities"

static const ndx_expect MANIFEST[] = {
    /* CNAMES.NDX -- ndx.md ss2.2 worked example (primary ground truth).
     * root_page=6: ndx.md ss2 "Page number of the current root node. CNAMES=6."
     * total_pages=7: ndx.md ss2 "CNAMES=7."
     * key_length=40: "CNAMES=40 (LASTNAME C20 + FIRSTNAME C20)."
     * keys_per_page=10: "CNAMES=10 (508/48=10)."
     * key_type=0: "CNAMES=0 (char)."
     * group_length=48: "ceil4(40+8)=ceil4(48)=48."
     * key_expr verbatim: ndx.md ss2.1 "LASTNAME + FIRSTNAME " with trailing space.
     *   [verified: bytes 0x18-0x2C = "LASTNAME + FIRSTNAME \0"] */
    { PRISTINE_REL "/CNAMES.NDX",  "CNAMES",
      6, 7, 40, 10, 0, 48, 0, "LASTNAME + FIRSTNAME " },

    /* TOURDATE.NDX -- ndx.md ss4.2 date key example + Verification.
     * key_type=1: "TOURDATE=1 (D field)." [ndx.md ss2]
     * KL=8: double (8-byte IEEE-754). KPP=31 (508/16=31). grp=16.
     * key_expr: "DEPARTURE " [ndx.md Verification "casing verbatim"] */
    { PRISTINE_REL "/TOURDATE.NDX", "TOURDATE",
      1, 2, 8, 31, 1, 16, 0, "DEPARTURE " },

    /* ZIPCODE.NDX -- ndx.md Verification "KL 5->grp 16/kpp 31".
     * key_type=0 (char), KL=5, KPP=31, grp=16.
     * key_expr: "ZIPCODE " [ndx.md Verification]. */
    { PRISTINE_REL "/ZIPCODE.NDX",  "ZIPCODE",
      3, 4, 5, 31, 0, 16, 0, "ZIPCODE " },

    /* CUSTOMER.NDX -- ndx.md Verification "KL 28->grp 36/kpp 14".
     * root=1, total=2: single-page tree (header + one leaf).
     * key_expr: "CDES_CITY+CNAME " [ndx.md Verification]. */
    { PRISTINE_REL "/CUSTOMER.NDX", "CUSTOMER",
      1, 2, 28, 14, 0, 36, 0, "CDES_CITY+CNAME " },

    /* CHKNO.NDX -- numeric key_type=1 over N field; Verification.
     * "CHKNO=1 (N field)." [ndx.md ss2] CHECKS.DBF nrec=0 so leaf is empty.
     * key_expr: "Chkno " [ndx.md ss2.1 casing: "mixed case; the DBF field is CHKNO"]. */
    { PRISTINE_REL "/CHKNO.NDX",    "CHKNO",
      1, 2, 8, 31, 1, 16, 0, "Chkno " },
};
#define MANIFEST_N ((int)(sizeof(MANIFEST) / sizeof(MANIFEST[0])))

/* -----------------------------------------------------------------------
 * Tier-1 extended manifest (full 11 fixtures)
 * subset we check at Tier-1 beyond Tier-0: node entry_count + first-entry
 * structure for several goldens.
 * ----------------------------------------------------------------------- */

/* Join base + relpath -> buf. */
static char *join(char *buf, size_t cap, const char *base, const char *rel)
{
    snprintf(buf, cap, "%s/%s", base, rel);
    return buf;
}

/* True if the file is readable. */
static int file_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

/* -----------------------------------------------------------------------
 * check_header: open one golden, assert all header fields match the manifest.
 * ----------------------------------------------------------------------- */
static void check_header(samir_pal_t *pal, const char *base, const ndx_expect *e)
{
    char       path[1024];
    char       msg[256];
    ndx_index *idx = (ndx_index *)0;
    int        rc;

    join(path, sizeof(path), base, e->relpath);
    if (!file_exists(path)) {
        fprintf(stderr,
                "  SKIP (LOUD): golden absent: %s -- pass corpus base as argv[1]\n",
                path);
        return;
    }

    rc = ndx_open(pal, path, &idx);
    snprintf(msg, sizeof(msg), "%s: ndx_open succeeds (rc=%d)", e->label, rc);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0, msg);
    if (rc != NDX_OK || !idx) return;

    /* All 10 header fields per ndx.md ss2 table / ndx_format.h NDX_HDR_* . */
    snprintf(msg, sizeof(msg), "%s: root_page %u", e->label, e->root_page);
    CHECK(ndx_root_page(idx) == e->root_page, msg);

    snprintf(msg, sizeof(msg), "%s: total_pages %u", e->label, e->total_pages);
    CHECK(ndx_total_pages(idx) == e->total_pages, msg);

    /* reserved: always 0 in all 11 III+ fixtures [ndx.md Verification]. */
    snprintf(msg, sizeof(msg), "%s: reserved == 0", e->label);
    CHECK(ndx_reserved(idx) == 0u, msg);

    snprintf(msg, sizeof(msg), "%s: key_length %u", e->label, e->key_length);
    CHECK(ndx_key_length(idx) == e->key_length, msg);

    snprintf(msg, sizeof(msg), "%s: keys_per_page %u", e->label, e->keys_per_page);
    CHECK(ndx_keys_per_page(idx) == e->keys_per_page, msg);

    snprintf(msg, sizeof(msg), "%s: key_type %u", e->label, e->key_type);
    CHECK(ndx_key_type(idx) == e->key_type, msg);

    snprintf(msg, sizeof(msg), "%s: group_length %u", e->label, e->group_length);
    CHECK(ndx_group_length(idx) == e->group_length, msg);

    /* dummy: volatile (0x0010/0x0008/0x000F or 0) -- NOT asserted [ndx.md ss2]. */

    snprintf(msg, sizeof(msg), "%s: unique_flag %u", e->label, e->unique_flag);
    CHECK(ndx_unique_flag(idx) == e->unique_flag, msg);

    /* key_expr: verbatim (NOT lowercased).
     * Ref: ndx.md ss2.1 "stored verbatim AS TYPED (NOT lowercased)". */
    snprintf(msg, sizeof(msg), "%s: key_expr == %s", e->label, e->key_expr);
    CHECK(strcmp(ndx_key_expr(idx), e->key_expr) == 0, msg);

    rc = ndx_close(idx);
    snprintf(msg, sizeof(msg), "%s: ndx_close succeeds (rc=%d)", e->label, rc);
    CHECK(rc == NDX_OK, msg);
}

/* -----------------------------------------------------------------------
 * check_cnames_node: verify root node of CNAMES.NDX.
 *
 * CNAMES root page 6 is a branch node with entry_count=4 and filler=0.
 * [ndx.md Verification: "CNAMES root count=4, filler=0"]
 * First entry: child=1, recno=0, key="Collins             Sara".
 * [ndx.md Verification: "CNAMES root e0 'Collins...Sara' == leaf page1 last key"]
 * ----------------------------------------------------------------------- */
static void check_cnames_node(samir_pal_t *pal, const char *base)
{
    char        path[1024];
    ndx_index  *idx = (ndx_index *)0;
    ndx_node_t *node = (ndx_node_t *)0;
    int         rc;
    char        msg[256];

    join(path, sizeof(path), base, PRISTINE_REL "/CNAMES.NDX");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): CNAMES.NDX absent for node check\n");
        return;
    }

    rc = ndx_open(pal, path, &idx);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0, "CNAMES node: ndx_open");
    if (rc != NDX_OK || !idx) return;

    /* Read the root node (page 6).
     * Ref: ndx.md ss2 "CNAMES=6" (root_page). */
    rc = ndx_read_node(idx, 6u, &node);
    CHECK(rc == NDX_OK && node != (ndx_node_t *)0, "CNAMES root: ndx_read_node(6)");
    if (rc != NDX_OK || !node) { ndx_close(idx); return; }

    /* entry_count=4, filler=0.
     * [ndx.md ss3 / Verification "CNAMES root count=4, filler=0"] */
    CHECK(node->entry_count == 4, "CNAMES root: entry_count == 4");
    CHECK(node->filler == 0,      "CNAMES root: filler == 0");

    /* Entry 0 is a branch entry: child_page=1, recno=0 (branch entries have recno=0).
     * [ndx.md ss3.1 "branch entries carry recno=0 and child!=0";
     *  Verification "CNAMES root entry0 key 'Collins...Sara' == leaf page1 last key"]
     * ndx.md ss3.1: "child_page: 0 => LEAF, non-zero => BRANCH" */
    CHECK(node->entry_count >= 1u, "CNAMES root: at least one entry");
    if (node->entry_count >= 1u) {
        snprintf(msg, sizeof(msg),
                 "CNAMES root entry[0].child_page==1 (got %u)",
                 node->entries[0].child_page);
        CHECK(node->entries[0].child_page == 1u, msg);
        CHECK(node->entries[0].dbf_recno == 0u,
              "CNAMES root entry[0].dbf_recno==0 (branch)");

        /* First 7 bytes of key: "Collins".
         * [ndx.md Verification: "CNAMES root entry0 'Collins...Sara'"] */
        CHECK(node->entries[0].key_data[0] == 'C', "CNAMES root entry[0] key[0]=='C'");
        CHECK(node->entries[0].key_data[1] == 'o', "CNAMES root entry[0] key[1]=='o'");
        CHECK(node->entries[0].key_data[2] == 'l', "CNAMES root entry[0] key[2]=='l'");
        CHECK(node->entries[0].key_data[3] == 'l', "CNAMES root entry[0] key[3]=='l'");
        CHECK(node->entries[0].key_data[4] == 'i', "CNAMES root entry[0] key[4]=='i'");
        CHECK(node->entries[0].key_data[5] == 'n', "CNAMES root entry[0] key[5]=='n'");
        CHECK(node->entries[0].key_data[6] == 's', "CNAMES root entry[0] key[6]=='s'");
    }

    /* Trailing child pointer (rightmost subtree, after last separator).
     * [ndx.md Verification "CNAMES root trailing child=5"] */
    CHECK(node->trail_child == 5u, "CNAMES root: trail_child == 5");

    ndx_node_free(idx, node);
    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * check_tourdate_node: verify leaf node of TOURDATE.NDX.
 *
 * TOURDATE is a single-page tree: root = page 1 (the only node, a leaf).
 * entry_count = 30 (all 30 TOURS records), filler = 0x0000.
 * [ndx.md Verification "TOURDATE key_type=1, KL=8, grp=16, 30 live entries"]
 *
 * First entry (entry 0):
 *   child_page = 0 (leaf)
 *   dbf_recno  = 1  (TOURS record 1)
 *   key_data   = 00 00 00 80 E5 A9 42 41  (JDN 2446283.0 = 1985-08-05)
 * [ndx.md ss4.2 "TOURDATE leaf entry 0 key_data = 00 00 00 80 E5 A9 42 41"]
 * [ndx.md Verification "19850805->JDN 2446283->'00 00 00 80 E5 A9 42 41'"]
 *
 * This check is the primary mutant-detector: under NDX_MUTATE_SUBLAYOUT the
 * child/recno fields are swapped and key_data is shifted +4, so child!=0 (wrong),
 * recno!=1 (wrong), and key bytes are garbage -> several CHECKs go RED.
 * ----------------------------------------------------------------------- */
static void check_tourdate_node(samir_pal_t *pal, const char *base)
{
    char        path[1024];
    ndx_index  *idx  = (ndx_index *)0;
    ndx_node_t *node = (ndx_node_t *)0;
    int         rc;
    char        msg[256];

    join(path, sizeof(path), base, PRISTINE_REL "/TOURDATE.NDX");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): TOURDATE.NDX absent for node check\n");
        return;
    }

    rc = ndx_open(pal, path, &idx);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0, "TOURDATE node: ndx_open");
    if (rc != NDX_OK || !idx) return;

    /* root_page = 1 (from header). Read it.
     * [ndx.md: TOURDATE root=1, total=2] */
    rc = ndx_read_node(idx, 1u, &node);
    CHECK(rc == NDX_OK && node != (ndx_node_t *)0, "TOURDATE root: ndx_read_node(1)");
    if (rc != NDX_OK || !node) { ndx_close(idx); return; }

    /* entry_count = 30 (all 30 TOURS records fit on one leaf).
     * [ndx.md Verification "30 live entries"; kpp=31, so one full-ish leaf] */
    snprintf(msg, sizeof(msg),
             "TOURDATE root: entry_count==30 (got %u)", node->entry_count);
    CHECK(node->entry_count == 30u, msg);

    /* Entry 0 is a LEAF entry: child_page=0, recno=1.
     * [ndx.md ss4.2 "TOURDATE leaf entry 0 ... recno=1";
     *  Verification "19850805->JDN 2446283 -> 00 00 00 80 E5 A9 42 41 (match)"] */
    if (node->entry_count >= 1u) {
        snprintf(msg, sizeof(msg),
                 "TOURDATE entry[0].child_page==0 (got %u)",
                 node->entries[0].child_page);
        CHECK(node->entries[0].child_page == 0u, msg);

        snprintf(msg, sizeof(msg),
                 "TOURDATE entry[0].dbf_recno==1 (got %u)",
                 node->entries[0].dbf_recno);
        CHECK(node->entries[0].dbf_recno == 1u, msg);

        /* Key bytes for JDN 2446283.0 = 1985-08-05.
         * [ndx.md ss4.2 worked example; Verification byte-exact match]
         * Expected raw LE bytes: 00 00 00 80 E5 A9 42 41. */
        CHECK(node->entries[0].key_data[0] == 0x00u,
              "TOURDATE entry[0] key[0]==0x00");
        CHECK(node->entries[0].key_data[1] == 0x00u,
              "TOURDATE entry[0] key[1]==0x00");
        CHECK(node->entries[0].key_data[2] == 0x00u,
              "TOURDATE entry[0] key[2]==0x00");
        CHECK(node->entries[0].key_data[3] == 0x80u,
              "TOURDATE entry[0] key[3]==0x80");
        CHECK(node->entries[0].key_data[4] == 0xE5u,
              "TOURDATE entry[0] key[4]==0xE5");
        CHECK(node->entries[0].key_data[5] == 0xA9u,
              "TOURDATE entry[0] key[5]==0xA9");
        CHECK(node->entries[0].key_data[6] == 0x42u,
              "TOURDATE entry[0] key[6]==0x42");
        CHECK(node->entries[0].key_data[7] == 0x41u,
              "TOURDATE entry[0] key[7]==0x41");
    }

    ndx_node_free(idx, node);
    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * check_customer_node: verify CUSTOMER.NDX leaf (single-page tree).
 *
 * CUSTOMER root = page 1, entry_count = 5, filler = 0x5543 (garbage).
 * [ndx.md ss3 Verification "CUSTOMER entry_count=5, filler=0x5543 garbage"]
 * All entries are leaf entries (child_page=0).
 * rec1 key starts with "ATL" (CDES_CITY) then "LOUIS" (CNAME).
 * [ndx.md Verification "CUSTOMER rec1 == CDES_CITY 'ATL'+CNAME 'LOUIS' (28)"]
 * ----------------------------------------------------------------------- */
static void check_customer_node(samir_pal_t *pal, const char *base)
{
    char        path[1024];
    ndx_index  *idx  = (ndx_index *)0;
    ndx_node_t *node = (ndx_node_t *)0;
    int         rc;
    char        msg[256];
    uint32_t    i;

    join(path, sizeof(path), base, PRISTINE_REL "/CUSTOMER.NDX");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): CUSTOMER.NDX absent for node check\n");
        return;
    }

    rc = ndx_open(pal, path, &idx);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0, "CUSTOMER node: ndx_open");
    if (rc != NDX_OK || !idx) return;

    rc = ndx_read_node(idx, 1u, &node);
    CHECK(rc == NDX_OK && node != (ndx_node_t *)0, "CUSTOMER root: ndx_read_node(1)");
    if (rc != NDX_OK || !node) { ndx_close(idx); return; }

    /* entry_count = 5 (5 records in CUSTOMER.DBF).
     * [ndx.md ss3 Verification "CUSTOMER leaf: entry_count=5"] */
    snprintf(msg, sizeof(msg),
             "CUSTOMER root: entry_count==5 (got %u)", node->entry_count);
    CHECK(node->entry_count == 5u, msg);

    /* filler = 0x5543 (confirmed garbage).
     * [ndx.md ss3 Verification "filler=0x5543 garbage"] */
    snprintf(msg, sizeof(msg),
             "CUSTOMER root: filler==0x5543 (got 0x%04X)", node->filler);
    CHECK(node->filler == 0x5543u, msg);

    /* All 5 entries are leaf entries (child_page=0).
     * [ndx.md ss3.1 "0 => this entry is in a LEAF"] */
    for (i = 0u; i < node->entry_count; i++) {
        snprintf(msg, sizeof(msg),
                 "CUSTOMER entry[%u].child_page==0 (leaf)", i);
        CHECK(node->entries[i].child_page == 0u, msg);
    }

    /* rec1 (dbf_recno=1) key prefix: "ATL" + "LOUIS".
     * [ndx.md Verification "CUSTOMER rec1 == CDES_CITY 'ATL'+CNAME 'LOUIS' (28)"]
     * Find the entry with dbf_recno==1. */
    for (i = 0u; i < node->entry_count; i++) {
        if (node->entries[i].dbf_recno == 1u) {
            CHECK(node->entries[i].key_data[0] == 'A',
                  "CUSTOMER rec1 key[0]=='A'");
            CHECK(node->entries[i].key_data[1] == 'T',
                  "CUSTOMER rec1 key[1]=='T'");
            CHECK(node->entries[i].key_data[2] == 'L',
                  "CUSTOMER rec1 key[2]=='L'");
            CHECK(node->entries[i].key_data[3] == 'L',
                  "CUSTOMER rec1 key[3]=='L'  (CNAME starts 'LOUIS')");
            CHECK(node->entries[i].key_data[4] == 'O',
                  "CUSTOMER rec1 key[4]=='O'");
            CHECK(node->entries[i].key_data[5] == 'U',
                  "CUSTOMER rec1 key[5]=='U'");
            CHECK(node->entries[i].key_data[6] == 'I',
                  "CUSTOMER rec1 key[6]=='I'");
            CHECK(node->entries[i].key_data[7] == 'S',
                  "CUSTOMER rec1 key[7]=='S'");
            break;
        }
    }

    ndx_node_free(idx, node);
    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * check_aval_flt_node: verify AVAL_FLT.NDX (KL=14, grp=24, dummy=0x0010).
 *
 * AVAL_FLT.NDX: KL=14 (ADEP 3 + ADES 3 + DTOC(ADATE) 8), grp=24, KPP=21.
 * [ndx.md Verification "KL 14->24/21"; dummy=0x0010 is known garbage]
 * entry key_expr="adep_city+ades_city+dtoc(adate) " (lowercase; verbatim).
 * [ndx.md Verification casing: "AVAL_FLT 'adep_city+ades_city+dtoc(adate) ' (lower)
 *  ... bytes simply echo the source text"]
 * ----------------------------------------------------------------------- */
static void check_aval_flt(samir_pal_t *pal, const char *base)
{
    char       path[1024];
    ndx_index *idx = (ndx_index *)0;
    int        rc;
    char       msg[256];

    join(path, sizeof(path), base, PRISTINE_REL "/AVAL_FLT.NDX");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): AVAL_FLT.NDX absent\n");
        return;
    }

    rc = ndx_open(pal, path, &idx);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0, "AVAL_FLT: ndx_open");
    if (rc != NDX_OK || !idx) return;

    snprintf(msg, sizeof(msg),
             "AVAL_FLT: key_length==14 (got %u)", ndx_key_length(idx));
    CHECK(ndx_key_length(idx) == 14u, msg);

    snprintf(msg, sizeof(msg),
             "AVAL_FLT: group_length==24 (got %u)", ndx_group_length(idx));
    CHECK(ndx_group_length(idx) == 24u, msg);

    snprintf(msg, sizeof(msg),
             "AVAL_FLT: keys_per_page==21 (got %u)", ndx_keys_per_page(idx));
    CHECK(ndx_keys_per_page(idx) == 21u, msg);

    /* dummy == 0x0010 (known garbage value -- volatile but byte-verified).
     * [ndx.md ss2 "dummy" / Verification "AVAL_FLT=0x0010"]
     * We assert the exact value because the Verification section nailed it. */
    snprintf(msg, sizeof(msg),
             "AVAL_FLT: dummy==0x0010 (got 0x%04X)", ndx_dummy(idx));
    CHECK(ndx_dummy(idx) == 0x0010u, msg);

    /* key_expr lowercase verbatim.
     * [ndx.md Verification "adep_city+ades_city+dtoc(adate) " (lower)] */
    CHECK(strcmp(ndx_key_expr(idx), "adep_city+ades_city+dtoc(adate) ") == 0,
          "AVAL_FLT: key_expr lowercase verbatim");

    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * check_trips_node: verify TRIPS.NDX root branch (3 separators + trailing child).
 *
 * TRIPS: root=5, total=6, KL=24 (TRAVELCODE 4 + LASTNAME 20), KPP=15, grp=32.
 * [ndx.md Verification "TRIPS root count=3 separators (children 1,2,3) + trailing 4"]
 * root branch entry_count=3, trail_child=4.
 * ----------------------------------------------------------------------- */
static void check_trips_node(samir_pal_t *pal, const char *base)
{
    char        path[1024];
    ndx_index  *idx  = (ndx_index *)0;
    ndx_node_t *node = (ndx_node_t *)0;
    int         rc;
    char        msg[256];

    join(path, sizeof(path), base, PRISTINE_REL "/TRIPS.NDX");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): TRIPS.NDX absent for node check\n");
        return;
    }

    rc = ndx_open(pal, path, &idx);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0, "TRIPS node: ndx_open");
    if (rc != NDX_OK || !idx) return;

    /* root_page = 5 [verified by direct python byte-check above]. */
    snprintf(msg, sizeof(msg),
             "TRIPS: root_page==5 (got %u)", ndx_root_page(idx));
    CHECK(ndx_root_page(idx) == 5u, msg);
    CHECK(ndx_key_length(idx) == 24u, "TRIPS: key_length==24");
    CHECK(ndx_group_length(idx) == 32u, "TRIPS: group_length==32");
    CHECK(ndx_keys_per_page(idx) == 15u, "TRIPS: keys_per_page==15");
    CHECK(strcmp(ndx_key_expr(idx), "TRAVELCODE + LASTNAME ") == 0,
          "TRIPS: key_expr==TRAVELCODE + LASTNAME ");

    rc = ndx_read_node(idx, 5u, &node);
    CHECK(rc == NDX_OK && node != (ndx_node_t *)0, "TRIPS root: ndx_read_node(5)");
    if (rc != NDX_OK || !node) { ndx_close(idx); return; }

    /* entry_count=3: 3 separator keys for 4 children.
     * [ndx.md Verification "TRIPS root 3 separators (children 1,2,3) + trailing 4"]
     * ndx.md ss5 "branch separator = HIGH key of subtree" */
    snprintf(msg, sizeof(msg),
             "TRIPS root: entry_count==3 (got %u)", node->entry_count);
    CHECK(node->entry_count == 3u, msg);

    /* All 3 are branch entries (child_page != 0).
     * [ndx.md Verification: children 1, 2, 3] */
    if (node->entry_count >= 3u) {
        CHECK(node->entries[0].child_page == 1u,
              "TRIPS root entry[0].child_page==1");
        CHECK(node->entries[1].child_page == 2u,
              "TRIPS root entry[1].child_page==2");
        CHECK(node->entries[2].child_page == 3u,
              "TRIPS root entry[2].child_page==3");
        /* Branch entries have dbf_recno=0.
         * [ndx.md ss3.1 "Meaningful in LEAF entries; 0 (ignored) in branch entries"] */
        CHECK(node->entries[0].dbf_recno == 0u,
              "TRIPS root entry[0].dbf_recno==0 (branch)");
    }

    /* Trailing child = 4 (rightmost subtree).
     * [ndx.md Verification "TRIPS root trailing child 4"] */
    snprintf(msg, sizeof(msg),
             "TRIPS root: trail_child==4 (got %u)", node->trail_child);
    CHECK(node->trail_child == 4u, msg);

    ndx_node_free(idx, node);
    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * check_remaining_fixtures: open and basic-validate the remaining 6 goldens.
 * Just ndx_open + reserved==0 + formula check. Loud-skip if absent.
 * ----------------------------------------------------------------------- */
typedef struct {
    const char *relpath;
    const char *label;
    uint16_t    key_length;
    uint16_t    group_length;
    uint16_t    keys_per_page;
} ndx_basic;

static const ndx_basic REMAINING[] = {
    /* ndx.md Verification "KL 2->grp 12/kpp 42 (LOCATION)" */
    { PRISTINE_REL "/LOCATION.NDX", "LOCATION", 2,  12, 42 },
    /* ndx.md Verification "KL 3->12/42 (FLT_NO)" */
    { PRISTINE_REL "/FLT_NO.NDX",   "FLT_NO",   3,  12, 42 },
    /* NAMES.NDX: header identical to CNAMES per Verification ("byte-identical"). */
    { PRISTINE_REL "/NAMES.NDX",    "NAMES",    40, 48, 10 },
    /* TNAMES.NDX: same as NAMES (Verification "NAMES.NDX and TNAMES.NDX ... byte-identical"). */
    { PRISTINE_REL "/TNAMES.NDX",   "TNAMES",   40, 48, 10 },
};
#define REMAINING_N ((int)(sizeof(REMAINING) / sizeof(REMAINING[0])))

static void check_remaining(samir_pal_t *pal, const char *base)
{
    int i;
    for (i = 0; i < REMAINING_N; i++) {
        char       path[1024];
        ndx_index *idx = (ndx_index *)0;
        int        rc;
        char       msg[256];

        join(path, sizeof(path), base, REMAINING[i].relpath);
        if (!file_exists(path)) {
            fprintf(stderr, "  SKIP (LOUD): %s absent\n", REMAINING[i].label);
            continue;
        }

        rc = ndx_open(pal, path, &idx);
        snprintf(msg, sizeof(msg), "%s: ndx_open (rc=%d)", REMAINING[i].label, rc);
        CHECK(rc == NDX_OK && idx != (ndx_index *)0, msg);
        if (rc != NDX_OK || !idx) continue;

        /* reserved == 0 for all III+ fixtures [ndx.md Verification]. */
        CHECK(ndx_reserved(idx) == 0u, "reserved==0 (III+ fixture)");

        snprintf(msg, sizeof(msg),
                 "%s: key_length==%u (got %u)",
                 REMAINING[i].label, REMAINING[i].key_length,
                 ndx_key_length(idx));
        CHECK(ndx_key_length(idx) == REMAINING[i].key_length, msg);

        snprintf(msg, sizeof(msg),
                 "%s: group_length==%u (got %u)",
                 REMAINING[i].label, REMAINING[i].group_length,
                 ndx_group_length(idx));
        CHECK(ndx_group_length(idx) == REMAINING[i].group_length, msg);

        snprintf(msg, sizeof(msg),
                 "%s: keys_per_page==%u (got %u)",
                 REMAINING[i].label, REMAINING[i].keys_per_page,
                 ndx_keys_per_page(idx));
        CHECK(ndx_keys_per_page(idx) == REMAINING[i].keys_per_page, msg);

        ndx_close(idx);
    }
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    const char *base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    struct pal_host_cfg cfg;
    samir_pal_t *pal;
    int i;
    int any_present = 0;
    char path[1024];

    /* Injected fixed date (Rule 11 -- today() is not exercised here). */
    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy  = 99;
    cfg.date_mm  = 12;
    cfg.date_dd  = 31;
    /* Arena: one ndx_index is ~200 bytes; a node with KPP=31 entries and
     * KL=40 is ~200 + 31*16 + 31*40 bytes ~ 1.7 KB. A few goldens opened
     * serially (close before open) so 64 KB is ample. */
    cfg.heap_size = 64u * 1024u;

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }

    /* ---- Tier 0 + Tier 1 header checks ---- */
    for (i = 0; i < MANIFEST_N; i++) {
        if (file_exists(join(path, sizeof(path), base, MANIFEST[i].relpath)))
            any_present = 1;
        check_header(pal, base, &MANIFEST[i]);
    }

    if (!any_present) {
        fprintf(stderr,
                "  SKIP (LOUD): no goldens found under base '%s'\n"
                "               pass the corpus base as argv[1]\n"
                "               (node checks below will also loud-skip)\n",
                base);
    }

    /* ---- Node structure checks (Tier-1 + mutation target) ---- */
    check_cnames_node(pal, base);
    check_tourdate_node(pal, base);   /* primary mutant detector */
    check_customer_node(pal, base);
    check_aval_flt(pal, base);
    check_trips_node(pal, base);
    check_remaining(pal, base);

    pal_host_free(pal);
    return TEST_SUMMARY("test-ndx-parse");
}
