/* Ref: ADR-0014 DEC-14-6.1/.3/.5, DEC-14-8; bead initech-qshpn.
 * The resident side of section COLD's Restart / Shut Down / Exit to
 * InitechDOS (os/milton/power_cold.c). Everything named here is DEFINED
 * resident in kmain.c (FLAIR tenant kmain, KMAIN_OVL_IMPORTED) and reached
 * from the overlay through the generated, address-checked import table. */
#ifndef INITECH_POWER_COLD_H
#define INITECH_POWER_COLD_H
#include <stdint.h>
#include "boot_info.h"
#include "console.h"
#include "psp.h"
#include "fat12.h"

/* Resident helpers kept OUT of the overlay on purpose (kmain.c). */
void finder_shutdown_life_check(void);  /* bounded-life halt (kmain's tick)  */
int  flair_exit_teardown(const boot_info_t *bi); /* DEC-14-6.5 hooks + console */
void exitdos_banner(void);              /* the locked banner lines' home     */

/* kmain.c objects the exit-to-DOS hand-off binds or reads. */
void flair_live_tenant_heard(flair_live_ctx_t *ctx, const boot_info_t *bi);
extern psp_t g_kernel_psp;
void int21_exit_hook(uint8_t code);
void int21_setvect_idt(uint8_t vec, uint32_t handler);
uint32_t int21_getvect_idt(uint8_t vec);
extern fat12_volume_t g_absdisk_vol;
int int21_absdisk_read(uint32_t lba, uint32_t count, void *buf);
int int21_absdisk_write(uint32_t lba, uint32_t count, const void *buf);
uint16_t loader_exec_by_name(const char *name83, uint16_t dir_start,
                             const char *cmd_tail, uint32_t cmd_tail_len,
                             uint32_t env_block, uint8_t *out_rc);
uint16_t loader_overlay_by_name(const char *name83, uint16_t dir_start,
                                uint32_t load_addr, uint32_t reloc_factor);
extern void (*g_transient_exec_secondary)(const char *tail, uint32_t len);
void serial_puthex32(uint32_t v);
uint32_t kernel_image_end(void);        /* _kernel_end (a linker symbol) */

/* The private boot-volume read, shared with the overlay manager: COLD looks
 * COMMAND.COM up through this resident wrapper, which also holds the
 * KMAIN_MUT_COMMAND_VOLUME knob (the mutant kernel's own resident object). */
extern fat12_volume_t g_boot_vol;
extern uint8_t g_boot_cluster[4096];
int exitdos_root_find(const char *name83, dir_entry_t *d, uint32_t *v);
#endif
