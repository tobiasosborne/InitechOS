/* Source grammar for the InitechBase merge gate (initech-j29).
 * Ref: PUB-REF-0001 GC-01/03; initech-xbase.sty and initech-syntax.sty at
 * 31b286e4. This is a restricted language, NOT a TeX interpreter. Unknown
 * tokens, expansion contexts, support revisions and paths are RED. The
 * independent real typesetting/commands/replay session gates remain final.
 */
#include <stdint.h>
#include <limits.h>
typedef struct {
    char kind[16],state[8],dir[PATH_MAX],root[PATH_MAX],gen[PATH_MAX];
    char env[64][64];size_t nenv,nex,ntrans;int phase,files,depth,generated;
    unsigned definitions;int class_seen;
    Set *doc,*draft;const char *book;
} Src;
static uint64_t srchash(const char *p,size_t n)
{
    uint64_t h=UINT64_C(14695981039346656037);
    while(n--) h=(h^(unsigned char)*p++)*UINT64_C(1099511628211);
    return h;
}
/* These are syntax-version guards, not expected command inventories.
 * Review any support delta against the grammar and run the real session
 * oracle before updating a guard. FNV-1a/64 over the complete file bytes.
 * An added package cannot be loaded by the pinned class or source grammar.
 */
static const struct {const char *path;uint64_t hash;} srcsupport[]={
    {"class/initech-borland.sty", UINT64_C(0x52071d0d39cf7634)},
    {"class/initech-forms.sty", UINT64_C(0x86bc26eda6c9930e)},
    {"class/initech-manual.cls", UINT64_C(0x16e152aa10bd3351)},
    {"class/initech-manual.ist", UINT64_C(0x414221e48ad8783a)},
    {"class/initech-ops.lua", UINT64_C(0x455e785827bd4f01)},
    {"class/initech-ops.sty", UINT64_C(0xe9cfd671a864ef47)},
    {"class/initech-screen.sty", UINT64_C(0x0b51bc9f8e2d0b97)},
    {"class/initech-syntax.sty", UINT64_C(0x774d35058c6171ef)},
    {"class/initech-wpc.sty", UINT64_C(0x74e0925815254c9b)},
    {"class/initech-xbase.sty", UINT64_C(0xe4c18d096db1b955)},
    {"common/colophon.tex", UINT64_C(0x75840a9653b5df66)},
    {"common/initech-logo.tex", UINT64_C(0xf516b10e6b9980db)}
};
static const char *skipws(const char *p)
{ while(*p==' ' || *p=='\n' || *p=='\t' || *p=='\r') p++;return p; }
static const char *srccomment(const char *p)
{ while(*p && *p!='\n') p++;return *p?p+1:p; }
static void srcpath(char *out,size_t cap,const char *base,const char *name,const char *tag)
{ if(snprintf(out,cap,"%s/%s",base,name)>=(int)cap) die(tag,base,"path too long"); }
static void srcregular(const char *path,const char *tag)
{
    struct stat st;char real[PATH_MAX];
    if(lstat(path,&st) || !S_ISREG(st.st_mode) || !realpath(path,real) || strcmp(real,path))
        die(tag,path,"expected a resolved regular file (no symlinks)");
}
static void srcsupportcheck(const char *root)
{
    for(size_t i=0;i<sizeof srcsupport/sizeof *srcsupport;i++) {
        char path[PATH_MAX],*t;srcpath(path,sizeof path,root,srcsupport[i].path,"SRC-SUPPORT");
        srcregular(path,"SRC-SUPPORT");t=readfile(path);
        if(srchash(t,strlen(t))!=srcsupport[i].hash)
            die("SRC-SUPPORT",path,"support changed: review grammar and run the real session oracle");
        free(t);
    }
}
/* A bounded argument span. Escapes and comments do not balance braces.
 * Identifiers below deliberately reject comments and macro expansion. */
