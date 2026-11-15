/* A screen for host checks: replays recorded Toolbox drawing (tbx_fill_rect,
 * tbx_draw_text) into a 640 x 480 map of colour tokens, as the gate does
 * (sdk/include/toolbox_gate.h Sec 7): coordinates are content-local, every
 * draw is clipped to the window's content, a Geneva 9 run's bg box is the
 * sum of its advances wide and GENEVA9_CELL_H rows tall with the glyph ink
 * over it (FLAIR's public strike, flair/include/assets/geneva9.h). It reads
 * only the op stream and the windows' recorded bounds with FLAIR's insets
 * (host_windows.h); nothing here knows how Calc paints. */
#ifndef CALC_TEST_REPLAY_H
#define CALC_TEST_REPLAY_H
#include <string.h>
#include "assets/geneva9.h"
/* One window's content, content-local: token + 1, 0 where nothing drew. */
typedef struct { unsigned char px[480][640]; } replay_screen_t;
static inline void replay_content(int32_t w,int *l,int *t,int *cw,int *ch)
{
    *l=tbx_host.windows[w-1].l+6; *t=tbx_host.windows[w-1].t+22;
    *cw=tbx_host.windows[w-1].r-tbx_host.windows[w-1].l-host_inset_w(w);
    *ch=tbx_host.windows[w-1].b-tbx_host.windows[w-1].t-host_inset_h(w);
}
static inline void replay_set(replay_screen_t *s,int32_t w,int x,int y,unsigned tok)
{
    int l,t,cw,ch; replay_content(w,&l,&t,&cw,&ch);
    if(x<0||y<0||x>=cw||y>=ch||x>=640||y>=480) return;
    s->px[y][x]=(unsigned char)(tok+1);
}
/* Replays window w's drawing in ops [from, tbx_host.nops); returns how many
 * drawing calls (to any window) there were. */
static inline unsigned replay(replay_screen_t *s,int32_t win,unsigned from)
{
    unsigned n=0;
    for(unsigned i=from;i<tbx_host.nops;i++) {
        const tbx_host_op_t *o=&tbx_host.ops[i];
        int32_t w=o->a[0];
        if(w!=win) { n+=!strcmp(o->name,"tbx_fill_rect")||!strcmp(o->name,"tbx_draw_text")||!strcmp(o->name,"tbx_draw_cells"); continue; }
        if(!strcmp(o->name,"tbx_fill_rect")) {
            n++;
            for(int y=o->a[2];y<o->a[4];y++) for(int x=o->a[1];x<o->a[3];x++) replay_set(s,w,x,y,(unsigned)o->a[5]);
        } else if(!strcmp(o->name,"tbx_draw_text")) {
            n++;
            unsigned fg=(unsigned)o->a[3]&255u, bg=(unsigned)o->a[4]; int x0=o->a[1];
            for(const char *c=o->text;*c;c++) {
                const unsigned char *g=geneva9_glyph(*c); int adv=(int)geneva9_advance_w(*c);
                for(int r=0;r<GENEVA9_CELL_H;r++) for(int k=0;k<adv;k++)
                    replay_set(s,w,x0+k,o->a[2]+r,(k<8&&(g[r]&(0x80u>>k)))?fg:bg);
                x0+=adv;
            }
        } else if(!strcmp(o->name,"tbx_draw_cells")) n++;
    }
    return n;
}
#endif
