/* fileio_fat.h -- FAT12-backed INT 21h file backend (beads initech-509.5).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): shipped InitechDOS kernel code. Binds the
 * concrete int21_file_backend_t (os/milton/int21.h) to a mounted FAT12 volume
 * (os/milton/fat12.h) so the file-handle INT 21h functions (3Dh OPEN / 3Fh
 * READ / 4Eh/4Fh FINDFIRST/NEXT) can reach real files. Kernel-only (it pulls
 * fat12.c + the volume); the host unit oracle binds a mock backend instead.
 *
 * Ref (Law 1): docs/research/fs-mount-sft-ground-truth.md Sec 4 / Sec 6 Step
 *   4/5; os/milton/int21.h (int21_file_backend_t). Rule 12 (ASCII), Rule 11
 *   (deterministic).
 */
#ifndef INITECH_MILTON_FILEIO_FAT_H
#define INITECH_MILTON_FILEIO_FAT_H

#include "fat12.h"   /* fat12_volume_t */

/* Cache the volume's FAT, then bind the FAT12 file backend into int21 (so the
 * file-handle functions resolve through this volume). `vol` must be a mounted
 * volume that outlives the binding (kernel BSS). Returns 0 on success, negative
 * (a FAT12_ERR_* from fat12_read_fat) on failure -- fail loud (Rule 2), the
 * caller does NOT bind a half-initialised backend. */
int fileio_fat_bind(const fat12_volume_t *vol);

/* Return the backend's cached FAT buffer (+ its byte length via *out_len) so
 * SYSINIT can read CONFIG.SYS off the volume WITHOUT a second ~4.6 KiB FAT
 * buffer (beads initech-509.2 -- keeps the kernel .bss clear of PROGRAM_BASE).
 * Returns 0 / *out_len = 0 before a successful fileio_fat_bind. */
void *fileio_fat_fat_buffer(uint32_t *out_len);

#endif /* INITECH_MILTON_FILEIO_FAT_H */
