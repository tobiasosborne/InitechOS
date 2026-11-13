/*
 * os/flair/tbxmenu.h -- how a disk tenant HEARS its own menu choices
 * (THE ARTIFACT; bead initech-tdnl.31; spec/toolbox_gate.h Sec 6a).
 *
 * The two pure decisions of the menu route, header-only so the SAME text is
 * compiled into the kernel (os/flair/tbxgate.c) and into the host oracle
 * (harness/proptest/test_tbx_menu.c) -- the tbxgate module itself is kernel-
 * only (flat-32 tenant pointers, the asm trampolines), these two are not:
 *
 *   tbx_menu_key_sel  the KEY route: a keyDown carrying Ctrl or Cmd is
 *                     resolved with MenuKey over the tenant's OWN bar (MTE
 *                     Listing 2-6, p. 2-44: DoMenuCommand(MenuKey(key)) when
 *                     the Command key is down and event.what = keyDown). PC
 *                     Ctrl IS Cmd (event.c never sets the Cmd bit on a PS/2
 *                     keyboard; kmain flair_live_finder_key states the same
 *                     rule for the Finder). 0 == not a menu command.
 *   tbx_menu_event    the CONVERGENCE: the mouse route (the shell's
 *                     MenuSelect result over band 2) and the key route both
 *                     become ONE TBX_EVT_MENU EventRecord whose message is the
 *                     MenuSelect/MenuKey LONGINT (MTE p. 3-78: "In either
 *                     case, the application passes the function result ...
 *                     to the DoMenuCommand procedure"). Delivered only when
 *                     the choice was made in the tenant's OWN bar and chose
 *                     something (high word 0 == nothing chosen, MTE p. 3-78).
 *
 * And the flat copy every push-callback delivery uses (spec Sec 6), so the
 * host oracle reads the 24 bytes the tenant's eventProc actually sees.
 *
 * MUTATION KNOBS (Rule 6; -D on the TU that includes this header -- the host
 * oracle's mutants and, through tbxgate.c, the emulator mutant kernels; NEVER
 * in a real build):
 *   TBX_MUT_MENU_KEY_PLAIN  the chord is never resolved: Ctrl-Q reaches the
 *                           tenant as a plain keyDown (the audit-pass-2 G03
 *                           symptom this bead fixes)
 *   TBX_MUT_MENU_ANY_BAR    the bar-identity guard is gone: a choice made in
 *                           ANOTHER app's bar is delivered to the tenant
 *
 * Freestanding (Law 3): <stdint.h> + the menu/event headers only. ASCII-clean
 * (Rule 12). Deterministic (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_TBXMENU_H
#define INITECH_OS_FLAIR_TBXMENU_H

#include <stdint.h>

#include "menu.h"           /* MenuBar, MenuKey, MenuResultID/Item           */
#include "event_model.h"    /* EventRecord, keyDown, FLAIR_EVT_MOD_*         */
#include "toolbox_gate.h"   /* TBX_EVT_MENU, FLAIR_EVBUF_*                   */

_Static_assert(TBX_EVT_MENU > 15u && TBX_EVT_MENU != 23u &&
               TBX_EVT_MENU <= 0xFFFFu,
               "TBX_EVT_MENU must alias no Event Manager code (MTE Table 2-1)");

/* The KEY route. Returns the MenuKey result word, or 0. */
static inline uint32_t tbx_menu_key_sel(const MenuBar *bar,
                                        const EventRecord *ev)
{
    if (bar == (const MenuBar *)0 || ev == (const EventRecord *)0) return 0u;
    if (ev->what != (uint16_t)keyDown) return 0u;   /* not autoKey/keyUp */
#ifndef TBX_MUT_MENU_KEY_PLAIN
    if ((ev->modifiers & (uint16_t)(FLAIR_EVT_MOD_CONTROL_KEY |
                                    FLAIR_EVT_MOD_CMD_KEY)) == 0u)
        return 0u;
    /* event.c cooks message = (vkey << 8) | ascii; MenuKey folds case and
     * hands out only ENABLED, non-divider items (menu.c). */
    return MenuKey(bar, (char)(ev->message & 0xFFu));
#else
    /* MUTANT TBX_MUT_MENU_KEY_PLAIN (Rule 6): no chord is ever a menu
     * command; it falls through to the plain keyDown delivery. NEVER in a
     * real build. */
    return 0u;
#endif
}

/* The CONVERGENCE. `owner` is the tenant's installed bar, `tracked` the bar
 * the choice was made in, `sel` the MenuSelect/MenuKey word, `src` the
 * triggering mouseDown/keyDown (may be NULL). Fills `out` and returns 1 when
 * the choice is the tenant's to hear, else 0 (and `out` is untouched). */
static inline int tbx_menu_event(const MenuBar *owner, const MenuBar *tracked,
                                 uint32_t sel, const EventRecord *src,
                                 EventRecord *out)
{
    if (owner == (const MenuBar *)0 || out == (EventRecord *)0) return 0;
#ifndef TBX_MUT_MENU_ANY_BAR
    if (tracked != owner) return 0;
#else
    /* MUTANT TBX_MUT_MENU_ANY_BAR (Rule 6): the identity guard is gone, so
     * a choice in the Finder's (or any) bar reaches the tenant. NEVER in a
     * real build. */
    (void)tracked;
#endif
    if (MenuResultID(sel) == 0 || MenuResultItem(sel) == 0u) return 0;
    out->what      = (uint16_t)TBX_EVT_MENU;
    out->message   = sel;
    out->when      = (src != (const EventRecord *)0) ? src->when : 0u;
    out->where.h   = (src != (const EventRecord *)0) ? src->where.h : 0;
    out->where.v   = (src != (const EventRecord *)0) ? src->where.v : 0;
    out->modifiers = (src != (const EventRecord *)0) ? src->modifiers : 0u;
    return 1;
}

/* The flat FlairEvent copy (spec Sec 6): six int32 scalars, each EventRecord
 * field widened. `eb` is the tenant's evBufPtr. */
static inline void tbx_evbuf_fill(volatile int32_t *eb, const EventRecord *ev)
{
    eb[FLAIR_EVBUF_WHAT_OFF / 4u]   = (int32_t)ev->what;
    eb[FLAIR_EVBUF_MSG_OFF / 4u]    = (int32_t)ev->message;
    eb[FLAIR_EVBUF_MODS_OFF / 4u]   = (int32_t)ev->modifiers;
    eb[FLAIR_EVBUF_WHEN_OFF / 4u]   = (int32_t)ev->when;
    eb[FLAIR_EVBUF_WHEREH_OFF / 4u] = (int32_t)ev->where.h;
    eb[FLAIR_EVBUF_WHEREV_OFF / 4u] = (int32_t)ev->where.v;
}

#endif /* INITECH_OS_FLAIR_TBXMENU_H */
