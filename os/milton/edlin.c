/* EDLIN.COM, DOS 3.3 line editor, initech-udr1 (DEBUG is not implemented here).
 * Ground truth: MS-DOS 3.3 User's Guide, printed pp.162-193 (PDF pp.247-278).
 * ADR-0003 App A supplies every DOS operation; DEC-08 supplies the flat image.
 * All text, switches, numbers and defaults below cite the relevant manual page.
 * Packed NUL-terminated lines occupy a bounded conventional-memory work area;
 * the input handle remains open for A/W staged editing (pp.163,174,193).
 */
#include "edlin.h"
#include "conedit.h"
#include "dos_messages.h" /* DEC-13/App C; MS-DOS 3.3 p.330 (EDLIN). */
#define CAP 49152u
#define FILL (CAP*3u/4u)
#define MAXLINE 253u
static const edlin_io *io;
static char text[CAP],command[254],edited[254],oldtext[254],newtext[254];
static char name[128],backup[128],temporary[128];
static unsigned used,lines,current;
static int output,existed,backup_ro,binary,failed;
typedef struct {
    int h,eof,pos,size,push,ready;
    char buffer[512],pending[254];
} reader;
static reader input,transfer;
static unsigned length(const char *s){unsigned n=0;while(s[n])n++;return n;}
static void copy(char *d,const char *s,unsigned n){for(unsigned i=0;i<n;i++)d[i]=s[i];}
static void shift(char *d,const char *s,unsigned n){
    if(d>s){while(n){n--;d[n]=s[n];}}else for(unsigned i=0;i<n;i++)d[i]=s[i];
}
static int upper(int c){return c>='a'&&c<='z'?c-32:c;}
static void putn(const char *s,unsigned n){if(io->write(1,s,n)!=(int)n)failed=1;}
static void emit(const char *s){putn(s,length(s));}
static void crlf(void){emit("\r\n");}
static void echo(int c){char b[2];if(c<32&&c!=9){b[0]='^';b[1]=(char)(c+64);putn(b,2);}else {b[0]=(char)c;putn(b,1);}}
/* pp.162,172: consecutive numbers, current-line '*' between ':' and text.
 * Printed examples pp.177-191 reserve five positions for the decimal number. */
static void prefix(unsigned n,int star){
    char b[7]={' ',' ',' ',' ',' ',':',' '};
#ifdef EDLIN_MUT_NUMBER
    n++; /* Rule 6: off-by-one numbering. */
#endif
    unsigned i=5;do{b[--i]=(char)('0'+n%10);n/=10;}while(n&&i);
    if(star)b[6]='*';
    putn(b,7);
}
static unsigned offset(unsigned line){unsigned p=0;for(unsigned i=1;i<line&&p<used;i++)p+=length(text+p)+1;return p;}
static void show_text(unsigned n,const char *s){prefix(n,n==current);while(*s)echo((unsigned char)*s++);crlf();}
static void show(unsigned n){show_text(n,text+offset(n));}
static void entry_error(void){emit("Entry error\r\n");}
static int insert(unsigned at,const char *s){
    unsigned n=length(s)+1,p=offset(at);
    if(n>CAP-used||lines>=65533u){emit("Insufficient memory\r\n");return 0;}
    shift(text+p+n,text+p,used-p);copy(text+p,s,n);used+=n;lines++;return 1;
}
static void erase(unsigned a,unsigned b){
    unsigned p=offset(a),q=offset(b+1);shift(text+p,text+q,used-q);used-=q-p;lines-=b-a+1;
}
static int change(unsigned n,const char *s){
    unsigned a=offset(n),b=offset(n+1),size=length(s)+1;
    if(size>CAP-(used-(b-a))){emit("Insufficient memory\r\n");return 0;}
    shift(text+a+size,text+b,used-b);copy(text+a,s,size);used=used-(b-a)+size;return 1;
}
/* pp.164-169,177: share the resident editor mechanics. EDLIN keeps its
 * own template per edited line and its raw Ctrl-C / Ctrl-V policy (p.171). */