static const char *srcspan(const char *p,char open,const char **a,const char **b,const char *where)
{
    int depth=0;p=skipws(p);
    if(*p!=open) {
        char detail[100];snprintf(detail,sizeof detail,"expected %c argument near %.60s",open,p);
        die("SRC-SYNTAX",where,detail);
    }
    *a=++p;
    for(;*p;p++) {
        if(*p=='\\' && p[1]) {p++;continue;}
        if(*p=='%') {p=srccomment(p);if(!*p) break;p--;continue;}
        if(*p=='{') depth++;
        else if(*p=='}') {
            if(open=='{' && !depth) {*b=p;return p+1;}
            if(--depth<0) die("SRC-SYNTAX",where,"unbalanced argument group");
        } else if(*p==']' && open=='[' && !depth) {*b=p;return p+1;}
    }
    die("SRC-SYNTAX",where,"unbalanced argument group");return p;
}
static const char *srcarg(const char *p,char *out,size_t cap,const char *where)
{
    const char *a,*b;p=srcspan(p,'{',&a,&b,where);
    if((size_t)(b-a)>=cap || memchr(a,'%',(size_t)(b-a)))
        die("SRC-SYNTAX",where,"argument too long or commented identifier");
    memcpy(out,a,(size_t)(b-a));out[b-a]=0;return p;
}
static void srcscan(Src *,const char *,const char *,int,const char *);
static void srcwalk(Src *,const char *,const char *);
/* Only the following macros are known to have no registration/input side
 * effect. m/o are recursively checked passive arguments, v a VERBATIM
 * argument of the class (not a potentially executable TeX argument).
 * Consuming complete arguments prevents treating stored/quoted entries as
 * executed ones. No unknown macro is skipped.
 */
