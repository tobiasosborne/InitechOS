/* edit.c -- III+ stacked EDIT/CHANGE and APPEND record forms.
 * Ref: dbase3-decomp/specs/full-screen/assist-ui-and-editors.md section 4;
 * specs/full-screen/say-get-read.md section 8; Using III+ U5-26 (APPEND).
 * Writes go through APPEND BLANK / REPLACE / DELETE / RECALL so the engine
 * owns DBF persistence, record-cache refresh, and open-index maintenance.
 */
#include "ui_apps.h"
#include "samir/kit.h"
#include "samir/workarea.h"
#include "samir/nav.h"
#include "samir/dbf.h"
#include "samir/set.h"
#include "samir/rt.h"

#define ED_FIELDS 128
#define ED_COL 11
#define ED_VISIBLE 21

typedef struct {
    kit *screen;
    xb_interp *ip;
    wa_env *env;
    dbf_table *table;
    int area, mode, fresh, deleted, old_deleted, typed;
    int nf, nshow, selected[ED_FIELDS], widths[ED_FIELDS];
    char *text[ED_FIELDS];
    char *storage;
    unsigned char changed[ED_FIELDS];
    uint32_t first, last, insert_at;
    char temp[11];
} edit_state;
/* Record text uses the PAL heap; only small navigation metadata is static. */
static edit_state ed;

static const char *ed_space(const char *s)
{ while (*s == ' ' || *s == '\t') ++s; return s; }

static int ed_error(int *ec, int code)
{ if (ec) *ec = code; return -INTERP_ERR_EVAL; }

static int ed_number(const char **src, uint32_t *out)
{
    const char *s = ed_space(*src);
    uint32_t n = 0;
    if (*s < '0' || *s > '9') return 0;
    do {
        unsigned d = (unsigned)(*s++ - '0');
        if (n > (0xffffffffu - d) / 10u) return 0;
        n = n * 10u + d;
    } while (*s >= '0' && *s <= '9');
    *src = ed_space(s); *out = n; return 1;
}

static int ed_field(const char *s, int len)
{
    int i;
    for (i = 0; i < ed.nf; ++i) {
        const char *name = dbf_field(ed.table, i)->name;
        int j = 0;
        while (j < len && name[j]) {
            char c = s[j];
            if (c >= 'a' && c <= 'z') c -= 32;
            if (c != name[j]) break;
            ++j;
        }
        if (j == len && !name[j]) return i;
    }
    return -1;
}

