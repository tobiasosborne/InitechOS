/* command_sweep.h -- the GOLDEN-MASTER item 4 dead-command sweep.
 *
 * One engine, copied byte for byte into the tests/ of Note Pad, Calculator,
 * Control Panels, InitechText and InitechPaint (each component is
 * self-contained; the copies go when the SDK host publishes one). It reads
 * the app README's frozen command inventory, between the markers
 * "<!-- command-inventory NAME -->" and "<!-- end-command-inventory -->"
 * (NAME is SWEEP_BLOCK, the program's on-disk name):
 *
 *   Context: NAME = STEP; STEP; ...     user actions after a fresh launch
 *   Title: TITLE | N                    the bar, in order, N rows each
 *   Menu: TITLE | ROW | KEY | CTX | EFFECT   one row, in order (KEY or -)
 *   Menu: TITLE | -                     a divider row
 *   Absent: TITLE | ROW                 a frozen omission
 *   Key: MODS | CODES | CTX | EFFECT    keyboard routes (MODS none, shift,
 *                                       control or option; CODES hex
 *                                       cXX[-YY] for the character byte with
 *                                       no key code, vXX[-YY] for the key
 *                                       code with no character)
 *   Button: LABEL@N | CTX | EFFECT      a clickable dialog item: its text
 *                                       and its item number N, the order of
 *                                       the item list handed to the gate
 *
 * STEP is: type TEXT | key [MODS:]cXX/vXX | menu TITLE/ROW | scrap TEXT |
 * scraplines N | file NAME TEXT | sfreply NAME | sfcancel | alert N |
 * button LABEL@N | click X Y | update. "\r" in TEXT is a carriage return.
 * scrap and scraplines put TEXT on the desk scrap as another program would,
 * then bring this one to the front again (activateEvt).
 * The context "base" is the fresh launch itself.
 *
 * EFFECT names what must change (several joined by '+'): screen (the
 * complete paint of a forced update, window list and dialog items), scrap,
 * file (every byte on the host volume), exit, alert, dialog (standard file
 * or FILE COPY state), beep, settings (the OS settings record and clock) or
 * menu (the installed bar's rows and enable bytes). Liveness is judged only
 * from these host-observable outputs, never from the app's variables.
 *
 * Checks, all against the README and nothing the app computes:
 *  1. In every context, the installed bar equals the Title/Menu rows: order,
 *     row text, command key, divider. No row says it is not implemented.
 *     No Absent row is installed.
 *  2. Each command row is ENABLED in its context (no grayed stub) and, chosen
 *     there, changes its declared effect. Its command key, sent as the shell
 *     sends it (Control is Command on this keyboard; flair/tbxmenu.h), has
 *     exactly the same outcome as the menu choice.
 *  3. Every character byte and every key code, under each of the four
 *     modifiers, is sent in base and in every Key/Button context. A key is
 *     live when any effect changes. Every live key not resolved by the shell
 *     to a menu row must be declared by a Key line (no hidden route), and
 *     every declared key must be live with its effect in its own context
 *     (no dead route).
 *  4. Every clickable dialog item visible in a swept context must have a
 *     Button line, and each Button line's item, clicked in its context,
 *     changes its effect.
 *
 * The including test defines SWEEP_README (path), SWEEP_BLOCK and SWEEP_NAME, includes
 * the app source and tbx_host.h first, and supplies sweep_launch(): a fresh
 * volume, tbx_app_main() and the first update. The summary is the factory's
 * counted form "NAME: N checks, F failures".
 */
#ifndef INITECH_COMMAND_SWEEP_H
#define INITECH_COMMAND_SWEEP_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void sweep_launch(void);

enum { SW_SCREEN, SW_SCRAP, SW_FILE, SW_EXIT, SW_ALERT, SW_DIALOG, SW_BEEP,
       SW_SETTINGS, SW_MENU, SW_NCLASS };
static const char *const sw_class[SW_NCLASS] = {
    "screen","scrap","file","exit","alert","dialog","beep","settings","menu" };
static const char *const sw_modname[4] = { "none","shift","control","option" };
static const int sw_modbits[4] = { 0, FLAIR_EVT_MOD_SHIFT_KEY,
    FLAIR_EVT_MOD_CONTROL_KEY, FLAIR_EVT_MOD_OPTION_KEY };

