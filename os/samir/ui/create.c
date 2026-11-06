/* Ref: dbase3-decomp/specs/full-screen/assist-ui-and-editors.md section 6;
 * specs/file-formats/dbf.md and dbt.md. III+ structure-definition editor. */
#include "samir/kit.h"
#include "samir/rt.h"
#include "samir/dbf.h"
#include "samir/dbt.h"
#include "samir/workarea.h"
#include "ui_apps.h"

#define CR_FIELDS 128
typedef struct {
    kit k;
    dbf_field_t f[CR_FIELDS];
    int count;
    char file[128];
} cr_editor;

static char cr_upper(char c)
{ return c >= 'a' && c <= 'z' ? (char)(c - 32) : c; }

static void cr_number(char *s, unsigned n)
{
    char tmp[12]; int i = 0, j = 0;
    do { tmp[i++] = (char)('0' + n % 10); n /= 10; } while (n);
    while (i) s[j++] = tmp[--i];
    s[j] = 0;
}

static void cr_ext(char *dst, const char *src, const char *ext)
{
    unsigned i = 0, dot = 0;
    while (src[i] && i < 122) {
        dst[i] = src[i];
        if (src[i] == '\\' || src[i] == '/' || src[i] == ':') dot = 0;
        if (src[i] == '.') dot = i;
        i++;
    }
    if (dot) i = dot;
    while (*ext && i < 127) dst[i++] = *ext++;
    dst[i] = 0;
}

static int cr_filename(char *dst, const char *args)
{
    unsigned n = 0; int dot = 0; char quote = 0;
    while (*args == ' ') args++;
    if (*args == '"' || *args == '\'') quote = *args++;
    while (*args && (quote ? *args != quote : *args != ' ')) {
        char c = cr_upper(*args++);
        if (n >= 120) return 0;
        if (c == '\\' || c == '/' || c == ':') dot = 0;
        if (c == '.') dot = 1;
        dst[n++] = c;
    }
    if (quote && *args == quote) args++;
    while (*args == ' ') args++;
    if (*args || !n) return 0;
    dst[n] = 0;
    if (!dot) rt_memcpy(dst + n, ".DBF", 5);
    return 1;
}

static int cr_equal(const char *a, const char *b)
{ return rt_strncmp(a, b, 128) == 0; }

static int cr_exists(samir_pal_t *p, const char *name)
{
    pal_fd fd = p->open(p, name, PAL_RD);
    if (fd < 0) return 0;
    p->close(p, fd); return 1;
}

static unsigned cr_length(const cr_editor *e)
{
    unsigned n = 1;
    for (int i = 0; i < e->count; i++) n += e->f[i].field_len;
    return n;
}

static const char *cr_type(char t)
{
    switch (t) {
    case 'N': return "Numeric";
    case 'D': return "Date";
    case 'L': return "Logical";
    case 'M': return "Memo";
    default: return "Character";
    }
}

static void cr_defaults(dbf_field_t *f, char type)
{
    f->type = type; f->dec_count = 0;
    f->field_len = type == 'D' ? 8 : type == 'L' ? 1 : 10;
}

static void cr_draw(cr_editor *e, int current, int modify)
{
    kit *k = &e->k; char num[12]; unsigned len = cr_length(e);
    int base = current / 36 * 36;
    kit_clear_rows(k, 0, 21);
    kit_put(k, 0, 0, e->file, -1);
    kit_put(k, 0, 43, "Bytes remaining:", -1);
    cr_number(num, len < 4000 ? 4000 - len : 0); kit_put(k, 0, 68, num, -1);
    kit_put(k, 1, 43, "Fields defined:", -1);
    cr_number(num, (unsigned)e->count); kit_put(k, 1, 68, num, -1);
    kit_put(k, 3, 0, "     Field Name  Type       Width   Dec", -1);
    kit_put(k, 3, 40, "     Field Name  Type       Width   Dec", -1);
    for (int j = 0; j < 36 && base + j < CR_FIELDS; j++) {
        int i = base + j, row = 4 + j % 18, col = j / 18 * 40;
        const dbf_field_t *f = &e->f[i];
        cr_number(num, (unsigned)i + 1); kit_put(k, row, col, num, -1);
        if (!f->name[0]) continue;
        kit_put(k, row, col + 5, f->name, -1);
        kit_put(k, row, col + 17, cr_type(f->type), -1);
        cr_number(num, f->field_len); kit_put(k, row, col + 30, num, -1);
        if (f->type == 'N') {
            cr_number(num, f->dec_count); kit_put(k, row, col + 37, num, -1);
        }
    }
    kit_status(k, modify ? "MODIFY STRUCTURE" : "CREATE", e->file,
               (uint32_t)current + 1, (uint32_t)e->count, 1);
    kit_message(k, 23, "Enter: next item   Up/Down: field   Ctrl-End/Ctrl-W: save   Esc: abandon");
    kit_message(k, 24, "Field names: letter first, then letters, digits or _. Types: C N D L M");
}

