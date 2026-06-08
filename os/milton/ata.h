/*
 * os/milton/ata.h -- minimal ATA PIO LBA28 sector read backend (MILTON).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): shipped InitechDOS kernel code, freestanding
 * (only <stdint.h>). This is the REAL-HARDWARE sector backend that satisfies
 * the blockdev_t read contract (os/milton/blockdev.h) on the primary ATA
 * channel. It CANNOT be host-unit-tested -- it touches I/O ports 0x1F0..0x1F7
 * -- so its only build-time guarantee here is that it compiles clean under the
 * freestanding flags; functional validation defers to the emulator boot oracle
 * (make test-fs -- the FIRST real ata.c exercise on the emulator).
 *
 * Ref (Law 1): ATA/ATAPI-6 spec, READ SECTORS (0x20) PIO protocol; standard
 *   primary-channel port map 0x1F0..0x1F7 + control 0x3F6; the drive/head
 *   register bit 4 (master/slave) and bit 6 (LBA). PRD Sec 6.1;
 *   docs/research/fs-mount-sft-ground-truth.md Sec 1.3, Sec 2.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 */
#ifndef INITECH_MILTON_ATA_H
#define INITECH_MILTON_ATA_H

#include <stdint.h>

#include "blockdev.h"

/* Standard ATA channel I/O port bases (ATA/ATAPI-6 Table 8). */
#define ATA_IO_BASE_PRIMARY   0x1F0u /* primary channel command-block base   */
#define ATA_IO_BASE_SECONDARY 0x170u /* secondary channel command-block base */

/* Device-control / alternate-status register, one per channel. The 400 ns
 * settle delay reads altstatus here (reading it has no side effects, unlike
 * reading the primary status register which clears a pending IRQ). */
#define ATA_CTRL_PRIMARY      0x3F6u
#define ATA_CTRL_SECONDARY    0x376u

/* Drive/head register selection bytes: bit 6 = LBA, bit 5 = always 1,
 * bit 4 = drive (0 = master, 1 = slave); low nibble carries LBA bits 24..27
 * at command time. (ATA/ATAPI-6; brief Sec 1.3.) */
#define ATA_DRIVE_MASTER_LBA  0xE0u  /* LBA, master (bit 4 = 0) */
#define ATA_DRIVE_SLAVE_LBA   0xF0u  /* LBA, slave  (bit 4 = 1) */

/* Fail-loud error codes (CLAUDE.md Rule 2). All negative; 0 == success. These
 * are returned by ata_read_sectors so a caller (fat12_mount) surfaces the
 * exact failure rather than spinning forever (brief Sec 2.3). */
enum {
	ATA_ERR_ARG       = -1, /* NULL buffer / zero or oversized count / bad lba */
	ATA_ERR_NO_DRIVE  = -2, /* STATUS == 0xFF: floating bus, no device present */
	ATA_ERR_TIMEOUT   = -3, /* BSY never cleared / DRQ never set within bound  */
	ATA_ERR_DEVICE    = -4  /* STATUS reported ERR/DF after the command        */
};

/*
 * Per-device ATA context (brief Sec 1.3). The blockdev_t.ctx points at one of
 * these so ata_read_sectors knows which channel + drive to address. Kernel
 * owns the storage (no heap in the artifact).
 */
typedef struct ata_ctx {
	uint16_t io_base;       /* command-block base: 0x1F0 primary / 0x170 sec.  */
	uint16_t ctrl_base;     /* device-control/altstatus: 0x3F6 / 0x376         */
	uint8_t  drive_select;  /* ATA_DRIVE_MASTER_LBA or ATA_DRIVE_SLAVE_LBA     */
} ata_ctx_t;

/* Read `count` sectors at `lba` (LBA28) into `buf` (>= count*512 bytes) via
 * PIO. `ctx` MUST point at a valid ata_ctx_t (channel + drive). Returns 0 on
 * success, a negative ATA_ERR_* on error (fail loud, never an infinite spin).
 * Signature matches blockdev_t::read_sectors. */
int ata_read_sectors(void *ctx, uint32_t lba, uint32_t count, void *buf);

/*
 * Initialise `dev` as a blockdev_t wrapping the ATA PIO read on the channel +
 * drive described by `ctx`. Both `dev` and `ctx` are caller-owned (kernel BSS);
 * `ctx` must outlive `dev`. The caller fills `ctx` via ata_ctx_init (below) or
 * by hand. write_sectors is left NULL (FAT write is beads initech-509.11).
 */
void ata_blockdev_init(blockdev_t *dev, ata_ctx_t *ctx);

/* Fill `ctx` for the primary master (the boot disk, drive-select 0xE0). */
void ata_ctx_init_primary_master(ata_ctx_t *ctx);

/* Fill `ctx` for the primary slave (the FAT12 data disk, drive-select 0xF0). */
void ata_ctx_init_primary_slave(ata_ctx_t *ctx);

#endif /* INITECH_MILTON_ATA_H */
