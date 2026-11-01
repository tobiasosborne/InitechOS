/* Ref: ADR-0014 DEC-14-8. Shared resident model services for cold operations.
 * The enumeration callback and traversal model remain resident. */
#ifndef FLAIR_FINDER_WINDOWS_PRIVATE_H
#define FLAIR_FINDER_WINDOWS_PRIVATE_H
#include "finder_windows.h"
typedef struct fw_item {
    int      want, seen;
    char     name[FINDER_DESK_NAME_MAX];
    uint8_t  attr;
    uint32_t size;
    uint16_t cl;
} fw_item_t;
void fw_zero(void *dst, uint32_t n);
int fw_ieq(const char *a, const char *b);
void fw_copy83(char *dst, const char *src);
int fw_slot_ok(const finder_shell_t *sh, int slot);
int fw_item(finder_shell_t *sh, uint16_t dir, int n, fw_item_t *it);
int fw_trash_ok(const finder_shell_t *sh);
void fw_refresh_group_head(finder_shell_t *sh);
#endif
