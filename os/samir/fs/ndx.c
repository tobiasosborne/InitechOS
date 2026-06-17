/*
 * os/samir/fs/ndx.c -- SAMIR (InitechBase) .ndx B-tree index codec.
 *                       Steps S4.1 (header + node parse), S4.2 (key decode +
 *                       collation), S4.3 (B-tree traverse + SEEK), and S4.4
 *                       (bulk INDEX ON build).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib, CDR-0001). Includes ONLY <stdint.h> plus engine headers and the
 * LOCKED spec/samir/ndx_format.h. No libc, no int 0x21. All OS contact is
 * through the PAL vtable (plan Sec 2.B/2.D).
 *
 * dBASE III PLUS 1.1 ONLY (plan Sec 2.C): .ndx single-file B-tree. .mdx/.ntx
 * are out of scope for M6.
 *
 * S4.1 scope:
 *   ndx_open  -- open the file (keeps fd open), validate all 10 header fields,
 *                copy key_expr verbatim into arena.
 *   ndx_close -- close the fd, release arena memory.
 *   Header accessors -- return the 10 parsed header fields.
 *   ndx_read_node -- seek to page PAGE_NO, read 512 bytes, parse the 2+2 node
 *                    header + per-group {child_page, dbf_recno, key_data} array.
 *   ndx_node_free -- unwind the PAL arena to the mark saved before this node.
 *
 * S4.2 scope (key decode + collation):
 *   ndx_key_decode -- decode raw key_data bytes to xb_val (char -> XB_C into
 *                     caller buffer; type-1 -> XB_N from raw LE double).
 *   ndx_key_cmp   -- compare two raw key_data arrays for B-tree order:
 *                     char = unsigned byte compare; type-1 = arithmetic double
 *                     compare (NOT memcmp). [re/mint-results-001.md VERIFIED]
 *
 * S4.3 scope (B-tree traverse + SEEK):
 *   ndx_inorder -- in-order traversal of the whole tree, invoking a callback
 *                  per leaf entry in ascending collation order (ndx.md ss8 step
 *                  3). The spine S4.4 (INDEX ON build verification) and S5
 *                  (index-ordered nav) reuse.
 *   ndx_seek    -- descend the B-tree to the first key >= target (ndx.md ss5),
 *                  resolve the landing recno, and set FOUND per SET EXACT
 *                  (char OFF = directional begins-with per mint-002; char ON =
 *                  full-length equal; numeric/date = exact arithmetic equal).
 *                  EOF when the target exceeds every key.
 *
 * S4.4 scope (bulk INDEX ON build):
 *   ndx_build  -- build a fresh .ndx that reproduces dBASE's bulk INDEX ON /
 *                 REINDEX output byte-for-byte (meaningful bytes): collect one
 *                 key per record via a caller key-provider callback (NO eval.c
 *                 dependency -- the decoupling barrier of bead initech-ahu.4),
 *                 STABLE-sort by ndx_key_cmp (ties keep ascending recno), PACK
 *                 LEAVES 100% left-to-right with the remainder in the last leaf
 *                 (NOT 50/50), then build the root branch over the leaves' HIGH
 *                 keys (N keys / N+1 children). [VERIFIED minted BIGIDX.NDX +
 *                 NCOST.NDX; re/mint-results-003.md]
 *
 * S4.4 mutation hook (Rule 6 / plan S4.4 "50/50 split"):
 *   Build with -DNDX_MUTATE_SPLIT_5050. Leaves are packed ~50/50 instead of
 *   100%-then-remainder; the leaf fill levels (and therefore the page bytes +
 *   the root separator set) diverge from the golden -> the byte-exact rebuild
 *   oracle goes RED. [re/mint-results-003.md: the bulk build packs to 100%,
 *   never 50/50.]
 *
 * S4.3 mutation hook (Rule 6 / plan S4.3 "wrong child descent"):
 *   Build with -DNDX_MUTATE_SEEK_CHILD. The branch descent follows the WRONG
 *   child: it descends into entry[i+1]'s child (off-by-one) instead of the
 *   first separator entry whose key >= target. Per ndx.md ss5 the separator at
 *   entry i is the HIGH key of subtree child_page[i], so following child[i+1]
 *   skips the subtree that actually contains the target -> SEEK resolves the
 *   wrong recno and the in-order traversal (which shares the descent stepper)
 *   loses/duplicates leaves, so the sortedness + completeness + SEEK-recno
 *   oracle checks go RED. [ndx.md ss5 "descend into THAT entry's child_page";
 *   the mutation descends into the next entry's child.]
 *
 * S4.1 mutation hook (Rule 6 / plan S4.1 "clicketyclick wrong 18-23 sublayout"):
 *   Build with -DNDX_MUTATE_SUBLAYOUT. The perturbed read swaps the child_page
 *   and dbf_recno field offsets within each group AND shifts the key_data start
 *   by +4 bytes -- modeling the wrong clicketyclick header diagram that places
 *   "Size of key record" as a 4-byte long at bytes 18-21 and unique as a single
 *   byte at 23. Under the mutation every parsed child/recno/key mismatches the
 *   golden -> the oracle goes RED. [ndx.md Open questions: "clicketyclick's
 *   diagram puts 'Size of key record' as a 4-byte long at bytes 18-21 ... The
 *   real bytes contradict it; the bytes win."]
 *
 * S4.2 mutation hook (Rule 6 / plan S4.2 "sign-flip that mint-001 disproved"):
 *   Build with -DNDX_MUTATE_KEY_SIGNFLIP. Applies a sign-flip transform (XOR
 *   bit 7 of the highest byte) to numeric keys in ndx_key_cmp, implementing
 *   the hypothesis that re/mint-results-001.md was specifically minted to
 *   disprove. Under this mutation the numeric ordering assertions go RED because
 *   sign-flipping inverts the comparison for negative values. Cite:
 *   [re/mint-results-001.md "corrects the prior ndx.md speculation about a
 *   sign-flip transform -- there is none." VERIFIED minted NCOST.NDX 2026-06-16]
 *
 * Fail loud (CLAUDE.md Rule 2): every structural violation returns a negative
 * ndx_err. No half-open state is left on error.
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11): no timestamps, no wall-clock.
 *
 * Ref (Law 1 -- cited per decision in comments):
 *   - spec/samir/ndx_format.h     LOCKED byte-offset constants (used for every
 *                                  offset access; no hardcoded numbers).
 *   - ../dbase3-decomp/specs/file-formats/ndx.md  ss1 (page geometry),
 *       ss2 (header 10 fields + casing RESOLVED), ss2.2 (CNAMES byte dump),
 *       ss3 (node 2+2 header), ss3.1 (group layout), ss3.2 (trailing child),
 *       ss4 (key encoding), ss4.1 (char: space-padded CP437), ss4.2 (numeric:
 *       raw LE double, NO sign-flip, arithmetic compare), ss5 (B-tree ordering
 *       invariant + search algorithm + HIGH-key separator), ss6 (collation),
 *       ss8 (implementation checklist).
 *   - ../dbase3-decomp/re/mint-results-002.md  SET EXACT default OFF; char "="
 *       is directional begins-with (LEFT begins with RIGHT): "ab"="a"->.T.,
 *       "a"="ab"->.F. The S4.3 SEEK FOUND rule for char keys.
 *   - ../dbase3-decomp/re/mint-results-001.md  THE authority for numeric key
 *       encoding (raw LE double, NO sign-flip, arithmetic compare). Table of
 *       verified bytes: -123.45 cd cc cc cc cc dc 5e c0; -1.0 00..f0 bf;
 *       0.0 all zeros; 279.0 00..70 71 40. [VERIFIED minted NCOST.NDX 2026-06-16]
 *   - docs/plans/SAMIR-implementation-plan.md S4.1/S4.2 contract + Sec 3.3.
 *   - os/samir/include/samir/pal.h (byte I/O vtable).
 *   - os/samir/include/samir/rt.h  (rt_memcpy, rt_memset).
 *   - os/samir/include/samir/value.h (xb_val, XB_C, XB_N, xb_c, xb_n).
 */

#include <stdint.h>
#include "samir/pal.h"
#include "samir/rt.h"
#include "samir/value.h"
#include "samir/ndx.h"
#include "samir/ndx_format.h"

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

/*
 * ceil4(n) = ((n + 3) / 4) * 4.
 * Ref: ndx.md ss2 group_length formula; spec/samir/ndx_format.h "Derived
 * formulas" section.
 */
static uint32_t ceil4(uint32_t n)
{
    return ((n + 3u) / 4u) * 4u;
}

/* -----------------------------------------------------------------------
 * Concrete ndx_index struct
 * (opaque to callers; ndx.h only forward-declares it)
 * ----------------------------------------------------------------------- */

struct ndx_index {
    samir_pal_t *pal;

    /* Open file descriptor -- kept alive for ndx_read_node page seeks.
     * ndx_close() closes it. Matching dBASE's own behaviour: one handle
     * per open index. Ref: pal.h PAL_RD open mode. */
    pal_fd fd;

