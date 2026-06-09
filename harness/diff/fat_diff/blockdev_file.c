/*
 * harness/diff/fat_diff/blockdev_file.c -- host file-backed blockdev (FACTORY).
 *
 * FACTORY code (CLAUDE.md Law 3): hosted, libc OK. Implements the artifact's
 * blockdev_t read contract (os/milton/blockdev.h) against a disk-image FILE so
 * the oracle exercises os/milton/fat12.c on the host with no emulator.
 *
 * Ref (Law 1): os/milton/blockdev.h contract -- sector size 512, read `count`
 *   consecutive sectors at `lba`, 0 on success / negative on error (fail loud,
 *   CLAUDE.md Rule 2: a short read MUST report an error, never partial green).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 */

#include "blockdev_file.h"

#include <stddef.h>

/* read_sectors implementation: ctx is the blockdev_file_t. Seeks to
 * lba*512 and reads count*512 bytes. Returns 0 on a full read, negative on
 * any seek/short/EOF error (Rule 2). */
static int blockdev_file_read(void *ctx, uint32_t lba, uint32_t count, void *buf)
{
	blockdev_file_t *bf = (blockdev_file_t *)ctx;
	long             off;
	size_t           want;
	size_t           got;

	if (bf == NULL || bf->fp == NULL || buf == NULL || count == 0u) {
		return -1;
	}

	/* Byte offset of the first requested sector. */
	off  = (long)lba * (long)BLOCKDEV_SECTOR_SIZE;
	want = (size_t)count * (size_t)BLOCKDEV_SECTOR_SIZE;

	if (fseek(bf->fp, off, SEEK_SET) != 0) {
		return -1;
	}
	got = fread(buf, 1u, want, bf->fp);
	if (got != want) {
		/* Short read / past EOF -> fail loud, do not return garbage. */
		return -1;
	}
	return 0;
}

/* write_sectors implementation: seek to lba*512 and write count*512 bytes from
 * buf. Returns 0 on a full write, negative on any seek/short-write error (Rule
 * 2 -- never report a partial write as success). Mirrors the read path. */
static int blockdev_file_write(void *ctx, uint32_t lba, uint32_t count,
                               const void *buf)
{
	blockdev_file_t *bf = (blockdev_file_t *)ctx;
	long             off;
	size_t           want;
	size_t           put;

	if (bf == NULL || bf->fp == NULL || buf == NULL || count == 0u) {
		return -1;
	}
	off  = (long)lba * (long)BLOCKDEV_SECTOR_SIZE;
	want = (size_t)count * (size_t)BLOCKDEV_SECTOR_SIZE;

	if (fseek(bf->fp, off, SEEK_SET) != 0) {
		return -1;
	}
	put = fwrite(buf, 1u, want, bf->fp);
	if (put != want) {
		return -1;
	}
	if (fflush(bf->fp) != 0) {
		return -1;   /* commit to the file so a later read sees it */
	}
	return 0;
}

int blockdev_file_open(blockdev_file_t *bf, const char *path)
{
	if (bf == NULL || path == NULL) {
		return -1;
	}
	bf->fp = fopen(path, "rb");
	if (bf->fp == NULL) {
		return -1;
	}
	bf->dev.ctx           = bf;
	bf->dev.read_sectors  = blockdev_file_read;
	bf->dev.write_sectors = NULL; /* read-only open */
	return 0;
}

int blockdev_file_open_rw(blockdev_file_t *bf, const char *path)
{
	if (bf == NULL || path == NULL) {
		return -1;
	}
	bf->fp = fopen(path, "r+b");   /* read-write; file must already exist */
	if (bf->fp == NULL) {
		return -1;
	}
	bf->dev.ctx           = bf;
	bf->dev.read_sectors  = blockdev_file_read;
	bf->dev.write_sectors = blockdev_file_write;  /* WRITE oracle backend */
	return 0;
}

void blockdev_file_close(blockdev_file_t *bf)
{
	if (bf != NULL && bf->fp != NULL) {
		fclose(bf->fp);
		bf->fp = NULL;
	}
}