#define SW_MAX 96
typedef struct { char name[32], script[512]; } sw_ctx_t;
typedef struct { char title[40]; int rows; } sw_title_t;
typedef struct { char title[40], row[64], ctx[32]; int key, divider, effect; } sw_row_t;
typedef struct { char title[40], row[64]; } sw_absent_t;
typedef struct { int mod, lo, hi, vk; char ctx[32]; int effect; } sw_key_t;
typedef struct { char label[64], ctx[32]; int effect; } sw_button_t;

static sw_ctx_t sw_ctxs[SW_MAX]; static int sw_nctx;
static sw_title_t sw_titles[SW_MAX]; static int sw_ntitle;
static sw_row_t sw_rows[SW_MAX]; static int sw_nrow;
static sw_absent_t sw_absents[SW_MAX]; static int sw_nabsent;
static sw_key_t sw_keys[SW_MAX]; static int sw_nkey;
static sw_button_t sw_buttons[SW_MAX]; static int sw_nbutton;
static long sw_checks, sw_fails;

#define SW_CHECK(c, ...) do { sw_checks++; if (!(c)) { sw_fails++; \
    printf("FAIL " SWEEP_NAME ": "); printf(__VA_ARGS__); printf("\n"); } } while (0)

typedef struct { uint64_t h[SW_NCLASS]; } sw_snap_t;

static uint64_t sw_fnv(uint64_t h, const void *p, size_t n)
{ const unsigned char *b = p; for (size_t i = 0; i < n; i++) h = (h ^ b[i]) * 1099511628211ull; return h; }
static uint64_t sw_str(uint64_t h, const char *s) { return sw_fnv(h, s ? s : "", s ? strlen(s) + 1 : 1); }
static uint64_t sw_int(uint64_t h, long v) { return sw_fnv(h, &v, sizeof v); }

static void sw_event(int what, int32_t msg, int mods, int x, int y)
{
    tbx_event_t ev = { what, msg, mods, 0, x, y };
    if (tbx_host.exit_calls) return;
    if (!tbx_host_push_event(&ev)) { printf("FAIL " SWEEP_NAME ": event queue\n"); exit(2); }
    (void)tbx_host_dispatch(tbx_app_event);
}

