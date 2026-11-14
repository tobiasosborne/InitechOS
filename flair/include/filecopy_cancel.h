/* GM3 cancellation on the live modal path. Key conventions: Inside Macintosh,
 * Toolbox Essentials, Event Manager Table 2-2 (Command modifier); Escape and
 * Escape follows dialog.c's existing modal policy. Command-period is the
 * authored GM3 shortcut using that captured Command modifier. */
#ifndef FLAIR_FILECOPY_CANCEL_H
#define FLAIR_FILECOPY_CANCEL_H
#include "event_model.h"
static inline int flair_filecopy_cancel(uint16_t what,uint32_t message,uint16_t modifiers)
{
    return (what==keyDown || what==autoKey) &&
        ((message&0xffu)==27u || ((message&0xffu)=='.' && (modifiers&FLAIR_EVT_MOD_CMD_KEY)));
}
#endif
