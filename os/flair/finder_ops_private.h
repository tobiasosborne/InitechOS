/* Ref: ADR-0014 DEC-14-6.2. Enumeration callback stays resident: the
 * filesystem receives its resident address, never a raw overlay address. */
#ifndef FLAIR_FINDER_OPS_PRIVATE_H
#define FLAIR_FINDER_OPS_PRIVATE_H
#include "finder_ops.h"
typedef struct fo_entry_probe {
    const char *name;
    finder_win_meta_t meta;
    uint16_t cluster;
    int found;
} fo_entry_probe_t;
int fo_entry_cb(const finder_dirent_t *, void *);
#endif
