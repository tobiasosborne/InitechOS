/* SET COLOR parser/PAL oracle. Ref: set-commands.md Sec 3 SET COLOR;
 * HELP.DBS lines 1390-1398, CGA attribute byte (console.c). */
#include <stdio.h>
#include <string.h>
#include "test_assert.h"
#include "samir/interp.h"
#include "samir/set.h"
TEST_HARNESS();
struct pal_host_cfg { uint8_t yy,mm,dd; uint32_t heap; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(samir_pal_t *);
static unsigned calls;
static uint8_t last_attr;
static void attribute(samir_pal_t *p,uint8_t a)
{ (void)p; calls++;last_attr=a; }
static void quiet(samir_pal_t *p,const char *s,uint32_t n)
{ (void)p;(void)s;(void)n; }
int main(void)
{
    static const struct { const char *text;uint8_t attr; } valid[]={
        {"N/N",0x00},{"B/N",0x01},{"G/N",0x02},{"BG/N",0x03},
        {"R/N",0x04},{"RB/N",0x05},{"GR/N",0x06},{"RG/N",0x06},
        {"W/N",0x07},{"W+/B",0x1f},{"B*/W",0xf1},
        {"W+*/BG",0xbf},{"U",0x01},{"I",0x70},{"X",0x00},
        {"w/b",0x17},{" W / BG ",0x37},{"W/N+*",0x8f},
        {"N/B",0x10},{"N/G",0x20},{"N/BG",0x30},{"N/R",0x40},
        {"N/RB",0x50},{"N/GR",0x60},{"N/W",0x70},{"W*+/N",0x8f}
    };
    static const char *const bad[]={
        "SET COLOR","SET COLOR W/N","SET COLOR TO Q/N",
        "SET COLOR TO W/Q","SET COLOR TO WW/N","SET COLOR TO W//N",
        "SET COLOR TO /N","SET COLOR TO W/","SET COLOR TO W++/N",
        "SET COLOR TO W**/N","SET COLOR TO W/N junk",
        "SET COLOR TO W/N,N/W,N/N,N/N,W/N",
        "SET COLOR TO W/N,Q/N","SET COLOR TO W/N,N/W,Q/N",
        "SET COLOR TO W/N,N/W,N/N,Q/N","SET COLOR TO W/N+ +",
        "SET COLOR TO W+/N+","SET COLOR TO B","SET COLOR TO ,,,,"
    };
    samir_pal_t *pal=pal_host_make((struct pal_host_cfg){86,7,24,1048576});
    xb_interp *ip;char command[128];unsigned i;
    CHECK(pal!=NULL,"host PAL allocated");if(!pal)return 1;
    pal->conout=quiet;pal->set_attr=attribute;
    ip=xb_interp_make(pal);CHECK(ip!=NULL,"interpreter allocated");
    if(!ip){pal_host_free(pal);return 1;}
    CHECK(set_register(ip)==0,"SET registered");
    CHECK(set_get_state(ip)->color_standard==7 &&
          set_get_state(ip)->color_enhanced==0x70 &&
          set_get_state(ip)->color_border==0 &&
          set_get_state(ip)->color_background==0,"SET COLOR HELP defaults stored");
    for(i=0;i<sizeof valid/sizeof valid[0];i++) {
        unsigned before=calls;
        snprintf(command,sizeof command,"SET COLOR TO %s",valid[i].text);
        CHECK(samir_do(ip,command)==0,"SET COLOR valid attribute accepted");
        CHECK(calls==before+1 && last_attr==valid[i].attr,
              "SET COLOR standard attribute reaches PAL");
    }
    CHECK(samir_do(ip,"SET COLOR TO W+/B,N/W,BG/N,R/N")==0,
          "SET COLOR four attributes accepted");
    CHECK(set_get_state(ip)->color_standard==0x1f &&
          set_get_state(ip)->color_enhanced==0x70 &&
          set_get_state(ip)->color_border==3 &&
          set_get_state(ip)->color_background==4,"SET COLOR all four attributes stored");
    CHECK(samir_do(ip,"SET COLOR TO ,R/B,,G/N")==0 && last_attr==0x1f,
          "SET COLOR omitted standard retained and applied");
    CHECK(set_get_state(ip)->color_enhanced==0x14 &&
          set_get_state(ip)->color_border==3 &&
          set_get_state(ip)->color_background==2,"SET COLOR omitted slots retained");
    for(i=0;i<sizeof bad/sizeof bad[0];i++) {
        unsigned before=calls;uint8_t saved=last_attr;
        set_state snapshot=*set_get_state(ip);
        CHECK(samir_do(ip,bad[i])!=0,"SET COLOR malformed input rejected");
        CHECK(samir_last_error(ip)==10,"SET COLOR malformed input raises Syntax error 10");
        CHECK(calls==before && last_attr==saved,"SET COLOR rejection leaves PAL unchanged");
        CHECK(set_get_state(ip)->color_standard==snapshot.color_standard &&
              set_get_state(ip)->color_enhanced==snapshot.color_enhanced &&
              set_get_state(ip)->color_border==snapshot.color_border &&
              set_get_state(ip)->color_background==snapshot.color_background,
              "SET COLOR rejection leaves stored attributes unchanged");
    }
    CHECK(samir_do(ip,"SET COLOR TO")==0 && last_attr==0x07,
          "SET COLOR empty TO restores white on black");
    CHECK(set_get_state(ip)->color_enhanced==0x70 &&
          set_get_state(ip)->color_border==0 && set_get_state(ip)->color_background==0,
          "SET COLOR reset restores every slot");
    CHECK(samir_do(ip,"SET COLOR TO R/B")==0,"SET COLOR nondefault before second interpreter");
    {
        xb_interp *other=xb_interp_make(pal);
        CHECK(other && set_register(other)==0,"second SET interpreter registered");
        if(other) {
            CHECK(set_get_state(other)->color_standard==7 &&
                  set_get_state(ip)->color_standard==0x14,"SET COLOR state is per interpreter");
            xb_interp_free(other);
        }
    }
    xb_interp_free(ip);pal_host_free(pal);
    return TEST_SUMMARY("test-samir-set-color");
}
