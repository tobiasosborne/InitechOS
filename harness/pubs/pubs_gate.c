/* Publications correctness oracle. Ref: PUB-REF-0001 GC-01/03/05/06,
 * PN-01/05, VO-30; PRD Sec 8; CLAUDE.md Law 2, Rules 2/6. Host C only. */
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <regex.h>
#define MAX_ITEMS 1024
#define MAX_NAME 128
typedef struct { char item[MAX_ITEMS][MAX_NAME]; size_t n; } Set;
static void die(const char *tag,const char *path,const char *detail)
{ fflush(stdout);fprintf(stderr,"!!! test-pubs RED %s: %s: %s\n",tag,path,detail);exit(1); }
static char *readfile(const char *p)
{
    FILE *f=fopen(p,"rb"); long n; char *s;
    if(!f) die("INPUT",p,"cannot open");
    if(fseek(f,0,SEEK_END) || (n=ftell(f))<0 || n>16*1024*1024) die("INPUT",p,"size");
    rewind(f);s=calloc((size_t)n+1,1);
    if(!s || fread(s,1,(size_t)n,f)!=(size_t)n) die("INPUT",p,"read");
    fclose(f);return s;
}
static int has(const Set *s,const char *name)
{ for(size_t i=0;i<s->n;i++) if(!strcmp(s->item[i],name)) return 1;return 0; }
static void add(Set *s,const char *name)
{
    if(has(s,name)) return;
    if(!*name || strlen(name)>=MAX_NAME || s->n==MAX_ITEMS) die("SET",name,"invalid/too many names");
    strcpy(s->item[s->n++],name);
}
static Set list(const char *path,int allow)
{
    Set s={0};char *text=readfile(path),*line=text;
    while(*line) {
        char *end=strchr(line,'\n');if(end) *end=0;
        if(*line && *line!='#') {
            if(allow==1) {
                char *bar=strchr(line,'|');
                if(!bar || strlen(bar+1)<12 || bar[5]!='-' || bar[8]!='-' || bar[11]!='|' || !bar[12])
                    die("ALLOWLIST-FORMAT",path,line);
                for(int i=1;i<=10;i++) if(i!=5 && i!=8 && !isdigit((unsigned char)bar[i]))
                    die("ALLOWLIST-FORMAT",path,line);
                *bar=0;
            }
            if(has(&s,line) && allow!=-1) die("DUPLICATE",path,line);
            add(&s,line);
        }
        if(!end) break;
        line=end+1;
    }
    free(text);return s;
}
static int cmpname(const void *a,const void *b) { return strcmp(a,b); }
/* The shipped inventory is one line per NAME|KIND, KIND one of internal
 * (command.c dispatch table), int21 (the controlled INT 21h register),
 * external (APPS executables), batch and config (dos_surface_inventory: the
 * real batch_classify / config_sys_parse).  BREAK is both internal and
 * config, ECHO both internal and batch: a pair is one documentable item. */
static void addkind(Set *s,const char *name,const char *kind)
{
    char buf[MAX_NAME];
    if(snprintf(buf,sizeof(buf),"%s|%s",name,kind)>=(int)sizeof(buf)) die("INVENTORY",name,"long name");
    add(s,buf);
}
static void inventory(int argc,char **argv)
{
    if(argc<6) die("USAGE","inventory","command.i int21.list surface.list APPS-paths...");
    Set s={0}; size_t internal=0; char *src=readfile(argv[2]);
    /* GCC can keep whitespace differently; the table name is unique here. */
    char *p=strstr(src,"table[] = {");
    if(!p) die("INVENTORY",argv[2],"command dispatch table missing");
    char *stop=strstr(p,"};");if(!stop) die("INVENTORY",argv[2],"unterminated dispatch table");
    while((p=strchr(p,'"')) && p<stop) {
        char *end=strchr(++p,'"');if(!end || end>=stop) die("INVENTORY",argv[2],"bad table name");
        char save=*end;*end=0;addkind(&s,p,"internal");internal++;*end=save;p=end+1;
    }
    if(internal<10) die("INVENTORY",argv[2],"empty/truncated internal set");
    free(src);
    Set ints=list(argv[3],0);if(!ints.n) die("INVENTORY",argv[3],"empty INT21 register");
    for(size_t i=0;i<ints.n;i++) addkind(&s,ints.item[i],"int21");
    Set surf=list(argv[4],0);size_t batch=0,config=0;
    for(size_t i=0;i<surf.n;i++) {
        const char *bar=strchr(surf.item[i],'|');
        if(!bar) die("INVENTORY",argv[4],surf.item[i]);
        if(!strcmp(bar+1,"batch")) batch++; else if(!strcmp(bar+1,"config")) config++;
        else die("INVENTORY",argv[4],surf.item[i]);
        add(&s,surf.item[i]);
    }
    if(!batch || !config) die("INVENTORY",argv[4],"no batch subcommands or CONFIG.SYS directives");
    size_t external=0;
    for(int i=5;i<argc;i++) {
        const char *name=strrchr(argv[i],'/');name=name?name+1:argv[i];
        char buf[MAX_NAME];if(strlen(name)>=sizeof(buf)) die("INVENTORY",name,"long path");
        strcpy(buf,name);char *dot=strrchr(buf,'.');
        if(dot && (!strcmp(dot,".COM") || !strcmp(dot,".EXE") || !strcmp(dot,".BAT"))) {
            *dot=0;addkind(&s,buf,"external");external++;
        }
    }
    if(!external) die("INVENTORY","APPS","no shipped executables");
    qsort(s.item,s.n,sizeof(s.item[0]),cmpname);
    for(size_t i=0;i<s.n;i++) puts(s.item[i]);
}
/* NAME of a NAME|KIND[|...] line, into buf. */
static const char *namepart(const char *item,char *buf)
{
    size_t n=strcspn(item,"|");memcpy(buf,item,n);buf[n]=0;return buf;
}
/* Shared index vocabulary, checked before the source gate's DRAFT exemption. */
static int commandkind(const char *kind)
{
    static const char *kinds[]={"internal","external","batch","config","int21",
        "wpkey","wpmenu","wppull","wpcode","wpmsg","wpgray","command","function","set"};
    for(size_t i=0;i<sizeof kinds/sizeof *kinds;i++) if(!strcmp(kind,kinds[i])) return 1;
    return 0;
}
/* A book's documented-command list, <job>.cmds (initech-syntax.sty
 * \RegisterCommand): NAME|KIND|STATE|ENTRY. Several books may be given;
 * a pair documented in two books is one item. DRAFT pairs are returned
 * in *draft (the build lacks the behaviour, D-14). */