static const struct {const char *name,*args;} srcpassive[]={
    {"ProductName","m"},{"DocTitle","m"},{"PartNumber","m"},{"PrintCode","m"},
    {"FamilyName","m"},{"FamilyColor","m"},{"CopyrightYears","m"},{"Edition","m"},
    {"EditionApplies","m"},{"Revision","m"},{"Trademarks","m"},
    {"XbaseIndexMentions",""},{"MasterIndexPartner","m"},{"FrontMatterFolios",""},
    {"MakeTitlePage",""},{"tableofcontents",""},{"BeginSection","mm"},
    {"SectionNotes","m"},{"appendix",""},{"EndSections",""},{"IndexKey","m"},
    {"IndexKeyLine","mm"},{"printindex",""},{"FinishBook","o"},{"QRCover",""},
    {"QRFinish","o"},{"QRSection","m"},{"QRMessage","mm"},
    {"chapter","m"},{"section","m"},{"FrontChapter","m"},{"SectionIntroduction","m"},
    {"SectionGlossary","m"},{"Gates","m"},{"NoDictionaryHeads",""},
    {"SideHead","m"},{"ATEHead","m"},{"ATEResponse","m"},{"GlossaryEntry","mm"},
    {"Syntax",""},{"Defaults",""},{"Usage",""},{"Options",""},{"Tips",""},
    {"Examples",""},{"SeeAlso","m"},{"SyntaxLine","m"},{"ProgramListing","m"},
    {"cmd","m"},{"fn","m"},{"file","m"},{"doctitle","m"},{"type","m"},
    {"key","m"},{"kw","m"},{"ph","m"},{"opt","m"},{"xp","m"},
    {"term","om"},{"xmsg","mm"},{"draft","m"},{"texttt","m"},{"textsf","m"},
    {"textbf","m"},{"textit","m"},{"textasciicircum",""},{"textbackslash",""},
    {"bs",""},{"tm",""},{"reg",""},{"rep",""},{"ldots",""},
    {"par",""},{"medskip",""},{"noindent",""},{"small",""},{"footnotesize",""},
    {"sffamily",""},{"bfseries",""},{"imLegalFont",""},{"fontsize","mm"},
    {"selectfont",""},{"hspace","sm"},{"vspace","sm"},{"rule","omm"},
    {"imHang",""},{"linewidth",""},{"quad",""},{"qquad",""},{"hline",""},
    {"item","o"},{"allowbreak",""},{"index","m"},{"begingroup",""},{"endgroup",""},
    {"DefineCoercionRule","mm"},{"CoercionRule","m"},{"FormatTable","m"},
    {"FormatRow","mmmm"},{"FormatDBFRecordBytes",""},{"FormatDBFTerminator",""},
    {"FormatDBFLive",""},{"FormatDBFDeleted",""},{"FormatDBTBlock",""},
    {"FormatDBTTerm",""},{"FormatNDXPage",""},{"Screen","mm"},
    {"VerbatimInput","om"},{"IsThisGoodForTheCompany",""},{"InitechAddressLine",""}
};
static const char *srcargs(Src *s,const char *p,const char *sig,const char *where)
{
    for(;*sig;sig++) {
        const char *a,*b;
        if(*sig=='s') {if(*p=='*') p++;continue;}
        if(*sig=='o' && *skipws(p)!='[') continue;
        if(*sig=='v') {
            /* The messages use xparse's verbatim argument, not executable
             * tokens. Support its present plain, single braced form only. */
            p=skipws(p);
            if(*p++!='{') die("SRC-SYNTAX",where,"expected verbatim argument");
            while(*p && *p!='}') {
                if(strchr("{\\%\n\r",*p)) die("SRC-SYNTAX",where,"unsupported verbatim argument");
                p++;
            }
            if(*p++!='}') die("SRC-SYNTAX",where,"unclosed verbatim argument");
            continue;
        }
        p=srcspan(p,*sig=='o'?'[':'{',&a,&b,where);
        srcscan(s,a,b,1,where);
    }
    return p;
}
static void srceffects(Src *s,int passive,const char *where)
{
    if(passive || s->phase!=1 || s->generated)
        die("SRC-CONTEXT",where,"entry/input outside an executable book body");
}
static void srcreg(Src *s,const char *name,const char *kind,const char *state)
{
    char key[MAX_NAME];
    if(!*name || strpbrk(name,"\\{}| \t\r\n")) die("SRC-NAME",s->book,name);
    if(!commandkind(kind)) die("SRC-KIND",s->book,kind);
    if(snprintf(key,sizeof key,"%s|%s",name,kind)>=(int)sizeof key) die("SRC-NAME",s->book,name);
    if(has(s->doc,key)) die("DOCUMENTED-TWICE",s->book,key);
    addkind(s->doc,name,kind);
    if(!strcmp(state,"DRAFT")) addkind(s->draft,name,kind);
}
static void srctitle(const char *title,const char *where)
{
    /* Compound titles become ENTRY, the fourth index field. Accept only
     * literal command words or the one audited escaped ampersand. Stored
     * macros, empty titles and delimiters cannot yield a provable field. */
    if(!strcmp(title,"\\&")) return;
    if(!*skipws(title) || strlen(title)>=MAX_NAME || strspn(title,"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ")!=strlen(title))
        die("SRC-NAME",where,"compound title must be literal command words or escaped ampersand");
}
static int srcstem(const char *p)
{
    if(!*p) return 0;
    for(;*p;p++) if(!isalnum((unsigned char)*p) && *p!='-' && *p!='_') return 0;
    return 1;
}
static void srcexample(Src *s,const char *stem,int transcript,const char *where)
{
    char path[PATH_MAX],f[PATH_MAX];const char *name;
    const char *tag=transcript?"TRANSCRIPT-PAIR":"EXAMPLE-PAIR";
    if(transcript && !strncmp(stem,"../../examples/samir/",21)) {
        name=stem+21;
        if(!srcstem(name)) die(tag,where,"unsupported program transcript path");
        srcpath(path,sizeof path,s->root,stem+6,tag);
    } else if(!transcript && !strncmp(stem,"examples/",9) && srcstem(stem+9))
        srcpath(path,sizeof path,s->dir,stem,tag);
    else {die(tag,where,"unsupported example path");return;}
    if(snprintf(f,sizeof f,"%s.txt",path)>=(int)sizeof f) die(tag,where,"long example path");
    srcregular(f,tag);char *out=readfile(f);
    if(!transcript) {
        struct stat st;
        if(stat(f,&st) || strlen(out)!=(size_t)st.st_size) die("EXAMPLE-BYTES",f,"embedded NUL in transcript");
        for(const char *p=out;*p;p++) if((unsigned char)*p>126 || ((unsigned char)*p<32 && *p!='\n'))
            die("EXAMPLE-BYTES",f,"non-printable byte (the class's esc check)");
    }
    if(snprintf(f,sizeof f,"%s.%s",path,transcript?"prg":"dot")>=(int)sizeof f)
        die(tag,where,"long example path");
    srcregular(f,tag);
    if(transcript) {s->ntrans++;free(out);return;}
    /* Exact initech_xbase.example typed-line matching: suffix in order,
     * preceded by start-of-line or space/colon/question-mark/right-paren.
     * Keep CR bytes and empty lines, as Lua lines_of does. No replay or
     * expected transcript is regenerated by this source check.
     */
    char *typed=readfile(f),*t=typed,*line=out;struct stat st;
    if(stat(f,&st) || strlen(typed)!=(size_t)st.st_size)
        die("EXAMPLE-TYPED",f,"embedded NUL in typed input");
    while(*t && *line) {
        char *te=strchr(t,'\n'),*le=strchr(line,'\n');
        size_t nt=te?(size_t)(te-t):strlen(t),nl=le?(size_t)(le-line):strlen(line);
        if((!nt && !nl) || (nt && nl>=nt && !memcmp(line+nl-nt,t,nt) &&
           (nl==nt || strchr(" :?)",line[nl-nt-1])))) t=te?te+1:t+nt;
        line=le?le+1:line+nl;
    }
    if(*t) die("EXAMPLE-TYPED",f,"transcript does not show every typed line in order");
    free(typed);free(out);s->nex++;
}
/* Existing preamble definitions are accepted BYTE FOR BYTE, before the
 * document only. They cannot be edited into wrappers around registrations.
 * In particular, there is no generic newcommand/def-body interpreter.
 */