    /* --- 10 header fields (ndx.md ss2 / spec/samir/ndx_format.h) --- */
    uint32_t root_page;       /* NDX_HDR_ROOT_PAGE_OFF    0x00 */
    uint32_t total_pages;     /* NDX_HDR_TOTAL_PAGES_OFF  0x04 */
    uint32_t reserved;        /* NDX_HDR_RESERVED_OFF     0x08 */
    uint16_t key_length;      /* NDX_HDR_KEY_LENGTH_OFF   0x0C */
    uint16_t keys_per_page;   /* NDX_HDR_KEYS_PER_PAGE_OFF 0x0E */
    uint16_t key_type;        /* NDX_HDR_KEY_TYPE_OFF     0x10 */
    uint16_t group_length;    /* NDX_HDR_GROUP_LENGTH_OFF 0x12 */
    uint16_t dummy;           /* NDX_HDR_DUMMY_OFF        0x14 */
    uint16_t unique_flag;     /* NDX_HDR_UNIQUE_FLAG_OFF  0x16 */
    /* key_expr: verbatim, NUL-terminated, cap 100 bytes.
     * Ref: ndx.md ss2.1 casing RESOLVED: NOT lowercased. +1 for safety NUL. */
    char     key_expr[NDX_HDR_KEY_EXPR_SIZE + 1];

    /* Arena management. */
    void    *arena_mark;      /* mark before this struct; ndx_close resets here */
    void    *last_node_mark;  /* mark before last ndx_read_node alloc; freed by
                               * ndx_node_free. Only valid while one node is live. */
};

/* -----------------------------------------------------------------------
 * PAL I/O helpers
 * ----------------------------------------------------------------------- */

/*
 * read_exact: read exactly N bytes from FD.
 * Returns NDX_OK or -NDX_ERR_IO (fail loud on short read -- Rule 2).
 */
static int read_exact(samir_pal_t *pal, pal_fd fd, void *buf, uint32_t n)
{
    int32_t got = pal->read(pal, fd, buf, n);
    if (got < 0 || (uint32_t)got != n)
        return -NDX_ERR_IO;
    return NDX_OK;
}

/*
 * seek_to: seek FD to byte offset OFF from start of file.
 * Returns NDX_OK or -NDX_ERR_IO.
 */
static int seek_to(samir_pal_t *pal, pal_fd fd, uint32_t off)
{
    int32_t pos = pal->seek(pal, fd, (int32_t)off, PAL_SEEK_SET);
    if (pos < 0 || (uint32_t)pos != off)
        return -NDX_ERR_IO;
    return NDX_OK;
}

/* -----------------------------------------------------------------------
 * LE integer decoders (no aliasing cast; freestanding safe)
 * ----------------------------------------------------------------------- */

static uint32_t u32le(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] <<  8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static uint16_t u16le(const uint8_t *p)
{
    return (uint16_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8));
}

/* -----------------------------------------------------------------------
 * ndx_open
 * ----------------------------------------------------------------------- */

/*
 * ndx_open: open and parse the .ndx header page.
 *
 * Validation order (Rule 2 -- fail loud at the first violation):
 *   1. PAL open (PAL_RD) succeeds; fd kept open for ndx_read_node.
 *   2. File size = seek(fd,0,PAL_SEEK_END) >= 512 and a multiple of 512.
 *      [ndx.md ss1: "all fixture file sizes are multiples of 512";
 *       pal.h: "seek(fd,0,PAL_SEEK_END) IS the file-size primitive"]
 *   3. Seek to 0; read 512-byte header page into stack buffer.
 *      [ndx.md ss1 "Page 0 = header"; spec/samir/ndx_format.h NDX_HEADER_PAGE=0]
 *   4. Parse all 10 fields via NDX_HDR_* LOCKED offsets.
 *      [spec/samir/ndx_format.h; ndx.md ss2 table + ss2.2 CNAMES byte dump]
 *   5. key_type in {NDX_KEY_TYPE_CHAR=0, NDX_KEY_TYPE_NUMERIC=1}.
 *      [ndx.md ss2; spec/samir/ndx_format.h]
 *   6. group_length == ceil4(key_length + NDX_GRP_OVERHEAD).
 *      NDX_GRP_OVERHEAD = 8 = sizeof(child_page) + sizeof(dbf_recno).
 *      [ndx.md ss2 formula verified all 11 fixtures: KL5->16, KL8->16,
 *       KL14->24, KL28->36, KL40->48; Verification section]
 *   7. keys_per_page == (NDX_PAGE_SIZE - NDX_NODE_HDR_SIZE) / group_length.
 *      [ndx.md ss2 "MAX keys per node = floor((512-4)/group_length)";
 *       verified all 11: KPP10, KPP21, KPP14, KPP31 etc.]
 *   8. total_pages == filesize / NDX_PAGE_SIZE.
 *      [ndx.md ss1 "Total pages always equals filesize/512"; all 11 verified]
 *   9. root_page in [1, total_pages). Page 0 is always the header.
 *      [ndx.md ss1; byte-checked: CNAMES root=6, TOURDATE root=1]
 *  10. key_expr: NUL-terminated at NDX_HDR_KEY_EXPR_OFF(0x18), cap 100 bytes,
 *      verbatim (NOT lowercased).
 *      [ndx.md ss2.1 RESOLVED: "stored verbatim AS TYPED, NOT lowercased";
 *       byte-check: CNAMES "LASTNAME + FIRSTNAME " upper, CHKNO "Chkno " mixed,
 *       AVAL_FLT "adep_city+ades_city+dtoc(adate) " lower -- source text echoed]
 *
 * On success: *out is set, NDX_OK returned.
 * On failure: fd closed (if open), *out == NULL, negative error returned.
 */
int ndx_open(samir_pal_t *pal, const char *name, ndx_index **out)
{
    pal_fd    fd;
    int32_t   fsize_raw;
    uint32_t  fsize;
    int       rc;
    uint8_t   page0[NDX_PAGE_SIZE];
    ndx_index *idx;
    void      *mark;
    uint32_t   expr_i;

    *out = (ndx_index *)0;

    /* Step 1: open. */
    fd = pal->open(pal, name, PAL_RD);
    if (fd < 0)
        return -NDX_ERR_IO;

    /* Step 2: file size. */
    fsize_raw = pal->seek(pal, fd, 0, PAL_SEEK_END);
    if (fsize_raw <= 0) {
        pal->close(pal, fd);
        return -NDX_ERR_IO;
    }
    fsize = (uint32_t)fsize_raw;
    if (fsize < (uint32_t)NDX_PAGE_SIZE
        || (fsize % (uint32_t)NDX_PAGE_SIZE) != 0u) {
        pal->close(pal, fd);
        return -NDX_ERR_BAD_SIZE;
    }

    /* Step 3: read header page. */
    rc = seek_to(pal, fd, 0u);
    if (rc != NDX_OK) { pal->close(pal, fd); return rc; }
    rc = read_exact(pal, fd, page0, (uint32_t)NDX_PAGE_SIZE);
    if (rc != NDX_OK) { pal->close(pal, fd); return rc; }

    /* Steps 4-9: parse + validate. */
    {
        uint32_t root_page    = u32le(page0 + NDX_HDR_ROOT_PAGE_OFF);
        uint32_t total_pages  = u32le(page0 + NDX_HDR_TOTAL_PAGES_OFF);
        uint32_t reserved     = u32le(page0 + NDX_HDR_RESERVED_OFF);
        uint16_t key_length   = u16le(page0 + NDX_HDR_KEY_LENGTH_OFF);
        uint16_t keys_per_page= u16le(page0 + NDX_HDR_KEYS_PER_PAGE_OFF);
        uint16_t key_type     = u16le(page0 + NDX_HDR_KEY_TYPE_OFF);
        uint16_t group_length = u16le(page0 + NDX_HDR_GROUP_LENGTH_OFF);
        uint16_t dummy        = u16le(page0 + NDX_HDR_DUMMY_OFF);
        uint16_t unique_flag  = u16le(page0 + NDX_HDR_UNIQUE_FLAG_OFF);
        uint32_t computed_grp;
        uint32_t computed_kpp;

        /* Step 5. */
        if (key_type != (uint16_t)NDX_KEY_TYPE_CHAR
            && key_type != (uint16_t)NDX_KEY_TYPE_NUMERIC) {
            pal->close(pal, fd);
            return -NDX_ERR_BAD_KEYTYPE;
        }

        /* Step 6: group_length == ceil4(key_length + 8).
         * NDX_GRP_OVERHEAD = 8 (child_page + dbf_recno).
         * Ref: spec/samir/ndx_format.h NDX_GRP_OVERHEAD. */
        computed_grp = ceil4((uint32_t)key_length + (uint32_t)NDX_GRP_OVERHEAD);
        if ((uint32_t)group_length != computed_grp) {
            pal->close(pal, fd);
            return -NDX_ERR_BAD_GROUP;
        }

        /* Step 7: keys_per_page == 508 / group_length.
         * Ref: spec/samir/ndx_format.h "keys_per_page = 508 / group_length". */
        computed_kpp = (group_length > 0u)
            ? ((uint32_t)(NDX_PAGE_SIZE - NDX_NODE_HDR_SIZE)
               / (uint32_t)group_length)
            : 0u;
        if ((uint32_t)keys_per_page != computed_kpp) {
            pal->close(pal, fd);
            return -NDX_ERR_BAD_KPP;
        }

        /* Step 8: total_pages == filesize / 512. */
        if (total_pages != fsize / (uint32_t)NDX_PAGE_SIZE) {
            pal->close(pal, fd);
            return -NDX_ERR_BAD_SIZE;
        }

        /* Step 9: root_page in [1, total_pages). */
        if (root_page == 0u || root_page >= total_pages) {
            pal->close(pal, fd);
            return -NDX_ERR_BAD_ROOT;
        }

        /* Allocate ndx_index from PAL arena.
         * Mark is saved BEFORE the alloc so ndx_close can free everything. */
        mark = pal->alloc(pal, 0u);   /* get current bump pointer as mark */
        idx  = (ndx_index *)pal->alloc(pal, (uint32_t)sizeof(ndx_index));
        if (!idx) {
            pal->close(pal, fd);
            pal->reset(pal, mark);
            return -NDX_ERR_OOM;
        }
        rt_memset(idx, 0, sizeof(ndx_index));

        idx->pal            = pal;
        idx->fd             = fd;
        idx->arena_mark     = mark;
        idx->last_node_mark = (void *)0;
        idx->root_page      = root_page;
        idx->total_pages    = total_pages;
        idx->reserved       = reserved;
        idx->key_length     = key_length;
        idx->keys_per_page  = keys_per_page;
        idx->key_type       = key_type;
        idx->group_length   = group_length;
        idx->dummy          = dummy;
        idx->unique_flag    = unique_flag;

        /* Step 10: key_expr verbatim.
         * NDX_HDR_KEY_EXPR_OFF = 0x18, NDX_HDR_KEY_EXPR_SIZE = 100. */
        for (expr_i = 0u; expr_i < (uint32_t)NDX_HDR_KEY_EXPR_SIZE; expr_i++) {
            uint8_t b = page0[NDX_HDR_KEY_EXPR_OFF + expr_i];
            if (b == 0u) break;
            idx->key_expr[expr_i] = (char)b;
        }
        idx->key_expr[expr_i] = '\0';
    }

    *out = idx;
    return NDX_OK;
}

