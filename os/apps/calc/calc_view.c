/*
 * os/apps/calc/calc_view.c -- the Calculator's face (THE ARTIFACT; bead
 * initech-tdnl.17). See calc_view.h. The geometry is the locked
 * spec/calc/calc_layout.h; the glyphs the locked spec/assets/calc_geneva9.h.
 * ASCII-clean (Rule 12).
 */
#include "calc_view.h"
#include "calc/calc_layout.h"
#include "calc_geneva9.h"

static const calc_key_rect_t g_keys[CALC_KEY_COUNT] = CALC_KEY_TABLE;
static const calc_glyph_t    g_g9[CALC_G9_COUNT] = CALC_G9_TABLE;

static const calc_glyph_t *glyph(char ch)
{
    for (int i = 0; i < CALC_G9_COUNT; i++)
        if (g_g9[i].ch == ch) return &g_g9[i];
    return &g_g9[0];          /* ' ' sorts first: an unknown char is blank */
}

/* One glyph at pen (x, cap top y): each row's ink as horizontal runs. */
static void draw_glyph(const calc_glyph_t *g, int x, int y, int tok,
                       calc_rect_fn fn, void *user)
{
    for (int r = 0; r < (int)g->nrows; r++) {
        unsigned bits = g->rows[r];
        int c = 0;
        while (c < 8) {
            if (bits & (0x80u >> c)) {
                int s = c;
                while (c < 8 && (bits & (0x80u >> c))) c++;
                fn(user, x + s, y + r, x + c, y + r + 1, tok);
            } else {
                c++;
            }
        }
    }
}

int calc_view_text_left(const char *s)
{
    int w = 0;
    for (int i = 0; s[i]; i++) w += glyph(s[i])->adv;
#if defined(CALC_MUT_READOUT_SHIFT)
    return CALC_DISP_TEXT_RIGHT - w - 1;
#else
    return CALC_DISP_TEXT_RIGHT - w;
#endif
}

void calc_view_paint_readout(const char *readout, calc_rect_fn fn, void *user)
{
    fn(user, CALC_DISP_L + 1, CALC_DISP_T + 1, CALC_DISP_R, CALC_DISP_B,
       CALC_TOK_WHITE);
    int x = calc_view_text_left(readout);
    for (int i = 0; readout[i]; i++) {
        const calc_glyph_t *g = glyph(readout[i]);
        draw_glyph(g, x, CALC_DISP_TEXT_TOP, CALC_TOK_BLACK, fn, user);
        x += g->adv;
    }
}

void calc_view_key_rect(int idx, int *l, int *t, int *r, int *b)
{
    const calc_key_rect_t *k = &g_keys[idx];
    *l = k->l; *t = k->t; *r = k->l + k->w; *b = k->t + k->h;
}

void calc_view_paint_key(int idx, int pressed, calc_rect_fn fn, void *user)
{
    const calc_key_rect_t *k = &g_keys[idx];
    int l = k->l, t = k->t, r = k->l + k->w, b = k->t + k->h;
    int s = CALC_KEY_SHADOW;

    /* the shadow: the frame rect offset (+2,+2), only its exposed L */
    fn(user, r, t + s, r + s, b + s, CALC_TOK_BLACK);
    fn(user, l + s, b, r, b + s, CALC_TOK_BLACK);
    /* the frame, then the face (pressed: the face stays black) */
    fn(user, l, t, r, b, CALC_TOK_BLACK);
    if (!pressed) fn(user, l + 1, t + 1, r - 1, b - 1, CALC_TOK_WHITE);
    draw_glyph(glyph(k->label), l + k->ldx, t + k->ldy,
               pressed ? CALC_TOK_WHITE : CALC_TOK_BLACK, fn, user);
}

void calc_view_paint(const char *readout, calc_rect_fn fn, void *user)
{
    fn(user, 0, 0, CALC_CONTENT_W, CALC_CONTENT_H, CALC_TOK_GRAY);
    fn(user, CALC_DISP_L, CALC_DISP_T, CALC_DISP_R + 1, CALC_DISP_B + 1,
       CALC_TOK_BLACK);
    calc_view_paint_readout(readout, fn, user);
    for (int i = 0; i < CALC_KEY_COUNT; i++)
        calc_view_paint_key(i, 0, fn, user);
}

int calc_view_hit(int x, int y)
{
    for (int i = 0; i < CALC_KEY_COUNT; i++) {
        const calc_key_rect_t *k = &g_keys[i];
        if (x >= k->l && x < k->l + k->w && y >= k->t && y < k->t + k->h)
            return i;
    }
    return -1;
}

calc_cmd_t calc_view_click_cmd(int idx)
{
    if (idx < 0 || idx >= CALC_KEY_COUNT) return CALC_NONE;
#if defined(CALC_MUT_KEY_CLICK_DIVERGE)
    if (g_keys[idx].cmd == CALC_SUB) return CALC_ADD;   /* MUTANT, never real */
#endif
    return (calc_cmd_t)g_keys[idx].cmd;
}

int calc_view_key_of_cmd(calc_cmd_t k)
{
    for (int i = 0; i < CALC_KEY_COUNT; i++)
        if (g_keys[i].cmd == (signed char)k) return i;
    return -1;
}
