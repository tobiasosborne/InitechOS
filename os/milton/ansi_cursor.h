/* DOS 3.30 TR Ch 4 CUD/CUF/CUU/CUB bridge. Shared by the live kernel
 * and its host regression; native page cursor positioning has its own bounds. */
#ifndef INITECH_ANSI_CURSOR_H
#define INITECH_ANSI_CURSOR_H
#include "console.h"
static void ansi_cursor_rel(void *ctx, int drow, int dcol)
{
    console_t *con = (console_t *)ctx;
    if (con) {
        uint32_t r = 0, c = 0;
        int nr, nc;
        console_get_cursor(con, &r, &c);
        /* Clamp the LOW edge here: console_set_cursor takes uint32_t and only
         * clamps the HIGH edge, so a negative relative move (cursor up past row
         * 0) must be pinned to 0 before the cast (else it wraps to ~UINT32 and
         * clamps to the LAST row -- the wrong direction). */
        nr = (int)r + drow;
        nc = (int)c + dcol;
        if (nr < 0) { nr = 0; }
        if (nc < 0) { nc = 0; }
        console_set_cursor(con, (uint32_t)nr, (uint32_t)nc);
    }
}
#endif
