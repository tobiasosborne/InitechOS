/* SAMIR text screen, Using III+ U5-16/PDF 128, Fig 5-17 U5-259/PDF 371.
 * Manual-derived cells, no real STATUS capture. No palette policy here.
 * Normal/reverse attributes are PAL values; terminal transport belongs to PAL.
 */
#ifndef SAMIR_SCREEN_H
#define SAMIR_SCREEN_H
#include <stdint.h>
typedef void (*samir_screen_draw)(void *,uint8_t,uint8_t,uint8_t,const char *,uint32_t);
typedef struct {
    char text[22][80];
    char status[80], guidance[80];
    uint8_t row,col,active;
    samir_screen_draw draw;
    void *user;
} samir_screen;
void samir_screen_init(samir_screen *,samir_screen_draw,void *);
void samir_screen_status(samir_screen *,int,char,const char *,uint32_t,uint32_t);
void samir_screen_write(samir_screen *,const char *,uint32_t);
#endif