static int cr_name_valid(cr_editor *e, int index, const char *s)
{
    if (*s < 'A' || *s > 'Z') return 0;
    for (unsigned i = 1; s[i]; i++)
        if (!((s[i] >= 'A' && s[i] <= 'Z') ||
              (s[i] >= '0' && s[i] <= '9') || s[i] == '_')) return 0;
    for (int i = 0; i < e->count; i++)
        if (i != index && cr_equal(e->f[i].name, s)) return 0;
    return 1;
}

/* Overwrite-on-first-type cells do not auto-advance ten-character names. */
static int cr_cell(kit *k, int row, int col, char *s, int width, int numeric)
{
    int pos = 0, first = 1;
    for (;;) {
        int key, len = (int)rt_strlen(s);
        kit_fill(k, row, col, width, ' ', KIT_ATTR_REVERSE);
        kit_put_attr(k, row, col, KIT_ATTR_REVERSE, s, len);
        kit_cursor(k, row, col + (pos < width ? pos : width - 1));
        key = kit_key(k);
        if (key == KIT_K_ENTER || key == KIT_K_TAB || key == KIT_K_UP ||
            key == KIT_K_DOWN || key == KIT_K_SAVE || key == KIT_K_CTRL_END ||
            key == KIT_K_ESC || key == KIT_K_ABORT || key == KIT_K_EOF ||
            key == KIT_K_PGUP || key == KIT_K_PGDN || key == KIT_K_CTRL_Y) return key;
        if (key == KIT_K_HOME) { pos = 0; first = 0; }
        else if (key == KIT_K_END) { pos = len; first = 0; }
        else if (key == KIT_K_LEFT) { if (pos) pos--; first = 0; }
        else if (key == KIT_K_RIGHT) { if (pos < len) pos++; first = 0; }
        else if (key == KIT_K_BKSP && pos) {
            for (int i = pos - 1; i < len; i++) s[i] = s[i + 1];
            pos--; first = 0;
        } else if (key == KIT_K_DEL && pos < len) {
            for (int i = pos; i < len; i++) s[i] = s[i + 1];
            first = 0;
        } else if (key >= 32 && key < 127 && (!numeric || (key >= '0' && key <= '9'))) {
            if (first) { s[0] = 0; len = pos = 0; first = 0; }
            if (len < width && pos <= len) {
                for (int i = len; i >= pos; i--) s[i + 1] = s[i];
                s[pos++] = cr_upper((char)key);
            }
        }
    }
}

static int cr_type_edit(kit *k, int row, int col, dbf_field_t *f)
{
    const char *types = "CNDLM";
    for (;;) {
        int key;
        kit_fill(k, row, col, 9, ' ', KIT_ATTR_REVERSE);
        kit_put_attr(k, row, col, KIT_ATTR_REVERSE, cr_type(f->type), -1);
        kit_cursor(k, row, col); key = kit_key(k);
        if (key == ' ') {
            int i = 0; while (types[i] && types[i] != f->type) i++;
            cr_defaults(f, types[i] && types[i + 1] ? types[i + 1] : types[0]);
        } else if (key < 128 && key >= 32) {
            char c = cr_upper((char)key);
            for (int i = 0; types[i]; i++) if (types[i] == c) cr_defaults(f, c);
        } else return key;
    }
}

static int cr_yes(kit *k, const char *message)
{
    kit_message(k, 24, message);
    for (;;) {
        int key = kit_key(k);
        if (key == 'y' || key == 'Y') return 1;
        if (key == 'n' || key == 'N' || key == KIT_K_ESC || key == KIT_K_EOF) return 0;
    }
}

static int cr_copy(samir_pal_t *p, const char *src, const char *dst)
{
    char buf[512]; int ok = 1; int32_t n;
    pal_fd in = p->open(p, src, PAL_RD), out;
    if (in < 0) return 0;
    out = p->open(p, dst, PAL_WR | PAL_CREATE | PAL_TRUNC);
    if (out < 0) { p->close(p, in); return 0; }
    while ((n = p->read(p, in, buf, sizeof buf)) > 0)
        if (p->write(p, out, buf, (uint32_t)n) != n) { ok = 0; break; }
    if (n < 0) ok = 0;
    if (p->close(p, out) < 0) ok = 0;
    p->close(p, in); return ok;
}

