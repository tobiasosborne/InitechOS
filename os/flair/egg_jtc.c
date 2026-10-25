/* Ref: PRD 6.3/6.4; spec/easter/jump_to_conclusions.json (authored);
 * os/apps/ref_tenant.c (AC-2 records and updateEvt clipping);
 * spec/window_record.h noGrowDocProc (standard Platinum, no grow box).
 * Modeless document; the shell supplies the modal alert.
 * No application file, menu row, command or manual entry advertises it.
 *
 * OVERLAID (ADR-0014 DEC-14-10 lane 1, epic initech-x8gsi): this file is the
 * JTC mat's part of section COLD of C:\FLAIR.OVL, linked at its overlay-window address against
 * the running kernel (os/milton/overlay.ld). It is NOT in the resident kernel.
 * Its two entry points are reached only through the generated resident stubs
 * named egg_jtc_open / egg_jtc_event (DEC-14-6.1/.2: egg_jtc_procs holds stub
 * addresses); it calls the kernel directly through checked imports (6.3) and
 * installs no IRQ hook (6.5). The per-event pad poll and the pending pad stay
 * resident in egg_jtc_model.c, so the hot path never touches this section. */
#include "egg_jtc.h"
#include "window.h"
#include "blitter.h"
#include "flair_look.h"
#include "text.h"

enum { ROWS = 32, POOL = 128 };
typedef struct jtc_region {
    region_t r;
    rgn_row_t rows[ROWS];
    int16_t pool[POOL];
} jtc_region;
typedef struct jtc_records {
    WindowRecord w;
    jtc_region r[4];
} jtc_records;
typedef struct jtc_state {
    jtc_records *rec;
    const bitmap_t *dst;
} jtc_state;

static void box(const bitmap_t *dst, const region_t *clip, rgn_rect_t r, int part)
{
    blitter_fill_rect_clipped(dst,r,flair_look_pixel_depth(dst->bpp,part),clip);
}
static void label(const bitmap_t *dst, const region_t *clip, rgn_rect_t r,
                  const char *s, uint32_t ink, uint32_t face)
{
    /* Chicago 12 through the existing clipped text_draw seam. Longer labels
     * wrap on a space into two centred cells, never into the next pad. */
    char a[JTC_CELL], b[JTC_CELL]; int n = 0, cut = 0;
    while (s[n]) { a[n] = s[n]; n++; } a[n] = 0;
    if (text_measure(FONT_CHICAGO,s) > r.right-r.left-8)
        for (int i = 0; i < n; i++) if (s[i] == ' ' && i <= n/2) cut = i;
    b[0] = 0;
    if (cut) {
        a[cut] = 0;
        for (int i = cut+1; i <= n; i++) b[i-cut-1] = s[i];
    }
    text_clip_t tc = {clip,0,r.left,r.top,r.right,r.bottom};
    int y = (r.top+r.bottom)/2 - (cut ? 16 : 8);
    text_draw_run(dst,r.left+text_center_in(r.right-r.left,a,FONT_CHICAGO),y,
                  a,FONT_CHICAGO,0,ink,face,&tc);
    if (cut) text_draw_run(dst,r.left+text_center_in(r.right-r.left,b,FONT_CHICAGO),y+16,
                           b,FONT_CHICAGO,0,ink,face,&tc);
}
static void paint(jtc_state *p, const region_t *clip)
{
    const bitmap_t *dst = p->dst;
    rgn_rect_t c = region_get_bbox(p->rec->w.contRgn), h = c;
    uint32_t ink = flair_look_pixel_depth(dst->bpp,FLAIR_PART_FRAME);
    uint32_t face = flair_look_pixel_depth(dst->bpp,FLAIR_PART_PLAT_FACE);
    box(dst,clip,c,FLAIR_PART_PLAT_TROUGH);
    h.bottom = (int16_t)(h.top+egg_jtc_value(JTC_HEADING_H)+12);
    label(dst,clip,h,egg_jtc_heading(),ink,
          flair_look_pixel_depth(dst->bpp,FLAIR_PART_PLAT_TROUGH));
    for (int i = 0; i < egg_jtc_count(); i++) {
        rgn_rect_t r = egg_jtc_pad(c,i);
        box(dst,clip,r,FLAIR_PART_FRAME);
        r.left++; r.top++; r.right--; r.bottom--;
        box(dst,clip,r,FLAIR_PART_CONTENT);
        r.left++; r.top++; r.right--; r.bottom--;
        box(dst,clip,r,FLAIR_PART_PLAT_FACE);
#ifndef EGG_MUT_OVL_ONE_LABEL  /* MUTANT overlay only (DEC-14-9(a), Rule 6) */
        label(dst,clip,r,egg_jtc_label(i),ink,face);
#else
        label(dst,clip,r,egg_jtc_label(0),ink,face);   /* every pad says pad 0 */
#endif
    }
}
#ifdef EGG_MUT_OVL_LEAK
/* MUTANT overlay only (DEC-14-6.2, review reproducer B; Rule 6): a GLOBAL
 * overlay function whose address is stored where resident code could keep it.
 * test-ovl-context-static-mutant must go RED on the object's relocation. */