typedef struct {char bytes[512];unsigned n;} editor_echo;
static void edlin_echo(uint8_t c,void *ctx){
    editor_echo *e=ctx;
    /* Emit one key's response in one DOS write. This keeps a template recall
     * one output operation, including CR/LF and the backspace erase triplet;
     * raw Ctrl-C belongs to the next get(), not a per-glyph disk operation. */
    if(c<32&&c!=9&&c!=8&&c!=13&&c!=10&&c!=7){
        e->bytes[e->n++]='^';e->bytes[e->n++]=(char)(c+64);
    }else e->bytes[e->n++]=(char)c;
}
static int readline(char *out,const char *source){
    conedit_t editor;conedit_init(&editor,(const uint8_t *)source,length(source));
    for(;;){int c=io->get();
        if(c<0){failed=1;return -1;}
        if(c==3){emit("^C\r\n");return -1;}
        if(c==22 && !editor.extended && !editor.target){
            c=upper(io->get())&31;
            /* Quoted controls are data, even Enter or Escape (p.171). */
            if(editor.count<MAXLINE){out[editor.count++]=(char)c;echo(c);
                if(!editor.insert&&editor.pos<editor.length)editor.pos++;}
            else emit("\007");
            continue;
        }
        editor_echo response={{0},0};
        int done=conedit_key(&editor,(uint8_t)c,(uint8_t *)out,MAXLINE,edlin_echo,&response);
        if(response.n)putn(response.bytes,response.n);
        if(done){
            out[editor.count]=0;return (int)editor.count;
        }
    }
}
static void reader_init(reader *r,int h){r->h=h;r->eof=0;r->pos=r->size=0;r->push=-1;r->ready=0;}
static int getbyte(reader *r){
    if(r->push>=0){int c=r->push;r->push=-1;return c;}
    if(r->eof)return -1;
    if(r->pos==r->size){r->size=io->read(r->h,r->buffer,sizeof r->buffer);r->pos=0;
        if(r->size<=0){if(r->size<0){emit("File access denied\r\n");failed=1;}r->eof=1;return -1;}}
    int c=(unsigned char)r->buffer[r->pos++];
    if(c==26&&!binary){r->eof=1;return -1;} /* pp.171-172 /B */
    return c;
}
static int nextline(reader *r){
    if(r->ready)return 1;
    unsigned n=0;int c;
    while((c=getbyte(r))>=0){
        if(c==10)break;
        if(c==13)continue;
        if(!c){emit("Entry error\r\n");failed=1;return 0;}
        r->pending[n++]=(char)c;
        if(n==MAXLINE){c=getbyte(r);if(c==13)c=getbyte(r);if(c!=10&&c>=0)r->push=c;break;}
    }
    r->pending[n]=0;r->ready=(n||c>=0);return r->ready;
}
/* pp.163,174: default appends to 3/4 memory, explicit n appends n lines.
 * Preserve a pending line when it cannot fit; never lose part of the input. */
static void append(unsigned count,int automatic,int banner){
    while(count&&(!automatic||used<FILL)&&nextline(&input)){
        unsigned n=length(input.pending)+1;
        if(n>CAP-used)break;
        if(!insert(lines+1,input.pending))break;
        input.ready=0;count--;
    }
    if(input.eof&&!input.ready&&banner)emit("End of input file\r\n");
}
static int writebytes(const char *p,unsigned n){
    if(io->write(output,p,n)!=(int)n){emit("Disk full - file write not completed\r\n");failed=1;return 0;}return 1;
}
static int output_open(void){
    if(output>=0)return 1;
    output=io->create(temporary);
    if(output<0){emit("File access denied\r\n");return 0;}return 1;
}
/* p.193: W removes written lines and renumbers from 1. With no count it
 * writes only enough to leave memory 3/4 full; the original is never truncated. */
