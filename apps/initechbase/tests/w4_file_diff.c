/* Factory C. initech-9nzpr, DEC-03a.10, PRD 6.6.
 * Independent III+ readers: dbase3-decomp/specs/file-formats/{frm,lbl,qry,
 * scr-and-fmt,vue,prg-and-aux-files}.md and environment/config-db.md.
 * See harness/README.md for the exhaustive normalization policy. Unknown
 * fields remain significant. No artifact headers or renderer tables used.
 * Build once per KIND (frm/lbl/qry/scr/fmt/vue/prg/config/exact).
 * Exit: 0 equal, 1 different/invalid format, 2 usage or I/O failure.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KIND
#error KIND must name the file format
#endif
typedef struct { unsigned char *p; size_t n; } blob;
static const char *input;
static void invalid(const char *why)
{
    fprintf(stderr, "RED: %s_diff invalid %s: %s\n", KIND, input, why);
    exit(1);
}
static unsigned word(const blob *b, size_t o)
{
    if (o >= b->n || b->n-o < 2) invalid("truncated word");
    return b->p[o] | ((unsigned)b->p[o+1]<<8);
}
static blob load(const char *path)
{
    blob b;
    FILE *f = fopen(path,"rb");
    if (!f) { perror(path); exit(2); }
    if (fseek(f,0,SEEK_END)) { perror(path); exit(2); }
    long len=ftell(f);
    if (len<0 || len>16*1024*1024) { fputs("RED: file size limit\n",stderr); exit(2); }
    rewind(f); b.n=(size_t)len; b.p=malloc(b.n+1);
    if (!b.p || fread(b.p,1,b.n,f)!=b.n || ferror(f)) { perror(path); exit(2); }
    b.p[b.n]=0;
    if (fclose(f)) { perror(path); exit(2); }
    return b;
}
static size_t asciiz(const blob *b, size_t o, size_t end)
{
    if (o>=end || end>b->n) invalid("missing ASCIIZ");
    const unsigned char *p=memchr(b->p+o,0,end-o);
    if (!p) invalid("unterminated ASCIIZ");
    return (size_t)(p-b->p)+1;
}
static void string_slot(const blob *b, size_t o, size_t len, size_t end)
{
    if (!len || o>=end || len>end-o || asciiz(b,o,o+len)!=o+len)
        invalid("length/string boundary mismatch");
}
static void frm(blob *b)
{
    if (b->n!=1990 || word(b,0)!=2 || word(b,0x7c4)!=2)
        invalid("FRM size/version/end marker");
    size_t heap=word(b,2), sum=0, used=0;
    if (!heap || heap>0x68c-0xe0) invalid("FRM heap extent");
    for (size_t i=0;i<55;i++) {
        unsigned len=word(b,4+2*i); sum+=len;
        if (len) {
            if (used!=i) invalid("FRM noncontiguous lengths");
            size_t off=i ? word(b,0x74+2*(i-1)) : 0;
            string_slot(b,0xe0+off,len,0xe0+heap); used++;
        }
    }
    if (heap!=sum || used<3 || (used-1)%2) invalid("FRM string accounting");
    size_t cols=word(b,0x7be), pairs=(used-1)/2;
    if (!cols || cols>pairs || pairs-cols>2 || cols>(0x7b6-0x68c)/12)
        invalid("FRM columns/groups");
    for (size_t i=0;i<cols;i++) {
        size_t o=0x68c+i*12;
        unsigned ex=word(b,o+8), heading=word(b,o+10);
        if (!word(b,o) || !ex || ex>=used || !heading || heading>=used)
            invalid("FRM column string index/width");
    }
    /* frm.md Sec 4/6: only the gap and unused descriptor area are volatile.
     * +4 in USED descriptors is uncertain: deliberately NOT normalized. */
    memset(b->p+0xe0+heap,0,0x68c-0xe0-heap);
    memset(b->p+0x68c+12*cols,0,0x7b6-0x68c-12*cols);
}
static void lbl(blob *b)
{
    if (b->n!=1034 || b->p[0]!=2 || b->p[1033]!=2) invalid("LBL framing");
    unsigned height=word(b,0x3d);
    if (!height || height>16 || !word(b,0x3f) || !word(b,0x47))
        invalid("LBL dimensions");
    /* lbl.md Sec 4: only first height slots consumed. Fixed-width active
     * slots and remarks (INCLUDING their spaces) stay byte-exact. */
    memset(b->p+0x49+60*height,0,60*(16-height));
}
static void qry(blob *b)
{
    if (b->n<2 || b->p[0]!=3 || b->p[b->n-1]!=0) invalid("QRY framing");
    size_t o=1;
    while (o<b->n-1) {
        size_t len=word(b,o+1);
        if (len<8 || len>b->n-1-o) invalid("QRY clause extent");
        size_t name=asciiz(b,o+3,o+len);
        if (name==o+4 || name+2>=o+len) invalid("QRY empty field/operator");
        (void)word(b,name);
        if (asciiz(b,name+2,o+len)!=o+len) invalid("QRY constant/record boundary");
        o+=len;
    }
    /* qry.md Sec 2.2: so-called pad bytes are HIGH bytes of u16 words.
     * Nothing is discarded, including unknown flags/operator encodings. */
}
static void stale_name(blob *b,size_t o)
{
    size_t end=asciiz(b,o,o+11);
    memset(b->p+end,0,o+11-end);
}
static void scr(blob *b)
{
    size_t o=0, fields=0; int separator=0;
    while (o<b->n && b->p[o]==',' && b->n-o>=78 &&
           !memcmp(b->p+o+74,"....",4)) {
        /* scr-and-fmt.md Sec 3.2/3.3: ASCIIZ aliases/names, stale name
         * tails, embedded descriptor address and two runtime pointers. */
        stale_name(b,o+1); stale_name(b,o+12);
        if (!b->p[o+23] || !strchr("CNDLM",b->p[o+23]) || !b->p[o+28] ||
            word(b,o+54)>24 || word(b,o+56)>79) invalid("SCR field/type/coordinate");
        memset(b->p+o+24,0,4); memset(b->p+o+44,0,8);
        fields++; o+=78;
        if (o<b->n && b->p[o]=='.') { o++; separator=1; break; }
    }
    if (!fields || !separator) invalid("SCR unsupported empty/unknown field section or missing separator");
    while (o<b->n && b->p[o]==',') {
        if (b->n-o<19 || word(b,o+1)>1 || word(b,o+3)>24 ||
            word(b,o+5)>79 || word(b,o+7)>24 || word(b,o+9)>79)
            invalid("SCR box extent/coordinate");
        /* Sec 3.4: eight-byte stale pointer/trailer, no durable meaning. */
        memset(b->p+o+11,0,8); o+=19;
    }
    if (b->n-o<5 || memcmp(b->p+o,".\r\n",3) || b->p[b->n-1]!=26 ||
        !memchr(b->p+o+3,26,b->n-o-4)) invalid("SCR image framing");
    /* All reserved fields, page/flags, image/masks and CRLFs stay exact. */
}
static void vue(blob *b)
{
    if (b->n<28 || b->p[0]!=3 || b->p[b->n-1]!=26) invalid("VUE framing");
    size_t count=word(b,4), o=26;
    if (!count || count>10) invalid("VUE work-area count");
    for (size_t i=0;i<count;i++) {
        o=asciiz(b,o,b->n-1);
        if (b->n-o<3) invalid("VUE work-area trailer");
        o+=2;
    }
    /* vue.md Sec 6-8: the proven single-format/single-relation dialect.
     * Refuse other shapes; do not infer missing-format/filter encodings. */
    o=asciiz(b,o,b->n-1);
    if (b->n-o<17) invalid("VUE format/relation extent");
    o+=4; string_slot(b,o,12,o+12); o+=12;
    if (asciiz(b,o,b->n-1)!=b->n-1) invalid("VUE field list extent");
}
static void text(blob *b, int format)
{
    if (format && (b->n<3 || memcmp(b->p+b->n-3,"\r\n\032",3)))
        invalid("FMT must end CR LF Ctrl-Z");
    for (size_t i=0;i<b->n;i++) {
        unsigned c=b->p[i];
        if (!c) invalid("NUL in text file (linked PRG unsupported)");
        if (c=='\r' && (i+1==b->n || b->p[i+1]!='\n')) invalid("bare CR");
        if (c=='\n' && (!i || b->p[i-1]!='\r')) invalid("bare LF");
    }
    /* No whitespace/case/line-ending/EOF normalization. OEM bytes retained. */
}
static void parse(blob *b)
{
    if (!strcmp(KIND,"frm")) frm(b);
    else if (!strcmp(KIND,"lbl")) lbl(b);
    else if (!strcmp(KIND,"qry")) qry(b);
    else if (!strcmp(KIND,"scr")) scr(b);
    else if (!strcmp(KIND,"vue")) vue(b);
    else if (!strcmp(KIND,"fmt")) text(b,1);
    else if (!strcmp(KIND,"prg") || !strcmp(KIND,"config")) text(b,0);
    else if (strcmp(KIND,"exact")) invalid("unknown comparator kind");
}
int main(int argc,char **argv)
{
    if (argc!=3) { fprintf(stderr,"usage: %s REFERENCE ACTUAL\n",argv[0]); return 2; }
    input=argv[1]; blob a=load(input); parse(&a);
    input=argv[2]; blob b=load(input); parse(&b);
    if (a.n!=b.n) { fprintf(stderr,"RED: %s_diff size %zu != %zu\n",KIND,a.n,b.n); return 1; }
    for (size_t i=0;i<a.n;i++) if (a.p[i]!=b.p[i]) {
        fprintf(stderr,"RED: %s_diff meaningful byte 0x%zx: %02x != %02x\n",KIND,i,a.p[i],b.p[i]);
        return 1;
    }
    free(a.p); free(b.p); printf("GREEN: %s_diff meaningful bytes equal\n",KIND); return 0;
}