/* -----------------------------------------------------------------------
 * ndx_close
 * ----------------------------------------------------------------------- */

int ndx_close(ndx_index *idx)
{
    if (!idx)
        return NDX_OK;
    idx->pal->close(idx->pal, idx->fd);
    idx->pal->reset(idx->pal, idx->arena_mark);
    return NDX_OK;
}

/* -----------------------------------------------------------------------
 * Header accessors
 * ----------------------------------------------------------------------- */

uint32_t ndx_root_page    (const ndx_index *idx) { return idx->root_page;     }
uint32_t ndx_total_pages  (const ndx_index *idx) { return idx->total_pages;   }
uint32_t ndx_reserved     (const ndx_index *idx) { return idx->reserved;      }
uint16_t ndx_key_length   (const ndx_index *idx) { return idx->key_length;    }
uint16_t ndx_keys_per_page(const ndx_index *idx) { return idx->keys_per_page; }
uint16_t ndx_key_type     (const ndx_index *idx) { return idx->key_type;      }
uint16_t ndx_group_length (const ndx_index *idx) { return idx->group_length;  }
uint16_t ndx_dummy        (const ndx_index *idx) { return idx->dummy;         }
uint16_t ndx_unique_flag  (const ndx_index *idx) { return idx->unique_flag;   }
const char *ndx_key_expr  (const ndx_index *idx) { return idx->key_expr;      }

/* -----------------------------------------------------------------------
 * ndx_read_node
 * ----------------------------------------------------------------------- */

/*
 * ndx_read_node: seek to page PAGE_NO and parse the 512-byte B-tree node.
 *
 * Node layout (ndx.md ss3):
 *   [0x00, 2 bytes] entry_count  uint16 LE  -- 2-byte live key count
 *   [0x02, 2 bytes] filler       uint16 LE  -- garbage (ignored)
 *   [0x04, ...]     entries[]               -- entry_count groups, group_length each
 *
 * Each group (ndx.md ss3.1 / spec/samir/ndx_format.h NDX_GRP_*):
 *   [NDX_GRP_CHILD_PAGE_OFF = 0x00, 4 bytes]  child_page uint32 LE  (0 = leaf)
 *   [NDX_GRP_DBF_RECNO_OFF  = 0x04, 4 bytes]  dbf_recno  uint32 LE  (0 in branches)
 *   [NDX_GRP_KEY_DATA_OFF   = 0x08, KL bytes] key_data   (raw bytes; S4.2 decodes)
 *   [alignment filler: 0..3 bytes, garbage]
 *
 * Trailing (N+1th) child pointer (ndx.md ss3.2):
 *   Slot at [NDX_NODE_ENTRIES_OFF + entry_count * group_length], first 4 bytes.
 *   Non-zero only in branch nodes (rightmost subtree). In leaf nodes, garbage.
 *   Guard: read only if the 4-byte child_page fits within the 512-byte page.
 *   [verified: CNAMES page1 full leaf (entry_count=10=kpp), trailing at byte 484,
 *    484+4=488<=512 -- safe. ndx.md ss3.2 overflow caveat applies when even the
 *    4-byte read would extend past 512 -- guard prevents that.]
 *
 * NDX_MUTATE_SUBLAYOUT (Rule 6):
 *   Swap child_page and dbf_recno read offsets, shift key_data start +4.
 *   [ndx.md Open questions: "clicketyclick's diagram puts 'Size of key record'
 *   as a 4-byte long at bytes 18-21 and unique as a single byte at 23 -- the
 *   real bytes contradict it"; this mutation implements that wrong layout]
 *   Under mutation every parsed child/recno/key mismatches golden -> RED.
 *
 * Allocates ndx_node_t + ndx_entry_t array + key_data copies from PAL arena.
 * The arena mark before alloc is stored in idx->last_node_mark for ndx_node_free.
 * Callers MUST call ndx_node_free before opening the next node.
 *
 * Ref: ndx.md ss3 (node header byte-check CUSTOMER count=5 filler=0x5543,
 *      CNAMES root count=4 filler=0); ss3.1 (group layout + AVAL_FLT 2-byte
 *      filler confirmed 0x0000); ss3.2 (trailing child + full-leaf overflow).
 *      spec/samir/ndx_format.h NDX_NODE_* and NDX_GRP_* constants.
 */
