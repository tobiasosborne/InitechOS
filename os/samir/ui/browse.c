/* BROWSE -- dBASE III PLUS table editor.
 * Ref: dbase3-decomp/specs/full-screen/assist-ui-and-editors.md, section 5.
 * Writes use REPLACE/DELETE/RECALL so the engine maintains open NDX files.
 */
#include "ui_apps.h"
#include "samir/kit.h"
#include "samir/workarea.h"
#include "samir/nav.h"
#include "samir/dbf.h"
#include "samir/rt.h"

#define BR_FIELDS 128
#define BR_ROWS 20
#define BR_FIRST 2
#define BR_GUTTER 8

typedef struct browse_state {
    kit screen;
    xb_interp *ip;
    wa_env *env;
    dbf_table *table;
    int area, count, field, left, locked, frozen, width, noappend, nomenu, nofollow;
    int col[BR_FIELDS], span[BR_FIELDS], fields[BR_FIELDS];
    uint32_t rows[BR_ROWS], top;
    xb_val values[BR_FIELDS];
    char variable[11];
} browse_state;

/* The UI is modal. Keep the screen and decoded record off the DOS stack. */
static int br_pending;
static int32_t (*br_conin)(samir_pal_t *);

static char br_upper(char c)
{ return c >= 'a' && c <= 'z' ? (char)(c - 'a' + 'A') : c; }

static int br_equal(const char *a, const char *b)
{
    while (*a && br_upper(*a) == br_upper(*b)) { a++; b++; }
    return !*a && !*b;
}

static int br_keyword(const char *s, const char *word)
{
    int n = 0;
    while (s[n] && word[n] && br_upper(s[n]) == word[n]) n++;
    return !s[n] && (!word[n] || n >= 4);
}

static const char *br_word(const char *s, char word[32])
{
    int n = 0;
    while (*s == ' ' || *s == '\t' || *s == ',') s++;
    while (*s && *s != ' ' && *s != '\t' && *s != ',') {
        if (n < 31) word[n++] = br_upper(*s);
        s++;
    }
    word[n] = 0;
    return s;
}

static int br_option(const char *s)
{
    static const char *const words[] = {
        "FIELDS", "WIDTH", "LOCK", "FREEZE", "NOFOLLOW", "NOAPPEND", "NOMENU"
    };
    int i;
    for (i = 0; i < 7; i++) if (br_keyword(s, words[i])) return i + 1;
    return 0;
}

static int br_field_index(browse_state *b, const char *name)
{
    int i;
    for (i = 0; i < dbf_nfields(b->table); i++)
        if (br_equal(name, dbf_field(b->table, i)->name)) return i;
    return -1;
}

static int br_number(const char *s)
{
    int n = 0;
    if (!*s) return -1;
    while (*s) {
        if (*s < '0' || *s > '9' || n > 1000000) return -1;
        n = n * 10 + *s++ - '0';
    }
    return n;
}

static int br_options(browse_state *b, const char *s)
{
    char word[32], operand[32];
    int i, list = 0, has_fields = 0, freeze = -1;
    b->frozen = -1;
    b->width = 254;
    while (1) {
        s = br_word(s, word);
        if (!word[0]) break;
        i = br_option(word);
        if (!i && list) {
            int f = br_field_index(b, word);
            if (f < 0 || b->count == BR_FIELDS) return 48;
            b->fields[b->count++] = f;
            continue;
        }
        list = 0;
        switch (i) {
        case 1: b->count = 0; has_fields = list = 1; break;
        case 2:
        case 3:
            s = br_word(s, operand);
            if (br_number(operand) < (i == 2 ? 1 : 0)) return 10;
            if (i == 2) b->width = br_number(operand);
            else b->locked = br_number(operand);
            break;
        case 4:
            s = br_word(s, operand);
            freeze = br_field_index(b, operand);
            if (freeze < 0) return 48;
            break;
        case 5: b->nofollow = 1; break;
        case 6: b->noappend = 1; break;
        case 7: b->nomenu = 1; break;
        default: return 36;
        }
    }
    if (!b->count) {
        if (has_fields) return 10;
        b->count = dbf_nfields(b->table);
        for (i = 0; i < b->count; i++) b->fields[i] = i;
    }
    if (b->locked >= b->count) b->locked = b->count - 1;
    if (b->locked < 0) b->locked = 0;
    b->left = b->locked;
    if (freeze >= 0) {
        for (i = 0; i < b->count; i++)
            if (b->fields[i] == freeze) b->frozen = b->field = i;
        if (b->frozen < 0) return 48;
    }
    return 0;
}

