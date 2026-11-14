/* Approved dead-command split, GM6. This manifest is deliberately independent
 * of product tables. Historical WP references remain untouched. Each route is
 * sent through the tenant's event dispatcher, including absent prompt keys. */
#define main iword_old_main
#include "test_iword_face.c"
#undef main
static const unsigned removed[4] = {
    (1u<<2)|(1u<<8),
    (1u<<2)|(1u<<3)|(1u<<4)|(1u<<5)|(1u<<6)|(1u<<8),
    (1u<<0)|(1u<<4)|(1u<<5)|(1u<<6)|(1u<<7)|(1u<<8)|(1u<<9),
    (1u<<0)|(1u<<1)|(1u<<2)|(1u<<4)|(1u<<5)|(1u<<6)|(1u<<8)|(1u<<9)
};
static const int mods[4]={0,SHIFT,ALT,CTRL};
static void fresh(void){ g_mode=M_EDIT;g_reveal=0;doc_reset();paint_all(); }
static void absent(const char *s)
{
    for(int r=0;r<24;r++)CHECK(!strstr(m_scr[r],s),"omission","removed advertisement absent");
}
static void unchanged(int ch,int mode)
{
    char before[24][77]; memcpy(before,m_scr,sizeof before);
    uint32_t cur=g_ed.cur,len=g_ed.doc.len;int dirty=g_ed.dirty;
    tk(ch);
    CHECK(g_mode==mode && g_ed.cur==cur && g_ed.doc.len==len && g_ed.dirty==dirty && !memcmp(before,m_scr,sizeof before),"omission","absent number keeps prompt, bytes and cursor");
}
static void sweep(void)
{
    for(int b=0;b<2;b++)for(int m=0;m<4;m++)for(int k=0;k<10;k++)if(removed[m]&(1u<<k)){
        fresh();tks("Keep");fkey(0x4b,0);if(b)fkey(F4,ALT);
        char before[24][77];memcpy(before,m_scr,sizeof before);
        uint32_t cur=g_ed.cur,len=g_ed.doc.len;int dirty=g_ed.dirty;
        fkey(0x3b+k,mods[m]);
        CHECK(g_mode==M_EDIT && g_ed.cur==cur && g_ed.doc.len==len && g_ed.dirty==dirty && !memcmp(before,m_scr,sizeof before),"omission","removed key produces no name prompt or state change");
    }
    fresh();fkey(F8,CTRL);CHECK(row_is(23,"1 Size; 2 Appearance; 3 Normal: 0"),"omission","Font manifest");unchanged('4',M_FONT);unchanged('5',M_FONT);
    fresh();fkey(F4,ALT);fkey(F4,CTRL);CHECK(row_is(23,"Move: 1 Block: 0"),"omission","Move manifest");unchanged('2',M_MOVE);unchanged('3',M_MOVE);tk('1');
    CHECK(row_is(23,"1 Move; 2 Copy; 3 Delete: 0"),"omission","Block manifest");unchanged('4',M_MOVEBLK);
    fresh();fkey(F4,CTRL);tk('4');CHECK(row_is(23,"Retrieve: 1 Block: 0"),"omission","Retrieve manifest");unchanged('2',M_RETMENU);unchanged('3',M_RETMENU);
    fresh();fkey(F8,SHIFT);absent("Page");absent("Document");absent("Other");absent("Hyphen");absent("Tab Set");unchanged('2',M_FORMAT);unchanged('3',M_FORMAT);unchanged('4',M_FORMAT);tk('1');
    const char *bad[]={"Hyphen","Height","Numbering","Tab Set","Widow","Zone"};
    for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++)absent(bad[i]);
    for(const char *p="124589";*p;p++)unchanged(*p,M_FMTLINE);
    fresh();fkey(F1,SHIFT);absent("2 Display");absent("Initial Settings");absent("Keyboard Layout");absent("Location of Files");tk('3');tk('1');absent("Timed");
    fresh();fkey(F1,SHIFT);tks("38");CHECK(row_is(4,"Display and Entry of Numbers: Inches"),"omission","entry units read only");unchanged('1',M_UNITS);
    fresh();tks("Keep");fkey(F4,ALT);fkey(F10,0);CHECK(g_mode==M_EDIT,"omission","block F10 has no Save route");menu(2);CHECK(g_mode==M_EDIT,"omission","block native Save has no route");
}
static void unit_capture(const char *group,const char *name)
{
    char path[512],expected[81];unsigned char vga[4000];
    snprintf(path,sizeof path,"tests/fixtures/status-units/%s/%s.vga",group,name);
    FILE *f=fopen(path,"rb");CHECK(f!=NULL,"units","independent capture exists");if(!f)return;
    size_t n=fread(vga,1,sizeof vga,f);fclose(f);CHECK(n==4000,"units","whole raw VGA capture");if(n!=4000)return;
    for(int c=0;c<80;c++)expected[c]=(char)vga[2*(24*80+c)];
    expected[80]=0;
    char *s=strstr(expected,"Doc 1");CHECK(s!=NULL,"units","capture status exists");if(!s)return;
    char *end=s+strlen(s);while(end>s&&end[-1]==' ')*--end=0;
    if(!strstr(m_scr[23],s))fprintf(stderr,"expected [%s], got [%s]\n",s,m_scr[23]);
    CHECK(strstr(m_scr[23],s)!=NULL,"units","status equals real unrounded-coordinate capture");
}
static void choose_unit(char u){fkey(F1,SHIFT);tks("382");tk(u);fkey(F7,0);fkey(F7,0);fkey(F7,0);}
static void units(void)
{
    const char *us="ucpwi";char name[32];
    fresh();tks("ABCDE\rXYZ");
    for(const char *p=us;*p;p++){choose_unit(*p);snprintf(name,sizeof name,"base_%c",*p);unit_capture("word-new-probes",name);}
    fkey(F8,SHIFT);tks("16");tks("1.25\r");fkey(F7,0);tks("\rZ");
    for(const char *p=us;*p;p++){choose_unit(*p);snprintf(name,sizeof name,"fraction_%c",*p);unit_capture("word-new-probes",name);}
    ev(keyDown,(0x1c<<8)|'\r',CTRL);tk('Z');
    for(const char *p=us;*p;p++){choose_unit(*p);snprintf(name,sizeof name,"page_%c",*p);unit_capture("word-new-probes",name);}
    fresh();fkey(F8,SHIFT);tks("17");tks("1.25\r\r");fkey(F7,0);choose_unit('u');unit_capture("word-size-probes","margin");
    for(char c='1';c<='7';c++){fkey(F8,CTRL);tk('1');tk(c);tks("ABC\r");snprintf(name,sizeof name,"size_%c",c);unit_capture("word-size-probes",name);}
    for(const char *p=us;*p;p++){
        choose_unit(*p);const tbx_host_file_t *f=tbx_host_file("IWORD.SET");
        CHECK(f&&f->size==8&&!memcmp(f->data,"IWS1i",5)&&f->data[5]==(unsigned char)*p&&f->data[7]==0,"units","literal IWS1 status preference bytes");
        settings_defaults();settings_load();CHECK(g_settings[4]=='i'&&g_settings[5]==(unsigned char)*p,"units","all status choices reload independently");
    }
}
static void name_search(void)
{
    static const m_dirent_t entries[]={
        {"","ALDIR",0x10,0,0,0},{"","ZZDIR",0x10,0,0,0},
        {"","AONE",0x20,0,0,344},{"","ATWO",0x20,0,0,344},{"","BONE",0x20,0,0,344}
    };
    const char *queries[]={"a","at","az","z","0","\\zz","/al","At"};
    const char *expected[]={"AONE","ATWO","ATWO","BONE","AONE","ZZDIR","ALDIR","ATWO"};
    tbx_host_reset();CHECK(tbx_app_main()==0,"name","fresh tenant");
    fresh();tks("Keep this document");uint32_t len=g_ed.doc.len,cur=g_ed.cur;
    m_dirents=entries;m_ndirents=sizeof entries/sizeof entries[0];
    fkey(0x3f,0);tk('\r');
    CHECK(row_is(22,"1 Retrieve; 2 Delete; 3 Move/Rename;")&&row_is(23,"6 Look; 8 Copy; N Name Search: 6"),"name","six real numbered List Files routes");
    for(unsigned q=0;q<sizeof queries/sizeof queries[0];q++){
        tk('n');tks(queries[q]);
        CHECK(g_mode==M_LIST&&g_lf_sel>=2&&!strcmp(g_lf_ent[g_lf_sel-2].name,expected[q]),"name","captured case/prefix/class/closest selection");
        CHECK(strstr(m_scr[23],"Name Search; Enter or arrows to Exit")!=NULL,"name","captured search prompt");
        fkey(F7,0);CHECK(g_mode==M_LIST&&row_is(23,"6 Look; 8 Copy; N Name Search: 6"),"name","F7 leaves search, keeps list");
    }
    tks("natz");tk(8);CHECK(g_lf_sel>=2&&!strcmp(g_lf_ent[g_lf_sel-2].name,"ATWO"),"name","backspace recomputes prefix");
    fkey(0x50,0);CHECK(g_mode==M_LIST&&!strstr(m_scr[23],"arrows to Exit"),"name","arrow exits search and moves normally");
    tks("na");fkey(ESC,0);CHECK(g_mode==M_LIST,"name","Esc dismisses search");
    tks("na");tk('\r');CHECK(g_mode==M_LIST,"name","Enter dismisses search without opening");
    CHECK(g_ed.doc.len==len&&g_ed.cur==cur&&g_ed.dirty,"name","search preserves edit buffer and cursor");
    for(const char *p="4579";*p;p++)unchanged(*p,M_LIST);
    m_dirents=NULL;m_ndirents=0;
}
int main(void)
{
    static int ctr[2];g_ctr=ctr;if(tbx_app_main())return 2;sweep();units();name_search();
    printf("Word commands: %d checks, %d failures\n",ctr[0],ctr[1]);return ctr[1]?1:0;
}