static int write_lines(unsigned count,int automatic){
    if(!automatic||used>FILL){if(!output_open())return 0;}
    unsigned done=0,p=0;
    while(done<lines&&count&&(!automatic||used-p>FILL)){
        unsigned len=length(text+p);
        if(!writebytes(text+p,len)||!writebytes("\r\n",2))return 0;
        p+=len+1;done++;count--;
    }
    if(done){erase(1,done);current=1;}return 1;
}
/* pp.163-164,179,186: save via temporary; old input becomes .BAK, old BAK
 * replaced unless read-only. Q preserves both original and pre-existing BAK. */
static int save(void){
    if(!write_lines(lines,0))return 0;
    while(nextline(&input)){if(!writebytes(input.pending,length(input.pending))||!writebytes("\r\n",2))return 0;input.ready=0;}
    if(!writebytes("\032",1))return 0;
    if(io->close(output)<0){failed=1;return 0;}output=-1;
    if(input.h>=0){io->close(input.h);input.h=-1;}
    if(!backup_ro){int e=io->remove(backup);if(e<0&&e!=-2){emit("File access denied\r\n");return 0;}}
    if(existed){
#ifdef EDLIN_MUT_BACKUP
        if(io->remove(name)<0)return 0; /* Rule 6: omit original .BAK. */
#else
        if(backup_ro){if(io->remove(name)<0){emit("File access denied\r\n");return 0;}}
        else if(io->rename(name,backup)<0){emit("File access denied\r\n");return 0;}
#endif
    }
    if(io->rename(temporary,name)<0){emit("File access denied\r\n");return 0;}
    return 1;
}
static int number_start(int c){return(c>='0'&&c<='9')||c=='.'||c=='#'||c=='+'||c=='-';}
/* pp.170-172: absolute, relative, '.', '#', missing comma fields, spaces. */
static int address(char **cursor,int *bad){
    char *p=*cursor;while(*p==' ')p++;
    int n=0,sign=1,relative=0;
    if(*p=='.'){p++;n=(int)current;}
    else if(*p=='#'){p++;n=(int)lines+1;}
    else if(number_start(*p)){
        if(*p=='+'||*p=='-'){relative=1;if(*p=='-')sign=-1;p++;}
        if(*p<'0'||*p>'9')*bad=1;
        while(*p>='0'&&*p<='9'){n=n*10+*p++-'0';if(n>=65534){*bad=1;n=65534;}}
        if(relative)n=(int)current+sign*n;
    }else n=-1;
    *cursor=p;if(n==0||n< -1){*bad=1;return -1;}return n;
}
static unsigned bound(int n,unsigned def){return n<0?def:((unsigned)n>lines+1?lines+1:(unsigned)n);}
static int approve(void){emit("O.K.? ");int c=io->get();if(c!=13)echo(c);crlf();return c=='Y'||c=='y'||c==13;}
static int match(const char *s,const char *t){while(*t){if(*s++!=*t++)return 0;}return 1;}
/* pp.187-191: literal, case-sensitive S/R share remembered text1. R visits
 * EVERY nonoverlapping occurrence (and displays once per occurrence), including
 * multiple hits on a line. It skips inserted text when looking for the next hit.
 * PM 2026-10-16: p.187 rule governs p.188's truncated illustrative result.
 */
