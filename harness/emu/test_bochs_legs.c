/* Factory inventory oracle, written before the cqej second-half legs and
 * re-keyed by WL-0170 to the video path Bochs 2.7 really takes (0101h on the
 * stock ROM; the forced mode-13h legs on an ISA Cirrus / a plain VGA).
 * Independent roster: WL-0131 Bochs row, WL-0140..0155, WL-0170 and the lane
 * brief. PRD Sec 8 / Rule 5 require named legs in the executed vector. These
 * are BOOT boundaries, not input/disk-operation acceptance (bochs.h); the
 * pixel grade is a separate named leg (bochs_legs.mk bochs-pixel-leg).
 * IBM DOS 3.30 TR p.1-7 (PDF24) grounds the preserved system-file chain;
 * ADR-0003 Appendix A grounds the existing DOS gates, not this wiring test.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct leg { const char *name, *image, *data, *end, *video, *checks; };
#define DATA "build/flair_data.img"
#define TRACER "build/tracer_secondary.img"
#define FLAIR_CHECKS "FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC"
#define SHELL_CHECKS "IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC"
#define GUARD_LINE "'PANIC flair-desktop: LFB smaller than 640x480 w=320 h=200' HALTED"
#define FLAIR(n) { n, "build/flair_tenants.img", DATA, "FLAIR-LIVE-READY", "vbe101", FLAIR_CHECKS }
#define WORD(n) { n, "build/flair_tenants_mut_iword_gate.img", DATA, "FLAIR-LIVE-READY", "vbe101", FLAIR_CHECKS }
#define CP(n) { "flair-cpanels" n, "build/flair_tenants_mut_cpanels_gate.img", DATA, "FLAIR-LIVE-READY", "vbe101", FLAIR_CHECKS }
#define DOS(n) { n, TRACER, DATA, "SHELL-READY", "vbe101", SHELL_CHECKS }
#define DML(n) DOS("samir-dml-" n "-emu")
static const struct leg legs[] = {
    /* ADR-0009 DEC-08 / initech-hdlb: SAMIR input and pixels, beyond boot. */
    { "samir-boot", TRACER, "build/samir_list.img", "SHELL-READY", "vbe101", SHELL_CHECKS },
    { "command-file-emu", "build/command-file/fragment.img", DATA, "FLAIR-LIVE-READY", "vbe101", FLAIR_CHECKS },
    FLAIR("command-file-negative"),
    /* The DAC row: the FORCE_VBE_101 gate IO.SYS pads the scanline to 648. */
    { "flair-desktop-8bpp", "build/vesa8/flagship.img", DATA, "FLAIR-LIVE-READY", "vbe101pad", FLAIR_CHECKS },
    WORD("flair-notepad"), WORD("flair-scrapbook"),
    WORD("flair-notepad-itext"), WORD("flair-notepad-restart"),
    { "flair-notepad-corrupt", "build/flair_tenants_mut_iword_gate.img",
      "build/da_corrupt_source.img", "FLAIR-LIVE-READY", "vbe101", FLAIR_CHECKS },
    CP(""), CP("-date"), CP("-patterns"), CP("-mouse"), CP("-sound"),
    CP("-memory"), CP("-settings"), CP("-persist"),
    { "flair-cpanels-corrupt", "build/flair_tenants_mut_cpanels_gate.img",
      "build/cp_corrupt_data.img", "FLAIR-LIVE-READY", "vbe101", FLAIR_CHECKS },
    FLAIR("flair-grow-box"), DOS("block-device-emu"), DOS("fdisk-emu"),
    DOS("formatsys-emu"), DOS("format-system-boot"), FLAIR("dos-folder"),
    DOS("conkeys"), DOS("j1lu-control-emu"),
    { "ansi-emu", TRACER, "build/ansi_probe_data.img", "SHELL-READY", "vbe101", SHELL_CHECKS },
    FLAIR("flair-finder-icons"), FLAIR("flair-trash-full"),
    FLAIR("flair-toolbox-hardening"), DOS("fat-hygiene-emu"),
    /* WL-0162: the four legs re-keyed from hand-rolled ENOMODE recipes. */
    { "flair-disk-windows", "build/flair_tenants.img", DATA, "FLAIR-LIVE-READY", "vbe101",
      "FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK DESKTOP-DB-CREATE TRASH-CREATE " "FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC" },
    { "flair-file-ops", "build/flair_tenants.img", DATA, "FLAIR-LIVE-READY", "vbe101",
      "FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK DESKTOP-DB-CREATE TRASH-CREATE " "FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC" },
    { "flair-trash-window", "build/flair_tenants.img", DATA, "FLAIR-LIVE-READY", "vbe101",
      "FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK DESKTOP-DB-CREATE TRASH-CREATE " "FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC" },
    /* initech-buze: the R3.2 DesktopMgr leg, re-keyed from the dead ENOMODE
     * claim to the flagship on the pristine volume (+ its pixel twin, PIXEL). */
    { "flair-desktop-icons", "build/flair_tenants.img", DATA, "FLAIR-LIVE-READY", "vbe101",
      "FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK DESKTOP-DB-CREATE TRASH-CREATE " "FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC" },
    { "flair-alert-chrome", "build/flair_tenants_mut_i123_file_gate.img",
      "build/flair_i123_quit_fixture.img", "FLAIR-LIVE-READY", "vbe101", FLAIR_CHECKS },
    FLAIR("trash-hidden-emu"),
    { "ata-no-drive-emu", TRACER, NULL, "SHELL-READY", "vbe101",
      "'FAT-MOUNT-FAIL rc=-1 base_lba=0 hidden_sectors=0' " SHELL_CHECKS },
    { "baked-exec-guard", "build/baked_guard.img", "build/fat_exec.img", "EXEC-4B-END", "vbe101",
      "IRQ-LIVE CONIN-LIVE EXEC-4B-END !PANIC" },
    { "mzload-entry-emu", "build/mzexec_boot.img", "build/fat_mzexec.img", "MZEXEC-END", "vbe101",
      "IRQ-LIVE CONIN-LIVE MZEXEC-END !PANIC" },
    DOS("samir-core-emu"), DOS("samir-dml-emu"),
    DML("clear"), DML("macro"), DML("close"), DML("index"), DML("reindex"),
    DML("sort"), DML("total"), DML("append"), DML("save"), DML("restore"),
    DML("copyfile"), DML("type"), DML("rename"), DML("erase"), DML("label"),
    DML("run"),     /* initech-n8q8: back in SAMIR_DML_CASES */
    DOS("samir-print-emu"), DOS("shell-files-emu"), DOS("command-c-emu"),
    DOS("xcopy-cwd-emu"), DOS("fat-truncate-emu"), DOS("dir-match-emu"),
    DOS("shell-dir-look"),
    WORD("flair-iword-codes"), WORD("flair-iword-block"), WORD("flair-iword-format"),
    WORD("flair-iword-fkeys"), WORD("flair-iword-search"),
    WORD("flair-iword-replace"), WORD("flair-iword-undo"), WORD("flair-iword-setup"),
    FLAIR("kernel-size-r4-emu"),
    /* WL-0170: the forced mode-13h legs (ISA Cirrus = VBE-ENOMODE, plain VGA =
     * VBE-E00); the 640x480 guard is the FLAIR tooth there. */
    { "boot-vga13", TRACER, DATA, "SHELL-READY", "vga13", SHELL_CHECKS },
    { "boot-novbe", TRACER, DATA, "SHELL-READY", "e00", SHELL_CHECKS },
    { "flair-desktop-vga13", "build/flair_desktop.img", NULL, "HALTED", "vga13",
      "FLAIR-HEAP-OK " GUARD_LINE },
    { "flair-appswitch-vga13", "build/flair_tenants.img", DATA, "HALTED", "vga13",
      "FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK " GUARD_LINE },
    { "flair-desktop-novbe", "build/flair_desktop.img", NULL, "HALTED", "e00",
      "FLAIR-HEAP-OK " GUARD_LINE }
};

