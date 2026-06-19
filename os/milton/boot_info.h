/* boot_info.h -- stage2 -> C kernel parameter block.
 *
 * Ref: PRD Sec 5 (hardware contract: VBE 2.0 LFB, 640x480, 8/32bpp, flat
 *      binary kernel handed off by stage2, ADR-0003 DEC-08); stage2.asm
 *      lfb_addr/pitch/bpp variables; docs/research/boot-to-text-ground-truth.md
 *      Sec 2 (the handoff contract); CLAUDE.md ADR-0002 + CDR-0001
 *      (freestanding C, gcc -m32 -ffreestanding -nostdlib).
 * beads: initech-d00.
 *
 * ARTIFACT code: freestanding, <stdint.h> only, no libc.
 *
 * The block lives at a fixed physical address (BOOT_INFO_ADDR). stage2 writes
 * the 28 bytes in field order in real mode (before the PM switch); the flat C
 * kernel reads them by casting the constant to a volatile pointer -- no
 * register convention, no relocation (a physical constant, flat-binary safe).
 *
 * EXTENSION (beads initech-k8o5.5; ADR-0004 DEC-03 / FO-5 / AM-5, RATIFIED
 * 2026-06-19): the LAST field `ext_mem_kb` is a backward-compatible append --
 * stage2 probes installed extended memory (INT 15h E820/E801/88h) in real mode
 * and writes the total KiB above 1 MiB here; the kernel gates boot on it
 * (FLAIR_HEAP_MIN, spec/memory_map.h). Existing readers that only touch the
 * first six (LFB + font) fields are unaffected -- the block grew 24 -> 28 bytes
 * but no earlier field moved.
 */
#ifndef INITECH_BOOT_INFO_H
#define INITECH_BOOT_INFO_H

#include <stdint.h>

typedef struct {
    uint32_t lfb_addr;    /* physical base of the linear framebuffer        */
    uint32_t lfb_pitch;   /* bytes per scanline (may exceed width * bpp/8)  */
    uint32_t lfb_bpp;     /* bits per pixel: 24 or 32                       */
    uint32_t lfb_width;   /* horizontal resolution in pixels (640)          */
    uint32_t lfb_height;  /* vertical resolution in pixels (480)            */
    uint32_t font_addr;   /* physical address of the 4096-byte ROM font     */
                          /* (256 glyphs * 16 bytes; MSB=leftmost pixel)    */
    uint32_t ext_mem_kb;  /* extended-memory KiB above 1 MiB, probed by      */
                          /* stage2 via INT 15h E820 (E801/AH=88h fallbacks);*/
                          /* 0 if every probe failed. Gates the FLAIR heap   */
                          /* (FLAIR_HEAP_MIN, spec/memory_map.h; ADR-0004     */
                          /* DEC-03). LAST field -- append-only (see header). */
} boot_info_t;

/* Fixed physical address of the boot_info block (above the BDA at 0x4FF). */
#define BOOT_INFO_ADDR  0x00000500u

/* Fixed physical address of the captured ROM font (4096 bytes). */
#define FONT_STASH_ADDR 0x00001000u

#endif /* INITECH_BOOT_INFO_H */
