/*
 * os/samir/core/ovl.c -- the SAMIR overlay manager (InitechBase overlays
 * Phase B, epic initech-5etom; design: docs/design/samir-overlays.md).
 *
 * THE ARTIFACT (Law 3): freestanding C; all OS contact through the PAL.
 *
 * Two parts:
 *   1. Pure helpers (CRC-32, header/directory checks, relocation). Compiled in
 *      every build that lists this file; the host oracle test_samir_ovl.c
 *      drives them against literal bytes.
 *   2. The Milton manager (SAMIR_OVERLAYS): one fixed slot inside SAMIR's own
 *      image window (samir.ld), filled from SAMIR.OVL on demand the way
 *      DBASE.EXE fills its overlay area from DBASE.OVL -- open, LSEEK, READ
 *      (DBASE.EXE file offsets 0x204B4 / 0x204FC / 0x20553; spec/samir/
 *      ovl_format.h) -- then relocated by the resident load base B exactly as
 *      the InitechMZ loader relocated the resident module (ADR-0003 DEC-08a.1).
 *
 * Failure model (Rule 2; design note Sec 6): SAMIR.OVL is verified whole at
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
#include "samir/ovl.h"
#include "samir/ovl_format.h"   /* LOCKED spec-data, on -Ispec */
#include "samir/rt.h"
#include "samir/lifetime.h"

static uint32_t ovl_rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

uint32_t samir_ovl_crc32(uint32_t crc, const void *buf, uint32_t n)
{
    const uint8_t *p = (const uint8_t *)buf;
    crc = ~crc;
    while (n--) {
        crc ^= *p++;
        for (int i = 0; i < 8; i++)
            crc = (crc >> 1) ^ (SAMIR_OVL_CRC_POLY & (0u - (crc & 1u)));
    }
    return ~crc;
}

int samir_ovl_check_header(const uint8_t *h, uint32_t stamp, uint32_t slot_len,
                           uint32_t count)
{
#ifndef SAMIR_OVL_MUTATE_NO_STAMP
    if (rt_memcmp(h + SAMIR_OVL_HDR_SIG_OFF, SAMIR_OVL_SIG, SAMIR_OVL_SIG_LEN) ||
        (uint32_t)(h[SAMIR_OVL_HDR_VER_OFF] | (h[SAMIR_OVL_HDR_VER_OFF + 1] << 8)) != SAMIR_OVL_VERSION ||
        (uint32_t)(h[SAMIR_OVL_HDR_COUNT_OFF] | (h[SAMIR_OVL_HDR_COUNT_OFF + 1] << 8)) != count ||
        ovl_rd32(h + SAMIR_OVL_HDR_STAMP_OFF) != stamp ||
        ovl_rd32(h + SAMIR_OVL_HDR_SLOT_OFF) != slot_len)
        return SAMIR_OVL_FOREIGN;
#else
    /* MUTANT (Rule 6; test-samir-ovl-emu-mutant only): accept an overlay file
     * from any build -- the stale-file emu leg must go RED. */
    (void)stamp; (void)slot_len; (void)count;
#endif
    if (ovl_rd32(h + SAMIR_OVL_HDR_FLEN_OFF) < SAMIR_OVL_HDR_SIZE)
        return SAMIR_OVL_CORRUPT;
    return SAMIR_OVL_OK;
}

int samir_ovl_check_dir(const uint8_t *e, uint32_t slot_len, uint32_t file_len,
                        uint32_t *off, uint32_t *len, uint32_t *nrel, uint32_t *crc)
{
    uint32_t o = ovl_rd32(e + SAMIR_OVL_DIR_OFF_OFF);
    uint32_t l = ovl_rd32(e + SAMIR_OVL_DIR_LEN_OFF);
    uint32_t r = ovl_rd32(e + SAMIR_OVL_DIR_NREL_OFF);
    /* The module must carry at least the slot header, fit the slot, and the
     * module plus its relocation table must lie inside the file (no wrap). */
    if (l < SAMIR_OVL_SLOT_HDR_SIZE || l > slot_len || r > (file_len >> 2) ||
        o < SAMIR_OVL_HDR_SIZE || o > file_len || l > file_len - o ||
        r * 4u > file_len - o - l)
        return SAMIR_OVL_CORRUPT;
    *off = o; *len = l; *nrel = r; *crc = ovl_rd32(e + SAMIR_OVL_DIR_CRC_OFF);
    return SAMIR_OVL_OK;
}

