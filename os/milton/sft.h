/* sft.h -- InitechDOS System File Table (SFT) + Job File Table (JFT) indirection.
 *
 * beads: initech-509.3 ("JFT(20) + SFT + DUP/DUP2 I/O redirection (DEC-06,
 *        load-bearing)"). The handle layer ALL file/device I/O depends on.
 * Ref:   ADR-0003 Sec 5.6 (DEC-06): "A per-process Job File Table of twenty (20)
 *        entries shall map process file handles to entries in a system-wide
 *        System File Table, the capacity of which shall be governed by the
 *        FILES= directive of CONFIG.SYS." ADR-0003 Appendix D.2 (FILES=20).
 *        DEC-06: "Handles 0 through 4 shall be predefined (standard input,
 *        output, error, auxiliary, printer)." docs/research/
 *        fs-mount-sft-ground-truth.md Sec 3 (the SFT/JFT model, the entry
 *        struct, DUP/DUP2 algorithms). spec/dos_structs.h (psp_t.jft[20] at
 *        offset 0x18; dir_entry_t). DOS 3.3 Programmer's Reference Manual
 *        AH=45h DUP / AH=46h DUP2. CLAUDE.md Law 1 (cite), Law 2 (oracle),
 *        Law 3 (artifact = C), Rule 2 (fail loud), Rule 8 (specs-as-data),
 *        Rule 11 (deterministic), Rule 12 (ASCII).
 *
 * DESIGN STANCE (ADR-0003 Sec 5.5): vestigial structures are implemented IN
 * FULL. Handles 0-4 are ALL predefined per DEC-06 -- including AUX (3) and PRN
 * (4) -- even though no AUX/PRN device driver exists yet (deferred to the device
 * milestone). The JFT/SFT indirection is the real, complete table a DOS process
 * sees; it is not stubbed.
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only, NO
 * I/O. Pure table manipulation, so it compiles BOTH freestanding (the kernel)
 * and HOSTED (the factory oracle, test_sft.c) -- fully host-unit-testable.
 */
#ifndef INITECH_SFT_H
#define INITECH_SFT_H

#include <stdint.h>
#include "psp.h"   /* psp_t (jft[20]); pulls dos_structs.h (dir_entry_t) */

/* FILES=20 (ADR-0003 Appendix D.2 CONFIG.SYS baseline) governs the SFT capacity
 * AND the JFT width (DEC-06: a 20-entry JFT). */
#define SFT_MAX_ENTRIES   20u
#define JFT_MAX_ENTRIES   20u

/* JFT sentinel: a process handle slot that is closed / unused. Matches the
 * real-DOS 0xFF sentinel psp.c writes into jft[] (psp.c:131-136). */
#define JFT_CLOSED        0xFFu

/* Predefined SFT slots for the standard devices (handles 0-4 map here). Slots
 * 0..3 are reserved for devices at sft_init(); file opens use slots 4..19. */
#define SFT_SLOT_CON_IN   0u   /* stdin  (CON, read)                 */
#define SFT_SLOT_CON_OUT  1u   /* stdout + stderr (CON, write)       */
#define SFT_SLOT_AUX      2u   /* AUX  (COM1)                        */
#define SFT_SLOT_PRN      3u   /* PRN  (LPT1)                        */
#define SFT_FIRST_FILE    4u   /* lowest SFT slot a file open may use */

/* DOS error codes returned by the table operations (0 == success). These are
 * the standard DOS 3.3 INT 21h codes; int21.c sets AX = the returned code on a
 * CF=1 error. Ref: DOS 3.3 Programmer's Reference Manual error returns. */
#define SFT_OK                   0u
#define SFT_ERR_INVALID_HANDLE   0x0006u  /* bad/closed handle (DOS code 6)   */
#define SFT_ERR_TOO_MANY_OPEN    0x0004u  /* no free JFT/SFT slot (DOS code 4) */

/* What an SFT entry is backed by. */
typedef enum sft_kind {
    SFT_KIND_FREE   = 0,   /* slot unused                          */
    SFT_KIND_DEVICE = 1,   /* a character device (CON / AUX / PRN)  */
    SFT_KIND_FILE   = 2    /* an open file on the mounted volume    */
} sft_kind_t;

/* The predefined character devices. */
typedef enum sft_dev_id {
    SFT_DEV_CON = 0,   /* CON: keyboard in / screen out */
    SFT_DEV_AUX = 1,   /* AUX: COM1                     */
    SFT_DEV_PRN = 2    /* PRN: LPT1                     */
} sft_dev_id_t;

