/* Locked inventory validator, factory C. initech-9nzpr / DEC-03a.10.
 * Independent ten-group list is the ADR table, not the JSON being tested.
 * Intentionally small ASCII JSON reader; rejects duplicate keys, malformed
 * numbers, trailing data, unsafe paths, missing fields and orphan mutants.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char data[131072];
static size_t pos, len;
typedef struct { size_t a,b; int type,first,next,count; } node;
static node ns[8192];
static int nn;
static const char *ids[]={"browse","edit","structure","command","report","label",
                          "query-screen-view","assist","help","config"};
static void die(const char *s) { fprintf(stderr,"RED: w4 inventory: %s\n",s); exit(1); }
static void ws(void)
{
    while (pos<len && (data[pos]==' ' || data[pos]=='\t' || data[pos]=='\r' || data[pos]=='\n')) pos++;
}
static int equal(int n,const char *s)
{ return n && ns[n].type=='s' && ns[n].b-ns[n].a==strlen(s) && !memcmp(data+ns[n].a,s,strlen(s)); }
static void value(int n,char *out,size_t cap)
{
    if (!n || ns[n].type!='s' || ns[n].b-ns[n].a>=cap) die("expected bounded string");
    size_t k=ns[n].b-ns[n].a; memcpy(out,data+ns[n].a,k); out[k]=0;
    /* Inventory paths, verbs and provenance use literal ASCII, no escapes. */
    if (strchr(out,'\\')) die("inventory strings must use literal ASCII");
}
static int parse(unsigned depth)
{
    if (depth>40 || ++nn>=8192) die("JSON depth/token limit");
    ws(); int n=nn; ns[n].a=pos;
    if (pos==len) die("unexpected JSON EOF");
    char c=data[pos++];
    if (c=='"') {
        ns[n].type='s'; ns[n].a=pos;
        while (pos<len && data[pos]!='"') {
            unsigned ch=(unsigned char)data[pos++];
            if (ch<32 || ch>=127) die("JSON must be ASCII");
            if (ch=='\\') {
                if (pos==len) die("bad JSON escape");
                ch=(unsigned char)data[pos++];
                if (ch=='u') { for (int i=0;i<4;i++) if (pos==len || !isxdigit((unsigned char)data[pos++])) die("bad unicode escape"); }
                else if (!strchr("\"\\/bfnrt",ch)) die("bad JSON escape");
            }
        }
        if (pos==len) die("unterminated JSON string");
        ns[n].b=pos++; return n;
    }
    ns[n].type=c;
    if (c=='{' || c=='[') {
        int last=0; char end=c=='{'?'}':']'; ws();
        if (pos<len && data[pos]==end) { pos++; ns[n].b=pos; return n; }
        for (;;) {
            int key=0;
            if (c=='{') {
                ws(); if (pos==len || data[pos]!='"') die("expected object key");
                key=parse(depth+1);
                for (int k=ns[n].first;k;k=ns[ns[k].next].next)
                    if (ns[k].b-ns[k].a==ns[key].b-ns[key].a &&
                        !memcmp(data+ns[k].a,data+ns[key].a,ns[k].b-ns[k].a)) die("duplicate JSON key");
                if (last) ns[last].next=key; else ns[n].first=key;
                ws(); if (pos==len || data[pos++]!=':') die("expected colon");
            }
            int child=parse(depth+1);
            if (key) ns[key].next=child;
            else if (last) ns[last].next=child; else ns[n].first=child;
            last=child; ns[n].count++;
            ws(); if (pos==len) die("unterminated container");
            c=data[pos++];
            if (c==end) break;
            if (c!=',') die("expected comma");
            c=ns[n].type;
        }
        ns[n].b=pos; return n;
    }
    pos--;
    if (data[pos]=='-' || isdigit((unsigned char)data[pos])) {
        if (data[pos]=='-') pos++;
        if (pos==len || !isdigit((unsigned char)data[pos])) die("bad number");
        if (data[pos]=='0') pos++; else while (pos<len && isdigit((unsigned char)data[pos])) pos++;
        if (pos<len && data[pos]=='.') {
            pos++; if (pos==len || !isdigit((unsigned char)data[pos])) die("bad fraction");
            while (pos<len && isdigit((unsigned char)data[pos])) pos++;
        }
        if (pos<len && (data[pos]=='e'||data[pos]=='E')) {
            pos++; if (pos<len && (data[pos]=='+'||data[pos]=='-')) pos++;
            if (pos==len || !isdigit((unsigned char)data[pos])) die("bad exponent");
            while (pos<len && isdigit((unsigned char)data[pos])) pos++;
        }
    } else {
        const char *p=NULL;
        if (len-pos>=4 && !memcmp(data+pos,"true",4)) p="true";
        if (len-pos>=5 && !memcmp(data+pos,"false",5)) p="false";
        if (len-pos>=4 && !memcmp(data+pos,"null",4)) p="null";
        if (!p) die("invalid JSON value");
        pos+=strlen(p);
    }
    ns[n].b=pos; return n;
}
static int get(int obj,const char *key)
{
    if (!obj || ns[obj].type!='{') die("expected object");
    for (int k=ns[obj].first;k;k=ns[ns[k].next].next)
        if (equal(k,key)) return ns[k].next;
    fprintf(stderr,"missing field: %s\n",key); die("required field absent"); return 0;
}
static void nonempty(int n)
{
    if (!n || (ns[n].type=='s' ? ns[n].b==ns[n].a : !ns[n].count)) die("empty required evidence");
}
static void strings(int n)
{
    if (ns[n].type!='[') die("expected string array");
    for (int k=ns[n].first;k;k=ns[k].next) {
        if (ns[k].type!='s') die("expected string array member");
        nonempty(k);
        for (int j=ns[k].next;j;j=ns[j].next)
            if (ns[k].b-ns[k].a==ns[j].b-ns[j].a && !memcmp(data+ns[k].a,data+ns[j].a,ns[k].b-ns[k].a)) die("duplicate array member");
    }
}
static void path(int n,int exists)
{
    char p[1024]; value(n,p,sizeof p);
    if (!*p || p[0]=='/' || strstr(p,"..") || strchr(p,' ') || strchr(p,'\n')) die("unsafe relative path");
    struct stat st;
    if (exists && (stat(p,&st) || !S_ISREG(st.st_mode) || !st.st_size)) {
        fprintf(stderr,"missing evidence/tool: %s\n",p); die("file absent/empty");
    }
}
int main(int argc,char **argv)
{
    if (argc<2 || argc>4) { fputs("usage: w4_inventory JSON [--ready|--retained APP]\n",stderr); return 2; }
    int mutation=argc==3 && (!strcmp(argv[1],"--remove-first") || !strcmp(argv[1],"--remove-mutant"));
    FILE *f=fopen(argv[mutation?2:1],"rb");
    if (!f) { fprintf(stderr,"RED: w4 inventory: missing JSON %s\n",argv[1]); return 1; }
    len=fread(data,1,sizeof data-1,f);
    if (ferror(f) || !feof(f)) die("JSON read/size");
    fclose(f);
    int top=parse(0); ws(); if (pos!=len) die("trailing JSON");
    if (mutation) {
        int apps=get(top,"apps"), first=ns[apps].first;
        size_t a=ns[first].a,b=ns[first].b;
        if (!strcmp(argv[1],"--remove-first")) {
            while (b<len && isspace((unsigned char)data[b])) b++;
            if (b==len || data[b]!=',') die("cannot remove first row");
            b++;
        } else {
            int mutants=get(get(first,"l2"),"mutants");
            a=ns[ns[mutants].first].a-1; b=ns[ns[mutants].first].b+1;
            while (b<len && isspace((unsigned char)data[b])) b++;
            if (data[b]==',') b++;
        }
        fwrite(data,1,a,stdout); fwrite(data+b,1,len-b,stdout); return ferror(stdout)?2:0;
    }
    if (!equal(get(top,"status"),"locked")) die("inventory not locked");
    char provenance[2048]; value(get(top,"_comment"),provenance,sizeof provenance);
    if (!strstr(provenance,"DEC-03a.10") || !strstr(provenance,"initech-9nzpr")) die("missing provenance");
    int w3=get(top,"w3");
    if (!equal(get(w3,"grading"),"byte-exact-screens")) die("W3 exact grading lost");
    strings(get(w3,"apps")); nonempty(get(w3,"apps")); path(get(w3,"gate_source"),1);
    int apps=get(top,"apps"), seen[10]={0}, selected=0;
    if (ns[apps].type!='[' || ns[apps].count!=10) die("expected ten W4 app groups");
    for (int a=ns[apps].first;a;a=ns[a].next) {
        int id=get(a,"id"), ix=-1;
        for (int i=0;i<10;i++) if (equal(id,ids[i])) ix=i;
        if (ix<0 || seen[ix]++) die("missing/duplicate/unknown W4 group");
        if (argc==4 && equal(id,argv[3])) selected=a;
        nonempty(get(a,"name")); strings(get(a,"dispatch_verbs")); nonempty(get(a,"dispatch_verbs"));
        if (!equal(get(a,"status"),"not-started") && !equal(get(a,"status"),"complete")) die("bad app status");
        int l1=get(a,"l1"); if (ns[l1].type!='[') die("L1 array required"); nonempty(l1);
        for (int e=ns[l1].first;e;e=ns[e].next) {
            nonempty(get(e,"kind")); nonempty(get(e,"format_spec"));
            char s[1024]; value(get(e,"format_spec"),s,sizeof s);
            if (strncmp(s,"specs/",6) || strstr(s,"..")) die("expected sister specs path");
            path(get(e,"tool"),1); nonempty(get(e,"normalization")); nonempty(get(e,"golden"));
        }
        int l2=get(a,"l2"); char expected[512],s[512];
        const char *keys[]={"keys","check"};
        for (int i=0;i<2;i++) {
            snprintf(expected,sizeof expected,"harness/diff/dbf_diff/xbase_prog_diff/w4/%s/%s",ids[ix],i?"CHECK":"KEYS");
            value(get(l2,keys[i]),s,sizeof s); if (strcmp(s,expected)) die("L2 trace convention drift");
        }
        nonempty(get(l2,"scope"));
        int asserts=get(l2,"assertions"), mutants=get(l2,"mutants");
        strings(asserts); strings(mutants);
        if (!ns[asserts].count || ns[asserts].count!=ns[mutants].count) die("missing/orphan structural mutant");
        int m=ns[mutants].first;
        for (int e=ns[asserts].first;e;e=ns[e].next,m=ns[m].next) {
            char assertion[128]; value(e,assertion,sizeof assertion);
            snprintf(expected,sizeof expected,"harness/diff/dbf_diff/xbase_prog_diff/w4/%s/mutants/%s.sh",ids[ix],assertion);
            value(m,s,sizeof s);
            if (strcmp(expected,s)) die("missing/orphan structural mutant");
        }
        int l3=get(a,"l3"); value(get(l3,"script"),s,sizeof s);
        if (strcmp(s,ids[ix])) die("L3 script convention drift");
        nonempty(get(l3,"clip")); nonempty(get(l3,"repro_gate"));
        int retained=get(a,"retained_exact_screens");
        if (ns[retained].type!='[') die("retained screens array required");
        for (int e=ns[retained].first;e;e=ns[e].next) {
            nonempty(get(e,"name")); path(get(e,"dump"),1); nonempty(get(e,"provenance"));
        }
        if ((ix==2 || ix==6 || ix==8) && !ns[retained].count) die("required cheap exact screens dropped");
    }
    if (argc!=2) {
        if (argc!=4 || !selected) die("unknown app/argument");
        if (!strcmp(argv[2],"--mutants")) {
            int r=get(get(selected,"l2"),"mutants");
            for (int e=ns[r].first;e;e=ns[e].next) { char s[1024]; value(e,s,sizeof s); puts(s); }
            return 0;
        }
        if (!strcmp(argv[2],"--retained")) {
            int r=get(selected,"retained_exact_screens");
            for (int e=ns[r].first;e;e=ns[e].next) { char s[1024]; value(get(e,"dump"),s,sizeof s); puts(s); }
            return 0;
        }
        if (strcmp(argv[2],"--ready")) die("unknown mode");
        int l2=get(selected,"l2"); path(get(l2,"keys"),1); path(get(l2,"check"),1);
        int m=get(l2,"mutants"); for (int e=ns[m].first;e;e=ns[e].next) path(e,1);
    }
    puts("GREEN: w4 inventory: ten unique groups, L1/L2/L3, retained dumps, W3 exact"); return 0;
}
