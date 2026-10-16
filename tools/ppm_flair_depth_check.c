/* initech-2gva; PRD Sec 5/8, ADR-0004 OD-2. Compare REAL QMP RGB captures,
 * not the host render.c reimplementation. Exact equality; no tolerance change.
 * The scene is independently graded by ppm_flair_check / appswitch_check.
 */
#include <stdio.h>
#include <stdlib.h>

static unsigned char *read_ppm(const char *path)
{
    FILE *f = fopen(path, "rb");
    int w, h, max, used = 0;
    unsigned char *p;
    char line[160];
    if (!f) return NULL;
    if (!fgets(line, sizeof line, f) || line[0] != 'P' || line[1] != '6') goto bad;
    do {
        if (!fgets(line, sizeof line, f)) goto bad;
    } while (line[0] == '#');
    if (sscanf(line, "%d %d", &w, &h) != 2 || w != 640 || h != 480) goto bad;
    if (!fgets(line, sizeof line, f) || sscanf(line, "%d%n", &max, &used) != 1 ||
        max != 255) goto bad;
    p = malloc(640u * 480u * 3u);
    if (!p) goto bad;
    if (fread(p, 1, 640u * 480u * 3u, f) != 640u * 480u * 3u || fgetc(f) != EOF) {
        free(p);
        goto bad;
    }
    fclose(f);
    return p;
bad:
    fclose(f);
    return NULL;
}

int main(int argc, char **argv)
{
    unsigned char *a, *b;
    if (argc != 3) return 2;
    a = read_ppm(argv[1]); b = read_ppm(argv[2]);
    if (!a || !b) {
        fprintf(stderr, "RED flair-depth invalid/missing 640x480 P6 capture\n");
        free(a); free(b); return 2;
    }
    for (unsigned i = 0; i < 640u * 480u; ++i) {
        unsigned k = i * 3;
        if (a[k] != b[k] || a[k+1] != b[k+1] || a[k+2] != b[k+2]) {
            fprintf(stderr, "RED flair-depth RGB (%u,%u) 32bpp=#%02X%02X%02X 8bpp=#%02X%02X%02X\n",
                    i % 640, i / 640, a[k], a[k+1], a[k+2], b[k], b[k+1], b[k+2]);
            free(a); free(b); return 1;
        }
    }
    free(a); free(b);
    puts("GREEN flair-depth: all 307200 RGB pixels identical (32bpp == 8bpp)");
    return 0;
}
