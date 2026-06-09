/* sysinit.h -- InitechDOS SYSINIT: the named, ordered system bring-up contract
 * (beads initech-509.2).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): shipped InitechDOS kernel code, freestanding.
 * This extracts the ad-hoc init that was scattered through kmain.c kernel_main()
 * into NAMED, DOCUMENTED phases -- a single init contract with a clear hook point
 * for CONFIG.SYS (sysinit_apply_config). It is a RELOCATION, not a re-ordering:
 * the phases run in EXACTLY the order kernel_main ran them, and kernel_main still
 * calls them at the same points (the boot path is behaviorally unchanged).
 *
 * THE ORDERING MINEFIELD (kmain.c ~line 832 / CLAUDE.md "real mode -> protected
 * is a minefield" / Rule 5): the interrupt + device bring-up order is load-bearing
 *   1. pic_remap_and_mask  (move the 8259 away from the CPU exception vectors)
 *   2. idt_init            (256 gates; exceptions 0-31 as fail-loud panics)
 *   3. int21 sink + exit hook bound BEFORE any `int 0x21`
 *   4. sft_init + kernel PSP + int21_set_psp (valid standard handles before a load)
 *   5. install the 0x21 / 0x20 trap gates
 *   ... (console, FAT mount, CONFIG.SYS apply, program demos) ...
 *   6. program the PIT + keyboard, install the IRQ gates, UNMASK, then `sti`
 *      -- devices + gates BEFORE unmask, unmask BEFORE sti, so a line can never
 *      fire into an uninstalled gate. This relative order is PRESERVED here.
 *
 * The phases that depend on kernel_main locals (the boot_info, the live console,
 * the mounted FAT volume) stay in kernel_main; the phases that operate purely on
 * kernel globals + the int21 glue are what this header names. The CONFIG.SYS phase
 * (sysinit_apply_config) is the new behavior; everything else is relocation.
 *
 * Ref (Law 1): kmain.c kernel_main (the original ordered sequence); ADR-0003
 *   DEC-06 (FILES= governs the SFT capacity); os/milton/config_sys.h; os/milton/
 *   sft.h. Rule 2 (fail loud), Rule 11 (deterministic), Rule 12 (ASCII).
 */
#ifndef INITECH_MILTON_SYSINIT_H
#define INITECH_MILTON_SYSINIT_H

#include <stdint.h>
#include "psp.h"     /* psp_t */
#include "int21.h"   /* int21_sink_fn / int21_exit_fn */
#include "fat12.h"   /* fat12_volume_t (the CONFIG.SYS source) */

/* A serial-output sink so a SYSINIT phase can emit the SAME boot markers
 * kernel_main emitted (PIC / IDT / INT21 / IRQ-LIVE / SYSINIT: ...), keeping the
 * serial capture byte-for-byte identical. kernel_main passes its serial_puts. */
typedef void (*sysinit_serial_fn)(const char *s);

/* PHASE 1 -- interrupt + syscall foundation (kmain.c original lines 431-467, IN
 * ORDER): pic_remap_and_mask -> "PIC" -> idt_init -> "IDT" -> bind int21 sink +
 * exit hook -> sft_init -> build the kernel PSP into *kernel_psp -> int21_set_psp
 * -> install the 0x21 + 0x20 trap gates -> "INT21". `sink` and `exit_hook` are the
 * kernel's CON glue; `kernel_psp` is the kernel-context PSP (BSS, outlives the
 * call). `serial` emits the markers. NO `sti` here (IF stays 0 this phase). */
void sysinit_early(int21_sink_fn sink, int21_exit_fn exit_hook,
                   psp_t *kernel_psp, sysinit_serial_fn serial);

/* PHASE 2 -- CONFIG.SYS apply (the NEW behavior + the hook point, beads
 * initech-509.2). If `vol` is non-NULL and CONFIG.SYS exists on it, read + parse
 * it; otherwise fall back to the LOCKED baseline (spec/dos_config_sys_baseline.txt,
 * embedded). Apply the honorable directives: FILES=N -> sft_set_files_limit (the
 * one with teeth); SHELL= recorded; DEVICE/INSTALL/BUFFERS/LASTDRIVE recorded +
 * logged "accepted(deferred)". Emits a one-line "SYSINIT: ..." serial summary the
 * in-emulator oracle asserts. `sector_buf`/`fat_buf` are caller scratch (>=512 /
 * >= a FAT image) so this stays malloc-free (Law 3). Called AFTER the FAT bind,
 * BEFORE the program demos. Returns the effective FILES= cap that was applied. */
uint8_t sysinit_apply_config(const fat12_volume_t *vol, void *sector_buf,
                             void *fat_buf, uint32_t fat_buf_len,
                             sysinit_serial_fn serial);

/* PHASE 3 -- enable hardware interrupts (kmain.c original lines 844-850, IN
 * ORDER, the minefield tail): pit_init -> kbd_init -> install IRQ0 gate (0x28) ->
 * install IRQ1 gate (0x29) -> pic_unmask_irq0_irq1 -> `sti` -> "IRQ-LIVE". Devices
 * + gates BEFORE unmask; unmask BEFORE sti. This is the FIRST `sti` of the boot. */
void sysinit_enable_irqs(sysinit_serial_fn serial);

#endif /* INITECH_MILTON_SYSINIT_H */
