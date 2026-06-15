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

#include <stdint.h>
#include <stdio.h>

#include "blockdev.h"   /* os/milton/blockdev.h, on -Ios/milton */

/* Backing state for a file-backed blockdev. The caller owns this struct and
 * passes &state.dev (a fully wired blockdev_t) to fat12_mount.
 *
 * FAULT-INJECTION SEAM (beads initech-lpf3): the structurally-present rollback
 * paths in fat12_mkdir (set_entry-EOC-fail, flush-fail post-grow), fat12_create
 * and fat12_write_file are only reached when an UNDERLYING write_sectors fails
 * mid-operation. A real disk-geometry exhaustion only reaches the NO_SPACE leg
 * (fat12_find_free returns 0 -- no device write happens). To drive the
 * write-FAULT legs RED we count write_sectors calls and force the Nth one to
 * return an error (Rule 2: a forced fault is reported loud, like a real device
 * I/O error). DISARMED by default (`write_fail_at == 0`), so every existing
 * oracle's behaviour is byte-identical -- the seam only bites when armed.
 *
 * Ref (Law 1): os/milton/blockdev.h write contract (0 ok / negative on error);
 *   the rollback discipline these fields exercise is fat12.c's own (Rule 2/3,
 *   beads initech-m0bp rollback fix + the fat12_write_file partial-allocation
 *   rollback the bead cites). The injected error mirrors a real ata_write
 *   failure -- it is NOT a compile-time mutant of fat12.c. */
typedef struct blockdev_file {
	FILE      *fp;             /* open disk-image file (read mode)            */
	blockdev_t dev;            /* wired blockdev_t with ctx = this struct     */

	/* Fault-injection state (all 0 == disarmed; set by the open helpers). */
	uint32_t   write_fail_at;  /* 1-based ordinal of the write to fail; 0=off */
	uint32_t   write_calls;    /* count of write_sectors calls seen so far    */
	int        write_faulted;  /* set non-zero once the injected fault fired  */
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

/* ARM the write-fault seam (beads initech-lpf3): make the `nth` (1-based)
 * subsequent write_sectors call return an error, exactly as a real device I/O
 * failure would (Rule 2). The counter is RESET to 0 on arm, so `nth == K`
 * fails the K-th write_sectors after this call. `nth == 0` DISARMS the seam
 * (writes always succeed). Must be called AFTER a successful open_rw (which
 * itself disarms the seam). Arming a fresh fault (nth != 0) CLEARS the fired
 * flag; DISARMING (nth == 0) PRESERVES it, so a caller can disarm and THEN
 * inspect blockdev_file_write_faulted(). Returns nothing -- NULL bf is a no-op. */
void blockdev_file_arm_write_fault(blockdev_file_t *bf, uint32_t nth);

/* Non-zero iff the armed write fault has actually fired (the Nth write was
 * forced to fail). Lets an oracle assert the fault was REACHED (a rollback
 * test that never tripped the fault has proven nothing -- Law 2). */
int blockdev_file_write_faulted(const blockdev_file_t *bf);

/* Close the backing file. Safe to call once after a successful open. */
void blockdev_file_close(blockdev_file_t *bf);

#endif /* INITECH_FAT_DIFF_BLOCKDEV_FILE_H */
