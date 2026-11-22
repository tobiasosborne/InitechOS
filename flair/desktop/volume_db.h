/* Per-volume projection of the Finder's runtime identities onto the locked
 * DESKTOP.DB bytes. MTE Finder Interface: volume + directory identity. */
#ifndef INITECH_FINDER_VOLUME_DB_H
#define INITECH_FINDER_VOLUME_DB_H
#include "finder_windows.h"
int finder_volume_db_has_state(const finder_shell_t *sh,unsigned drive);
uint32_t finder_volume_db_encode(const finder_shell_t *sh,unsigned drive,
                                 uint8_t *out,uint32_t cap);
unsigned finder_volume_db_load(finder_shell_t *sh,unsigned drive,
                               const uint8_t *data,uint32_t len);
void finder_volume_db_positions(finder_shell_t *sh,unsigned drive,
                                const uint8_t *data,uint32_t len);
#endif
