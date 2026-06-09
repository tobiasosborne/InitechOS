/*
 * os/milton/ata.c -- minimal ATA PIO LBA28 sector read (MILTON).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): shipped InitechDOS kernel code. Freestanding
 * (only <stdint.h>). Implements READ SECTORS (0x20) PIO on a configurable ATA
 * channel + drive (primary master = boot disk; primary slave = FAT12 data
 * disk) via a per-device ata_ctx_t.
 *
 * FIRST EMULATOR RUN (brief Sec 2.1): until now this file only had to compile;
 * make test-fs is its first functional exercise. The PIO poll is therefore
 * armoured with fail-loud, bounded guards (CLAUDE.md Rule 2 / Rule 3):
 *   - 400 ns settle delay (four altstatus reads) after the drive-select write;
 *   - a floating-bus check (STATUS == 0xFF => no drive) that returns an error
 *     IMMEDIATELY instead of spinning on BSY forever (the guaranteed hang);
 *   - a BOUNDED BSY-clear / DRQ-set poll (a large but finite spin -> timeout);
 *   - an ERR/DF check after the command.
 * No path can spin forever.
 *
 * CANNOT be host-unit-tested: it issues port I/O (inb/outb), which only means
 * anything on real hardware or an emulator. Build-time contract: "compiles
 * clean under the freestanding flags"; functional validation = the make
 * test-fs boot oracle.
 *
 * Ref (Law 1): ATA/ATAPI-6, READ SECTORS (0x20) PIO protocol; primary-channel
 *   port map 0x1F0..0x1F7 + device-control 0x3F6; drive/head register bit 4
 *   (master/slave), bit 6 (LBA); the 400 ns post-select delay + the
 *   floating-bus 0xFF convention. docs/research/fs-mount-sft-ground-truth.md
 *   Sec 1.3, Sec 2.2, Sec 2.3.
 *
 * ASCII-clean (Rule 12). Reproducible (Rule 11): no timestamps / host paths.
 */

#include "ata.h"

#include <stddef.h>   /* NULL */

/* Command-block register offsets from io_base (ATA/ATAPI-6 Table 8). */
#define ATA_REG_DATA      0u /* 16-bit data register (PIO transfer)     */
#define ATA_REG_ERROR     1u /* error (read) / features (write)         */
#define ATA_REG_SECCOUNT  2u /* sector count                            */
#define ATA_REG_LBA_LO    3u /* LBA bits 0..7                           */
#define ATA_REG_LBA_MID   4u /* LBA bits 8..15                          */
#define ATA_REG_LBA_HI    5u /* LBA bits 16..23                         */
#define ATA_REG_DRIVE     6u /* drive/head + LBA bits 24..27            */
#define ATA_REG_STATUS    7u /* status (read) / command (write)         */
#define ATA_REG_COMMAND   7u

/* Status register bits. */
#define ATA_SR_BSY  0x80u /* busy        */
#define ATA_SR_DRQ  0x08u /* data request*/
#define ATA_SR_DF   0x20u /* device fault*/
#define ATA_SR_ERR  0x01u /* error       */

#define ATA_CMD_READ_SECTORS  0x20u /* READ SECTORS  (PIO, LBA28)            */
#define ATA_CMD_WRITE_SECTORS 0x30u /* WRITE SECTORS (PIO, LBA28)            */
#define ATA_CMD_CACHE_FLUSH   0xE7u /* FLUSH CACHE (commit the write buffer) */

/* Floating-bus value on the status register when no drive is attached to the
 * selected slot (brief Sec 2.3). Reading 0xFF means BSY appears set forever; we
 * MUST bail rather than poll. */
#define ATA_FLOATING_BUS 0xFFu

/* Bounded poll budget. Each iteration is one IN from a port (~hundreds of ns on
 * real ISA, far less on QEMU). 1e7 is generous for a settled controller yet
 * finite -- a stuck drive yields ATA_ERR_TIMEOUT, never an infinite hang
 * (Rule 2). */
#define ATA_POLL_LIMIT 10000000u

/* Freestanding port I/O primitives. The artifact has no libc; these are the
 * canonical x86 inb/outb/inw wrappers. */
static inline uint8_t inb(uint16_t port)
{
	uint8_t v;
	__asm__ __volatile__("inb %1, %0" : "=a"(v) : "Nd"(port));
	return v;
}