int ndx_read_node(ndx_index *idx, uint32_t page_no, ndx_node_t **node_out)
{
    uint8_t     page[NDX_PAGE_SIZE];
    int         rc;
    uint16_t    entry_count;
    uint16_t    filler;
    uint32_t    entries_end;
    ndx_node_t *node;
    uint8_t    *key_pool;
    uint32_t    node_struct_size;
    uint32_t    keys_total;
    uint32_t    trail_off;
    uint32_t    trail_child;
    uint32_t    i;
    void       *node_mark;

#ifndef NDX_MUTATE_SUBLAYOUT
    /* Correct sub-layout per ndx.md ss3.1 / spec/samir/ndx_format.h NDX_GRP_*:
     *   child_page at  NDX_GRP_CHILD_PAGE_OFF = 0x00
     *   dbf_recno  at  NDX_GRP_DBF_RECNO_OFF  = 0x04
     *   key_data   at  NDX_GRP_KEY_DATA_OFF   = 0x08  */
    const uint32_t child_off_in_grp = (uint32_t)NDX_GRP_CHILD_PAGE_OFF;
    const uint32_t recno_off_in_grp = (uint32_t)NDX_GRP_DBF_RECNO_OFF;
    const uint32_t key_off_in_grp   = (uint32_t)NDX_GRP_KEY_DATA_OFF;
#else
    /* MUTATED layout -- implements clicketyclick's wrong "Size of key record
     * as 4-byte long at bytes 18-21, unique at 23" sub-layout.
     * child_page read from the dbf_recno slot (+4), dbf_recno from +0,
     * key_data shifted to +12 instead of +8.
     * [ndx.md Open questions: "the real bytes contradict it; see Open questions.
     *  Recorded here so no one re-imports the bad layout."]
     * Under this mutation every parsed field mismatches -> oracle goes RED. */
    const uint32_t child_off_in_grp = (uint32_t)NDX_GRP_DBF_RECNO_OFF;    /* SWAPPED */
    const uint32_t recno_off_in_grp = (uint32_t)NDX_GRP_CHILD_PAGE_OFF;   /* SWAPPED */
    const uint32_t key_off_in_grp   = (uint32_t)NDX_GRP_KEY_DATA_OFF + 4u; /* SHIFTED */
#endif

    *node_out = (ndx_node_t *)0;

    /* Validate page_no: [1, total_pages).
     * Page 0 is always the header, not a node.
     * Ref: ndx.md ss1. */
    if (page_no == 0u || page_no >= idx->total_pages)
        return -NDX_ERR_BAD_PAGE;

    /* Seek to page_no * 512 and read the 512-byte page.
     * Ref: ndx.md ss1 "page P is at byte offset P * 512";
     *      spec/samir/ndx_format.h NDX_PAGE_SIZE = 512. */
    rc = seek_to(idx->pal, idx->fd, page_no * (uint32_t)NDX_PAGE_SIZE);
    if (rc != NDX_OK) return rc;
    rc = read_exact(idx->pal, idx->fd, page, (uint32_t)NDX_PAGE_SIZE);
    if (rc != NDX_OK) return rc;

    /* Parse the 4-byte node header.
     * Ref: ndx.md ss3 "entry_count is a 2-byte count at offset 0, with
     * 2 bytes of filler at offset 2 (NOT a 4-byte count)".
     * [verified: piclist "count (two bytes) ... and two bytes of padding";
     *  byte-check: CUSTOMER count=5, filler=0x5543; CNAMES root count=4, filler=0]
     * spec/samir/ndx_format.h NDX_NODE_ENTRY_COUNT_OFF=0, NDX_NODE_FILLER_OFF=2. */
    entry_count = u16le(page + NDX_NODE_ENTRY_COUNT_OFF);
    filler      = u16le(page + NDX_NODE_FILLER_OFF);

    /* Bounds check: all live entries must fit within the 512-byte page.
     * Ref: ndx.md ss3 impl checklist step 5 "Never read past offset 512".
     * NDX_NODE_ENTRIES_OFF = 4; group_length = ceil4(key_length+8). */
    entries_end = (uint32_t)NDX_NODE_ENTRIES_OFF
                + (uint32_t)entry_count * (uint32_t)idx->group_length;
    if (entries_end > (uint32_t)NDX_PAGE_SIZE)
        return -NDX_ERR_PAGE_OVF;

    /* Allocate ndx_node_t + entry array + key_data pool from PAL arena.
     * Save mark for ndx_node_free to unwind.
     * Layout in arena:
     *   [sizeof(ndx_node_t) base + (entry_count-1)*sizeof(ndx_entry_t) flex tail]
     *   [entry_count * key_length bytes: raw key_data copies]
     * entries[0] is embedded in ndx_node_t; entries[1..n-1] extend beyond it.
     * If entry_count==0 the node_struct_size stays sizeof(ndx_node_t). */
    node_mark = idx->pal->alloc(idx->pal, 0u);   /* current mark */

    node_struct_size = (uint32_t)sizeof(ndx_node_t);
    if (entry_count > 1u)
        node_struct_size += ((uint32_t)entry_count - 1u)
                          * (uint32_t)sizeof(ndx_entry_t);

    keys_total = (uint32_t)entry_count * (uint32_t)idx->key_length;

    node = (ndx_node_t *)idx->pal->alloc(idx->pal,
                                          node_struct_size + keys_total);
    if (!node) {
        idx->pal->reset(idx->pal, node_mark);
        return -NDX_ERR_OOM;
    }
    rt_memset(node, 0, node_struct_size + keys_total);

    /* key_pool follows the ndx_node_t tail in the arena block. */
    key_pool = (uint8_t *)node + node_struct_size;

    node->entry_count = entry_count;
    node->filler      = filler;

    /* Parse each live group entry.
     * Ref: ndx.md ss3.1 table; spec/samir/ndx_format.h NDX_GRP_* constants. */
    for (i = 0u; i < (uint32_t)entry_count; i++) {
        uint32_t base    = (uint32_t)NDX_NODE_ENTRIES_OFF
                         + i * (uint32_t)idx->group_length;
        uint32_t key_len = (uint32_t)idx->key_length;
        uint8_t *key_dst = key_pool + i * key_len;

        /* Sanity: key_data region must fit in the page (checked per entry under
         * mutation since key_off_in_grp is shifted by +4). */
        if (base + key_off_in_grp + key_len > (uint32_t)NDX_PAGE_SIZE) {
            idx->pal->reset(idx->pal, node_mark);
            return -NDX_ERR_PAGE_OVF;
        }

        node->entries[i].child_page = u32le(page + base + child_off_in_grp);
        node->entries[i].dbf_recno  = u32le(page + base + recno_off_in_grp);
        rt_memcpy(key_dst, page + base + key_off_in_grp, key_len);
        node->entries[i].key_data   = key_dst;
    }

    /* Trailing (N+1th) child pointer.
     * Ref: ndx.md ss3.2 "In a branch node this slot's first 4 bytes are the
     * rightmost child pointer... In a leaf node this trailing slot is unused."
     * [verified: CNAMES root trailing child=5; ZIPCODE trailing=2; TRIPS root
     *  3 separators + trailing child 4 -- byte-check Verification section]
     * Overflow guard: "when entry_count==keys_per_page the trailing slot may
     * not fully fit in 512 bytes; only the leading 4 bytes are guaranteed."
     * Read only if those 4 bytes are in-page.
     * NDX_GRP_CHILD_PAGE_OFF=0x00, NDX_GRP_CHILD_PAGE_SIZE=4. */
    trail_off = entries_end;
    trail_child = 0u;
    if (trail_off + (uint32_t)NDX_GRP_CHILD_PAGE_SIZE <= (uint32_t)NDX_PAGE_SIZE) {
        trail_child = u32le(page + trail_off + (uint32_t)NDX_GRP_CHILD_PAGE_OFF);
    }
    node->trail_child = trail_child;

    /* Store the mark for ndx_node_free. */
    idx->last_node_mark = node_mark;

    *node_out = node;
    return NDX_OK;
}

/* -----------------------------------------------------------------------
 * ndx_node_free
 * ----------------------------------------------------------------------- */

/*
 * ndx_node_free: release PAL arena memory for NODE.
 *
 * Resets the arena to idx->last_node_mark, unwinding the node allocation.
 * NODE (and all key_data pointers in it) is invalid after this call.
 * Must be called before the next ndx_read_node on the same IDX.
 */
void ndx_node_free(ndx_index *idx, ndx_node_t *node)
{
    (void)node;    /* opaque; all the info we need is in idx->last_node_mark */
    if (!idx || !idx->last_node_mark)
        return;
    idx->pal->reset(idx->pal, idx->last_node_mark);
    idx->last_node_mark = (void *)0;
}

/* -----------------------------------------------------------------------
 * S4.2: Key decode + collation
 * ----------------------------------------------------------------------- */

/*
 * read_le_double: decode 8 bytes at P as a little-endian IEEE-754 double.
 *
 * Uses rt_memcpy into a local double to avoid strict-aliasing UB (Rule 12;
 * freestanding discipline: no memcpy from libc). The compiler is free to
 * optimise this to a direct load on targets where alignment is not an issue.
 *
 * Ref: ndx.md ss4.2 "8-byte IEEE-754 double, little-endian";
 *      C11 6.5 (strict aliasing: only safe via memcpy or character access).
 */
static double read_le_double(const uint8_t *p)
{
    double v;
    rt_memcpy(&v, p, 8u);
    return v;
}

/*
 * ndx_key_decode: decode KEY_DATA bytes to a typed xb_val.
 *
 * char (key_type 0):
 *   Copies ndx_key_length(idx) bytes into BUF, produces XB_C with p=BUF.
 *   The caller owns BUF; xb_val is valid while BUF is live (value.h contract).
 *   [ndx.md ss4.1: space-padded ASCII/OEM CP437, left-justified.]
 *
 * numeric/date (key_type 1):
 *   Reads the 8-byte LE double via read_le_double (no aliasing UB), produces
 *   XB_N. BUF is unused (may be NULL).
 *   N-vs-D BOUNDARY: key_type==1 covers BOTH N and D fields; the .ndx has no
 *   separate D bit. Always returns XB_N. A caller that knows the field is a
 *   date column can reinterpret the double as a JDN and use xb_d(out->u.n).
 *   For collation (ndx_key_cmp) the N/D distinction is irrelevant.
 *   [ndx.md ss4.2 / re/mint-results-001.md / plan Sec 3.3]
 *
 * Returns NDX_OK or -NDX_ERR_BAD_KEYTYPE (fail loud: Rule 2).
 */
int ndx_key_decode(const ndx_index *idx, const uint8_t *key_data,
                   char *buf, xb_val *out)
{
    if (idx->key_type == (uint16_t)NDX_KEY_TYPE_CHAR) {
        /* char key: copy raw bytes (space-padded CP437) into caller buffer.
         * Ref: ndx.md ss4.1. */
        rt_memcpy(buf, key_data, (uint32_t)idx->key_length);
        *out = xb_c(buf, idx->key_length);
        return NDX_OK;
    }
    if (idx->key_type == (uint16_t)NDX_KEY_TYPE_NUMERIC) {
        /* numeric/date: raw LE double, NO sign-flip, stored as XB_N.
         * N-vs-D boundary: always XB_N here; see header for S4.2->S5 note.
         * Ref: ndx.md ss4.2; re/mint-results-001.md [VERIFIED minted NCOST.NDX
         *      2026-06-16: -123.45->cd cc cc cc cc dc 5e c0; key_length always 8
         *      for type-1: ndx.md ss4.2 "key_length==8, group_length==16".] */
        *out = xb_n(read_le_double(key_data));
        return NDX_OK;
    }
    /* key_type is always validated at ndx_open; reaching here indicates
     * memory corruption. Fail loud (Rule 2). */
    return -NDX_ERR_BAD_KEYTYPE;
}

