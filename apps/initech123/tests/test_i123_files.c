/* R2.2 Reference 5-13..18 and fixtures/files; literal DOS directory entries,
 * raw captured metadata, and complete worksheet bytes are the independent oracle. */
#include "tbx.h"
static int listing_fixture;
int32_t fixture_find(const char *,uint32_t,void *,uint32_t);
#define tbx_file_find fixture_find
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main
#undef tbx_file_find
#include <sys/stat.h>
int32_t fixture_find(const char *path,uint32_t attr,void *records,uint32_t max)
{
    if(!listing_fixture||max!=512)return tbx_file_find(path,attr,records,max);
    unsigned count=listing_fixture==2?512u:listing_fixture==3?511u:173u;
    for(unsigned i=0;i<count&&i<max;i++) {
        tbx_find_rec_t r={0};unsigned item=count-1-i;
        r.date=6242;r.time=19392;r.size=8;
        if(listing_fixture>=2)snprintf(r.name,sizeof r.name,"X%03u.PRN",item);
        else if(item<170)snprintf(r.name,sizeof r.name,"P%02u.PRN",item+1);
        else if(item==170)strcpy(r.name,"PRINT.PRN");
        else {strcpy(r.name,item==171?"EMPTY":"SUB");r.attr=16;}
        memcpy((char *)records+i*sizeof r,&r,sizeof r);
    }
    return (int32_t)(count<max?count:max);
}
static unsigned category;
static void fixture(void)
{
    static const char *names[]={"ALPHA.WKS","BASE.WK1","BETA.WK1","DECOY.WK2","GAMMA.WK3","GRAPH.PIC","P01.PRN","P02.PRN","PRINT.PRN","RONLY.TXT","HIDDEN.TXT","SUB","EMPTY"};
    CHECK(tbx_host_put_file("FILES",0,0),"FILES","directory fixture");((tbx_host_file_t *)tbx_host_file("FILES"))->attr=16;
    for(unsigned i=0;i<sizeof names/sizeof names[0];i++) {
        char path[80];snprintf(path,sizeof path,"FILES\\%s",names[i]);
        CHECK(tbx_host_put_file(path,"sample\r\n",8),"FILES","sentinel fixture");
        tbx_host_file_t *f=(tbx_host_file_t *)tbx_host_file(path);f->date=6242;f->time=19392;
        if(i==9)f->attr=1;else if(i==10)f->attr=2;else if(i>10){f->attr=16;f->size=0;}
    }
    tks("'KEEP\r/fsBEFORE\r/fdFILES\r");
    CHECK(g_mode==M_READY&&!g_dirty,"FILES","session Directory accepts an existing relative directory");
}
static void metadata(const char *capture)
{
    char path[256];unsigned char vga[4000];
    snprintf(path,sizeof path,"tests/fixtures/files/%s.vga",capture);FILE *f=fopen(path,"rb");if(!f||fread(vga,1,sizeof vga,f)!=sizeof vga)_exit(2);fclose(f);
    char expected[77];for(unsigned i=0;i<76;i++)expected[i]=(char)vga[2*(2*80+i)];expected[76]=0;
    CHECK(!memcmp(m_scr[2],expected,76),"FILES","selected name/date/time/size match captured raw metadata row");
}
static void command_case(void)
{
    static const char letters[]="wpgo";
    static const char *targets[]={"FILES\\ALPHA.WKS","FILES\\P01.PRN","FILES\\GRAPH.PIC","FILES\\ALPHA.WKS"};
    char input[64];fixture();snprintf(input,sizeof input,"/fl%c",letters[category]);tks(input);
    CHECK(line_has(0,"FILES")&&line_has(1,"Enter extension of files to list:"),"FILES","List opens full file overlay");
    if(category==0){metadata("steady3/list_w");CHECK(line_has(3,"ALPHA.WKS")&&line_has(3,"DECOY.WK2"),"FILES","real worksheet wildcard includes WK2");}
    if(category==1)metadata("steady3/list_p");
    if(category==2)metadata("steady3/list_g");
    if(category==3)metadata("steady3/list_o");
    tk('\r');CHECK(g_mode==M_READY&&!g_dirty,"FILES","List Return restores clean worksheet");
    snprintf(input,sizeof input,"/fe%c",letters[category]);tks(input);
    CHECK(line_has(0,"FILES")&&line_has(1,"Enter name of file to erase:"),"FILES","Erase opens filtered chooser");
    tk('\r');CHECK(line_has(1,"No  Yes")&&line_has(2,"Cancel command -- Do not erase the file"),"FILES","confirmation defaults to No");
    tk('\r');CHECK(file_exists(targets[category])&&!g_dirty,"FILES","No preserves chosen file and dirty state");
    snprintf(input,sizeof input,"/fe%c\ry",letters[category]);tks(input);
    CHECK(g_mode==M_READY&&!file_exists(targets[category])&&!g_dirty,"FILES","only confirmed chosen file disappears");
    CHECK(file_exists("FILES\\RONLY.TXT")&&file_exists("FILES\\HIDDEN.TXT")&&file_exists("FILES\\PRINT.PRN"),"FILES","other sentinels survive");
    tks("/fd\\\r/fsAFTER\r");
}
static void errors(void)
{
    fixture();tks("/feoRONLY.TXT\ry");
    CHECK(g_mode==M_NOTICE&&g_notice&&!strcmp(g_notice,"File or directory is read-only"),"FILES","real read-only refusal");
    CHECK(file_exists("FILES\\RONLY.TXT")&&!g_dirty,"FILES","read-only refusal preserves file and sheet");
    tk(3);tks("/feoHIDDEN.TXT\r");ev(keyDown,VK_RIGHT<<8);
    CHECK(line_has(2,"Erase the file"),"FILES","captured Yes description");tk('\r');
    CHECK(!file_exists("FILES\\HIDDEN.TXT")&&!g_dirty,"FILES","explicit confirmed hidden file deletion works without listing it");
    tk(3);tks("/fewZZZ\ry");CHECK(g_mode==M_NOTICE&&g_notice&&!strcmp(g_notice,"File does not exist"),"FILES","real absent-file refusal");
    tk(3);tks("/fdBOGUS\r");CHECK(g_mode==M_NOTICE&&g_notice&&!strcmp(g_notice,"Directory does not exist"),"FILES","invalid relative directory refused");
    tk(3);tks("/fdZ:\\\r");CHECK(g_mode==M_NOTICE&&g_notice&&!strcmp(g_notice,"Directory does not exist"),"FILES","unsupported drive refused");
    tk(3);tks("/feoZ:\\X.TXT\r");CHECK(line_has(1,"No  Yes"),"FILES","missing-drive explicit deletion still asks No/Yes");
    tk('y');CHECK(g_mode==M_NOTICE&&g_notice&&!strcmp(g_notice,"File does not exist"),"FILES","confirmed missing-drive deletion reports captured refusal");
    tk(3);tks("/fd");CHECK(line_has(1,"A:\\FILES"),"FILES","errors preserve old session directory");
    tk(3);tks("/fdSUB\r/fd");CHECK(line_has(1,"A:\\FILES\\SUB"),"FILES","relative directory is validated and normalized");
    tk(3);tks("/fd..\r/fd");CHECK(line_has(1,"A:\\FILES"),"FILES","parent path resolves within root");
    tk(3);tks("/fdEMPTY\r/flo");CHECK(line_has(0,"EDIT"),"FILES","empty chooser is editable, no fabricated entry");
    tk(3);CHECK(!g_dirty,"FILES","browsing/cancel/errors preserve dirty state");
}
static void captured_page(const char *capture)
{
    char path[256];unsigned char vga[4000];
    snprintf(path,sizeof path,"tests/fixtures/files/boundaries/%s.vga",capture);
    FILE *f=fopen(path,"rb");if(!f||fread(vga,1,sizeof vga,f)!=sizeof vga)_exit(2);fclose(f);
    for(unsigned r=2;r<24;r++) {
        char expected[77];for(unsigned x=0;x<76;x++)expected[x]=(char)vga[2*(r*80+x)];expected[76]=0;
        CHECK(!memcmp(m_scr[r],expected,76),"FILES","full page/metadata agrees with raw R2.2 capture");
    }
}
static void pages(void)
{
    /* Give the native window the reference's 25 text lines. The default
     * Mac content height differs; paging otherwise uses its actual height. */
    m_ch=400;ev(updateEvt,0);
    listing_fixture=1;tks("/flp");captured_page("page_first");
    ev(keyDown,VK_PGDN<<8);captured_page("page_second");
    ev(keyDown,VK_PGDN<<8);captured_page("page_last");
    ev(keyDown,VK_PGUP<<8);captured_page("page_back");
    ev(keyDown,VK_HOME<<8);tk('\r');CHECK(g_mode==M_READY&&!g_dirty,"FILES","paging restores unchanged clean worksheet");
    listing_fixture=3;tks("/flp");ev(keyDown,VK_END<<8);
    CHECK(line_has(0,"FILES")&&line_has(2,"X510.PRN"),"FILES","complete 511-entry list includes final file");
    ev(keyDown,VK_HOME<<8);tk('\r');
    listing_fixture=2;tks("/flo");
    CHECK(g_mode==M_NOTICE&&g_notice&&!strcmp(g_notice,"Too many files (511 maximum)"),"FILES","512th sentinel reports native limit instead of incomplete list");
    CHECK(!g_dirty&&!g_sheet.ncells,"FILES","capacity refusal preserves worksheet");
}
static void load_fixture(const char *path,const char *target)
{
    unsigned char bytes[8192];FILE *f=fopen(path,"rb");if(!f)_exit(2);
    size_t n=fread(bytes,1,sizeof bytes,f);if(ferror(f)||fclose(f))_exit(2);
    CHECK(tbx_host_put_file(target,bytes,(uint32_t)n),"FILES","install independent WK1/path fixture");
}
static void directory_io(void)
{
    fixture();tk(3);tks("/fd\\\r");
    load_fixture("tests/fixtures/zero/left/BEFORE.WK1","SAME.WK1");
    load_fixture("tests/fixtures/fill/basic/RESULT.WK1","FILES\\SAME.WK1");
    load_fixture("tests/fixtures/files/import/CR/INPUT.PRN","FILES\\INPUT.PRN");
    CHECK(tbx_host_put_file("INPUT.PRN","ROOT\r\n",6),"FILES","same input basename in other directory");
    tks("/frSAME\r/fdFILES\r/frSAME\r");
    char value[80];cell(0,0,value,sizeof value);CHECK(!strcmp(value,"10"),"FILES","Retrieve uses chosen directory's SAME, not root SAME");
    tks("/fsSAVED\r");CHECK(file_exists("FILES\\SAVED.WK1")&&!file_exists("SAVED.WK1"),"FILES","Save uses chosen directory");
    ev(keyDown,VK_F5<<8);tks("A1\r");
    tks("/fitINPUT.PRN\r");cell(0,0,value,sizeof value);
    CHECK(!strcmp(value,"'one"),"FILES","Import uses chosen directory's input, not root input");
    tks("/fewSAVED.WK1\ry");CHECK(!file_exists("FILES\\SAVED.WK1")&&file_exists("SAME.WK1"),"FILES","Erase uses chosen directory and preserves root file");
    tks("/fsIMPORTED\r");
    const tbx_host_file_t *root=tbx_host_file("SAME.WK1");unsigned char original[8192];
    FILE *f=fopen("tests/fixtures/zero/left/BEFORE.WK1","rb");if(!f)_exit(2);size_t n=fread(original,1,sizeof original,f);fclose(f);
    CHECK(root&&root->size==n&&!memcmp(root->data,original,n),"FILES","all five File operations leave same-named root worksheet byte-identical");
    tk(3);tks("/fd\\\r/frSAME\r/fsREADBEF\r");
    tbx_host.file_error[TBX_FILE_READ]=5;tks("/fdFILES\r/fitINPUT.PRN\r");
    cell(0,0,value,sizeof value);CHECK(!strcmp(value,"0")&&!g_dirty,"FILES","reported import read error preserves the whole saved worksheet");
    unsigned open=0;for(unsigned i=0;i<TBX_HOST_HANDLES;i++)if(tbx_host.handles[i].used)open++;
    CHECK(!open,"FILES","read-error import closes its handle");
    tbx_host.file_error[TBX_FILE_READ]=0;tk(3);tks("/fsREADAFT\r");
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    char dir[512];
    g_ctr=mmap(0,2*sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);if(g_ctr==MAP_FAILED)return 2;
    for(category=0;category<4;category++) {
        snprintf(dir,sizeof dir,"%s/c%u",argv[1],category);if(mkdir(dir,0700))return 2;m_dir=dir;
        launch("filtered List/Erase",command_case);
    }
    snprintf(dir,sizeof dir,"%s/errors",argv[1]);if(mkdir(dir,0700))return 2;m_dir=dir;
    launch("directory and refusal cases",errors);
    snprintf(dir,sizeof dir,"%s/pages",argv[1]);if(mkdir(dir,0700))return 2;m_dir=dir;
    launch("complete pages and native limit",pages);
    snprintf(dir,sizeof dir,"%s/routing",argv[1]);if(mkdir(dir,0700))return 2;m_dir=dir;
    launch("Directory routes all file operations",directory_io);
    printf("Initech 123 files: %d checks, %d failures\n",g_ctr[0],g_ctr[1]);return g_ctr[1]?1:0;
}