static inline void outb(uint16_t port, uint8_t v)
{
	__asm__ __volatile__("outb %0, %1" : : "a"(v), "Nd"(port));
}

static inline uint16_t inw(uint16_t port)
{
	uint16_t v;
	__asm__ __volatile__("inw %1, %0" : "=a"(v) : "Nd"(port));
	return v;
}

static inline void outw(uint16_t port, uint16_t v)
{
	__asm__ __volatile__("outw %0, %1" : : "a"(v), "Nd"(port));
}

/* 400 ns settle delay: read the alternate-status register four times and
 * discard. Reading altstatus (the device-control port) has NO side effects
 * (unlike the primary status port, which acks a pending IRQ). On QEMU this is a
 * near no-op; on real hardware / 86Box it gives the device time to assert BSY
 * after the drive-select write (brief Sec 2.3). */
static void ata_delay_400ns(uint16_t ctrl_base)
{
	(void)inb(ctrl_base);
	(void)inb(ctrl_base);
	(void)inb(ctrl_base);
	(void)inb(ctrl_base);
}

/*
 * Wait until the selected drive is ready to hand over one sector of data:
 *   - if STATUS reads 0xFF the slot is empty (floating bus) -> ATA_ERR_NO_DRIVE
 *     IMMEDIATELY (brief Sec 2.3 -- never spin on a phantom BSY);
 *   - poll (bounded) for BSY to clear -> ATA_ERR_TIMEOUT on budget exhaustion;
 *   - once BSY clears, ERR/DF set -> ATA_ERR_DEVICE;
 *   - DRQ not set (and no error) -> ATA_ERR_DEVICE (command accepted but no
 *     data transfer -- a genuine anomaly).
 * Returns 0 only when DRQ is set with no error (fail loud, Rule 2).
 */
static int ata_wait_drq(uint16_t io_base, uint16_t ctrl_base)
{
	uint8_t st;
	uint32_t spins;

	/* Floating-bus guard FIRST: an absent slave reads 0xFF forever. */
	st = inb((uint16_t)(io_base + ATA_REG_STATUS));
	if (st == ATA_FLOATING_BUS) {
		return ATA_ERR_NO_DRIVE;
	}

	/* Bounded BSY-clear poll. */
	for (spins = 0; spins < ATA_POLL_LIMIT; spins++) {
		st = inb((uint16_t)(io_base + ATA_REG_STATUS));
		if (st == ATA_FLOATING_BUS) {
			return ATA_ERR_NO_DRIVE;
		}
		if (!(st & ATA_SR_BSY)) {
			break;
		}
	}
	if (st & ATA_SR_BSY) {
		return ATA_ERR_TIMEOUT;   /* never cleared within the budget */
	}

	if (st & (ATA_SR_ERR | ATA_SR_DF)) {
		return ATA_ERR_DEVICE;
	}
	if (!(st & ATA_SR_DRQ)) {
		return ATA_ERR_DEVICE;    /* ready but no data request: anomaly */
	}

	(void)ctrl_base;
	return 0;
}

/*
 * Wait until the drive is NOT busy (BSY clear) with NO error -- used after the
 * CACHE FLUSH command (0xE7), which transfers no data (no DRQ to wait on), and
 * after the final WRITE sector to let the write buffer drain before FLUSH. Same
 * fail-loud discipline as ata_wait_drq (floating-bus guard, bounded poll,
 * ERR/DF check), but it does NOT require DRQ. Returns 0 only when BSY is clear
 * with no error (Rule 2).
 */
static int ata_wait_ready(uint16_t io_base, uint16_t ctrl_base)
{
	uint8_t  st;
	uint32_t spins;

	st = inb((uint16_t)(io_base + ATA_REG_STATUS));
	if (st == ATA_FLOATING_BUS) {
		return ATA_ERR_NO_DRIVE;
	}

	for (spins = 0; spins < ATA_POLL_LIMIT; spins++) {
		st = inb((uint16_t)(io_base + ATA_REG_STATUS));
		if (st == ATA_FLOATING_BUS) {
			return ATA_ERR_NO_DRIVE;
		}
		if (!(st & ATA_SR_BSY)) {
			break;
		}
	}
	if (st & ATA_SR_BSY) {
		return ATA_ERR_TIMEOUT;
	}
	if (st & (ATA_SR_ERR | ATA_SR_DF)) {
		return ATA_ERR_DEVICE;
	}

	(void)ctrl_base;
	return 0;
}