static sw_snap_t sw_snap(void)
{
    sw_snap_t s; uint64_t h = 1469598103934665603ull;
    for (int i = 0; i < SW_NCLASS; i++) s.h[i] = h;
    tbx_host.nops = 0;
    if (!tbx_host.exit_calls) sw_event(updateEvt, 0, 0, 0, 0);
    for (unsigned i = 0; i < tbx_host.nops; i++) {
        const tbx_host_op_t *o = &tbx_host.ops[i];
        h = sw_str(h, o->name); h = sw_fnv(h, o->a, sizeof o->a); h = sw_str(h, o->text);
    }
    h = sw_int(h, tbx_host.nwindows);
    for (int w = 0; w < tbx_host.nwindows; w++) {
        h = sw_int(h, tbx_host.windows[w].l); h = sw_int(h, tbx_host.windows[w].t);
        h = sw_int(h, tbx_host.windows[w].r); h = sw_int(h, tbx_host.windows[w].b);
        h = sw_str(h, tbx_host.windows[w].title);
        h = sw_int(h, tbx_host.windows[w].scroll_len); h = sw_int(h, tbx_host.windows[w].scroll_val);
    }
    h = sw_int(h, tbx_host.scroll_len); h = sw_int(h, tbx_host.scroll_val);
    if (tbx_host.dialog_items)
        for (unsigned i = 0; i < tbx_host.dialog_count; i++) {
            const cp_item_t *d = &tbx_host.dialog_items[i];
            h = sw_int(h, d->type); h = sw_int(h, d->value); h = sw_str(h, d->text);
            h = sw_int(h, d->left); h = sw_int(h, d->top);
        }
    s.h[SW_SCREEN] = h;
    h = sw_int(s.h[SW_SCRAP], tbx_host.scrap_len);
    if (tbx_host.scrap_len > 0) h = sw_fnv(h, tbx_host.scrap, (size_t)tbx_host.scrap_len);
    s.h[SW_SCRAP] = sw_int(h, tbx_host.scrap_puts);
    h = s.h[SW_FILE];
    for (int i = 0; i < TBX_HOST_FILES; i++) {
        const tbx_host_file_t *f = &tbx_host.files[i];
        if (!f->used) continue;
        h = sw_str(h, f->name); h = sw_int(h, f->size); h = sw_int(h, f->attr);
        if (f->size) h = sw_fnv(h, f->data, f->size);
    }
    s.h[SW_FILE] = sw_int(h, tbx_host.ndeleted);
    s.h[SW_EXIT] = sw_int(s.h[SW_EXIT], tbx_host.exit_calls);
    h = sw_int(s.h[SW_ALERT], tbx_host.alerts); h = sw_int(h, tbx_host.alert_pending);
    s.h[SW_ALERT] = sw_str(h, tbx_host.alert_text);
    h = sw_int(s.h[SW_DIALOG], tbx_host.sf); h = sw_str(h, tbx_host.sf_prompt);
    h = sw_str(h, tbx_host.sf_name); h = sw_int(h, tbx_host.copy_pending);
    s.h[SW_DIALOG] = sw_str(h, tbx_host.copy_caption);
    s.h[SW_BEEP] = sw_int(s.h[SW_BEEP], (long)tbx_host.beeps);
    h = sw_fnv(s.h[SW_SETTINGS], &tbx_host.settings, sizeof tbx_host.settings);
    s.h[SW_SETTINGS] = sw_fnv(h, &tbx_host.clock, sizeof tbx_host.clock);
    h = s.h[SW_MENU];
    if (tbx_host.bar)
        for (unsigned m = 0; m < tbx_host.bar->n_menus; m++) {
            const tbx_menuinfo_t *mi = &tbx_host.bar->menus[m];
            h = sw_str(h, mi->title); h = sw_int(h, mi->n_items);
            for (unsigned r = 0; r < mi->n_items; r++) {
                const tbx_menuitem_t *it = &mi->items[r];
                h = sw_str(h, it->text); h = sw_int(h, it->cmd_char); h = sw_int(h, it->enabled);
                h = sw_int(h, it->mark); h = sw_int(h, it->is_divider);
            }
        }
    s.h[SW_MENU] = h;
    return s;
}
static int sw_diff(const sw_snap_t *a, const sw_snap_t *b)
{ int m = 0; for (int i = 0; i < SW_NCLASS; i++) if (a->h[i] != b->h[i]) m |= 1 << i; return m; }
static void sw_effects(int m, char *out, size_t n)
{
    out[0] = 0;
    for (int i = 0; i < SW_NCLASS; i++) if (m & (1 << i)) {
        if (out[0]) strncat(out, "+", n - strlen(out) - 1);
        strncat(out, sw_class[i], n - strlen(out) - 1);
    }
    if (!out[0]) snprintf(out, n, "nothing");
}

