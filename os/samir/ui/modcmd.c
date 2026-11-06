/* Ref: dbase3-decomp/specs/full-screen/assist-ui-and-editors.md section 9.
 * III+ text editor: 5000-byte files, 254-character lines, CRLF and DOS EOF. */
#include "samir/kit.h"
#include "samir/rt.h"
#include "ui_apps.h"

#define MC_MAX 5000
#define MC_ROWS 21
typedef struct {
    kit k;
    char text[MC_MAX + 1];
    char file[128];
    int len, pos, top, left, preferred, insert, dirty, filemode;
} mc_editor;

static void mc_num(char *s, unsigned n)
{
    char t[12]; int i = 0, j = 0;
    do { t[i++] = (char)('0' + n % 10); n /= 10; } while (n);
    while (i) s[j++] = t[--i];
    s[j] = 0;
}

static void mc_ext(char *dst, const char *src, const char *ext)
{
    int n = 0, dot = -1;
    while (src[n] && n < 122) {
        dst[n] = src[n];
        if (src[n] == '\\' || src[n] == '/' || src[n] == ':') dot = -1;
        if (src[n] == '.') dot = n;
        n++;
    }
    if (dot >= 0) n = dot;
    while (*ext && n < 127) dst[n++] = *ext++;
    dst[n] = 0;
}

static int mc_name(char *dst, const char *src, int filemode)
{
    int n = 0, dot = 0; char quote = 0;
    while (*src == ' ') src++;
    if (*src == '\'' || *src == '"') quote = *src++;
    while (*src && (quote ? *src != quote : *src != ' ')) {
        char c = *src++;
        if (n >= 120) return 0;
        if (c >= 'a' && c <= 'z') c -= 32;
        if (c == '\\' || c == '/' || c == ':') dot = 0;
        if (c == '.') dot = 1;
        dst[n++] = c;
    }
    if (quote && *src == quote) src++;
    while (*src == ' ') src++;
    if (!n || *src) return 0;
    dst[n] = 0;
    if (!dot && !filemode) rt_memcpy(dst + n, ".PRG", 5);
    return 1;
}

static int mc_start(const mc_editor *e, int pos)
{ while (pos > 0 && e->text[pos - 1] != '\n') pos--; return pos; }
static int mc_end(const mc_editor *e, int pos)
{ while (pos < e->len && e->text[pos] != '\n') pos++; return pos; }
static int mc_line(const mc_editor *e, int pos)
{
    int line = 0;
    for (int i = 0; i < pos; i++) if (e->text[i] == '\n') line++;
    return line;
}
static int mc_offset(const mc_editor *e, int line)
{
    int pos = 0;
    while (line-- > 0 && pos < e->len) {
        pos = mc_end(e, pos);
        if (pos < e->len) pos++;
    }
    return pos;
}

static int mc_disk_size(const mc_editor *e)
{
    int n = e->len + 1;
    for (int i = 0; i < e->len; i++) if (e->text[i] == '\n') n++;
    return n;
}

static int mc_load(mc_editor *e)
{
    samir_pal_t *p = e->k.pal; pal_fd fd = p->open(p, e->file, PAL_RD);
    char buf[512]; int32_t n; int cr = 0, done = 0, column = 0;
    if (fd == -PAL_ENOENT) return 0;
    if (fd < 0) return 29;
    while (!done && (n = p->read(p, fd, buf, sizeof buf)) > 0) {
        for (int i = 0; i < n; i++) {
            char c = buf[i];
            if (c == 0x1A) { done = 1; break; }
            if (c == '\n' && cr) { cr = 0; continue; }
            cr = c == '\r';
            if (cr) c = '\n';
            if (e->len >= MC_MAX - 1) { p->close(p, fd); return 23; }
            e->text[e->len++] = c;
            if (c == '\n') column = 0;
            else if (++column > 254) { p->close(p, fd); return 18; }
        }
    }
    p->close(p, fd);
    e->text[e->len] = 0;
    return n < 0 ? 29 : mc_disk_size(e) > MC_MAX ? 23 : 0;
}

