/*
 * os/flair/ostype.h -- the FLAIR four-character resource/scrap TYPE code (the
 * artifact). The era-universal 32-bit "OSType" / "ResType" key.
 *
 * beads: initech-49ez (FLAIR Phase 4.5 -- Platform Services). Shared prelude for
 *        the Resource Manager (initech-0w45) and the Scrap (initech-b2vk): both
 *        key their stores by a 4-character type code, so the type + the
 *        endian-NEUTRAL constructor macro live in ONE header to dodge a
 *        ResType/OSType redefinition collision between scrap.h and resource.h.
 *
 * WHAT THIS IS (Law 1):
 *   Classic Mac OS keys every resource (Resource Manager) and every clipboard
 *   flavor (Scrap Manager) by a 4-character ASCII code packed big-endian into a
 *   32-bit word -- 'TEXT' == 0x54455854, 'WIND' == 0x57494E44, etc. (Inside
 *   Macintosh; ../system7-decomp/specs/resources/resource-manager.md Sec 1.4
 *   "resType: 4-character resource type code (OSType)"). FLAIR adopts the same
 *   key space for its clean-room Resource Manager + Scrap so the addressing model
 *   is period-authentic and the Resource Manager can parse REAL big-endian Mac
 *   resource forks (resource-manager.md Sec 1.1-1.6).
 *
 * THE ENDIANNESS DISCIPLINE (Law 3 / Rule 11):
 *   FLAIR_OSTYPE(a,b,c,d) builds the packed word by VALUE -- (a<<24)|(b<<16)|
 *   (c<<8)|d -- so a compile-time type constant is the SAME numeric value on the
 *   little-endian host oracle build and the little-endian flat-32 kernel. It is
 *   NOT a `char[4]` reinterpret-cast (which would byte-flip between host memory
 *   order and the on-disk big-endian fork). The Resource Manager's fork parser
 *   reads on-disk OSType bytes positionally (b[0]<<24|...|b[3]) so a parsed
 *   on-disk type compares EQUAL to a FLAIR_OSTYPE() literal on either build --
 *   deterministic, host==kernel (the dual-compile / reproducible-build contract).
 *
 * FREESTANDING (Law 3): <stdint.h> only; no libc. Compiles BOTH under the kernel
 * flags (gcc -m32 -ffreestanding -nostdlib -std=c11 -Wall -Wextra -Werror) AND
 * hosted for the property suite.
 *
 * Ref: ../system7-decomp/specs/resources/resource-manager.md Sec 1.4 (OSType);
 *      Inside Macintosh Vol I (Scrap Manager TEXT/PICT flavors); ADR-0012 D-2b
 *      (Phase 4.5 platform services); ADR-0013 Sec 6 (the shell-owned Scrap).
 *      CLAUDE.md Law 1 (ground truth), Law 3 (freestanding, no 2026-ism, host==
 *      kernel determinism), Rule 11 (deterministic), Rule 12 (ASCII-clean).
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_OSTYPE_H
#define INITECH_OS_FLAIR_OSTYPE_H

#include <stdint.h>

/* The 4-character resource/scrap type code, packed big-endian into 32 bits. */
typedef uint32_t flair_ostype_t;

/* FLAIR_OSTYPE -- pack four chars into the big-endian-ordered 32-bit key, BY
 * VALUE (endian-neutral; see the header note). Each char is masked to a byte so
 * a signed `char` with the high bit set cannot sign-extend into the upper bits. */
#define FLAIR_OSTYPE(a, b, c, d)                              \
    ((flair_ostype_t)(((uint32_t)(uint8_t)(a) << 24) |        \
                      ((uint32_t)(uint8_t)(b) << 16) |        \
                      ((uint32_t)(uint8_t)(c) << 8) |         \
                      ((uint32_t)(uint8_t)(d))))

/* Pin the packing: 'TEXT' is 0x54 0x45 0x58 0x54 big-endian. If this assert ever
 * fails the macro's byte order drifted and every type constant is wrong. */
_Static_assert(FLAIR_OSTYPE('T', 'E', 'X', 'T') == 0x54455854u,
               "FLAIR_OSTYPE must pack 4 chars big-endian (host==kernel)");

#endif /* INITECH_OS_FLAIR_OSTYPE_H */
