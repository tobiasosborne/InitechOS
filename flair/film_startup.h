/* Audit DAS L001/B08; CLAUDE Rule 2. Required media/asset refusal remains
 * terminal, but a mount/read/format failure must never claim heap exhaustion.
 * tbx_launch maps only LOADER_ERR_NOMEM to TBX_ERR_NOMEM; other loader details
 * remain in TENANT-LOAD-FAIL's serial rc (tbxgate.c). */
#ifndef INITECH_FILM_STARTUP_H
#define INITECH_FILM_STARTUP_H
#include "../milton/panic_screen.h"
#include "../milton/fat12.h"
#include "../../spec/toolbox_gate.h"
static inline panic_reason_t film_startup_reason(int volume, int apps, int load)
{
#ifdef PANIC_MUTATE_STARTUP_OOM
    if (volume!=FAT12_OK || apps!=FAT12_OK || load!=TBX_OK)
        return PANIC_REASON_HEAP;
#endif
    if (volume!=FAT12_OK) return PANIC_REASON_DATA_VOLUME;
    if (apps!=FAT12_OK) return PANIC_REASON_APPS;
    if (load==TBX_ERR_NOMEM) return PANIC_REASON_HEAP;
    if (load!=TBX_OK) return PANIC_REASON_CALC_LOAD;
    return PANIC_REASON_NONE;
}
#endif
