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
static void inventory(int argc,char **argv)
{
    if(argc<5) die("USAGE","inventory","command.i int21.list APPS-paths...");
    Set s={0}; char *src=readfile(argv[2]);
    /* GCC can keep whitespace differently; the table name is unique here. */
    char *p=strstr(src,"table[] = {");
    if(!p) die("INVENTORY",argv[2],"command dispatch table missing");
    char *stop=strstr(p,"};");if(!stop) die("INVENTORY",argv[2],"unterminated dispatch table");
    while((p=strchr(p,'"')) && p<stop) {
        char *end=strchr(++p,'"');if(!end || end>=stop) die("INVENTORY",argv[2],"bad table name");
        char save=*end;*end=0;add(&s,p);*end=save;p=end+1;
    }
    if(s.n<10) die("INVENTORY",argv[2],"empty/truncated internal set");
    free(src);
    Set ints=list(argv[3],0);if(!ints.n) die("INVENTORY",argv[3],"empty INT21 register");
    for(size_t i=0;i<ints.n;i++) add(&s,ints.item[i]);
    size_t external=0;
    for(int i=4;i<argc;i++) {
        const char *name=strrchr(argv[i],'/');name=name?name+1:argv[i];
        char buf[MAX_NAME];if(strlen(name)>=sizeof(buf)) die("INVENTORY",name,"long path");
        strcpy(buf,name);char *dot=strrchr(buf,'.');
        if(dot && (!strcmp(dot,".COM") || !strcmp(dot,".EXE") || !strcmp(dot,".BAT"))) {
            *dot=0;add(&s,buf);external++;
        }
    }
    if(!external) die("INVENTORY","APPS","no shipped executables");
    qsort(s.item,s.n,sizeof(s.item[0]),cmpname);
    for(size_t i=0;i<s.n;i++) puts(s.item[i]);
}
static void crosscheck(const char *shipped,const char *documented,const char *allowpath)
{
    Set ship=list(shipped,0),doc=list(documented,0),allow=list(allowpath,1);
    if(!ship.n || !doc.n) die("COMMAND-INDEX",documented,"empty inventory/index");
    for(size_t i=0;i<doc.n;i++) if(!has(&ship,doc.item[i])) die("DOC-NOT-SHIPPED",documented,doc.item[i]);
    for(size_t i=0;i<allow.n;i++) {
        if(!has(&ship,allow.item[i])) die("STALE-ALLOWLIST",allowpath,allow.item[i]);
        if(has(&doc,allow.item[i])) die("STALE-ALLOWLIST",allowpath,allow.item[i]);
        printf("!!! test-pubs ALLOWED-OMISSION: %s (%s)\n",allow.item[i],allowpath);
    }
    for(size_t i=0;i<ship.n;i++) if(!has(&doc,ship.item[i]) && !has(&allow,ship.item[i]))
        die("SHIPPED-NOT-DOCUMENTED",documented,ship.item[i]);
    printf(">>> test-pubs-commands: green (%zu shipped, %zu entries, %zu explicit omissions)\n",ship.n,doc.n,allow.n);
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
    else if(argc==5 && !strcmp(argv[1],"commands")) crosscheck(argv[2],argv[3],argv[4]);
    else if(argc==4 && !strcmp(argv[1],"monotonic")) monotonic(argv[2],argv[3]);
    else if(argc==4 && !strcmp(argv[1],"mentions")) {
        Set ship=list(argv[2],0),mentions=list(argv[3],-1);
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
