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
        /* DOS ANSI movement stays on the page; the native page API may
         * carry real III+'s off-grid row 25 (initech-fzsvo). */
        nr = (int)r + drow;
        nc = (int)c + dcol;
        if (nr < 0) { nr = 0; }
        if (nc < 0) { nc = 0; }
        if (nr >= (int)con->rows) nr = (int)con->rows - 1;
        if (nc >= (int)con->cols) nc = (int)con->cols - 1;
        console_set_cursor(con, (uint32_t)nr, (uint32_t)nc);
    }
}
#endif