static void cmdsfile(const char *path,Set *doc,Set *draft)
{
    char *text=readfile(path),*line=text;
    while(*line) {
        char *end=strchr(line,'\n');if(end) *end=0;
        if(*line && *line!='#') {
            char f[4][MAX_NAME];int k=0;const char *q=line;
            for(;k<4;k++) {
                size_t n=strcspn(q,"|");
                if(n>=MAX_NAME) die("COMMAND-INDEX",path,line);
                memcpy(f[k],q,n);f[k][n]=0;q+=n;
                if(*q=='|') q++; else { k++;break; }
            }
            if(k!=4 || *q || !*f[0] || !*f[3]) die("COMMAND-INDEX",path,line);
            /* InitechWord (harness/pubs/iword_probe.c, bead initech-ipu5.10):
             * wpkey wpmenu wppull wpcode wpmsg wpgray */
            if(!commandkind(f[1]))
                die("COMMAND-INDEX",path,line);
            if(strcmp(f[2],"GATED") && strcmp(f[2],"DRAFT")) die("COMMAND-INDEX",path,line);
            addkind(doc,f[0],f[1]);
            if(!strcmp(f[2],"DRAFT")) addkind(draft,f[0],f[1]);
        }
        if(!end) break;
        line=end+1;
    }
    free(text);
}
/* The both-directions rule shared by `commands` and `srclockstep`. */
static void crosscheck_sets(const char *docs0,const char *allowpath,const Set *ship_,const Set *doc_,const Set *draft_,const Set *allow_)
{
    static Set ship,doc,draft,allow;
    char a[MAX_NAME],b[MAX_NAME];
    const char *docs[1];
    ship=*ship_;doc=*doc_;draft=*draft_;allow=*allow_;docs[0]=docs0;
    if(!ship.n || !doc.n) die("COMMAND-INDEX",docs[0],"empty inventory/index");
    for(size_t i=0;i<doc.n;i++)
        if(!has(&ship,doc.item[i]) && !has(&draft,doc.item[i])) die("DOC-NOT-SHIPPED",docs[0],doc.item[i]);
    for(size_t i=0;i<allow.n;i++) {
        size_t shipped_pairs=0,open=0;
        for(size_t j=0;j<ship.n;j++) if(!strcmp(namepart(ship.item[j],a),allow.item[i])) {
            shipped_pairs++; if(!has(&doc,ship.item[j])) open++;
        }
        if(!shipped_pairs || !open) die("STALE-ALLOWLIST",allowpath,allow.item[i]);
        printf("!!! test-pubs ALLOWED-OMISSION: %s (%s)\n",allow.item[i],allowpath);
    }
    for(size_t i=0;i<ship.n;i++) if(!has(&doc,ship.item[i]) && !has(&allow,namepart(ship.item[i],b)))
        die("SHIPPED-NOT-DOCUMENTED",docs[0],ship.item[i]);
    for(size_t i=0;i<draft.n;i++) printf("!!! test-pubs DRAFT-ENTRY: %s (documented as DRAFT; GC-12 blocks release)\n",draft.item[i]);
}
static void crosscheck(const char *shipped,int ndoc,char **docs,const char *allowpath)
{
    static Set ship,doc,draft,allow;
    ship=list(shipped,0);allow=list(allowpath,1);doc.n=draft.n=0;
    for(int i=0;i<ndoc;i++) cmdsfile(docs[i],&doc,&draft);
    crosscheck_sets(docs[0],allowpath,&ship,&doc,&draft,&allow);
    printf(">>> test-pubs-commands: green (%zu shipped, %zu documented, %zu DRAFT, %zu explicit omissions)\n",ship.n,doc.n,draft.n,allow.n);
}
#include "srclockstep.h"
static void monotonic(const char *now,const char *before)
{
    Set a=list(now,1),b=list(before,1);
    for(size_t i=0;i<a.n;i++) if(!has(&b,a.item[i])) die("ALLOWLIST-GROWTH",now,a.item[i]);
    printf(">>> test-pubs-allowlist: green (%zu -> %zu; additions forbidden)\n",b.n,a.n);
}
static void same(const char *tag,const char *a,const char *b)
{
    FILE *fa=fopen(a,"rb"),*fb=fopen(b,"rb");size_t at=0;int ca,cb;
    if(!fa || !fb) die(tag,a,"missing expected/replay file");
    do { ca=fgetc(fa);cb=fgetc(fb);if(ca!=cb) {
        char detail[256];snprintf(detail,sizeof(detail),"byte %zu expected=%d actual=%d (%s)",at,ca,cb,b);
        die(tag,a,detail);
    }at++; } while(ca!=EOF);
    fclose(fa);fclose(fb);
}
/* InitechBase inline mentions (bead initech-ipu5.8): \cmd{LIST FOR ...}
 * names the command LIST, \cmd{SET EXACT ON} the option EXACT|set, and
 * \fn{SUBSTR()} or \fn{SUBSTR(NAME, 1, 3)} the function SUBSTR|function (a
 * name followed at once by a parenthesis) -- each must be shipped
 * (samir_inventory.c NAME|KIND lines). */
static void samirmentions(int argc,char **argv)
{
    Set ship=list(argv[2],0);size_t n=0;char w1[MAX_NAME],w2[MAX_NAME],key[2*MAX_NAME];
    for(int f=3;f<argc;f++) {
        Set m=list(argv[f],-1);
        for(size_t i=0;i<m.n;i++) {
            const char *t=m.item[i];size_t id=0;
            while(isupper((unsigned char)t[id]) || isdigit((unsigned char)t[id])) id++;
            if(id && t[id]=='(') {
                /* \fn{UPPER(LASTNAME)}: a function call names the function */
                snprintf(key,sizeof(key),"%.*s|function",(int)id,t);
            } else {
                w1[0]=w2[0]=0;sscanf(t,"%127s %127s",w1,w2);
                if(!strcmp(w1,"SET")) snprintf(key,sizeof(key),"%s|set",w2);
                else snprintf(key,sizeof(key),"%s|command",w1);
            }
            if(!has(&ship,key)) die("DOC-NOT-SHIPPED",argv[f],key);
            n++;
        }
    }
    printf(">>> test-pubs-mentions: green (%zu InitechBase inline commands, options and functions shipped)\n",n);
}
/* GC-02 for InitechBase (bead initech-ipu5.8).  catalogue: n|text lines of
 * the shipped g_msg (samir_inventory messages).  Each <job>.msgs line is
 * n|text|STATE (STATE GATED or DRAFT: an AN-06 entry; MENTION: an inline
 * \xmsg).  Every printed text equals its catalogue text byte for byte; the
 * FIRST file (the book with the Error Messages chapter) has exactly one
 * entry per catalogue message; the others may only mention. */