int ata_read_sectors(void *ctx, uint32_t lba, uint32_t count, void *buf)
{
	const ata_ctx_t *c = (const ata_ctx_t *)ctx;
	uint8_t *out = (uint8_t *)buf;
	uint16_t io_base;
	uint16_t ctrl_base;
	uint32_t s;

	if (c == NULL || buf == NULL || count == 0u) {
		return ATA_ERR_ARG;
	}
	/* LBA28 + the 8-bit sector count register cap this transfer. */
	if (count > 256u || lba > 0x0FFFFFFFu) {
		return ATA_ERR_ARG;
	}

	io_base   = c->io_base;
	ctrl_base = c->ctrl_base;

	/* Program the IDE registers for an LBA28 READ SECTORS (ATA/ATAPI-6). The
	 * drive/head register selects channel device + carries LBA bits 24..27.
	 * Write order (DRIVE, SECCOUNT, LBA..., COMMAND) is the ATA-conformant
	 * sequence Bochs/86Box enforce (brief Sec 2.2). */
	outb((uint16_t)(io_base + ATA_REG_DRIVE),
	     (uint8_t)(c->drive_select | ((lba >> 24) & 0x0Fu)));

	/* 400 ns settle so the device has time to assert BSY before we poll, and
	 * to register the drive selection (brief Sec 2.3). */
	ata_delay_400ns(ctrl_base);

	outb((uint16_t)(io_base + ATA_REG_SECCOUNT), (uint8_t)(count & 0xFFu));
	outb((uint16_t)(io_base + ATA_REG_LBA_LO),   (uint8_t)(lba & 0xFFu));
	outb((uint16_t)(io_base + ATA_REG_LBA_MID),  (uint8_t)((lba >> 8) & 0xFFu));
	outb((uint16_t)(io_base + ATA_REG_LBA_HI),   (uint8_t)((lba >> 16) & 0xFFu));
	outb((uint16_t)(io_base + ATA_REG_COMMAND),  ATA_CMD_READ_SECTORS);

	/* One DRQ/transfer cycle per sector (256 words = 512 bytes each). */
	for (s = 0; s < count; s++) {
		int i;
		int rc = ata_wait_drq(io_base, ctrl_base);
		if (rc != 0) {
			return rc;   /* propagate the exact fail-loud code */
		}
		for (i = 0; i < 256; i++) {
			uint16_t w = inw((uint16_t)(io_base + ATA_REG_DATA));
			*out++ = (uint8_t)(w & 0xFFu);
			*out++ = (uint8_t)((w >> 8) & 0xFFu);
		}
	}

	return 0;
}

/*
 * ata_write_sectors -- ATA PIO LBA28 WRITE SECTORS (0x30) + CACHE FLUSH (0xE7).
 *
 * THE WRITE HALF of the disk path (beads initech-509.11). Mirrors
 * ata_read_sectors' register-programming order and fail-loud guards exactly,
 * but the data direction is REVERSED: after the WRITE SECTORS command the drive
 * raises DRQ when it is ready to ACCEPT a sector, and we push 256 16-bit words
 * (512 bytes) PER sector OUT to the data port. After the last sector we issue
 * CACHE FLUSH so the controller commits its write buffer to the medium (without
 * it, a power loss -- or in our case, a subsequent read -- could see stale
 * data). CANNOT be host-unit-tested (port I/O); proven by the in-emulator
 * round-trip (make test-fatwrite).
 *
 * Ref (Law 1): ATA/ATAPI-6, WRITE SECTORS (0x30) PIO protocol + FLUSH CACHE
 *   (0xE7); the DRQ-per-sector handshake (host writes data when DRQ is set);
 *   docs/research/fs-mount-sft-ground-truth.md Sec 1.3, Sec 2.2, Sec 2.3 (the
 *   same channel/drive-select + 400 ns settle + floating-bus convention as the
 *   read path). Returns 0 on success, a negative ATA_ERR_* on error (never an
 *   infinite spin -- bounded polls throughout, Rule 2).
 *
 * NOTE: a const-correct write buffer (the blockdev_t::write_sectors signature
 * takes `const void *`) -- we never modify it; we only read words OUT of it.
 */
