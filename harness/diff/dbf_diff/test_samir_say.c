/* Ref: say-get-read.md sections 1-2; real III+ fns-mint/{row,col,prow,pcol}.
 * Literal expectations exercise expressions, formatting, routing and errors. */
#include <stdio.h>
#include <string.h>
#include "test_assert.h"
#include "samir/interp.h"
#include "samir/screen.h"
TEST_HARNESS();
struct pal_host_cfg { uint8_t yy,mm,dd;uint32_t heap; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(samir_pal_t *);
extern int set_register(xb_interp *);
extern int query_register(xb_interp *);
static char stream[1024];static unsigned used;
static void output(samir_pal_t *p,const char *s,uint32_t n)
{ (void)p;if(used+n<sizeof stream){memcpy(stream+used,s,n);used+=n;stream[used]=0;} }
static void mirror(void *u,const char *s,uint32_t n)
{ output(u,s,n); }
static char painted[1024],printed[1024];static unsigned printed_n,paints;static uint8_t paint_row,paint_col;
static int screen(samir_pal_t *p,uint8_t row,uint8_t col,const char *s,uint32_t n)
{
    (void)p;paint_row=row;paint_col=col;paints++;
    if(n>=sizeof painted)return -1;
    memcpy(painted,s,n);painted[n]=0;return 0;
}
static int printer(samir_pal_t *p,const char *s,uint32_t n)
{
    (void)p;if(n+printed_n>=sizeof printed)return -1;
    memcpy(printed+printed_n,s,n);printed_n+=n;printed[printed_n]=0;return 0;
}
static void error(xb_interp *ip,const char *program,int code)
{
    unsigned before=paints;
    CHECK(samir_do(ip,program)!=INTERP_OK && samir_last_error(ip)==code,"SAY exact catalogue error");
    CHECK(paints==before,"rejected SAY paints nothing");
}
static void model_draw(void *u,uint8_t r,uint8_t c,uint8_t a,const char *text,uint32_t n)
{ (void)u;(void)r;(void)c;(void)a;(void)text;(void)n; }
static void value(xb_interp *ip,const char *expr,double expected)
{
    xb_val v;int ec=0;
    CHECK(xb_interp_eval(ip,expr,&v,&ec)==INTERP_OK && v.t==XB_N && v.u.n==expected,expr);
}
int main(void)
{
    samir_pal_t *p=pal_host_make((struct pal_host_cfg){86,7,24,1048576});
    xb_interp *ip=xb_interp_make(p);
    p->conout=output;p->say_screen=screen;p->printout=printer;set_register(ip);query_register(ip);
    xb_interp_ctx(ip)->mirror_output=mirror;xb_interp_ctx(ip)->mirror_user=p;
    value(ip,"ROW()",0);value(ip,"COL()",0);value(ip,"PROW()",0);value(ip,"PCOL()",0);
    CHECK(samir_do(ip,"@ 1+1, 6/2 SAY \"X\"\n")==INTERP_OK,"SAY numeric expressions");
    value(ip,"ROW()",2);value(ip,"COL()",4);
    CHECK(paint_row==2 && paint_col==3 && !strcmp(painted,"X"),"PAL paints expression at coordinates");
    CHECK(!used,"SAY bypasses console transcript and ALTERNATE");
    CHECK(samir_do(ip,"SET DEVICE TO PRINT\n@ 2,3 SAY \"X\"\n")==INTERP_OK,"printer SAY");
    value(ip,"PROW()",2);value(ip,"PCOL()",4);value(ip,"ROW()",2);value(ip,"COL()",4);
    CHECK(!strcmp(printed,"\r\n\r\n   X"),"PAL printer motion and value bytes");
    CHECK(samir_do(ip,"SET DEVICE TO SCREEN\n@ 23,78 SAY \"X\"\n")==INTERP_OK,"screen edge SAY");
    value(ip,"ROW()",23);value(ip,"COL()",79);
    CHECK(samir_do(ip,"@ 0,0 SAY \"\"\n?? \"ab\"\n? \"c\"\n")==INTERP_OK,"question cursor output");
    value(ip,"ROW()",1);value(ip,"COL()",1);
    CHECK(samir_do(ip,"@ 2.9,3.9 SAY .T.\n")==INTERP_OK,"fractional coordinates and logical formatting");
    value(ip,"ROW()",2);value(ip,"COL()",6);
    CHECK(!strcmp(painted,".T."),"SAY shares question logical formatting");
    CHECK(samir_do(ip,"@ 0,0 SAY 42\n")==INTERP_OK && !strcmp(painted,"42"),"SAY shares numeric formatting");
    CHECK(samir_do(ip,"@ 0,0 SAY CTOD(\"07/24/86\")\n")==INTERP_OK && !strcmp(painted,"07/24/86"),"SAY shares date formatting");
    CHECK(samir_do(ip,"@-0.5,-0.5 SAY \"\"\n")==INTERP_OK,"fractional coordinates truncate toward zero");
    value(ip,"ROW()",0);value(ip,"COL()",0);
    error(ip,"@ 25,0 SAY \"X\"\n",30);
    error(ip,"@ 0,80 SAY \"X\"\n",30);
    error(ip,"@ -1,0 SAY \"X\"\n",30);
    error(ip,"@ \"x\",0 SAY \"X\"\n",27);
    error(ip,"@ 0,0 SAY \"X\" PICTURE \"!\"\n",36);
    error(ip,"@ 0,0 SAY \"X\" GET INPUT\n",36);
    error(ip,"SET DEVICE TO UNKNOWN\n",10);
    error(ip,"SET DEVICE SCREEN\n",10);
    {
        const char *functions[]={"ROW(1)","COL(1)","PROW(1)","PCOL(1)"};
        for(unsigned i=0;i<4;i++) { xb_val v;int ec=0;
            CHECK(xb_interp_eval(ip,functions[i],&v,&ec)!=INTERP_OK && ec==11,"extra cursor argument is error 11");
        }
    }
    CHECK(samir_do(ip,"@ 2,3 SAY CHR(10)+\"X\"\n")==INTERP_OK,"SAY embedded LF");
    value(ip,"ROW()",3);value(ip,"COL()",1);
    CHECK(samir_do(ip,"@ 2,3 SAY CHR(13)+\"X\"+CHR(9)+CHR(7)\n")==INTERP_OK,"SAY CR TAB BEL");
    value(ip,"ROW()",2);value(ip,"COL()",8);
    {
        samir_screen model;
        samir_screen_init(&model,NULL,NULL);
        samir_screen_at(&model,23,78,"XYZ\rQ\nR",7);
        CHECK(model.row==24 && model.col==1,"STATUS model full grid and controls");
        CHECK(model.text[23][0]=='Q' && model.text[24][0]=='R',"STATUS model scrolling cells");
        /* PM scope regression: Using III+ U5-16/PDF128 reserves the status
         * rows; the established writer's editing/control policy must survive
         * positioned SAY (say-get-read.md sections 1-2). */
        samir_screen_init(&model,model_draw,NULL);
        model.row=1;model.col=0;
        samir_screen_write(&model,"\b",1);
        CHECK(model.row==0 && model.col==79,"dot-prompt backspace still wraps to previous row");
        model.row=model.col=0;
        samir_screen_write(&model,"\t\f\a",3);
        CHECK(model.row==0 && model.col==3 && !memcmp(model.text[0],"\t\f\a",3),"dot-prompt control-byte policy unchanged");
        samir_screen_at(&model,2,3,"X",1);
        for(unsigned i=0;i<30;i++)samir_screen_write(&model,"\n",1);
        CHECK(model.row==21 && model.col==0,"SAY does not disable status-row reservation");
    }
    xb_interp_free(ip);pal_host_free(p);
    return TEST_SUMMARY("test-samir-say");
}