static void messages(int argc,char **argv)
{
    Set cat=list(argv[2],0),entries={0};size_t drafts=0,mentions=0;
    for(int f=3;f<argc;f++) {
        char *text=readfile(argv[f]),*line=text;
        while(*line) {
            char *end=strchr(line,'\n');if(end) *end=0;
            if(*line) {
                char *bar1=strchr(line,'|'),*bar2=strrchr(line,'|');
                if(!bar1 || bar2==bar1) die("MESSAGE-INDEX",argv[f],line);
                *bar2=0;const char *state=bar2+1;
                if(!has(&cat,line)) die("MESSAGE-TEXT",argv[f],line);
                if(!strcmp(state,"MENTION")) mentions++;
                else if(!strcmp(state,"GATED") || !strcmp(state,"DRAFT")) {
                    if(f!=3) die("MESSAGE-INDEX",argv[f],"message entries belong to the first book only");
                    *bar1=0;
                    if(has(&entries,line)) die("DUPLICATE",argv[f],line);
                    add(&entries,line);
                    if(!strcmp(state,"DRAFT")) drafts++;
                } else die("MESSAGE-INDEX",argv[f],state);
            }
            if(!end) break;
            line=end+1;
        }
        free(text);
    }
    char num[MAX_NAME];
    for(size_t i=0;i<cat.n;i++) if(!has(&entries,namepart(cat.item[i],num)))
        die("MESSAGE-NOT-DOCUMENTED",argv[3],cat.item[i]);
    if(entries.n!=cat.n) die("MESSAGE-INDEX",argv[3],"entries for numbers the catalogue does not have");
    printf(">>> test-pubs-messages: green (%zu catalogue messages, %zu entries, %zu DRAFT, %zu inline quotations verbatim)\n",
           cat.n,entries.n,drafts,mentions);
}
static int match(const char *expr,const char *s)
{
    regex_t r; if(regcomp(&r,expr,REG_EXTENDED|REG_NOSUB)) die("REGEX",expr,"invalid");
    int yes=regexec(&r,s,0,NULL,0)==0;regfree(&r);return yes;
}
static void metadata(const char *src,const char *path,const char *macro,const char *expr)
{
    const char *p=src;char value[256];
    while((p=strstr(p,macro))) {
        p+=strlen(macro);const char *end=strchr(p,'}');
        if(!end || (size_t)(end-p)>=sizeof(value)) die("METADATA",path,macro);
        memcpy(value,p,(size_t)(end-p));value[end-p]=0;
        if(!match(expr,value)) die("METADATA",path,value);
        p=end+1;
    }
}
static Set denied;
static void lintfile(const char *path)
{
    char *src=readfile(path);const char *dot=strrchr(path,'.');
    FILE *raw=fopen(path,"rb");int c;
    if(!raw) die("INPUT",path,"cannot open");
    int tabok=dot && (!strcmp(dot,".mk") || !strcmp(dot,".sh"));
    /* A replayed transcript (.txt) is the shipped program's exact bytes: the
     * InitechBase dot prompt ends lines CR LF like real dBASE III PLUS (the
     * samirdml parity fix), so CR is a legitimate byte there. PM 2026-10-17. */
    int crok=dot && !strcmp(dot,".txt");
    while((c=fgetc(raw))!=EOF) if(c>126 || (c<32 && c!='\n' && !(c=='\t' && tabok) && !(c=='\r' && crok)))
        die("ASCII",path,"non-printable/non-ASCII byte");
    fclose(raw);
    if(!dot || (strcmp(dot,".tex") && strcmp(dot,".txt") && strcmp(dot,".prg"))) { free(src);return; }
    if(!strcmp(path,"pubs/common/colophon.tex") || !strcmp(path,"pubs/denylist.txt")) { free(src);return; }
    /* TeX comments are not printed prose (VO-18). Strip unescaped percent
     * through newline, preserving line boundaries. The factory files are
     * ASCII-linted above; vocabulary applies to publication material. */
    if(!strcmp(dot,".tex")) {
        int comment=0;
        for(char *p=src;*p;p++) {
            if(*p=='\n') comment=0;
            if(comment) *p=' ';
            else if(*p=='%' && (p==src || p[-1]!='\\')) { comment=1;*p=' '; }
        }
    }
    char *lower=malloc(strlen(src)+1);if(!lower) die("MEMORY",path,"");
    size_t srclen=strlen(src);
    for(size_t i=0;i<=srclen;i++) lower[i]=(char)tolower((unsigned char)src[i]);
    for(size_t i=0;i<denied.n;i++) {
        char *p=lower;size_t len=strlen(denied.item[i]);
        while((p=strstr(p,denied.item[i]))) {
            int literal=strchr(denied.item[i],'.')!=NULL || strchr(denied.item[i],':')!=NULL;
            if(literal || ((p==lower || !isalnum((unsigned char)p[-1])) && !isalnum((unsigned char)p[len])))
                die("BANNED-WORD",path,denied.item[i]);
            p++;
        }
    }
    for(char *p=src;*p;p++) if(isdigit((unsigned char)*p) &&
        (p==src || !isalnum((unsigned char)p[-1]))) {
        /* Skip the complete PN-01 token, not arbitrary hyphenated years. */
        if(match("^[0-9]{4}-[0-9]{4}-[0-9]{2}([^0-9]|$)",p)) { p+=11;continue; }
        /* The decimal places of a number (18.7500, 0.4998 in a replayed
         * InitechBase posting) are not a year: digits right after a point
         * that follows a digit are skipped (bead initech-ipu5.8).  A year is
         * never so written, so no year escapes: "1994." and ".1994" alone
         * are still read. */
        if(p-src>=2 && p[-1]=='.' && isdigit((unsigned char)p[-2])) {
            while(isdigit((unsigned char)p[1])) p++;
            continue;
        }
        char *end;long value=strtol(p,&end,10);
        int extension=0;
        if(value==2504) {
            size_t offset=(size_t)(p-src),start=offset>80?offset-80:0;
            char context[81];memcpy(context,lower+start,offset-start);context[offset-start]=0;
            extension=match("extension[[:space:]]*$",context);
        }
        if(end-p==4 && value>1993 && !extension && !isalnum((unsigned char)*end))
            die("PERIOD-YEAR",path,"year after 1993");
        p=end-1;
    }
    metadata(src,path,"\\PartNumber{","^(1000|1024|1031|1040|1051|1062|1077)-[0-9]{4}-[0-9]{2}$");
    metadata(src,path,"\\PrintCode{","^[A-Z][1-9]-(1[0-2]|[1-9])/(91|92|93)$");
    if(strstr(src,"\\PartNumber{") && !strstr(src,"\\PrintCode{")) die("METADATA",path,"missing PrintCode");
    free(lower);free(src);
}
static void walk(const char *path)
{
    struct stat st;if(stat(path,&st)) die("INPUT",path,"stat");
    if(S_ISDIR(st.st_mode)) {
        DIR *d=opendir(path);struct dirent *e;if(!d) die("INPUT",path,"opendir");
        while((e=readdir(d))) if(strcmp(e->d_name,".") && strcmp(e->d_name,"..")) {
            char p[4096];if(snprintf(p,sizeof(p),"%s/%s",path,e->d_name)>=(int)sizeof(p)) die("PATH",path,"long");
            walk(p);
        }
        closedir(d);
    } else if(S_ISREG(st.st_mode)) lintfile(path);
}
/* ---- the house register of tolerated inconsistencies (PUB-REF-0001 VO-30;
 * bead initech-ipu5.16; WL-0150) -------------------------------------------
 * spec/pubs_inconsistencies.json is the LOCKED register: every slip a book
 * carries is \Slip{ID}{text} in its sources (initech-syntax.sty), and the
 * class writes every printing of one to <job>.slips as ID|PLACE|PAGE|text,
 * PLACE set by where it printed (cover, spine, title-page, running-head,
 * heading, contents, cross-reference, body, index).  For each book:
 *   - the slips in the sources (main, cover, spine and chapters) number
 *     between the floor and the ceiling of the budget for the printed pages
 *     of main.pdf ("just enough to pass QA"; zero slips is RED);
 *   - each entry is used min_per_book..max_per_book times, only in the
 *     families it names, with text its pattern accepts;
 *   - every printing is in a place the entry allows, every source slip is
 *     printed, and nothing printed lacks its source;
 *   - an index slip differs only in case from one of the book's heads, and a
 *     stale cross-reference names no head of any book in the run;
 *   - a variant with a scan pattern never appears outside a \Slip (no slip
 *     escapes the budget);
 *   - the slogan keeps "Is This Good for the COMPANY?" exactly, in the books
 *     and in the class, and no \Slip touches it.
 * Every entry must be used by at least one book of the run.  Registration
 * of commands is by NAME|KIND (.cmds) and is untouched by any slip. */
