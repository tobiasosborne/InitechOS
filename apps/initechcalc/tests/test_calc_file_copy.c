/* File > Save, the FILE COPY box and the saved document. The REAL Calc tenant
 * over the real Initech 123 core, its Toolbox calls recorded by sdk/host.
 * Ref: PRD 6.3/6.5 and Appendix A (File > Save opens the FILE COPY box
 * captioned "Saving tables to disk..."); toolbox_gate.h Sec 12/16 (asked by
 * the trap, answered by TBX_EVT_FILECOPY); README.md "The ledger document".
 * Expected bytes are the shipped fixture data/LEDGER.TXT, edited here by
 * literal text substitution; never Calc's serializer. The window insets
 * (6, 22, 27, 43) are FLAIR's CalcDocContentRect, as sdk/host uses them. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tbx_host.h"
static int drag_h[3], drag_v[3], collapsed[3];
static int32_t fake_origin(int32_t w)
{
    if(w<1||w>tbx_host.nwindows) return TBX_ERR_BADARG;
    const int l=tbx_host.windows[w-1].l+6+drag_h[w], t=tbx_host.windows[w-1].t+22+drag_v[w];
    return (int32_t)(((uint32_t)(uint16_t)t<<16)|(uint16_t)l);
}
static int32_t fake_size(int32_t w)
{
    if(w<1||w>tbx_host.nwindows) return TBX_ERR_BADARG;
    if(collapsed[w]) return 0;
    const int cw=tbx_host.windows[w-1].r-tbx_host.windows[w-1].l-27, ch=tbx_host.windows[w-1].b-tbx_host.windows[w-1].t-43;
    return (int32_t)(((uint32_t)(uint16_t)ch<<16)|(uint16_t)cw);
}
#define tbx_content_origin fake_origin
#define tbx_content_size fake_size
#include "../calc.c"
#undef tbx_content_origin
#undef tbx_content_size

static unsigned checks, failures;
#define CHECK(c, s) do { checks++; if (!(c)) { failures++; fprintf(stderr,"FAIL file-copy routing: %s\n",s); } } while(0)
static unsigned calls(const char *name) { unsigned n=0;for(unsigned i=0;i<tbx_host.nops;i++)if(!strcmp(tbx_host.ops[i].name,name))n++;return n; }
static char shipped_bytes[4096]; static size_t shipped_len;
static char staged[8192]; static size_t staged_len;
static unsigned copies;
static void event(int32_t what,int32_t msg)
{
    tbx_event_t e={what,msg,0,0,0,0};
    unsigned before=calls("tbx_file_copy");
    if(what==TBX_EVT_FILECOPY&&msg>=0) {   /* the transfer's own completion: copy, then answer */
        const tbx_host_file_t *f=tbx_host_file(tbx_host.copy_src);
        if(f) tbx_host_put_file(tbx_host.copy_dst,f->data,f->size);
    }
    if(!tbx_host_push_event(&e)) exit(2);
    (void)tbx_host_dispatch(tbx_app_event);
    if(calls("tbx_file_copy")>before) {    /* keep what was handed to FILE COPY */
        copies++;
        const tbx_host_file_t *f=tbx_host_file(tbx_host.copy_src);
        staged_len=0; if(f&&f->size<sizeof staged) { memcpy(staged,f->data,f->size); staged_len=f->size; }
    }
}
static void key(int ch) { event(keyDown,ch); }
static int staged_is(const char *want,size_t n) { return staged_len==n&&!memcmp(staged,want,n); }
/* fixture with one literal substitution */
static size_t edited(char *out,const char *from,const char *to)
{
    const char *p=strstr(shipped_bytes,from); size_t a=(size_t)(p-shipped_bytes);
    if(!p) return 0;
    memcpy(out,shipped_bytes,a); strcpy(out+a,to); strcpy(out+a+strlen(to),p+strlen(from));
    return strlen(out);
}
#define SAVE TBX_MENU_WORD(129,1)
#define QUIT TBX_MENU_WORD(129,2)
/* An ordinary session: open what is on the disk, type `typed` into the first
 * cell, Save (approving a replacement), let the copy land, then start Calc
 * again on the disk as the Save left it. Returns the alerts the second launch
 * raised (0: the saved document read cleanly); *saved is 0 when no copy was
 * asked for. */
