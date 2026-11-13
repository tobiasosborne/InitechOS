/* Terminal non-CPU diagnostics: physical display, no heap or console state.
 * Caller preserves its serial context and halts after the paint completes. */
#ifndef INITECH_PANIC_H
#define INITECH_PANIC_H
#include "panic_screen.h"
void panic_show_reason(panic_reason_t reason, const char *subject);
/* Nonzero once a panic path has begun (a CPU exception reached the dispatcher,
 * or a typed reason is being painted). ADR-0014 DEC-14-6.4: the overlay
 * manager refuses to load from the panic path (CONTEXT). */
extern volatile uint8_t g_panic_active;
#endif
