/* Independent literal screen expectations: Using III+ Fig 5-17 U5-259/PDF 371,
 * reserved row 22 U5-16/PDF 128. Manual-derived, no local STATUS golden. */
#include <stdio.h>
#include <string.h>
#include "test_assert.h"
#include "samir/screen.h"
#include "samir/interp.h"
#include "samir/workarea.h"
TEST_HARNESS();
static char cells[24][81];static uint8_t attrs[24][80];
static void draw(void *u,uint8_t r,uint8_t col,uint8_t attr,const char *s,uint32_t n)
{
    (void)u;
    CHECK(r<24 && col+n<=80,"bounded screen paint");
    if(r<24 && col+n<=80) { memcpy(cells[r]+col,s,n);memset(attrs[r]+col,attr,n);cells[r][80]=0; }
}
static void status_check(const char *db,const char *rec)
{
    CHECK(!memcmp(cells[22],"Command Line",12),"status: Command Line at col 0");
    CHECK(!memcmp(cells[22]+17,"(C:>)",5),"status: drive at col 17");
    CHECK(!memcmp(cells[22]+23,db,strlen(db)),"status: database at col 23");
    CHECK(!memcmp(cells[22]+46,"Rec: ",5),"status: Rec label at col 46");
    CHECK(!memcmp(cells[22]+51,rec,strlen(rec)),"status: record position/count at col 51");
    CHECK(!memcmp(cells[23]+24,"Enter a dBASE III PLUS command.",30),"status: guidance at col 24");
    CHECK(attrs[22][0]==0x70 && attrs[22][79]==0x70,"status: full reverse band");
    CHECK(attrs[23][24]==0x07,"status: normal guidance");
}
struct pal_host_cfg { uint8_t yy,mm,dd;uint32_t heap; };
samir_pal_t *pal_host_make(struct pal_host_cfg);
void pal_host_free(samir_pal_t *);
int set_register(xb_interp *);
int query_register(xb_interp *);
static int enabled;static uint32_t position,total;static char database[256];
static void metadata(samir_pal_t *p,int on,const char *name,uint32_t rec,uint32_t count)
{ (void)p;enabled=on;position=rec;total=count;snprintf(database,sizeof(database),"%s",name); }
static void integration(const char *corpus)
{
    samir_pal_t *p=pal_host_make((struct pal_host_cfg){85,1,1,1048576});
    xb_interp *ip=xb_interp_make(p);char path[512];
    CHECK(ip!=NULL,"status: interpreter exists");if(!ip) return;
    p->status=metadata;set_register(ip);query_register(ip);
    samir_status_refresh(ip);CHECK(!enabled,"status: interpreter default OFF");
    snprintf(path,sizeof(path),"%s/mint/work/TOURS.DBF",corpus);
    CHECK(wa_set_open(xb_interp_env(ip),1,path,"OTHER",NULL)==0,"status: real TOURS opened read-only");
    CHECK(samir_do(ip,"SET STATUS ON\nGO 2\n")==INTERP_OK,"status: dispatch and navigation");
    samir_status_refresh(ip);
    CHECK(enabled && position==2 && total==30,"status: live TOURS record 2/30");
    CHECK(strstr(database,"TOURS.DBF")!=NULL && !strstr(database,"OTHER"),"status: filename rather than alias");
    snprintf(path,sizeof(path),"%s/mint/work/CLIENTS.DBF",corpus);
    CHECK(wa_set_open(xb_interp_env(ip),2,path,"CLIENTS",NULL)==0,"status: real CLIENTS opened read-only");
    wa_select(xb_interp_env(ip),2);samir_status_refresh(ip);
    CHECK(position==1 && total==49 && strstr(database,"CLIENTS.DBF"),"status: selected work area updates 1/49");
    wa_close(xb_interp_env(ip),2);samir_status_refresh(ip);
    CHECK(position==0 && total==0 && !database[0],"status: close clears database and count");
    CHECK(samir_do(ip,"SET STATUS OFF\n")==INTERP_OK,"status: dispatch OFF");
    samir_status_refresh(ip);CHECK(!enabled,"status: OFF reaches PAL");
    xb_interp_free(ip);pal_host_free(p);
}
int main(int argc,char **argv)
{
    samir_screen s;char saved[81];int i;
    memset(cells,' ',sizeof(cells));
    samir_screen_init(&s,draw,NULL);
    CHECK(!s.active,"default STATUS OFF without CONFIG.DB");
    samir_screen_status(&s,1,'C',"CLIENTS",1,49);status_check("CLIENTS","1/49");
    memcpy(saved,cells[22],81);
    for(i=0;i<100;i++) samir_screen_write(&s,"record output\n",14);
    CHECK(!memcmp(saved,cells[22],80),"scroll cannot overwrite reserved status");
    CHECK(s.row==21,"command viewport stops above row 22");
    samir_screen_status(&s,1,'C',"TOURS",30,30);status_check("TOURS","30/30");
    samir_screen_status(&s,1,'C',"",0,0);status_check("","0/0");
    samir_screen_status(&s,0,'C',"",0,0);
    CHECK(!s.active,"SET STATUS OFF releases viewport");
    CHECK(!memcmp(cells[22],"                                                                                ",80),"OFF erases band");
    CHECK(!memcmp(cells[23],"                                                                                ",80),"OFF erases guidance");
    CHECK(attrs[22][0]==0x07,"OFF clears reverse attribute");
    if(argc!=2) return 2;
    integration(argv[1]);
    return TEST_SUMMARY("test-samir-status");
}
