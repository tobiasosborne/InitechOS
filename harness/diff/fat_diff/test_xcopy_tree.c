/* Factory oracle bridge: SAME shipped C, REAL INT21/FAT backend. The
 * independent reference is mtools on the identical seed (xcopy_tree.sh).
 * TREE expected text comes ONLY from mdir -/ -b and the IBM printed D-6
 * sample, never from the artifact renderer. ADR-0003 Appendix A. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "xcopy_tree.h"
#include "int21.h"
#include "fileio_fat.h"
#include "blockdev_file.h"
#include "sft.h"
#include "psp.h"

static unsigned char bounce[8192]; /* -no-pie: within the INT21 flat aperture */
static psp_t psp;
static const char *keys;
static int corrupt_verify, verify_reads;
static unsigned char dest_handle[256];
static void sink(char c) { putchar(c); }
static int copy_clock(uint16_t *y, uint8_t *mo, uint8_t *d, uint8_t *h,
                      uint8_t *mi, uint8_t *s, uint8_t *dow)
{
    *y=1988; *mo=2; *d=29; *h=23; *mi=59; *s=58; *dow=1;
    return 1;
}

int du_call(du_regs *r)
{
    int_frame_t f = {0};
    unsigned ah = (r->a >> 8) & 255, al = r->a & 255;
    uintptr_t original = r->d;
    int to = ah == 0x39 || ah == 0x3b || ah == 0x3c || ah == 0x3d ||
             ah == 0x43 || ah == 0x4e;
    if (ah == 7 || ah == 8) {
        if (!*keys) { fputs("ORACLE RED: unexpected keyboard wait\n", stderr); exit(90); }
        r->a = (unsigned char)*keys++;
        /* Reproduce the kernel's default ^C action on AH=08h. AH=07h must
         * reach the program, which owns XCOPY's documented return code 2. */
        if (ah == 8 && r->a == 3) exit(0);
        return 0;
    }
    f.eax = r->a; f.ebx = r->b; f.ecx = r->c;
    f.edx = (uint32_t)r->d; f.esi = (uint32_t)r->s; f.eflags = 0x202;
    if (to) {
        if (strlen((char *)original) >= sizeof(bounce)) exit(91);
        strcpy((char *)bounce, (char *)original);
        f.edx = (uint32_t)(uintptr_t)bounce;
    } else if (ah == 0x3f || ah == 0x40) {
        if (r->c > sizeof(bounce)) exit(92);
        if (ah == 0x40) memcpy(bounce, (void *)original, r->c);
        f.edx = (uint32_t)(uintptr_t)bounce;
    } else if (ah == 0x47) {
        f.esi = (uint32_t)(uintptr_t)bounce;
    }
    int21_dispatch(&f);
    if (!(f.eflags & 1)) {
        if (ah == 0x3f) {
            if (dest_handle[r->b & 255]) {
                verify_reads++;
                if (corrupt_verify && f.eax) bounce[0] ^= 1;
            }
            memcpy((void *)original, bounce, f.eax);
        }
        if (ah == 0x47) strcpy((char *)r->s, (char *)bounce);
        if (ah == 0x3d || ah == 0x3c)
            dest_handle[f.eax & 255] = (unsigned char)(ah == 0x3d && al == 0 &&
                strncmp((char *)original, "\\DST", 4) == 0);
        if (ah == 0x3e) dest_handle[r->b & 255] = 0;
    }
    r->a = f.eax; r->b = f.ebx; r->c = f.ecx; r->d = f.edx;
    return (int)(f.eflags & 1);
}

/* mdir emits ::/SRC/CHILD/ and ::/SRC/FILE.EXT, in directory order.
 * Keep that independent inventory; render its immediate children at each
 * path, then visit the subdirectories. No FAT or artifact helpers here. */
static char entries[256][260];
static int nentries;
static FILE *gold;
static void render(const char *path, int files)
{
    size_t n = strlen(path);
    fprintf(gold, "\r\nPath: ");
    for (const char *p = path; *p; p++) fputc(*p == '/' ? '\\' : *p, gold);
    fputs("\r\n\r\nSub-directories:", gold);
    int count = 0;
    for (int i = 0; i < nentries; i++) {
        char *e = entries[i]; size_t len = strlen(e);
        if (strncmp(e, path, n) || e[n] != '/' || len <= n+1) continue;
        char *leaf = e+n+1, *slash = strchr(leaf, '/');
        if (!slash || slash[1]) continue;
        fprintf(gold, "%s%.*s\r\n", count++ ? "               : " : " ",
                (int)(slash-leaf), leaf);
    }
    if (!count) fputs(" None\r\n", gold);
    if (files) {
        fputs("\r\nFiles:\r\n", gold); count = 0;
        for (int i = 0; i < nentries; i++) {
            char *e = entries[i];
            if (!strncmp(e, path, n) && e[n] == '/' && e[n+1] && !strchr(e+n+1, '/')) {
                fprintf(gold, "        %s\r\n", e+n+1); count++;
            }
        }
        if (!count) fputs("        None\r\n", gold);
    }
    for (int i = 0; i < nentries; i++) {
        char child[260], *e = entries[i]; size_t len = strlen(e);
        if (strncmp(e, path, n) || e[n] != '/' || len <= n+1) continue;
        char *slash = strchr(e+n+1, '/');
        if (!slash || slash[1]) continue;
        memcpy(child, e, len-1); child[len-1] = 0;
        render(child, files);
    }
}

int main(int argc, char **argv)
{
    if (argc == 6 && !strcmp(argv[1], "--tree-golden")) {
        FILE *in = fopen(argv[2], "r");
        gold = fopen(argv[5], "wb");
        if (!in || !gold) return 91;
        char line[270];
        while (fgets(line, sizeof(line), in)) {
            line[strcspn(line, "\r\n")] = 0;
            if (strncmp(line, "::/", 3)) return 92;
            if (nentries == 256) return 93;
            strcpy(entries[nentries++], line+2);
        }
        fclose(in);
        fputs("DIRECTORY PATH LISTING FOR VOLUME TESTVOL\r\n", gold);
        render(argv[3], atoi(argv[4]));
        fclose(gold);
        return 0;
    }
    if (argc < 5 || strcmp(argv[1], "--run")) return 94;
    keys = argc > 5 ? argv[5] : "";
    corrupt_verify = argc > 6;
    blockdev_file_t bf;
    fat12_volume_t vol;
    unsigned char sector[512];
    if (blockdev_file_open_rw(&bf, argv[2]) || fat12_mount(&vol, &bf.dev, sector) ||
        fileio_fat_bind(&vol)) return 95;
    psp_params_t params = {0};
    params.alloc_end_linear = 0x80000;
    psp_build(&psp, &params); sft_init(); int21_set_psp(&psp); int21_set_sink(sink);
    if (getenv("DOS_COPY_CLOCK")) int21_set_clock(copy_clock, NULL);
    const char *working = getenv("ZUVO_CWD");
    if (working) {
        du_regs cd={0x3b00,0,0,(uintptr_t)working,0};
        if (du_call(&cd)) return 97;
    }
    char tail[128];
    if (strlen(argv[4]) >= sizeof(tail)) return 96;
    strcpy(tail, argv[4]);
    int rc = du_main(!strcmp(argv[3], "TREE"), tail);
    blockdev_file_close(&bf);
    fprintf(stderr, "ORACLE RUN: exit=%d verify_reads=%d\n", rc, verify_reads);
    return rc;
}