int ata_write_sectors(void *ctx, uint32_t lba, uint32_t count, const void *buf)
{
	const ata_ctx_t *c   = (const ata_ctx_t *)ctx;
	const uint8_t   *in  = (const uint8_t *)buf;
	uint16_t io_base;
	uint16_t ctrl_base;
	uint32_t s;
	int      rc;

	if (c == NULL || buf == NULL || count == 0u) {
		return ATA_ERR_ARG;
	}
	if (count > 256u || lba > 0x0FFFFFFFu) {
		return ATA_ERR_ARG;
	}

	io_base   = c->io_base;
	ctrl_base = c->ctrl_base;

	/* Same LBA28 register-programming order as the read path, but the command
	 * is WRITE SECTORS (0x30). (brief Sec 2.2 -- the ATA-conformant sequence
	 * Bochs/86Box enforce.) */
	outb((uint16_t)(io_base + ATA_REG_DRIVE),
	     (uint8_t)(c->drive_select | ((lba >> 24) & 0x0Fu)));

	ata_delay_400ns(ctrl_base);

	outb((uint16_t)(io_base + ATA_REG_SECCOUNT), (uint8_t)(count & 0xFFu));
	outb((uint16_t)(io_base + ATA_REG_LBA_LO),   (uint8_t)(lba & 0xFFu));
	outb((uint16_t)(io_base + ATA_REG_LBA_MID),  (uint8_t)((lba >> 8) & 0xFFu));
	outb((uint16_t)(io_base + ATA_REG_LBA_HI),   (uint8_t)((lba >> 16) & 0xFFu));
	outb((uint16_t)(io_base + ATA_REG_COMMAND),  ATA_CMD_WRITE_SECTORS);

	/* One DRQ/transfer cycle per sector -- the host PUSHES the data this time
	 * (256 words = 512 bytes each). DRQ means "ready to accept a sector". */
	for (s = 0; s < count; s++) {
		int i;
		rc = ata_wait_drq(io_base, ctrl_base);
		if (rc != 0) {
			return rc;   /* propagate the exact fail-loud code */
		}
		for (i = 0; i < 256; i++) {
			uint16_t w = (uint16_t)((uint16_t)in[0] | ((uint16_t)in[1] << 8));
			outw((uint16_t)(io_base + ATA_REG_DATA), w);
			in += 2;
		}
		/* Per the PIO protocol, allow the drive a moment to deassert DRQ /
		 * raise BSY for the next sector or completion (the 400 ns altstatus
		 * settle covers this without side effects). */
		ata_delay_400ns(ctrl_base);
	}

	/* CACHE FLUSH so the write buffer is committed before any read sees it
	 * (Rule 2 / Rule 11: a subsequent read of what we wrote must be authoritative,
	 * not a stale-cache artifact). FLUSH transfers no data -> wait BSY-clear. */
	outb((uint16_t)(io_base + ATA_REG_COMMAND), ATA_CMD_CACHE_FLUSH);
	rc = ata_wait_ready(io_base, ctrl_base);
	if (rc != 0) {
		return rc;
	}

	return 0;
}

void ata_blockdev_init(blockdev_t *dev, ata_ctx_t *ctx)
{
	if (dev == NULL) {
		return;
	}
	dev->ctx           = ctx;            /* channel + drive descriptor       */
	dev->read_sectors  = ata_read_sectors;
	dev->write_sectors = ata_write_sectors;  /* FAT write (beads initech-509.11) */
}

void ata_ctx_init_primary_master(ata_ctx_t *ctx)
{
	if (ctx == NULL) {
		return;
	}
	ctx->io_base      = ATA_IO_BASE_PRIMARY;
	ctx->ctrl_base    = ATA_CTRL_PRIMARY;
	ctx->drive_select = ATA_DRIVE_MASTER_LBA;
}

void ata_ctx_init_primary_slave(ata_ctx_t *ctx)
{
	if (ctx == NULL) {
		return;
	}
	ctx->io_base      = ATA_IO_BASE_PRIMARY;
	ctx->ctrl_base    = ATA_CTRL_PRIMARY;
	ctx->drive_select = ATA_DRIVE_SLAVE_LBA;
}
