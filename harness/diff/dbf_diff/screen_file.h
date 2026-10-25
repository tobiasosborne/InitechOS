/* Locked W4.0 screen format; PRD 6.6, samirctl-mint/scrdump.asm,
 * spec/tsr_contract.h: 2000 char/attribute pairs, row/column bytes. */
#ifndef SCREEN_FILE_H
#define SCREEN_FILE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>

typedef struct { unsigned char bytes[4002]; } screen_file;
static inline void screen_die(const char *s)
{
    fprintf(stderr, "screen: %s\n", s);
    exit(2);
}
static FILE *screen_open(const char *p, const char *mode)
{
    FILE *f = fopen(p, mode);
    if (!f) { perror(p); exit(2); }
    return f;
}
static inline void screen_bounds(const screen_file *s)
{
    if (s->bytes[4000] >= 25 || s->bytes[4001] >= 80)
        screen_die("cursor outside 80x25");
}
static inline void screen_raw(const char *p, screen_file *s)
{
    FILE *f = screen_open(p, "rb");
    if (fread(s->bytes, 1, 4002, f) != 4002 || fgetc(f) != EOF)
        screen_die("raw capture must be exactly 4002 bytes");
    fclose(f);
    screen_bounds(s);
}
static inline void screen_write(const char *p, const screen_file *s)
{
    FILE *f = screen_open(p, "wb");
    iconv_t cd = iconv_open("UTF-8", "CP437");
    if (cd == (iconv_t)-1) screen_die("CP437 converter unavailable");
    screen_bounds(s);
    fputs("INITECH-SCREEN 1 CP437 80 25\nTEXT\n", f);
    for (int r = 0; r < 25; r++) {
        for (int c = 0; c < 80; c++) {
            unsigned char ch = s->bytes[2*(80*r+c)];
            if (ch < 32 || ch == 127) {
                /* Visible, reversible control pictures, never embedded LF. */
                unsigned u = ch == 127 ? 0x2421 : 0x2400 + ch;
                fputc(0xe0 | (u >> 12), f);
                fputc(0x80 | ((u >> 6) & 63), f);
                fputc(0x80 | (u & 63), f);
            } else {
                char in = (char)ch, out[8], *ip = &in, *op = out;
                size_t il = 1, ol = sizeof out;
                if (iconv(cd, &ip, &il, &op, &ol) == (size_t)-1)
                    screen_die("invalid CP437 byte");
                fwrite(out, 1, sizeof out - ol, f);
            }
        }
        fputc('\n', f);
    }
    fputs("ATTR\n", f);
    for (int r = 0; r < 25; r++) {
        for (int c = 0; c < 80; c++) fprintf(f, "%02X", s->bytes[2*(80*r+c)+1]);
        fputc('\n', f);
    }
    fprintf(f, "CURSOR %02u %02u\nEND\n", s->bytes[4000], s->bytes[4001]);
    iconv_close(cd);
    if (fclose(f)) screen_die("write failed");
}
static inline void screen_line(FILE *f, char *b, size_t n)
{
    if (!fgets(b, (int)n, f) || !strchr(b, '\n')) screen_die("missing/long line");
}
static inline void screen_read(const char *p, screen_file *s)
{
    FILE *f = screen_open(p, "rb");
    char b[1024];
    iconv_t cd = iconv_open("CP437", "UTF-8");
    if (cd == (iconv_t)-1) screen_die("CP437 converter unavailable");
    screen_line(f, b, sizeof b);
    if (strcmp(b, "INITECH-SCREEN 1 CP437 80 25\n")) screen_die("bad screen header");
    screen_line(f, b, sizeof b);
    if (strcmp(b, "TEXT\n")) screen_die("missing TEXT");
    for (int r = 0; r < 25; r++) {
        screen_line(f, b, sizeof b);
        char *ip = b;
        size_t il = strlen(b)-1;
        for (int c = 0; c < 80; c++) {
            unsigned char ch;
            if (il >= 3 && (unsigned char)ip[0] == 0xe2 &&
                (unsigned char)ip[1] == 0x90 &&
                (unsigned char)ip[2] >= 0x80 && (unsigned char)ip[2] <= 0xa1 &&
                (unsigned char)ip[2] != 0xa0) {
                ch = (unsigned char)ip[2] == 0xa1 ? 127 : ((unsigned char)ip[2]-0x80);
                ip += 3; il -= 3;
            } else {
                char out, *op = &out;
                size_t ol = 1;
                /* One output byte. E2BIG after it is expected. */
                (void)iconv(cd, &ip, &il, &op, &ol);
                if (ol || (unsigned char)out < 32 || (unsigned char)out == 127)
                    screen_die("bad UTF-8 text cell");
                ch = (unsigned char)out;
            }
            s->bytes[2*(80*r+c)] = ch;
        }
        if (il) screen_die("text row must have 80 characters");
    }
    screen_line(f, b, sizeof b);
    if (strcmp(b, "ATTR\n")) screen_die("missing ATTR");
    const char *hex = "0123456789ABCDEF";
    for (int r = 0; r < 25; r++) {
        screen_line(f, b, sizeof b);
        if (strlen(b) != 161) screen_die("attribute row must have 160 hex digits");
        for (int c = 0; c < 80; c++) {
            const char *hi = strchr(hex, b[2*c]), *lo = strchr(hex, b[2*c+1]);
            if (!hi || !lo) screen_die("bad attribute hex");
            s->bytes[2*(80*r+c)+1] = (unsigned char)((hi-hex)*16+(lo-hex));
        }
    }
    screen_line(f, b, sizeof b);
    unsigned row, col; int end = 0;
    if (sscanf(b, "CURSOR %2u %2u%n", &row, &col, &end) != 2 ||
        strcmp(b+end, "\n") || row >= 25 || col >= 80) screen_die("bad CURSOR");
    char canonical[32];
    snprintf(canonical, sizeof canonical, "CURSOR %02u %02u\n", row, col);
    if (strcmp(b, canonical)) screen_die("noncanonical CURSOR");
    s->bytes[4000] = (unsigned char)row; s->bytes[4001] = (unsigned char)col;
    screen_line(f, b, sizeof b);
    if (strcmp(b, "END\n") || fgetc(f) != EOF) screen_die("bad END/trailing data");
    iconv_close(cd); fclose(f);
}
static inline int screen_compare(const screen_file *g, const screen_file *a)
{
    for (int i = 0; i < 4002; i++) if (g->bytes[i] != a->bytes[i]) {
        if (i < 4000)
            printf("RED: first difference row=%d col=%d %s golden=0x%02X actual=0x%02X\n",
                   i/160, (i/2)%80, i%2 ? "attribute" : "character", g->bytes[i], a->bytes[i]);
        else printf("RED: first difference cursor-%s golden=%u actual=%u\n",
                    i == 4000 ? "row" : "col", g->bytes[i], a->bytes[i]);
        return 1;
    }
    puts("GREEN: screens identical (2000 characters, 2000 attributes, cursor)");
    return 0;
}
#endif