#define FMT(b,...) do { if(snprintf(b,sizeof(b),__VA_ARGS__)>=(int)sizeof(b)) die("FORMAT",#b,"truncated"); } while(0)
typedef struct JV JV;
struct JV { int t; char *s; double num; size_t len; char **k; JV **v; };
static const char *jp,*jpath;
static void jfail(const char *why) { die("REGISTER-JSON",jpath,why); }
static void *xalloc(void *p,size_t n) { p=realloc(p,n?n:1); if(!p) die("MEMORY","json",""); return p; }
static void jws(void) { while(*jp==' ' || *jp=='\n' || *jp=='\t' || *jp=='\r') jp++; }
static JV *jnew(int t) { JV *v=xalloc(NULL,sizeof(*v)); memset(v,0,sizeof(*v)); v->t=t; return v; }
static char *jstr(void)
{
    size_t cap=64,n=0; char *b=xalloc(NULL,cap);
    jp++;
    while(*jp!='"') {
        char c=*jp++;
        if(!c) jfail("unterminated string");
        if((unsigned char)c<0x20 || (unsigned char)c>0x7e) jfail("control or non-ASCII byte in a string");
        if(c=='\\') {
            c=*jp++;
            if(c=='n') c='\n'; else if(c=='t') c='\t';
            else if(c!='"' && c!='\\' && c!='/') jfail("unsupported escape");
        }
        if(n+2>cap) b=xalloc(b,cap*=2);
        b[n++]=c;
    }
    jp++; b[n]=0; return b;
}
static JV *jval(void)
{
    JV *v; jws();
    if(*jp=='{' || *jp=='[') {
        int obj=*jp=='{'; char close=obj?'}':']';
        v=jnew(obj?'o':'a'); jp++; jws();
        if(*jp==close) { jp++; return v; }
        for(;;) {
            char *key=NULL;
            jws();
            if(obj) {
                if(*jp!='"') jfail("object key");
                key=jstr(); jws();
                if(*jp++!=':') jfail("missing colon");
                for(size_t i=0;i<v->len;i++) if(!strcmp(v->k[i],key)) jfail("duplicate key");
            }
            v->k=xalloc(v->k,(v->len+1)*sizeof(*v->k)); v->v=xalloc(v->v,(v->len+1)*sizeof(*v->v));
            v->k[v->len]=key; v->v[v->len]=jval(); v->len++;
            jws();
            if(*jp==',') { jp++; continue; }
            if(*jp==close) { jp++; return v; }
            jfail(obj?"object":"array");
        }
    }
    if(*jp=='"') { v=jnew('s'); v->s=jstr(); return v; }
    if(*jp=='-' || isdigit((unsigned char)*jp)) {
        char *end; v=jnew('n'); v->num=strtod(jp,&end); if(end==jp) jfail("number"); jp=end; return v;
    }
    if(!strncmp(jp,"true",4)) { jp+=4; v=jnew('t'); return v; }
    if(!strncmp(jp,"false",5)) { jp+=5; v=jnew('f'); return v; }
    if(!strncmp(jp,"null",4)) { jp+=4; v=jnew('z'); return v; }
    jfail("value"); return NULL;
}
static JV *jget(const JV *o,const char *key)
{ for(size_t i=0;i<o->len;i++) if(!strcmp(o->k[i],key)) return o->v[i]; return NULL; }
static JV *jneed(const JV *o,const char *key,int t,const char *ctx)
{
    JV *v=jget(o,key); char d[160];
    if(!v || v->t!=t) { FMT(d,"%s: field \"%s\" missing or of the wrong type",ctx,key); jfail(d); }
    return v;
}

#define INC_PLACES 9
static const char *inc_place[INC_PLACES]={"cover","spine","title-page","running-head","heading","contents",
                                            "cross-reference","body","index"};
