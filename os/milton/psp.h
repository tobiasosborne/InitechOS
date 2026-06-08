/* psp.h -- InitechDOS Program Segment Prefix (PSP) construction.
 *
 * beads: initech-509.4 ("PSP full 256-byte construction (DEC-05 / App B.2)").
 *        This task is PSP CONSTRUCTION ONLY -- a pure, host-unit-testable
 *        function. The flat program loader (memory placement, control transfer,
 *        the INT 20h trap gate, the baked test program) is the NEXT task and is
 *        NOT implemented here. psp_build() is deliberately free of I/O so it can
 *        compile and run BOTH freestanding (kernel) and hosted (factory oracle).
 *
 * Ref:   docs/research/psp-loader-ground-truth.md Sec 2 (the field-by-field
 *        value map; the flat-mode adaptation of the vestigial SEGMENT fields,
 *        "Option B"); spec/dos_structs.h lines 91-107 (the LOCKED psp_t, 256
 *        bytes, _Static_assert); ADR-0003 DEC-05 (Sec 5.5) + Appendix B.2 (PSP
 *        offsets). CLAUDE.md Law 1 (cite source), Law 2 (oracle is truth),
 *        Law 3 (artifact = C), Rule 2 (fail loud), Rule 8 (specs-as-data),
 *        Rule 11 (deterministic), Rule 12 (ASCII).
 *
 * DESIGN STANCE (ADR-0003 Sec 5.5): vestigial DOS structures are implemented IN
 * FULL, not stubbed. The PSP is mostly vestigial in a flat 32-bit world, but the
 * structure is populated completely; fields whose backing subsystem is deferred
 * are explicitly zero-filled with the deferral noted (saved_vectors -> 509.8;
 * the two default FCBs -> 509.9).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only. No
 * malloc; the caller supplies the 256-byte psp_t buffer. The SAME translation
 * unit (psp.c) ALSO compiles HOSTED for the factory oracle (test_psp.c).
 */
#ifndef INITECH_PSP_H
#define INITECH_PSP_H

#include <stdint.h>
#include "dos_structs.h"   /* psp_t -- the LOCKED 256-byte layout (spec) */

/* Maximum command-tail TEXT length that fits the 128-byte cmd_tail region:
 * byte 0 is the length count, the text follows, and a 0x0D (CR) terminator
 * follows the text. 1 (count) + 127 (text) + ... but the CR must also fit, so
 * the real text maximum is 126 chars + CR at offset 127. Real DOS uses 127 as
 * the nominal cap (count byte can express up to 127); we clamp text to 126 so
 * the trailing CR always lands inside the 128-byte region (offset <= 127).
 * Ref: docs/research/psp-loader-ground-truth.md Sec 2.11. */
#define PSP_CMD_TAIL_MAX_TEXT 126u

/* psp_params_t -- the loader-supplied inputs to psp_build(). The vestigial
 * SEGMENT fields are expressed as flat LINEAR addresses; psp_build() converts
 * each to a "fake paragraph" value (linear >> 4) for storage in the uint16_t
 * segment slots, per the brief's recommended Option B (flat-address-in-fake-
 * paragraph-units). The loader passes the actual addresses; nothing is
 * hard-coded magic in psp_build() (CLAUDE.md Rule 8 -- keep magic in the spec /
 * the caller, not buried in the builder).
 *
 * Ref: docs/research/psp-loader-ground-truth.md Sec 2.2 (alloc_end_seg),
 *      Sec 2.5 (parent_psp), Sec 2.7 (env_seg), Sec 2.11 (cmd_tail). */
typedef struct psp_params {
    /* Flat linear address of the first byte BEYOND the program's allocation
     * (the memory ceiling). Stored at psp_t.alloc_end_seg as (addr >> 4) &
     * 0xFFFF -- a flat address in fake paragraph units. Ref Sec 2.2 / Sec 3.2
     * (e.g. 0x70000 -> 0x7000). */
    uint32_t alloc_end_linear;

    /* Flat linear address of the environment block. Stored at psp_t.env_seg as
     * (addr >> 4) & 0xFFFF. Pass 0 for "no environment block" -> stored env_seg
     * = 0x0000 (the honest null sentinel: a program walking env_seg:0 reads the
     * IVT; the brief's empty-block variant stores 0x2020 for a 0x20200 block).
     * Ref Sec 2.7 / Sec 3.2. */
    uint32_t env_linear;

    /* Flat linear address of the PARENT process's PSP, or 0 for "no parent yet"
     * (the kernel-is-parent / null case in this milestone). Stored at
     * psp_t.parent_psp as (addr >> 4) & 0xFFFF. The brief (Sec 2.5) recommends
     * 0x0000 for now (no COMMAND.COM PSP exists); the loader decides. */
    uint32_t parent_psp_linear;

    /* The command-tail TEXT (NOT counting the leading space, the length byte,
     * or the trailing CR). May be NULL or empty for a no-argument launch.
     * cmd_tail_len is the number of bytes of text (a NUL terminator is NOT
     * required; the length is authoritative). A tail longer than
     * PSP_CMD_TAIL_MAX_TEXT is CLAMPED to PSP_CMD_TAIL_MAX_TEXT (fail-loud:
     * psp_build() returns a non-zero clamp count) -- never an overflow.
     * Ref Sec 2.11. */
    const char *cmd_tail;
    uint32_t    cmd_tail_len;
} psp_params_t;

/* psp_build -- zero-init the 256-byte PSP then populate every field per the
 * App B.2 value map (docs/research/psp-loader-ground-truth.md Sec 2).
 *
 * Returns 0 on success. Returns the number of command-tail bytes that were
 * DROPPED by clamping (> 0) when params->cmd_tail_len exceeds
 * PSP_CMD_TAIL_MAX_TEXT -- the tail is clamped, never overflowed (Rule 2:
 * fail loud, do not silently corrupt past offset 0xFF).
 *
 * Fails LOUD (panic / abort, never a silent no-op) on a NULL psp or NULL
 * params, or on a non-NULL cmd_tail with a non-zero length but a NULL pointer.
 * In the kernel this routes through panic(); hosted it routes through abort().
 *
 * CLAUDE.md Rule 2 (fail loud), Law 2. */
uint32_t psp_build(psp_t *psp, const psp_params_t *params);

#endif /* INITECH_PSP_H */
