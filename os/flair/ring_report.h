/* owpg: bounded ring-loss diagnostics, called ONLY by task-context consumers.
 * Reuses kmain's bounded COM1 writer; no logging or port I/O in an ISR.
 * Ref: ADR-0004 D-4; Rule 2. The host sink lets the oracle capture exact bytes. */
#ifndef INITECH_RING_REPORT_H
#define INITECH_RING_REPORT_H
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__
#include <stdio.h>
#define RING_REPORT(s) ((void)fputs((s), stderr))
#else
extern void serial_puts(const char *s);
#define RING_REPORT(s) serial_puts(s)
#endif
#endif