/* dbf_create/dbf_append_rec live in the FILE overlay and cannot be called
 * from this resident UI (samir.ld NOCROSSREFS_TO). Emit only the empty
 * schema here, per dbf.md sections 2/4/8; all record codecs stay in dbf.c. */
static int cr_empty(cr_editor *e, const char *name, dbf_table **out)
{
    samir_pal_t *p = e->k.pal; unsigned hlen = 33 + 32 * (unsigned)e->count;
    unsigned rlen = cr_length(e); uint8_t block[32], tail[2] = {13, 26};
    int memo = 0, ok = 1; pal_fd fd;
    for (int i = 0; i < e->count; i++) if (e->f[i].type == 'M') memo = 1;
    rt_memset(block, 0, sizeof block); block[0] = memo ? 0x83 : 0x03;
    p->today(p, block + 1, block + 2, block + 3);
    block[8] = (uint8_t)hlen; block[9] = (uint8_t)(hlen >> 8);
    block[10] = (uint8_t)rlen; block[11] = (uint8_t)(rlen >> 8);
    fd = p->open(p, name, PAL_WR | PAL_CREATE | PAL_TRUNC);
    if (fd < 0) return 0;
    if (p->write(p, fd, block, sizeof block) != (int32_t)sizeof block) ok = 0;
    for (int i = 0; i < e->count && ok; i++) {
        rt_memset(block, 0, sizeof block);
        rt_memcpy(block, e->f[i].name, rt_strlen(e->f[i].name));
        block[11] = (uint8_t)e->f[i].type;
        block[16] = e->f[i].field_len; block[17] = e->f[i].dec_count;
        if (p->write(p, fd, block, sizeof block) != (int32_t)sizeof block) ok = 0;
    }
    if (ok && p->write(p, fd, tail, sizeof tail) != (int32_t)sizeof tail) ok = 0;
    if (p->close(p, fd) < 0) ok = 0;
    return ok && dbf_open_rw(p, name, out) == DBF_OK;
}