int samir_ovl_relocate(uint8_t *slot, uint32_t len, const uint8_t *rel,
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
#ifndef SAMIR_OVL_MUTATE_NO_RELOC
        v += base;
#else
        /* MUTANT (Rule 6; test-samir-ovl-mutant / swap-emu-mutant only): the
         * overlay is used unrelocated. */
        (void)base;
#endif
        p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
        p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
    }
    return 0;
}

#ifdef SAMIR_OVERLAYS
/* ---- The Milton manager ---------------------------------------------------
 *
 * Link-time facts (os/samir/boot/samir.ld). Every one is reached through an
 * absolute reference the InitechMZ loader relocated, so it reads as a run-time
 * address: __samir_base is the resident load base B, __ovl_slot the slot.
 */
extern char __samir_base[];
extern char __ovl_slot[];
extern char __ovl_slot_end[];
/* Patched by tools/samir_ovl_pack.c with the SAMIR.OVL stamp (the CRC-32 of the
 * file body) after the link; volatile so the compiler never folds the 0. */
const volatile uint32_t samir_ovl_stamp = 0u;

/* pal_milton.c: the only int 0x21 TU (ADR-0009 DEC-06). */
void pal_milton_terminate(uint8_t code) __attribute__((noreturn));
void pal_milton_overlay_panic(const char *,uint32_t,uint32_t,uint32_t,uintptr_t)
    __attribute__((noreturn));

#define OVL_NONE 0xFFFFFFFFu

static struct {
    samir_pal_t *pal;
    pal_fd       fd;          /* SAMIR.OVL, open for the session (or -1)  */
    uint32_t     loaded;      /* id in the slot, or OVL_NONE              */
    uint32_t     requested;
    uint32_t     depth;       /* overlay frames on the stack              */
    uint32_t     file_len;
    uint8_t      dir[SAMIR_OVL_MAX * SAMIR_OVL_DIR_SIZE];
} O;

static void __attribute__((noreturn)) ovl_panic(const char *why,uintptr_t p)
{
    pal_milton_overlay_panic(why,O.loaded,O.requested,O.depth,p);
}
void samir_ovl_require_resident(uintptr_t p,const char *where)
{
    if (p >= (uintptr_t)__ovl_slot && p < (uintptr_t)__ovl_slot_end)
        ovl_panic(where,p);
}
void samir_ovl_validate_pal(const samir_pal_t *pal)
{
    SAMIR_RETAIN(pal);
    if (!pal) return;
    SAMIR_RETAIN(pal->open);
    SAMIR_RETAIN(pal->close);
    SAMIR_RETAIN(pal->read);
    SAMIR_RETAIN(pal->write);
    SAMIR_RETAIN(pal->seek);
    SAMIR_RETAIN(pal->remove);
    SAMIR_RETAIN(pal->rename);
    SAMIR_RETAIN(pal->conout);
    SAMIR_RETAIN(pal->conin_line);
    SAMIR_RETAIN(pal->conin_char);
    SAMIR_RETAIN(pal->gotoxy);
    SAMIR_RETAIN(pal->set_attr);
    SAMIR_RETAIN(pal->today);
    SAMIR_RETAIN(pal->alloc);
    SAMIR_RETAIN(pal->reset);
    SAMIR_RETAIN(pal->acquire);
    SAMIR_RETAIN(pal->release);
    SAMIR_RETAIN(pal->status);
    SAMIR_RETAIN(pal->conin_line_noecho);
    SAMIR_RETAIN(pal->exec);
    SAMIR_RETAIN(pal->conin_escape);
    SAMIR_RETAIN(pal->curdrive);
    SAMIR_RETAIN(pal->getenv);
    SAMIR_RETAIN(pal->os_version);
    SAMIR_RETAIN(pal->diskspace);
    SAMIR_RETAIN(pal->conin_poll);
    SAMIR_RETAIN(pal->time);
    SAMIR_RETAIN(pal->iscolor);
    SAMIR_RETAIN(pal->say_screen);
    SAMIR_RETAIN(pal->printout);
}
void samir_ovl_validate_ctx(const xb_ctx *c)
{
    SAMIR_RETAIN(c);
    if (!c) return;
    SAMIR_RETAIN(c->mirror_output);
    SAMIR_RETAIN(c->mirror_user);
    SAMIR_RETAIN(c->resolve);
    SAMIR_RETAIN(c->user);
    SAMIR_RETAIN(c->scratch);
    SAMIR_RETAIN(c->dbcur);
    SAMIR_RETAIN(c->dbcur_user);
    SAMIR_RETAIN(c->file_pal);
    SAMIR_RETAIN(c->record_size);
    SAMIR_RETAIN(c->on_error);
    SAMIR_RETAIN(c->message);
    SAMIR_RETAIN(c->on_escape);
    SAMIR_RETAIN(c->program_name);
    SAMIR_RETAIN(c->on_error_text);
    SAMIR_RETAIN(c->on_escape_text);
    samir_ovl_validate_pal(c->file_pal);
    if(c->dbcur) {
        SAMIR_RETAIN(c->dbcur->recno);
        SAMIR_RETAIN(c->dbcur->reccount);
        SAMIR_RETAIN(c->dbcur->eof);
        SAMIR_RETAIN(c->dbcur->bof);
        SAMIR_RETAIN(c->dbcur->found);
        SAMIR_RETAIN(c->dbcur->deleted);
        SAMIR_RETAIN(c->dbcur->field_name);
        SAMIR_RETAIN(c->dbcur->dbf_name);
        SAMIR_RETAIN(c->dbcur->file_exists);
        SAMIR_RETAIN(c->dbcur->last_update);
        SAMIR_RETAIN(c->dbcur->index_name);
    }
}

