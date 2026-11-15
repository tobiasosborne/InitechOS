/* Resident startup binding, not an application filesystem entry point.
 * DOS drive/default selection contract: kernel/include/int21.h. */
#ifndef INITECH_DESKTOP_VOLUME_BINDING_H
#define INITECH_DESKTOP_VOLUME_BINDING_H
#include "fat12.h"
typedef struct finder_volume_binding {
    const fat12_volume_t *volume;
    void *fat;
    uint32_t fat_len;
    unsigned drive;
} finder_volume_binding_t;
int finder_bind_startup_volume(finder_volume_binding_t *out);
#endif
