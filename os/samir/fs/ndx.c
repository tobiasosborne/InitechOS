/*
 * os/samir/fs/ndx.c -- SAMIR (InitechBase) .ndx B-tree index codec, step S4.1.
 *                       Header + node parse. Key decode to typed values is S4.2.
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
 * Mutation hook (Rule 6 / plan S4.1 "clicketyclick wrong 18-23 sublayout"):
 *   Build with -DNDX_MUTATE_SUBLAYOUT. The perturbed read swaps the child_page
 *   and dbf_recno field offsets within each group AND shifts the key_data start
 *   by +4 bytes -- modeling the wrong clicketyclick header diagram that places
 *   "Size of key record" as a 4-byte long at bytes 18-21 and unique as a single
 *   byte at 23. Under the mutation every parsed child/recno/key mismatches the
 *   golden -> the oracle goes RED. [ndx.md Open questions: "clicketyclick's
 *   diagram puts 'Size of key record' as a 4-byte long at bytes 18-21 ... The
 *   real bytes contradict it; the bytes win."]
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
 *       ss4 (key encoding boundaries), ss8 (implementation checklist).
 *   - docs/plans/SAMIR-implementation-plan.md S4.1 contract + Sec 3.3.
 *   - os/samir/include/samir/pal.h (byte I/O vtable).
 *   - os/samir/include/samir/rt.h  (rt_memcpy, rt_memset).
 */

#include <stdint.h>
#include "samir/pal.h"
#include "samir/rt.h"
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