/*
 * ndx_key_cmp: compare two raw key_data byte arrays for B-tree collation.
 *
 * char (key_type 0): unsigned byte compare over the full key_length.
 *   Matches plain memcmp on a platform where chars are unsigned (or where we
 *   compare byte-by-byte as uint8_t). We do the byte-by-byte loop explicitly
 *   to stay freestanding (no libc memcmp dependency in the artifact).
 *   [ndx.md ss6 "unsigned byte value (ASCII/OEM CP437), left-to-right".
 *    Verified: CNAMES DeBello(0x42) < Dean(0x61); full in-order traversal
 *    of CNAMES yields 49 leaf entries in sorted byte order. [ndx.md Verification]]
 *
 * numeric/date (key_type 1): decode both as LE doubles, compare arithmetically.
 *   MUST NOT use memcmp on raw bytes: IEEE doubles have the sign bit in the MSB
 *   of byte 7, making raw byte-compare wrong across the sign boundary (a large
 *   negative double has 0xC0 in byte 7; a positive double has 0x40; byte-compare
 *   would put the negative above the positive). The minted NCOST.NDX leaf shows
 *   -123.45 < -1 < 0 < 279 in true numeric order even though raw bytes don't sort
 *   that way. [re/mint-results-001.md "Key comparison is ARITHMETIC, not byte-wise,
 *   for key_type==1." / ndx.md ss4.2 VERIFIED minted NCOST.NDX 2026-06-16]
 *
 * Mutation hook -DNDX_MUTATE_KEY_SIGNFLIP (Rule 6):
 *   XOR bit 7 of byte 7 before decoding either double. This implements the
 *   sign-flip transform that mint-001 was specifically minted to disprove.
 *   Under this mutation the leaf ordering check goes RED for NCOST.NDX because
 *   the "flipped" compare produces the wrong order for negatives.
 *   [re/mint-results-001.md: "corrects the prior ndx.md speculation about a
 *   sign-flip transform -- there is none."]
 */
int ndx_key_cmp(const ndx_index *idx, const uint8_t *a, const uint8_t *b)
{
    uint32_t i;
    if (idx->key_type == (uint16_t)NDX_KEY_TYPE_CHAR) {
        /* Unsigned byte compare, left-to-right over key_length bytes.
         * Ref: ndx.md ss6 "unsigned byte value". */
        for (i = 0u; i < (uint32_t)idx->key_length; i++) {
            if (a[i] < b[i]) return -1;
            if (a[i] > b[i]) return  1;
        }
        return 0;
    }
    /* key_type == NDX_KEY_TYPE_NUMERIC (or any other validated value):
     * arithmetic double compare. key_length is always 8 for type-1.
     * [ndx.md ss4.2 "key_length==8"] */
    {
        double va, vb;

#ifndef NDX_MUTATE_KEY_SIGNFLIP
        /* Correct path: raw LE double, NO sign-flip.
         * [re/mint-results-001.md VERIFIED minted NCOST.NDX 2026-06-16] */
        va = read_le_double(a);
        vb = read_le_double(b);
#else
        /* MUTATED: apply sign-flip (XOR bit 7 of byte 7) before decoding.
         * This implements the hypothesis that mint-001 disproved. Under this
         * mutation: for negative doubles (byte 7 has MSB set, e.g. 0xC0),
         * XOR 0x80 clears the sign bit, turning -123.45 into a large positive.
         * The leaf ordering -123.45 < -1 < 0 < ... will appear wrong because
         * the mutated comparison puts negatives above positives -> RED.
         * [re/mint-results-001.md "corrects the prior ndx.md speculation about
         *  a sign-flip transform -- there is none."]
         * The XOR modifies only the comparison bytes; we do NOT modify key_data
         * in place (const pointer). Use local copies. */
        {
            uint8_t ma[8], mb[8];
            rt_memcpy(ma, a, 8u);
            rt_memcpy(mb, b, 8u);
            ma[7] ^= 0x80u;
            mb[7] ^= 0x80u;
            va = read_le_double(ma);
            vb = read_le_double(mb);
        }
#endif
        if (va < vb) return -1;
        if (va > vb) return  1;
        return 0;
    }
}

/* -----------------------------------------------------------------------
 * S4.3: B-tree traverse + SEEK
 * -----------------------------------------------------------------------
 *
 * The descent rule (ndx.md ss5, NOT guessed):
 *   "Branch separator = HIGH key of subtree": in a branch node, the key_data of
 *   entry i (for i < entry_count) is the LARGEST key in the subtree rooted at
 *   child_page[i]. The rightmost child (trailing slot, ss3.2) holds keys greater
 *   than the last separator.
 *   "Search (find key K): start at root_page. In each node, scan entries in
 *   order until you find an entry whose key >= K. If that node is internal
 *   (child_page != 0), descend into THAT entry's child_page (or the rightmost
 *   child if K exceeds all separators) and repeat. When you reach a leaf
 *   (child_page == 0), an exact key match returns dbf_recno."
 *
 * Both ndx_inorder and ndx_seek use the same shared node stepper so the S4.3
 * mutation (-DNDX_MUTATE_SEEK_CHILD wrong-child descent) bites BOTH the SEEK
 * recno resolution AND the in-order sortedness/completeness checks.
 * ----------------------------------------------------------------------- */

/* Sentinel: a branch entry with child_page == 0 is a structural contradiction.
 * A leaf entry has child_page == 0; a branch entry has child_page != 0. We use
 * "first entry has child_page != 0" to decide branch-vs-leaf at the node level,
 * which is what ndx.md ss3.1 specifies ("0 => this entry is in a LEAF"). */

/* -----------------------------------------------------------------------
 * ndx_inorder
 *
 * In-order traversal. Recursion depth is bounded by total_pages (a cycle would
 * otherwise loop forever -- Rule 2). DEPTH is the remaining recursion budget;
 * each descent decrements it, and reaching 0 is a cycle/over-deep failure.
 *
 * Ref: ndx.md ss8 step 3 (recurse child before emitting separator (branch);
 * emit (key_data, dbf_recno) at a leaf; after the last entry, if internal,
 * recurse into the trailing child pointer).
 * ----------------------------------------------------------------------- */
static int inorder_page(ndx_index *idx, uint32_t page_no, uint32_t depth,
                        ndx_visit_fn visit, void *ctx)
{
    ndx_node_t *node = (ndx_node_t *)0;
    int         rc;
    uint32_t    i;
    int         is_branch;

    /* Bounded descent (Rule 2): a corrupt tree could cycle. */
    if (depth == 0u)
        return -NDX_ERR_CYCLE;

    rc = ndx_read_node(idx, page_no, &node);
    if (rc != NDX_OK)
        return rc;

    /* A node is a branch iff its first live entry has a child page (ss3.1).
     * Empty nodes (entry_count == 0) are treated as empty leaves. */
    is_branch = (node->entry_count > 0u && node->entries[0].child_page != 0u);

    for (i = 0u; i < (uint32_t)node->entry_count; i++) {
        if (is_branch) {
#ifndef NDX_MUTATE_SEEK_CHILD
            /* Correct: recurse into THIS entry's child before the separator.
             * Ref: ndx.md ss8 step 3 / ss5 descent rule. */
            uint32_t child = node->entries[i].child_page;
#else
            /* MUTATED wrong-child descent: recurse into the NEXT entry's child
             * (off-by-one). For the last entry it folds onto the trailing child.
             * This skips/duplicates subtrees -> the in-order sequence is no
             * longer the sorted leaf order, and leaf counts go wrong. RED. */
            uint32_t child = (i + 1u < (uint32_t)node->entry_count)
                                 ? node->entries[i + 1u].child_page
                                 : node->trail_child;
#endif
            /* Free the current node before recursing: the arena is a bump stack
             * and the child allocation must sit above the freed mark. We re-read
             * the parent after the child subtree completes. */
            ndx_node_free(idx, node);
            node = (ndx_node_t *)0;

            rc = inorder_page(idx, child, depth - 1u, visit, ctx);
            if (rc != NDX_OK)
                return rc;   /* error (<0) or caller-abort (>0) propagates */

            rc = ndx_read_node(idx, page_no, &node);
            if (rc != NDX_OK)
                return rc;
        } else {
            /* Leaf entry: emit (key_data, dbf_recno).
             * Ref: ndx.md ss8 step 3. */
            rc = visit(ctx, node->entries[i].key_data,
                       node->entries[i].dbf_recno);
            if (rc != 0) {        /* caller aborts the traversal */
                ndx_node_free(idx, node);
                return rc;
            }
        }
    }

    /* After the last entry, if internal, recurse into the rightmost child.
     * Ref: ndx.md ss8 step 3 + ss3.2 (trailing child = keys > last separator).
     * Under -DNDX_MUTATE_SEEK_CHILD the wrong-child loop already consumed the
     * trailing child as entry[last]'s "next", so emitting it again would be
     * wrong -- but the mutation's damage is already visible in the entry loop;
     * we keep the trailing recursion correct so the mutation's signature is the
     * skewed entry-loop descent, not a double-free. */
    if (is_branch && node && node->trail_child != 0u) {
        uint32_t trail = node->trail_child;
        ndx_node_free(idx, node);
        node = (ndx_node_t *)0;
        return inorder_page(idx, trail, depth - 1u, visit, ctx);
    }

    if (node)
        ndx_node_free(idx, node);
    return NDX_OK;
}

