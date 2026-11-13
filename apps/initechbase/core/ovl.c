/*
 * os/ibase/core/ovl.c -- the IBASE overlay manager (InitechBase overlays
 * Phase B, epic initech-5etom; design: docs/design/ibase-overlays.md).
 *
 * THE ARTIFACT (Law 3): freestanding C; all OS contact through the PAL.
 *
 * Two parts:
 *   1. Pure helpers (CRC-32, header/directory checks, relocation). Compiled in
 *      every build that lists this file; the host oracle test_ibase_ovl.c
 *      drives them against literal bytes.
 *   2. The InitechDOS manager (IBASE_OVERLAYS): one fixed slot inside IBASE's own
 *      image window (ibase.ld), filled from INITBASE.OVL on demand the way
 *      DBASE.EXE fills its overlay area from DBASE.OVL -- open, LSEEK, READ
 *      (DBASE.EXE file offsets 0x204B4 / 0x204FC / 0x20553; spec/ibase/
 *      ovl_format.h) -- then relocated by the resident load base B exactly as
 *      the InitechMZ loader relocated the resident module (ADR-0003 DEC-08a.1).
 *
 * Failure model (Rule 2; design note Sec 6): INITBASE.OVL is verified whole at
 * startup (ADR-0014 DEC-14-5 Alternative 5A, ratified for the kernel's own
 * overlay: refuse before the user has work to lose) and every overlay is
 * CRC-checked again on each load. Anything wrong -> the "Insert System Disk"
 * prompt (ENTER retries, Ctrl-C aborts). An abort mid-session closes every
 * work area (headers flushed) and terminates. A swap requested while an
 * overlay is running is an invariant violation (the packer's call-graph check
 * makes it unreachable) and panics.
 *
 * ASCII-clean (Rule 12). No timestamps (Rule 11).
 */
#include <stdint.h>
#include <stddef.h>
#include "ibase/ovl.h"
#include "ibase/ovl_format.h"   /* LOCKED spec-data, on -Ispec */
#include "ibase/rt.h"
#include "ibase/lifetime.h"

static uint32_t ovl_rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

uint32_t ibase_ovl_crc32(uint32_t crc, const void *buf, uint32_t n)
{
    const uint8_t *p = (const uint8_t *)buf;
    crc = ~crc;
    while (n--) {
        crc ^= *p++;
        for (int i = 0; i < 8; i++)
            crc = (crc >> 1) ^ (IBASE_OVL_CRC_POLY & (0u - (crc & 1u)));
    }
    return ~crc;
}

int ibase_ovl_check_header(const uint8_t *h, uint32_t stamp, uint32_t slot_len,
                           uint32_t count)
{
#ifndef IBASE_OVL_MUTATE_NO_STAMP
    if (rt_memcmp(h + IBASE_OVL_HDR_SIG_OFF, IBASE_OVL_SIG, IBASE_OVL_SIG_LEN) ||
        (uint32_t)(h[IBASE_OVL_HDR_VER_OFF] | (h[IBASE_OVL_HDR_VER_OFF + 1] << 8)) != IBASE_OVL_VERSION ||
        (uint32_t)(h[IBASE_OVL_HDR_COUNT_OFF] | (h[IBASE_OVL_HDR_COUNT_OFF + 1] << 8)) != count ||
        ovl_rd32(h + IBASE_OVL_HDR_STAMP_OFF) != stamp ||
        ovl_rd32(h + IBASE_OVL_HDR_SLOT_OFF) != slot_len)
        return IBASE_OVL_FOREIGN;
#else
    /* MUTANT (Rule 6; test-ibase-ovl-emu-mutant only): accept an overlay file
     * from any build -- the stale-file emu leg must go RED. */
    (void)stamp; (void)slot_len; (void)count;
#endif
    if (ovl_rd32(h + IBASE_OVL_HDR_FLEN_OFF) < IBASE_OVL_HDR_SIZE)
        return IBASE_OVL_CORRUPT;
    return IBASE_OVL_OK;
}

