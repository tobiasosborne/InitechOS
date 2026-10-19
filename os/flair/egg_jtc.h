/* Ref: PRD 6.3/6.4; authored spec/easter/jump_to_conclusions.json. */
#ifndef FLAIR_EGG_JTC_H
#define FLAIR_EGG_JTC_H
#include "process.h"
#include "egg_jtc_model.h"
extern const FlairAppProcs egg_jtc_procs;
/* The shell owns modal alerts; take one pending pad, or -1. */
int egg_jtc_take_pad(void);
#endif
