/*
 * spec/assets/font_linkage.h -- ONE out-of-line copy of each font table in
 * the kernel (bead initech-tdnl.33, PM ruling 7 / slice-1 ruling 3).
 *
 * WHY: the strike headers used to define their tables `static const`, so every
 * kernel translation unit that called an inline accessor carried its own copy
 * (measured at 1cf74a6 on kernel_flairtenants.elf: chicago8x16 x6 = 6 x 1,456 B,
 * geneva9 + geneva9_advance x3 = 3 x 1,140 B). Kernel headroom is a gated
 * resource (test-kernel-headroom, KERNEL_HEADROOM_FLOOR), so the tables are now
 * linked ONCE.
 *
 * THE RULE (freestanding = the kernel; hosted = the host oracles and tools):
 *   - freestanding, FONT_TABLES_DEFINE unset: an `extern const` DECLARATION.
 *     The kernel links os/flair/text.o, the ONE TU that defines
 *     FONT_TABLES_DEFINE, so the bytes exist exactly once in every kernel image.
 *   - freestanding, FONT_TABLES_DEFINE set (text.c): the one DEFINITION.
 *   - hosted (__STDC_HOSTED__ == 1): a `static const` definition per TU, as
 *     before. Host oracles link arbitrary subsets of the Toolbox (well over a
 *     hundred link lines); per-TU copies cost nothing on the host and keep those
 *     link lines unchanged. The DATA is the same header in both worlds, so the
 *     oracles still see exactly the bytes the kernel ships.
 *
 * Usage in a strike header:
 *     #if FONT_TABLE_EMIT
 *     FONT_TABLE unsigned char tbl[N] = { ... };
 *     #else
 *     FONT_TABLE unsigned char tbl[N];
 *     #endif
 *
 * ASCII-clean (Rule 12). No nondeterminism (Rule 11).
 */
#ifndef INITECH_FONT_LINKAGE_H
#define INITECH_FONT_LINKAGE_H

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__
#  define FONT_TABLE_EMIT 1
#  define FONT_TABLE      static const __attribute__((unused))
#elif defined(FONT_TABLES_DEFINE)
#  define FONT_TABLE_EMIT 1
#  define FONT_TABLE      const
#else
#  define FONT_TABLE_EMIT 0
#  define FONT_TABLE      extern const
#endif

#endif /* INITECH_FONT_LINKAGE_H */