static void search_replace(unsigned first,unsigned last,int replace,int ask,char *arg){
    char *p=arg;unsigned n=0;
    while(*p&&*p!=26){if(n<MAXLINE)edited[n++]=*p;p++;}edited[n]=0;
    if(n)copy(oldtext,edited,n+1);
    if(*p==26&&replace){p++;n=0;while(*p&&*p!=26){if(n<MAXLINE)newtext[n++]=*p;p++;}newtext[n]=0;}
    unsigned oldlen=length(oldtext),newlen=length(newtext);if(!oldlen)return;
    int found=0;
    if(last>lines)last=lines;
    for(unsigned i=first;i<=last;i++){
        unsigned at=0;
        for(;;){const char *s=text+offset(i);unsigned len=length(s);
            while(at+oldlen<=len&&!match(s+at,oldtext))at++;
            if(at+oldlen>len)break;
            if(!replace){show(i);if(!ask||approve()){current=i;return;}break;}
            if(len-oldlen+newlen>MAXLINE){emit("Line too long\r\n");return;}
            copy(edited,s,at);copy(edited+at,newtext,newlen);copy(edited+at+newlen,s+at+oldlen,len-at-oldlen+1);
            show_text(i,edited);found=1;
            if(!ask||approve()){if(!change(i,edited))return;at+=newlen;}else at+=oldlen;
#ifdef EDLIN_MUT_FIRST
            break; /* Rule 6: incorrectly replace just the first hit per line. */
#endif
        }
    }
    if(!found)emit("Not found\r\n");
}
/* p.175 copies snapshot range count times; p.183 rejects an overlapping move.
 * Packed arena copies adjust source offsets AFTER opening destination space.
 * Move rotates three byte ranges in place and needs no second edit buffer. */