/* Scope and field selection are deliberately parsed before entering the kit. */
static int ed_options(const char *args, int *ec)
{
    const char *s = ed_space(args);
    uint32_t n, count = dbf_nrec(ed.table);
    int k, i;
    ed.first = 1; ed.last = count;
    if ((k = rt_kw_match(s, "ALL")) != 0) {
        s = ed_space(s + k);
        if (wa_nav_go_top(ed.env, ed.area) != NAV_OK) return ed_error(ec, 4);
    } else if ((k = rt_kw_match(s, "REST")) != 0) {
        ed.first = wa_recno(ed.env, ed.area);
        s = ed_space(s + k);
    } else if ((k = rt_kw_match(s, "NEXT")) != 0) {
        s = ed_space(s + k);
        if (!ed_number(&s, &n)) return ed_error(ec, 10);
        ed.first = wa_recno(ed.env, ed.area);
        ed.last = n && ed.first <= count && n - 1u < count - ed.first ? ed.first + n - 1u : count;
        if (!n) ed.last = 0;
    } else {
        k = rt_kw_match(s, "RECORD");
        if (k) s = ed_space(s + k);
        if (k || (*s >= '0' && *s <= '9')) {
            if (!ed_number(&s, &n) || n < 1 || n > count) return ed_error(ec, 4);
            if (wa_nav_goto(ed.env, ed.area, n) != NAV_OK) return ed_error(ec, 4);
            ed.first = ed.last = n;
        }
    }
    ed.nshow = 0;
    if ((k = rt_kw_match(s, "FIELDS")) != 0) {
        s = ed_space(s + k);
        do {
            const char *start = s;
            int f;
            while ((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z') ||
                   (*s >= '0' && *s <= '9') || *s == '_') ++s;
            f = ed_field(start, (int)(s - start));
            if (f < 0) return ed_error(ec, 9);
            for (i = 0; i < ed.nshow; ++i) if (ed.selected[i] == f) break;
            if (i == ed.nshow) ed.selected[ed.nshow++] = f;
            s = ed_space(s);
            if (*s != ',') break;
            s = ed_space(s + 1);
        } while (*s);
    } else {
        for (i = 0; i < ed.nf; ++i) ed.selected[ed.nshow++] = i;
    }
    if (*s || !ed.nshow) return ed_error(ec, 10);
    return CMD_OK;
}

static int ed_load(int fresh, int *ec)
{
    int i;
    xb_val values[ED_FIELDS];
    ed.fresh = fresh; ed.deleted = ed.old_deleted = ed.typed = 0;
    rt_memset(ed.changed, 0, sizeof(ed.changed));
    if (!fresh && dbf_read_rec(ed.table, wa_recno(ed.env, ed.area),
                              values, &ed.deleted) != DBF_OK)
        return ed_error(ec, 4);
    ed.old_deleted = ed.deleted;
    for (i = 0; i < ed.nf; ++i) {
        const dbf_field_t *f = dbf_field(ed.table, i);
        int w = f->field_len;
        char *b = ed.text[i];
        rt_memset(b, ' ', (uint32_t)(w > 10 ? w : 10));
        if (f->type == 'D') {
            w = (int)rt_date_format(fresh ? 0 : values[i].u.d,
                (unsigned)set_get_date_fmt(ed.ip), set_get_century(ed.ip), b, 0);
        } else if (f->type == 'M') {
            rt_memcpy(b, "memo", 4);
        } else if (!fresh) {
            xb_val *v = &values[i];
            if (f->type == 'C') {
                uint32_t len = v->u.c.len;
                if (len > (uint32_t)w) len = (uint32_t)w;
                rt_memcpy(b, v->u.c.p, len);
            } else if (f->type == 'N') {
                dec_format_even(v->u.n, w, f->dec_count, b);
            } else if (f->type == 'L') b[0] = v->t == XB_U ? '?' : (v->u.l ? 'T' : 'F');
        }
        ed.widths[i] = w;
        b[w] = 0;
    }
    return CMD_OK;
}

static int ed_date(const char *b, int w, xb_val *out)
{
    int part[3] = {0, 0, 0}, digits[3] = {0, 0, 0};
    int p = 0, i, any = 0, y, m, d, yy, mm, dd;
    set_date_fmt fmt = set_get_date_fmt(ed.ip);
    for (i = 0; i < w; ++i) {
        char c = b[i];
        if (c >= '0' && c <= '9') {
            any = 1; part[p] = part[p] * 10 + c - '0'; ++digits[p];
        } else if (c == '/' || c == '.' || c == '-') {
            if (++p > 2) return 0;
        } else if (c != ' ') return 0;
    }
    if (!any) { *out = xb_d(0); return 1; }
    if (p != 2 || !digits[0] || !digits[1] || !digits[2]) return 0;
    if (fmt == SET_DATE_ANSI || fmt == SET_DATE_JAPAN) {
        y = part[0]; m = part[1]; d = part[2]; i = 0;
    } else if (fmt == SET_DATE_AMERICAN || fmt == SET_DATE_USA) {
        m = part[0]; d = part[1]; y = part[2]; i = 2;
    } else { d = part[0]; m = part[1]; y = part[2]; i = 2; }
    if (digits[i] <= 2) y += 1900;
    if (y < 1900 || y > 2155 || m < 1 || m > 12 || d < 1 || d > 31) return 0;
    {
        int32_t a, c, e;
        int32_t jdn = jdn_from_ymd(y, m, d);
        ymd_from_jdn(jdn, &a, &c, &e);
        yy = a; mm = c; dd = e;
        if (yy != y || mm != m || dd != d) return 0;
        *out = xb_d((double)jdn);
    }
    return 1;
}

static int ed_value(int i, xb_val *out)
{
    const dbf_field_t *f = dbf_field(ed.table, i);
    const char *b = ed.text[i];
    int w = ed.widths[i], first = 0, end = w, j;
    while (first < end && b[first] == ' ') ++first;
    while (end > first && b[end - 1] == ' ') --end;
    if (f->type == 'C') { *out = xb_c(b, (uint16_t)w); return 1; }
    if (f->type == 'D') return ed_date(b, w, out);
    if (f->type == 'L') {
        char c = first < end ? b[first] : 'F';
        if (c >= 'a' && c <= 'z') c -= 32;
        if (c != 'T' && c != 'Y' && c != 'N' && c != 'F' && c != '?') return 0;
        *out = c == '?' ? xb_u() : xb_l(c == 'T' || c == 'Y'); return 1;
    }
    if (f->type == 'N') {
        int dot = 0;
        if (first < end && (b[first] == '-' || b[first] == '+')) ++first;
        for (j = first; j < end; ++j) {
            if (b[j] == '.' && !dot) { dot = 1; continue; }
            if (b[j] < '0' || b[j] > '9') return 0;
        }
        *out = xb_n(dec_parse(b, w)); return 1;
    }
    return 0;
}

static int ed_run(const char *command, int *ec)
{
    int rc = samir_do(ed.ip, command);
    if (rc < 0 && ec) *ec = samir_last_error(ed.ip);
    return rc;
}

/* INSERT is a physical shift through the DBF codec. Indexed INSERT is
 * refused before mutation; memo pointers cannot be shifted by dbf_replace(M). */
static int ed_insert(int *ec)
{
    uint32_t count = dbf_nrec(ed.table), r;
    xb_val values[ED_FIELDS];
    int i, deleted, rc = ed_run("APPEND BLANK", ec);
    if (rc < 0) return rc;
    for (r = count; r >= ed.insert_at && r > 0; --r) {
        rc = dbf_read_rec(ed.table, r, values, &deleted);
        if (rc != DBF_OK) return ed_error(ec, 25);
        for (i = 0; i < ed.nf; ++i)
            if (dbf_replace(ed.table, r + 1u, i, &values[i]) != DBF_OK)
                return ed_error(ec, 25);
        rc = deleted ? dbf_delete(ed.table, r + 1u) : dbf_recall(ed.table, r + 1u);
        if (rc != DBF_OK) return ed_error(ec, 25);
    }
    for (i = 0; i < ed.nf; ++i) {
        char type = dbf_field(ed.table, i)->type;
        xb_val blank = type == 'C' ? xb_c("", 0) :
                       type == 'N' ? xb_n(0) : type == 'D' ? xb_d(0) : xb_u();
        if (dbf_replace(ed.table, ed.insert_at, i, &blank) != DBF_OK)
            return ed_error(ec, 25);
    }
    if (dbf_recall(ed.table, ed.insert_at) != DBF_OK || dbf_flush(ed.table) != DBF_OK)
        return ed_error(ec, 25);
    (void)wa_refresh(ed.env, ed.area, ed.insert_at);
    wa_nav_reset(ed.area);
    ed.last = dbf_nrec(ed.table);
    return CMD_OK;
}

static int ed_save(int *ec)
{
    int i, rc;
    if (ed.fresh && !ed.typed) return CMD_OK;
    if (ed.fresh) {
        rc = ed.mode == UI_EDIT_INSERT ? ed_insert(ec) : ed_run("APPEND BLANK", ec);
        if (rc < 0) return rc;
        ed.fresh = 0;
    }
    for (i = 0; i < ed.nf; ++i) {
        xb_val v;
        char command[64];
        int n, j;
        if (!ed.changed[i] || dbf_field(ed.table, i)->type == 'M') continue;
        if (!ed_value(i, &v)) return ed_error(ec, 9);
        if (xb_interp_store(ed.ip, ed.temp, &v) != INTERP_OK) return ed_error(ec, 43);
        rt_memcpy(command, "REPLACE ", 8); n = 8;
        for (j = 0; dbf_field(ed.table, i)->name[j]; ++j)
            command[n++] = dbf_field(ed.table, i)->name[j];
        rt_memcpy(command + n, " WITH ", 6); n += 6;
        for (j = 0; ed.temp[j]; ++j) command[n++] = ed.temp[j];
        command[n] = 0;
        rc = ed_run(command, ec);
        if (rc < 0) return rc;
    }
    if (ed.deleted != ed.old_deleted) {
        rc = ed_run(ed.deleted ? "DELETE" : "RECALL", ec);
        if (rc < 0) return rc;
    }
    rt_memset(ed.changed, 0, sizeof(ed.changed));
    ed.old_deleted = ed.deleted;
    return CMD_OK;
}

static void ed_draw(int active)
{
    kit *k = ed.screen;
    int top = (active / ED_VISIBLE) * ED_VISIBLE;
    int row, at;
    kit_clear_rows(k, 0, 21);
    for (at = top, row = 0; at < ed.nshow && row < ED_VISIBLE; ++at, ++row) {
        int f = ed.selected[at], w = ed.widths[f];
        const dbf_field_t *field = dbf_field(ed.table, f);
        if (w > KIT_COLS - ED_COL) w = KIT_COLS - ED_COL;
        kit_put(k, row, 0, field->name, -1);
        kit_put_attr(k, row, ED_COL, KIT_ATTR_REVERSE, ed.text[f], w);
    }
    kit_status(k, ed.mode == UI_EDIT_EDIT ? "EDIT" : ed.mode == UI_EDIT_INSERT ? "INSERT" : "APPEND",
               dbf_name(ed.table), ed.fresh ? dbf_nrec(ed.table) + 1u : wa_recno(ed.env, ed.area),
               dbf_nrec(ed.table), 0);
    if (ed.deleted) kit_put_attr(k, 22, 61, KIT_ATTR_REVERSE, "Del", 3);
    kit_message(k, 23, "Up/Down: field   PgUp/PgDn: record   Ctrl-U: delete/recall");
    kit_message(k, 24, "Ctrl-W/Ctrl-End: save and exit   Esc: abandon record");
}

static int ed_has_text(int i)
{
    int j;
    for (j = 0; j < ed.widths[i]; ++j) if (ed.text[i][j] != ' ') return 1;
    return 0;
}

int ui_edit(xb_interp *ip, int mode, const char *args, int *ec)
{
    int rc, active = 0, i, has_editable = 0;
    uint32_t count, bytes = 0;
    kit screen;
    samir_pal_t *pal = xb_interp_pal(ip);
    if (ec) *ec = 0;
    rt_memset(&ed, 0, sizeof(ed));
    ed.screen = &screen;
    ed.ip = ip; ed.env = xb_interp_env(ip); ed.area = wa_selected(ed.env);
    ed.table = wa_table(ed.env, ed.area); ed.mode = mode;
    if (!ed.table) return ed_error(ec, 52);
    ed.nf = dbf_nfields(ed.table);
    if (mode == UI_EDIT_INSERT && dbf_has_memo(ed.table)) return ed_error(ec, 25);
    if (mode == UI_EDIT_INSERT && wa_index_count(ed.env, ed.area)) return ed_error(ec, 147);
    ed.insert_at = wa_recno(ed.env, ed.area);
    if (ed.insert_at > dbf_nrec(ed.table)) ed.insert_at = dbf_nrec(ed.table);
    ++ed.insert_at;
    if (ed.nf < 1 || ed.nf > ED_FIELDS) return ed_error(ec, 43);
    rc = ed_options(args, ec);
    if (rc < 0) return rc;
    count = dbf_nrec(ed.table);
    if (mode == UI_EDIT_EDIT && (!count || !ed.last)) return CMD_OK;
    if (mode == UI_EDIT_EDIT && wa_eof(ed.env, ed.area))
        (void)wa_nav_goto(ed.env, ed.area, count);
    for (i = 0; i < ed.nshow; ++i)
        if (dbf_field(ed.table, ed.selected[i])->type != 'M') has_editable = 1;
    for (i = 0; i < ed.nf; ++i) {
        int w = dbf_field(ed.table, i)->field_len;
        bytes += (uint32_t)(w > 10 ? w : 10) + 1u;
    }
    ed.storage = pal->acquire(pal, bytes);
    if (!ed.storage) return ed_error(ec, 43);
    bytes = 0;
    for (i = 0; i < ed.nf; ++i) {
        int w = dbf_field(ed.table, i)->field_len;
        ed.text[i] = ed.storage + bytes;
        bytes += (uint32_t)(w > 10 ? w : 10) + 1u;
    }
    /* A private temporary avoids quoting user text and preserves caller vars. */
    if (xb_interp_scope_enter(ip) < 0) {
        pal->release(pal, ed.storage); return ed_error(ec, 43);
    }
    for (i = 0; i <= ED_FIELDS; ++i) {
        ed.temp[0] = 'Z'; ed.temp[1] = 'E'; ed.temp[2] = 'D';
        ed.temp[3] = (char)('0' + i / 100);
        ed.temp[4] = (char)('0' + (i / 10) % 10);
        ed.temp[5] = (char)('0' + i % 10); ed.temp[6] = 0;
        if (ed_field(ed.temp, 6) < 0) break;
    }
    rc = xb_interp_declare_private(ip, ed.temp);
    if (rc < 0) { xb_interp_scope_leave(ip); pal->release(pal, ed.storage); return ed_error(ec, 43); }
    rc = ed_load(mode != UI_EDIT_EDIT, ec);
    if (rc < 0) { xb_interp_scope_leave(ip); pal->release(pal, ed.storage); return rc; }
    kit_open(ed.screen, ip);
    for (;;) {
        int f, key, changed = 0, width;
        char datepic[11];
        const char *picture = (const char *)0;
        uint32_t oldrec;
        xb_val valid;
        while (has_editable && dbf_field(ed.table, ed.selected[active])->type == 'M')
            active = (active + 1) % ed.nshow;
        ed_draw(active);
        f = ed.selected[active]; width = ed.widths[f];
        if (dbf_field(ed.table, f)->type == 'D') {
            for (i = 0; i < width; ++i) {
                char c = ed.text[f][i];
                datepic[i] = c == '/' || c == '.' || c == '-' ? c : '9';
            }
            datepic[width] = 0; picture = datepic;
        }
        key = has_editable ? kit_edit_field(ed.screen, active % ED_VISIBLE, ED_COL,
            ed.text[f], width, dbf_field(ed.table, f)->type, picture, &changed)
            : kit_key(ed.screen);
        if (changed) { ed.changed[f] = 1; ed.typed = 1; }
        if (key == KIT_K_ESC || key == KIT_K_ABORT || key == KIT_K_EOF) break;
        if (has_editable && ed.changed[f] && !ed_value(f, &valid)) {
            const char *msg = dbf_field(ed.table, f)->type == 'D' ?
                "Invalid date. (press SPACE)" : "Invalid value. (press SPACE)";
            kit_message(ed.screen, 24, msg);
            do { key = kit_key(ed.screen); }
            while (key != ' ' && key != KIT_K_ESC && key != KIT_K_EOF);
            if (key == KIT_K_ESC || key == KIT_K_EOF) break;
            continue;
        }
        if (key == KIT_K_SAVE || key == KIT_K_CTRL_END) { rc = ed_save(ec); break; }
        if (key == KIT_K_TOGGLE_DEL) { ed.deleted = !ed.deleted; continue; }
        if (key == KIT_K_UP) {
            i = active - 1;
            while (i >= 0 && dbf_field(ed.table, ed.selected[i])->type == 'M') --i;
            if (i >= 0) active = i;
            continue;
        }
        if (key == KIT_K_ENTER || key == KIT_K_DOWN || key == KIT_K_TAB) {
            if (ed.fresh && !ed.typed && active == 0 && key == KIT_K_ENTER && !ed_has_text(f)) break;
            i = active + 1;
            while (i < ed.nshow && dbf_field(ed.table, ed.selected[i])->type == 'M') ++i;
            if (i < ed.nshow) { active = i; continue; }
            key = KIT_K_PGDN;
        }
        if (key != KIT_K_PGUP && key != KIT_K_PGDN) continue;
        oldrec = wa_recno(ed.env, ed.area);
        rc = ed_save(ec);
        if (rc < 0) break;
        if (ed.mode == UI_EDIT_APPEND) {
            uint32_t rec = ed.fresh ? dbf_nrec(ed.table) + 1u : wa_recno(ed.env, ed.area);
            if (key == KIT_K_PGDN && rec >= dbf_nrec(ed.table)) {
                rc = ed_load(1, ec);
            } else {
                if (key == KIT_K_PGUP && rec > 1) --rec;
                else if (key == KIT_K_PGDN) ++rec;
                if (rec > dbf_nrec(ed.table)) rec = dbf_nrec(ed.table);
                if (!rec) continue;
                (void)wa_nav_goto(ed.env, ed.area, rec);
                rc = ed_load(0, ec);
            }
        } else {
            rc = wa_nav_skip(ed.env, ed.area, key == KIT_K_PGUP ? -1 : 1);
            if (rc != NAV_OK) { rc = ed_error(ec, wa_view_error(ed.env)); break; }
            if (wa_eof(ed.env, ed.area) || wa_bof(ed.env, ed.area) ||
                wa_recno(ed.env, ed.area) < ed.first || wa_recno(ed.env, ed.area) > ed.last)
                (void)wa_nav_goto(ed.env, ed.area, oldrec);
            rc = ed_load(0, ec);
        }
        if (rc < 0) break;
        active = 0;
    }
    kit_close(ed.screen);
    xb_interp_scope_leave(ip);
    pal->release(pal, ed.storage);
    return rc < 0 ? rc : CMD_OK;
}
