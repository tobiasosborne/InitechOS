/* Every shipped route comes from the independent RI subtraction, not the
 * dispatcher's table. Complete command behavior is graded by the other suites. */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main
struct route {char parent[512],name[128],desc[512],path[640];};
static struct route routes[512];static unsigned nr,current;
static unsigned position(unsigned index)
{unsigned n=0;for(unsigned i=0;i<index;i++)if(!strcmp(routes[i].parent,routes[index].parent))n++;return n;}
static void choose(unsigned index)
{
    ev(keyDown,VK_HOME<<8);for(unsigned i=0;i<position(index);i++)right();
    const char *description=!strcmp(routes[index].desc,"\"\"")?"":routes[index].desc;
    unsigned n=(unsigned)strlen(description);if(n>76)n=76;
    CHECK(!memcmp(m_scr[2],description,n),"ROUTES","selected description agrees with independent full-path manifest");
    tk('\r');
}
static void route_case(void)
{
    unsigned ancestors[12],depth=0;const char *parent=routes[current].parent;
    while(strcmp(parent,"/")) {
        unsigned i;for(i=0;i<nr;i++)if(!strcmp(routes[i].path,parent))break;
        if(i==nr||depth==12)_exit(2);
        ancestors[depth++]=i;parent=routes[i].parent;
    }
    tk('/');while(depth)choose(ancestors[--depth]);choose(current);
    CHECK(!(g_mode==M_NOTICE&&g_notice&&!strcmp(g_notice,"Command not implemented")),"ROUTES","every shipped leaf dispatches a retained command");
}
static void unmapped(void)
{
    static const unsigned keys[]={0x3d,0x3e,0x40,0x41,0x42,0x44};
    for(unsigned i=0;i<sizeof keys/sizeof keys[0];i++) {
        ev(keyDown,(int32_t)(keys[i]<<8));
        CHECK(g_mode==M_READY&&!g_dirty&&!g_sheet.ncells,"ROUTES","deferred READY function keys remain unmapped without command-name prompts");
    }
}
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    FILE *f=fopen(argv[2],"r");if(!f)return 2;char line[2048];
    while(fgets(line,sizeof line,f)) {
        if(nr==512)return 2;
        struct route *r=&routes[nr++];
        char *name=strchr(line,'\t');if(!name)return 2;*name++=0;char *desc=strchr(name,'\t');if(!desc)return 2;*desc++=0;
        char *id=strchr(desc,'\t');if(id)*id=0;desc[strcspn(desc,"\r\n")]=0;
        if(strlen(line)>=sizeof r->parent||strlen(name)>=sizeof r->name||strlen(desc)>=sizeof r->desc)return 2;
        strcpy(r->parent,line);strcpy(r->name,name);strcpy(r->desc,desc);
        int n=snprintf(r->path,sizeof r->path,"%s%s%s",line,strcmp(line,"/")?"/":"",name);if(n<0||(unsigned)n>=sizeof r->path)return 2;
    }
    if(ferror(f)||fclose(f)||!nr)return 2;
    m_dir=argv[1];g_ctr=mmap(0,2*sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);if(g_ctr==MAP_FAILED)return 2;
    unsigned leaves=0;
    for(current=0;current<nr;current++) {
        int branch=0;for(unsigned j=0;j<nr;j++)if(!strcmp(routes[current].path,routes[j].parent))branch=1;
        if(!branch){launch(routes[current].path,route_case);leaves++;}
    }
    launch("deferred READY aliases",unmapped);
    printf("Initech 123 menu routes: %u leaves, %d checks, %d failures\n",leaves,g_ctr[0],g_ctr[1]);return g_ctr[1]?1:0;
}
