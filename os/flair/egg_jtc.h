/* Ref: PRD 6.3/6.4; authored spec/easter/jump_to_conclusions.json. */
#ifndef FLAIR_EGG_JTC_H
#define FLAIR_EGG_JTC_H
#include "process.h"
#include "egg_jtc_model.h"
/* Resident vtable (kmain.c): stub addresses only, never overlay addresses
 * (ADR-0014 DEC-14-6.2). */
extern const FlairAppProcs egg_jtc_procs;
/* The overlaid entry points (os/flair/egg_jtc.c, section COLD of FLAIR.OVL).
 * The resident kernel resolves these names to its GENERATED stubs. */
int egg_jtc_open(FlairApp *self, const FlairLaunchParams *lp);
void egg_jtc_event(FlairApp *self, const EventRecord *e);
#endif
