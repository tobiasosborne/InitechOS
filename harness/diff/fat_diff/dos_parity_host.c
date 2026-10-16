/* Factory oracle for dwbl. IBM DOS 3.30 Technical Reference pp.5-12/13,
 * 6-119/122/136/139; ADR-0003 Appendix A. Drive the REAL dispatcher/backend;
 * dos_parity.sh compares mdir output to independently dated mcopy -m files. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "int21.h"
#include "psp.h"
#include "sft.h"
#include "fileio_fat.h"
#include "blockdev_file.h"

/* -no-pie keeps these DOS flat pointers below 4 GiB on the hosted build. */
static psp_t process;
static char path[128], data[8] = "stamp\r\n";
static int later;
static int clock_get(uint16_t *y, uint8_t *mo, uint8_t *d, uint8_t *h,
                     uint8_t *mi, uint8_t *s, uint8_t *dow)
{
    *y = later ? 1988 : 1987; *mo = later ? 2 : 7; *d = later ? 29 : 6;
    *h = later ? 23 : 13; *mi = later ? 59 : 30;
    *s = later ? 59 : 58; *dow = 1;
    return 1;
}
static unsigned call(unsigned ax, unsigned bx, unsigned cx, const char *name)
{
    int_frame_t f;
    memset(&f, 0, sizeof f);
    if (name) snprintf(path, sizeof path, "%s", name);
    f.eax = ax; f.ebx = bx; f.ecx = cx;
    f.edx = (uint32_t)(uintptr_t)(ax == 0x4000 ? data : path);
    int21_dispatch(&f);
    if (f.eflags & 1) {
        fprintf(stderr, "STAMP RED: AH=%02x AX=%04x %s\n", ax >> 8,
                (unsigned)f.eax, name ? name : "");
        exit(1);
    }
    return f.eax;
}
static void create(const char *name, int write)
{
    unsigned h = call(0x3c00, 0, 0, name);
    if (write) call(0x4000, h, 7, NULL);
    call(0x3e00, h, 0, NULL);
}
static dir_entry_t entry(const char *image, const char *name)
{
    blockdev_file_t disk;
    fat12_volume_t vol;
    fat12_dir_t dir;
    dir_entry_t result;
    uint8_t sector[512], fat[6144];
    char normalized[128];
    snprintf(normalized, sizeof normalized, "%s", name);
    for (char *p=normalized; *p; p++) if (*p=='/') *p='\\';
    if (blockdev_file_open(&disk, image) || fat12_mount(&vol, &disk.dev, sector) ||
        fat12_read_fat(&vol, fat, sizeof fat) ||
        fat12_resolve_path(&vol, sector, fat, sizeof fat, normalized, &dir, &result)) {
        fprintf(stderr, "STAMP RED: cannot resolve %s\n", name); exit(1);
    }
    blockdev_file_close(&disk);
    return result;
}
int main(int argc, char **argv)
{
    blockdev_file_t disk;
    fat12_volume_t vol;
    uint8_t sector[512];
    psp_params_t params;
    if (argc == 6 && !strcmp(argv[1], "--compare")) {
        dir_entry_t actual = entry(argv[2], argv[3]);
        dir_entry_t reference = entry(argv[4], argv[5]);
        if (actual.mtime != reference.mtime || actual.mdate != reference.mdate) {
            fprintf(stderr, "STAMP RED: %s raw %04x/%04x != mcopy %04x/%04x\n",
                    argv[3], actual.mtime, actual.mdate, reference.mtime, reference.mdate);
            return 1;
        }
        printf("STAMP WORDS: %s %04x/%04x == mcopy (two-second resolution)\n",
               argv[3], actual.mtime, actual.mdate);
        return 0;
    }
    if (argc != 2) return 2;
    memset(&params, 0, sizeof params);
    params.alloc_end_linear = 0x70000;
    if (blockdev_file_open_rw(&disk, argv[1]) ||
        fat12_mount(&vol, &disk.dev, sector) || fileio_fat_bind(&vol)) return 2;
    psp_build(&process, &params); sft_init(); int21_set_psp(&process);
    int21_set_clock(clock_get, NULL);
    create("NEW.TXT", 0);
    create("OLD.TXT", 1);
    create("TRUNC.TXT", 1);
    call(0x3900, 0, 0, "SUB");
    create("SUB\\CHILD.TXT", 1);
    later = 1;
    unsigned h = call(0x3d02, 0, 0, "OLD.TXT");
    call(0x4000, h, 7, NULL); call(0x3e00, h, 0, NULL);
    create("TRUNC.TXT", 0);
    /* Read-only open/close must preserve the creation stamp. */
    h = call(0x3d00, 0, 0, "NEW.TXT"); call(0x3e00, h, 0, NULL);
    blockdev_file_close(&disk);
    puts("STAMP HOST: real INT21 create/write/truncate/mkdir/read-close completed");
    return 0;
}
