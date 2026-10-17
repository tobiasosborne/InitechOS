/*
 * harness/tbx/tbxhost32.c -- run a REAL InitechMZ disk tenant on the host
 * and print the ordered Toolbox requests it makes (factory C; beads
 * initech-tdnl.22 R5.2, initech-tdnl.23 R5.3).
 *
 *   tbxhost32 <TENANT.EXE> <events.txt>
 *
 * THE LOAD mirrors the kernel's (os/milton/mz.c mz_parse_header +
 * mz_apply_relocs; spec/toolbox_gate.h Sec 8): the InitechMZ tag is
 * required; image_bytes from e_cp/e_cblp; the module is [header, image);
 * the block is [ PSP 256 | module | e_minalloc*16 zeroed BSS ] at a fixed
 * base; every relocation site gets the module's base added; entry = base +
 * e_ip (e_cs 0, DEC-08a.2), called with EBX = the PSP.
 *
 * THE GATE (tbx32_rt.c catches the real `int 0x81`): every request is
 * decoded the way os/flair/tbxgate.c decodes it -- the registration record
 * validated (tag, imageBase == the load base, imageLen == the module
 * length, namePtr / evBufPtr / eventProc inside the module), strings and the
 * MenuBar graph read from inside the module, Scrap buffers from the module
 * or its BSS -- and handed to the model (tbxmodel.c), which prints the trace
 * line and answers.
 *
 * THE EVENTS (DEC-AC3-3 push-callback): after the entry returns, each line
 * of events.txt is copied into the registered evBufPtr and eventProc is
 * called (EVT line first); delivery stops at EXIT. A REGISTER that never
 * happened or was refused delivers nothing (the kernel admits nothing).
 *
 * Exit: 0 = ran to the end; 2 = usage / load error; 3 = tenant crash.
 * ASCII-clean (Rule 12). Deterministic: no address reaches the trace.
 */
#include "tbx32_rt.h"
#include "tbxmodel.h"

#define LOAD_BLOCK  0x30000000u
#define PSP_BYTES   TBX_TENANT_PSP_BYTES
#define FILE_CAP    (TBX_TENANT_IMAGE_MAX + 4096)
#define EVENTS_CAP  (4 * 1024 * 1024)
#define EV_MAX      200000

static char    g_file[FILE_CAP];
static char    g_evtext[EVENTS_CAP];
static int32_t g_ev[EV_MAX][6];

static uint32_t g_base, g_modlen, g_bsslen;
static uint32_t g_evbuf, g_proc;

static uint16_t rd16(const char *p)
{
    return (uint16_t)((uint8_t)p[0] | ((uint16_t)(uint8_t)p[1] << 8));
}

static uint32_t rd32(uint32_t a)
{
    return *(const volatile uint32_t *)(uintptr_t)a;
}

static int in_module(uint32_t a, uint32_t len)
{
    uint32_t hi = g_base + g_modlen;
    return a >= g_base && a <= hi && len <= hi - a;
}

static int in_block(uint32_t a, uint32_t len)     /* module or its BSS */
{
    uint32_t hi = g_base + g_modlen + g_bsslen;
    return a >= g_base && a <= hi && len <= hi - a;
}

/* A NUL-terminated string wholly in the module, at most max bytes with its
 * NUL (the kernel's image_str): copied into out (>= max bytes). */
static int module_str(uint32_t a, uint32_t max, char *out)
{
    uint32_t i;
    for (i = 0; i < max; i++) {
        if (!in_module(a + i, 1u)) return -1;
        out[i] = *(const char *)(uintptr_t)(a + i);
        if (out[i] == 0) return (int)i;
    }
    return -1;
}

static int32_t bad_string(const char *what)
{
    tm_bad(what);
    return TBX_ERR_BADARG;
}

