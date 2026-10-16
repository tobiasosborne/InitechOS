/* Film-scene arrangement policy: existing kmain fd_win_bounds, PRD 6.5.
 * The windows belong to the real disk tenant; only Window Manager verbs here.
 * Escape dismisses the staged Saving tableau (no file I/O is claimed).
 * Authored interaction policy, not a recovered Mac timing/film dimension. */
#ifndef INITECH_FLAIR_FILM_SCENE_H
#define INITECH_FLAIR_FILM_SCENE_H
#include "window.h"
#include "cursor.h"
#include "event_model.h"
static inline void flair_film_arrange(WindowMgr *wm, WindowPtr ledger, WindowPtr chart)
{
    MoveWindow(wm,ledger,60,80); SizeWindow(wm,ledger,300,220);
    MoveWindow(wm,chart,300,120); SizeWindow(wm,chart,260,240);
}
/* Use the same cooked Escape forms as DialogHandleEvent (dialog.c). */
static inline int flair_film_dismiss(const EventRecord *ev)
{
    return ev->what==keyDown && ((ev->message&255u)==27u ||
                                ((ev->message>>8)&255u)==1u);
}
static inline void flair_film_cursor(int modal_up)
{ flair_cursor_set_busy(modal_up); }
#endif