static void require(int ok, const char *rule, const char *name)
{
    if (!ok) {
        fprintf(stderr, "RED bochs-legs %s: %s\n", rule, name);
        exit(1);
    }
}

static char *read_all(const char *path)
{
    FILE *f = fopen(path, "rb");
    long n;
    char *s;
    if (!f || fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET)) exit(2);
    s = malloc((size_t)n + 1);
    if (!s || fread(s, 1, (size_t)n, f) != (size_t)n) exit(2);
    s[n] = 0;
    fclose(f);
    return s;
}

int main(int argc, char **argv)
{
    size_t count = sizeof legs / sizeof legs[0];
    char *db, *vector, *end;
    if (argc == 2 && strcmp(argv[1], "--list") == 0) {
        for (size_t i = 0; i < count; i++) printf("test-%s-bochs\n", legs[i].name);
        return 0;
    }
    if (argc != 2) return 2;
    db = read_all(argv[1]);
    vector = strstr(db, "\nTEST_EMU_GATES := ");
    require(vector != NULL, "VECTOR", "TEST_EMU_GATES missing");
    vector += strlen("\nTEST_EMU_GATES := ");
    end = strchr(vector, '\n');
    require(end != NULL, "VECTOR", "unterminated vector");
    /* Copy the vector: target stanzas still need the original database. */
    size_t vn = (size_t)(end - vector);
    char *v = malloc(vn + 3);
    if (!v) return 2;
    v[0] = ' '; memcpy(v + 1, vector, vn); v[vn + 1] = ' '; v[vn + 2] = 0;
    for (size_t i = 0; i < count; i++) {
        const struct leg *l = &legs[i];
        int samir = strcmp(l->name, "samir-boot") == 0;
        const char *image = samir ? "$(SECONDARY_TRACER_IMG)" : l->image;
        const char *data = samir ? "$(SAMIR_BOCHS_SOURCE)" : l->data;
        char target[160], needle[512], *row, *stop, *body;
        snprintf(target, sizeof target, "test-%s-bochs", l->name);
        snprintf(needle, sizeof needle, "\n%s: ", target);
        row = strstr(db, needle);
        require(row != NULL, "MISSING", target);
        row++;
        stop = strstr(row, "\n\n");
        require(stop != NULL, "MISSING", target);
        size_t n = (size_t)(stop - row);
        body = malloc(n + 1);
        if (!body) return 2;
        memcpy(body, row, n); body[n] = 0;
        snprintf(needle, sizeof needle, " %s ", target);
        char *hit = strstr(v, needle);
        if (!hit && samir) {
            /* initech-hgmx: a KNOWN-RED leg lives by name in test-bochs-pending
             * (house rule: named, never loosened) until its bead closes. */
            char *pend = strstr(db, "\ntest-bochs-pending: ");
            char *pend_end = pend ? strchr(pend + 1, '\n') : NULL;
            require(pend && pend_end, "VECTOR", target);
            *pend_end = 0;
            hit = strstr(pend, target);
            *pend_end = '\n';
            require(hit != NULL, "VECTOR", target);
        } else {
            require(hit && !strstr(hit + 1, needle), "VECTOR", target);
        }
        snprintf(needle, sizeof needle, "--disk \"%s\"", image);
        require(strstr(body, needle) != NULL, "IMAGE", target);
        if (l->data) {
            snprintf(needle, sizeof needle, "cp -f %s ", data);
            require(strstr(body, needle) != NULL, "IMAGE", target);
        } else {
            require(strstr(body, "--disk2") == NULL, "IMAGE", target);
        }
        char *nl = strchr(body, '\n');
        require(nl != NULL, "IMAGE", target);
        *nl = 0;
        require(strstr(body, l->image) && (!l->data || strstr(body, l->data)), "IMAGE", target);
        *nl = '\n';
        if (samir) {
            /* Preserve the independent concrete image/data expectation even
             * though this handwritten recipe uses Make variables (WL-0170). */
            require(strstr(db, "\nSECONDARY_TRACER_IMG := build/tracer_secondary.img\n") &&
                    strstr(db, "\nSAMIR_LIST_IMG := build/samir_list.img\n") &&
                    strstr(db, "\nSAMIR_BOCHS_SOURCE = $(SAMIR_LIST_IMG)\n"), "IMAGE", target);
            require(strstr(body, "--keys \"$(SAMIRBOOT_SCRN_KEYS)\" --keys-after SHELL-READY") &&
                    strstr(body, "--screendump-after SHELL-READY") &&
                    strstr(body, "$(BUILD)/ppm_bochs_check \"$(SAMIRBOOT_PPM)\"") &&
                    strstr(body, "sh harness/emu/samir_boot_check.sh $@ "),
                    "SAMIR", target);
        }
        /* DAC (before BOUNDARY, which would also notice): the VESA8 flagship leg must grade the padded 648 scanline. */
        if (strcmp(l->name, "flair-desktop-8bpp") == 0)
            require(strcmp(l->video, "vbe101pad") == 0 && strstr(body, " vbe101pad FLAIR-HEAP-OK"),
                    "DAC", target);
        /* PIXEL: a leg whose Bochs frame is graded against the QEMU golden
         * keeps its pixel twin as a prerequisite (the boot leg alone cannot
         * see a wrong desktop). The prerequisites are the stanza's first line. */
        if (strcmp(l->name, "flair-desktop-8bpp") == 0 || strcmp(l->name, "flair-desktop-icons") == 0) {
            char *first = strchr(body, '\n');
            snprintf(needle, sizeof needle, " %s-pixel", target);
            if (first) *first = 0;
            int ok = strstr(body, needle) != NULL;
            if (first) *first = '\n';
            require(ok, "PIXEL", target);
        }
        /* BOUNDARY: the completion line is both the harness --expect and its
         * --quit-after, and the ONE checker gets the leg's exact checks. */
        snprintf(needle, sizeof needle, "--expect %s --quit-after %s ", l->end, l->end);
        require(strstr(body, needle) != NULL, "BOUNDARY", target);
        snprintf(needle, sizeof needle, "sh harness/emu/bochs_leg_check.sh %s ", target);
        char *chk = strstr(body, needle);
        require(chk != NULL, "BOUNDARY", target);
        char *chk_end = strchr(chk, '\n');
        if (chk_end) *chk_end = 0;
        snprintf(needle, sizeof needle, " %s %s", l->video, l->checks);
        require(strstr(chk, needle) != NULL && strcmp(strstr(chk, needle), needle) == 0,
                "BOUNDARY", target);
        if (chk_end) *chk_end = '\n';
        /* VIDEO: the forced paths really reconfigure the Bochs card; the 0101h
         * paths must not (the stock ROM + its VBE extension is the finding). */
        if (strcmp(l->video, "vga13") == 0)
            require(strstr(body, "--vga-ext cirrus --vgabios /") != NULL, "VIDEO", target);
        else if (strcmp(l->video, "e00") == 0)
            require(strstr(body, "--vga-ext none ") != NULL, "VIDEO", target);
        else
            require(strstr(body, "--vga-ext") == NULL, "VIDEO", target);
        free(body);
    }
    free(v); free(db);
    printf("GREEN test-bochs-legs: %zu named boot legs; vector, images, boundaries, video paths, the padded DAC row, the pixel twins\n", count);
    return 0;
}
