/* Full-screen kit: dbase3-decomp/specs/full-screen/assist-ui-and-editors.md
 * and picture-function-templates.md; Using III+ U5-18..19 (@R and @S).
 * Experimental SAMIR_W4 UI only.
 * The PAL owns terminal transport; this module owns all 80x25 shadow cells. */
#include "samir/kit.h"
#include "samir/rt.h"

static int bound(int n, int lo, int hi)
{ return n < lo ? lo : n > hi ? hi : n; }
static int upper(int c)
{ return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c; }
static int length(const char *s)
{ return s ? (int)rt_strlen(s) : 0; }

static void push(kit *k, int row, int first, int end)
{
    int col, next;
    if (!k->pal || !k->pal->paint) return;
    for (col = first; col < end; col = next) {
        for (next = col + 1; next < end && k->at[row][next] == k->at[row][col]; next++) { }
        if (k->pal->set_attr) k->pal->set_attr(k->pal, k->at[row][col]);
        (void)k->pal->paint(k->pal, (uint8_t)row, (uint8_t)col,
                          k->ch[row] + col, (uint32_t)(next - col));
    }
    if (k->pal->gotoxy) k->pal->gotoxy(k->pal, k->cur_row, k->cur_col);
}
void kit_cursor(kit *k, int row, int col)
{
    k->cur_row = (uint8_t)bound(row, 0, KIT_ROWS - 1);
    k->cur_col = (uint8_t)bound(col, 0, KIT_COLS - 1);
    if (k->pal && k->pal->gotoxy) k->pal->gotoxy(k->pal, k->cur_row, k->cur_col);
}
void kit_flush_row(kit *k, int row)
{ if (row >= 0 && row < KIT_ROWS) push(k, row, 0, KIT_COLS); }
void kit_put_attr(kit *k, int row, int col, uint8_t attr, const char *s, int n)
{
    int i;
    if (!s || row < 0 || row >= KIT_ROWS || col >= KIT_COLS) return;
    if (n < 0) n = length(s);
    if (col < 0) {
        if (col <= -n) return;
        s -= col; n += col; col = 0;
    }
    if (n > KIT_COLS - col) n = KIT_COLS - col;
    for (i = 0; i < n; i++) {
        /* Control bytes cannot be allowed to scroll; CP437 boxes are legal. */
        k->ch[row][col + i] = (unsigned char)s[i] < 32 ? ' ' : s[i];
        k->at[row][col + i] = attr;
    }
    if (n > 0) push(k, row, col, col + n);
}
void kit_put(kit *k, int row, int col, const char *s, int n)
{ kit_put_attr(k, row, col, k->attr, s, n); }
void kit_fill(kit *k, int row, int col, int n, char c, uint8_t attr)
{
    if (row < 0 || row >= KIT_ROWS || col >= KIT_COLS || n <= 0) return;
    if (col < 0) {
        if (col <= -n) return;
        n += col; col = 0;
    }
    if (n > KIT_COLS - col) n = KIT_COLS - col;
    if ((unsigned char)c < 32) c = ' ';
    rt_memset(k->ch[row] + col, c, (uint32_t)n);
    rt_memset(k->at[row] + col, attr, (uint32_t)n);
    push(k, row, col, col + n);
}
void kit_clear_rows(kit *k, int top, int bottom)
{
    int r;
    if (top < 0) top = 0;
    if (bottom >= KIT_ROWS) bottom = KIT_ROWS - 1;
    for (r = top; r <= bottom; r++) kit_fill(k, r, 0, KIT_COLS, ' ', KIT_ATTR_NORMAL);
}
void kit_clear(kit *k)
{
    k->attr = KIT_ATTR_NORMAL;
    kit_cursor(k, 0, 0);
    kit_clear_rows(k, 0, KIT_ROWS - 1);
}
void kit_open(kit *k, xb_interp *ip)
{
    rt_memset(k, 0, sizeof(*k));
    k->ip = ip; k->pal = ip ? xb_interp_pal(ip) : (samir_pal_t *)0;
    /* Suspend Milton's separate dot-prompt viewport/cursor. On return the
     * REPL's status refresh initializes a clean command page. */
    if (k->pal && k->pal->status) k->pal->status(k->pal, 0, "", 0, 0);
    kit_clear(k);
}
void kit_close(kit *k)
{
    /* A nested engine command (for example LIST in ASSIST) may have enabled
     * its viewport again. Disable it before clearing, so its stale cursor
     * cannot relocate or scroll the first dot prompt after this modal UI. */
    if (k->pal && k->pal->status) k->pal->status(k->pal, 0, "", 0, 0);
    kit_clear(k);
    if (k->pal && k->pal->set_attr) k->pal->set_attr(k->pal, KIT_ATTR_NORMAL);
    kit_cursor(k, 0, 0);
}
void kit_box(kit *k, int top, int left, int bottom, int right, int dbl)
{
    int r;
    char h = (char)(dbl ? 205 : 196), v = (char)(dbl ? 186 : 179);
    char tl = (char)(dbl ? 201 : 218), tr = (char)(dbl ? 187 : 191);
    char bl = (char)(dbl ? 200 : 192), br = (char)(dbl ? 188 : 217);
    if (bottom < top || right < left) return;
    kit_fill(k, top, left, right - left + 1, h, k->attr);
    kit_fill(k, bottom, left, right - left + 1, h, k->attr);
    for (r = bound(top + 1, 0, KIT_ROWS); r <= bottom - 1 && r < KIT_ROWS; r++) {
        kit_fill(k, r, left, 1, v, k->attr); kit_fill(k, r, right, 1, v, k->attr);
    }
    kit_fill(k, top, left, 1, tl, k->attr); kit_fill(k, top, right, 1, tr, k->attr);
    kit_fill(k, bottom, left, 1, bl, k->attr); kit_fill(k, bottom, right, 1, br, k->attr);
}
void kit_save(kit *k, kit_saved *s)
{
    rt_memcpy(s->ch, k->ch, sizeof(s->ch)); rt_memcpy(s->at, k->at, sizeof(s->at));
    s->cur_row = k->cur_row; s->cur_col = k->cur_col;
}
void kit_restore(kit *k, const kit_saved *s)
{
    int r;
    rt_memcpy(k->ch, s->ch, sizeof(k->ch)); rt_memcpy(k->at, s->at, sizeof(k->at));
    k->cur_row = s->cur_row; k->cur_col = s->cur_col;
    for (r = 0; r < KIT_ROWS; r++) kit_flush_row(k, r);
    kit_cursor(k, s->cur_row, s->cur_col);
}
static int number(char *s, uint32_t n)
{
    char rev[10]; int i = 0, j;
    do { rev[i++] = (char)('0' + n % 10); n /= 10; } while (n);
    for (j = 0; j < i; j++) s[j] = rev[i - j - 1];
    return i;
}
void kit_status(kit *k, const char *mode, const char *file, uint32_t rec, uint32_t count, int ins)
{
    char row[KIT_COLS], value[24]; const char *base, *p; int i, n, drive = 'A';
    rt_memset(row, ' ', sizeof(row));
    if (k->pal && k->pal->curdrive) drive = k->pal->curdrive(k->pal);
    if (drive < 'A' || drive > 'Z') drive = 'A';
    if (file && file[0] && file[1] == ':') drive = upper(file[0]);
    for (i = 0; mode && mode[i] && i < 15; i++) row[1 + i] = mode[i];
    row[16] = row[22] = row[45] = row[65] = row[72] = '|';
    row[17] = '<'; row[18] = (char)drive; row[19] = ':'; row[20] = '>';
    base = file;
    for (p = file; p && *p; p++) if (*p == '/' || *p == '\\' || *p == ':') base = p + 1;
    for (i = 0; base && base[i] && base[i] != '.' && i < 22; i++) row[23 + i] = (char)upper(base[i]);
    rt_memcpy(row + 46, "Rec: ", 5);
    n = number(value, rec); value[n++] = '/'; n += number(value + n, count);
    if (n > 14) n = 14;
    rt_memcpy(row + 51, value, (uint32_t)n);
    rt_memcpy(row + 67, ins ? "Ins" : "Ovr", 3);
    kit_put_attr(k, 22, 0, KIT_ATTR_REVERSE, row, KIT_COLS);
}
void kit_message(kit *k, int row, const char *msg)
{
    int n = length(msg);
    if (n > KIT_COLS) n = KIT_COLS;
    kit_fill(k, row, 0, KIT_COLS, ' ', KIT_ATTR_NORMAL);
    if (n) kit_put_attr(k, row, (KIT_COLS - n) / 2, KIT_ATTR_NORMAL, msg, n);
}
int kit_key(kit *k)
{
    int c, scan;
    if (!k->pal || !k->pal->conin_char) return KIT_K_EOF;
    c = k->pal->conin_char(k->pal);
    if (c < 0) return KIT_K_EOF;
    if (c == 0 || c == 0xE0 || (c >= 0x100 && c <= 0x1FF)) {
        scan = c >= 0x100 ? c & 255 : k->pal->conin_char(k->pal);
        if (scan < 0) return KIT_K_EOF;
        switch (scan) {
        case 0x48: return KIT_K_UP; case 0x50: return KIT_K_DOWN;
        case 0x4B: return KIT_K_LEFT; case 0x4D: return KIT_K_RIGHT;
        case 0x49: return KIT_K_PGUP; case 0x51: return KIT_K_PGDN;
        case 0x47: return KIT_K_HOME; case 0x4F: return KIT_K_END;
        case 0x52: return KIT_K_INS; case 0x53: return KIT_K_DEL;
        case 0x3B: return KIT_K_F1; case 0x44: return KIT_K_F10;
        case 0x75: return KIT_K_SAVE; case 0x77: return KIT_K_CTRL_HOME;
        case 0x0F: return KIT_K_UP;
        default: return KIT_K_NONE;
        }
    }
    switch (c) {
    case 5: return KIT_K_UP; case 24: return KIT_K_DOWN;
    case 19: return KIT_K_LEFT; case 4: return KIT_K_RIGHT;
    case 18: return KIT_K_PGUP; case 3: return KIT_K_PGDN;
    case 23: return KIT_K_SAVE; case 17: return KIT_K_ABORT;
    case 21: return KIT_K_TOGGLE_DEL; case 14: return KIT_K_CTRL_N;
    case 22: return KIT_K_INS; case 7: return KIT_K_DEL;
    case 25: return KIT_K_CTRL_Y; case 10: return KIT_K_ENTER;
    case 127: return KIT_K_BKSP;
    default: return c >= 32 && c <= 126 ? c :
                    c == KIT_K_ENTER || c == KIT_K_ESC || c == KIT_K_BKSP || c == KIT_K_TAB ? c : KIT_K_NONE;
    }
}
static const char *template_start(const char *pic, int *caps, int *alpha)
{
    *caps = *alpha = 0;
    if (!pic) return "";
    if (*pic == '@') {
        for (pic++; *pic && *pic != ' '; pic++) {
            if (*pic == '!') *caps = 1;
            if (upper(*pic) == 'A') *alpha = 1;
        }
        while (*pic == ' ') pic++;
    }
    return pic;
}
static int slot(const char *pic, int len, int pos)
{
    int c;
    if (pos >= len) return 1;
    c = pic[pos];
    return c == '9' || c == '#' || c == 'A' || c == 'N' || c == 'X' ||
           c == '!' || c == 'Y' || c == 'L';
}
static int next_slot(const char *pic, int len, int pos, int w, int step)
{
    while (pos >= 0 && pos < w && !slot(pic, len, pos)) pos += step;
    return pos;
}
static int accept_char(int c, char type, int mask, int caps, int alpha)
{
    int u = upper(c), digit = c >= '0' && c <= '9';
    int letter = u >= 'A' && u <= 'Z';
    if (type == 'L' || mask == 'L' || mask == 'Y') {
        if (u != 'T' && u != 'F' && u != 'Y' && u != 'N') return -1;
        return mask == 'Y' ? (u == 'T' || u == 'Y' ? 'Y' : 'N') :
                             (u == 'T' || u == 'Y' ? 'T' : 'F');
    }
    if (type == 'N' && !digit && c != '+' && c != '-' && c != '.' && c != ' ') return -1;
    if (type == 'D' && !digit && c != '/' && c != '-' && c != '.' && c != ' ') return -1;
    if ((mask == 'A' || alpha) && !letter) return -1;
    if (mask == 'N' && !letter && !digit) return -1;
    if (mask == '9' && !digit && !(type == 'N' && (c == '+' || c == '-' || c == '.' || c == ' '))) return -1;
    if (mask == '#' && !digit && c != '+' && c != '-' && c != ' ' && !(type == 'N' && c == '.')) return -1;
    return caps || mask == '!' ? u : c;
}
static void align_integer(char *buf, const char *pic, int len, int point)
{
    int from = next_slot(pic, len, point - 1, point, -1), to = from;
    while (from >= 0 && buf[from] == ' ') from = next_slot(pic, len, from - 1, point, -1);
    while (to >= 0) {
        buf[to] = from >= 0 ? buf[from] : ' ';
        if (from >= 0) from = next_slot(pic, len, from - 1, point, -1);
        to = next_slot(pic, len, to - 1, point, -1);
    }
}
static int picture_width(const char *picture)
{
    int width = KIT_COLS;
    if (picture && *picture == '@') {
        for (picture++; *picture && *picture != ' '; picture++) {
            if (upper(*picture) == 'S' && picture[1] >= '0' && picture[1] <= '9') {
                width = 0;
                while (picture[1] >= '0' && picture[1] <= '9') {
                    int digit = *++picture - '0';
                    if (width < KIT_COLS) width = bound(width * 10 + digit, 0, KIT_COLS);
                }
                if (!width) width = KIT_COLS;
            }
        }
    }
    return width;
}
static void field_draw(kit *k, int row, int col, const char *buf, int w, int pos, int *offset, int limit)
{
    int visible = KIT_COLS - col;
    if (visible > limit) visible = limit;
    if (visible > w) visible = w;
    if (pos < *offset) *offset = pos;
    if (pos >= *offset + visible) *offset = pos - visible + 1;
    kit_put_attr(k, row, col, KIT_ATTR_REVERSE, buf + *offset, visible);
    kit_cursor(k, row, col + pos - *offset);
}
static int edit_field(kit *k, int row, int col, char *buf, int w, char type,
                      const char *picture, int *changed)
{
    int pos, key, c, i, j, ins = 0, offset = 0, caps, alpha, len, dirty = 0, first = 1, point = -1;
    char datepic[11];
    const char *pic = template_start(picture, &caps, &alpha);
    int limit = picture_width(picture);
    if (changed) *changed = 0;
    if (!buf || w <= 0) return KIT_K_ENTER;
    col = bound(col, 0, KIT_COLS - 1); row = bound(row, 0, KIT_ROWS - 1);
    type = (char)upper(type); len = length(pic);
    if (type == 'N') for (i = 0; i < len && i < w; i++) if (pic[i] == '.') { point = i; break; }
    /* The engine formats dates according to SET DATE/CENTURY. Preserve its
     * separators and skip over them when the operator types only digits. */
    if (type == 'D' && !len && w <= 10) {
        int separators = 0;
        for (i = 0; i < w; i++) {
            if (buf[i] == '/' || buf[i] == '.' || buf[i] == '-') {
                datepic[i] = buf[i]; separators++;
            } else datepic[i] = '9';
        }
        datepic[w] = 0;
        if (separators == 2) { pic = datepic; len = w; }
    }
    /* Literal template positions are stored as well as displayed in III+. */
    for (i = 0; i < w; i++) if (!slot(pic, len, i)) buf[i] = pic[i];
    pos = next_slot(pic, len, 0, w, 1);
    if (pos >= w) {
        kit_put_attr(k, row, col, KIT_ATTR_REVERSE, buf, w);
        return KIT_K_ENTER;
    }
    for (;;) {
        field_draw(k, row, col, buf, w, pos, &offset, limit);
        key = kit_key(k);
        if (key == KIT_K_NONE) continue;
        if (key == KIT_K_LEFT) {
            first = 0;
            i = next_slot(pic, len, pos - 1, w, -1); if (i >= 0) pos = i;
        } else if (key == KIT_K_RIGHT) {
            first = 0;
            i = next_slot(pic, len, pos + 1, w, 1); if (i < w) pos = i;
            else return KIT_K_ENTER;
        } else if (key == KIT_K_HOME) { first = 0; pos = next_slot(pic, len, 0, w, 1); }
        else if (key == KIT_K_END) {
            first = 0;
            pos = w - 1;
            while (pos > 0 && buf[pos] == ' ') pos--;
            if (pos < w - 1) pos++;
            pos = next_slot(pic, len, pos, w, -1);
            if (pos < 0) pos = next_slot(pic, len, 0, w, 1);
        } else if (key == KIT_K_INS) {
            ins = !ins;
            if (k->at[22][0] == KIT_ATTR_REVERSE)
                kit_put_attr(k, 22, 67, KIT_ATTR_REVERSE, ins ? "Ins" : "Ovr", 3);
        } else if (key == KIT_K_CTRL_Y) {
            for (i = 0; i < w; i++) if (slot(pic, len, i)) buf[i] = ' ';
            pos = next_slot(pic, len, 0, w, 1); dirty = 1; first = 0;
        } else if (key == KIT_K_BKSP || key == KIT_K_DEL) {
            first = 0;
            if (key == KIT_K_BKSP) {
                i = next_slot(pic, len, pos - 1, w, -1);
                if (i < 0) continue;
                pos = i;
            }
            for (i = pos; i < w; i = j) {
                j = next_slot(pic, len, i + 1, w, 1);
                buf[i] = j < w ? buf[j] : ' ';
            }
            dirty = 1;
        } else if (key >= 32 && key <= 126) {
            if (key == '.' && point >= 0) {
                if (first) for (i = 0; i < w; i++) if (slot(pic, len, i)) buf[i] = ' ';
                first = 0; dirty = 1;
                align_integer(buf, pic, len, point);
                if (changed) *changed = dirty;
                i = next_slot(pic, len, point + 1, w, 1);
                if (i >= w) return KIT_K_ENTER;
                pos = i; continue;
            }
            c = accept_char(key, type, pos < len ? pic[pos] : 0, caps, alpha);
            if (c < 0) continue;
            /* Numeric first-key replacement must remove the previous
             * right-justified value. Cursor movement selects typeover. */
            if (first && type == 'N')
                for (i = 0; i < w; i++) if (slot(pic, len, i)) buf[i] = ' ';
            first = 0;
            if (ins) {
                for (i = next_slot(pic, len, w - 1, w, -1); i > pos; i = j) {
                    j = next_slot(pic, len, i - 1, w, -1); buf[i] = buf[j];
                }
            }
            buf[pos] = (char)c; dirty = 1;
            if (changed) *changed = dirty;
            i = next_slot(pic, len, pos + 1, w, 1);
            if (i >= w || type == 'L') {
                field_draw(k, row, col, buf, w, pos, &offset, limit);
                return KIT_K_ENTER;
            }
            pos = i;
        } else {
            if (dirty && point >= 0) {
                align_integer(buf, pic, len, point);
                field_draw(k, row, col, buf, w, pos, &offset, limit);
            }
            if (changed) *changed = dirty;
            return key;
        }
        if (changed) *changed = dirty;
    }
}
int kit_edit_field(kit *k, int row, int col, char *buf, int w, char type,
                   const char *picture, int *changed)
{
    char display[512];
    const char *p = picture, *pic;
    int insert_literals = 0, caps, alpha, len, i, used = 0, width = 0, key;
    if (p && *p == '@') {
        for (p++; *p && *p != ' '; p++) if (upper(*p) == 'R') insert_literals = 1;
    }
    if (!insert_literals || upper(type) != 'C' || !buf || w <= 0 || w >= (int)sizeof(display))
        return edit_field(k, row, col, buf, w, type, picture, changed);
    pic = template_start(picture, &caps, &alpha); len = length(pic);
    /* @R inserts literal positions only into the editing display. The
     * caller's raw character field keeps all of its original data slots. */
    while (used < w && width < (int)sizeof(display)) {
        display[width] = slot(pic, len, width) ? buf[used++] : pic[width];
        width++;
    }
    while (width < len && width < (int)sizeof(display) && !slot(pic, len, width)) {
        display[width] = pic[width]; width++;
    }
    if (used < w) return edit_field(k, row, col, buf, w, type, picture, changed);
    key = edit_field(k, row, col, display, width, type, picture, changed);
    for (used = i = 0; i < width && used < w; i++)
        if (slot(pic, len, i)) buf[used++] = display[i];
    return key;
}
int kit_prompt(kit *k, int row, const char *prompt, char *buf, int w)
{
    int n = 0, col, key;
    if (!buf || w <= 0) return 0;
    kit_fill(k, row, 0, KIT_COLS, ' ', KIT_ATTR_NORMAL);
    col = bound(length(prompt), 0, KIT_COLS - 2);
    kit_put_attr(k, row, 0, KIT_ATTR_NORMAL, prompt, col); col++;
    while (n < w && buf[n]) n++;
    while (n < w) buf[n++] = ' ';
    do {
        key = kit_edit_field(k, row, col, buf, w, 'C', (const char *)0, (int *)0);
    } while (key != KIT_K_ENTER && key != KIT_K_SAVE && key != KIT_K_CTRL_END &&
             key != KIT_K_ESC && key != KIT_K_ABORT && key != KIT_K_EOF);
    n = w; while (n > 0 && buf[n - 1] == ' ') n--;
    buf[n] = 0;
    return key == KIT_K_ENTER || key == KIT_K_SAVE || key == KIT_K_CTRL_END;
}
static const char *menu_item(const char *list, int index)
{
    if (!list) return "";
    while (index-- > 0 && *list) list += rt_strlen(list) + 1;
    return list;
}
static int menu_count(const char *list, int *width)
{
    int n = 0, w;
    *width = 1;
    while (list && *list) {
        w = length(list); if (w > *width) *width = w;
        n++; list += w + 1;
    }
    return n;
}
static void clipped_text(kit *k, int row, int col, uint8_t attr, const char *s, int width)
{
    int n = length(s);
    kit_put_attr(k, row, col, attr, s, n < width ? n : width);
}
int kit_menu(kit *k, const char *const *titles, const char *const *items,
             int n, int start_pad, const char *const *help_lines)
{
    kit_saved saved;
    int pad, item = 0, count, width, first = 0, col, left, i, key, result = -1, height;
    uint8_t attr = k->attr;
    if (!titles || !items || n <= 0) return -1;
    pad = bound(start_pad, 0, n - 1); kit_save(k, &saved);
    for (;;) {
        kit_restore(k, &saved);
        kit_fill(k, 0, 0, KIT_COLS, ' ', KIT_ATTR_NORMAL);
        left = 0; col = 1;
        for (i = 0; i < n; i++) {
            int w = length(titles[i]);
            if (i == pad) left = col - 1;
            kit_fill(k, 0, col - 1, w + 2, ' ', i == pad ? KIT_ATTR_REVERSE : KIT_ATTR_NORMAL);
            kit_put_attr(k, 0, col, i == pad ? KIT_ATTR_REVERSE : KIT_ATTR_NORMAL, titles[i], w);
            col += w + 3;
        }
        count = menu_count(items[pad], &width); width = bound(width + 2, 3, KIT_COLS - 2);
        height = count < 19 ? count : 19;
        item = count ? bound(item, 0, count - 1) : 0;
        if (item < first) first = item;
        if (height && item >= first + height) first = item - height + 1;
        left = bound(left, 0, KIT_COLS - width - 2);
        k->attr = KIT_ATTR_NORMAL; kit_box(k, 1, left, height + 2, left + width + 1, 0);
        for (i = 0; i < height; i++) {
            uint8_t a = first + i == item ? KIT_ATTR_REVERSE : KIT_ATTR_NORMAL;
            kit_fill(k, 2 + i, left + 1, width, ' ', a);
            clipped_text(k, 2 + i, left + 2, a, menu_item(items[pad], first + i), width - 2);
        }
        kit_message(k, 23, "Left/Right: menu   Up/Down: option   Enter: select   Esc/F10: previous");
        kit_message(k, 24, help_lines ? help_lines[pad] : "Select an option with the cursor keys.");
        kit_cursor(k, 2 + item - first, left + 2);
        key = kit_key(k);
        if (key == KIT_K_EOF || key == KIT_K_ESC || key == KIT_K_ABORT || key == KIT_K_F10) break;
        if (key == KIT_K_ENTER && count) { result = (pad << 8) | item; break; }
        if (key == KIT_K_LEFT || key == KIT_K_RIGHT || key == KIT_K_TAB) {
            pad = (pad + (key == KIT_K_LEFT ? n - 1 : 1)) % n; item = first = 0;
        } else if (key == KIT_K_UP && count) item = (item + count - 1) % count;
        else if (key == KIT_K_DOWN && count) item = (item + 1) % count;
        else if (key == KIT_K_HOME) item = 0;
        else if (key == KIT_K_END && count) item = count - 1;
        else if (key == KIT_K_PGUP) item = bound(item - height, 0, count ? count - 1 : 0);
        else if (key == KIT_K_PGDN) item = bound(item + height, 0, count ? count - 1 : 0);
        else if (key >= 32 && key <= 126 && count) {
            for (i = 1; i <= count; i++) {
                int at = (item + i) % count;
                if (upper(menu_item(items[pad], at)[0]) == upper(key)) { item = at; break; }
            }
        }
    }
    kit_restore(k, &saved); k->attr = attr; return result;
}
int kit_pick(kit *k, int top, int left, const char *const *items, int n, int start)
{
    kit_saved saved;
    int selected, first = 0, width = 1, height, i, key, result = -1;
    uint8_t attr = k->attr;
    if (!items || n <= 0) return -1;
    top = bound(top, 0, 19); left = bound(left, 0, KIT_COLS - 4);
    height = 20 - top; if (height > n) height = n;
    for (i = 0; i < n; i++) if (length(items[i]) > width) width = length(items[i]);
    width = bound(width + 2, 2, KIT_COLS - left - 2);
    selected = bound(start, 0, n - 1); kit_save(k, &saved);
    k->attr = KIT_ATTR_NORMAL;
    kit_box(k, top, left, top + height + 1, left + width + 1, 0);
    for (;;) {
        if (selected < first) first = selected;
        if (selected >= first + height) first = selected - height + 1;
        for (i = 0; i < height; i++) {
            uint8_t a = first + i == selected ? KIT_ATTR_REVERSE : KIT_ATTR_NORMAL;
            kit_fill(k, top + i + 1, left + 1, width, ' ', a);
            clipped_text(k, top + i + 1, left + 2, a, items[first + i], width - 2);
        }
        kit_cursor(k, top + 1 + selected - first, left + 2);
        key = kit_key(k);
        if (key == KIT_K_EOF || key == KIT_K_ESC || key == KIT_K_ABORT || key == KIT_K_F10) break;
        if (key == KIT_K_ENTER) { result = selected; break; }
        if (key == KIT_K_UP) selected = (selected + n - 1) % n;
        else if (key == KIT_K_DOWN) selected = (selected + 1) % n;
        else if (key == KIT_K_HOME) selected = 0;
        else if (key == KIT_K_END) selected = n - 1;
        else if (key == KIT_K_PGUP) selected = bound(selected - height, 0, n - 1);
        else if (key == KIT_K_PGDN) selected = bound(selected + height, 0, n - 1);
        else if (key >= 32 && key <= 126) {
            for (i = 1; i <= n; i++) {
                int at = (selected + i) % n;
                if (items[at] && upper(items[at][0]) == upper(key)) { selected = at; break; }
            }
        }
    }
    kit_restore(k, &saved); k->attr = attr; return result;
}
