/* Factory regression: the icon graders must follow the LOCKED scene.
 * Ref: spec/flair_trash_traces.mk FLAIR_EMPTY_OK_SPEC double-clicks Trash;
 * spec/flair_finder_icons_traces.mk moves APPS to (250,180);
 * spec/dos_folder_apps.txt is the independent shipped directory inventory.
 * Pixel inputs below are real mint-008 capture cells, NOT artifact renders
 * or strike data. This tests the grader's state routing, not the icon art
 * (test-finder-icon-art already grades the renderer against those captures).
 * Rule 6: TRASH_CHECK_MUT_LOCKED_UNSELECTED and ICON_SCENE_MUT_STALE_DOC.
 */
#define main trash_check_main
#include "../../tools/ppm_flair_trash_check.c"
#undef main

static int full_state(const char *leg, int state)
{
    static unsigned char frame[SCRW * SCRH * 3];
    static icap_cell_t cap[ICAP__KINDS];
    if (icap_load(ICON_CAPTURES_DIR, cap)) return 1;
    memset(frame, 0, sizeof frame);
    for (int r = 0; r < 32; r++) {
        for (int c = 0; c < 32; c++) {
            /* Seafoam is the stated InitechOS backdrop, not a Mac colour.
             * Opaque pixels come only from the independent Mac capture. */
            unsigned int px = cap[ICAP_TRASH_FULL].opaque[r][c]
                ? cap[ICAP_TRASH_FULL].rgb[state][r][c] : 0x8DDCDCu;
            unsigned char *p = frame + 3 * ((TR_Y + r) * SCRW + TR_X + c);
            p[0] = (unsigned char)(px >> 16);
            p[1] = (unsigned char)(px >> 8);
            p[2] = (unsigned char)px;
        }
    }
    g_buf = frame;
    g_leg = leg;
    g_fail = 0;
    trash_full();
    printf("%s FULL-state routing %s: %s\n", g_fail ? "FAIL" : "PASS",
           leg, state == ICAP_SEL ? "selected" : "unselected");
    return g_fail;
}

int main(int argc, char **argv)
{
    char name[64], want[96];
    const char *const required[] = { "123.EXE", "WORD.EXE", "TPSPARM.DAT" };
    const char *const kind[] = { "APP", "APP", "DOC" };
    int slot[3] = { -1, -1, -1 }, n = 0, bad;
    FILE *f;
    if (argc != 9) {
        fprintf(stderr, "usage: %s <APPS inventory> <7 scene cells>\n", argv[0]);
        return 2;
    }
    f = fopen(argv[1], "r");
    if (!f) { perror(argv[1]); return 1; }
    while (fscanf(f, "%63s", name) == 1) {
        for (int k = 0; k < 3; k++)
            if (strcmp(name, required[k]) == 0) slot[k] = n;
        n++;
    }
    fclose(f);
    bad = full_state("locked-full", ICAP_SEL);
    bad += full_state("alert", ICAP_UNSEL);
    /* These four cells are outside APPS and do not depend on its order. */
    const char *const fixed[] = {
        "VOLUME:SEL@584,48/8DDCDC", "TRASH:UNSEL@584,404/8DDCDC",
        "DOC:SEL@44,86/FFFFFF", "FOLDER:UNSEL@112,86/FFFFFF"
    };
    for (int k = 0; k < 4; k++) {
        if (strcmp(argv[k + 2], fixed[k]) != 0) {
            printf("FAIL scene fixed cell: got %s want %s\n", argv[k + 2], fixed[k]);
            bad++;
        }
    }
    /* Hand-derived from the trace's moved window, content inset (24,26),
     * four columns and pitch 68x52 (finder_windows.h Sec 2/3). No renderer
     * or image recipe is consulted to decide the expected name or kind. */
    for (int k = 0; k < 3; k++) {
        if (slot[k] < 0) {
            printf("FAIL scene inventory missing %s\n", required[k]);
            bad++;
            continue;
        }
        snprintf(want, sizeof want, "%s:UNSEL@%d,%d/FFFFFF", kind[k],
                 274 + 68 * (slot[k] % 4), 206 + 52 * (slot[k] / 4));
        if (strcmp(argv[k + 6], want) != 0) {
            printf("FAIL scene APPS %s: got %s want %s (inventory cell %d)\n",
                   required[k], argv[k + 6], want, slot[k]);
            bad++;
        } else printf("PASS scene APPS %s: %s\n", required[k], want);
    }
    printf("test-finder-icon-oracles: %s (%d failures)\n", bad ? "RED" : "green", bad);
    return bad ? 1 : 0;
}
