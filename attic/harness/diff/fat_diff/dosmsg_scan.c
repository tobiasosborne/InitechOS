/* DEC-13 source-purity tooth of test-dosmsg (initech-bcg.13).
 * Ref: ADR-0003 5.13/7 item 4/App C; PRD 6.1. Factory C only.
 * Inventory ALL C strings and NASM data strings, not a diagnostic word regex.
 * Non-output tokens/debug text and non-diagnostic UI need exact, dated rows.
 * Adjacent C literals are also decoded together, so splitting a message cannot
 * hide an inline catalogue string. Comments/character constants are not text.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define CAP 4096
#define ROWS 2048
typedef struct { char path[256], literal[CAP]; int used; } row_t;
static row_t allow[ROWS], baseline[ROWS];
static int na, nb, errors, strings, refs;
static char names[256][32], messages[256][CAP];
static int nm;
static char **image_sources;
static int image_count;

static void die(const char *why) { fprintf(stderr,"RED dosmsg-source: %s\n",why); exit(1); }
static char *readfile(const char *path)
{
    FILE *f=fopen(path,"rb"); long n; char *p;
    if(!f) die(path);
    if(fseek(f,0,SEEK_END) || (n=ftell(f))<0 || fseek(f,0,SEEK_SET)) die("seek");
    p=calloc((size_t)n+1,1); if(!p) die("allocation");
    if(fread(p,1,(size_t)n,f)!=(size_t)n) die("read");
    fclose(f); return p;
}
static int rows(const char *path,row_t *r,int dated)
{
    FILE *f=fopen(path,"r"); char line[CAP+512]; int n=0;
    if(!f) die(path);
    while(fgets(line,sizeof line,f)) {
        char *a,*b,*c;
        if(line[0]=='#' || line[0]=='\n') continue;
        line[strcspn(line,"\r\n")]=0;
        a=strchr(line,'\t'); if(!a) die("bad allow row"); *a++=0;
        b=strchr(a,'\t'); if(b) *b++=0;
        if(dated) {
            if(!b || !(c=strchr(b,'\t'))) die("allow row needs date and reason");
            *c++=0;
            if(strlen(b)!=10 || b[4]!='-' || b[7]!='-' || strlen(c)<12)
                die("allow row needs ISO date and substantive reason");
        }
        if(n==ROWS || strlen(line)>=sizeof r[n].path || strlen(a)>=CAP) die("row overflow");
        strcpy(r[n].path,line); strcpy(r[n].literal,a); n++;
    }
    fclose(f); return n;
}
static int lookup(row_t *r,int n,const char *path,const char *literal)
{ for(int i=0;i<n;i++) if(!strcmp(path,r[i].path)&&!strcmp(literal,r[i].literal)) return i; return -1; }
/* Decode C escapes; NASM ordinary quotes retain backslashes literally. */
static const char *quoted(const char *p,char *raw,char *value,int nasm)
{
    char quote=*p; int nr=0,nv=0;
    raw[nr++]=*p++;
    while(*p && *p!=quote) {
        unsigned c=(unsigned char)*p++;
        if(nr>CAP-8 || nv>CAP-8) die("literal overflow");
        raw[nr++]=(char)c;
        if(!nasm && c=='\\') {
            c=(unsigned char)*p++; if(!c) die("unterminated escape"); raw[nr++]=(char)c;
            if(c=='n') c='\n'; else if(c=='r') c='\r'; else if(c=='t') c='\t';
            else if(c=='b') c='\b'; else if(c=='a') c=7;
            else if(c>='0'&&c<='7') {
                unsigned v=c-'0'; int k=1;
                while(k++<3 && *p>='0'&&*p<='7') { raw[nr++]=*p; v=v*8+(unsigned)(*p++-'0'); }
                c=v;
            } else if(c=='x') {
                unsigned v=0; int k=0;
                while(isxdigit((unsigned char)*p)) {
                    unsigned d=(unsigned char)*p; raw[nr++]=*p++;
                    v=v*16+(isdigit(d)?d-'0':(unsigned)tolower(d)-'a'+10); k++;
                    if(nr>CAP-8) die("hex escape overflow");
                }
                if(!k) die("bad hex escape");
                c=v;
            }
        }
        value[nv++]=(char)c;
    }
    if(*p!=quote) die("unterminated literal");
    raw[nr++]=*p++; raw[nr]=0; value[nv]=0; return p;
}
static void catalogue(const char *header)
{
    char *data=readfile(header),*p=data;
    while((p=strstr(p,"#define MSG_DOS_"))) {
        char raw[CAP]; int n=0; p+=8;
        while(*p&&!isspace((unsigned char)*p)) { if(n==31) die("macro name"); names[nm][n++]=*p++; }
        names[nm][n]=0; while(isspace((unsigned char)*p)) p++;
        if(*p!='"' || nm==255) die("catalogue header syntax");
        p=(char *)quoted(p,raw,messages[nm++],0);
    }
    free(data); if(!nm) die("empty catalogue header");
}
static void inline_check(const char *path,int line,char *value)
{
    size_t n=strlen(value);
    while(n && (value[n-1]=='\r'||value[n-1]=='\n'||value[n-1]=='$')) value[--n]=0;
    if(!n) return;
    for(int i=0;i<nm;i++) if(!strcmp(value,messages[i])) {
        fprintf(stderr,"RED dosmsg-source: %s:%d inline %s (use macro)\n",path,line,names[i]); errors++; break;
    }
}
static void scan(const char *path,const char *input,int inventory)
{
    char *data=readfile(input); const char *p=data;
    int nasm=strstr(path,".asm")!=NULL,line=1,group_line=1,db=0;
    char group[CAP]="";
    while(*p) {
        if(*p=='\n') {line++;db=0;p++;continue;}
        if(isspace((unsigned char)*p)) {p++;continue;}
        if((!nasm&&p[0]=='/'&&p[1]=='/') || (nasm&&*p==';')) {while(*p&&*p!='\n')p++;continue;}
        if(!nasm&&p[0]=='/'&&p[1]=='*') {
            p+=2; while(*p&&!(p[0]=='*'&&p[1]=='/')) {if(*p=='\n')line++;p++;}
            if(!*p) die("unterminated comment");
            p+=2;continue;
        }
        if((!nasm&&*p=='#') || (nasm&&*p=='%'&&!strncmp(p,"%include",8))) {
            /* Includes are dependency names. Macro definitions are still scanned. */
            const char *q=p+1; while(*q==' ')q++;
            if(!strncmp(q,"include",7)) {while(*p&&*p!='\n')p++;continue;}
        }
        if(*p=='"' || *p=='\'') {
            char raw[CAP],value[CAP]; int text=nasm?db:*p=='"';
            p=quoted(p,raw,value,nasm);
            /* Macro arguments can emit data too (e.g. SAY). Single-character
             * instruction operands are not message strings. */
            if(nasm&&strlen(value)>1)text=1;
            if(!text) continue;
            strings++;
            if(inventory==1) printf("%s\t%s\n",path,raw);
            else if(!inventory) {
                int i=lookup(allow,na,path,raw);
                if(i<0) {fprintf(stderr,"RED dosmsg-source: %s:%d unlisted literal %s\n",path,line,raw);errors++;}
                else allow[i].used++;
                if(strlen(group)+strlen(value)>=CAP) die("adjacent literal overflow");
                if(!*group) group_line=line;
                strcat(group,value);
            }
            else if(inventory==3) {
                if(strlen(group)+strlen(value)>=CAP)die("adjacent literal overflow");
                if(!*group)group_line=line;
                strcat(group,value);
            }
            continue;
        }
        if((!inventory||inventory==3) && *group) {inline_check(path,group_line,group);group[0]=0;}
        if(isalpha((unsigned char)*p)||*p=='_') {
            char token[128]; int n=0;
            while(isalnum((unsigned char)*p)||*p=='_') {if(n<127)token[n++]=*p;p++;} token[n]=0;
            if(nasm&&!strcmp(token,"db")) db=1;
            if(!strncmp(token,"MSG_DOS_",8)) {
                int found=0; for(int i=0;i<nm;i++) if(!strcmp(token,names[i]))found=1;
                if(inventory!=1&&!found) {fprintf(stderr,"RED dosmsg-source: %s:%d unknown macro %s\n",path,line,token);errors++;}
                refs++;
                if(image_count && found) {
                    for(int i=0;i<nm;i++) if(!strcmp(token,names[i])) {
                        size_t len=strcspn(messages[i],"%"); int present=0;
                        for(int k=0;k<image_count;k++) {
                            char *blob=readfile(image_sources[k]);
                            FILE *f=fopen(image_sources[k],"rb"); long size;
                            if(!f||fseek(f,0,SEEK_END)||(size=ftell(f))<0)die("image size");
                            fclose(f);
                            for(long j=0;j+(long)len<=size;j++)if(!memcmp(blob+j,messages[i],len)){present=1;break;}
                            free(blob);
                        }
                        if(!present){fprintf(stderr,"RED dosmsg-image: %s missing %s\n",path,token);errors++;}
                    }
                }
            }
        } else p++;
    }
    if((!inventory||inventory==3)&&*group)inline_check(path,group_line,group);
    free(data);
}
int main(int argc,char **argv)
{
    int inventory=argc>1&&!strcmp(argv[1],"--inventory");
    if(inventory) {for(int i=2;i<argc;i++)scan(argv[i],argv[i],1);return 0;}
    if(argc>=4&&!strcmp(argv[1],"--known")) {
        catalogue(argv[2]);
        for(int i=3;i<argc;i++)scan(argv[i],argv[i],3);
        if(errors)return 1;
        printf("GREEN dosmsg-source: original all-MILTON inline-catalogue check retained (%d files)\n",argc-3);
        return 0;
    }
    if(argc>=5&&!strcmp(argv[1],"--image")) {
        catalogue(argv[2]);image_sources=argv+4;image_count=argc-4;
        /* Image mode checks macro references only; no exception policy needed. */
        scan(argv[3],argv[3],2);
        if(errors)return 1;
        printf("GREEN dosmsg-image: %s references present in %d program image(s)\n",argv[3],image_count);return 0;
    }
    if(argc<6)die("usage: scan header allow baseline source... [--replace path file]");
    catalogue(argv[1]);na=rows(argv[2],allow,1);nb=rows(argv[3],baseline,0);
    for(int i=0;i<na;i++) {
        if(lookup(baseline,nb,allow[i].path,allow[i].literal)<0)die("allow-list growth beyond frozen baseline");
        if(lookup(allow,i,allow[i].path,allow[i].literal)>=0)die("duplicate allow row");
    }
    for(int i=4;i<argc;i++) {
        const char *input=argv[i];
        /* Mutants scan a temporary file under its real source identity. */
        if(i+3<argc&&!strcmp(argv[i+1],"--replace")) {
            if(strcmp(argv[i],argv[i+2]))die("replacement identity");
            input=argv[i+3];
            scan(argv[i],input,0);i+=3;
        } else scan(argv[i],input,0);
    }
    for(int i=0;i<na;i++) if(!allow[i].used) {
        fprintf(stderr,"RED dosmsg-source: stale allow row %s %s\n",allow[i].path,allow[i].literal);errors++;
    }
    if(errors)return 1;
    printf("GREEN dosmsg-source: %d literals, %d catalogue references, %d exact dated exceptions; no diagnostics inline\n",strings,refs,na);
    return 0;
}