static int placeidx(const char *s)
{ for(int i=0;i<INC_PLACES;i++) if(!strcmp(inc_place[i],s)) return i; return -1; }
#define INC_MAXE 64
#define INC_TEXT 256
typedef struct {
    const char *id,*check; int places,minb,maxb,used;
    regex_t match[2]; int hasmatch[2]; regex_t scan; int hasscan; JV *families;
} IncEntry;
typedef struct { char file[512]; int line,entry,printed; char text[INC_TEXT]; } SrcSlip;
typedef struct { int entry,place,sourced; char page[32],text[INC_TEXT]; } PrnSlip;
typedef struct { char **h; size_t n; } Heads;
static IncEntry ient[INC_MAXE]; static size_t nient;
static JV *head_title,*head_sentence;
static int entryidx(const char *id)
{ for(size_t i=0;i<nient;i++) if(!strcmp(ient[i].id,id)) return (int)i; return -1; }
static void rcomp(regex_t *r,const char *expr,const char *ctx)
{ if(regcomp(r,expr,REG_EXTENDED|REG_NOSUB)) die("REGISTER-JSON",ctx,expr); }
static int rmatch(const regex_t *r,const char *s) { return regexec(r,s,0,NULL,0)==0; }
static int instrlist(const JV *a,const char *s)
{ for(size_t i=0;a && i<a->len;i++) if(a->v[i]->t=='s' && !strcmp(a->v[i]->s,s)) return 1; return 0; }
static JV *loadregister(const char *path)
{
    char *text=readfile(path); JV *root;
    jpath=path; jp=text; root=jval(); jws(); if(*jp) jfail("trailing bytes");
    if(root->t!='o') jfail("top level is not an object");
    JV *places=jneed(root,"places",'a',"register");
    if(places->len!=INC_PLACES) jfail("places: the vocabulary is the nine places the class reports");
    for(size_t i=0;i<places->len;i++) if(places->v[i]->t!='s' || placeidx(places->v[i]->s)<0) jfail("places: unknown place");
    JV *hc=jneed(root,"head_case",'o',"register");
    head_title=jneed(hc,"title",'a',"head_case"); head_sentence=jneed(hc,"sentence",'a',"head_case");
    JV *es=jneed(root,"entries",'a',"register");
    if(!es->len || es->len>INC_MAXE) jfail("entries: none, or too many");
    for(size_t i=0;i<es->len;i++) {
        JV *e=es->v[i]; IncEntry *n=&ient[nient]; char ctx[64];
        if(e->t!='o') jfail("entry is not an object");
        n->id=jneed(e,"id",'s',"entry")->s; FMT(ctx,"entry %s",n->id);
        if(entryidx(n->id)>=0) jfail("duplicate entry id");
        jneed(e,"name",'s',ctx); jneed(e,"canonical",'s',ctx); jneed(e,"variants",'a',ctx);
        JV *ev=jneed(e,"evidence",'a',ctx);
        if(!ev->len) jfail("an entry without period evidence (the register is grounded, not invented)");
        for(size_t k=0;k<ev->len;k++) {
            if(ev->v[k]->t!='o') jfail("evidence");
            jneed(ev->v[k],"annex",'s',ctx); jneed(ev->v[k],"original",'s',ctx); jneed(ev->v[k],"file",'s',ctx);
            jneed(ev->v[k],"pdf",'n',ctx); jneed(ev->v[k],"page",'s',ctx); jneed(ev->v[k],"quote",'s',ctx);
        }
        JV *m=jget(e,"match");
        if(m && m->t=='s') { rcomp(&n->match[0],m->s,n->id); rcomp(&n->match[1],m->s,n->id); n->hasmatch[0]=n->hasmatch[1]=1; }
        else if(m && m->t=='o') {
            rcomp(&n->match[0],jneed(m,"title",'s',ctx)->s,n->id); rcomp(&n->match[1],jneed(m,"sentence",'s',ctx)->s,n->id);
            n->hasmatch[0]=n->hasmatch[1]=1;
        } else jfail("entry without a match pattern");
        JV *s=jget(e,"scan"); if(s) { if(s->t!='s') jfail("scan"); rcomp(&n->scan,s->s,n->id); n->hasscan=1; }
        JV *c=jget(e,"check");
        if(c) { if(c->t!='s' || (strcmp(c->s,"not-a-heading") && strcmp(c->s,"case-differs-from-heading"))) jfail("check");
                n->check=c->s; }
        JV *pl=jneed(e,"places",'a',ctx);
        if(!pl->len) jfail("an entry allowed nowhere");
        for(size_t k=0;k<pl->len;k++) {
            int p=pl->v[k]->t=='s'?placeidx(pl->v[k]->s):-1;
            if(p<0) jfail("entry place outside the vocabulary");
            n->places|=1<<p;
        }
        n->families=jget(e,"families");
        if(n->families && n->families->t!='a') jfail("families");
        double mn=jneed(e,"min_per_book",'n',ctx)->num,mx=jneed(e,"max_per_book",'n',ctx)->num;
        if(mn<0 || mx<1 || mn>mx || mn!=(int)mn || mx!=(int)mx) jfail("min_per_book/max_per_book");
        n->minb=(int)mn; n->maxb=(int)mx; nient++;
    }
    return root;
}
/* TeX source without comments: an unescaped % through the newline becomes
 * blanks, so line numbers and offsets stay. */
static void stripcomments(char *src)
{
    int comment=0;
    for(char *p=src;*p;p++) {
        if(*p=='\n') { comment=0; continue; }
        if(comment) *p=' ';
        else if(*p=='%' && (p==src || p[-1]!='\\')) { comment=1; *p=' '; }
    }
}
static int cmpstr(const void *a,const void *b) { return strcmp(*(char *const *)a,*(char *const *)b); }
/* The *.tex files of one directory, sorted (deterministic report). */
static size_t texfiles(const char *dir,char ***out)
{
    DIR *d=opendir(dir); struct dirent *e; size_t n=0; char **v=NULL;
    if(!d) die("INPUT",dir,"opendir");
    while((e=readdir(d))) {
        size_t l=strlen(e->d_name);
        if(l>4 && !strcmp(e->d_name+l-4,".tex")) {
            char p[1024]; FMT(p,"%s/%s",dir,e->d_name);
            v=xalloc(v,(n+1)*sizeof(*v)); v[n]=xalloc(NULL,strlen(p)+1); strcpy(v[n++],p);
        }
    }
    closedir(d); qsort(v,n,sizeof(*v),cmpstr); *out=v; return n;
}
/* Copy a balanced {...} group starting at p (on '{') into out; returns the
 * byte after the closing brace. */
static const char *group(const char *p,char *out,size_t cap,const char *ctx)
{
    int depth=0; size_t n=0;
    if(*p!='{') die("SLIP-SYNTAX",ctx,"a group was expected");
    for(;*p;p++) {
        if(*p=='{' && depth++==0) continue;
        if(*p=='}' && --depth==0) { out[n]=0; return p+1; }
        if(n+1>=cap) die("SLIP-SYNTAX",ctx,"group too long");
        out[n++]=*p;
    }
    die("SLIP-SYNTAX",ctx,"unbalanced braces"); return NULL;
}
/* A head as printed: \numberline{..} and the ID of a \Slip dropped, control
 * words dropped, braces dropped, "\ " a space, the DRAFT mark of the
 * contents removed. */