int ibase_ovl_check_dir(const uint8_t *e, uint32_t slot_len, uint32_t file_len,
                        uint32_t *off, uint32_t *len, uint32_t *nrel, uint32_t *crc)
{
    uint32_t o = ovl_rd32(e + IBASE_OVL_DIR_OFF_OFF);
    uint32_t l = ovl_rd32(e + IBASE_OVL_DIR_LEN_OFF);
    uint32_t r = ovl_rd32(e + IBASE_OVL_DIR_NREL_OFF);
    /* The module must carry at least the slot header, fit the slot, and the
     * module plus its relocation table must lie inside the file (no wrap). */
    if (l < IBASE_OVL_SLOT_HDR_SIZE || l > slot_len || r > (file_len >> 2) ||
        o < IBASE_OVL_HDR_SIZE || o > file_len || l > file_len - o ||
        r * 4u > file_len - o - l)
        return IBASE_OVL_CORRUPT;
    *off = o; *len = l; *nrel = r; *crc = ovl_rd32(e + IBASE_OVL_DIR_CRC_OFF);
    return IBASE_OVL_OK;
}

int ibase_ovl_relocate(uint8_t *slot, uint32_t len, const uint8_t *rel,
                       uint32_t n, uint32_t base)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        uint32_t o = ovl_rd32(rel + 4u * i);
        if (len < 4u || o > len - 4u) return -1;
    }
    for (i = 0; i < n; i++) {
        uint8_t *p = slot + ovl_rd32(rel + 4u * i);
        uint32_t v = ovl_rd32(p);
#ifndef IBASE_OVL_MUTATE_NO_RELOC
        v += base;
#else
        /* MUTANT (Rule 6; test-ibase-ovl-mutant / swap-emu-mutant only): the
         * overlay is used unrelocated. */
        (void)base;
#endif
        p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
        p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
    }
    return 0;
}

#ifdef IBASE_OVERLAYS
/* ---- The InitechDOS manager ---------------------------------------------------
 *
 * Link-time facts (os/ibase/boot/ibase.ld). Every one is reached through an
 * absolute reference the InitechMZ loader relocated, so it reads as a run-time
 * address: __ibase_base is the resident load base B, __ovl_slot the slot.
 */
extern char __ibase_base[];
extern char __ovl_slot[];
extern char __ovl_slot_end[];
/* Patched by tools/ibase_ovl_pack.c with the INITBASE.OVL stamp (the CRC-32 of the
 * file body) after the link; volatile so the compiler never folds the 0. */
const volatile uint32_t ibase_ovl_stamp = 0u;

/* pal_dos.c: the only int 0x21 TU (ADR-0009 DEC-06). */
void pal_dos_terminate(uint8_t code) __attribute__((noreturn));
void pal_dos_overlay_panic(const char *,uint32_t,uint32_t,uint32_t,uintptr_t)
    __attribute__((noreturn));

#define OVL_NONE 0xFFFFFFFFu

static struct {
    ibase_pal_t *pal;
    pal_fd       fd;          /* INITBASE.OVL, open for the session (or -1)  */
    uint32_t     loaded;      /* id in the slot, or OVL_NONE              */
    uint32_t     requested;
    uint32_t     depth;       /* overlay frames on the stack              */
    uint32_t     file_len;
    uint8_t      dir[IBASE_OVL_MAX * IBASE_OVL_DIR_SIZE];
} O;

static void __attribute__((noreturn)) ovl_panic(const char *why,uintptr_t p)
{
    pal_dos_overlay_panic(why,O.loaded,O.requested,O.depth,p);
}
void ibase_ovl_require_resident(uintptr_t p,const char *where)
{
    if (p >= (uintptr_t)__ovl_slot && p < (uintptr_t)__ovl_slot_end)
        ovl_panic(where,p);
}
/* Ref: design Sec 6, rule 4. Keep the original audit order and spelling,
 * including the two cursor prefixes (binding vs boundary). Each descriptor
 * costs two bytes, with no per-field pointer/relocation. Build the full
 * diagnostic only on failure; successful audits just advance over its name.
 * The lists generate both metadata and names so they cannot get out of step.
 */
#define PAL_FIELDS(F) \
    F(open) F(close) F(read) F(write) F(seek) F(remove) F(rename) \
    F(conout) F(conin_line) F(conin_char) F(gotoxy) F(set_attr) F(today) \
    F(alloc) F(reset) F(acquire) F(release) F(status) F(conin_line_noecho) \
    F(exec) F(conin_escape) F(curdrive) F(getenv) F(os_version) F(diskspace) \
    F(conin_poll) F(time) F(iscolor) F(say_screen) F(printout) F(paint) \
    F(prompt) F(conin_command)