/* ---- the README inventory ---------------------------------------------- */
static char *sw_trim(char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    size_t n = strlen(s);
    while (n && (s[n-1] == ' ' || s[n-1] == '\t' || s[n-1] == '\r' || s[n-1] == '\n')) s[--n] = 0;
    return s;
}
static int sw_fields(char *s, char **f, int max)
{
    int n = 0;
    for (;;) {
        char *bar = strchr(s, '|');
        if (bar) *bar = 0;
        if (n < max) f[n++] = sw_trim(s);
        if (!bar) break;
        s = bar + 1;
    }
    return n;
}
static int sw_effect(const char *s)
{
    int m = 0; char buf[128]; snprintf(buf, sizeof buf, "%s", s);
    for (char *t = strtok(buf, "+"); t; t = strtok(0, "+")) {
        int k = -1;
        for (int i = 0; i < SW_NCLASS; i++) if (!strcmp(sw_trim(t), sw_class[i])) k = i;
        if (k < 0) { printf("FAIL " SWEEP_NAME ": README effect '%s'\n", t); exit(2); }
        m |= 1 << k;
    }
    return m;
}
static int sw_mod(const char *s)
{
    for (int i = 0; i < 4; i++) if (!strcmp(s, sw_modname[i])) return i;
    printf("FAIL " SWEEP_NAME ": README modifier '%s'\n", s); exit(2);
}
static void sw_bad(const char *line) { printf("FAIL " SWEEP_NAME ": README line '%s'\n", line); exit(2); }
static void sw_read_inventory(void)
{
    FILE *f = fopen(SWEEP_README, "r");
    char line[1024], copy[1024]; int in = 0, seen = 0;
    const char *open_marker = "<!-- command-inventory " SWEEP_BLOCK " -->";
    if (!f) { printf("FAIL " SWEEP_NAME ": cannot read %s\n", SWEEP_README); exit(2); }
    while (fgets(line, sizeof line, f)) {
        char *s = sw_trim(line), *fld[8]; int n;
        if (!strcmp(s, open_marker)) { in = 1; seen = 1; continue; }
        if (!strcmp(s, "<!-- end-command-inventory -->")) { in = 0; continue; }
        if (!in || !*s) continue;
        snprintf(copy, sizeof copy, "%s", s);
        if (!strncmp(s, "Context:", 8)) {
            char *eq = strchr(s, '=');
            if (!eq || sw_nctx == SW_MAX) sw_bad(copy);
            *eq = 0;
            snprintf(sw_ctxs[sw_nctx].name, 32, "%s", sw_trim(s + 8));
            snprintf(sw_ctxs[sw_nctx].script, 512, "%s", sw_trim(eq + 1)); sw_nctx++;
        } else if (!strncmp(s, "Title:", 6)) {
            n = sw_fields(s + 6, fld, 8);
            if (n != 2 || sw_ntitle == SW_MAX) sw_bad(copy);
            snprintf(sw_titles[sw_ntitle].title, 40, "%s", fld[0]);
            sw_titles[sw_ntitle++].rows = atoi(fld[1]);
        } else if (!strncmp(s, "Menu:", 5)) {
            sw_row_t *r = &sw_rows[sw_nrow];
            n = sw_fields(s + 5, fld, 8);
            if (sw_nrow == SW_MAX || !(n == 2 || n == 5)) sw_bad(copy);
            memset(r, 0, sizeof *r);
            snprintf(r->title, 40, "%s", fld[0]); snprintf(r->row, 64, "%s", fld[1]);
            if (n == 2) { if (strcmp(fld[1], "-")) sw_bad(copy); r->divider = 1; }
            else {
                r->key = strcmp(fld[2], "-") ? (unsigned char)fld[2][0] : 0;
                if (r->key && fld[2][1]) sw_bad(copy);
                snprintf(r->ctx, 32, "%s", fld[3]); r->effect = sw_effect(fld[4]);
            }
            sw_nrow++;
        } else if (!strncmp(s, "Absent:", 7)) {
            n = sw_fields(s + 7, fld, 8);
            if (n != 2 || sw_nabsent == SW_MAX) sw_bad(copy);
            snprintf(sw_absents[sw_nabsent].title, 40, "%s", fld[0]);
            snprintf(sw_absents[sw_nabsent++].row, 64, "%s", fld[1]);
        } else if (!strncmp(s, "Key:", 4)) {
            n = sw_fields(s + 4, fld, 8);
            if (n != 4) sw_bad(copy);
            int mod = sw_mod(fld[0]), effect = sw_effect(fld[3]);
            char codes[256]; snprintf(codes, sizeof codes, "%s", fld[1]);
            for (char *t = strtok(codes, " "); t; t = strtok(0, " ")) {
                sw_key_t *k = &sw_keys[sw_nkey]; char *dash; char *end;
                if (sw_nkey == SW_MAX || (t[0] != 'c' && t[0] != 'v')) sw_bad(copy);
                k->mod = mod; k->vk = t[0] == 'v'; k->effect = effect;
                snprintf(k->ctx, 32, "%s", fld[2]);
                k->lo = (int)strtol(t + 1, &end, 16);
                dash = strchr(t, '-');
                k->hi = dash ? (int)strtol(dash + 1, 0, 16) : k->lo;
                if (end == t + 1 || k->lo < 1 || k->hi > 255 || k->hi < k->lo) sw_bad(copy);
                sw_nkey++;
            }
        } else if (!strncmp(s, "Button:", 7)) {
            n = sw_fields(s + 7, fld, 8);
            if (n != 3 || sw_nbutton == SW_MAX) sw_bad(copy);
            snprintf(sw_buttons[sw_nbutton].label, 64, "%s", fld[0]);
            snprintf(sw_buttons[sw_nbutton].ctx, 32, "%s", fld[1]);
            sw_buttons[sw_nbutton++].effect = sw_effect(fld[2]);
        } else sw_bad(copy);
    }
    fclose(f);
    if (!seen || !sw_ntitle) { printf("FAIL " SWEEP_NAME ": README has no command inventory\n"); exit(2); }
}

