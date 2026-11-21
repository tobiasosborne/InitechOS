/* Kernel-private startup wiring; no entry for disk applications. */
#ifndef INITECH_STARTUP_DISKS_H
#define INITECH_STARTUP_DISKS_H
#include "fat12.h"
const fat12_volume_t *startup_disks_init(void);
#endif
