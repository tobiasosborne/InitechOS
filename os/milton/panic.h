/* Terminal non-CPU diagnostics: physical display, no heap or console state.
 * Caller preserves its serial context and halts after the paint completes. */
#ifndef INITECH_PANIC_H
#define INITECH_PANIC_H
#include "panic_screen.h"
void panic_show_reason(panic_reason_t reason, const char *subject);
#endif
