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

#endif /* INITECH_TEST_PROG_H */
