/* IBASE text screen, Using III+ U5-16/PDF 128, Fig 5-17 U5-259/PDF 371.
 * Manual-derived cells, no real STATUS capture. No palette policy here.
 * Normal/reverse attributes are PAL values; terminal transport belongs to PAL.
 */
#ifndef IBASE_SCREEN_H
#define IBASE_SCREEN_H
#include <stdint.h>
typedef void (*ibase_screen_draw)(void *,uint8_t,uint8_t,uint8_t,const char *,uint32_t);
typedef struct {
    /* at-dot/at-dot-c: STATUS OFF uses all 25 rows; SCOREBOARD ON
     * preserves row 0. Positioned output always addresses the whole page. */
    char text[25][80];
    char status[80], guidance[80];
    uint8_t row,col,active,scoreboard,attr;
    ibase_screen_draw draw;
    void (*scroll)(void *,uint8_t,uint8_t);
    void *user;
} ibase_screen;
void ibase_screen_init(ibase_screen *,ibase_screen_draw,void *);
void ibase_screen_status(ibase_screen *,int,char,const char *,uint32_t,uint32_t);
void ibase_screen_at(ibase_screen *,uint8_t,uint8_t,const char *,uint32_t);
void ibase_screen_write(ibase_screen *,const char *,uint32_t);
void ibase_screen_prompt(ibase_screen *,int,int);
#endif
