/* GM3 cancellation uses the same ASCII-or-scan Escape as DialogHandleEvent.
 * Command-period also accepts the physical Control modifier used by the PC
 * keyboard and the desktop's command shortcuts. Event Manager Table 2-2. */
#ifndef FLAIR_FILECOPY_CANCEL_H
#define FLAIR_FILECOPY_CANCEL_H
#include "event_model.h"
static inline int flair_filecopy_cancel(uint16_t what,uint32_t message,uint16_t modifiers)
{
    return (what==keyDown || what==autoKey) &&
        ((message&0xffu)==27u || ((message>>8)&0xffu)==1u ||
         ((message&0xffu)=='.' &&
          (modifiers&(FLAIR_EVT_MOD_CMD_KEY|FLAIR_EVT_MOD_CONTROL_KEY))));
}
#endif
