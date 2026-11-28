/* Pure CONFIG.SYS text reader. Ground truth: genuine DOS 3.30 CONFIG
 * boot captures (initech-hp0tb), with directive-specific token termination.
 * Invalid lines leave their settings untouched and can report a diagnostic.
 * Recognizing syntax does not imply an unsupported directive is applied. */
#ifndef INITECH_INITECHDOS_CONFIG_SYS_H
#define INITECH_INITECHDOS_CONFIG_SYS_H

#include <stdint.h>

/* Bounds on the parsed directive arrays. CONFIG.SYS realistically carries only a
 * handful of DEVICE=/INSTALL= lines; these small fixed arrays avoid malloc (The constitution)
 * and overflow extras are dropped (DOS itself caps loadable drivers). An 8.3 name
 * is at most 12 chars ("NAME.EXT") + NUL = 13; round to 16 for alignment headroom. */
#define CONFIG_SYS_NAME_MAX     16u   /* one 8.3 device/install name + NUL          */
#define CONFIG_SYS_MAX_DEVICES   8u   /* DEVICE= lines retained                      */
#define CONFIG_SYS_MAX_INSTALL   8u   /* INSTALL= lines retained                     */
#define CONFIG_SYS_SHELL_MAX    64u   /* SHELL= command tail (path + switches) + NUL */

/* DOS numeric token bounds. Invalid values are refused, not clamped. SYSINIT
 * separately refuses sizes beyond its physical SFT, and BUFFERS requests
 * because a configurable disk cache is not implemented. */
#define CONFIG_SYS_FILES_MIN     5u
#define CONFIG_SYS_FILES_MAX   255u
#define CONFIG_SYS_BUFFERS_MIN   1u
#define CONFIG_SYS_BUFFERS_MAX  99u

/* The parsed CONFIG.SYS. Caller-allocated (BSS / stack); the parser zero-fills it
 * then fills the directives it recognized. Counts say how many array slots are
 * live; `*_present` flags distinguish "directive absent" from "directive present
 * with a default-looking value". */
typedef struct dos_config {
    uint16_t files;            /* FILES= requested value; 0 if absent             */
    uint8_t  files_present;
    uint16_t buffers;          /* BUFFERS= requested value; 0 if absent            */
    uint8_t  buffers_present;
    char     lastdrive;        /* LASTDRIVE= uppercased letter ('A'..'Z'); 0 if absent */
    uint8_t  lastdrive_present;

    uint8_t  device_count;     /* number of DEVICE= names retained                  */
    char     devices[CONFIG_SYS_MAX_DEVICES][CONFIG_SYS_NAME_MAX];
    uint8_t  device_has_args[CONFIG_SYS_MAX_DEVICES];

    uint8_t  install_count;    /* number of INSTALL= names retained                 */
    char     install[CONFIG_SYS_MAX_INSTALL][CONFIG_SYS_NAME_MAX];

    char     shell[CONFIG_SYS_SHELL_MAX]; /* SHELL= full tail, e.g. "COMMAND.COM /P /E:512" */
    uint8_t  shell_present;

    /* BREAK=ON|OFF sets the flag queried by AH=33h. The boot default is OFF. */
    uint8_t  break_on;         /* parsed BREAK= state (1=ON, 0=OFF); 0 if absent */
    uint8_t  break_present;    /* 1 iff a well-formed BREAK=ON|OFF line was seen  */
} dos_config_t;

/* Parse a CONFIG.SYS text buffer (`buf`, `len` bytes; need not be NUL-terminated)
 * into *out. Lenient + case-insensitive: blank lines, ';'-comment lines, and
 * unknown directives are skipped (NOT fatal). Line endings may be LF or CRLF.
 * Returns the number of directives it RECOGNIZED (>= 0). Fails loud (panic/abort)
 * ONLY on out == NULL (a programmer bug, The constitution); buf == NULL with len 0 is the
 * empty file (out is zeroed, returns 0). Pure (no I/O), so host-testable. */
int config_sys_parse(const char *buf, uint32_t len, dos_config_t *out);

/* Report syntax failures without aborting CONFIG parsing. */
typedef void (*config_sys_diagnostic_fn)(void *context);
int config_sys_parse_report(const char *buf, uint32_t len, dos_config_t *out,
                            config_sys_diagnostic_fn report, void *context);

#endif /* INITECH_INITECHDOS_CONFIG_SYS_H */
