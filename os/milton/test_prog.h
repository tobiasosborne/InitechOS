/* test_prog.h -- the baked flat test program image (beads initech-509.5).
 *
 * The bytes are GENERATED at build time from os/milton/test_program.asm:
 *   nasm -f bin test_program.asm -o build/test_program.bin
 *   tools/bin2c build/test_program.bin g_test_prog_image > build/test_prog_blob.h
 * and defined in that generated header (linked into the kernel .rodata). The
 * loader copies these bytes to PROGRAM_IMAGE (0x38100) and JMPs in.
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

/* The baked IRQ-STORM program (FINDFIRST/NEXT enumeration + a multi-cluster READ
 * + a second concurrent handle, run WHILE the harness storms keystrokes and the
 * PIT ticks; beads initech-xk2). Generated from os/milton/irqstorm_program.asm.
 * Run only in the -DBOOT_IRQSTORM self-test image (make test-int21-irqstorm),
 * which attaches a FAT12 storm disk (ALPHA/BRAVO/.../STORM.DAT). */
extern const uint8_t  g_irqstorm_prog_image[];
extern const uint32_t g_irqstorm_prog_image_len;

/* The baked DATE/TIME program (AH=2Ah GET DATE + AH=2Ch GET TIME + AH=36h GET
 * DISK FREE SPACE + AH=62h GET PSP, results to serial; beads initech-yv9).
 * Generated from os/milton/datetime_program.asm. Run only in the -DBOOT_DATETIME
 * self-test image (make test-datetime), booted with a PINNED RTC (-rtc base). */
extern const uint8_t  g_datetime_prog_image[];
extern const uint32_t g_datetime_prog_image_len;

/* The baked SETVECT/GETVECT + INT 24h critical-error program (AH=35h GETVECT 0x24
 * -> "V24PRE=" / int $0x24 -> MSG-DOS-0001 + injected 'a' -> "CRIT-AL=" / AH=25h
 * SETVECT 0x24 to a bogus handler then EXIT; beads initech-509.8). Generated from
 * os/milton/vect_program.asm. Run only in the -DBOOT_VECT self-test image (make
 * test-vect), where the harness injects the operator key and kmain reports the
 * post-EXIT 0x24 vector ("V24POST=") to prove the loader restored it. */
extern const uint8_t  g_vect_prog_image[];
extern const uint32_t g_vect_prog_image_len;

/* The baked INT 25h/26h ABSOLUTE-DISK round-trip program (int $0x26 WRITE a
 * deterministic pattern to a SAFE scratch LBA -> int $0x25 READ it back ->
 * byte-compare -> "ABS-W26=OK"/"ABS-R25=OK"/"ABS-RT=OK" to serial; beads
 * initech-8403). Generated from os/milton/absdisk_program.asm. Run only in the
 * -DBOOT_ABSDISK self-test image (make test-absdisk-emu), which closes the
 * asm-stub coverage gap on int25_entry/int26_entry -> dispatch -> IRETD that the
 * HOST oracle (test_absdisk.c) cannot reach. Requires a WRITABLE FAT12 data disk
 * on --disk2 so the absolute-disk seam is bound (kmain ABSDISK-BIND-OK). */
extern const uint8_t  g_absdisk_prog_image[];
extern const uint32_t g_absdisk_prog_image_len;

#endif /* INITECH_TEST_PROG_H */
