/* Publications correctness oracle. Ref: PUB-REF-0001 GC-01/03/05/06,
 * PN-01/05; PRD Sec 8; CLAUDE.md Law 2, Rules 2/6. Host C only. */
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
/* A book's documented-command list, <job>.cmds (initech-syntax.sty
 * \RegisterCommand): NAME|KIND|STATE|ENTRY.  Several books' lists may be
 * given; a pair documented in two books is one documented item.  DRAFT
 * pairs (the build lacks the behaviour, D-14) are returned in *draft. */
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
            /* FLAIR kinds (bead initech-ipu5.11, initech-flair.sty): menu,
             * file, alert, operation -- checked by test-pubs-flair against
             * harness/pubs/flair_inventory.c, never by the DOS books' gate. */
            if(strcmp(f[1],"internal") && strcmp(f[1],"external") && strcmp(f[1],"batch") &&
               strcmp(f[1],"config") && strcmp(f[1],"int21") && strcmp(f[1],"menu") &&
               strcmp(f[1],"file") && strcmp(f[1],"alert") && strcmp(f[1],"operation"))
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
static void crosscheck(const char *shipped,int ndoc,char **docs,const char *allowpath)
{
    Set ship=list(shipped,0),doc={0},draft={0},allow=list(allowpath,1);
    char a[MAX_NAME],b[MAX_NAME];
    for(int i=0;i<ndoc;i++) cmdsfile(docs[i],&doc,&draft);
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
    printf(">>> test-pubs-commands: green (%zu shipped, %zu documented, %zu DRAFT, %zu explicit omissions)\n",ship.n,doc.n,draft.n,allow.n);
}
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
    while((c=fgetc(raw))!=EOF) if(c>126 || (c<32 && c!='\n' && !(c=='\t' && tabok)))
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
int main(int argc,char **argv)
{
    if(argc>=2 && !strcmp(argv[1],"inventory")) inventory(argc,argv);
    else if(argc>=5 && !strcmp(argv[1],"commands")) crosscheck(argv[2],argc-4,argv+4,argv[3]);
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
    else if(argc==5 && !strcmp(argv[1],"same")) same(argv[2],argv[3],argv[4]);
    else if(argc>=4 && !strcmp(argv[1],"lint")) {
        denied=list(argv[2],0);
        for(size_t i=0;i<denied.n;i++) for(char *p=denied.item[i];*p;p++) *p=(char)tolower((unsigned char)*p);
        for(int i=3;i<argc;i++) walk(argv[i]);
        puts(">>> test-pubs-lint: green (ASCII, period vocabulary/years, part numbers and print codes)");
    } else die("USAGE","pubs_gate","inventory|commands|monotonic|same|lint");
    return 0;
}