#define CTX_FIELDS(F) \
    F(mirror_output) F(mirror_user) F(resolve) F(user) F(scratch) F(dbcur) \
    F(dbcur_user) F(file_pal) F(record_size) F(on_error) F(message) \
    F(on_escape) F(program_name) F(on_error_text) F(on_escape_text)
#define CURSOR_FIELDS(F) \
    F(recno) F(reccount) F(eof) F(bof) F(found) F(deleted) F(field_name) \
    F(dbf_name) F(file_exists) F(last_update) F(index_name)

typedef struct { uint8_t offset, name_len; } ovl_field;
typedef struct {
    const ovl_field *fields;
    const char *names;
} ovl_fields;
#define FIELD_NAME(f) #f "\0"
#define FIELD_DESC(t,f) { offsetof(t,f)/sizeof(uintptr_t), sizeof(#f) },
/* Pointer-sized offsets keep the two-byte descriptor valid on both the
 * flat x86 artifact and the host oracle as the PAL gains callbacks. Check
 * alignment and range rather than truncating a future layout extension. */
#define FIELD_ASSERT(t,f) \
    _Static_assert(offsetof(t,f)/sizeof(uintptr_t) < 255 \
        && offsetof(t,f)%sizeof(uintptr_t) == 0 && sizeof(((t *)0)->f) == sizeof(uintptr_t) \
        && sizeof(#f) <= 32, "overlay field descriptor: " #t "." #f);
#define PAL_ASSERT(f) FIELD_ASSERT(ibase_pal_t,f)
#define CTX_ASSERT(f) FIELD_ASSERT(xb_ctx,f)
#define CURSOR_ASSERT(f) FIELD_ASSERT(xb_dbcursor,f)
PAL_FIELDS(PAL_ASSERT)
CTX_FIELDS(CTX_ASSERT)
CURSOR_FIELDS(CURSOR_ASSERT)
#define PAL_DESC(f) FIELD_DESC(ibase_pal_t,f)
#define CTX_DESC(f) FIELD_DESC(xb_ctx,f)
#define CURSOR_DESC(f) FIELD_DESC(xb_dbcursor,f)
static const ovl_field pal_fields[] = { PAL_FIELDS(PAL_DESC) {255,0} };
static const ovl_field ctx_fields[] = { CTX_FIELDS(CTX_DESC) {255,0} };
static const ovl_field cursor_fields[] = { CURSOR_FIELDS(CURSOR_DESC) {255,0} };
static const ovl_fields pal_audit = { pal_fields, PAL_FIELDS(FIELD_NAME) };
static const ovl_fields ctx_audit = { ctx_fields, CTX_FIELDS(FIELD_NAME) };
static const ovl_fields cursor_audit = { cursor_fields, CURSOR_FIELDS(FIELD_NAME) };

static void ovl_validate_fields(const void *base,const ovl_fields *audit,
                                const char *prefix)
{
    const ovl_field *f = audit->fields;
    const char *name = audit->names;
    if (!base) return;
    for (; f->offset != 255; name += f->name_len, ++f) {
        uintptr_t p;
        /* Copy the representation: no pointer-type aliasing or alignment
         * assumptions when reading function and data pointers alike. */
        __builtin_memcpy(&p,(const char *)base + f->offset*sizeof(uintptr_t),sizeof p);
        if (p >= (uintptr_t)__ovl_slot && p < (uintptr_t)__ovl_slot_end) {
            char where[48]; /* longest prefix 10 + field including NUL <=32 */
            uint32_t n = rt_strlen(prefix);
            rt_memcpy(where,prefix,n);
            rt_memcpy(where+n,name,f->name_len);
            ovl_panic(where,p);
        }
    }
}
void ibase_ovl_validate_pal(const ibase_pal_t *pal)
{
    IBASE_RETAIN(pal);
    ovl_validate_fields(pal,&pal_audit,"pal->");
}
void ibase_ovl_validate_cursor(const xb_dbcursor *v)
{
    /* The binder already checked v before passing it here. */
    ovl_validate_fields(v,&cursor_audit,"v->");
}
void ibase_ovl_validate_ctx(const xb_ctx *c)
{
    IBASE_RETAIN(c);
    ovl_validate_fields(c,&ctx_audit,"c->");
    if (!c) return;
    ibase_ovl_validate_pal(c->file_pal);
    ovl_validate_fields(c->dbcur,&cursor_audit,"c->dbcur->");
}

static uint32_t ovl_slot_len(void)
{
    return (uint32_t)(__ovl_slot_end - __ovl_slot);
}

static int ovl_read_at(uint32_t off, void *buf, uint32_t n)
{
    ibase_pal_t *pal = O.pal;
    if (pal->seek(pal, O.fd, (int32_t)off, PAL_SEEK_SET) != (int32_t)off) return -1;
    return pal->read(pal, O.fd, buf, n) == (int32_t)n ? 0 : -1;
}

static void ovl_close(void)
{
    if (O.fd >= 0) (void)O.pal->close(O.pal, O.fd);
    O.fd = -1;
    O.loaded = OVL_NONE;
}

/* Open INITBASE.OVL and verify all of it. 0 = ready, else a IBASE_OVL_* reason
 * (the file is closed again on failure). */
static int ovl_ready(void)
{
    ibase_pal_t *pal = O.pal;
    uint8_t hdr[IBASE_OVL_HDR_SIZE];
    uint8_t buf[256];
    uint32_t pos, crc = 0u, i, off, len, nrel, ecrc;
    int32_t end;
    int r;

    ovl_close();
    O.fd = pal->open(pal, IBASE_OVL_FILE_NAME, PAL_RD);
    if (O.fd < 0) { O.fd = -1; return IBASE_OVL_MISSING; }
    end = pal->seek(pal, O.fd, 0, PAL_SEEK_END);
    if (end < (int32_t)IBASE_OVL_HDR_SIZE || ovl_read_at(0u, hdr, sizeof hdr)) {
        ovl_close();
        return IBASE_OVL_MISSING;
    }
    O.file_len = (uint32_t)end;
    r = ibase_ovl_check_header(hdr, ibase_ovl_stamp, ovl_slot_len(), IBASE_OVL_COUNT);
    if (r == IBASE_OVL_OK && ovl_rd32(hdr + IBASE_OVL_HDR_FLEN_OFF) != O.file_len)
        r = IBASE_OVL_CORRUPT;
    if (r) { ovl_close(); return r; }
    /* The stamp is the CRC-32 of everything after the header: one pass proves
     * the directory and every module and relocation table. */
    for (pos = IBASE_OVL_HDR_SIZE; pos < O.file_len; pos += i) {
        i = O.file_len - pos;
        if (i > sizeof buf) i = sizeof buf;
        if (ovl_read_at(pos, buf, i)) { ovl_close(); return IBASE_OVL_MISSING; }
        crc = ibase_ovl_crc32(crc, buf, i);
    }
#ifndef IBASE_OVL_MUTATE_NO_CRC
    if (crc != ovl_rd32(hdr + IBASE_OVL_HDR_STAMP_OFF)) { ovl_close(); return IBASE_OVL_CORRUPT; }
#endif
    if (ovl_read_at(IBASE_OVL_HDR_SIZE, O.dir, IBASE_OVL_COUNT * IBASE_OVL_DIR_SIZE)) {
        ovl_close();
        return IBASE_OVL_MISSING;
    }
    for (i = 0; i < IBASE_OVL_COUNT; i++) {
        if (ibase_ovl_check_dir(O.dir + i * IBASE_OVL_DIR_SIZE, ovl_slot_len(), O.file_len,
                                &off, &len, &nrel, &ecrc)) {
            ovl_close();
            return IBASE_OVL_CORRUPT;
        }
    }
    return IBASE_OVL_OK;
}

/* The refusal: print the prompt, wait for ENTER (1 = retry) or Ctrl-C / end
 * of input (0 = abort). Other keys are ignored. */
static int ovl_prompt(void)
{
    ibase_pal_t *pal = O.pal;
    static const char prompt[] = IBASE_OVL_PROMPT;
    pal->conout(pal, prompt, sizeof prompt - 1u);
    for (;;) {
        int32_t k = pal->conin_char ? pal->conin_char(pal) : -1;
        if (k == (int32_t)IBASE_OVL_KEY_RETRY) return 1;
        if (k < 0 || k == (int32_t)IBASE_OVL_KEY_ABORT) {
            pal->conout(pal, "\r\n", 2u);
            return 0;
        }
    }
}

int ibase_ovl_open(ibase_pal_t *pal)
{
    ibase_ovl_validate_pal(pal);
    O.pal = pal;
    O.fd = -1;
    O.loaded = OVL_NONE;
    O.depth = 0u;
#ifdef IBASE_OVL_MUTATE_SKIP_OPEN
    /* MUTANT (Rule 6; test-ibase-ovl-emu-mutant only): no start-up check --
     * a missing INITBASE.OVL reaches the dot prompt; the missing leg goes RED. */
    return 0;
#endif
    while (ovl_ready() != IBASE_OVL_OK) {
        if (!ovl_prompt()) return -1;
    }
    return 0;
}

/* Read overlay `id` into the slot, CRC it, relocate it. 0 = loaded. */
static int ovl_fetch(uint32_t id)
{
    uint8_t *slot = (uint8_t *)__ovl_slot;
    uint8_t rel[256];
    uint32_t off, len, nrel, ecrc, crc, done, n;
    const ibase_ovl_slot_hdr *h = (const ibase_ovl_slot_hdr *)(const void *)__ovl_slot;

    O.loaded = OVL_NONE;
    if (O.fd < 0 && ovl_ready()) return -1;
    if (ibase_ovl_check_dir(O.dir + id * IBASE_OVL_DIR_SIZE, ovl_slot_len(), O.file_len,
                            &off, &len, &nrel, &ecrc))
        return -1;
    if (ovl_read_at(off, slot, len)) return -1;
    crc = ibase_ovl_crc32(0u, slot, len);
    /* The relocation table follows the module; read, CRC and apply it in
     * chunks (every chunk bounds-checked before any write). */
    for (done = 0; done < nrel; done += n) {
        n = nrel - done;
        if (n > sizeof rel / 4u) n = sizeof rel / 4u;
        if (O.pal->read(O.pal, O.fd, rel, n * 4u) != (int32_t)(n * 4u)) return -1;
        crc = ibase_ovl_crc32(crc, rel, n * 4u);
        if (ibase_ovl_relocate(slot, len, rel, n, (uint32_t)(uintptr_t)__ibase_base)) return -1;
    }
#ifndef IBASE_OVL_MUTATE_NO_CRC
    if (crc != ecrc) return -1;
#endif
    if (h->magic != IBASE_OVL_MAGIC || h->id != id) return -1;
    if ((uintptr_t)h->entry < (uintptr_t)slot + IBASE_OVL_SLOT_HDR_SIZE ||
        (uintptr_t)h->entry >= (uintptr_t)slot + len)
        ovl_panic("entry outside slot",(uintptr_t)h->entry);
    O.loaded = id;
    return 0;
}

int ibase_ovl_call(unsigned id, int k, xb_interp *ip, const char *args, int *ec)
{
#if defined(IBASE_OVL_MUTATE_SKIP_OPEN) || defined(IBASE_OVL_MUTATE_NO_STAMP)
    /* Cold SET construction now loads an overlay before the first prompt
     * (initech-0spyf/fzsvo). These startup-verification fault injections must
     * not enter unchecked code: SKIP_OPEN would be rescued by its second
     * check, and NO_STAMP would execute a foreign resident ABI. */
    if(id==IBASE_OVL_SET && k==1)return 0;
#endif
    const ibase_ovl_slot_hdr *h = (const ibase_ovl_slot_hdr *)(const void *)__ovl_slot;
    int rc;

    O.requested=id;
    if (id >= IBASE_OVL_COUNT) ovl_panic("invalid id",id);
    if (O.depth) ovl_panic("live swap",0u);
    IBASE_RETAIN(ip);
    ibase_ovl_validate_pal(O.pal);
    ibase_ovl_validate_ctx(xb_interp_ctx(ip));
#ifdef IBASE_OVL_MUTATE_NO_SWAP
    /* MUTANT (Rule 6; test-ibase-ovl-swap-emu-mutant only): load once, never
     * swap -- whatever overlay came first answers every later call. */
    if (O.loaded == OVL_NONE) {
#else
    if (O.loaded != id) {
#endif
        /* Entry above forbids reentry, including an indirect same-id call. */
        while (ovl_fetch(id)) {
            ovl_close();
            if (!ovl_prompt()) {
                /* Abort: close every work area (headers and indexes flushed,
                 * exactly as QUIT does) and leave. */
                xb_interp_free(ip);
                pal_dos_terminate(1u);
            }
        }
    }
    O.depth++;
    rc = h->entry(k, ip, args, ec);
    /* Public PAL/ctx fields may have been written directly by overlay code.
     * Reject escape before control leaves the active overlay, not after a swap. */
    ibase_ovl_validate_pal(O.pal);
    ibase_ovl_validate_ctx(xb_interp_ctx(ip));
    O.depth--;
    return rc;
}
#endif /* IBASE_OVERLAYS */
