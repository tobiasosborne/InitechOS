/* ibase_install.h -- how an emulator harness puts IBASE on a FAT image
 * (InitechBase overlays Phase B, initech-5etom.1; docs/design/
 * ibase-overlays.md Sec 7).
 *
 * IBASE ships as two files, INITBASE.EXE (the InitechMZ resident) and INITBASE.OVL
 * (its overlays), like DBASE.EXE beside DBASE.OVL. Every build -- the shipped
 * one and every mutant -- writes the pair side by side (Makefile ibase_pack:
 * X.EXE and X.OVL), so a harness is handed the .EXE path and derives the
 * overlay from it. Both land in the root as INITBASE.EXE / INITBASE.OVL whatever
 * the build-side names were.
 *
 * Factory C (Law 3). ASCII-clean (Rule 12).
 */
#ifndef IBASE_INSTALL_H
#define IBASE_INSTALL_H
#include <stdio.h>
#include <string.h>

/* `ovl` = `exe` with its extension replaced by .OVL. Returns 0, or -1 when
 * `exe` does not end in .EXE (a harness handed a stale .COM fails loud). */
static __attribute__((unused)) int ibase_ovl_path(const char *exe, char *ovl, size_t cap)
{
    size_t n = strlen(exe);
    if (n < 4 || n + 1 > cap || strcmp(exe + n - 4, ".EXE")) return -1;
    memcpy(ovl, exe, n - 4);
    memcpy(ovl + n - 4, ".OVL", 5);
    return 0;
}

/* Write "mcopy -o -i IMG EXE ::INITBASE.EXE && mcopy -o -i IMG OVL ::INITBASE.OVL"
 * into cmd. Returns 0, or -1 (bad name / no room). */
static __attribute__((unused)) int ibase_install_cmd(char *cmd, size_t cap, const char *img, const char *exe)
{
    char ovl[1024];
    int n;
    if (ibase_ovl_path(exe, ovl, sizeof ovl)) return -1;
    n = snprintf(cmd, cap, "mcopy -o -i %s %s ::INITBASE.EXE && mcopy -o -i %s %s ::INITBASE.OVL",
                 img, exe, img, ovl);
    return n > 0 && (size_t)n < cap ? 0 : -1;
}
#endif