static void headnorm(const char *in,char *out,size_t cap)
{
    size_t n=0; char skip[INC_TEXT];
    while(*in && n+1<cap) {
        if(*in=='\\' && isalpha((unsigned char)in[1])) {
            const char *w=in+1; size_t l=0; while(isalpha((unsigned char)w[l])) l++;
            int numberline=l==10 && !strncmp(w,"numberline",10),slip=l==4 && !strncmp(w,"Slip",4);
            in=w+l; while(*in==' ') in++;
            if((numberline || slip) && *in=='{') in=group(in,skip,sizeof(skip),"contents");
            continue;
        }
        if(*in=='\\' && in[1]) { out[n++]=in[1]==' '?' ':in[1]; in+=2; continue; }
        if(*in=='{' || *in=='}') { in++; continue; }
        if(isspace((unsigned char)*in)) { if(n && out[n-1]!=' ') out[n++]=' '; in++; continue; }
        out[n++]=*in++;
    }
    while(n && out[n-1]==' ') n--;
    out[n]=0;
    if(n>8 && !strcmp(out+n-8,"(DRAFT)")) { out[n-8]=0; n-=8; while(n && out[n-1]==' ') out[--n]=0; }
}
static void loadheads(const char *tocpath,Heads *h)
{
    char *text=readfile(tocpath),*p=text,buf[INC_TEXT*2],norm[INC_TEXT*2];
    while((p=strstr(p,"\\contentsline {"))) {
        p=strchr(p,'}');
        if(!p || p[1]!='{') die("SLIP-CONTENTS",tocpath,"unexpected \\contentsline");
        p=(char *)group(p+1,buf,sizeof(buf),tocpath);
        headnorm(buf,norm,sizeof(norm));
        if(*norm) { h->h=xalloc(h->h,(h->n+1)*sizeof(*h->h)); h->h[h->n]=xalloc(NULL,strlen(norm)+1); strcpy(h->h[h->n++],norm); }
    }
    free(text);
}
static int pagesof(const char *logpath)
{
    char *text=readfile(logpath),*w=text;
    for(char *r=text;*r;r++) if(*r!='\n') *w++=*r;
    *w=0;
    char *p=strstr(text,"Output written on ");
    if(!p) die("SLIP-PAGES",logpath,"no \"Output written on\" line (the book did not typeset)");
    p=strstr(p," pages,");
    if(!p) die("SLIP-PAGES",logpath,"page count not found");
    while(p>text && isdigit((unsigned char)p[-1])) p--;
    int pages=atoi(p); free(text);
    if(pages<1) die("SLIP-PAGES",logpath,"no pages");
    return pages;
}
static int ieq(const char *a,const char *b)
{ for(;*a && *b;a++,b++) if(tolower((unsigned char)*a)!=tolower((unsigned char)*b)) return 0; return *a==*b; }
/* The slogan: every "is this good for the" (whitespace runs read as one
 * space, comments ignored) is exactly the lead, and the next "company" is
 * the word in capitals. */
