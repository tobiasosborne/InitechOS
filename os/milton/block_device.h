/* ADR-0003 DEC-17. IBM DOS 3.30 TR pp9-6..9; BIOS TR pp2-58..67.
 * INT 13h uses DEC-04a flat registers, never the FAT volume origin. */
#ifndef INITECH_BLOCK_DEVICE_H
#define INITECH_BLOCK_DEVICE_H
#include "blockdev.h"
#include "idt.h"
#include "diskutil.h"
int block_device_map(unsigned letter, du_map *map);
int block_device_chs(uint16_t cx, uint8_t head, uint32_t sectors, uint32_t *lba);
uint8_t block_device_status(int ata_error);
void block_device_bind(unsigned index, const blockdev_t *dev, uint32_t sectors);
void block_device_mounted(uint8_t drive, uint32_t base, uint32_t sectors,
                         void (*written)(uint32_t, uint32_t, const void *));
/* Fixed C (2) / D (3): successful AH=03h writes inside m's partition are
 * reported volume-relative with the letter (L042: FORMAT/SYS coherence). */
void block_device_watch(unsigned letter, const du_map *m,
                        void (*written)(unsigned, uint32_t, uint32_t, const void *));
void KERNEL_ASM_ABI int13_dispatch(int_frame_t *f);
#endif
