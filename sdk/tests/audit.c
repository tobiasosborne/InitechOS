/* Source consistency audit for the current simple C wrappers/dispatch table.
 * Tokenization ignores comments and strings. Fail closed on changed idioms;
 * do not silently treat a missing match as success. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MAXTOK 100000
static char tok[MAXTOK][128];
static unsigned nt;
static struct { char name[80],fn[80];int num,arity,defs,bindings,dispatch,args; } rows[128];
static unsigned nr;
static void die(const char *s) { fprintf(stderr,"ABI audit: %s\n",s);exit(1); }
static void tokenize(const char *path)
{
    FILE *f=fopen(path,"rb");if(!f)die(path);nt=0;int c;
    while((c=fgetc(f))!=EOF) {
        if(isspace(c))continue;
        if(c=='/') { int d=fgetc(f);if(d=='*'){int prev=0;while((c=fgetc(f))!=EOF){if(prev=='*'&&c=='/')break;prev=c;}continue;}if(d=='/'){while((c=fgetc(f))!=EOF&&c!='\n');continue;}if(d!=EOF)ungetc(d,f); }
        if(nt==MAXTOK)die("token capacity");
        unsigned k=0;tok[nt][k++]=(char)c;
        if(isalnum(c)||c=='_') { while((c=fgetc(f))!=EOF&&(isalnum(c)||c=='_')){if(k==127)die("token length");tok[nt][k++]=(char)c;}if(c!=EOF)ungetc(c,f); }
        else if(c=='"'||c=='\''){int q=c;while((c=fgetc(f))!=EOF){if(c=='\\'){(void)fgetc(f);continue;}if(c==q)break;} }
        tok[nt++][k]=0;
    }
    fclose(f);
}
static int eq(unsigned i,const char *s) { return i<nt&&!strcmp(tok[i],s); }
static int lookup(const char *s) { for(unsigned i=0;i<nr;i++)if(!strcmp(rows[i].name,s))return (int)i;return -1; }
static int value(const char *s)
{
    char *end;long n=strtol(s,&end,0);if(end!=s&&(!*end||!strcmp(end,"u")))return (int)n;
    const char *prefix="TBX_ARGC_";
    if(!strncmp(s,prefix,strlen(prefix))) { char code[128];snprintf(code,sizeof code,"TBX_%s",s+strlen(prefix));int i=lookup(code);if(i>=0)return rows[i].arity; }
    die("unrecognized arity expression");return -2;
}
int main(int argc,char **argv)
{
    if(argc!=6)die("usage: audit table header cp-header runtime OS-source");
    FILE *f=fopen(argv[1],"r");if(!f)die("table");char line[512];
    while(fgets(line,sizeof line,f)) {
        if(line[0]=='#')continue;
        if(nr==128||sscanf(line,"%79s %i %d %79s",rows[nr].name,&rows[nr].num,&rows[nr].arity,rows[nr].fn)!=4)die("table row");
        for(unsigned j=0;j<nr;j++)if(rows[j].num==rows[nr].num||!strcmp(rows[j].name,rows[nr].name))die("duplicate stable row");
        nr++;
    }
    fclose(f);
    /* Active/reserved numbers compared to independently pinned legacy table.
     * An extra trap define in the numbered ranges is reported too. */
    for(int file=2;file<=3;file++) {
        tokenize(argv[file]);
        for(unsigned k=0;k+3<nt;k++)if(eq(k,"#")&&eq(k+1,"define")) {
            int i=lookup(tok[k+2]);
            if(i<0) {
                const char *name=tok[k+2];
                if(!strncmp(name,"TBX_",4)&&!strncmp(tok[k+3],"0x",2)&&
                   strncmp(name,"TBX_GATE_",9)&&strncmp(name,"TBX_EVT_",8)&&
                   strncmp(name,"TBX_FIND_",9)&&strncmp(name,"TBX_TEXT_",9))
                    die("unknown numbered trap in header");
                continue;
            }
            char *end;long n=strtol(tok[k+3],&end,0);
            if(end==tok[k+3]||n!=rows[i].num)die(rows[i].name);
            rows[i].defs++;
        }
    }
    tokenize(argv[4]);char fn[128]="";
    for(unsigned k=0;k+5<nt;k++) {
        if(eq(k,"int32_t")&&!strncmp(tok[k+1],"tbx_",4)&&eq(k+2,"("))snprintf(fn,sizeof fn,"%s",tok[k+1]);
        if(!eq(k,"tbx_trap")||!eq(k+1,"(")||strncmp(tok[k+2],"TBX_",4))continue;
        int i=lookup(tok[k+2]);if(i<0)die("binding has no stable table row");
        if(strcmp(fn,rows[i].fn)||!eq(k+3,",")||value(tok[k+4])!=rows[i].arity)die("binding name/arity differs");
        rows[i].bindings++;
        /* Nonzero calls pass a local dword array; check initializer count. */
        if(rows[i].arity>0) {
            unsigned start=k;while(start>0&&!eq(start,"{"))start--;
            if(start==k)die("binding argument array not found");
            int depth=1,count=1;for(unsigned x=start+1;x<k&&depth;x++) { if(eq(x,"{"))depth++;if(eq(x,"}"))depth--;if(eq(x,",")&&depth==1)count++; }
            if(count!=rows[i].arity)die("binding argument array arity");
        }
    }
    tokenize(argv[5]);unsigned sw=0;
    for(unsigned k=0;k+3<nt;k++)if(eq(k,"switch")&&eq(k+1,"(")&&eq(k+2,"ax")&&eq(k+3,")")){sw=k+4;break;}
    if(!sw||!eq(sw,"{"))die("OS switch(ax) not found; publish dispatch manifest");
    int depth=1;for(unsigned k=sw+1;k+2<nt&&depth;k++) {
        if(eq(k,"{"))depth++;
        if(eq(k,"}"))depth--;
        if(eq(k,"case")&&eq(k+2,":")){int i=lookup(tok[k+1]);if(i<0)die("OS dispatch has no stable row");rows[i].dispatch++;}
    }
    /* The OS argn[] table uses explicit uint8_t casts. Zero-argument verbs
     * intentionally omit rows; default argc=0. */
    for(unsigned k=0;k+12<nt;k++)if(eq(k,"{")&&eq(k+1,"(")&&eq(k+2,"uint8_t")&&eq(k+3,")")) {
        int i=lookup(tok[k+4]);if(i<0)continue;
        if(!eq(k+5,",")||!eq(k+6,"(")||!eq(k+7,"uint8_t")||!eq(k+8,")")||!eq(k+9,"("))die("OS arity table idiom changed");
        if(value(tok[k+10])!=rows[i].arity)die("OS arity differs");
        rows[i].args++;
    }
    unsigned active=0;
    for(unsigned i=0;i<nr;i++) {
        if(rows[i].defs!=1)die("missing/duplicate header trap");
        int want=rows[i].arity>=0;
        if(rows[i].bindings!=want||rows[i].dispatch!=want||(rows[i].arity>0?rows[i].args!=1:rows[i].args>1)) {fprintf(stderr,"%s: bindings=%d dispatch=%d argrows=%d\n",rows[i].name,rows[i].bindings,rows[i].dispatch,rows[i].args);die("missing/duplicate binding or OS dispatch/arity row");}
        active+=(unsigned)want;
    }
    printf("ABI: %u active traps, %u reserved; stable numbers, unique bindings and OS dispatch/arity agree\n",active,nr-active);
    return 0;
}