int ndx_inorder(ndx_index *idx, ndx_visit_fn visit, void *ctx)
{
    if (!idx || !visit)
        return -NDX_ERR_BAD_PAGE;
    /* Depth budget = total_pages: a well-formed tree never descends deeper than
     * the number of pages, so exceeding it means a cycle (Rule 2). */
    return inorder_page(idx, idx->root_page, idx->total_pages, visit, ctx);
}

/* -----------------------------------------------------------------------
 * SEEK: build the search key bytes from an xb_val
 *
 * The on-disk key_data layout is fixed (ndx.md ss4):
 *   char (key_type 0): key_length bytes, left-justified, space-padded.
 *   numeric/date (key_type 1): 8-byte LE IEEE-754 double.
 * The caller's xb_val is encoded into a fixed scratch buffer in this layout,
 * then compared against stored keys with ndx_key_cmp (which already implements
 * the correct collation per key_type).
 *
 * For char SEEK we also track the "significant length" of the search key (its
 * non-space-padded length) so the OFF/begins-with rule can compare only that
 * prefix of the stored key (mint-002 directional begins-with).
 * ----------------------------------------------------------------------- */

/* SEEK scratch buffer: char keys can be up to key_length (validated <= 100 by
 * the expression cap, but key_length itself is uint16). 256 covers every III+
 * fixture key_length (max observed 40) with ample margin; we bound-check. */
#define NDX_SEEK_KEYBUF 256

/*
 * build_search_key: render KEY into BUF (NDX_SEEK_KEYBUF bytes) in on-disk
 * layout for IDX. On success returns NDX_OK and (for char keys) writes the
 * significant length (trailing-space-trimmed) of the search key to *sig_len.
 *
 * Type compatibility (Rule 2 fail-loud):
 *   key_type 0 (char): KEY must be XB_C or XB_M. Shorter than key_length is
 *     right-padded with spaces; longer is truncated to key_length.
 *   key_type 1 (num/date): KEY must be XB_N or XB_D. The double is stored LE.
 * Any other combination returns -NDX_ERR_BAD_KEYTYPE.
 */
static int build_search_key(const ndx_index *idx, const xb_val *key,
                            uint8_t *buf, uint32_t *sig_len)
{
    uint32_t kl = (uint32_t)idx->key_length;

    if (kl == 0u || kl > (uint32_t)NDX_SEEK_KEYBUF)
        return -NDX_ERR_BAD_GROUP;   /* nonsensical key width */

    if (idx->key_type == (uint16_t)NDX_KEY_TYPE_CHAR) {
        const char *p;
        uint32_t    slen;
        uint32_t    n;
        uint32_t    i;

        if (key->t != XB_C && key->t != XB_M)
            return -NDX_ERR_BAD_KEYTYPE;

        p    = key->u.c.p;
        slen = (uint32_t)key->u.c.len;

        /* Copy up to key_length bytes; right-pad the rest with spaces (0x20),
         * matching the on-disk left-justified space-padded layout (ss4.1). */
        n = (slen < kl) ? slen : kl;
        for (i = 0u; i < n; i++)
            buf[i] = (uint8_t)p[i];
        for (i = n; i < kl; i++)
            buf[i] = (uint8_t)' ';

        /* Significant length = the search key's length with trailing spaces
         * trimmed, clamped to key_length. The OFF/begins-with rule compares the
         * stored key's first sig_len bytes against the search key.
         * [mint-002: char "=" is directional, LEFT begins with RIGHT.] */
        {
            uint32_t s = n;
            while (s > 0u && buf[s - 1u] == (uint8_t)' ')
                s--;
            *sig_len = s;
        }
        return NDX_OK;
    }

    if (idx->key_type == (uint16_t)NDX_KEY_TYPE_NUMERIC) {
        double v;
        if (key->t == XB_N)      v = key->u.n;
        else if (key->t == XB_D) v = key->u.d;
        else                     return -NDX_ERR_BAD_KEYTYPE;

        if (kl != 8u)
            return -NDX_ERR_BAD_GROUP;   /* type-1 key_length is always 8 */
        rt_memcpy(buf, &v, 8u);
        *sig_len = 8u;
        return NDX_OK;
    }

    return -NDX_ERR_BAD_KEYTYPE;
}

/*
 * match_at_landing: decide FOUND for the stored landing key vs the search key.
 *
 *   stored   the landing leaf entry's raw key_data (key_length bytes).
 *   skey     the rendered search key (key_length bytes; from build_search_key).
 *   sig_len  significant length of the search key (char) or 8 (num/date).
 *   set_exact 0 = OFF (char begins-with), non-zero = ON (full equal).
 *
 * Returns 1 (found) or 0 (not found).
 *
 * char OFF (begins-with): stored[0..sig_len) == skey[0..sig_len). The stored
 *   key is the LEFT operand; it must BEGIN WITH the search key (mint-002).
 *   sig_len == 0 (empty search key) matches anything ("" begins every key) --
 *   consistent with directional begins-with where RIGHT is empty.
 * char ON (full equal): the full key_length bytes must match (ndx_key_cmp == 0).
 * num/date: arithmetic equality (ndx_key_cmp == 0).
 */
static int match_at_landing(const ndx_index *idx, const uint8_t *stored,
                            const uint8_t *skey, uint32_t sig_len, int set_exact)
{
    if (idx->key_type == (uint16_t)NDX_KEY_TYPE_CHAR && set_exact == 0) {
        /* OFF: directional begins-with over the search key's significant prefix.
         * Ref: re/mint-results-002.md (LEFT begins with RIGHT). */
        uint32_t i;
        for (i = 0u; i < sig_len; i++) {
            if (stored[i] != skey[i])
                return 0;
        }
        return 1;
    }
    /* char ON, or num/date: exact equality via the collation comparator. */
    return (ndx_key_cmp(idx, stored, skey) == 0) ? 1 : 0;
}

int ndx_seek(ndx_index *idx, const xb_val *key, int set_exact,
             uint32_t *recno_out, int *found_out)
{
    uint8_t  skey[NDX_SEEK_KEYBUF];
    uint32_t sig_len = 0u;
    int      rc;
    uint32_t page_no;
    uint32_t depth;

    if (recno_out) *recno_out = 0u;
    if (found_out) *found_out = 0;

    if (!idx || !key)
        return -NDX_ERR_BAD_PAGE;

    rc = build_search_key(idx, key, skey, &sig_len);
    if (rc != NDX_OK)
        return rc;

    /* Descend from the root to a leaf, bounded by total_pages (Rule 2). */
    page_no = idx->root_page;
    depth   = idx->total_pages;

    for (;;) {
        ndx_node_t *node = (ndx_node_t *)0;
        int         is_branch;
        uint32_t    i;
        uint32_t    next_page = 0u;
        int         descended = 0;

        if (depth == 0u)
            return -NDX_ERR_CYCLE;
        depth--;

        rc = ndx_read_node(idx, page_no, &node);
        if (rc != NDX_OK)
            return rc;

        is_branch = (node->entry_count > 0u
                     && node->entries[0].child_page != 0u);

        if (is_branch) {
            /* Find the first separator entry whose key >= search key; descend
             * into THAT entry's child. The separator at entry i is the HIGH key
             * of subtree child_page[i] (ss5), so the first separator >= K bounds
             * the subtree that may contain K. If K exceeds every separator,
             * descend into the rightmost (trailing) child.
             * Ref: ndx.md ss5 search algorithm. */
            for (i = 0u; i < (uint32_t)node->entry_count; i++) {
                if (ndx_key_cmp(idx, node->entries[i].key_data, skey) >= 0) {
#ifndef NDX_MUTATE_SEEK_CHILD
                    next_page = node->entries[i].child_page;
#else
                    /* MUTATED wrong-child descent: take entry[i+1]'s child
                     * (off-by-one) -- skips the subtree that holds K (whose HIGH
                     * key is entry[i]), so SEEK lands in the wrong subtree and
                     * resolves the wrong recno / EOFs. RED. For the last entry
                     * this folds onto the trailing child. */
                    next_page = (i + 1u < (uint32_t)node->entry_count)
                                    ? node->entries[i + 1u].child_page
                                    : node->trail_child;
#endif
                    descended = 1;
                    break;
                }
            }
            if (!descended) {
                /* K exceeds all separators: rightmost child (ss5 / ss3.2). */
                next_page = node->trail_child;
                descended = 1;
            }
            ndx_node_free(idx, node);
            if (next_page == 0u)
                return -NDX_ERR_BAD_PAGE;   /* branch with no child -- corrupt */
            page_no = next_page;
            continue;
        }

        /* Leaf: scan for the FIRST entry whose key >= search key. That is the
         * landing entry. If none (every key < search key), the landing is past
         * the end of this leaf. Since the descent already bounded us to the leaf
         * whose subtree HIGH key >= K (or the rightmost leaf when K exceeds all),
         * "past the end of THIS leaf" means past the end of the index -> EOF.
         * Ref: ndx.md ss5 (leaf exact match returns dbf_recno). */
        {
            int found_landing = 0;
            for (i = 0u; i < (uint32_t)node->entry_count; i++) {
                if (ndx_key_cmp(idx, node->entries[i].key_data, skey) >= 0) {
                    uint32_t recno   = node->entries[i].dbf_recno;
                    int      matched = match_at_landing(idx,
                                            node->entries[i].key_data,
                                            skey, sig_len, set_exact);
                    if (recno_out) *recno_out = recno;
                    if (found_out) *found_out = matched;
                    found_landing = 1;
                    break;
                }
            }
            ndx_node_free(idx, node);
            if (!found_landing) {
                /* EOF: positioned past the last key (recno 0, not found). */
                if (recno_out) *recno_out = 0u;
                if (found_out) *found_out = 0;
            }
            return NDX_OK;
        }
    }
}