static void reverse(unsigned a,unsigned b){while(a<b){char c=text[a];text[a++]=text[--b];text[b]=c;}}
static void copy_move(unsigned a,unsigned b,unsigned dest,unsigned count,int move){
    unsigned first=offset(a),end=offset(b+1),d=offset(dest),size=end-first;
    if(a>b||b>lines||(dest>=a&&dest<=b)){entry_error();return;}
    if(move){
        if(d<first){reverse(d,first);reverse(first,end);reverse(d,end);current=dest;}
        else {reverse(first,end);reverse(end,d);reverse(first,d);current=dest-(b-a+1);}
    }else {
        if(!count||count>(CAP-used)/size||count>(65533-lines)/(b-a+1)){emit("Insufficient memory\r\n");return;}
        unsigned total=size*count;shift(text+d+total,text+d,used-d);if(first>=d)first+=total;
        for(unsigned k=0;k<count;k++)copy(text+d+k*size,text+first,size);
        used+=total;lines+=(b-a+1)*count;current=dest;
    }
}
static int commands(void){
    char *p=command;
    do {
        while(*p==' '||*p==';')p++;
        int a[4]={-1,-1,-1,-1},ac=0,bad=0,ask=0;
        for(;;){if(ac==4){bad=1;break;}a[ac++]=address(&p,&bad);int space=0;while(*p==' '){space=1;p++;}
            if(*p==','){p++;continue;}if(space&&number_start(*p))continue;break;}
        if(*p=='?'){ask=1;p++;}while(*p==' ')p++;
        int cmd=upper((unsigned char)*p);if(cmd&&cmd!=';')p++;else cmd=0;
        unsigned first=bound(a[0],current),last=bound(a[1],first);
        if(bad||(ask&&cmd!='S'&&cmd!='R')){entry_error();return 0;}
        switch(cmd){
        case 'I': { /* pp.180-181: current line follows inserted block. */
            unsigned pos=first,previous=lines;
            for(;;){prefix(pos,1);const char *t=pos>1?text+offset(pos-1):"";int n=readline(edited,t);if(n<0)break;if(!insert(pos,edited))break;pos++;}
            current=first<=previous?pos:(pos>1?pos-1:1);break;
        }
        case 'D':if(first>lines||last<first)entry_error();else{erase(first,last);current=first;}break; /* p.176 */
        case 'C':case 'M':if(ac<3||a[2]<0||first>lines||last<first)entry_error();else copy_move(first,last,bound(a[2],0),a[3]<0?1u:(unsigned)a[3],cmd=='M');break;
        case 'L':case 'P': { /* pp.182,185 */
            first=bound(a[0],cmd=='L'?(current>11?current-11:1):current+1);
            last=bound(a[1],first+22);if(last>lines)last=lines;
            for(unsigned i=first;i<=last;i++)show(i);
            if(cmd=='P'&&last>=first)current=last;
            break;
        }
        case 'S':case 'R': {
            first=bound(a[0],current+1);last=bound(a[1],lines);
            search_replace(first,last,cmd=='R',ask,p);
            /* Ctrl-Z can delimit a following command, p.171. */
            while(*p&&*p!=26)p++;
            if(*p==26){p++;if(cmd=='R'){while(*p&&*p!=26)p++;if(*p==26)p++;}}break;
        }
        case 'T': { /* p.192 */
            while(*p==' ')p++;
            char path[128];unsigned n=0;while(*p&&*p!=';'&&n<127)path[n++]=*p++;path[n]=0;
            int h=io->open(path);if(h<0){emit(MSG_DOS_0003 "\r\n");break;}
            reader_init(&transfer,h);unsigned pos=first;
            while(nextline(&transfer)){if(!insert(pos++,transfer.pending))break;transfer.ready=0;}
            io->close(h);break;
        }
        case 'A':append(a[0]<0?65533u:(unsigned)a[0],a[0]<0,1);break; /* p.174 */
        case 'W':write_lines(a[0]<0?lines:(unsigned)a[0],a[0]<0);break; /* p.193 */
        case 'E':if(ac>1||a[0]>=0){entry_error();break;}if(save())return 1;
            break; /* p.179 */
        case 'Q': /* p.186: require Abort prompt; preserve old BAK. */
#ifdef EDLIN_MUT_QUIT
            /* Rule 6: omit the required Abort prompt but still consume reply. */
#else
            emit("Abort edit (Y/N)?");
#endif
            if(readline(edited,"")>=0&&upper((unsigned char)edited[0])=='Y')return 1;
            break;
        case 0: { /* pp.177-178: bare Return edits current+1, Return keeps template. */
            unsigned n=bound(a[0],current+1);if(n>lines){entry_error();break;}
            current=n;show(n);prefix(n,1);int k=readline(edited,text+offset(n));if(k>0)change(n,edited);break;
        }
        default:entry_error();return 0;
        }
        if(failed)return 1;
    }while(*p);
    return 0;
}
static void extension(char *dst,const char *src,const char *ext){
    unsigned n=length(src),base=0;copy(dst,src,n+1);
    for(unsigned i=0;i<n;i++){if(src[i]=='\\'||src[i]=='/'||src[i]==':')base=i+1;}
    unsigned dot=n;for(unsigned i=base;i<n;i++)if(src[i]=='.')dot=i;
    if(dot>123)dot=123;
    copy(dst+dot,ext,5);
}
int edlin_run(const edlin_io *ops,const char *tail){
    io=ops;used=lines=0;current=1;output=-1;failed=binary=0;oldtext[0]=newtext[0]=0;
    while(*tail==' ')tail++;
    unsigned n=0;while(*tail&&*tail!=' '&&*tail!='/'&&n<123)name[n++]=*tail++;name[n]=0;
    while(*tail==' ')tail++;
    if(*tail=='/'&&upper(tail[1])=='B'){binary=1;tail+=2;while(*tail==' ')tail++;}
    if(!n||*tail){entry_error();return 1;}
    /* Appendix F p.313: .BAK is reserved; refuse before any file mutation. */
    if(n>=4&&name[n-4]=='.'&&upper(name[n-3])=='B'&&
       upper(name[n-2])=='A'&&upper(name[n-1])=='K'){
        emit("Cannot edit .BAK file--rename file\r\n");return 1;
    }
    extension(backup,name,".BAK");extension(temporary,name,".$$$");
    backup_ro=io->attr(backup);backup_ro=backup_ro>=0&&(backup_ro&1);
    if(backup_ro)emit("WARNING! Backup file is read-only -- backup\r\nwill not be made\r\n"); /* p.164 */
    int h=io->open(name);existed=h>=0;
    if(h<0&&h!=-2){emit("File access denied\r\n");return 1;}
    reader_init(&input,h);
    if(h<0){input.eof=1;emit("New file\r\n");}else append(65533,1,1);
    for(;;){emit("*");if(readline(command,"")<0){if(failed)break;continue;}if(commands())break;}
    if(input.h>=0)io->close(input.h);
    if(output>=0)io->close(output);
    io->remove(temporary);
    return failed?1:0;
}