static void br_uint(char *out, uint32_t n, int width)
{
    int i;
    rt_memset(out, ' ', (uint32_t)width);
    for (i = width - 1; i >= 0; i--) {
        out[i] = (char)('0' + n % 10u);
        n /= 10u;
        if (!n) break;
    }
}

static int br_width(browse_state *b, int i)
{
    const dbf_field_t *f = dbf_field(b->table, b->fields[i]);
    int w = f->field_len, name = (int)rt_strlen(f->name);
    if (f->type == 'C' && w > b->width) w = b->width;
    if (f->type == 'D') w = xb_interp_ctx(b->ip)->set_century ? 10 : 8;
    if (f->type == 'M') w = 10;
    if (w < name) w = name;
    if (w > 70) w = 70;
    return w;
}

static void br_layout(browse_state *b)
{
    int i, x, pass;
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < b->count; i++) { b->col[i] = -1; b->span[i] = 0; }
        x = BR_GUTTER;
        for (i = 0; i < b->count && x < 79; i++) {
            int w;
            if (i >= b->locked && i < b->left) continue;
            w = br_width(b, i);
            /* Leave room for the active, unpinned column. */
            if (i < b->locked && b->field >= b->locked && x + w > 57) continue;
            if (w > 79 - x) w = 79 - x;
            b->col[i] = x;
            b->span[i] = w;
            x += w + 1;
        }
        if (b->col[b->field] >= 0) break;
        b->left = b->field;
    }
}

static int br_value(browse_state *b, int index, char buf[256])
{
    const dbf_field_t *f = dbf_field(b->table, index);
    const xb_val *v = &b->values[index];
    int n = f->field_len;
    rt_memset(buf, ' ', 255);
    buf[255] = 0;
    if (f->type == 'C') {
        if (v->t == XB_C) rt_memcpy(buf, v->u.c.p, v->u.c.len);
    } else if (f->type == 'N') {
        if (v->t == XB_N) dec_format_even(v->u.n, n, f->dec_count, buf);
    } else if (f->type == 'D') {
        xb_ctx *ctx = xb_interp_ctx(b->ip);
        n = (int)rt_date_format(v->u.d, ctx->set_date_fmt, ctx->set_century, buf, 0);
    } else if (f->type == 'L') {
        buf[0] = v->t == XB_L ? (v->u.l ? 'T' : 'F') : '?';
    } else {
        rt_memcpy(buf, "<Memo>", 6);
        n = 10;
    }
    return n;
}

static int br_row(browse_state *b, uint32_t rec)
{
    int i;
    for (i = 0; i < BR_ROWS; i++) if (b->rows[i] == rec) return i;
    return -1;
}

