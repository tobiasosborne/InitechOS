/* CHKDSK.COM contract. Ref: PRD 6.1; ADR-0003 App A/B;
 * MS-DOS 3.3 User's Guide p43, pp318-319,334,344 (PDF 128,403-404,419,429).
 * IBM DOS Reference pp7-48..7-51,A-46 (PDF 153..156,459).
 * Factory drives the SAME freestanding engine through a file block device.
 */
#ifndef INITECH_CHKDSK_H
#define INITECH_CHKDSK_H
#include <stdint.h>
#include "blockdev.h"
typedef struct chkdsk_io {
    blockdev_t disk;
    void *ctx;
    void (*put)(void *, const char *);
    int (*get)(void *);
    /* Memory summary in DATA bytes (total excludes MCB headers), from the
     * OEM INT 21h AH=F9h service in the product (initech-q9p80; contract in
     * kernel/README.md). 0 on success, nonzero when no summary exists. */
    int (*memory)(uint32_t *total_bytes, uint32_t *free_bytes);
    uint8_t drive;
    /* GETCWD of a zero-based drive (dospath.h contract; q45q). */
    int (*cwd)(unsigned drive, char out[64]);
} chkdsk_io_t;
int chkdsk_run(chkdsk_io_t *io, const char *tail);
#endif