/* -----------------------------------------------------------------------
 * S4.4: bulk INDEX ON build
 * -----------------------------------------------------------------------
 *
 * The byte-exact bulk-build algorithm, grounded entirely in minted/pristine
 * bytes (NOT guessed -- Law 1):
 *
 *   - PACK leaves 100% left-to-right, remainder in the last leaf.
 *     [re/mint-results-003.md: BIGIDX 245 = 13*18 + 11; ndx.md Open questions
 *      "bulk build" RESOLVED]
 *   - Root branch separator i = HIGH (last) key of leaf i; trailing child = the
 *     last leaf (N keys / N+1 children).
 *     [VERIFIED: BIGIDX root sep[i] == last key of leaf i+1 byte-checked here;
 *      NCOST root sep[0]=3295.0 == leaf 1 high key; ZIPCODE sep[0]="91306".]
 *   - Page numbering: leaves on pages 1..L; L==1 -> root_page=1,total=2;
 *     L>1 -> root_page=L+1,total=L+2.
 *     [VERIFIED: NDATE root=1/total=2; NCOST root=3/total=4; BIGIDX 15/16.]
 *   - Equal-key tie-break = ascending recno (stable sort over physical order).
 *     [VERIFIED: BIGIDX five "Beman" entries in recno order 42,91,140,189,238.]
 *
 * NORMALIZE bytes are written 0x00 (Rule 11: SAMIR is more normalized than
 * genuine III+; the oracle masks these before cmp -- header reserved/dummy,
 * header tail past the expr NUL, group filler, node filler word, page tails,
 * and the non-child bytes of the trailing slot).
 * ----------------------------------------------------------------------- */

