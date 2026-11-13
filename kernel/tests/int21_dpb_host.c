/* Factory driver: real INT21 AH=1Fh/32h over the real FAT backend on the
 * DEC-18 map: A = argv[1] (raw FAT), C/D = primaries 1/2 of argv[2]. It only
 * PRINTS what the artifact returned; it computes no expected value. The
 * golden is independent: int21_dpb.sh derives every DPB field from minfo /
 * mdir of the same images (docs/research/dpb-ground-truth.md Sec 3/5).
 * Line format, shared with the emulator program int21_dpb_program.asm:
 *   DPB <AH> <DL> <AL> <CF> <64 hex digits | - > <next drive letter | END | ->
 * An AL=FFh line prints "-" for the DPB only if EBX came back untouched.
 * Ref: ADR-0003 Appendix A (0Fh-24h, 32h) + B.4; WL-0167. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "int21.h"
#include "psp.h"
#include "sft.h"
#include "fileio_fat.h"
#include "blockdev_file.h"

#define EBX_SENTINEL 0xDEADBEEFu
static psp_t process;

static void query(unsigned ah, unsigned dl)
{
    int_frame_t f;
    memset(&f, 0, sizeof f);
    f.eax = ah << 8; f.edx = dl; f.ebx = EBX_SENTINEL; f.eflags = 0x202u;
    int21_dispatch(&f);
    unsigned al = f.eax & 0xFFu;
    printf("DPB %02X %02X %02X %u ", ah, dl, al, (unsigned)(f.eflags & 1u));
    if (al != 0u) {
        printf("%s -\n", f.ebx == EBX_SENTINEL ? "-" : "CLOBBERED");
        return;
    }
    /* Non-PIE (DOS_VOLUMES_FLAGS -no-pie): the flat EBX is a real address. */
    const uint8_t *d = (const uint8_t *)(uintptr_t)f.ebx;
    for (unsigned i = 0; i < 32u; i++) printf("%02X", d[i]);
    uint32_t next = (uint32_t)d[0x18] | (uint32_t)d[0x19] << 8 |
                    (uint32_t)d[0x1A] << 16 | (uint32_t)d[0x1B] << 24;
    if (next == 0xFFFFFFFFu) printf(" END\n");
    else printf(" %c\n", 'A' + *(const uint8_t *)(uintptr_t)next);
}

static void select_drive(unsigned dl)
{
    int_frame_t f;
    memset(&f, 0, sizeof f);
    f.eax = 0x0E00u; f.edx = dl;
    int21_dispatch(&f);
}

int main(int argc, char **argv)
{
    blockdev_file_t disks[2]; fat12_volume_t vols[3]; uint8_t sector[512];
    if (argc != 3) return 2;
    for (unsigned i = 0; i < 2u; i++)
        if (blockdev_file_open(disks + i, argv[i + 1])) {
            fprintf(stderr, "RED int21-dpb OPEN %s\n", argv[i + 1]);
            return 1;
        }
    if (fat12_mount(vols, &disks[0].dev, sector) ||
        fat12_mount_primary(vols + 1, &disks[1].dev, sector, 1) ||
        fat12_mount_primary(vols + 2, &disks[1].dev, sector, 2)) {
        fprintf(stderr, "RED int21-dpb MOUNT A/C/D\n");
        return 1;
    }
    if (fileio_fat_bind(vols) || fileio_fat_add_drive(2, vols + 1) ||
        fileio_fat_add_drive(3, vols + 2)) {
        fprintf(stderr, "RED int21-dpb BIND A/C/D\n");
        return 1;
    }
    psp_params_t p = {0}; p.alloc_end_linear = 0x70000;
    if (psp_build(&process, &p)) return 1;
    sft_init(); int21_set_psp(&process);
    /* The fixed query script (int21_dpb.sh EXPECT, same order as the .asm). */
    select_drive(0);
    query(0x1F, 0); query(0x32, 0); query(0x32, 1); query(0x32, 2);
    query(0x32, 3); query(0x32, 4); query(0x32, 5); query(0x32, 0x1B);
    select_drive(2);
    query(0x1F, 0); query(0x32, 0);
    select_drive(0);
    for (unsigned i = 0; i < 2u; i++) blockdev_file_close(disks + i);
    return 0;
}