static int32_t do_register(uint32_t rec)
{
    char name[FLAIR_TENANT_NAME_MAX + 1];
    uint32_t namep, evb, proc;
    if (!in_module(rec, FLAIR_TENANT_REC_BYTES))
        return tm_register_bad("rec-outside-image");
    if (rd32(rec + FLAIR_TENANT_REC_TAG_OFF) != FLAIR_TENANT_REC_TAG)
        return tm_register_bad("tag");
    if (rd32(rec + FLAIR_TENANT_REC_BASE_OFF) != g_base ||
        rd32(rec + FLAIR_TENANT_REC_LEN_OFF) != g_modlen)
        return tm_register_bad("image-mismatch");
    namep = rd32(rec + FLAIR_TENANT_REC_NAME_OFF);
    evb   = rd32(rec + FLAIR_TENANT_REC_EVB_OFF);
    proc  = rd32(rec + FLAIR_TENANT_REC_PROC_OFF);
    if (module_str(namep, FLAIR_TENANT_NAME_MAX, name) <= 0 ||
        !in_module(evb, FLAIR_EVBUF_BYTES) || !in_module(proc, 1u))
        return tm_register_bad("pointer-outside-image");
    if (tm_register(name) == TBX_OK) {
        g_evbuf = evb;
        g_proc = proc;
        return TBX_OK;
    }
    return TBX_ERR_BUSY;
}

static tm_mbar_t g_bar;

static int32_t do_setmbar(uint32_t bp)
{
    uint32_t menus, items;
    int i, j;
    if (!in_module(bp, TBX_MBAR_BYTES)) return bad_string("SETMBAR bar outside image");
    menus = rd32(bp + TBX_MBAR_MENUS_OFF);
    g_bar.n_menus = *(const uint16_t *)(uintptr_t)(bp + TBX_MBAR_COUNT_OFF);
    g_bar.has_apple = *(const uint8_t *)(uintptr_t)(bp + TBX_MBAR_APPLE_OFF);
    if (g_bar.n_menus > (int)TBX_MBAR_MAX_MENUS) return bad_string("SETMBAR n_menus");
    if (g_bar.n_menus > 0 && !in_module(menus, (uint32_t)g_bar.n_menus * TBX_MINFO_BYTES))
        return bad_string("SETMBAR menus outside image");
    for (i = 0; i < g_bar.n_menus; i++) {
        uint32_t mi = menus + (uint32_t)i * TBX_MINFO_BYTES;
        tm_menu_t *m = &g_bar.menu[i];
        m->id = *(const int16_t *)(uintptr_t)(mi + TBX_MINFO_ID_OFF);
        if (module_str(rd32(mi + TBX_MINFO_TITLE_OFF), TBX_MBAR_STR_MAX, m->title) < 0)
            return bad_string("SETMBAR title");
        items = rd32(mi + TBX_MINFO_ITEMS_OFF);
        m->n_items = *(const uint16_t *)(uintptr_t)(mi + TBX_MINFO_COUNT_OFF);
        if (m->n_items > (int)TBX_MBAR_MAX_ITEMS) return bad_string("SETMBAR n_items");
        if (m->n_items > 0 && !in_module(items, (uint32_t)m->n_items * TBX_MITEM_BYTES))
            return bad_string("SETMBAR items outside image");
        for (j = 0; j < m->n_items; j++) {
            uint32_t it = items + (uint32_t)j * TBX_MITEM_BYTES;
            tm_item_t *x = &m->item[j];
            if (module_str(rd32(it + TBX_MITEM_TEXT_OFF), TBX_MBAR_STR_MAX, x->text) < 0)
                return bad_string("SETMBAR item text");
            x->mark    = *(const uint8_t *)(uintptr_t)(it + TBX_MITEM_MARK_OFF);
            x->cmd     = *(const uint8_t *)(uintptr_t)(it + TBX_MITEM_CMD_OFF);
            x->style   = *(const uint8_t *)(uintptr_t)(it + TBX_MITEM_STYLE_OFF);
            x->enabled = *(const uint8_t *)(uintptr_t)(it + TBX_MITEM_ENABLED_OFF);
            x->divider = *(const uint8_t *)(uintptr_t)(it + TBX_MITEM_DIVIDER_OFF);
        }
    }
    return tm_setmbar(&g_bar);
}

