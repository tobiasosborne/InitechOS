/* screens.c -- compare IDE screen dumps with real TP 5.5 captures.
 *
 * Usage: screens CAPTURES SHOTS EXPECT
 * CAPTURES holds MINT06 NAME.vga (B800h bytes) and NAME.crt (CRTC cursor);
 * SHOTS holds the IDE's SHOTnnn.SCR dumps (host.pas) and shots.list,
 * the shot names in dump order. The expected screen of every shot is a
 * capture, changed only by the explicit lines of EXPECT:
 *   shot NAME CAPTURE        start NAME's expectation from CAPTURE
 *   copy CAPTURE SR SC DR DC LEN   cells from a capture (".": the shot's own)
 *   fill ROW C0 C1 ATTR      blank cells: omitted chrome (README "Absent")
 *   attr ROW C0 C1 ATTR      recolour cells, e.g. a selected item
 *   cursor hidden | cursor COL ROW   cursor from another capture
 *   use BLOCK                apply a block defined by "define BLOCK" ... "end"
 * Everything after '#' is the reason. Nothing is ever computed from the IDE.
 * Characters, attributes and the cursor (position when visible) must match. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *captures, *shots;
static char base[64];
static unsigned char want[4000];
static unsigned char mask_char[2000];
static int want_col, want_row, want_on;
static char blocks[16][32][256];
static char block_name[16][64];
static int block_len[16], nblocks;

static void load_capture(const char *name, unsigned char *cells, int *col, int *row, int *on)
{
    char p[1024];
    snprintf(p, sizeof p, "%s/%s.vga", captures, name);
    FILE *f = fopen(p, "rb");
    if (!f || fread(cells, 1, 4000, f) != 4000) { fprintf(stderr, "cannot read %s\n", p); exit(2); }
    fclose(f);
    snprintf(p, sizeof p, "%s/%s.crt", captures, name);
    f = fopen(p, "r");
    unsigned loc, vis;
    char text[256];
    if (!f || !fgets(text, sizeof text, f)) { fprintf(stderr, "cannot read %s\n", p); exit(2); }
    fclose(f);
    char *l = strstr(text, "location="), *v = strstr(text, "visible=");
    if (!l || !v || sscanf(l, "location=%u", &loc) != 1 || sscanf(v, "visible=%u", &vis) != 1) {
        fprintf(stderr, "bad %s\n", p); exit(2);
    }
    if (col) { *col = (int)(loc % 80); *row = (int)(loc / 80); *on = (int)vis; }
}

static void apply(char *line, const char *where);

static void edit(char *line, const char *where)
{
    char *hash = strchr(line, '#');
    if (hash) *hash = 0;
    char op[32], a[64];
    int r, c0, c1, at, sr, sc, dr, dc, len;
    if (sscanf(line, "%31s", op) != 1) return;
    if (!strcmp(op, "copy") && sscanf(line, "copy %63s %d %d %d %d %d", a, &sr, &sc, &dr, &dc, &len) == 6) {
        unsigned char src[4000];
        load_capture(strcmp(a, ".") ? a : base, src, NULL, NULL, NULL);
        for (int i = 0; i < len; i++) {
            want[(dr * 80 + dc + i) * 2] = src[(sr * 80 + sc + i) * 2];
            want[(dr * 80 + dc + i) * 2 + 1] = src[(sr * 80 + sc + i) * 2 + 1];
        }
    } else if (!strcmp(op, "mask") && sscanf(line, "mask %d %d %d", &r, &c0, &c1) == 3) {
        if (r < 0 || r >= 25 || c0 < 0 || c1 >= 80 || c1 < c0) {
            fprintf(stderr, "invalid value mask: %s\n", where); exit(2);
        }
        for (int i = c0; i <= c1; i++) mask_char[r * 80 + i] = 1;
    } else if (!strcmp(op, "fill") && sscanf(line, "fill %d %d %d %i", &r, &c0, &c1, &at) == 4) {
        for (int i = c0; i <= c1; i++) { want[(r * 80 + i) * 2] = ' '; want[(r * 80 + i) * 2 + 1] = (unsigned char)at; }
    } else if (!strcmp(op, "attr") && sscanf(line, "attr %d %d %d %i", &r, &c0, &c1, &at) == 4) {
        for (int i = c0; i <= c1; i++) want[(r * 80 + i) * 2 + 1] = (unsigned char)at;
    } else if (!strcmp(op, "cursor") && sscanf(line, "cursor %63s", a) == 1 && !strcmp(a, "hidden")) {
        want_on = 0;
    } else if (!strcmp(op, "cursor") && sscanf(line, "cursor %d %d", &c0, &r) == 2) {
        want_on = 1; want_col = c0; want_row = r;
    } else if (!strcmp(op, "use") && sscanf(line, "use %63s", a) == 1) {
        int b = -1;
        for (int i = 0; i < nblocks; i++) if (!strcmp(block_name[i], a)) b = i;
        if (b < 0) { fprintf(stderr, "%s: unknown block %s\n", where, a); exit(2); }
        for (int i = 0; i < block_len[b]; i++) { char copy[256]; strcpy(copy, blocks[b][i]); apply(copy, where); }
    } else { fprintf(stderr, "%s: bad line: %s\n", where, line); exit(2); }
}

static void apply(char *line, const char *where) { edit(line, where); }

static int compare(const char *name, int index)
{
    char p[1024];
    unsigned char got[4003];
    snprintf(p, sizeof p, "%s/SHOT%03d.SCR", shots, index);
    FILE *f = fopen(p, "rb");
    if (!f || fread(got, 1, 4003, f) != 4003) { printf("FAIL %s: no dump %s\n", name, p); return 1; }
    fclose(f);
    int bad = 0;
    for (int i = 0; i < 2000; i++)
        if ((!mask_char[i] && got[i * 2] != want[i * 2]) || got[i * 2 + 1] != want[i * 2 + 1]) {
            if (bad < 8)
                printf("FAIL %s: row %d col %d got %02X/%02X want %02X/%02X\n", name, i / 80, i % 80,
                       got[i * 2], got[i * 2 + 1], want[i * 2], want[i * 2 + 1]);
            bad++;
        }
    if (got[4002] != want_on) { printf("FAIL %s: cursor visible %d want %d\n", name, got[4002], want_on); bad++; }
    else if (want_on && (got[4000] != want_col || got[4001] != want_row)) {
        printf("FAIL %s: cursor %d,%d want %d,%d\n", name, got[4000], got[4001], want_col, want_row); bad++;
    }
    if (bad) printf("FAIL %s: %d differences\n", name, bad); else printf("PASS %s\n", name);
    return bad != 0;
}

int main(int argc, char **argv)
{
    if (argc != 4) { fprintf(stderr, "screens CAPTURES SHOTS EXPECT\n"); return 2; }
    captures = argv[1]; shots = argv[2];
    char names[256][64];
    int nnames = 0;
    char p[1024], line[256];
    snprintf(p, sizeof p, "%s/shots.list", shots);
    FILE *f = fopen(p, "r");
    if (!f) { perror(p); return 2; }
    while (nnames < 256 && fgets(line, sizeof line, f)) { line[strcspn(line, "\n")] = 0; strcpy(names[nnames++], line); }
    fclose(f);
    FILE *e = fopen(argv[3], "r");
    if (!e) { perror(argv[3]); return 2; }
    int failed = 0, checked = 0, current = -1, defining = -1;
    char cname[64] = "";
    while (fgets(line, sizeof line, e)) {
        line[strcspn(line, "\n")] = 0;
        char op[32], a[64], b[64];
        if (sscanf(line, "%31s", op) != 1 || op[0] == '#') continue;
        if (defining >= 0) {
            if (!strcmp(op, "end")) defining = -1;
            else strcpy(blocks[defining][block_len[defining]++], line);
            continue;
        }
        if (!strcmp(op, "define") && sscanf(line, "define %63s", a) == 1) {
            defining = nblocks++; strcpy(block_name[defining], a); continue;
        }
        if (!strcmp(op, "shot") && sscanf(line, "shot %63s %63s", a, b) == 2) {
            if (current >= 0) { failed += compare(cname, current + 1); checked++; }
            current = -1;
            for (int i = 0; i < nnames; i++) if (!strcmp(names[i], a)) current = i;
            if (current < 0) { printf("FAIL %s: not in the journey\n", a); return 1; }
            strcpy(cname, a);
            strcpy(base, b);
            memset(mask_char, 0, sizeof mask_char);
            load_capture(b, want, &want_col, &want_row, &want_on);
            continue;
        }
        if (current < 0) { fprintf(stderr, "%s: edit before shot\n", argv[3]); return 2; }
        edit(line, cname);
    }
    if (current >= 0) { failed += compare(cname, current + 1); checked++; }
    printf("screens: %d compared, %d failed\n", checked, failed);
    return failed ? 1 : 0;
}
