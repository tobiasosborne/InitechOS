/* Factory inventory oracle, written before the cqej second-half legs.
 * Independent roster: WL-0131 Bochs row, WL-0140..0155 and the lane brief.
 * PRD Sec 8 / Rule 5 require named legs in the executed vector. These are
 * BOOT boundaries, not input/pixel/disk-operation acceptance (bochs.h).
 * IBM DOS 3.30 TR p.1-7 (PDF24) grounds the preserved system-file chain;
 * ADR-0003 Appendix A grounds the existing DOS gates, not this wiring test.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct leg { const char *name, *image, *data, *end; };
#define DATA "build/flair_data.img"
#define FLAIR(n) { n, "build/flair_tenants.img", DATA, "HALTED" }
#define WORD(n) { n, "build/flair_tenants_mut_iword_gate.img", DATA, "HALTED" }
#define CP(n) { "flair-cpanels" n, "build/flair_tenants_mut_cpanels_gate.img", DATA, "HALTED" }
#define DOS(n) { n, "build/tracer_secondary.img", DATA, "SHELL-READY" }
#define DML(n) DOS("samir-dml-" n "-emu")
static const struct leg legs[] = {
    { "command-file-emu", "build/command-file/fragment.img", DATA, "HALTED" },
    FLAIR("command-file-negative"),
    { "flair-desktop-8bpp", "build/vesa8/flagship.img", DATA, "HALTED" },
    WORD("flair-notepad"), WORD("flair-scrapbook"),
    WORD("flair-notepad-itext"), WORD("flair-notepad-restart"),
    { "flair-notepad-corrupt", "build/flair_tenants_mut_iword_gate.img",
      "build/da_corrupt_source.img", "HALTED" },
    CP(""), CP("-date"), CP("-patterns"), CP("-mouse"), CP("-sound"),
    CP("-memory"), CP("-settings"), CP("-persist"),
    { "flair-cpanels-corrupt", "build/flair_tenants_mut_cpanels_gate.img",
      "build/cp_corrupt_data.img", "HALTED" },
    FLAIR("flair-grow-box"), DOS("block-device-emu"), DOS("fdisk-emu"),
    DOS("formatsys-emu"), DOS("format-system-boot"), FLAIR("dos-folder"),
    DOS("conkeys"), DOS("j1lu-control-emu"),
    { "ansi-emu", "build/tracer_secondary.img", "build/ansi_probe_data.img", "SHELL-READY" },
    FLAIR("flair-finder-icons"), FLAIR("flair-trash-full"),
    FLAIR("flair-toolbox-hardening"), DOS("fat-hygiene-emu"),
    FLAIR("trash-hidden-emu"),
    { "ata-no-drive-emu", "build/tracer_secondary.img", NULL, "SHELL-READY" },
    { "baked-exec-guard", "build/baked_guard.img", "build/fat_exec.img", "EXEC-4B-END" },
    { "mzload-entry-emu", "build/mzexec_boot.img", "build/fat_mzexec.img", "MZEXEC-END" },
    DOS("samir-core-emu"), DOS("samir-dml-emu"),
    DML("clear"), DML("macro"), DML("close"), DML("index"), DML("reindex"),
    DML("sort"), DML("total"), DML("append"), DML("save"), DML("restore"),
    DML("copyfile"), DML("type"), DML("rename"), DML("erase"), DML("label"), DML("run"),
    DOS("samir-print-emu"), DOS("shell-files-emu"), DOS("command-c-emu"),
    DOS("xcopy-cwd-emu"), DOS("fat-truncate-emu"), DOS("dir-match-emu"),
    DOS("shell-dir-look"),
    WORD("flair-iword-codes"), WORD("flair-iword-block"), WORD("flair-iword-format"),
    WORD("flair-iword-fkeys"), WORD("flair-iword-search"),
    WORD("flair-iword-replace"), WORD("flair-iword-undo"), WORD("flair-iword-setup"),
    FLAIR("kernel-size-r4-emu")
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
        char target[160], needle[256], *row, *stop, *body;
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
        require(hit && !strstr(hit + 1, needle), "VECTOR", target);
        snprintf(needle, sizeof needle, "--disk \"%s\"", l->image);
        require(strstr(body, needle) != NULL, "IMAGE", target);
        if (l->data) {
            snprintf(needle, sizeof needle, "cp -f %s ", l->data);
            require(strstr(body, needle) != NULL, "IMAGE", target);
        } else {
            require(strstr(body, "--disk2") == NULL &&
                    strstr(body, "grep -qxF 'FAT-MOUNT-FAIL rc=-1 base_lba=0 hidden_sectors=0'"),
                    "IMAGE", target);
        }
        char *nl = strchr(body, '\n');
        require(nl != NULL, "IMAGE", target);
        *nl = 0;
        require(strstr(body, l->image) && (!l->data || strstr(body, l->data)), "IMAGE", target);
        *nl = '\n';
        snprintf(needle, sizeof needle, "--expect %s ", l->end);
        require(strstr(body, needle) != NULL, "BOUNDARY", target);
        require(strstr(body, "rfb_unblocked=1") && strstr(body, "triple_fault=1") &&
                strstr(body, "VBE-ENOMODE VGA13 S1 PM KERNEL CONSOLE BANNER"),
                "BOUNDARY", target);
        if (strcmp(l->end, "HALTED") == 0)
            require(strstr(body, "FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK HALTED") &&
                    strstr(body, "grep -qxF 'PANIC flair-desktop: LFB smaller than 640x480 w=320 h=200'"),
                    "BOUNDARY", target);
        else
            require(strstr(body, "! grep -q '^PANIC'") != NULL, "BOUNDARY", target);
        require(strstr(body, "Bochs BOOT ONLY:") != NULL, "BOUNDARY", target);
        if (strcmp(l->name, "flair-desktop-8bpp") == 0)
            require(strstr(body, "grep -qxF 'VIDEO 320x200 bpp=8 pitch=320 lfb=0x000A0000 dac=6'") != NULL,
                    "DAC", target);
        free(body);
    }
    free(v); free(db);
    printf("GREEN test-bochs-legs: %zu named boot legs; vector, images, boundaries, honest VGA13\n", count);
    return 0;
}