static char s0[300], s1[300], s2[300], s3[300];

static int32_t on_trap(uint32_t ax, const uint32_t *a, uint32_t esp, uint32_t ebp)
{
    (void)esp; (void)ebp;
    switch (ax) {
    case TBX_REGISTER:
        return do_register(a[0]);
    case TBX_EXIT:
        return tm_exit((int32_t)a[0]);
    case TBX_NEWWINDOW:
        return tm_newwindow((int32_t)a[0], (int32_t)a[1], (int32_t)a[2],
                            (int32_t)a[3], a[4]);
    case TBX_SETWTITLE:
        if (module_str(a[1], 64u, s0) < 0) return bad_string("SETWTITLE string");
        return tm_setwtitle((int32_t)a[0], s0);
    case TBX_CONTENTORIGIN:
        return tm_contentorigin((int32_t)a[0]);
    case TBX_GLOBALTOLOCAL:
        return tm_globaltolocal((int32_t)a[0], (int32_t)a[1], (int32_t)a[2]);
    case TBX_TEXTDRAW:
        if (module_str(a[3], 81u, s0) < 0) return bad_string("TEXTDRAW string");
        return tm_textdraw((int32_t)a[0], (int32_t)a[1], (int32_t)a[2], s0, a[4], a[5]);
    case TBX_DRAWCELLS:
        if (module_str(a[3], TBX_CELLS_MAX_RUN + 1u, s0) < 0)
            return bad_string("DRAWCELLS string");
        return tm_drawcells((int32_t)a[0], (int32_t)a[1], (int32_t)a[2], s0, a[4], a[5]);
    case TBX_FILLRECT:
        return tm_fillrect((int32_t)a[0], (int32_t)a[1], (int32_t)a[2],
                           (int32_t)a[3], (int32_t)a[4], a[5]);
    case TBX_SETMBAR:
        return do_setmbar(a[0]);
    case TBX_DRAWMENUBAR:
        return tm_drawmenubar();
    case TBX_ALERT:
        if (module_str(a[0], 3u * TBX_ALERT_LINE_MAX, s0) < 0 ||
            module_str(a[1], TBX_ALERT_LABEL_MAX, s1) < 0 ||
            (a[2] != 0u && module_str(a[2], TBX_ALERT_LABEL_MAX, s2) < 0) ||
            (a[3] != 0u && module_str(a[3], TBX_ALERT_LABEL_MAX, s3) < 0))
            return bad_string("ALERT string");
        return tm_alert(s0, s1, a[2] ? s2 : 0, a[3] ? s3 : 0);
    case TBX_SCRAP_PUT:
        if (a[1] > 0u && !in_block(a[0], a[1])) return bad_string("SCRAP-PUT buffer");
        return tm_scrap_put((const uint8_t *)(uintptr_t)a[0], a[1]);
    case TBX_SCRAP_GET:
        if (a[0] != 0u && !in_block(a[0], a[1])) return bad_string("SCRAP-GET buffer");
        return tm_scrap_get((uint8_t *)(uintptr_t)a[0], a[1]);
    default:
        return tm_badcode(ax);
    }
}

static char g_serial[256];
static int  g_serialn;

static void on_serial(uint8_t c)
{
    if (c == '\n' || g_serialn == (int)sizeof g_serial - 1) {
        g_serial[g_serialn] = 0;
        tm_puts("SERIAL ");
        tm_putq(g_serial);
        tm_puts("\n");
        g_serialn = 0;
        if (c == '\n') return;
    }
    g_serial[g_serialn++] = (char)c;
}