static int cr_write(cr_editor *e, xb_interp *ip, int modify)
{
    samir_pal_t *p = e->k.pal; wa_env *env = xb_interp_env(ip);
    int area = wa_selected(env), memo = 0, rc = 0, del;
    dbf_table *src = modify ? wa_table(env, area) : 0, *out = 0;
    xb_val old[CR_FIELDS];
    int map[CR_FIELDS];
    char temp[128], backup[128], dbt[128], mt[128], mb[128];
    cr_ext(temp, e->file, ".$S$"); cr_ext(backup, e->file, ".BAK");
    if (cr_equal(temp, e->file)) cr_ext(temp, e->file, ".$T$");
    if (cr_equal(backup, e->file)) cr_ext(backup, e->file, ".BK1");
    cr_ext(dbt, e->file, ".DBT"); cr_ext(mt, e->file, ".$M$");
    cr_ext(mb, e->file, ".TBK");
    for (int i = 0; i < e->count; i++) {
        if (e->f[i].type == 'M') memo = 1;
        map[i] = -1;
        if (src) for (int j = 0; j < dbf_nfields(src); j++) {
            const dbf_field_t *f = dbf_field(src, j);
            if (cr_equal(f->name, e->f[i].name) && f->type == e->f[i].type) map[i] = j;
        }
    }
    if (memo && cr_equal(e->file, dbt)) return 0;
    if (!cr_empty(e, temp, &out)) return 0;
    for (uint32_t r = 1; src && r <= dbf_nrec(src); r++) {
        if (dbf_read_rec(src, r, old, &del)) { rc = 1; break; }
        if (dbf_append_blank(out)) { rc = 1; break; }
        for (int i = 0; i < e->count; i++) {
            if (map[i] < 0 || e->f[i].type == 'M' || old[map[i]].t == XB_U) continue;
            if (dbf_replace(out, r, i, &old[map[i]])) { rc = 1; break; }
        }
        if (del && dbf_delete(out, r)) rc = 1;
        if (rc) break;
    }
    if (!rc && dbf_flush(out)) rc = 1;
    if (dbf_close(out)) rc = 1;
    /* Existing memo block pointers remain valid because the companion is
     * copied whole. Patch these ten-byte pointers after the engine flush. */
    if (!rc && memo && src && dbf_has_memo(src)) {
        pal_fd fd = p->open(p, temp, PAL_RDWR);
        if (fd < 0) rc = 1;
        else {
            for (uint32_t r = 1; r <= dbf_nrec(src) && !rc; r++) {
                uint32_t offset = 33 + 32 * (uint32_t)e->count + (r - 1) * cr_length(e) + 1;
                if (dbf_read_rec(src, r, old, &del)) { rc = 1; break; }
                for (int i = 0; i < e->count; i++) {
                    if (e->f[i].type == 'M' && map[i] >= 0 && old[map[i]].t == XB_M) {
                        if (p->seek(p, fd, (int32_t)offset, PAL_SEEK_SET) < 0 ||
                            p->write(p, fd, old[map[i]].u.c.p, 10) != 10) { rc = 1; break; }
                    }
                    offset += e->f[i].field_len;
                }
            }
            if (p->close(p, fd) < 0) rc = 1;
        }
    }
    if (!rc && memo) {
        if (src && dbf_has_memo(src)) { if (!cr_copy(p, dbt, mt)) rc = 1; }
        else {
            dbt_file *m = 0;
            if (dbt_create(p, mt, &m)) rc = 1;
            else { if (dbt_flush(m)) rc = 1; if (dbt_close(m)) rc = 1; }
        }
    }
    if (rc) { p->remove(p, temp); if (memo) p->remove(p, mt); return 0; }
    /* Finish the new files before closing or replacing the old table. */
    if (modify && wa_close(env, area)) { p->remove(p, temp); return 0; }
    {
        int had = cr_exists(p, e->file), hadmemo = cr_exists(p, dbt);
        if (had) {
            if (p->remove(p, backup) < 0 && cr_exists(p, backup)) return 0;
            if (p->rename(p, e->file, backup) < 0) return 0;
        }
        if (memo && hadmemo) {
            if (p->remove(p, mb) < 0 && cr_exists(p, mb)) {
                if (had) p->rename(p, backup, e->file);
                return 0;
            }
            if (p->rename(p, dbt, mb) < 0) {
                if (had) p->rename(p, backup, e->file);
                return 0;
            }
        }
        if (memo && p->rename(p, mt, dbt) < 0) {
            if (hadmemo) p->rename(p, mb, dbt);
            if (had) p->rename(p, backup, e->file);
            return 0;
        }
        if (p->rename(p, temp, e->file) < 0) {
            if (memo) { p->remove(p, dbt); if (hadmemo) p->rename(p, mb, dbt); }
            if (had) p->rename(p, backup, e->file);
            return 0;
        }
    }
    return 1;
}

