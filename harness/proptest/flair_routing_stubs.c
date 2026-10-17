/* flair_routing_stubs.c -- the link seams test-flair-routing does NOT grade.
 *
 * bead initech-2svz (R3.7 follow-up). The routing suite links the REAL
 * os/flair/tbxgate.c (the disk-tenant host: disk_tenant_event, the INT 81h
 * dispatcher, the slot lifecycle) over the REAL Layer-5 spine (process.c,
 * window.c, heap.c, the region engine) and the REAL DOS PSP/SFT layer the
 * teardown reaches (psp.c, sft.c). tbxgate.c also references the drawing,
 * dialog, scrap, control-panel and file-copy surfaces; the routing suite never
 * drives those verbs, so each is a FAIL-LOUD stub here (Rule 2): reaching one
 * means the suite strayed outside its scope, and it aborts rather than
 * returning a plausible value. Each stub includes the artifact header that
 * declares it, so a drifted prototype is a compile error, not a silent ABI
 * mismatch.
 *
 * Factory code (Law 3): hosted C, never linked into the artifact. ASCII-only.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "blitter.h"
#include "control.h"
#include "desktop.h"
#include "dialog.h"
#include "event.h"
#include "scrap.h"
#include "menu.h"
#include "text.h"
#include "int21.h"

static void unreached(const char *what)
{
    fprintf(stderr, "  FAIL test-flair-routing: stub %s reached -- the routing "
                    "suite strayed outside its scope (Rule 2)\n", what);
    abort();
}

/* The control-panel live knobs (event.c owns them on the kernel). */
unsigned flair_mouse_tracking = 1u;
unsigned flair_double_click_ticks = 40u;

void blitter_fill_rect_clipped(const bitmap_t *dst, rgn_rect_t rect,
                               uint32_t px, const region_t *clip)
{ (void)dst; (void)rect; (void)px; (void)clip; unreached("blitter_fill_rect_clipped"); }

void control_init(ControlRecord *ctrl, flair_ctrl_type_t type, rgn_rect_t rect,
                  int16_t value, int16_t min_val, int16_t max_val, int16_t vis,
                  const char *title)
{
    (void)ctrl; (void)type; (void)rect; (void)value; (void)min_val;
    (void)max_val; (void)vis; (void)title;
    unreached("control_init");
}

void desktop_set_pattern(unsigned pattern) { (void)pattern; unreached("desktop_set_pattern"); }

int DialogHandleEvent(DialogPtr dp, EventRecord *ev, dialog_filter_fn filterProc,
                      uint16_t *itemHit)
{ (void)dp; (void)ev; (void)filterProc; (void)itemHit; unreached("DialogHandleEvent"); return 0; }

void DrawDialog(DialogPtr dp) { (void)dp; unreached("DrawDialog"); }

uint32_t flair_tick_count(void) { unreached("flair_tick_count"); return 0u; }

int32_t FlairGetScrap(const FlairScrap *s, flair_ostype_t type, void *buf,
                      uint32_t bufsz, uint32_t *len_out)
{ (void)s; (void)type; (void)buf; (void)bufsz; (void)len_out; unreached("FlairGetScrap"); return 0; }
int32_t FlairInfoScrap(const FlairScrap *s) { (void)s; unreached("FlairInfoScrap"); return 0; }
int32_t FlairPutScrap(FlairScrap *s, flair_ostype_t type, const void *data, uint32_t len)
{ (void)s; (void)type; (void)data; (void)len; unreached("FlairPutScrap"); return 0; }
void FlairZeroScrap(FlairScrap *s) { (void)s; unreached("FlairZeroScrap"); }

void MenuClock_enable(int enabled) { (void)enabled; unreached("MenuClock_enable"); }
uint32_t MenuKey(const MenuBar *bar, char ch) { (void)bar; (void)ch; unreached("MenuKey"); return 0u; }

text_font_t text_font_from_txfont(int txfont) { (void)txfont; unreached("text_font_from_txfont"); return FONT_CHICAGO; }

void int21_cwd_reset(void) { unreached("int21_cwd_reset"); }
int21_cwd_snapshot_t int21_cwd_save(void)
{ int21_cwd_snapshot_t s = { 0 }; unreached("int21_cwd_save"); return s; }
void int21_cwd_restore(const int21_cwd_snapshot_t *s) { (void)s; unreached("int21_cwd_restore"); }
void int21_dispatch(int_frame_t *frame) { (void)frame; unreached("int21_dispatch"); }
struct psp *int21_get_psp(void) { unreached("int21_get_psp"); return NULL; }
void int21_set_psp(struct psp *psp) { (void)psp; unreached("int21_set_psp"); }
int int21_mcb_usage(uint32_t *total, uint32_t *free_, uint32_t *largest)
{ (void)total; (void)free_; (void)largest; unreached("int21_mcb_usage"); return 0; }
int int21_path_is_device(const char *path) { (void)path; unreached("int21_path_is_device"); return 0; }