int tbx32_main(int argc, char **argv)
{
    int flen, elen, nev, i, k;
    uint16_t cblp, cp, crlc, cparhdr, minalloc, ip, cs, lfarlc;
    uint32_t image_bytes, header_bytes, psp;
    char *blk;

    if (argc != 3) tbx32_die("usage: tbxhost32 <TENANT.EXE> <events.txt>");
    flen = tbx32_read_file(argv[1], g_file, FILE_CAP);
    if (flen < 0x20) tbx32_die("cannot read the tenant (or it is not an MZ)");
    elen = tbx32_read_file(argv[2], g_evtext, EVENTS_CAP);
    if (elen < 0) tbx32_die("cannot read the event script");
    nev = tm_parse_events(g_evtext, elen, g_ev, EV_MAX);
    if (nev < 0) tbx32_die("malformed event script");

    /* os/milton/mz.c mz_parse_header, field for field */
    if (g_file[0] != 'M' || g_file[1] != 'Z') tbx32_die("no MZ signature");
    if (rd16(g_file + 0x1C) != 0x4943u) tbx32_die("untagged MZ (not InitechMZ)");
    cblp = rd16(g_file + 2);  cp = rd16(g_file + 4);   crlc = rd16(g_file + 6);
    cparhdr = rd16(g_file + 8); minalloc = rd16(g_file + 10);
    ip = rd16(g_file + 0x14); cs = rd16(g_file + 0x16); lfarlc = rd16(g_file + 0x18);
    if (cblp == 0u) image_bytes = (uint32_t)cp * 512u;
    else if (cp == 0u) tbx32_die("e_cp 0 with e_cblp");
    else image_bytes = (uint32_t)(cp - 1u) * 512u + cblp;
    header_bytes = (uint32_t)cparhdr * 16u;
    if (header_bytes > image_bytes || (uint32_t)flen < image_bytes)
        tbx32_die("inconsistent MZ sizes");
    if ((uint32_t)lfarlc + (uint32_t)crlc * 4u > (uint32_t)flen)
        tbx32_die("relocation table past the file");
    if (cs != 0u) tbx32_die("e_cs must be 0 (DEC-08a.2)");
    if ((uint32_t)minalloc * 16u > TBX_TENANT_BSS_MAX) tbx32_die("BSS over TBX_TENANT_BSS_MAX");

    g_modlen = image_bytes - header_bytes;
    g_bsslen = (uint32_t)minalloc * 16u;
    blk = (char *)tbx32_map_fixed(LOAD_BLOCK, 0x100000u);
    if (!blk) tbx32_die("cannot map the load block");
    psp = LOAD_BLOCK;
    g_base = LOAD_BLOCK + PSP_BYTES;
    memset(blk, 0, PSP_BYTES + g_modlen + g_bsslen + 16u);
    memcpy((char *)(uintptr_t)g_base, g_file + header_bytes, g_modlen);
    for (i = 0; i < (int)crlc; i++) {
        uint32_t off = (uint32_t)(uint8_t)g_file[lfarlc + 4 * i] |
                       ((uint32_t)(uint8_t)g_file[lfarlc + 4 * i + 1] << 8) |
                       ((uint32_t)(uint8_t)g_file[lfarlc + 4 * i + 2] << 16) |
                       ((uint32_t)(uint8_t)g_file[lfarlc + 4 * i + 3] << 24);
        if (off + 4u > g_modlen) tbx32_die("relocation site outside the module");
        *(uint32_t *)(uintptr_t)(g_base + off) += g_base;
    }

    tm_reset();
    tbx32_on_trap = on_trap;
    tbx32_on_serial = on_serial;
    tbx32_install();
    tm_puts("TBX ENTRY\n");
    (void)tbx32_call(g_base + ip, psp);
    for (i = 0; i < nev && tm_registered() && !tm_exited(); i++) {
        for (k = 0; k < 6; k++)
            *(int32_t *)(uintptr_t)(g_evbuf + 4u * (uint32_t)k) = g_ev[i][k];
        tm_event(g_ev[i]);
        (void)tbx32_call(g_proc, psp);
    }
    tm_puts("TBX END\n");
    return 0;
}