static int br_draw(browse_state *b)
{
    kit *k = &b->screen;
    uint32_t current = wa_recno(b->env, b->area);
    int r, i, deleted = 0;
    char value[256], gutter[8];
    br_layout(b);
    kit_clear_rows(k, 0, 21);
    if (!b->nomenu)
        kit_put_attr(k, 0, 0, KIT_ATTR_REVERSE,
                     " Bottom  Top  Lock  Record No.  Freeze  Find  None  Help       F1/F10 Menu ", -1);
    kit_fill(k, 1, 0, 80, ' ', KIT_ATTR_REVERSE);
    kit_put_attr(k, 1, 0, KIT_ATTR_REVERSE, "Record", -1);
    for (i = 0; i < b->count; i++) if (b->col[i] >= 0) {
        const char *name = dbf_field(b->table, b->fields[i])->name;
        int n = (int)rt_strlen(name);
        if (n > b->span[i]) n = b->span[i];
        kit_put_attr(k, 1, b->col[i], KIT_ATTR_REVERSE, name, n);
    }
    rt_memset(b->rows, 0, sizeof(b->rows));
    if (dbf_nrec(b->table)) {
        if (wa_nav_goto(b->env, b->area, b->top) != NAV_OK) return -1;
        for (r = 0; r < BR_ROWS && !wa_eof(b->env, b->area); r++) {
            uint32_t rec = wa_recno(b->env, b->area);
            b->rows[r] = rec;
            if (dbf_read_rec(b->table, rec, b->values, &deleted) != DBF_OK) return -1;
            br_uint(gutter, rec, 6);
            gutter[6] = deleted ? '*' : ' ';
            gutter[7] = (char)179;
            kit_put(k, r + BR_FIRST, 0, gutter, 8);
            for (i = 0; i < b->count; i++) if (b->col[i] >= 0) {
                int n = br_value(b, b->fields[i], value);
                uint8_t attr = rec == current && i == b->field ? KIT_ATTR_REVERSE : KIT_ATTR_NORMAL;
                if (n > b->span[i]) n = b->span[i];
                kit_fill(k, r + BR_FIRST, b->col[i], b->span[i], ' ', attr);
                kit_put_attr(k, r + BR_FIRST, b->col[i], attr, value, n);
                kit_fill(k, r + BR_FIRST, b->col[i] + b->span[i], 1, (char)179, KIT_ATTR_NORMAL);
            }
            if (wa_nav_skip(b->env, b->area, 1) != NAV_OK) return -1;
        }
        if (wa_nav_goto(b->env, b->area, current) != NAV_OK) return -1;
        if (dbf_read_rec(b->table, current, b->values, &deleted) != DBF_OK) return -1;
    } else {
        kit_message(k, 10, "No records. Press Down to add a record.");
    }
    kit_status(k, "BROWSE", dbf_name(b->table), current, dbf_nrec(b->table), 0);
    if (deleted) kit_put_attr(k, 22, 64, KIT_ATTR_REVERSE, "Del", 3);
    kit_message(k, 23, "Up/Down: Record  PgUp/PgDn: Page  Left/Right: Field  Enter: Next field");
    kit_message(k, 24, "Type: Edit  Ctrl-U: Delete/Recall  Ctrl-W/Ctrl-End: Save  Esc: Exit");
    r = br_row(b, current);
    if (r >= 0) kit_cursor(k, r + BR_FIRST, b->col[b->field]);
    return 0;
}

static int br_run(browse_state *b, const char *command, int *ec)
{
    int rc = samir_do(b->ip, command);
    if (rc < 0 && ec) *ec = samir_last_error(b->ip);
    b->table = wa_table(b->env, b->area);
    return rc;
}

/* Replay the printable key that selected edit mode. The PAL itself remains
 * the same object; restore its input method before running any engine verb. */
static int32_t br_edit_input(samir_pal_t *pal)
{
    if (br_pending >= 0) { int c = br_pending; br_pending = -1; return c; }
    return br_conin(pal);
}

