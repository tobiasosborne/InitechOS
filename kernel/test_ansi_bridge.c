/* initech-fzsvo: exercise the SAME adapter as kmain with the real console.
 * DOS 3.30 TR Ch 4: CUD stops at the last row. Native BIOS-page positioning
 * must still retain real III+ at-to-b/TE's cursor row 25. */
#include <stdio.h>
#include <string.h>
#include "ansi.h"
#include "ansi_cursor.h"
static void action(const ansi_action_t *a,void *ctx)
{
    if(a->kind==ANSI_ACT_CURSOR_REL) ansi_cursor_rel(ctx,a->delta_row,a->delta_col);
    if(a->kind==ANSI_ACT_MOVE_CURSOR) console_set_cursor(ctx,(unsigned)a->row,(unsigned)a->col);
}
int main(void)
{
    console_t con;
    ansi_state_t st;
    const char *p="\033[25;1H\033[B";
    memset(&con,0,sizeof con);con.cols=80;con.rows=25;
    ansi_init(&st,25,80);
    while(*p) ansi_feed(&st,(unsigned char)*p++,action,&con);
    if(con.cur_row!=24 || con.cur_col!=0) {
        fprintf(stderr,"RED ANSI bridge: CUD bottom row expected=24 actual=%u\n",con.cur_row);
        return 1;
    }
    console_set_cursor(&con,25,0);
    if(con.cur_row!=25) { fputs("RED ANSI bridge: native page row 25 lost\n",stderr);return 1; }
    puts("GREEN ANSI bridge: CUD clamps at 24; native page keeps 25");
    return 0;
}