extern void *egg_jtc_leak;
void egg_jtc_inner(void);
void egg_jtc_inner(void) { }
#endif
int egg_jtc_open(FlairApp *self, const FlairLaunchParams *lp)
{
#ifdef EGG_MUT_OVL_LEAK
    egg_jtc_leak = (void *)egg_jtc_inner;
#endif
#ifdef EGG_MUT_OVL_UD2
    /* MUTANT overlay only (DEC-14-9(c), DEC-14-6.6): a CPU exception with
     * EIP inside the overlay window must panic naming the section. */
    __asm__ __volatile__("ud2");
#endif
    jtc_state *p = flair_alloc(&self->arena,FLAIR_CLASS_GENERAL,sizeof *p);
    jtc_records *r = flair_alloc(&self->records_arena,FLAIR_CLASS_HANDLE,sizeof *r);
    if (!p || !r) return 1;
    for (unsigned i = 0; i < sizeof *r; i++) ((uint8_t *)r)[i] = 0;
    for (int i = 0; i < 4; i++) {
        r->r[i].r.rows = r->r[i].rows; r->r[i].r.cap_rows = ROWS;
        r->r[i].r.x_pool = r->r[i].pool; r->r[i].r.x_pool_cap = POOL;
        region_set_empty(&r->r[i].r);
    }
    r->w.strucRgn = &r->r[0].r; r->w.contRgn = &r->r[1].r; r->w.updateRgn = &r->r[2].r;
    rgn_rect_t f = {(int16_t)egg_jtc_value(JTC_Y),(int16_t)egg_jtc_value(JTC_X),
        (int16_t)(egg_jtc_value(JTC_Y)+egg_jtc_value(JTC_H)),
        (int16_t)(egg_jtc_value(JTC_X)+egg_jtc_value(JTC_W))};
    NewWindow(lp->wm,&r->w,f,CalcDocContentRect(f),documentKind,noGrowDocProc,1);
    SetWTitle(lp->wm,&r->w,egg_jtc_title());
    r->w.refCon = (int32_t)(uintptr_t)self;
    self->windows = &r->w; self->userData = p;
    p->rec = r; p->dst = lp->surface;
    return 0;
}
void egg_jtc_event(FlairApp *self, const EventRecord *e)
{
    jtc_state *p = self->userData;
    if (e->what == updateEvt) {
        region_t *clip = &p->rec->r[3].r;
        region_op(clip,p->rec->w.contRgn,p->rec->w.updateRgn,RGN_OP_INTERSECT);
        paint(p,clip);
    } else if (e->what == mouseDown) {
        egg_jtc_pending = egg_jtc_hit(region_get_bbox(p->rec->w.contRgn),e->where.h,e->where.v)+1;
    }
}
