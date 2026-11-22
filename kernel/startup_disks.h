/* Kernel-private startup wiring; no entry for disk applications. */
#ifndef INITECH_STARTUP_DISKS_H
#define INITECH_STARTUP_DISKS_H
#include "fat12.h"
const fat12_volume_t *startup_disks_init(void);
/* Disk errors fail silently until the keyboard is live (SYSINIT's INT 24h);
 * call after the console input source is bound. */
void startup_disks_enable_prompts(void);
#endif