static unsigned reopen_alerts;
static double stored; static int stored_ok;   /* the cell's number before Save */
static int first_cell(double *x);
static int save_and_reopen(const char *shipped_doc,const char *tables,const char *typed,int *saved)
{
    static char keep[8192]; size_t kl=0;
    tbx_host_reset(); copies=0; memset(drag_h,0,sizeof drag_h); memset(drag_v,0,sizeof drag_v);
    if(shipped_doc) tbx_host_put_file("APPS\\LEDGER.TXT",shipped_doc,(uint32_t)strlen(shipped_doc));
    if(tables) tbx_host_put_file("APPS\\TABLES.TXT",tables,(uint32_t)strlen(tables));
    if(tbx_app_main()!=0) return -1;
    if(tbx_host.alerts) event(TBX_EVT_ALERT,TBX_ALERT_DEFAULT);   /* dismiss the notice */
    i123_set_cursor(&g_sheet,0,0);
    for(const char *p=typed;*p;p++) key(*p);
    key('\r');
    stored_ok=first_cell(&stored);
    int a=tbx_host.alerts;
    event(TBX_EVT_MENU,SAVE);
    if(tbx_host.alerts>a&&strstr(tbx_host.alert_text,"replace")) event(TBX_EVT_ALERT,TBX_ALERT_DEFAULT);
    *saved=copies==1;
    if(*saved) event(TBX_EVT_FILECOPY,(int32_t)staged_len);
    if(*saved&&g_dirty) return -1;
    const tbx_host_file_t *t=tbx_host_file("APPS\\TABLES.TXT");
    if(t&&t->size<sizeof keep) { memcpy(keep,t->data,t->size); kl=t->size; }
    tbx_host_reset();
    if(shipped_doc) tbx_host_put_file("APPS\\LEDGER.TXT",shipped_doc,(uint32_t)strlen(shipped_doc));
    if(t) tbx_host_put_file("APPS\\TABLES.TXT",keep,(uint32_t)kl);
    if(tbx_app_main()!=0) return -1;
    reopen_alerts=tbx_host.alerts;
    return 0;
}
/* The stored number itself (the worksheet's numeric accessor), never the
 * 15-digit control-panel text, which would hide a rounded save
 * (initech-qdscg). */
static int first_cell(double *x)
{
    i123_val_t v;
    i123_cell_value(&g_sheet,0,0,&v);
    if(v.t!=I123_V_NUM) return 0;
    *x=v.n; return 1;
}
/* Row 1's first cell as the saved TABLES.TXT holds it, read with the C
 * library's strtod, independent of Calc and Initech 123: "(G) " and the
 * number, sign at the tail. */