/* ---- user actions -------------------------------------------------------- */
static const tbx_menuitem_t *sw_find(const char *title, const char *row, int *id, int *index)
{
    if (!tbx_host.bar) return 0;
    for (unsigned m = 0; m < tbx_host.bar->n_menus; m++) {
        const tbx_menuinfo_t *mi = &tbx_host.bar->menus[m];
        if (strcmp(mi->title, title)) continue;
        for (unsigned r = 0; r < mi->n_items; r++)
            if (!mi->items[r].is_divider && !strcmp(mi->items[r].text, row)) {
                *id = mi->menu_id; *index = (int)r + 1; return &mi->items[r];
            }
    }
    return 0;
}
static int sw_fold(int c) { return (c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c; }
/* The shell's key route (flair/tbxmenu.h, flair/menu.c MenuKey): a keyDown
 * with Control or Command and a character resolves to the first ENABLED
 * non-divider row with that command key, case folded; otherwise the key is
 * the application's. Returns 1 when the shell took it as a menu choice. */
static int sw_shell_key(int mods, int32_t msg)
{
    int c = (int)(msg & 255);
    if ((mods & (FLAIR_EVT_MOD_CONTROL_KEY | FLAIR_EVT_MOD_CMD_KEY)) && c && tbx_host.bar)
        for (unsigned m = 0; m < tbx_host.bar->n_menus; m++) {
            const tbx_menuinfo_t *mi = &tbx_host.bar->menus[m];
            for (unsigned r = 0; r < mi->n_items; r++) {
                const tbx_menuitem_t *it = &mi->items[r];
                if (!it->cmd_char || it->is_divider || !it->enabled || sw_fold(it->cmd_char) != sw_fold(c)) continue;
                sw_event(TBX_EVT_MENU, TBX_MENU_WORD(mi->menu_id, r + 1), 0, 0, 0);
                return 1;
            }
        }
    sw_event(keyDown, msg, mods, 0, 0);
    return 0;
}
static void sw_text(const char *s, char *out, int *n)
{
    *n = 0;
    for (; *s; s++) {
        if (s[0] == '\\' && s[1] == 'r') { out[(*n)++] = '\r'; s++; }
        else out[(*n)++] = *s;
    }
    out[*n] = 0;
}
static int sw_step(char *step)
{
    char *a = strchr(step, ' '); char buf[512]; int n;
    if (a) *a++ = 0; else a = step + strlen(step);
    if (!strcmp(step, "type")) {
        sw_text(a, buf, &n);
        for (int i = 0; i < n; i++) sw_event(keyDown, (unsigned char)buf[i], 0, 0, 0);
    } else if (!strcmp(step, "key")) {
        int mod = 0; char *colon = strchr(a, ':');
        if (colon) { *colon = 0; mod = sw_modbits[sw_mod(a)]; a = colon + 1; }
        int code = (int)strtol(a + 1, 0, 16);
        if (a[0] == 'c') (void)sw_shell_key(mod, code);
        else if (a[0] == 'v') sw_event(keyDown, code << 8, mod, 0, 0);
        else return 0;
    } else if (!strcmp(step, "menu")) {
        char *slash = strchr(a, '/'); int id, index;
        if (!slash) return 0;
        *slash = 0;
        const tbx_menuitem_t *it = sw_find(a, slash + 1, &id, &index);
        if (!it || !it->enabled) return 0;
        sw_event(TBX_EVT_MENU, TBX_MENU_WORD(id, index), 0, 0, 0);
    } else if (!strcmp(step, "scrap")) {
        sw_text(a, buf, &n); memcpy(tbx_host.scrap, buf, (size_t)n); tbx_host.scrap_len = n;
        sw_event(activateEvt, 0, FLAIR_EVT_MOD_ACTIVE_FLAG, 0, 0);
    } else if (!strcmp(step, "scraplines")) {
        int lines = atoi(a), k = 0;
        for (int i = 0; i < lines && k < 8000; i++) k += snprintf((char *)tbx_host.scrap + k, 16, "line %d\r", i + 1);
        tbx_host.scrap_len = k;
        sw_event(activateEvt, 0, FLAIR_EVT_MOD_ACTIVE_FLAG, 0, 0);
    } else if (!strcmp(step, "file")) {
        char *sp = strchr(a, ' ');
        if (!sp) return 0;
        *sp = 0; sw_text(sp + 1, buf, &n);
        if (!tbx_host_put_file(a, buf, (uint32_t)n)) return 0;
    } else if (!strcmp(step, "sfreply")) {
        if (!tbx_host.sf || !tbx_host.sf_reply) return 0;
        snprintf(tbx_host.sf_reply, TBX_SF_REPLY_BYTES, "%s", a);
        sw_event(TBX_EVT_SFREPLY, 1, 0, 0, 0);
    } else if (!strcmp(step, "sfcancel")) {
        if (!tbx_host.sf) return 0;
        sw_event(TBX_EVT_SFREPLY, 0, 0, 0, 0);
    } else if (!strcmp(step, "alert")) {
        if (!tbx_host.alert_pending) return 0;
        sw_event(TBX_EVT_ALERT, atoi(a), 0, 0, 0);
    } else if (!strcmp(step, "button")) {
        char *at = strrchr(a, '@'); unsigned i;
        if (!at) return 0;
        *at = 0; i = (unsigned)atoi(at + 1);
        (void)sw_snap();                /* the dialog's current item list */
        if (!tbx_host.dialog_items || i < 1 || i > tbx_host.dialog_count) return 0;
        const cp_item_t *d = &tbx_host.dialog_items[i - 1];
        if (d->type < 1 || d->type > 3 || strcmp(d->text, a)) return 0;
        tbx_host.dialog_result = (int32_t)i;
        sw_event(mouseDown, 0, 0, (d->left + d->right) / 2, (d->top + d->bottom) / 2);
        tbx_host.dialog_result = 0;
    } else if (!strcmp(step, "click")) {
        int x, y;
        if (sscanf(a, "%d %d", &x, &y) != 2) return 0;
        sw_event(mouseDown, 0, 0, x, y);
    } else if (!strcmp(step, "update")) {
        sw_event(updateEvt, 0, 0, 0, 0);
    } else return 0;
    return 1;
}
static const char *sw_script(const char *ctx)
{
    if (!strcmp(ctx, "base")) return "";
    for (int i = 0; i < sw_nctx; i++) if (!strcmp(sw_ctxs[i].name, ctx)) return sw_ctxs[i].script;
    return 0;
}
/* A fresh launch, then the context's steps. 0 when a step cannot be made. */
static int sw_fresh(const char *ctx)
{
    const char *script = sw_script(ctx); char buf[512];
    if (!script) { SW_CHECK(0, "unknown context '%s'", ctx); return 0; }
    sweep_launch();
    snprintf(buf, sizeof buf, "%s", script);
    for (char *p = buf, *semi; p && *p; p = semi) {
        semi = strchr(p, ';');
        if (semi) *semi++ = 0;
        char *step = sw_trim(p);
        if (*step && !sw_step(step)) { SW_CHECK(0, "context '%s' step '%s' cannot be made", ctx, step); return 0; }
    }
    return 1;
}

/* ---- the checks ------------------------------------------------------------ */
static int sw_ci_contains(const char *s, const char *w)
{
    for (; *s; s++) {
        size_t i = 0;
        while (w[i] && tolower((unsigned char)s[i]) == w[i]) i++;
        if (!w[i]) return 1;
    }
    return 0;
}
static void sw_check_bar(const char *ctx)
{
    const tbx_menubar_t *b = tbx_host.bar; int r = 0;
    SW_CHECK(b && b->n_menus == sw_ntitle, "%s: bar has %d titles, README %d", ctx, b ? b->n_menus : -1, sw_ntitle);
    if (!b) return;
    for (int m = 0; m < sw_ntitle && m < b->n_menus; m++) {
        const tbx_menuinfo_t *mi = &b->menus[m];
        SW_CHECK(!strcmp(mi->title, sw_titles[m].title) && mi->n_items == sw_titles[m].rows,
                 "%s: title %d is '%s' with %d rows, README '%s' with %d", ctx, m + 1, mi->title,
                 mi->n_items, sw_titles[m].title, sw_titles[m].rows);
        for (unsigned k = 0; k < mi->n_items; k++, r++) {
            const tbx_menuitem_t *it = &mi->items[k];
            const sw_row_t *w = r < sw_nrow ? &sw_rows[r] : 0;
            int same = w && !strcmp(w->title, mi->title) && w->divider == (it->is_divider != 0) &&
                       (w->divider || (!strcmp(w->row, it->text) && sw_fold(w->key) == sw_fold(it->cmd_char)));
            SW_CHECK(same, "%s: %s row %u is '%s' key %d, README '%s'", ctx, mi->title, k + 1,
                     it->is_divider ? "-" : it->text, it->cmd_char, w ? w->row : "(none)");
            SW_CHECK(it->is_divider || (!sw_ci_contains(it->text, "implement") &&
                     !sw_ci_contains(it->text, "unavailable")), "%s: row '%s' admits it does nothing", ctx, it->text);
            for (int a = 0; a < sw_nabsent; a++)
                SW_CHECK(it->is_divider || strcmp(it->text, sw_absents[a].row) || strcmp(mi->title, sw_absents[a].title),
                         "%s: frozen omission %s/%s is installed", ctx, sw_absents[a].title, sw_absents[a].row);
        }
    }
    SW_CHECK(r == sw_nrow, "%s: bar has %d rows, README %d", ctx, r, sw_nrow);
}
static void sw_check_rows(void)
{
    char want[96], got[96];
    for (int i = 0; i < sw_nrow; i++) {
        const sw_row_t *w = &sw_rows[i]; int id, index;
        if (w->divider) continue;
        if (!sw_fresh(w->ctx)) continue;
        const tbx_menuitem_t *it = sw_find(w->title, w->row, &id, &index);
        SW_CHECK(it && it->enabled, "%s/%s is grayed or missing in its context '%s'", w->title, w->row, w->ctx);
        if (!it || !it->enabled) continue;
        sw_snap_t a = sw_snap(), b, k;
        sw_event(TBX_EVT_MENU, TBX_MENU_WORD(id, index), 0, 0, 0);
        b = sw_snap();
        int d = sw_diff(&a, &b);
        sw_effects(w->effect, want, sizeof want); sw_effects(d, got, sizeof got);
        SW_CHECK((d & w->effect) == w->effect, "%s/%s in '%s' changed %s, README %s", w->title, w->row, w->ctx, got, want);
        if (!w->key || !sw_fresh(w->ctx)) continue;
        (void)sw_snap();
        SW_CHECK(sw_shell_key(FLAIR_EVT_MOD_CONTROL_KEY, sw_fold(w->key)),
                 "%s/%s: key %c is not resolved to the row", w->title, w->row, w->key);
        k = sw_snap();
        SW_CHECK(!sw_diff(&b, &k), "%s/%s: key %c differs from the menu choice", w->title, w->row, w->key);
    }
}
static int sw_declared(int mod, int vk, int code)
{
    for (int i = 0; i < sw_nkey; i++)
        if (sw_keys[i].mod == mod && sw_keys[i].vk == vk && code >= sw_keys[i].lo && code <= sw_keys[i].hi) return 1;
    return 0;
}
static int sw_button_declared(const char *label, unsigned item)
{
    char want[80]; snprintf(want, sizeof want, "%s@%u", label, item);
    for (int i = 0; i < sw_nbutton; i++) if (!strcmp(sw_buttons[i].label, want)) return 1;
    return 0;
}
/* live[ctx][mod][vk][code] = the effect mask, -1 when resolved by the shell. */
static int sw_live[SW_MAX][4][2][256];
static void sw_sweep_keys(void)
{
    const char *ctxs[SW_MAX]; int nctx = 0, undeclared = 0;
    ctxs[nctx++] = "base";
    for (int i = 0; i < sw_nkey; i++) {
        int dup = 0; for (int j = 0; j < nctx; j++) dup |= !strcmp(ctxs[j], sw_keys[i].ctx);
        if (!dup) ctxs[nctx++] = sw_keys[i].ctx;
    }
    for (int i = 0; i < sw_nbutton; i++) {
        int dup = 0; for (int j = 0; j < nctx; j++) dup |= !strcmp(ctxs[j], sw_buttons[i].ctx);
        if (!dup && nctx < SW_MAX) ctxs[nctx++] = sw_buttons[i].ctx;
    }
    for (int c = 0; c < nctx; c++) {
        if (!sw_fresh(ctxs[c])) continue;
        sw_check_bar(ctxs[c]);
        for (unsigned i = 0; tbx_host.dialog_items && i < tbx_host.dialog_count; i++) {
            const cp_item_t *d = &tbx_host.dialog_items[i];
            if (d->type >= 1 && d->type <= 3 && d->text[0])
                SW_CHECK(sw_button_declared(d->text, i + 1), "%s: dialog item %s@%u has no Button line", ctxs[c], d->text, i + 1);
        }
        sw_snap_t base = sw_snap(); int dirty = 0;
        sw_snap_t again = sw_snap();
        SW_CHECK(!sw_diff(&base, &again), "%s: two updates differ; liveness cannot be judged", ctxs[c]);
        for (int m = 0; m < 4; m++)
            for (int vk = 0; vk < 2; vk++)
                for (int code = 1; code < 256; code++) {
                    if (dirty) { if (!sw_fresh(ctxs[c])) return; base = sw_snap(); dirty = 0; }
                    int32_t msg = vk ? code << 8 : code;
                    int shell = vk ? (sw_event(keyDown, msg, sw_modbits[m], 0, 0), 0) : sw_shell_key(sw_modbits[m], msg);
                    sw_snap_t after = sw_snap();
                    int d = sw_diff(&base, &after);
                    sw_live[c][m][vk][code] = shell ? -1 : d;
                    if (d || shell) dirty = 1;
                    if (d && !shell && !sw_declared(m, vk, code)) {
                        char got[96]; sw_effects(d, got, sizeof got);
                        if (undeclared++ < 24)
                            SW_CHECK(0, "%s: %s %c%02X changes %s but has no Key line (hidden route)",
                                     ctxs[c], sw_modname[m], vk ? 'v' : 'c', code, got);
                        else sw_checks++, sw_fails++;
                    } else sw_checks++;
                }
    }
    for (int i = 0; i < sw_nkey; i++) {
        const sw_key_t *k = &sw_keys[i]; int c = 0;
        while (c < nctx && strcmp(ctxs[c], k->ctx)) c++;
        for (int code = k->lo; code <= k->hi; code++) {
            int d = sw_live[c][k->mod][k->vk][code];
            char want[96], got[96]; sw_effects(k->effect, want, sizeof want); sw_effects(d < 0 ? 0 : d, got, sizeof got);
            SW_CHECK(d >= 0 && (d & k->effect) == k->effect, "%s %c%02X in '%s' changed %s, README %s (dead route)",
                     sw_modname[k->mod], k->vk ? 'v' : 'c', code, k->ctx, d < 0 ? "a menu row" : got, want);
        }
    }
}
static void sw_check_buttons(void)
{
    char want[96], got[96], step[80];
    for (int i = 0; i < sw_nbutton; i++) {
        const sw_button_t *w = &sw_buttons[i];
        if (!sw_fresh(w->ctx)) continue;
        sw_snap_t a = sw_snap();
        snprintf(step, sizeof step, "button %s", w->label);
        SW_CHECK(sw_step(step), "button '%s' is not visible in '%s'", w->label, w->ctx);
        sw_snap_t b = sw_snap();
        int d = sw_diff(&a, &b);
        sw_effects(w->effect, want, sizeof want); sw_effects(d, got, sizeof got);
        SW_CHECK((d & w->effect) == w->effect, "button '%s' in '%s' changed %s, README %s", w->label, w->ctx, got, want);
    }
}
static int command_sweep_main(void)
{
    sw_read_inventory();
    sw_fresh("base");
    sw_check_bar("base");
    for (int i = 0; i < sw_nctx; i++) if (sw_fresh(sw_ctxs[i].name)) sw_check_bar(sw_ctxs[i].name);
    sw_check_rows();
    sw_check_buttons();
    sw_sweep_keys();
    printf("%s: %ld checks, %ld failures\n", SWEEP_NAME, sw_checks, sw_fails);
    return sw_fails != 0;
}
#endif
