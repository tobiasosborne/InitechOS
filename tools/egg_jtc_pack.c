/* Factory C: locked JSON -> compact data -> bin2c. Ref: initech-2ter (a).
 * This reads our flat, ASCII-only spec shape; escapes are deliberately refused.
 * An invalid authored contract fails before it reaches the kernel. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "egg_jtc_model.h"
static char json[8192];
static unsigned char packed[JTC_BYTES];
static size_t used = JTC_WORDS * 2;   /* strings follow the words, packed */
static void fail(const char *s) { fprintf(stderr, "FAIL JTC JSON: %s\n", s); exit(1); }
static const char *key(const char *name)
{
    char k[80];
    snprintf(k, sizeof k, "\"%s\"", name);
    const char *p = strstr(json, k);
    if (!p || !(p = strchr(p + strlen(k), ':'))) fail(name);
    return p + 1;
}
/* Appends the next string, NUL-terminated; strings land in call order
 * (title, heading, labels), which is the order egg_jtc_model.c walks. */
static const char *string_cell(const char *p)
{
    while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
    if (*p++ != '"') fail("expected string");
    int n = 0;
    while (*p && *p != '"') {
        unsigned char c = (unsigned char)*p++;
        if (c < 32 || c > 126 || c == '\\' || n >= JTC_CELL - 1) fail("ASCII string cell");
        packed[used + n++] = c;
    }
    if (!n || *p++ != '"') fail("empty or unterminated string");
    used += (size_t)n + 1u;   /* packed[] is zeroed: the NUL is already there */
    return p;
}
int main(int argc, char **argv)
{
    static const char *const keys[JTC_WORDS] = {
        "rows", "cols", "pad_w", "pad_h", "gap", "margin", "heading_h",
        "window_x", "window_y", "window_w", "window_h", "trigger_clicks"
    };
    unsigned v[JTC_WORDS];
    if (argc != 2) fail("usage: egg_jtc_pack spec.json > data.bin");
    FILE *f = fopen(argv[1], "rb");
    if (!f) fail("open");
    size_t len = fread(json, 1, sizeof json - 1, f);
    if (!feof(f) || ferror(f)) fail("oversize or unreadable spec");
    fclose(f); json[len] = 0;
    for (int i = 0; i < JTC_WORDS; i++) {
        if (sscanf(key(keys[i]), " %u", &v[i]) != 1 || v[i] > 640 || v[i] == 0) fail(keys[i]);
        packed[i * 2] = (unsigned char)v[i]; packed[i * 2 + 1] = (unsigned char)(v[i] >> 8);
    }
    unsigned count = v[JTC_ROWS] * v[JTC_COLS];
    if (count < 8 || count > JTC_MAX || v[JTC_CLICKS] < 2 ||
        v[JTC_X] + v[JTC_W] > 640 || v[JTC_Y] + v[JTC_H] > 480 ||
        2*v[JTC_MARGIN] + v[JTC_COLS]*v[JTC_PAD_W] + (v[JTC_COLS]-1)*v[JTC_GAP] > v[JTC_W]-27 ||
        2*v[JTC_MARGIN] + v[JTC_HEADING_H] + v[JTC_ROWS]*v[JTC_PAD_H] + (v[JTC_ROWS]-1)*v[JTC_GAP] > v[JTC_H]-43)
        fail("mat does not fit the Platinum content rectangle");
    (void)string_cell(key("title")); (void)string_cell(key("heading"));
    const char *p = key("labels");
    while (*p == ' ' || *p == '\n') p++;
    if (*p++ != '[') fail("labels array");
    for (unsigned i = 0; i < count; i++) {
        if (i) { while (*p == ' ' || *p == '\n') p++; if (*p++ != ',') fail("missing pad"); }
        p = string_cell(p);
    }
    while (*p == ' ' || *p == '\n') p++;
    if (*p != ']') fail("extra pad");
    return fwrite(packed, 1, used, stdout) != used;
}
