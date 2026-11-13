/* System-owned text-tenant launcher, shared by the pump and routing oracle.
 * Ref: ADR-0013 Sec 3.2/7 O-7; kmain.c DEC-LAUNCH-1. */
#ifndef INITECH_FLAIR_SYSTEM_KEY_H
#define INITECH_FLAIR_SYSTEM_KEY_H
#include "event.h"

#define FLAIR_TEXT_TENANT_HOTKEY_VKEY 0x2Bu

static inline int flair_text_tenant_hotkey(const EventRecord *ev)
{
    return ev->what == (uint16_t)keyDown &&
           ((ev->message >> 8) & 0xFFu) == FLAIR_TEXT_TENANT_HOTKEY_VKEY
#ifndef FLAIR_MUT_PLAIN_BSL_LAUNCH
           /* event_model.h Sec 2; event.c folds either physical side. */
           && (ev->modifiers & (FLAIR_EVT_MOD_CONTROL_KEY |
                               FLAIR_EVT_MOD_OPTION_KEY)) ==
                              (FLAIR_EVT_MOD_CONTROL_KEY |
                               FLAIR_EVT_MOD_OPTION_KEY)
#endif
           ;
}

/* ADR-0013 O-7; event_model.h Sec 2/5: consume text-tenant typeahead
 * without delivery, but cook modifier releases. Dropping raw bytes would
 * leave the launching Ctrl/Alt latched after QUIT. Preserve ticks/cursor
 * and lock state; never reinitialize the Event Manager on resume. */
static inline void flair_text_tenant_drain(flair_raw_ring_t *ring)
{
    uint32_t end = ring->head;
    EventRecord ignored;
    while (ring->tail != end) (void)GetNextEvent(ring, 0u, &ignored);
}
#endif