/* Open mode (AL from OPEN/CREAT; the device entries carry a nominal mode). */
#define SFT_MODE_READ   0u
#define SFT_MODE_WRITE  1u
#define SFT_MODE_RDWR   2u

/* One system-wide open-file (or device) registry entry. The JFT maps a process
 * handle (0..19) to an index into g_sft[]. ref_count is the number of JFT slots
 * (across DUP/DUP2) pointing here; when it falls to zero the slot is freed.
 *
 * Ref: fs-mount-sft-ground-truth.md Sec 3.2 (the recommended struct). The FILE
 * fields (dir_entry/file_offset/file_data) are populated by AH=3Dh OPEN in the
 * NEXT step (initech-509.5 read-side); this milestone establishes the table +
 * the device entries + DUP/DUP2, so file_data stays NULL here. */
typedef struct sft_entry {
    uint8_t      kind;          /* sft_kind_t                                  */
    uint8_t      open_mode;     /* SFT_MODE_*                                  */
    uint8_t      dev_id;        /* sft_dev_id_t (valid when kind == DEVICE)    */
    uint16_t     ref_count;     /* JFT references to this slot (DUP semantics) */
    /* --- FILE state (valid when kind == SFT_KIND_FILE; set by OPEN, 509.5) - */
    dir_entry_t  dir_entry;     /* copy of the 32-byte FAT dir entry at open   */
    uint32_t     file_offset;   /* current byte offset (READ/WRITE/LSEEK move) */
    const uint8_t *file_data;   /* whole-file buffer pointer (milestone, 509.5)*/
} sft_entry_t;

/* The kernel-global System File Table. Zero-initialised by the C runtime
 * (BSS) -> every slot starts SFT_KIND_FREE. sft_init() populates 0..3. */
extern sft_entry_t g_sft[SFT_MAX_ENTRIES];

/* Establish the predefined device entries (slots 0..3): CON-read, CON-write
 * (shared by stdout+stderr), AUX, PRN. Idempotent (re-zeroes the whole table
 * first). Called once at SYSINIT before the first process loads. The matching
 * per-process JFT (jft[0]=0,jft[1]=1,jft[2]=1,jft[3]=2,jft[4]=3,rest=0xFF) is
 * laid down by psp_build (psp.c). Ref: fs-mount-sft-ground-truth.md Sec 3.3. */
void sft_init(void);

/* Resolve a process handle (JFT index, 0..19) to its SFT entry, or NULL if the
 * handle is out of range / closed (jft==0xFF) / the psp is NULL. Fails LOUD
 * (panic/abort) on a CORRUPT JFT entry -- one that is neither the 0xFF closed
 * sentinel nor a valid in-range index of a live (non-FREE) SFT slot (Rule 2:
 * an invariant violation is never papered over). */
sft_entry_t *sft_from_handle(const psp_t *psp, uint8_t handle);

/* Lowest free JFT slot (jft[i]==0xFF) in 0..19, or JFT_CLOSED (0xFF) if full. */
uint8_t jft_alloc(const psp_t *psp);

/* Lowest free SFT slot in SFT_FIRST_FILE..19 (files never reuse the device
 * slots 0..3), or SFT_MAX_ENTRIES if the table is full. */
uint8_t sft_alloc(void);

/* AH=45h DUP: duplicate the open handle `src` into the lowest free JFT slot;
 * the new slot aliases the SAME SFT entry (ref_count++). On success returns
 * SFT_OK and writes the new handle to *out_handle. Errors: SFT_ERR_INVALID_HANDLE
 * (src closed/bad), SFT_ERR_TOO_MANY_OPEN (no free JFT slot). out_handle is
 * untouched on error. Ref: fs-mount-sft-ground-truth.md Sec 3.4. */
uint16_t sft_dup(psp_t *psp, uint8_t src, uint8_t *out_handle);

/* AH=46h DUP2: force handle `dst` to alias the open handle `src`. If `dst` is
 * currently open it is released first (ref_count--, freed at zero); then
 * jft[dst] = jft[src] and the shared SFT entry's ref_count++. src==dst is a
 * success no-op (DOS convention). Errors: SFT_ERR_INVALID_HANDLE (src
 * closed/bad, or dst out of range). This is the I/O-redirection primitive:
 * DUP2(src=file_handle, dst=1) repoints stdout at the file's SFT slot.
 * Ref: fs-mount-sft-ground-truth.md Sec 3.4. */
uint16_t sft_dup2(psp_t *psp, uint8_t src, uint8_t dst);

#endif /* INITECH_SFT_H */
