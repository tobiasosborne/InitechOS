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
/* Independent minimum contract: ADR-0009 DEC-03a.10 / initech-9nzpr.
 * Deliberately held in C, never read back from the inventory under audit.
 * Rows may add evidence; removing/replacing a required pair must fail. */
typedef struct {
    const char *id;
    const char *l1[6][2];
    const char *assertions[5]; /* Each requires mutants/<assertion>.sh too. */
    const char *screens[3][2];
} required;
#define DBF {".DBF header and records","harness/diff/dbf_diff/dbf_ref.py"}
#define DBT {".DBT memo effects","harness/diff/dbf_diff/dbf_ref.py"}
#define NDX {".NDX index effects","harness/diff/dbf_diff/ndx_ref.py"}
#define TRANSCRIPT {"command/startup transcript","build/exact_diff"}
#define W4 "harness/diff/dbf_diff/xbase_prog_diff/w4/"
static const required requirements[]={
    {"browse", {DBF, DBT, NDX},
     {"menu-toggle","cell-navigation","record-status","save"}, {{0}}},
    {"edit", {DBF, DBT},
     {"field-navigation","key-help","append-count","save-abandon"},
     {{"shared-W3-READ","harness/diff/dbf_diff/xbase_prog_diff/screen-mint/get/read-typed.screen"}}},
    {"structure", {DBF}, {"field-table","type-cycle","save-abandon"},
     {{"create-empty",W4 "create-retained/create-empty.screen"},
      {"modify-structure",W4 "structure-retained/modify-structure.screen"}}},
    {"command", {{".PRG and real-minted backup","build/prg_diff"}},
     {"text-edit","cursor","status","save-abandon"}, {{0}}},
    {"report", {{".FRM","build/frm_diff"},{".FRM unchanged re-save identity","build/exact_diff"}},
     {"sections","box-edit","status"}, {{0}}},
    {"label", {{".LBL","build/lbl_diff"},{".LBL unchanged re-save identity","build/exact_diff"}},
     {"dimensions","navigation","save-abandon"}, {{0}}},
    {"query-screen-view", {{".QRY","build/qry_diff"},{".SCR","build/scr_diff"},
      {".FMT","build/fmt_diff"},{".TXT screen documentation","build/exact_diff"},{".VUE","build/vue_diff"}},
     {"filter-rows","painter-cursor","draw-keys","view-workareas"},
     {{"generated-fmt",W4 "generated-form/generated-fmt.screen"}}},
    {"assist", {TRANSCRIPT, DBF, DBT, NDX, {"designer output files","build/exact_diff"}},
     {"pad-order","menu-navigation","issued-command"}, {{0}}},
    {"help", {{"read-only; no file changes","harness/diff/dbf_diff/w4_unchanged.sh"}},
     {"topic-index","f1-overlay","page-navigation","enter-prompt"},
     {{"help-main",W4 "retained/help-main.screen"},{"help-use",W4 "retained/help-use.screen"}}},
    {"config", {{"CONFIG.DB input text","build/config_diff"}, TRANSCRIPT},
     {"startup-settings","startup-memory"}, {{0}}}
};
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
static void require_pair(int array,const char *key,const char *val,
                         const char *key2,const char *val2,const char *reason)
{
    for (int e=ns[array].first;e;e=ns[e].next)
        if (equal(get(e,key),val) && equal(get(e,key2),val2)) return;
    fprintf(stderr,"missing requirement: %s / %s\n",val,val2); die(reason);
}
static void require_assertion(int array,const char *assertion)
{
    for (int e=ns[array].first;e;e=ns[e].next) if (equal(e,assertion)) return;
    fprintf(stderr,"missing requirement: %s\n",assertion);
    die("required assertion/mutant pair dropped");
}
/* Test mutations remove complete JSON members, preserving valid JSON. */
static void cut_member(int n,size_t *a,size_t *b)
{
    *a=ns[n].a-(ns[n].type=='s');
    *b=ns[n].b+(ns[n].type=='s');
    while (*b<len && isspace((unsigned char)data[*b])) (*b)++;
    if (*b<len && data[*b]==',') (*b)++;
    else {
        while (*a && isspace((unsigned char)data[*a-1])) (*a)--;
        if (*a && data[*a-1]==',') (*a)--;
    }
}
int main(int argc,char **argv)
{
    if (argc<2 || argc>4) { fputs("usage: w4_inventory JSON [--ready|--retained APP]\n",stderr); return 2; }
    int mutation=argc==3 && (!strcmp(argv[1],"--remove-first") || !strcmp(argv[1],"--remove-mutant") ||
        !strcmp(argv[1],"--remove-l1") || !strcmp(argv[1],"--remove-pair") || !strcmp(argv[1],"--remove-retained"));
    FILE *f=fopen(argv[mutation?2:1],"rb");
    if (!f) { fprintf(stderr,"RED: w4 inventory: missing JSON %s\n",argv[1]); return 1; }
    len=fread(data,1,sizeof data-1,f);
    if (ferror(f) || !feof(f)) die("JSON read/size");
    fclose(f);
    int top=parse(0); ws(); if (pos!=len) die("trailing JSON");
    if (mutation) {
        int apps=get(top,"apps"), first=ns[apps].first;
        size_t a,b,c=0,d=0;
        if (!strcmp(argv[1],"--remove-first")) {
            cut_member(first,&a,&b);
        } else if (!strcmp(argv[1],"--remove-l1")) {
            cut_member(ns[ns[get(first,"l1")].first].next,&a,&b);
        } else if (!strcmp(argv[1],"--remove-retained")) {
            int structure=ns[ns[first].next].next;
            cut_member(ns[get(structure,"retained_exact_screens")].first,&a,&b);
        } else {
            int mutants=get(get(first,"l2"),"mutants");
            cut_member(ns[mutants].first,&a,&b);
            if (!strcmp(argv[1],"--remove-pair")) {
                c=a; d=b;
                cut_member(ns[get(get(first,"l2"),"assertions")].first,&a,&b);
            }
        }
        fwrite(data,1,a,stdout);
        if (d) { fwrite(data+b,1,c-b,stdout); b=d; }
        fwrite(data+b,1,len-b,stdout); return ferror(stdout)?2:0;
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
        for (int i=0;i<10;i++) if (equal(id,requirements[i].id)) ix=i;
        if (ix<0 || seen[ix]++) die("missing/duplicate/unknown W4 group");
        const required *req=&requirements[ix];
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
        for (int i=0;req->l1[i][0];i++)
            require_pair(l1,"kind",req->l1[i][0],"tool",req->l1[i][1],"required L1 kind/tool dropped");
        int l2=get(a,"l2"); char expected[512],s[512];
        const char *keys[]={"keys","check"};
        for (int i=0;i<2;i++) {
            snprintf(expected,sizeof expected,"harness/diff/dbf_diff/xbase_prog_diff/w4/%s/%s",req->id,i?"CHECK":"KEYS");
            value(get(l2,keys[i]),s,sizeof s); if (strcmp(s,expected)) die("L2 trace convention drift");
        }
        nonempty(get(l2,"scope"));
        int asserts=get(l2,"assertions"), mutants=get(l2,"mutants");
        strings(asserts); strings(mutants);
        if (!ns[asserts].count || ns[asserts].count!=ns[mutants].count) die("missing/orphan structural mutant");
        int m=ns[mutants].first;
        for (int e=ns[asserts].first;e;e=ns[e].next,m=ns[m].next) {
            char assertion[128]; value(e,assertion,sizeof assertion);
            snprintf(expected,sizeof expected,"harness/diff/dbf_diff/xbase_prog_diff/w4/%s/mutants/%s.sh",req->id,assertion);
            value(m,s,sizeof s);
            if (strcmp(expected,s)) die("missing/orphan structural mutant");
        }
        for (int i=0;req->assertions[i];i++) require_assertion(asserts,req->assertions[i]);
        int l3=get(a,"l3"); value(get(l3,"script"),s,sizeof s);
        if (strcmp(s,req->id)) die("L3 script convention drift");
        nonempty(get(l3,"clip")); nonempty(get(l3,"repro_gate"));
        int retained=get(a,"retained_exact_screens");
        if (ns[retained].type!='[') die("retained screens array required");
        for (int e=ns[retained].first;e;e=ns[e].next) {
            nonempty(get(e,"name")); path(get(e,"dump"),1); nonempty(get(e,"provenance"));
        }
        for (int i=0;req->screens[i][0];i++)
            require_pair(retained,"name",req->screens[i][0],"dump",req->screens[i][1],"required retained screen dropped");
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
    puts("GREEN: w4 inventory: ten unique groups, required L1/pairs/retained minima, W3 exact"); return 0;
}