static uint32_t ovl_slot_len(void)
{
    return (uint32_t)(__ovl_slot_end - __ovl_slot);
}

static int ovl_read_at(uint32_t off, void *buf, uint32_t n)
{
    samir_pal_t *pal = O.pal;
    if (pal->seek(pal, O.fd, (int32_t)off, PAL_SEEK_SET) != (int32_t)off) return -1;
    return pal->read(pal, O.fd, buf, n) == (int32_t)n ? 0 : -1;
}

static void ovl_close(void)
{
    if (O.fd >= 0) (void)O.pal->close(O.pal, O.fd);
    O.fd = -1;
    O.loaded = OVL_NONE;
}

/* Open SAMIR.OVL and verify all of it. 0 = ready, else a SAMIR_OVL_* reason
 * (the file is closed again on failure). */
static int ovl_ready(void)
{
    samir_pal_t *pal = O.pal;
    uint8_t hdr[SAMIR_OVL_HDR_SIZE];
    uint8_t buf[256];
    uint32_t pos, crc = 0u, i, off, len, nrel, ecrc;
    int32_t end;
    int r;

    ovl_close();
    O.fd = pal->open(pal, SAMIR_OVL_FILE_NAME, PAL_RD);
    if (O.fd < 0) { O.fd = -1; return SAMIR_OVL_MISSING; }
    end = pal->seek(pal, O.fd, 0, PAL_SEEK_END);
    if (end < (int32_t)SAMIR_OVL_HDR_SIZE || ovl_read_at(0u, hdr, sizeof hdr)) {
        ovl_close();
        return SAMIR_OVL_MISSING;
    }
    O.file_len = (uint32_t)end;
    r = samir_ovl_check_header(hdr, samir_ovl_stamp, ovl_slot_len(), SAMIR_OVL_COUNT);
    if (r == SAMIR_OVL_OK && ovl_rd32(hdr + SAMIR_OVL_HDR_FLEN_OFF) != O.file_len)
        r = SAMIR_OVL_CORRUPT;
    if (r) { ovl_close(); return r; }
    /* The stamp is the CRC-32 of everything after the header: one pass proves
     * the directory and every module and relocation table. */
    for (pos = SAMIR_OVL_HDR_SIZE; pos < O.file_len; pos += i) {
        i = O.file_len - pos;
        if (i > sizeof buf) i = sizeof buf;
        if (ovl_read_at(pos, buf, i)) { ovl_close(); return SAMIR_OVL_MISSING; }
        crc = samir_ovl_crc32(crc, buf, i);
    }
#ifndef SAMIR_OVL_MUTATE_NO_CRC
    if (crc != ovl_rd32(hdr + SAMIR_OVL_HDR_STAMP_OFF)) { ovl_close(); return SAMIR_OVL_CORRUPT; }
#endif
    if (ovl_read_at(SAMIR_OVL_HDR_SIZE, O.dir, SAMIR_OVL_COUNT * SAMIR_OVL_DIR_SIZE)) {
        ovl_close();
        return SAMIR_OVL_MISSING;
    }
    for (i = 0; i < SAMIR_OVL_COUNT; i++) {
        if (samir_ovl_check_dir(O.dir + i * SAMIR_OVL_DIR_SIZE, ovl_slot_len(), O.file_len,
                                &off, &len, &nrel, &ecrc)) {
            ovl_close();
            return SAMIR_OVL_CORRUPT;
        }
    }
    return SAMIR_OVL_OK;
}

