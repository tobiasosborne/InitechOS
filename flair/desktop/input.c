/* Extracted production input; reference: legacy-final kmain.c. */
#include "desktop_internal.h"

void flair_consumed_key(uint8_t sc)
{
    (void)kbd_translate(&g_flair_consumed_keys, sc);
}

volatile uint32_t g_flair_mouse_activity;

int g_flair_cursor_visible;

int g_flair_cursor_x;

int g_flair_cursor_y;

__attribute__((unused))
void flair_live_kbd_post(uint8_t sc)
{
    flair_raw_event_t raw;
    raw.kind = (uint32_t)FLAIR_RAW_KEYBOARD;
    raw.tick = flair_tick_count();
    raw.payload = (uint32_t)sc;
    (void)flair_raw_post(&g_flair_kbd_ring, &raw);
}

__attribute__((unused))
void flair_live_mouse_post(int dx, int dy, uint8_t buttons)
{
    flair_raw_event_t raw;
    raw.kind = (uint32_t)FLAIR_RAW_MOUSE;
    raw.tick = flair_tick_count();
    raw.payload = mouse_pack_raw_payload(dx, dy, buttons);
    if (flair_raw_post(&g_flair_kbd_ring, &raw)) {
        g_flair_mouse_activity++;
    }
}

void flair_live_yield(void)
{
    __asm__ __volatile__("hlt");
}

void flair_live_emit_evt(const EventRecord *ev)
{
    serial_puts("FLAIR-EVT what=");
    serial_putu((uint32_t)ev->what);
    serial_puts(" where=");
    serial_puti((int32_t)ev->where.h);
    serial_putc(',');
    serial_puti((int32_t)ev->where.v);
    serial_puts(" msg=");
    serial_puthex32(ev->message & 0xFFFFu);
    serial_putc('\n');
}

void flair_live_cursor_track(const EventRecord *ev)
{
    int x;
    int y;
    if (g_flair_mouse_activity == 0u) return;
    x = (int)ev->where.h;
    y = (int)ev->where.v;
    if (!g_flair_cursor_visible) {
        flair_cursor_show(x, y);
        g_flair_cursor_visible = 1;
    } else {
        if (x == g_flair_cursor_x && y == g_flair_cursor_y) return;
        flair_cursor_move(x, y);
    }
    g_flair_cursor_x = x;
    g_flair_cursor_y = y;
    serial_puts("FLAIR-CURSOR x=");
    serial_puti((int32_t)x);
    serial_puts(" y=");
    serial_puti((int32_t)y);
    serial_putc('\n');
}