static const char *srcdefinition(Src *s,const char *p,const char *where)
{
    static const char *known[]={
        "\\providecommand{\\IsThisGoodForTheCompany}{\\par\\medskip\\noindent{\\sffamily Is This Good for the \\textbf{\\textit{COMPANY}}?}\\par\\noindent\\rule{\\linewidth}{0.5pt}\\medskip}",
        "\\NewDocumentCommand\\term{om}{\\textbf{#2}\\IfValueTF{#1}{\\index{#1}}{\\index{#2}}}",
        "\\renewcommand\\type[1]{\\texttt{\\bfseries #1}}",
        "\\newcommand\\bs{\\textbackslash}","\\newcommand\\ColophonInitechBase{}"
    };
    if(!s->phase && s->files==1 && s->depth==1) for(size_t i=0;i<sizeof known/sizeof *known;i++)
        if(!strncmp(p,known[i],strlen(known[i]))) {
            if(s->definitions & (1u<<i)) die("SRC-DEFINITION",where,"duplicate preamble definition");
            s->definitions|=1u<<i;return p+strlen(known[i]);
        }
    die("SRC-DEFINITION",where,"only the audited preamble definitions are supported");return p;
}
static void srcscan(Src *s,const char *p,const char *end,int passive,const char *where)
{
    if(++s->depth>64) die("SRC-SYNTAX",where,"source nesting too deep");
    while(p<end) {
        char word[128],a[512],b[512];const char *start=p,*x,*y;size_t n;
        if(*p=='%') {p=srccomment(p);continue;}
        if(isspace((unsigned char)*p)) {p++;continue;}
        if(s->phase==2) die("SRC-AFTER-END",where,"non-comment text after end{document}");
        if(*p=='{' ) {p=srcspan(p,'{',&x,&y,where);srcscan(s,x,y,passive,where);continue;}
        if(*p=='}' || (*p=='^' && p+1<end && p[1]=='^'))
            die("SRC-SYNTAX",where,"unexpected brace or TeX character-code escape");
        if(*p!='\\') {p++;continue;}
        p++;n=0;
        while(p<end && isalpha((unsigned char)*p)) {
            if(n+1>=sizeof word) die("SRC-UNKNOWN",where,"long control word");
            word[n++]=*p++;
        }
        word[n]=0;
        if(!n) {
            if(p<end && *p=='^' && p+1<end && p[1]=='^')
                die("SRC-SYNTAX",where,"TeX character-code escape after backslash");
            if(p<end && *p=='^') {p=srcargs(s,p+1,"m",where);continue;}
            if(p==end || !strchr("\\%&#_${} ,;:!-",*p)) die("SRC-UNKNOWN",where,"control symbol");
            if(*p++=='\\') p=srcargs(s,p,"o",where);
            continue;
        }
        if(!strncmp(word,"if",2) || !strcmp(word,"else") || !strcmp(word,"fi") || !strcmp(word,"unless"))
            die("SRC-CONDITIONAL",where,word);
        if(!strcmp(word,"include") || !strcmp(word,"RegisterCommand") || !strcmp(word,"csname"))
            die("SRC-UNSUPPORTED",where,word);
        if(!strcmp(word,"def") && s->generated) {
            /* samir_formats has only these numeric/display constants. */
            static const char *names[]={"FormatDBFRecordBytes","FormatDBFTerminator","FormatDBFLive", "FormatDBFDeleted","FormatNDXPage","FormatDBTBlock","FormatDBTTerm"};
            int ok=0;p=skipws(p);if(*p++!='\\') die("SRC-DEFINITION",where,"generated constant");
            n=0;while(isalpha((unsigned char)*p) && n+1<sizeof word) word[n++]=*p++;word[n]=0;
            for(size_t i=0;i<sizeof names/sizeof *names;i++) if(!strcmp(word,names[i])) ok=1;
            p=srcarg(p,a,sizeof a,where);
            if(!ok || !*a || strspn(a,"0123456789ABCDEFH ")!=strlen(a)) die("SRC-DEFINITION",where,"generated constant");
        } else if(!strcmp(word,"newcommand") || !strcmp(word,"renewcommand") || !strcmp(word,"providecommand") ||
                  !strcmp(word,"NewDocumentCommand") || !strcmp(word,"def")) {
            if(passive) die("SRC-DEFINITION",where,"definition inside argument");
            p=srcdefinition(s,start,where);
        } else if(!strcmp(word,"directlua")) {
            p=srcspan(p,'{',&x,&y,where);
            if(passive || s->generated || !strstr(where,"/ud-ascii.tex") ||
               srchash(start,(size_t)(p-start))!=UINT64_C(0xe709a4bb5ad93948))
                die("SRC-UNSUPPORTED",where,"only the audited ASCII table Lua is supported");
        } else if(!strcmp(word,"setlength")) {
            const char *known="\\setlength\\tabcolsep{4pt}";
            if(strncmp(start,known,strlen(known))) die("SRC-UNSUPPORTED",where,word);
            p=start+strlen(known);
        } else if(!strcmp(word,"documentclass")) {
            if(passive || s->phase || s->files!=1 || s->depth!=1 || s->class_seen++ || *p!='[')
                die("SRC-UNSUPPORTED",where,word);
            p=srcspan(p,'[',&x,&y,where);
            if(((size_t)(y-x)!=17 || strncmp(x,"ashtontate,binder",17)) &&
               ((size_t)(y-x)!=19 || strncmp(x,"ashtontate,quickref",19)))
                die("SRC-UNSUPPORTED",where,"documentclass options");
            p=srcarg(p,a,sizeof a,where);if(strcmp(a,"initech-manual")) die("SRC-UNSUPPORTED",where,a);
        } else if(!strcmp(word,"begin") || !strcmp(word,"end")) {
            int begin=!strcmp(word,"begin");
            if(*p!='{') die("SRC-ENVIRONMENT",where,"begin/end must immediately precede {name}");
            p=srcarg(p,a,sizeof a,where);
            if(passive) die("SRC-CONTEXT",where,"environment inside argument");
            if(!begin) {
                if(!s->nenv || strcmp(a,s->env[s->nenv-1])) die("SRC-ENVIRONMENT",where,"unmatched end");
                s->nenv--;
                if(!strcmp(a,"document")) s->phase=2;
            } else if(!strcmp(a,"TypeIn") || !strcmp(a,"verbatim")) {
                const char *close=!strcmp(a,"TypeIn")?"\\end{TypeIn}":"\\end{verbatim}";
                x=strstr(p,close);
                if(!x || x>=end || memchr(p,'\\',(size_t)(x-p)))
                    die("SRC-VERBATIM",where,"TeX control sequence in verbatim text or missing end");
                p=x+strlen(close);
            } else {
                const char *sig=NULL;
                if(!strcmp(a,"document")) {
                    if(s->phase || s->nenv || s->files!=1 || s->depth!=1 || !s->class_seen)
                        die("SRC-CONTEXT",where,"document boundary");
                    s->phase=1;sig="";
                } else {
                    if(!s->phase) die("SRC-CONTEXT",where,"environment before document");
                    if(!strcmp(a,"XbaseEntry")) {
                        srceffects(s,passive,where);
                        int dr=*skipws(p)=='[';p=srcargs(s,p,"o",where);
                        p=srcarg(p,b,sizeof b,where);p=srcarg(p,word,sizeof word,where);
                        if(strcmp(word,"compound") && strcmp(word,"command") && strcmp(word,"function") && strcmp(word,"set"))
                            die("SRC-KIND",where,word);
                        strcpy(s->state,dr?"DRAFT":"GATED");
                        if(strcmp(word,"compound")) {srcreg(s,b,word,s->state);strcpy(s->kind,word);}
                        else {strcpy(s->kind,"command");srctitle(b,where);}
                        sig="";
                    } else if(!strcmp(a,"XbaseMessageEntry")) sig="omv";
                    else if(!strcmp(a,"Transcript")) {
                        srceffects(s,passive,where);p=srcarg(p,b,sizeof b,where);srcexample(s,b,1,where);sig="";
                    } else if(!strcmp(a,"tabular")) sig="om";
                    else if(!strcmp(a,"CoercionTable")) sig="m";
                    else if(!strcmp(a,"AteNote") || !strcmp(a,"AteWarning") || !strcmp(a,"ChapterCovers") ||
                            !strcmp(a,"Steps") || !strcmp(a,"Tip") || !strcmp(a,"TitleVerso") ||
                            !strcmp(a,"enumerate") || !strcmp(a,"itemize") || !strcmp(a,"AssignmentTable")) sig="";
                }
                if(!sig || s->nenv==64 || strlen(a)>=sizeof s->env[0]) die("SRC-ENVIRONMENT",where,a);
                strcpy(s->env[s->nenv++],a);p=srcargs(s,p,sig,where);
            }
        } else if(!strcmp(word,"input") || !strcmp(word,"TableRows")) {
            srceffects(s,passive,where);
            if(*skipws(p)!='{') die("SRC-INPUT",where,"unbraced input is unsupported");
            p=srcarg(p,a,sizeof a,where);
            if(!strcmp(word,"TableRows") && strcmp(a,"samir_limits") && strcmp(a,"samir_formats"))
                die("SRC-INPUT",where,"TableRows supports only samir_limits and samir_formats");
            srcwalk(s,a,where);
        } else if(!strcmp(word,"QREntry")) {
            srceffects(s,passive,where);int dr=*skipws(p)=='[';p=srcargs(s,p,"o",where);
            p=srcarg(p,a,sizeof a,where);p=srcarg(p,b,sizeof b,where);
            if(strcmp(b,"compound") && !commandkind(b)) die("SRC-KIND",where,b);
            strcpy(s->state,dr?"DRAFT":"GATED");
            if(!strcmp(b,"compound")) {strcpy(s->kind,"command");srctitle(a,where);}
            else srcreg(s,a,b,s->state);
            p=srcargs(s,p,"mm",where);
        } else if(!strcmp(word,"QRFunction") || !strcmp(word,"XAlso") || !strcmp(word,"DotExample")) {
            srceffects(s,passive,where);p=srcarg(p,a,sizeof a,where);
            if(!strcmp(word,"DotExample")) srcexample(s,a,0,where);
            else {
                srcreg(s,a,!strcmp(word,"QRFunction")?"function":s->kind,!strcmp(word,"QRFunction")?"GATED":s->state);
                p=srcargs(s,p,"m",where);
            }
        } else {
            size_t i;
            if((!strcmp(word,"term") && !(s->definitions & 2u)) || (!strcmp(word,"bs") && !(s->definitions & 8u)))
                die("SRC-DEFINITION",where,"local macro has no audited definition");
            for(i=0;i<sizeof srcpassive/sizeof *srcpassive;i++) if(!strcmp(word,srcpassive[i].name)) break;
            if(i==sizeof srcpassive/sizeof *srcpassive) die("SRC-UNKNOWN",where,word);
            p=srcargs(s,p,srcpassive[i].args,where);
        }
        if(p>end) die("SRC-SYNTAX",where,"macro argument crosses enclosing group");
    }
    s->depth--;
}
static void srcwalk(Src *s,const char *file,const char *where)
{
    char name[512],path[PATH_MAX];int generated=0,old=s->generated;
    size_t n=strlen(file);
    if(n>4 && !strcmp(file+n-4,".tex")) n-=4;
    if(!n || n>=sizeof name) die("SRC-INPUT",where,file);
    memcpy(name,file,n);name[n]=0;
    if(!srcstem(name)) die("SRC-INPUT",where,"only literal book-local basenames and named generated tables are supported");
    static const char *tables[]={"samir_limits","samir_formats","samir_coercion","samir_messages"};
    for(size_t i=0;i<sizeof tables/sizeof *tables;i++) if(!strcmp(name,tables[i])) generated=1;
    if(snprintf(name+n,sizeof name-n,".tex")>=(int)(sizeof name-n)) die("SRC-INPUT",where,file);
    srcpath(path,sizeof path,s->dir,name,"SRC-INPUT");
    if(generated) {
        struct stat st;
        if(!lstat(path,&st)) die("SRC-INPUT",path,"book-local shadow of generated table");
        srcpath(path,sizeof path,s->gen,name,"SRC-INPUT");
    }
    if(++s->files>64) die("SRC-INPUT",where,"input nesting/cycle limit");
    srcregular(path,"SRC-INPUT");char *text=readfile(path);struct stat st;
    if(stat(path,&st) || strlen(text)!=(size_t)st.st_size)
        die("SRC-SYNTAX",path,"embedded NUL or source changed while reading");
    for(const char *p=text;*p;p++) if((unsigned char)*p>126 ||
       ((unsigned char)*p<32 && !strchr("\n\r\t",*p))) die("SRC-SYNTAX",path,"non-ASCII source byte");
    s->generated=generated;srcscan(s,text,text+strlen(text),0,path);s->generated=old;
    free(text);s->files--;
}
/* srclockstep <shipped> <allow> <vol-one> <quick-ref> <vol-two> <generated-dir> */
static void srclockstep(int argc,char **argv)
{
    static Set ship,allow,doc,draft;size_t nex=0,ntrans=0;
    if(argc!=8) die("USAGE","srclockstep","<shipped> <allow> <vol-one dir> <quick-ref dir> <vol-two dir> <generated dir>");
    ship=list(argv[2],0);allow=list(argv[3],1);
    for(int b=0;b<3;b++) {
        Src s;char rootpath[PATH_MAX];memset(&s,0,sizeof s);
        doc.n=draft.n=0;s.doc=&doc;s.draft=&draft;s.book=argv[4+b];strcpy(s.state,"GATED");
        if(!realpath(s.book,s.dir) || !realpath(argv[7],s.gen)) die("SRC-INPUT",s.book,"cannot resolve source/generated directory");
        srcpath(rootpath,sizeof rootpath,s.dir,"../..","SRC-INPUT");
        if(!realpath(rootpath,s.root)) die("SRC-INPUT",s.book,"cannot resolve publications root");
        srcsupportcheck(s.root);srcwalk(&s,"main",s.book);
        if(s.phase!=2 || s.nenv) die("SRC-ENVIRONMENT",s.book,"missing document end or unclosed environment");
        nex+=s.nex;ntrans+=s.ntrans;
        if(getenv("SRCLOCKSTEP_DUMP")) for(size_t i=0;i<doc.n;i++) fprintf(stderr,"%d|%s\n",b,doc.item[i]);
        if(b==2) {
            if(doc.n) die("COMMAND-SURFACE",s.book,"Volume Two teaches; its commands are documented in Using InitechBase");
            continue;
        }
        crosscheck_sets(s.book,argv[3],&ship,&doc,&draft,&allow);
        printf(">>> test-pubs-lockstep-src: %s: %zu shipped, %zu documented, %zu DRAFT\n",s.book,ship.n,doc.n,draft.n);
    }
    printf(">>> test-pubs-lockstep-src: green (%zu DotExample typed-line checks, %zu Transcript pairs; restricted source grammar, no LaTeX run)\n",nex,ntrans);
}
