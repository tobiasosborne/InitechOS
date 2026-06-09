/* config_sys.h -- PURE CONFIG.SYS parser for InitechDOS SYSINIT (beads initech-509.2).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): shipped InitechDOS kernel code, freestanding
 * (-ffreestanding -nostdlib), <stdint.h> only, NO I/O and NO malloc -- pure text
 * -> struct transformation over a caller-supplied buffer + caller-supplied output
 * struct. So it compiles BOTH freestanding (the kernel SYSINIT) and HOSTED (the
 * factory oracle test_config_sys.c), exactly like sft.c / psp.c.
 *
 * Ref (Law 1): spec/dos_config_sys_baseline.txt (the LOCKED baseline, Rule 8 --
 *   FILES=20 / BUFFERS=20 / LASTDRIVE=Z / DEVICE=ANSI.SYS / DEVICE=INITNET.SYS /
 *   INSTALL=SHARE.EXE / SHELL=COMMAND.COM /P /E:512); ADR-0003 Sec 5.6 (DEC-06:
 *   FILES= governs the SFT capacity) + Appendix D.2; os/milton/sft.h
 *   (SFT_MAX_ENTRIES=20). DOS 3.3 CONFIG.SYS directive semantics: case-insensitive
 *   keyword=value lines; DOS IGNORES blank lines, ';'-comment lines, and unknown
 *   directives (it does NOT abort the boot on a directive it does not recognize);
 *   numeric ranges are clamped to the DOS-documented bounds. CLAUDE.md Law 2
 *   (oracle is truth), Rule 2 (fail loud on a PROGRAMMER invariant -- NOT on a
 *   user typo in CONFIG.SYS), Rule 11 (deterministic), Rule 12 (ASCII).
 *
 * PERIOD-CORRECT LENIENCY: a malformed/unknown CONFIG.SYS line is NOT a fatal
 * error -- real DOS skips it and keeps booting. The parser therefore NEVER fails
 * the boot on user content; it returns the directives it understood. (The only
 * "fail loud" cases are programmer bugs: a NULL output pointer.)
 */
#ifndef INITECH_MILTON_CONFIG_SYS_H
#define INITECH_MILTON_CONFIG_SYS_H

#include <stdint.h>

/* Bounds on the parsed directive arrays. CONFIG.SYS realistically carries only a
 * handful of DEVICE=/INSTALL= lines; these small fixed arrays avoid malloc (Law 3)
 * and overflow extras are dropped (DOS itself caps loadable drivers). An 8.3 name
 * is at most 12 chars ("NAME.EXT") + NUL = 13; round to 16 for alignment headroom. */
#define CONFIG_SYS_NAME_MAX     16u   /* one 8.3 device/install name + NUL          */
#define CONFIG_SYS_MAX_DEVICES   8u   /* DEVICE= lines retained                      */
#define CONFIG_SYS_MAX_INSTALL   8u   /* INSTALL= lines retained                     */
#define CONFIG_SYS_SHELL_MAX    64u   /* SHELL= command tail (path + switches) + NUL */

/* DOS-documented numeric bounds (the parser clamps into these). FILES: DOS 3.3
 * accepts 8..255; we additionally clamp to SFT_MAX_ENTRIES at APPLY time (sft.h),
 * not here, so the parsed value still reflects what the user wrote. BUFFERS: 1..99
 * (vestigial this release -- recorded, not honored). */
#define CONFIG_SYS_FILES_MIN     8u
#define CONFIG_SYS_FILES_MAX   255u
#define CONFIG_SYS_BUFFERS_MIN   1u
#define CONFIG_SYS_BUFFERS_MAX  99u

/* The parsed CONFIG.SYS. Caller-allocated (BSS / stack); the parser zero-fills it
 * then fills the directives it recognized. Counts say how many array slots are
 * live; `*_present` flags distinguish "directive absent" from "directive present
 * with a default-looking value". */
typedef struct dos_config {
    uint16_t files;            /* FILES=    (clamped to [MIN,MAX]); 0 if absent     */
    uint8_t  files_present;
    uint16_t buffers;          /* BUFFERS=  (clamped); 0 if absent                  */
    uint8_t  buffers_present;
    char     lastdrive;        /* LASTDRIVE= uppercased letter ('A'..'Z'); 0 if absent */
    uint8_t  lastdrive_present;

    uint8_t  device_count;     /* number of DEVICE= names retained                  */
    char     devices[CONFIG_SYS_MAX_DEVICES][CONFIG_SYS_NAME_MAX];

    uint8_t  install_count;    /* number of INSTALL= names retained                 */
    char     install[CONFIG_SYS_MAX_INSTALL][CONFIG_SYS_NAME_MAX];

    char     shell[CONFIG_SYS_SHELL_MAX]; /* SHELL= full tail, e.g. "COMMAND.COM /P /E:512" */
    uint8_t  shell_present;
} dos_config_t;

/* Parse a CONFIG.SYS text buffer (`buf`, `len` bytes; need not be NUL-terminated)
 * into *out. Lenient + case-insensitive: blank lines, ';'-comment lines, and
 * unknown directives are skipped (NOT fatal). Line endings may be LF or CRLF.
 * Returns the number of directives it RECOGNIZED (>= 0). Fails loud (panic/abort)
 * ONLY on out == NULL (a programmer bug, Rule 2); buf == NULL with len 0 is the
 * empty file (out is zeroed, returns 0). Pure (no I/O), so host-testable. */
int config_sys_parse(const char *buf, uint32_t len, dos_config_t *out);

#endif /* INITECH_MILTON_CONFIG_SYS_H */
