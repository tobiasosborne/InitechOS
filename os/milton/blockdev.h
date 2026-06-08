/*
 * os/milton/blockdev.h -- sector-granular block-device interface (MILTON).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): this is shipped InitechDOS kernel code.
 * Freestanding-safe -- depends ONLY on <stdint.h>. No libc, no malloc; the
 * caller owns every buffer. This is the single seam between the FAT12 reader
 * (os/milton/fat12.c) and a concrete sector backend (ATA PIO on real hardware
 * via os/milton/ata.c; a host file in the factory oracle via
 * harness/diff/fat_diff/blockdev_file.c).
 *
 * Ref (Law 1): PRD Sec 6.1 (InitechDOS FAT/disk path); ADR-0003 DEC-07
 *   (Sec 5.7, on-volume layout); docs/research/fat12-ground-truth.md.
 *
 * Contract:
 *   - The unit of transfer is one sector. Sector size equals the volume's
 *     bytes_per_sector (512 for the 1.44 MB FAT12 floppy this slice targets).
 *   - read_sectors(ctx, lba, count, buf): read `count` consecutive sectors
 *     starting at logical block address `lba` into `buf`. `buf` MUST hold at
 *     least count * 512 bytes. Returns 0 on success, a negative value on
 *     error (fail loud, CLAUDE.md Rule 2 -- never report a short/garbage read
 *     as success). `ctx` is the backend's opaque handle.
 *   - write_sectors is reserved for a LATER issue (beads initech-509.11, FAT
 *     write). It is NULL for now and MUST NOT be called by this slice.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 */
#ifndef INITECH_MILTON_BLOCKDEV_H
#define INITECH_MILTON_BLOCKDEV_H

#include <stdint.h>

/* Sector size assumed by this read path (the volume's bytes_per_sector).
 * The FAT12 mount validates the on-disk BPB equals this; see fat12.c. */
#define BLOCKDEV_SECTOR_SIZE 512u

typedef struct blockdev {
	/* Opaque backend handle, passed back to the function pointers. */
	void *ctx;

	/* Read `count` sectors at `lba` into `buf` (>= count*512 bytes).
	 * Returns 0 on success, negative on error. Never NULL for a usable dev. */
	int (*read_sectors)(void *ctx, uint32_t lba, uint32_t count, void *buf);

	/* Reserved for FAT write (beads initech-509.11). NULL for this slice;
	 * DO NOT implement or call write in the read path. */
	int (*write_sectors)(void *ctx, uint32_t lba, uint32_t count, const void *buf);
} blockdev_t;

#endif /* INITECH_MILTON_BLOCKDEV_H */
