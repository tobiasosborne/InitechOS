/* CHKDSK.COM contract. Ref: PRD 6.1; ADR-0003 App A/B;
 * MS-DOS 3.3 User's Guide p43, pp318-319,334,344 (PDF 128,403-404,419,429).
 * IBM DOS Reference pp7-48..7-51,A-46 (PDF 153..156,459).
 * Factory drives the SAME freestanding engine through a file block device.
 */
#ifndef INITECH_CHKDSK_H
#define INITECH_CHKDSK_H
#include <stdint.h>
#include "blockdev.h"
#include "mcb.h"
typedef struct chkdsk_io {
    blockdev_t disk;
    void *ctx;
    void (*put)(void *, const char *);
    int (*get)(void *);
    mcb_arena_t arena;
    uint8_t drive;
    /* GETCWD of a zero-based drive (dospath.h contract; q45q). */
    int (*cwd)(unsigned drive, char out[64]);
} chkdsk_io_t;
int chkdsk_run(chkdsk_io_t *io, const char *tail);
#endif