static int br_edit(browse_state *b, int key, int *ec)
{
    const dbf_field_t *f = dbf_field(b->table, b->fields[b->field]);
    kit *k = &b->screen;
    char buf[256], command[80];
    int n, width, changed = 0, result, rc, i;
    uint32_t ordinal = wa_nav_ordpos(b->area);
    int row = br_row(b, wa_recno(b->env, b->area));
    xb_val value;
    if (!dbf_nrec(b->table)) return KIT_K_NONE;
    if (f->type == 'M') {
        kit_message(k, 24, "Memo editing is unavailable in BROWSE.");
        return KIT_K_NONE;
    }
    if (dbf_read_rec(b->table, wa_recno(b->env, b->area), b->values, &i) != DBF_OK)
        return KIT_K_EOF;
    n = br_value(b, b->fields[b->field], buf);
    /* The kit pans a full-width field through the remaining screen columns.
     * Do not shorten the stored/editable value to a WIDTH display clause. */
    width = n;
    br_pending = key;
    br_conin = k->pal->conin_char;
    k->pal->conin_char = br_edit_input;
    result = kit_edit_field(k, br_row(b, wa_recno(b->env, b->area)) + BR_FIRST,
                            b->col[b->field], buf, width, f->type, 0, &changed);
    k->pal->conin_char = br_conin;
    if (!changed || result == KIT_K_ESC || result == KIT_K_ABORT || result == KIT_K_EOF)
        return result;
    if (f->type == 'C') value = xb_c(buf, (uint16_t)n);
    else if (f->type == 'N') value = xb_n(dec_parse(buf, n));
    else if (f->type == 'L') value = xb_l(br_upper(buf[0]) == 'T' || br_upper(buf[0]) == 'Y');
    else {
        rt_memcpy(command, "CTOD('", 6);
        rt_memcpy(command + 6, buf, (uint32_t)n);
        rt_memcpy(command + 6 + n, "')", 3);
        rc = xb_interp_eval(b->ip, command, &value, ec);
        if (rc < 0) {
            *ec = 0;
            kit_message(k, 24, "Invalid date. Press a key to return to BROWSE.");
            (void)kit_key(k);
            return KIT_K_NONE;
        }
    }
    rc = xb_interp_store(b->ip, b->variable, &value);
    if (rc < 0) { if (ec) *ec = 43; return KIT_K_EOF; }
    rt_memcpy(command, "REPLACE ", 8);
    i = 8;
    n = (int)rt_strlen(f->name);
    rt_memcpy(command + i, f->name, (uint32_t)n); i += n;
    rt_memcpy(command + i, " WITH ", 6); i += 6;
    n = (int)rt_strlen(b->variable);
    rt_memcpy(command + i, b->variable, (uint32_t)n + 1u);
    if (br_run(b, command, ec) < 0) return KIT_K_EOF;
    if (wa_master_order(b->env, b->area) > 0) {
        uint32_t current = wa_recno(b->env, b->area), top;
        /* REPLACE invalidates the index navigation cache. Rebuild it before
         * deciding whether to follow the edited record or keep its ordinal. */
        if (wa_nav_goto(b->env, b->area, current) != NAV_OK) return KIT_K_EOF;
        if (b->nofollow) {
            uint32_t next = wa_nav_seq_recno(b->area, ordinal);
            if (next && wa_nav_goto(b->env, b->area, next) != NAV_OK) return KIT_K_EOF;
        }
        ordinal = wa_nav_ordpos(b->area);
        top = wa_nav_seq_recno(b->area, ordinal > (uint32_t)row ? ordinal - (uint32_t)row : 0);
        if (top) b->top = top;
    }
    return result;
}

static int br_append(browse_state *b, int *ec)
{
    int key;
    if (b->noappend) return 0;
    kit_message(&b->screen, 24, "Add new records? (Y/N)");
    do { key = kit_key(&b->screen); } while (key != 'y' && key != 'Y' &&
        key != 'n' && key != 'N' && key != KIT_K_ESC && key != KIT_K_EOF);
    if (key != 'y' && key != 'Y') return 0;
    if (br_run(b, "APPEND BLANK", ec) < 0) return -1;
    b->top = wa_recno(b->env, b->area);
    return 0;
}