int ui_create(xb_interp *ip, int modify, const char *args, int *ec)
{
    samir_pal_t *p = xb_interp_pal(ip); wa_env *env = xb_interp_env(ip);
    dbf_table *src = modify ? wa_table(env, wa_selected(env)) : 0;
    cr_editor *e; int current = 0, column = 0, saved = 0, append = 0, error = 0;
    char command[140];
    *ec = 0;
    if (modify && !src) { *ec = 52; return -INTERP_ERR_EVAL; }
    /* Leave CREATE ... FROM to the existing command engine. */
    if (!modify) {
        const char *a = args;
        while (*a && *a != ' ') a++;
        while (*a == ' ') a++;
        if (rt_kw_match(a, "FROM")) return CMD_UNKNOWN;
    }
    e = p->acquire(p, sizeof *e);
    if (!e) { *ec = 43; return -INTERP_ERR_EVAL; }
    rt_memset(e, 0, sizeof *e);
    kit_open(&e->k, ip);
    if (modify) {
        rt_memcpy(e->file, dbf_name(src), rt_strlen(dbf_name(src)) + 1);
        e->count = dbf_nfields(src);
        if (e->count > CR_FIELDS) { error = 23; goto done; }
        for (int i = 0; i < e->count; i++) e->f[i] = *dbf_field(src, i);
    } else {
        char name[128];
        while (*args == ' ') args++;
        if (!*args) {
            name[0] = 0;
            if (!kit_prompt(&e->k, 24, "Enter the name of the new file:", name, 40)) goto done;
            args = name;
        }
        if (!cr_filename(e->file, args)) { error = 10; goto done; }
        if (wa_name_is_open(env, e->file, -1)) { error = 17; goto done; }
        if (cr_exists(p, e->file) && !cr_yes(&e->k, "File exists. Replace it? (Y/N)")) goto done;
    }
    for (;;) {
        dbf_field_t *f = &e->f[current]; char cell[12];
        int key, j = current % 36, rownum = 4 + j % 18, col = j / 18 * 40;
        if (!f->type) cr_defaults(f, 'C');
        cr_draw(e, current, modify);
        if (column == 0) {
            rt_memcpy(cell, f->name, sizeof f->name);
            key = cr_cell(&e->k, rownum, col + 5, cell, 10, 0);
            if (key == KIT_K_ESC || key == KIT_K_ABORT || key == KIT_K_EOF) break;
            if (*cell && !cr_name_valid(e, current, cell)) {
                kit_message(&e->k, 24, "Invalid or duplicate field name. Press any key.");
                kit_key(&e->k); continue;
            }
            if (*cell) {
                rt_memcpy(f->name, cell, sizeof f->name);
                if (current >= e->count) e->count = current + 1;
            } else if (key == KIT_K_ENTER) key = KIT_K_SAVE;
        } else if (column == 1) key = cr_type_edit(&e->k, rownum, col + 17, f);
        else {
            unsigned n = 0;
            cr_number(cell, column == 2 ? f->field_len : f->dec_count);
            key = cr_cell(&e->k, rownum, col + (column == 2 ? 30 : 37), cell, column == 2 ? 3 : 2, 1);
            if (key == KIT_K_ESC || key == KIT_K_ABORT || key == KIT_K_EOF) break;
            for (int i = 0; cell[i]; i++) n = n * 10 + (unsigned)(cell[i] - '0');
            if (column == 2) {
                unsigned max = f->type == 'N' ? 19 : 254;
                if (n < 1 || n > max) {
                    kit_message(&e->k, 24, "Width out of range. Press any key."); kit_key(&e->k); continue;
                }
                f->field_len = (uint8_t)n;
                if (f->dec_count && (unsigned)f->dec_count + 2 > n) f->dec_count = 0;
            } else {
                if (n && n + 2 > f->field_len) {
                    kit_message(&e->k, 24, "Decimals must leave an integer digit and decimal point. Press any key.");
                    kit_key(&e->k); continue;
                }
                f->dec_count = (uint8_t)n;
            }
        }
        if (key == KIT_K_ESC || key == KIT_K_ABORT || key == KIT_K_EOF) break;
        if (key == KIT_K_CTRL_Y) {
            if (current < e->count) {
                for (int i = current; i + 1 < e->count; i++) e->f[i] = e->f[i + 1];
                e->count--;
                rt_memset(&e->f[e->count], 0, sizeof e->f[0]);
            }
            column = 0; continue;
        }
        if (key == KIT_K_SAVE || key == KIT_K_CTRL_END) {
            if (!e->count || cr_length(e) > 4000) {
                kit_message(&e->k, 24, "Define 1-128 fields using no more than 3999 data bytes. Press any key.");
                kit_key(&e->k); continue;
            }
            kit_message(&e->k, 24, "Press ENTER to confirm. Any other key to resume");
            if (kit_key(&e->k) != KIT_K_ENTER) continue;
            if (!cr_write(e, ip, modify)) { error = 29; break; }
            saved = 1;
            if (!modify) append = cr_yes(&e->k, "Input data records now? (Y/N)");
            break;
        }
        if (key == KIT_K_UP || key == KIT_K_PGUP) {
            current -= key == KIT_K_PGUP ? 18 : 1; if (current < 0) current = 0;
            column = 0;
        } else if (key == KIT_K_DOWN || key == KIT_K_PGDN) {
            if (current < e->count && current < CR_FIELDS - 1) current++;
            column = 0;
        } else if (key == KIT_K_ENTER || key == KIT_K_TAB) {
            if (!f->name[0]) column = 0;
            else if (column == 0) column = 1;
            else if (column == 1 && (f->type == 'C' || f->type == 'N')) column = 2;
            else if (column == 2 && f->type == 'N') column = 3;
            else { if (current < CR_FIELDS - 1) current++; column = 0; }
        }
    }
done:
    rt_memcpy(command, "USE ", 4);
    rt_memcpy(command + 4, e->file, rt_strlen(e->file) + 1);
    kit_close(&e->k); p->release(p, e);
    if (error) { *ec = error; return -INTERP_ERR_EVAL; }
    if (saved) {
        int rc = samir_do(ip, command);
        if (rc < 0) { *ec = samir_last_error(ip); return rc; }
        if (append) return samir_do(ip, "APPEND");
    }
    return CMD_OK;
}