static void mc_draw(mc_editor *e)
{
    kit *k = &e->k; char num[12];
    int line = mc_line(e, e->pos), column = e->pos - mc_start(e, e->pos);
    int pos;
    if (line < e->top) e->top = line;
    if (line >= e->top + MC_ROWS) e->top = line - MC_ROWS + 1;
    if (column < e->left) e->left = column;
    if (column >= e->left + KIT_COLS) e->left = column - KIT_COLS + 1;
    pos = mc_offset(e, e->top);
    for (int row = 0; row < MC_ROWS; row++) {
        int end = mc_end(e, pos), from = pos + e->left, n = end - from;
        kit_fill(k, row, 0, KIT_COLS, ' ', KIT_ATTR_NORMAL);
        if (n > KIT_COLS) n = KIT_COLS;
        if (n > 0) kit_put(k, row, 0, e->text + from, n);
        pos = end < e->len ? end + 1 : e->len;
    }
    kit_fill(k, 21, 0, KIT_COLS, ' ', KIT_ATTR_NORMAL);
    kit_put(k, 21, 0, "Line:", -1); mc_num(num, (unsigned)line + 1); kit_put(k, 21, 6, num, -1);
    kit_put(k, 21, 15, "Column:", -1); mc_num(num, (unsigned)column + 1); kit_put(k, 21, 23, num, -1);
    kit_put(k, 21, 34, "Bytes:", -1); mc_num(num, (unsigned)mc_disk_size(e)); kit_put(k, 21, 41, num, -1);
    if (e->dirty) kit_put(k, 21, 55, "Modified", -1);
    kit_status(k, e->filemode ? "MODIFY FILE" : "MODIFY COMMAND", e->file, (uint32_t)line + 1,
               (uint32_t)mc_line(e, e->len) + 1, e->insert);
    kit_message(k, 23, "Arrows: move   PgUp/PgDn: page   Home/End: line   Ins: insert/overwrite");
    kit_message(k, 24, "Ctrl-N: insert line   Ctrl-Y: delete line   Ctrl-W/Ctrl-End: save   Esc: abort");
    kit_cursor(k, line - e->top, column - e->left);
}

static void mc_error(mc_editor *e, const char *message)
{ kit_message(&e->k, 24, message); kit_key(&e->k); }

static int mc_insert(mc_editor *e, int pos, char c)
{
    if (mc_disk_size(e) + (c == '\n' ? 2 : 1) > MC_MAX) {
        mc_error(e, "File exceeds 5000 bytes. Press any key."); return 0;
    }
    for (int i = e->len; i >= pos; i--) e->text[i + 1] = e->text[i];
    e->text[pos] = c; e->len++; e->dirty = 1; return 1;
}

static void mc_delete(mc_editor *e, int start, int end)
{
    if (end <= start) return;
    for (int i = end; i <= e->len; i++) e->text[start + i - end] = e->text[i];
    e->len -= end - start; e->dirty = 1;
}

static void mc_vertical(mc_editor *e, int delta)
{
    int line = mc_line(e, e->pos), max = mc_line(e, e->len), start, end;
    if (e->preferred < 0) e->preferred = e->pos - mc_start(e, e->pos);
    line += delta; if (line < 0) line = 0; if (line > max) line = max;
    start = mc_offset(e, line); end = mc_end(e, start);
    e->pos = start + e->preferred;
    if (e->pos > end) e->pos = end;
}

static int mc_save(mc_editor *e)
{
    samir_pal_t *p = e->k.pal; char tmp[128], bak[128], buf[512];
    int used = 0, ok = 1, existed = 0; pal_fd fd, old;
    mc_ext(tmp, e->file, ".$E$"); mc_ext(bak, e->file, ".BAK");
    if (rt_strncmp(e->file, tmp, 128) == 0) mc_ext(tmp, e->file, ".$T$");
    if (rt_strncmp(e->file, bak, 128) == 0) mc_ext(bak, e->file, ".BK1");
    fd = p->open(p, tmp, PAL_WR | PAL_CREATE | PAL_TRUNC);
    if (fd < 0) return 0;
    for (int i = 0; i < e->len && ok; i++) {
        if (used > (int)sizeof buf - 3) {
            if (p->write(p, fd, buf, (uint32_t)used) != used) ok = 0;
            used = 0;
        }
        if (e->text[i] == '\n') buf[used++] = '\r';
        buf[used++] = e->text[i];
    }
    buf[used++] = 0x1A;
    if (ok && p->write(p, fd, buf, (uint32_t)used) != used) ok = 0;
    if (p->close(p, fd) < 0) ok = 0;
    if (!ok) { p->remove(p, tmp); return 0; }
    old = p->open(p, e->file, PAL_RD);
    if (old >= 0) {
        existed = 1; p->close(p, old);
        old = p->open(p, bak, PAL_RD);
        if (old >= 0) {
            p->close(p, old);
            if (p->remove(p, bak) < 0) { p->remove(p, tmp); return 0; }
        }
        if (p->rename(p, e->file, bak) < 0) { p->remove(p, tmp); return 0; }
    } else if (old != -PAL_ENOENT) { p->remove(p, tmp); return 0; }
    if (p->rename(p, tmp, e->file) < 0) {
        if (existed) p->rename(p, bak, e->file);
        p->remove(p, tmp); return 0;
    }
    e->dirty = 0; return 1;
}

static int mc_abandon(mc_editor *e)
{
    if (!e->dirty) return 1;
    kit_message(&e->k, 24, "Abandon changes? (Y/N)");
    for (;;) {
        int key = kit_key(&e->k);
        if (key == 'Y' || key == 'y' || key == KIT_K_EOF) return 1;
        if (key == 'N' || key == 'n' || key == KIT_K_ESC) return 0;
    }
}