static void slogan(const char *path,const char *lead,const char *word,int stripped_already,char *src)
{
    char *own=NULL;
    if(!src) { own=src=readfile(path); }
    if(!stripped_already) stripcomments(src);
    size_t n=strlen(src),k=0; char *flat=xalloc(NULL,n+1),*low=xalloc(NULL,n+1);
    for(size_t i=0;i<n;i++) {
        if(isspace((unsigned char)src[i])) { if(k && flat[k-1]!=' ') flat[k++]=' '; }
        else flat[k++]=src[i];
    }
    flat[k]=0;
    for(size_t i=0;i<=k;i++) low[i]=(char)tolower((unsigned char)flat[i]);
    char lowlead[128],lowword[64];
    FMT(lowlead,"%s",lead); FMT(lowword,"%s",word);
    for(char *q=lowlead;*q;q++) *q=(char)tolower((unsigned char)*q);
    for(char *q=lowword;*q;q++) *q=(char)tolower((unsigned char)*q);
    for(char *p=low;(p=strstr(p,lowlead));p++) {
        size_t at=(size_t)(p-low);
        if(strncmp(flat+at,lead,strlen(lead))) die("SLOGAN-CASE",path,"the slogan's words are not \"Is This Good for the\"");
        char *c=strstr(p+strlen(lowlead),lowword);
        if(!c || c-p>200) die("SLOGAN-CASE",path,"the slogan without its COMPANY");
        if(strncmp(flat+(c-low),word,strlen(word))) die("SLOGAN-CASE",path,"COMPANY is not in capitals");
    }
    free(flat); free(low); free(own);
}
static void inconsistency(int argc,char **argv)
{
    if(argc<7) die("USAGE","inconsistency","register.json class-dir source-root gate-root book...");
    JV *root=loadregister(argv[2]);
    JV *budget=jneed(root,"budget",'o',"register"),*sl=jneed(root,"slogan",'o',"register");
    double bmin=jneed(budget,"per_100_pages_min",'n',"budget")->num,bmax=jneed(budget,"per_100_pages_max",'n',"budget")->num;
    int babs=(int)jneed(budget,"absolute_min",'n',"budget")->num;
    const char *lead=jneed(sl,"lead",'s',"slogan")->s,*word=jneed(sl,"word",'s',"slogan")->s;
    if(strcmp(jneed(sl,"text",'s',"slogan")->s,"Is This Good for the COMPANY?") || strlen(lead)>100 || strlen(word)>60)
        jfail("slogan: AN-27 fixes \"Is This Good for the COMPANY?\"");
    const char *classdir=argv[3],*srcroot=argv[4],*gateroot=argv[5];
    int nb=argc-6; char **books=argv+6; char path[1024];
    Heads *heads=xalloc(NULL,(size_t)nb*sizeof(*heads)); memset(heads,0,(size_t)nb*sizeof(*heads));
    for(int b=0;b<nb;b++) {
        FMT(path,"%s/%s/main.toc",gateroot,books[b]); loadheads(path,&heads[b]);
    }
    size_t total=0;
    for(int b=0;b<nb;b++) {
        const char *book=books[b]; char dir[1024],**files,famname[64]=""; int fam=-1;
        FMT(dir,"%s/%s",srcroot,book);
        /* the family, from \documentclass[...] of main.tex */
        FMT(path,"%s/main.tex",dir);
        { char *m=readfile(path),*o=strstr(m,"\\documentclass["),opts[256];
          if(!o) die("SLIP-FAMILY",path,"no \\documentclass[...]");
          o+=15; size_t l=strcspn(o,"]"); if(l>=sizeof(opts)) die("SLIP-FAMILY",path,"options");
          memcpy(opts,o,l); opts[l]=0;
          for(char *t=strtok(opts,", ");t;t=strtok(NULL,", ")) {
              if(instrlist(head_title,t)) fam=0; else if(instrlist(head_sentence,t)) fam=1; else continue;
              FMT(famname,"%s",t); break; }
          if(fam<0) die("SLIP-FAMILY",path,"no family option the register knows");
          free(m); }
        FMT(path,"%s/%s/main.log",gateroot,book);
        int pages=pagesof(path);
        int floor_=(int)((bmin*pages+99.999)/100); if(floor_<babs) floor_=babs;
        int ceil_=(int)(bmax*pages/100);
        if(ceil_<floor_) die("SLIP-BUDGET",book,"the ceiling is below the floor for this page count");
        /* source slips */
        SrcSlip *src=NULL; size_t ns=0; size_t nf=texfiles(dir,&files);
        char **blanked=xalloc(NULL,nf*sizeof(*blanked));
        for(size_t f=0;f<nf;f++) {
            char *s=readfile(files[f]); stripcomments(s);
            char *bl=xalloc(NULL,strlen(s)+1); strcpy(bl,s);
            for(char *p=s;(p=strstr(p,"\\Slip"));) {
                char ctx[600],idb[INC_TEXT],txt[INC_TEXT]; int line=1;
                for(char *q=s;q<p;q++) if(*q=='\n') line++;
                FMT(ctx,"%s:%d",files[f],line);
                if(isalpha((unsigned char)p[5])) { p+=5; continue; }
                const char *q=group(p+5,idb,sizeof(idb),ctx);
                q=group(q,txt,sizeof(txt),ctx);
                if(strchr(txt,'\\') || strchr(txt,'{')) die("SLIP-TEXT",ctx,"a slip is plain text (no macros, no braces)");
                for(char *t=txt;*t;t++) if(*t=='\n') *t=' ';
                int e=entryidx(idb);
                if(e<0) die("SLIP-UNKNOWN",ctx,idb);
                src=xalloc(src,(ns+1)*sizeof(*src)); memset(&src[ns],0,sizeof(*src));
                FMT(src[ns].file,"%s",files[f]+strlen(srcroot)+1);
                src[ns].line=line; src[ns].entry=e; FMT(src[ns].text,"%s",txt); ns++;
                for(char *z=bl+(p-s);z<bl+(q-s);z++) if(*z!='\n') *z=' ';
                p=(char *)q;
            }
            free(s); blanked[f]=bl;
        }
        char detail[512];
        if((int)ns<floor_) { FMT(detail,"%zu slips in %d pages; the register's floor is %d (%g per 100 pages, at least %d)",ns,pages,floor_,bmin,babs); die("SLIP-BUDGET-LOW",book,detail); }
        if((int)ns>ceil_) { FMT(detail,"%zu slips in %d pages; the ceiling is %d (%g per 100 pages): more than QA would pass",ns,pages,ceil_,bmax); die("SLIP-BUDGET-HIGH",book,detail); }
        int count[INC_MAXE]={0};
        for(size_t i=0;i<ns;i++) {
            IncEntry *e=&ient[src[i].entry]; char ctx[600]; FMT(ctx,"%s:%d %s",src[i].file,src[i].line,e->id);
            if(e->families && !instrlist(e->families,famname)) die("SLIP-FAMILY",ctx,"the entry does not apply to this book's family");
            if(!rmatch(&e->match[fam],src[i].text)) die("SLIP-TEXT",ctx,src[i].text);
            char low[INC_TEXT]; for(size_t k=0;k<=strlen(src[i].text);k++) low[k]=(char)tolower((unsigned char)src[i].text[k]);
            if(strstr(low,"good for the") || strstr(low,"company")) die("SLOGAN-CASE",ctx,"the slogan is not a slip");
            count[src[i].entry]++;
        }
        for(size_t e=0;e<nient;e++) {
            if(count[e]<ient[e].minb && (!ient[e].families || instrlist(ient[e].families,famname))) {
                FMT(detail,"%s used %d times, at least %d per book",ient[e].id,count[e],ient[e].minb); die("SLIP-MIN",book,detail); }
            if(count[e]>ient[e].maxb) { FMT(detail,"%s used %d times, at most %d per book",ient[e].id,count[e],ient[e].maxb); die("SLIP-MAX",book,detail); }
            if(count[e]) ient[e].used=1;
        }
        /* printings */
        PrnSlip *prn=NULL; size_t np=0; const char *docs[3]={"main","cover","spine"};
        for(int d=0;d<3;d++) {
            FMT(path,"%s/%s.tex",dir,docs[d]);
            struct stat st; if(stat(path,&st)) continue;
            FMT(path,"%s/%s/%s.slips",gateroot,book,docs[d]);
            char *t=readfile(path),*line=t;
            while(*line) {
                char *end=strchr(line,'\n'); if(end) *end=0;
                if(*line) {
                    char f[4][INC_TEXT]; const char *q=line; int k=0;
                    for(;k<4;k++) { size_t l=k<3?strcspn(q,"|"):strlen(q); if(l>=INC_TEXT) die("SLIP-PRINTED",path,line);
                        memcpy(f[k],q,l); f[k][l]=0; q+=l; if(k<3) { if(*q!='|') die("SLIP-PRINTED",path,line); q++; } }
                    int e=entryidx(f[0]),pl=placeidx(f[1]);
                    if(e<0) die("SLIP-UNKNOWN",path,line);
                    if(pl<0) die("SLIP-PRINTED",path,line);
                    if(!(ient[e].places & (1<<pl))) { FMT(detail,"%s printed in the %s on page %s (\"%s\"); allowed: see spec/pubs_inconsistencies.json",f[0],f[1],f[2],f[3]); die("SLIP-PLACE",path,detail); }
                    prn=xalloc(prn,(np+1)*sizeof(*prn)); memset(&prn[np],0,sizeof(*prn));
                    prn[np].entry=e; prn[np].place=pl; FMT(prn[np].page,"%s",f[2]); FMT(prn[np].text,"%s",f[3]);
                    for(size_t i=0;i<ns;i++) if(src[i].entry==e && !strcmp(src[i].text,f[3])) { src[i].printed=1; prn[np].sourced=1; }
                    if(!prn[np].sourced) die("SLIP-UNSOURCED",path,line);
                    np++;
                }
                if(!end) break;
                line=end+1;
            }
            free(t);
        }
        for(size_t i=0;i<ns;i++) if(!src[i].printed) {
            FMT(detail,"%s:%d %s \"%s\" is in the sources but never printed",src[i].file,src[i].line,ient[src[i].entry].id,src[i].text);
            die("SLIP-NOT-PRINTED",book,detail); }
        for(size_t i=0;i<ns;i++) {
            const char *chk=ient[src[i].entry].check; int ok;
            if(!chk) continue;
            if(!strcmp(chk,"case-differs-from-heading")) {
                ok=0; for(size_t h=0;h<heads[b].n;h++) if(ieq(heads[b].h[h],src[i].text) && strcmp(heads[b].h[h],src[i].text)) ok=1;
                if(!ok) { FMT(detail,"%s:%d \"%s\" is not a head of this book in another capitalisation",src[i].file,src[i].line,src[i].text); die("SLIP-INDEX-CASE",book,detail); }
            } else {
                for(int o=0;o<nb;o++) for(size_t h=0;h<heads[o].n;h++) if(ieq(heads[o].h[h],src[i].text)) {
                    FMT(detail,"%s:%d \"%s\" is a current head of %s, not an old name",src[i].file,src[i].line,src[i].text,books[o]); die("SLIP-XREF-CURRENT",book,detail); }
            }
        }
        /* no variant outside a \Slip; the slogan exact */
        for(size_t f=0;f<nf;f++) {
            for(size_t e=0;e<nient;e++) if(ient[e].hasscan) {
                char *l=blanked[f]; int line=1;
                while(l) { char *end=strchr(l,'\n'); if(end) *end=0;
                    if(rmatch(&ient[e].scan,l)) { FMT(detail,"%s:%d: %s variant outside a \\Slip (every slip is registered and budgeted)",files[f],line,ient[e].id); die("SLIP-UNREGISTERED",book,detail); }
                    if(end) { *end='\n'; l=end+1; line++; } else l=NULL; }
            }
            slogan(files[f],lead,word,1,blanked[f]);
            free(blanked[f]); free(files[f]);
        }
        free(blanked); free(files);
        printf(">>> test-pubs-inconsistency: %s (%s): %d pages, %zu slips (floor %d, ceiling %d)\n",book,famname,pages,ns,floor_,ceil_);
        for(size_t i=0;i<ns;i++) {
            printf("    %s %s:%d",ient[src[i].entry].id,src[i].file,src[i].line);
            char seen[INC_PLACES][160]; memset(seen,0,sizeof(seen));
            for(size_t p=0;p<np;p++) if(prn[p].entry==src[i].entry && !strcmp(prn[p].text,src[i].text)) {
                size_t l=strlen(seen[prn[p].place]);
                if(l<120 && snprintf(seen[prn[p].place]+l,sizeof(seen[0])-l,"%s%s",l?",":"",prn[p].page)<0) die("FORMAT","seen","");
            }
            for(int p=0;p<INC_PLACES;p++) if(*seen[p]) printf(" %s %s",inc_place[p],seen[p]);
            printf(": \"%s\"\n",src[i].text);
        }
        total+=ns; free(src); free(prn);
    }
    for(size_t e=0;e<nient;e++) if(!ient[e].used) die("SLIP-UNUSED",ient[e].id,"no book of the run carries this register entry");
    /* the class sets the slogan too */
    { DIR *d=opendir(classdir); struct dirent *e; if(!d) die("INPUT",classdir,"opendir");
      while((e=readdir(d))) { size_t l=strlen(e->d_name);
          if(l>4 && (!strcmp(e->d_name+l-4,".sty") || !strcmp(e->d_name+l-4,".cls"))) {
              FMT(path,"%s/%s",classdir,e->d_name); slogan(path,lead,word,0,NULL); } }
      closedir(d); }
    printf(">>> test-pubs-inconsistency: green (%d books, %zu slips, all %zu register entries used, slogan exact)\n",nb,total,nient);
}
int main(int argc,char **argv)
{
    if(argc>=2 && !strcmp(argv[1],"inventory")) inventory(argc,argv);
    else if(argc>=5 && !strcmp(argv[1],"commands")) crosscheck(argv[2],argc-4,argv+4,argv[3]);
    else if(argc>=2 && !strcmp(argv[1],"srclockstep")) srclockstep(argc,argv);
    else if(argc==4 && !strcmp(argv[1],"monotonic")) monotonic(argv[2],argv[3]);
    else if(argc>=4 && !strcmp(argv[1],"mentions")) {
        Set pairs=list(argv[2],0),ship={0},mentions={0};char nb[MAX_NAME];
        for(size_t i=0;i<pairs.n;i++) add(&ship,namepart(pairs.item[i],nb));
        /* An inline mention names a command by its first word: \cmd{CHKDSK /F}
         * mentions CHKDSK, \cmd{ECHO OFF} mentions ECHO. */
        for(int f=3;f<argc;f++) { Set m=list(argv[f],-1); for(size_t i=0;i<m.n;i++) {
            m.item[i][strcspn(m.item[i]," ")]=0; if(*m.item[i]) add(&mentions,m.item[i]); } }
        for(size_t i=0;i<mentions.n;i++) if(!has(&ship,mentions.item[i]))
            die("DOC-NOT-SHIPPED",argv[3],mentions.item[i]);
        printf(">>> test-pubs-mentions: green (%zu distinct inline commands)\n",mentions.n);
    }
    else if(argc>=4 && !strcmp(argv[1],"keys")) {
        /* InitechWord running-text keys (initech-wpc.sty \K -> <job>.wpkeys):
         * every key of a mention ("Shift-F8", "Home, Home, Up") must be a
         * single shipped key, NAME|wpkey of the inventory, or a key that acts as
         * another (NAME=KEY|wpkey), or a key that shows the grey name of a
         * feature InitechWord does not perform (NAME=Print|wpgray) (GC-01). */
        Set pairs=list(argv[2],0);size_t n=0;
        for(int f=3;f<argc;f++) { Set m=list(argv[f],-1); for(size_t i=0;i<m.n;i++) {
            const char *p=m.item[i];
            while(*p) {
                char tok[MAX_NAME],want[MAX_NAME+8];size_t k=strcspn(p,", ");
                if(k) { memcpy(tok,p,k);tok[k]=0;snprintf(want,sizeof want,"%s|wpkey",tok);
                    int ok=has(&pairs,want);
                    /* a key that acts as another key: "Shift-Backspace=Backspace|wpkey" */
                    if(!ok) { size_t tl=strlen(tok);
                        for(size_t q=0;q<pairs.n && !ok;q++) { const char *it=pairs.item[q];size_t il=strlen(it);
                            ok=!strncmp(it,tok,tl) && it[tl]=='=' && ((il>6 && !strcmp(it+il-6,"|wpkey")) ||
                                (il>7 && !strcmp(it+il-7,"|wpgray"))); } }
                    if(!ok) die("DOC-NOT-SHIPPED",argv[f],m.item[i]);
                    n++; }
                p+=k;while(*p==',' || *p==' ') p++;
            } } }
        printf(">>> test-pubs-iword-keys: green (%zu keys named in running text, every one shipped)\n",n);
    }
    else if(argc==5 && !strcmp(argv[1],"same")) same(argv[2],argv[3],argv[4]);
    else if(argc>=4 && !strcmp(argv[1],"samirmentions")) samirmentions(argc,argv);
    else if(argc>=4 && !strcmp(argv[1],"messages")) messages(argc,argv);
    else if(argc>=4 && !strcmp(argv[1],"lint")) {
        denied=list(argv[2],0);
        for(size_t i=0;i<denied.n;i++) for(char *p=denied.item[i];*p;p++) *p=(char)tolower((unsigned char)*p);
        for(int i=3;i<argc;i++) walk(argv[i]);
        puts(">>> test-pubs-lint: green (ASCII, period vocabulary/years, part numbers and print codes)");
    } else if(argc>=7 && !strcmp(argv[1],"inconsistency")) inconsistency(argc,argv);
    else die("USAGE","pubs_gate","inventory|commands|srclockstep|monotonic|keys|same|lint|samirmentions|messages|inconsistency");
    return 0;
}