static int br_move(browse_state *b, int steps, int append, int *ec)
{
    uint32_t old = wa_recno(b->env, b->area), next;
    if (!dbf_nrec(b->table)) return append ? br_append(b, ec) : 0;
    if (wa_nav_skip(b->env, b->area, steps) != NAV_OK) return -1;
    if (wa_eof(b->env, b->area) || wa_bof(b->env, b->area)) {
        int end = wa_eof(b->env, b->area);
        if ((end ? wa_nav_go_bottom(b->env, b->area) : wa_nav_go_top(b->env, b->area)) != NAV_OK)
            return -1;
        if (end && old == wa_recno(b->env, b->area) && append) return br_append(b, ec);
    }
    next = wa_recno(b->env, b->area);
    if (br_row(b, next) < 0) {
        if (steps > 0) {
            if (wa_nav_skip(b->env, b->area, -(BR_ROWS - 1)) != NAV_OK) return -1;
            b->top = wa_recno(b->env, b->area);
            if (wa_nav_goto(b->env, b->area, next) != NAV_OK) return -1;
        } else b->top = next;
    }
    return 0;
}

static int br_menu(browse_state *b)
{
    static const char *const titles[] = { "Bottom", "Top", "Record No.", "Lock", "Freeze", "None", "Help" };
    static const char *const items[] = { "Go to bottom\0\0", "Go to top\0\0", "Go to record\0\0",
        "Lock columns\0\0", "Freeze this field\0\0", "Clear Lock/Freeze\0\0", "Navigation keys\0\0" };
    char number[12];
    int choice = kit_menu(&b->screen, titles, items, 7, 0, 0);
    if (choice < 0) return 0;
    choice >>= 8;
    if (choice == 0 || choice == 1) {
        if ((choice ? wa_nav_go_top(b->env, b->area) : wa_nav_go_bottom(b->env, b->area)) != NAV_OK) return -1;
        b->top = wa_recno(b->env, b->area);
    } else if (choice == 2 || choice == 3) {
        int n;
        number[0] = 0;
        if (kit_prompt(&b->screen, 24, choice == 2 ? "Record No.: " : "Lock how many fields: ", number, 10)) {
            n = br_number(number);
            if (choice == 2 && n > 0 && (uint32_t)n <= dbf_nrec(b->table)) {
                if (wa_nav_goto(b->env, b->area, (uint32_t)n) != NAV_OK) return -1;
                b->top = (uint32_t)n;
            } else if (choice == 3 && n >= 0 && n < b->count) b->left = b->locked = n;
        }
    } else if (choice == 4) b->frozen = b->field;
    else if (choice == 5) { b->locked = b->left = 0; b->frozen = -1; }
    else if (choice == 6) {
        kit_message(&b->screen, 23, "Arrows: Move  PgUp/PgDn: Page  Type: Edit  Ctrl-U: Delete/Recall");
        kit_message(&b->screen, 24, "Ctrl-W/Ctrl-End: Save and exit  Esc: Abandon current field  Any key: Return");
        (void)kit_key(&b->screen);
    }
    return 0;
}

