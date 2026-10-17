/* Deterministic serial marker storage transform (initech-tode round 4).
 * Ref: PRD Sec 5/6.1/8, CLAUDE.md Rules 6/11; existing kmain.c is the text
 * authority. Input is preprocessed C, so each mutant retains only its own
 * marker vocabulary. Never transform a non-output string or an argument with
 * a call / side effect. Original C vs generated C is the host byte oracle.
 * Factory C only. The generated decoder uses the original serial primitives.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAP (2u * 1024u * 1024u)
#define GROUPS 2048u
#define BYTES 65536u
static char source[CAP];
static unsigned char payload[BYTES], dict[128][2];
static unsigned offset[GROUPS], length[GROUPS], groups, used, ndict;
static int tbx_mode;
static unsigned counts[65536];

static void die(const char *s) { fprintf(stderr,"serial_trace_pack: %s\n",s); exit(1); }
static int ident(int c) { return isalnum((unsigned char)c) || c == '_'; }
static size_t space(size_t p) { while (isspace((unsigned char)source[p])) ++p; return p; }
static size_t quoted(size_t p)
{
    char quote=source[p++];
    while(source[p] && source[p]!=quote) {
        if(source[p]=='\\' && source[p+1]) ++p;
        ++p;
    }
    if(!source[p]) die("unterminated quoted token");
    return p+1;
}
static int escaped(size_t *p)
{
    int c=(unsigned char)source[(*p)++];
    if(c!='\\') return c;
    c=(unsigned char)source[(*p)++];
    if(c>='0' && c<='7') {
        int v=c-'0';
        for(int i=1;i<3 && source[*p]>='0' && source[*p]<='7';i++)
            v=v*8+source[(*p)++]-'0';
        return v;
    }
    if(c=='x') {
        int v=0;
        if(!isxdigit((unsigned char)source[*p])) die("empty hex escape");
        while(isxdigit((unsigned char)source[*p])) {
            int d=tolower((unsigned char)source[(*p)++]);
            v=v*16+(d<='9'?d-'0':d-'a'+10);
        }
        return v;
    }
    switch(c) {
    case 'n': return '\n'; case 'r': return '\r'; case 't': return '\t';
    case 'b': return '\b'; case 'a': return '\a'; case 'v': return '\v';
    case 'f': return '\f'; case '\\': case '\'': case '"': return c;
    default: die("unsupported C escape"); return 0;
    }
}
static void byte(unsigned char *b,unsigned *n,int c)
{
    if(*n>=BYTES-1 || c<0 || c>127) die("non-ASCII / oversized marker");
    b[(*n)++]=(unsigned char)c;
}
/* Decode only a complete literal argument; otherwise preserve it as an arg. */
static int literal(size_t a,size_t z,unsigned char *b,unsigned *n,int quote)
{
    size_t p=space(a); unsigned saved=*n;
    if(source[p]!=quote) return 0;
    do {
        ++p;
        while(p<z && source[p]!=quote) {
            int c=escaped(&p);
            if(c==0 || c<6) { *n=saved; return 0; }
            byte(b,n,c);
        }
        if(p>=z) die("bad literal boundary");
        p=space(p+1);
    } while(quote=='"' && source[p]==quote);
    if(p!=z) { *n=saved; return 0; }
    return 1;
}
typedef struct call { size_t arg,endarg,end; unsigned kind; } call_t;
static int call(size_t p,call_t *out)
{
    static const char *const names[]={"serial_putu","serial_puti",
        "serial_puthex32","serial_puts","serial_putc"};
    for(unsigned i=0;i<5;i++) {
        size_t n=strlen(names[i]),q;
        if(strncmp(source+p,names[i],n) || ident(source[p+n])) continue;
        q=space(p+n); if(source[q]!='(') return 0;
        out->arg=++q;
        unsigned depth=1;
        for(;source[q];q++) {
            if(source[q]=='"' || source[q]=='\'') {q=quoted(q)-1; continue;}
            if(source[q]=='(') ++depth;
            if(source[q]==')' && --depth==0) break;
        }
        if(!source[q]) return 0;
        out->endarg=q;
        q=space(q+1); if(source[q]!=';') return 0;
        /* Group only side-effect-free arguments. Casts are fine; identifier(
         * is a function call (or sizeof), so conservatively leave it alone. */
        for(size_t x=out->arg;x<out->endarg;x++) {
            if(source[x]=='"' || source[x]=='\'') { x=quoted(x)-1; continue; }
            if(source[x]=='=' || source[x]==',' || source[x]=='?' ||
               (source[x]=='+' && source[x+1]=='+') ||
               (source[x]=='-' && source[x+1]=='-')) return 0;
            if(ident(source[x])) {
                while(ident(source[x+1])) ++x;
                if(source[space(x+1)]=='(') return 0;
            }
        }
        out->kind=i+1; out->end=q+1; return 1;
    }
    return 0;
}
static void pack(void)
{
    for(ndict=0;ndict<128;ndict++) {
        memset(counts,0,sizeof counts);
        for(unsigned g=0;g<groups;g++) {
            unsigned a=offset[g],z=a+length[g];
            for(unsigned p=a;p+1<z;p++) {
                ++counts[256u*payload[p]+payload[p+1]];
                if(payload[p]==payload[p+1]) ++p; /* nonoverlapping pairs */
            }
        }
        unsigned best=0;
        for(unsigned i=1;i<65536;i++) if(counts[i]>counts[best]) best=i;
        if(counts[best]<4) break;
        dict[ndict][0]=(unsigned char)(best/256);
        dict[ndict][1]=(unsigned char)best;
        unsigned w=0;
        for(unsigned g=0;g<groups;g++) {
            unsigned p=offset[g],z=p+length[g]; offset[g]=w;
            while(p<z) {
                if(p+1<z && payload[p]==dict[ndict][0] && payload[p+1]==dict[ndict][1]) {
                    payload[w++]=(unsigned char)(128+ndict); p+=2;
                } else payload[w++]=payload[p++];
            }
            length[g]=w-offset[g]; payload[w++]=0;
        }
        used=w;
    }
}
static void arrays(FILE *f)
{
    if(tbx_mode) fputs(
        "\nstatic void serial_putc(char c) { char b[2]={c,0}; serial_puts(b); }\n"
        "static void serial_puthex32(unsigned v) { tputx(v,8); }\n",f);
    fprintf(f,"static const unsigned char st_pairs[][2]={");
    if(!ndict) fprintf(f,"{0,0}");
    for(unsigned i=0;i<ndict;i++) fprintf(f,"%s{%u,%u}",i?",":"",dict[i][0],dict[i][1]);
    fprintf(f,"};\n");
    fputs("static void st_trace(const unsigned char *p,...) {\n"
        " __builtin_va_list ap; __builtin_va_start(ap,p);\n"
        " while(*p) { unsigned char stack[129]; unsigned n=1; stack[0]=*p++;\n"
        "  while(n) { unsigned c=stack[--n];\n"
        "   if(c>=128) { stack[n++]=st_pairs[c-128][1]; stack[n++]=st_pairs[c-128][0]; }\n"
        "   else if(c<6) { unsigned long v=__builtin_va_arg(ap,unsigned long);\n"
        "    switch(c) { case 1: serial_putu((unsigned)v); break;\n"
        "    case 2: serial_puti((int)v); break; case 3: serial_puthex32((unsigned)v); break;\n"
        "    case 4: serial_puts((const char *)v); break; case 5: serial_putc((char)v); break; }\n"
        "   } else {\n"
        "#ifdef SERIAL_TRACE_MUT_BYTE\n"
        "    if(c>=32) c^=1;\n"
        "#endif\n"
        "    serial_putc((char)c); }\n"
        "  }\n"
        " } __builtin_va_end(ap);\n"
        "}\n",f);
}
int main(int argc,char **argv)
{
    tbx_mode=argc==4 && strcmp(argv[3],"tbx")==0;
    if(argc!=3 && !tbx_mode) die("usage: serial_trace_pack input.i output.c [tbx]");
    FILE *in=fopen(argv[1],"rb"); if(!in) die("open input");
    size_t size=fread(source,1,CAP-1,in); if(ferror(in)||!feof(in)) die("read input / capacity"); fclose(in);
    FILE *out=tmpfile(); if(!out) die("temporary output");
    size_t p=0,last=0;
    while(p<size) {
        if(source[p]=='"' || source[p]=='\'') {p=quoted(p); continue;}
        if(ident(source[p]) && (p==0 || !ident(source[p-1]))) {
            call_t calls[64]; unsigned count=0; size_t q=p;
            size_t prev=p;
            while(prev && isspace((unsigned char)source[prev-1])) --prev;
            /* A call after if(...), else, do or a label can be the single
             * controlled statement. Never absorb the following statement. */
            if(call(q,&calls[0])) {
                count=1; q=space(calls[0].end);
                if(prev && (source[prev-1]==';' || source[prev-1]=='{' || source[prev-1]=='}'))
                    while(count<64 && call(q,&calls[count])) { q=space(calls[count++].end); }
            }
            unsigned char single[BYTES]; unsigned nsingle=0;
            int single_string=count==1 && calls[0].kind==4 &&
                literal(calls[0].arg,calls[0].endarg,single,&nsingle,'"');
            if(count>=2 || single_string) {
                unsigned char bytes[BYTES]; unsigned n=0;
                unsigned char argkind[64]={0};
                for(unsigned i=0;i<count;i++) {
                    call_t *c=&calls[i];
                    if((c->kind==4 || c->kind==5) && literal(c->arg,c->endarg,bytes,&n,c->kind==4?'"':'\'')) continue;
                    byte(bytes,&n,(int)c->kind); argkind[i]=1;
                }
                if(groups>=GROUPS || used+n+1>=BYTES) die("marker table capacity");
                fwrite(source+last,1,p-last,out);
                fprintf(out,"st_trace(st_record_%u",groups);
                for(unsigned i=0;i<count;i++) if(argkind[i]) {
                    fputs(",(unsigned long)(",out);
                    fwrite(source+calls[i].arg,1,calls[i].endarg-calls[i].arg,out);
                    fputc(')',out);
                }
                fputs(");",out);
                offset[groups]=used; length[groups++]=n;
                memcpy(payload+used,bytes,n); used+=n; payload[used++]=0;
                p=calls[count-1].end; last=p; continue;
            }
            while(ident(source[p])) ++p;
        } else ++p;
    }
    fwrite(source+last,1,size-last,out);
    if(groups) { pack(); arrays(out); }
    FILE *dest=fopen(argv[2],"wb"); if(!dest) die("open output");
    if(groups) {
        fputs("static void st_trace(const unsigned char *,...);\n",dest);
        /* String-pool records allow the linker to share identical suffixes.
         * Their addresses are only decoder inputs, never identity keys. */
        for(unsigned g=0;g<groups;g++) {
            fprintf(dest,"#define st_record_%u ((const unsigned char *)\"",g);
            for(unsigned i=0;i<length[g];i++) fprintf(dest,"\\%03o",payload[offset[g]+i]);
            fputs("\")\n",dest);
        }
    }
    if(fseek(out,0,SEEK_SET)) die("rewind output");
    for(;;) {
        unsigned char buf[4096]; size_t n=fread(buf,1,sizeof buf,out);
        if(n && fwrite(buf,1,n,dest)!=n) die("write output");
        if(n<sizeof buf) { if(ferror(out)) die("read temporary output"); break; }
    }
    if(fclose(dest) || fclose(out)) die("close output");
    fprintf(stderr,"serial_trace_pack: %u records, %u packed bytes, %u pairs\n",groups,used,ndict);
    return 0;
}
