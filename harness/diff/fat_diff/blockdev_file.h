/*
 * harness/diff/fat_diff/blockdev_file.h -- host file-backed blockdev (FACTORY).
 *
 * FACTORY code (CLAUDE.md Law 3): hosted, libc OK. NOT shipped in the artifact.
 * This backs the artifact's blockdev_t read contract (os/milton/blockdev.h)
 * with an ordinary disk-image FILE on the host, so the FAT12 oracle can drive
 * the real os/milton/fat12.c without an emulator.
 *
 * Ref (Law 1): os/milton/blockdev.h (the contract this implements);
 *   docs/research/fat12-ground-truth.md.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 */
#ifndef INITECH_FAT_DIFF_BLOCKDEV_FILE_H
#define INITECH_FAT_DIFF_BLOCKDEV_FILE_H

#include <stdio.h>

#include "blockdev.h"   /* os/milton/blockdev.h, on -Ios/milton */

/* Backing state for a file-backed blockdev. The caller owns this struct and
 * passes &state.dev (a fully wired blockdev_t) to fat12_mount. */
typedef struct blockdev_file {
	FILE      *fp;   /* open disk-image file (read mode)        */
	blockdev_t dev;  /* wired blockdev_t with ctx = this struct */
} blockdev_file_t;

/* Open `path` read-only and wire `bf->dev`. Returns 0 on success, negative on
 * error (e.g. file not found). On success bf->dev.read_sectors is ready and
 * bf->dev.write_sectors is NULL. */
int blockdev_file_open(blockdev_file_t *bf, const char *path);

/* Open `path` read-WRITE ("r+b") and wire `bf->dev` with BOTH read_sectors and
 * write_sectors live -- the host backend for the FAT12 WRITE oracle (beads
 * initech-509.11). Returns 0 on success, negative on error. The file must
 * already exist (a freshly minted blank FAT12 image). */
int blockdev_file_open_rw(blockdev_file_t *bf, const char *path);

/* Close the backing file. Safe to call once after a successful open. */
void blockdev_file_close(blockdev_file_t *bf);

#endif /* INITECH_FAT_DIFF_BLOCKDEV_FILE_H */
