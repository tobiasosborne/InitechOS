/* Journey W markers, shared by the QEMU and Bochs frontends (initech-10ali,
 * initech-tbbri). A wait searches only after the end of the previous match,
 * so one serial line cannot satisfy a later wait. The match must lie on a
 * complete line and may not extend into a longer name: app=CALC does not
 * match app=CALCULAT, TENANT-EXIT does not match FLAIR-TENANT-EXIT. Name
 * characters are letters, digits, '_', '-' and '.'; a marker edge that is not
 * one (s="A:\WORD.WP5 before its closing quote) is a deliberate prefix and
 * needs no boundary. factory/testlib/marker-count.awk applies the same rule. */
#ifndef INITECH_EMU_MARKER_MATCH_H
#define INITECH_EMU_MARKER_MATCH_H
#include <stddef.h>
#include <string.h>
static int marker_name_char(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
}
static int marker_token_after(const char *txt, size_t len, size_t from,
                              const char *marker, size_t *end)
{
    size_t m = strlen(marker);
    if (m == 0 || from > len) return 0;
    for (size_t at = from; at + m <= len; at++) {
        if (memcmp(txt + at, marker, m) != 0) continue;
        if (marker_name_char(marker[0]) && at > 0 &&
            marker_name_char(txt[at - 1])) continue;
        if (!memchr(txt + at + m, '\n', len - at - m)) return 0; /* incomplete */
        if (marker_name_char(marker[m - 1]) && marker_name_char(txt[at + m]))
            continue;
        *end = at + m;
        return 1;
    }
    return 0;
}
#endif