int ui_browse(xb_interp *ip, const char *args, int *ec)
{
    browse_state *b;
    samir_pal_t *pal = xb_interp_pal(ip);
    int key, rc = CMD_OK, error, i, j;
    wa_env *env = xb_interp_env(ip);
    *ec = 0;
    if (!wa_is_open(env, wa_selected(env))) { *ec = XBEE_NO_DATABASE; return -INTERP_ERR_EVAL; }
    b = pal->acquire ? pal->acquire(pal, sizeof(*b)) : (void *)0;
    if (!b) { *ec = 43; return -INTERP_ERR_NOMEM; }
    rt_memset(b, 0, sizeof(*b));
    b->ip = ip; b->env = env; b->area = wa_selected(env); b->table = wa_table(env, b->area);
    error = br_options(b, args);
    if (error) { *ec = error; pal->release(pal, b); return -INTERP_ERR_SYNTAX; }
    if (xb_interp_scope_enter(ip) < 0) { *ec = 43; pal->release(pal, b); return -INTERP_ERR_EVAL; }
    /* Pick a private temporary whose name cannot resolve to a DBF field. */
    rt_memcpy(b->variable, "UIBR000000", 11);
    for (i = 0; br_field_index(b, b->variable) >= 0 && i < BR_FIELDS; i++) {
        br_uint(b->variable + 4, (uint32_t)i + 1u, 6);
        for (j = 4; j < 10; j++) if (b->variable[j] == ' ') b->variable[j] = '0';
    }
    if (xb_interp_declare_private(ip, b->variable) < 0) {
        xb_interp_scope_leave(ip); pal->release(pal, b); *ec = 43; return -INTERP_ERR_EVAL;
    }
    if (wa_eof(env, b->area) && dbf_nrec(b->table)) {
        if (wa_nav_go_bottom(env, b->area) != NAV_OK) { rc = -INTERP_ERR_EVAL; goto leave; }
    }
    b->top = wa_recno(env, b->area);
    kit_open(&b->screen, ip);
    while (1) {
        if (br_draw(b) < 0) { rc = -INTERP_ERR_EVAL; *ec = 29; break; }
        key = kit_key(&b->screen);
        if ((key >= 32 && key <= 126) || key == KIT_K_BKSP || key == KIT_K_DEL || key == KIT_K_CTRL_Y)
            key = br_edit(b, key == KIT_K_DEL ? 7 : key == KIT_K_CTRL_Y ? 25 : key, ec);
        if (key == KIT_K_SAVE || key == KIT_K_CTRL_END || key == KIT_K_ESC || key == KIT_K_ABORT || key == KIT_K_EOF) {
            if (key == KIT_K_EOF && *ec) rc = -INTERP_ERR_EVAL;
            break;
        }
        if (key == KIT_K_UP || key == KIT_K_DOWN || key == KIT_K_PGUP || key == KIT_K_PGDN) {
            int steps = key == KIT_K_UP ? -1 : key == KIT_K_DOWN ? 1 : key == KIT_K_PGUP ? -BR_ROWS : BR_ROWS;
            if (br_move(b, steps, key == KIT_K_DOWN, ec) < 0) { rc = -INTERP_ERR_EVAL; break; }
        } else if (key == KIT_K_RIGHT || key == KIT_K_ENTER || key == KIT_K_TAB || key == KIT_K_LEFT || key == KIT_K_HOME || key == KIT_K_END) {
            if (b->frozen < 0) {
                if (key == KIT_K_HOME) b->field = 0;
                else if (key == KIT_K_END) b->field = b->count - 1;
                else if (key == KIT_K_LEFT) { if (b->field) b->field--; }
                else if (++b->field >= b->count) {
                    b->field = 0;
                    if (br_move(b, 1, 1, ec) < 0) { rc = -INTERP_ERR_EVAL; break; }
                }
                if (b->field < b->left && b->field >= b->locked) b->left = b->field;
            } else if (key == KIT_K_ENTER || key == KIT_K_TAB) {
                if (br_move(b, 1, 1, ec) < 0) { rc = -INTERP_ERR_EVAL; break; }
            }
        } else if (key == KIT_K_TOGGLE_DEL && dbf_nrec(b->table)) {
            int del = wa_record_deleted(env, b->area);
            if (del < 0 || br_run(b, del ? "RECALL" : "DELETE", ec) < 0) { rc = -INTERP_ERR_EVAL; break; }
        } else if ((key == KIT_K_F1 || key == KIT_K_F10) && !b->nomenu) {
            if (br_menu(b) < 0) { rc = -INTERP_ERR_EVAL; break; }
        } else if (key == KIT_K_CTRL_HOME) {
            if (wa_nav_go_top(env, b->area) != NAV_OK) { rc = -INTERP_ERR_EVAL; break; }
            b->top = wa_recno(env, b->area);
        }
    }
    kit_close(&b->screen);
leave:
    xb_interp_scope_leave(ip);
    pal->release(pal, b);
    return rc;
}
