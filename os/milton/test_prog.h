/* test_prog.h -- the baked flat test program image (beads initech-509.5).
 *
 * The bytes are GENERATED at build time from os/milton/test_program.asm:
 *   nasm -f bin test_program.asm -o build/test_program.bin
 *   tools/bin2c build/test_program.bin g_test_prog_image > build/test_prog_blob.h
 * and defined in that generated header (linked into the kernel .rodata). The
 * loader copies these bytes to PROGRAM_IMAGE (0x20100) and JMPs in.
 *
 * Ref: docs/research/psp-loader-ground-truth.md Sec 5.4 (embedding). CLAUDE.md
 *      Law 1 (cite), Rule 11 (deterministic), Rule 12 (ASCII).
 */
#ifndef INITECH_TEST_PROG_H
#define INITECH_TEST_PROG_H

#include <stdint.h>

/* The flat (.COM-equivalent) test program bytes + length (generated blob). */
extern const uint8_t  g_test_prog_image[];
extern const uint32_t g_test_prog_image_len;

/* The baked TYPE program (OPEN+READ+WRITE+CLOSE HELLO.TXT; beads initech-509.5
 * read-side). Generated from os/milton/type_program.asm. */
extern const uint8_t  g_type_prog_image[];
extern const uint32_t g_type_prog_image_len;

/* The baked DIR program (FINDFIRST/FINDNEXT *.*; beads initech-509.5 read-side).
 * Generated from os/milton/dir_program.asm. */
extern const uint8_t  g_dir_prog_image[];
extern const uint32_t g_dir_prog_image_len;

/* The baked CON-INPUT program (AH=0Ah buffered input -> echo the line back;
 * beads initech-n62). Generated from os/milton/conin_program.asm. Run only in
 * the -DBOOT_CONIN self-test image (make test-conin), where the harness injects
 * keystrokes via QMP --keys. */
extern const uint8_t  g_conin_prog_image[];
extern const uint32_t g_conin_prog_image_len;

/* The baked WRITE round-trip program (CREAT+WRITE+CLOSE then OPEN+READ+echo;
 * beads initech-509.11). Generated from os/milton/write_program.asm. Run only in
 * the -DBOOT_WRITE self-test image (make test-fatwrite), which attaches a
 * WRITABLE FAT12 data disk. */
extern const uint8_t  g_write_prog_image[];
extern const uint32_t g_write_prog_image_len;

/* The baked MULTI-OPEN program (two concurrent OPENs + interleaved positioned
 * reads/LSEEK + a >64 KiB read; beads initech-0qh; epic initech-6qy). Generated
 * from os/milton/multiopen_program.asm. Run only in the -DBOOT_MULTIOPEN
 * self-test image (make test-multiopen), which attaches a FAT12 data disk with
 * HELLO.TXT + SECOND.TXT + a >64 KiB BIG.DAT. */
extern const uint8_t  g_multiopen_prog_image[];
extern const uint32_t g_multiopen_prog_image_len;

#endif /* INITECH_TEST_PROG_H */