static int mc_run(xb_interp *ip, const char *args, int *ec, int filemode)
{
    samir_pal_t *p = xb_interp_pal(ip); mc_editor *e;
    int error = 0;
    *ec = 0;
    e = p->acquire(p, sizeof *e);
    if (!e) { *ec = 43; return -INTERP_ERR_EVAL; }
    rt_memset(e, 0, sizeof *e); e->insert = 1; e->preferred = -1;
    e->filemode = filemode;
    kit_open(&e->k, ip);
    while (*args == ' ') args++;
    if (!*args) {
        char name[128]; name[0] = 0;
        if (!kit_prompt(&e->k, 24, filemode ? "Enter the name of the file:" :
                        "Enter the name of the command file:", name, 40)) goto done;
        if (!mc_name(e->file, name, filemode)) { error = 10; goto done; }
    } else if (!mc_name(e->file, args, filemode)) { error = 10; goto done; }
    error = mc_load(e);
    if (error) goto done;
    for (;;) {
        int key, start, end;
        mc_draw(e); key = kit_key(&e->k);
        start = mc_start(e, e->pos); end = mc_end(e, e->pos);
        if (key == KIT_K_SAVE || key == KIT_K_CTRL_END) {
            if (mc_save(e)) break;
            mc_error(e, "Unable to save file. Press any key to continue editing.");
        } else if (key == KIT_K_ESC || key == KIT_K_ABORT || key == KIT_K_EOF) {
            if (mc_abandon(e)) break;
        } else if (key == KIT_K_UP || key == KIT_K_DOWN || key == KIT_K_PGUP || key == KIT_K_PGDN) {
            mc_vertical(e, key == KIT_K_UP ? -1 : key == KIT_K_DOWN ? 1 : key == KIT_K_PGUP ? -MC_ROWS : MC_ROWS);
            continue;
        } else if (key == KIT_K_LEFT) { if (e->pos) e->pos--; }
        else if (key == KIT_K_RIGHT) { if (e->pos < e->len) e->pos++; }
        else if (key == KIT_K_HOME) e->pos = start;
        else if (key == KIT_K_END) e->pos = end;
        else if (key == KIT_K_CTRL_HOME) e->pos = 0;
        else if (key == KIT_K_INS) e->insert = !e->insert;
        else if (key == KIT_K_ENTER) { if (mc_insert(e, e->pos, '\n')) e->pos++; }
        else if (key == KIT_K_CTRL_N) { if (mc_insert(e, start, '\n')) e->pos = start; }
        else if (key == KIT_K_CTRL_Y) {
            if (end < e->len) mc_delete(e, start, end + 1);
            else if (start > 0) { mc_delete(e, start - 1, end); start--; }
            else mc_delete(e, start, end);
            e->pos = start;
        } else if (key == KIT_K_BKSP && e->pos > 0) {
            if (e->text[e->pos - 1] == '\n' &&
                end - e->pos + start - 1 - mc_start(e, start - 1) > 254)
                mc_error(e, "Line exceeds maximum of 254 characters. Press any key.");
            else { mc_delete(e, e->pos - 1, e->pos); e->pos--; }
        } else if (key == KIT_K_DEL && e->pos < e->len) {
            if (e->text[e->pos] == '\n' && mc_end(e, end + 1) - start - 1 > 254)
                mc_error(e, "Line exceeds maximum of 254 characters. Press any key.");
            else mc_delete(e, e->pos, e->pos + 1);
        } else if (key >= 32 && key < 127) {
            if (!e->insert && e->pos < end) { e->text[e->pos++] = (char)key; e->dirty = 1; }
            else if (end - start >= 254)
                mc_error(e, "Line exceeds maximum of 254 characters. Press any key.");
            else if (mc_insert(e, e->pos, (char)key)) e->pos++;
        } else if (key == KIT_K_TAB) {
            int count = 8 - (e->pos - start) % 8;
            if (end - start + count <= 254)
                while (count-- > 0 && mc_insert(e, e->pos, ' ')) e->pos++;
        }
        e->preferred = -1;
    }
done:
    kit_close(&e->k); p->release(p, e);
    if (error) { *ec = error; return -INTERP_ERR_EVAL; }
    return CMD_OK;
}

int ui_modify_command(xb_interp *ip, const char *args, int *ec)
{ return mc_run(ip, args, ec, 0); }

/* Shared dispatch currently discards COMMAND versus FILE. This entry lets
 * ui.c route FILE separately without changing the existing COMMAND ABI. */
int ui_modify_file(xb_interp *ip, const char *args, int *ec);
int ui_modify_file(xb_interp *ip, const char *args, int *ec)
{ return mc_run(ip, args, ec, 1); }