/* u32le_w / u16le_w: write a little-endian integer (no aliasing cast). */
static void u32le_w(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static void u16le_w(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

/*
 * build_cmp: compare two collected keys for the bulk-build sort. Mirrors
 * ndx_key_cmp but operates on a (key_type, key_len) pair directly (no open
 * ndx_index yet). char = unsigned byte compare over key_len; type-1 = arithmetic
 * double compare (NO sign-flip; ndx.md ss4.2). This MUST match ndx_key_cmp so
 * the rebuilt order equals the read order.
 */
static int build_cmp(uint16_t key_type, uint16_t key_len,
                     const uint8_t *a, const uint8_t *b)
{
    if (key_type == (uint16_t)NDX_KEY_TYPE_CHAR) {
        uint32_t i;
        for (i = 0u; i < (uint32_t)key_len; i++) {
            if (a[i] < b[i]) return -1;
            if (a[i] > b[i]) return  1;
        }
        return 0;
    }
    {
        double va = read_le_double(a);
        double vb = read_le_double(b);
        if (va < vb) return -1;
        if (va > vb) return  1;
        return 0;
    }
}

/*
 * stable_sort_keys: stable insertion sort of the parallel (keys, recnos)
 * arrays by build_cmp. STABLE so equal keys keep their input (physical recno)
 * order -- the verified tie-break (BIGIDX "Beman" recnos 42,91,140,189,238).
 * Insertion sort uses a strict `> 0` comparison so equal keys never swap.
 * N is small (one page-buffer of keys per record; corpus tops out ~245), so
 * O(N^2) is fine and avoids a recursive merge's extra arena.
 */
static void stable_sort_keys(uint16_t key_type, uint16_t key_len,
                             uint8_t *keys, uint32_t *recnos, uint32_t n,
                             uint8_t *tmpkey)
{
    uint32_t i;
    for (i = 1u; i < n; i++) {
        uint32_t tmpr = recnos[i];
        uint32_t j    = i;
        rt_memcpy(tmpkey, keys + (uint32_t)i * key_len, key_len);
        while (j > 0u &&
               build_cmp(key_type, key_len,
                         keys + (uint32_t)(j - 1u) * key_len, tmpkey) > 0) {
            rt_memcpy(keys + (uint32_t)j * key_len,
                      keys + (uint32_t)(j - 1u) * key_len, key_len);
            recnos[j] = recnos[j - 1u];
            j--;
        }
        rt_memcpy(keys + (uint32_t)j * key_len, tmpkey, key_len);
        recnos[j] = tmpr;
    }
}

/*
 * write_page: seek to page PAGE_NO * 512 and write the 512-byte PAGE buffer.
 * Returns NDX_OK or -NDX_ERR_IO (fail loud on short write -- Rule 2).
 */
static int write_page(samir_pal_t *pal, pal_fd fd, uint32_t page_no,
                      const uint8_t *page)
{
    int rc = seek_to(pal, fd, page_no * (uint32_t)NDX_PAGE_SIZE);
    int32_t put;
    if (rc != NDX_OK)
        return rc;
    put = pal->write(pal, fd, page, (uint32_t)NDX_PAGE_SIZE);
    if (put < 0 || (uint32_t)put != (uint32_t)NDX_PAGE_SIZE)
        return -NDX_ERR_IO;
    return NDX_OK;
}

int ndx_build(samir_pal_t *pal, const char *out_name,
              uint16_t key_type, uint16_t key_len, const char *key_expr,
              uint32_t nrec, ndx_key_provider get_key, void *user)
{
    void     *mark;
    uint8_t  *keys;       /* nrec * key_len bytes of on-disk key data */
    uint32_t *recnos;     /* nrec recnos, parallel to keys */
    uint8_t  *tmpkey;     /* one key_len scratch for the stable sort */
    uint32_t  group_len;
    uint32_t  kpp;        /* keys_per_page */
    uint32_t  nleaf;      /* number of leaf pages */
    uint32_t  root_page;
    uint32_t  total_pages;
    pal_fd    fd;
    uint8_t   page[NDX_PAGE_SIZE];
    int       rc;
    uint32_t  i;

    if (!pal || !out_name || !get_key)
        return -NDX_ERR_IO;

    /* --- Validate key_type / key_len (Rule 2). --- */
    if (key_type != (uint16_t)NDX_KEY_TYPE_CHAR
        && key_type != (uint16_t)NDX_KEY_TYPE_NUMERIC)
        return -NDX_ERR_BAD_KEYTYPE;
    if (key_type == (uint16_t)NDX_KEY_TYPE_NUMERIC
        && key_len != (uint16_t)NDX_KEY_LEN_DOUBLE)
        return -NDX_ERR_BAD_KEYTYPE;   /* type-1 key_length is always 8 */
    if (key_len == 0u)
        return -NDX_ERR_BAD_GROUP;

    /* --- Derived geometry (must match ndx_open's validation formulas). --- */
    group_len = ceil4((uint32_t)key_len + (uint32_t)NDX_GRP_OVERHEAD);
    if (group_len > 0xFFFFu)
        return -NDX_ERR_BAD_GROUP;     /* group_length must fit a u16 */
    kpp = (uint32_t)(NDX_PAGE_SIZE - NDX_NODE_HDR_SIZE) / group_len;
    if (kpp == 0u)
        return -NDX_ERR_BAD_GROUP;     /* a single key does not fit a page */

    mark = pal->alloc(pal, 0u);        /* arena entry mark; reset before return */

    /* --- Allocate the key buffer + recnos + sort scratch from the arena. --- */
    {
        uint32_t keys_bytes = nrec * (uint32_t)key_len;
        keys   = (uint8_t  *)pal->alloc(pal, keys_bytes ? keys_bytes : 1u);
        recnos = (uint32_t *)pal->alloc(pal,
                     (nrec ? nrec : 1u) * (uint32_t)sizeof(uint32_t));
        tmpkey = (uint8_t  *)pal->alloc(pal, (uint32_t)key_len);
        if (!keys || !recnos || !tmpkey) {
            pal->reset(pal, mark);
            return -NDX_ERR_OOM;
        }
    }

    /* --- Collect one key per record, in physical recno order (1..nrec). --- */
    for (i = 0u; i < nrec; i++) {
        uint32_t recno = i + 1u;       /* 1-based DBF recno */
        int prc = get_key(user, recno, keys + i * (uint32_t)key_len, key_len);
        if (prc != 0) {
            pal->reset(pal, mark);
            return (prc > 0) ? -prc : -NDX_ERR_IO;
        }
        recnos[i] = recno;
    }

    /* --- STABLE sort by collation; ties keep ascending recno. --- */
    stable_sort_keys(key_type, key_len, keys, recnos, nrec, tmpkey);

    /* --- Leaf count: pack 100% L->R, remainder last. --- */
    /* nleaf = ceil(nrec / kpp), minimum 1 (an empty index still has 1 leaf). */
    nleaf = (nrec + kpp - 1u) / kpp;
    if (nleaf == 0u)
        nleaf = 1u;

    /*
     * Page layout (VERIFIED):
     *   L == 1: the single leaf IS the root. root_page = 1, total_pages = 2.
     *   L  > 1: leaves on pages 1..L, root branch on page L+1.
     *           root_page = L+1, total_pages = L+2.
     *
     * MULTI-LEVEL GUARD (corpus-OPEN, Law 1 / Rule 2): when L > 1 the root
     * branch holds L-1 separators + a trailing child. All L child pointers must
     * fit one branch page: the L-1 separator GROUPS plus the trailing 4-byte
     * child slot must fit in 512 bytes, i.e. (L-1) <= kpp (the same keys_per_page
     * bound the reader validates). If L-1 > kpp a 3-level tree would be needed,
     * whose interior packing has no minted III+ golden -- FAIL LOUD instead of
     * guessing. (The two-level case -- one root branch over the leaves -- is
     * byte-exact and is what every corpus .ndx uses.)
     */
    if (nleaf > 1u && (nleaf - 1u) > kpp) {
        pal->reset(pal, mark);
        return -NDX_ERR_PAGE_OVF;
    }

    if (nleaf == 1u) {
        root_page   = 1u;
        total_pages = 2u;
    } else {
        root_page   = nleaf + 1u;
        total_pages = nleaf + 2u;
    }

    /* --- Open the output file (create/truncate). --- */
    fd = pal->open(pal, out_name, PAL_RDWR | PAL_CREATE | PAL_TRUNC);
    if (fd < 0) {
        pal->reset(pal, mark);
        return -NDX_ERR_IO;
    }

    /* --- Page 0: the header. --- */
    rt_memset(page, 0, (uint32_t)NDX_PAGE_SIZE);
    u32le_w(page + NDX_HDR_ROOT_PAGE_OFF,    root_page);
    u32le_w(page + NDX_HDR_TOTAL_PAGES_OFF,  total_pages);
    u32le_w(page + NDX_HDR_RESERVED_OFF,     0u);             /* NORMALIZE -> 0 */
    u16le_w(page + NDX_HDR_KEY_LENGTH_OFF,   key_len);
    u16le_w(page + NDX_HDR_KEYS_PER_PAGE_OFF,(uint16_t)kpp);
    u16le_w(page + NDX_HDR_KEY_TYPE_OFF,     key_type);
    u16le_w(page + NDX_HDR_GROUP_LENGTH_OFF, (uint16_t)group_len);
    u16le_w(page + NDX_HDR_DUMMY_OFF,        0u);             /* NORMALIZE -> 0 */
    u16le_w(page + NDX_HDR_UNIQUE_FLAG_OFF,  0u);             /* bulk INDEX ON is non-unique */
    /* key_expr verbatim, NUL-terminated, cap 100. Tail after NUL stays 0. */
    if (key_expr) {
        uint32_t e;
        for (e = 0u; e < (uint32_t)NDX_HDR_KEY_EXPR_SIZE; e++) {
            char c = key_expr[e];
            if (c == '\0') break;
            page[NDX_HDR_KEY_EXPR_OFF + e] = (uint8_t)c;
        }
        /* page already memset to 0 -> the NUL terminator + tail are 0. */
    }
    rc = write_page(pal, fd, 0u, page);
    if (rc != NDX_OK) { pal->close(pal, fd); pal->reset(pal, mark); return rc; }

    /* --- Leaf pages 1..nleaf: pack 100% L->R, remainder in the last leaf. --- */
    {
        uint32_t leaf;
        uint32_t key_i = 0u;           /* next sorted key index to place */
        for (leaf = 0u; leaf < nleaf; leaf++) {
            uint32_t remaining = (nrec > key_i) ? (nrec - key_i) : 0u;
            uint32_t fill;
            uint32_t leaves_left = nleaf - leaf;
            uint32_t g;

#ifndef NDX_MUTATE_SPLIT_5050
            /* CORRECT bulk-build packing: fill this leaf to capacity, leaving the
             * remainder for the LAST leaf. The last leaf takes whatever is left.
             * [re/mint-results-003.md: 100%-pack, remainder last; NOT 50/50.] */
            if (leaf + 1u < nleaf) {
                fill = kpp;            /* a non-final leaf is always full */
            } else {
                fill = remaining;      /* the final leaf gets the remainder */
            }
#else
            /* MUTATED ~50/50 split (Rule 6): spread the remaining keys evenly
             * across the remaining leaves (ceil division) instead of packing to
             * capacity. With L leaves this fills each to about N/L < kpp, so the
             * fill levels (and the root separators) diverge from the golden ->
             * the byte-exact rebuild oracle goes RED. */
            fill = (remaining + leaves_left - 1u) / leaves_left;
            if (fill > kpp) fill = kpp;
#endif
            (void)leaves_left;
            if (fill > remaining) fill = remaining;

            rt_memset(page, 0, (uint32_t)NDX_PAGE_SIZE);
            u16le_w(page + NDX_NODE_ENTRY_COUNT_OFF, (uint16_t)fill);
            u16le_w(page + NDX_NODE_FILLER_OFF, 0u);   /* NORMALIZE -> 0 */

            for (g = 0u; g < fill; g++) {
                uint32_t base = (uint32_t)NDX_NODE_ENTRIES_OFF
                              + g * group_len;
                u32le_w(page + base + NDX_GRP_CHILD_PAGE_OFF, 0u);  /* leaf */
                u32le_w(page + base + NDX_GRP_DBF_RECNO_OFF, recnos[key_i + g]);
                rt_memcpy(page + base + NDX_GRP_KEY_DATA_OFF,
                          keys + (key_i + g) * (uint32_t)key_len, key_len);
                /* group filler after key_data stays 0 (NORMALIZE). */
            }
            /* The unused page tail + trailing slot stay 0 (NORMALIZE; leaf
             * trailing slot is garbage in genuine III+, ndx.md ss3.2). */

            rc = write_page(pal, fd, leaf + 1u, page);
            if (rc != NDX_OK) {
                pal->close(pal, fd); pal->reset(pal, mark); return rc;
            }

            key_i += fill;
        }
    }

    /* --- Root branch page (only when L > 1). --- */
    if (nleaf > 1u) {
        /* The root branch has nleaf-1 separator entries + a trailing child.
         * separator i = the HIGH (last) key of leaf (i+1), child_page = leaf i+1
         * (1-based page number), recno = 0 (branch entries carry recno 0).
         * trailing child = leaf nleaf. (ndx.md ss5 / ss3.2; VERIFIED BIGIDX.)
         *
         * We recompute each leaf's high-key index from the SAME fill schedule
         * used above so the separator keys match the leaves byte-for-byte.
         */
        uint32_t nsep = nleaf - 1u;
        uint32_t leaf;
        uint32_t key_i = 0u;
        uint32_t sep   = 0u;

        rt_memset(page, 0, (uint32_t)NDX_PAGE_SIZE);
        u16le_w(page + NDX_NODE_ENTRY_COUNT_OFF, (uint16_t)nsep);
        u16le_w(page + NDX_NODE_FILLER_OFF, 0u);   /* NORMALIZE -> 0 */

        for (leaf = 0u; leaf < nleaf; leaf++) {
            uint32_t remaining = (nrec > key_i) ? (nrec - key_i) : 0u;
            uint32_t fill;
#ifndef NDX_MUTATE_SPLIT_5050
            if (leaf + 1u < nleaf) fill = kpp;
            else                   fill = remaining;
#else
            uint32_t leaves_left = nleaf - leaf;
            fill = (remaining + leaves_left - 1u) / leaves_left;
            if (fill > kpp) fill = kpp;
#endif
            if (fill > remaining) fill = remaining;

            if (leaf + 1u < nleaf) {
                /* separator for this leaf = its HIGH (last) key. */
                uint32_t hi_idx = key_i + fill - 1u;   /* last key of this leaf */
                uint32_t base   = (uint32_t)NDX_NODE_ENTRIES_OFF
                                + sep * group_len;
                u32le_w(page + base + NDX_GRP_CHILD_PAGE_OFF, leaf + 1u);
                u32le_w(page + base + NDX_GRP_DBF_RECNO_OFF, 0u);   /* branch */
                rt_memcpy(page + base + NDX_GRP_KEY_DATA_OFF,
                          keys + hi_idx * (uint32_t)key_len, key_len);
                sep++;
            }
            key_i += fill;
        }

        /* Trailing child slot = the last leaf (rightmost subtree, ndx.md ss3.2).
         * Only the 4-byte child pointer is MEANINGFUL; recno/key bytes stay 0. */
        {
            uint32_t trail_off = (uint32_t)NDX_NODE_ENTRIES_OFF
                               + nsep * group_len;
            u32le_w(page + trail_off + NDX_GRP_CHILD_PAGE_OFF, nleaf);
        }

        rc = write_page(pal, fd, root_page, page);
        if (rc != NDX_OK) {
            pal->close(pal, fd); pal->reset(pal, mark); return rc;
        }
    }

    pal->close(pal, fd);
    pal->reset(pal, mark);
    return NDX_OK;
}
