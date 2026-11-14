/* Validation of native typed calls. Source: the product validators in
 * flair/tbxgate.c, flair/tbxfile.h and flair/cp_service.h. Live captures in
 * tests/live/ independently grade these rules. No product validator is linked. */
static int span(const void *p,size_t n,int image)
{
    uintptr_t a=(uintptr_t)p,end=image?S.image_end:S.block_end;
    if(!n)return 1;
    if(!p)return 0;
    return !S.image_base || (a>=S.image_base&&a<=end&&n<=end-a);
}
static int string_len(const char *p,unsigned max,int image)
{
    for(unsigned i=0;i<max;i++) {
        if(!span(p?p+i:0,1,image))return -1;
        if(!p[i])return (int)i;
    }
    return -1;
}
static int live(void) { return S.registrations&&!S.exit_calls; }
#define LIVE() do { if(!live())return TBX_ERR_NOTREG; } while(0)
/* Chicago 12 NFNT 5478 owTable advances, System 7.0.1. These are metrics,
 * not glyphs; see flair/include/assets/chicago12.h provenance. */
static unsigned advance(unsigned c)
{
    static const uint8_t aw[]={
        4,6,7,10,7,11,10,3,5,5,7,7,4,7,4,7,
        8,8,8,8,8,8,8,8,8,8,4,4,6,8,6,8,
        11,8,8,8,8,7,7,8,8,6,7,9,7,12,9,8,
        8,8,8,7,6,8,8,12,8,8,8,5,7,5,8,8,
        6,8,8,7,8,8,6,8,8,4,6,8,4,12,8,8,
        8,8,6,7,6,8,8,12,8,8,8,5,5,5,8};
    return c==0xc9?14:c==0x12?11:c>=32&&c<=126?aw[c-32]:10;
}
static unsigned width(const char *p,unsigned n)
{ unsigned w=0;for(unsigned i=0;i<n;i++)w+=advance((unsigned char)p[i]);return w; }
static int menu_valid(const tbx_menubar_t *b)
{
    if(!span(b,sizeof *b,1)||b->n_menus>8||
       !span(b->menus,b->n_menus*sizeof *b->menus,1))return 0;
    unsigned x=b->has_apple?33:0;
    for(unsigned k=0;k<b->n_menus;k++) {
        const tbx_menuinfo_t *m=&b->menus[k];int n=string_len(m->title,32,1);
        if(m->menu_id<1||n<1||m->n_items>16||
           !span(m->items,m->n_items*sizeof *m->items,1))return 0;
        x+=width(m->title,(unsigned)n)+18;
        for(unsigned i=0;i<m->n_items;i++)if(string_len(m->items[i].text,32,1)<0)return 0;
    }
    return x<=640;
}
static int alert_valid(const char *t,const char *d,const char *c,const char *o)
{
    int len=string_len(t,144,1),start=0;unsigned n=0,nb=0;
    if(len<=0)return 0;
    for(int i=0;i<=len;i++)if(i==len||t[i]=='\r') {
        if(++n>3||i-start<=0||i-start>=48||width(t+start,(unsigned)(i-start))>285)return 0;
        start=i+1;
    }
    const char *btn[]={d,c,o};
    for(unsigned i=0;i<3&&btn[i];i++) {
        int size=string_len(btn[i],16,1);
        if(size<=0||width(btn[i],(unsigned)size)>(i==2?68u:43u))return 0;
        nb++;
    }
    return nb&&!(nb<3&&o);
}
static int path_valid(const char *p) { return string_len(p,128,0)>0; }
/* The native fixture mounts one data volume, A:, like the smoke disk. */
static int32_t volume_error(const char *p)
{ return p[0]&&p[1]==':'&&p[0]!='A'&&p[0]!='a'?TBX_ERR_DOS(15):0; }
static int device(const char *p)
{
    if(p[0]&&p[1]==':')p+=2;
    const char *s=p;for(;*p;p++)if(*p=='\\'||*p=='/')s=p+1;
    char name[9];unsigned i=0;
    while(s[i]&&s[i]!='.'&&i<8){char c=s[i];name[i]=c>='a'&&c<='z'?c-32:c;i++;}
    while(i&&name[i-1]==' ')i--;
    name[i]=0;
    return !strcmp(name,"CON")||!strcmp(name,"PRN")||!strcmp(name,"AUX")||!strcmp(name,"NUL")||
        !strcmp(name,"CLOCK$")||(i==4&&name[3]>='1'&&
        ((!strncmp(name,"COM",3)&&name[3]<='4')||(!strncmp(name,"LPT",3)&&name[3]<='3')));
}
void tbx_host_arena(const void *base,size_t module_bytes,size_t block_bytes)
{
    uintptr_t a=(uintptr_t)base;
    if(!a||module_bytes>block_bytes||block_bytes>UINTPTR_MAX-a)abort();
    S.image_base=a;S.image_end=a+module_bytes;S.block_end=a+block_bytes;
}