static int saved_first_cell(double *x)
{
    static char text[8192];
    const tbx_host_file_t *t=tbx_host_file("APPS\\TABLES.TXT");
    if(!t||t->size>=sizeof text) return 0;
    memcpy(text,t->data,t->size); text[t->size]=0;
    char *p=strstr(text,"\r\n1\t(G) "), *end;
    if(!p) return 0;
    *x=strtod(p+8,&end);
    if(end==p+8) return 0;
    if(*end=='-') { *x=-*x; end++; }
    return *end=='\r';
}
int main(void)
{
    FILE *f=fopen("data/LEDGER.TXT","rb"); if(!f) return 2;
    shipped_len=fread(shipped_bytes,1,sizeof shipped_bytes-1,f); fclose(f); shipped_bytes[shipped_len]=0;
    char want[8192]; size_t n;
    tbx_host_reset();
    tbx_host_put_file("APPS\\LEDGER.TXT",shipped_bytes,(uint32_t)shipped_len);
    CHECK(tbx_app_main()==0,"Calc launches");
    CHECK(tbx_host.nwindows==2&&tbx_host.alerts==0,"ledger and chart open from the document, no complaint");
    const tbx_menuitem_t *save=&tbx_host.bar->menus[0].items[0];
    CHECK(!strcmp(save->text,"Save")&&save->enabled&&save->cmd_char=='S',"SAVE_ENABLED File > Save, Cmd/Ctrl-S");

    event(TBX_EVT_MENU,SAVE);
    CHECK(copies==1&&tbx_host.alerts==0,"SAVE_MODAL Save asks the FILE COPY box exactly once");
    CHECK(!strcmp(tbx_host.copy_caption,"Saving tables to disk..."),"SAVE_CAPTION Saving tables to disk...");
    CHECK(!strcmp(tbx_host.copy_src,"APPS\\TABLES.TMP")&&!strcmp(tbx_host.copy_dst,"APPS\\TABLES.TXT"),"SAVE_PATHS the model is staged, then copied to APPS\\TABLES.TXT");
    CHECK(staged_is(shipped_bytes,shipped_len),"ROUND_TRIP an unedited ledger at its saved positions saves byte for byte as shipped");
    event(TBX_EVT_FILECOPY,(int32_t)shipped_len);
    CHECK(!tbx_host_file("APPS\\TABLES.TMP"),"STAGE_CLEANED the stage is deleted after the copy");
    CHECK(tbx_host_file("APPS\\TABLES.TXT")&&tbx_host_file("APPS\\TABLES.TXT")->size==shipped_len,"the copy landed");

    /* The user drags the ledger 10 right and 5 down, then saves again. */
    drag_h[1]=10; drag_v[1]=5;
    event(TBX_EVT_MENU,SAVE);
    CHECK(tbx_host.alerts==1&&copies==1,"REPLACE_ASKS Save onto an existing TABLES.TXT asks first");
    CHECK(strstr(tbx_host.alert_text,"TABLES.TXT")&&strstr(tbx_host.alert_text,"replace"),"REPLACE_NAMES the alert names TABLES.TXT and says replace");
    event(TBX_EVT_ALERT,TBX_ALERT_CANCEL);
    CHECK(copies==1,"REPLACE_CANCEL No leaves the existing file alone");
    event(TBX_EVT_MENU,SAVE); event(TBX_EVT_ALERT,TBX_ALERT_DEFAULT);
    n=edited(want,"WINDOW\t116\t104\t622\t474\r\n","WINDOW\t126\t109\t632\t479\r\n");
    CHECK(copies==2&&staged_is(want,n),"POSITIONS the saved document carries where the ledger window is now");
    event(TBX_EVT_FILECOPY,(int32_t)n);

    /* A collapsed window keeps the bounds it had. */
    collapsed[2]=1; event(TBX_EVT_MENU,SAVE); event(TBX_EVT_ALERT,TBX_ALERT_DEFAULT);
    CHECK(copies==3&&staged_is(want,n),"COLLAPSED a collapsed chart keeps its saved bounds");
    event(TBX_EVT_FILECOPY,(int32_t)n); collapsed[2]=0;

    /* An edit: 73 into the first cell. */
    key('7'); key('3'); key('\r');
    CHECK(g_dirty,"an edit is unsaved work");
    event(TBX_EVT_MENU,SAVE); event(TBX_EVT_ALERT,TBX_ALERT_DEFAULT);
    char want2[8192];
    {   size_t m=edited(want2,"ROWS\t26\r\n","ROWS\t26\r\n1\t(G) 73\r\n");
        char tmp[8192]; memcpy(tmp,want2,m+1);
        /* and the dragged ledger */
        char *p=strstr(tmp,"WINDOW\t116\t104\t622\t474"); if(p) memcpy(p,"WINDOW\t126\t109\t632\t479",22);
        CHECK(copies==4&&staged_is(tmp,m),"SAVE_EDIT the saved document holds the edit in row 1 as a general number");
        event(TBX_EVT_FILECOPY,(int32_t)m);
        CHECK(!g_dirty,"a landed save clears the unsaved mark");
    }

    /* Reopen: the saved document comes back, positions and all. */
    {   const tbx_host_file_t *t=tbx_host_file("APPS\\TABLES.TXT");
        static char saved[8192]; size_t sl=t?t->size:0; if(t) memcpy(saved,t->data,sl);
        tbx_host_reset(); memset(drag_h,0,sizeof drag_h); memset(drag_v,0,sizeof drag_v);
        tbx_host_put_file("APPS\\LEDGER.TXT",shipped_bytes,(uint32_t)shipped_len);
        tbx_host_put_file("APPS\\TABLES.TXT",saved,(uint32_t)sl);
        CHECK(tbx_app_main()==0&&tbx_host.alerts==0,"REOPEN Calc launches on the saved tables");
        CHECK(tbx_host.windows[0].l==126&&tbx_host.windows[0].t==109&&tbx_host.windows[0].r==632&&tbx_host.windows[0].b==479,"REOPEN the ledger opens where it was saved");
        CHECK(tbx_host.windows[1].l==349&&tbx_host.windows[1].t==72,"REOPEN the chart opens where it was saved");
        char s[96]; CHECK(cell_text(0,0,s,98)&&!strcmp(s,"73"),"REOPEN the edit is there");
        CHECK(cell_text(8,15,s,42)&&!strcmp(s,"570-"),"REOPEN the trailing-minus cell is there");
        copies=0;
        event(TBX_EVT_MENU,SAVE); event(TBX_EVT_ALERT,TBX_ALERT_DEFAULT);
        CHECK(copies==1&&staged_len==sl&&!memcmp(staged,saved,sl),"REOPEN saving again reproduces the saved bytes");
        event(TBX_EVT_FILECOPY,(int32_t)sl);
    }
    /* A stranger's APPS\TABLES.TMP survives every save. */
    tbx_host_put_file("APPS\\TABLES.TMP","stranger",8);
    event(TBX_EVT_MENU,SAVE); event(TBX_EVT_ALERT,TBX_ALERT_DEFAULT);
    CHECK(!strcmp(tbx_host.copy_src,"APPS\\TABLE~1.TMP"),"STAGE_CLOBBER an occupied APPS\\TABLES.TMP is not used as the stage");
    event(TBX_EVT_FILECOPY,10);
    {   const tbx_host_file_t *s=tbx_host_file("APPS\\TABLES.TMP");
        CHECK(s&&s->size==8&&!memcmp(s->data,"stranger",8),"STAGE_CLOBBER completion deletes only the stage this Save created"); }
    /* Quit > Save on an edited sheet: asks, replaces, quits when the copy lands. */
    key('9'); key('\r');
    int a0=tbx_host.alerts; unsigned c0=copies;
    event(TBX_EVT_MENU,QUIT);
    CHECK(tbx_host.alerts==a0+1&&tbx_host.exit_calls==0,"QUITSAVE Quit on an edited sheet asks first");
    event(TBX_EVT_ALERT,TBX_ALERT_DEFAULT);
    CHECK(tbx_host.alerts==a0+2&&tbx_host.exit_calls==0,"QUITSAVE Save over TABLES.TXT asks the replace question");
    event(TBX_EVT_ALERT,TBX_ALERT_DEFAULT);
    CHECK(copies==c0+1&&tbx_host.exit_calls==0&&strstr(staged,"\r\n1\t(G) 9\r\n"),"QUITSAVE the copy holds the second edit; the session waits for it");
    event(TBX_EVT_FILECOPY,50);
    CHECK(tbx_host.exit_calls==1,"QUITSAVE a successful Save ends the session");

    /* initech-3yye3: numbers Initech 123 writes in exponent form. Typed
     * 0.0000001 and 1000000000000000000000 (and their negatives) must save,
     * and the saved tables must reopen with the same value, never fall back
     * to the shipped ledger. Expected values are the typed literals. */
    {   /* literal 0: the typed text has 17 digits, and which double Initech
         * 123's entry stores for it is the engine's business; the save must
         * keep that double (initech-qdscg). */
        static const struct { const char *typed; double value; int literal; } big[]={
            {"0.0000001",1e-7,1},{"1000000000000000000000",1e21,1},{"0.0000001-",-1e-7,1},{"-1000000000000000000000",-1e21,1},{"1.5E-12",1.5e-12,1},
            /* initech-qdscg: more digits than the 15 the control panel shows */
            {"1234567890123456",1234567890123456.0,1},{"1234567890123456-",-1234567890123456.0,1},{"0.12345678901234567",0,0}};
        for(unsigned i=0;i<sizeof big/sizeof big[0];i++) {
            int saved=0; double x=0; char msg[96];
            int rc=save_and_reopen(shipped_bytes,0,big[i].typed,&saved);
            snprintf(msg,sizeof msg,"EXPONENT %s is stored as typed",big[i].typed);
            CHECK(stored_ok&&(!big[i].literal||stored==big[i].value),msg);
            snprintf(msg,sizeof msg,"EXPONENT %s saves and reopens as itself",big[i].typed);
            CHECK(rc==0&&saved&&reopen_alerts==0&&first_cell(&x)&&x==stored,msg);
            snprintf(msg,sizeof msg,"EXACT %s is saved on the disk as itself",big[i].typed);
            CHECK(saved_first_cell(&x)&&x==stored,msg);
            snprintf(msg,sizeof msg,"EXPONENT %s reopens with the rest of the ledger",big[i].typed);
            {   char s[96]; CHECK(cell_text(8,15,s,42)&&!strcmp(s,"570-")&&nseries==5,msg); }
        }
        /* A previously saved ledger is replaced only by one that reads back. */
        {   int saved=0; double x=0;
            static char tables[4096]; snprintf(tables,sizeof tables,"%s",shipped_bytes);
            CHECK(save_and_reopen(shipped_bytes,tables,"0.0000001",&saved)==0&&saved&&reopen_alerts==0&&first_cell(&x)&&x==1e-7,
                  "EXPONENT replacing saved tables keeps a document Calc can open");
        }
    }
    /* initech-gs1ww: with neither document readable Calc opens empty; what
     * is typed there must save into a document that opens again. Expected
     * bytes are written by hand from README.md "The ledger document": the
     * home bounds, no chart values, the full 26 rows, the one typed cell. */
    {   int saved=0; double x=0;
        static const char want_empty[]="INITECH LEDGER\r\nWINDOW\t116\t104\t622\t474\r\nCHART\t349\t72\t605\t288\r\nSERIES\r\nROWS\t26\r\n1\t(G) 73\r\n";
        int rc=save_and_reopen(0,0,"73",&saved);
        const tbx_host_file_t *t=tbx_host_file("APPS\\TABLES.TXT");
        CHECK(rc==0&&saved&&t&&t->size==sizeof want_empty-1&&!memcmp(t->data,want_empty,t->size),"EMPTY the empty ledger's entry is saved in an ordinary document");
        CHECK(reopen_alerts==0&&first_cell(&x)&&x==73,"EMPTY the saved document reopens with the entry");
    }

    /* initech-40kpb, initech-zev40: a stage whose DELETE DOS refuses is no
     * longer provably Calc's once its Save is over. Calc names it to the
     * user, leaves it exactly as it was, and the next Save takes another
     * name. (A stranger's TABLES.TMP is never touched: STAGE_CLOBBER above.) */
    {   tbx_host_reset(); copies=0; memset(drag_h,0,sizeof drag_h); memset(drag_v,0,sizeof drag_v);
        tbx_host_put_file("APPS\\LEDGER.TXT",shipped_bytes,(uint32_t)shipped_len);
        CHECK(tbx_app_main()==0,"LEFTOVER Calc launches");
        key('7'); key('3'); key('\r');
        event(TBX_EVT_MENU,SAVE);
        CHECK(copies==1&&!strcmp(tbx_host.copy_src,"APPS\\TABLES.TMP"),"LEFTOVER the first Save stages APPS\\TABLES.TMP");
        static char first[8192]; size_t fl=staged_len; memcpy(first,staged,fl);
        tbx_host.file_error[TBX_FILE_DELETE]=5;
        event(TBX_EVT_FILECOPY,TBX_ERR_BUSY);              /* cancelled; the stage cannot be deleted */
        tbx_host.file_error[TBX_FILE_DELETE]=0;
        CHECK(g_dirty&&!strcmp(tbx_host.alert_text,"The worksheet could not be saved.\rTemporary file remains:\rAPPS\\TABLES.TMP"),
              "LEFTOVER the failure notice names the file left behind");
        event(TBX_EVT_ALERT,TBX_ALERT_DEFAULT);            /* dismiss the failure notice */
        event(TBX_EVT_MENU,SAVE);
        CHECK(copies==2&&!strcmp(tbx_host.copy_src,"APPS\\TABLE~1.TMP"),"LEFTOVER the next Save takes another name");
        event(TBX_EVT_FILECOPY,(int32_t)staged_len);
        {   const tbx_host_file_t *l=tbx_host_file("APPS\\TABLES.TMP");
            CHECK(l&&l->size==fl&&!memcmp(l->data,first,fl),"LEFTOVER the file left behind is never deleted later"); }
        CHECK(!tbx_host_file("APPS\\TABLE~1.TMP")&&!g_dirty,"LEFTOVER the next Save lands and removes its own stage");
    }
    printf("test_calc_file_copy: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