/* The refusal: print the prompt, wait for ENTER (1 = retry) or Ctrl-C / end
 * of input (0 = abort). Other keys are ignored. */
static int ovl_prompt(void)
{
    samir_pal_t *pal = O.pal;
    static const char prompt[] = SAMIR_OVL_PROMPT;
    pal->conout(pal, prompt, sizeof prompt - 1u);
    for (;;) {
        int32_t k = pal->conin_char ? pal->conin_char(pal) : -1;
        if (k == (int32_t)SAMIR_OVL_KEY_RETRY) return 1;
        if (k < 0 || k == (int32_t)SAMIR_OVL_KEY_ABORT) {
            pal->conout(pal, "\r\n", 2u);
            return 0;
        }
    }
}

int samir_ovl_open(samir_pal_t *pal)
{
    samir_ovl_validate_pal(pal);
    O.pal = pal;
    O.fd = -1;
    O.loaded = OVL_NONE;
    O.depth = 0u;
#ifdef SAMIR_OVL_MUTATE_SKIP_OPEN
    /* MUTANT (Rule 6; test-samir-ovl-emu-mutant only): no start-up check --
     * a missing SAMIR.OVL reaches the dot prompt; the missing leg goes RED. */
    return 0;
#endif
    while (ovl_ready() != SAMIR_OVL_OK) {
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
    const samir_ovl_slot_hdr *h = (const samir_ovl_slot_hdr *)(const void *)__ovl_slot;

    O.loaded = OVL_NONE;
    if (O.fd < 0 && ovl_ready()) return -1;
    if (samir_ovl_check_dir(O.dir + id * SAMIR_OVL_DIR_SIZE, ovl_slot_len(), O.file_len,
                            &off, &len, &nrel, &ecrc))
        return -1;
    if (ovl_read_at(off, slot, len)) return -1;
    crc = samir_ovl_crc32(0u, slot, len);
    /* The relocation table follows the module; read, CRC and apply it in
     * chunks (every chunk bounds-checked before any write). */
    for (done = 0; done < nrel; done += n) {
        n = nrel - done;
        if (n > sizeof rel / 4u) n = sizeof rel / 4u;
        if (O.pal->read(O.pal, O.fd, rel, n * 4u) != (int32_t)(n * 4u)) return -1;
        crc = samir_ovl_crc32(crc, rel, n * 4u);
        if (samir_ovl_relocate(slot, len, rel, n, (uint32_t)(uintptr_t)__samir_base)) return -1;
    }
#ifndef SAMIR_OVL_MUTATE_NO_CRC
    if (crc != ecrc) return -1;
#endif
    if (h->magic != SAMIR_OVL_MAGIC || h->id != id) return -1;
    if ((uintptr_t)h->entry < (uintptr_t)slot + SAMIR_OVL_SLOT_HDR_SIZE ||
        (uintptr_t)h->entry >= (uintptr_t)slot + len)
        ovl_panic("entry outside slot",(uintptr_t)h->entry);
    O.loaded = id;
    return 0;
}

int samir_ovl_call(unsigned id, int k, xb_interp *ip, const char *args, int *ec)
{
    const samir_ovl_slot_hdr *h = (const samir_ovl_slot_hdr *)(const void *)__ovl_slot;
    int rc;

    O.requested=id;
    if (id >= SAMIR_OVL_COUNT) ovl_panic("invalid id",id);
    if (O.depth) ovl_panic("live swap",0u);
    SAMIR_RETAIN(ip);
    samir_ovl_validate_pal(O.pal);
    samir_ovl_validate_ctx(xb_interp_ctx(ip));
#ifdef SAMIR_OVL_MUTATE_NO_SWAP
    /* MUTANT (Rule 6; test-samir-ovl-swap-emu-mutant only): load once, never
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
                pal_milton_terminate(1u);
            }
        }
    }
    O.depth++;
    rc = h->entry(k, ip, args, ec);
    /* Public PAL/ctx fields may have been written directly by overlay code.
     * Reject escape before control leaves the active overlay, not after a swap. */
    samir_ovl_validate_pal(O.pal);
    samir_ovl_validate_ctx(xb_interp_ctx(ip));
    O.depth--;
    return rc;
}
#endif /* SAMIR_OVERLAYS */
