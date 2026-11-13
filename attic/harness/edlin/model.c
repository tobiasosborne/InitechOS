/* Independent, heap/vector EDLIN command model (never linked into the OS).
 * Ground truth: MS-DOS_3.3_Users_Guide_198707.pdf printed pp.162-193,
 * PDF pp.247-278 (rendered at 110 dpi). Cases below identify their pages.
 * Artifact deliberately uses a packed arena and streaming DOS handles instead.
 */
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define LIMIT 49152
static char *v[65534], *orig[65534];
static int n,cur=1,on,next,used;
static FILE *trans;
static const unsigned char *keys;
static char needle[254],replacement[254];
static const char letter[]="Dear Mr. Dimm,\n\nI was sorry to hear of your recent\nhospitalization due to electrical\nshock from our X-1000 Automatic\nPencil Sharpener.\n\nAs a result of your accident, we\nare redesigning our manual to\nwarn our customers against trying\nto sharpen metal objects.\n\nSincerely,\n\nI.R. Sharpe, President\n";
static const char motto[]="Sharpe Office Supplies\nThe World Leader in Office Sharpeware\nOur motto: \"You oughta be Sharpe too\"\n";
static const char original[]="Dear Mr. Dimm,\n\nI was sorry to hear of your recent\nhospitalization due to electrical\nshock from our Automatic\nPencil Sharpener.\n";
typedef struct { const char *name,*seed,*script,*tail; int page; } scenario;
static const scenario cases[]={
 {"new",NULL,"i\rSharpe Office Supplies.\r\000\075\r\003L\re\r","IN.TXT",162},
 {"edit",original,"5\r\000\074A\000\122X-1000 \000\075\rL\re\r","IN.TXT",178},
 {"replace",letter,"5,10rour\032the\r1,15L\re\r","IN.TXT",187},
 {"repeat","mine mine\n","1,1rmine\032ours\re\r","IN.TXT",187},
 {"commands",letter,"16,18,1m\r1,5,19,2c\r7d\r1,3p\r1,3L\re\r","IN.TXT",175},
 {"search",letter,"2,12sto\r1,?sto\rny1,1szzz\rq\rn\rq\ry\r","IN.TXT",191},
 {"confirm",letter,"5,10?rour\032the\rynynq\ry\r","IN.TXT",189},
 {"transfer",letter,"12tirsharpe.mem\r125w\r100a\re\r","IN.TXT",192},
 {"keys","Sharpe Office Supplies.\n","1\r\000\073\000\123\000\075\r1\rjunk\033\000\075\r1\r\000\074c\000\122cial\000\122 Sharpeware.\000\077\000\122Introducing: \000\075\r1\r\000\076c\000\075\re\r","IN.TXT",164},
 {"binary","one\n\032two\n","1,9L\re\r","IN.TXT /B",172},
 {"eof","one\n\032two\n","1,9L\re\r","IN.TXT",171},
 {"staged","@large","125w\r100a\r150,160L\re\r","IN.TXT",193},
 {"insert",original,"5i\rSharpe Office Supplies\r\0031,7L\re\r","IN.TXT",180},
 {"relative",letter,"15;-5,+5L\r\000\075\r1 3L\r3,20,15c\rq\ry\r","IN.TXT",171},
 {"robackup",letter,"e\r","IN.TXT",164},
 {"path",letter,"5,10rour\032the\re\r","A:\\SUB\\IN.TXT",170},
 {"move",letter,"16,18,1m\r1,18L\re\r","IN.TXT",184},
 {"bakname",letter,"","IN.BAK",313}
};
/* C strings contain embedded DOS extended-key NULs, so lengths are explicit. */
static const size_t scriptlen[]={sizeof("i\rSharpe Office Supplies.\r\000\075\r\003L\re\r")-1,
 sizeof("5\r\000\074A\000\122X-1000 \000\075\rL\re\r")-1,
 sizeof("5,10rour\032the\r1,15L\re\r")-1,sizeof("1,1rmine\032ours\re\r")-1,
 sizeof("16,18,1m\r1,5,19,2c\r7d\r1,3p\r1,3L\re\r")-1,
 sizeof("2,12sto\r1,?sto\rny1,1szzz\rq\rn\rq\ry\r")-1,
 sizeof("5,10?rour\032the\rynynq\ry\r")-1,
 sizeof("12tirsharpe.mem\r125w\r100a\re\r")-1,
 sizeof("1\r\000\073\000\123\000\075\r1\rjunk\033\000\075\r1\r\000\074c\000\122cial\000\122 Sharpeware.\000\077\000\122Introducing: \000\075\r1\r\000\076c\000\075\re\r")-1,
 sizeof("1,9L\re\r")-1,sizeof("1,9L\re\r")-1,sizeof("125w\r100a\r150,160L\re\r")-1,
 sizeof("5i\rSharpe Office Supplies\r\0031,7L\re\r")-1,
 sizeof("15;-5,+5L\r\000\075\r1 3L\r3,20,15c\rq\ry\r")-1,
 sizeof("e\r")-1,sizeof("5,10rour\032the\re\r")-1,
 sizeof("16,18,1m\r1,18L\re\r")-1,0};
static const unsigned char *keyend;
static void die(const char *s){fprintf(stderr,"RED EDLIN-model %s\n",s);exit(1);}
static int gc(void){if(keys==keyend)die("script exhausted");return *keys++;}
static void echo(int c){if(c<32 && c!=9){fputc('^',trans);fputc(c+64,trans);}else fputc(c,trans);}
static void prefix(int l,int star){fprintf(trans,"%5d:%c",l,star?'*':' ');}
static void show(int l,const char *s){prefix(l,l==cur);while(*s)echo((unsigned char)*s++);fputs("\r\n",trans);}
/* Independent template editor: the template has a separate cursor from output. */
static int line(char *out,const char *tmpl,int cap){
 char t[254];snprintf(t,sizeof t,"%s",tmpl);int len=0,pos=0,ins=0;
 for(;;){int c=gc();
  if(c==13){out[len]=0;fputs("\r\n",trans);return len;}
  if(c==3){fputs("^C\r\n",trans);return -1;}
  if(c==27){fputs("\\\r\n",trans);len=pos=ins=0;continue;}
  if(c==8){if(len){len--;if(pos && !ins)pos--;fputs("\b \b",trans);}continue;}
  if(!c){c=gc();int end=(int)strlen(t);
   if(c==82){ins=!ins;continue;}
   if(c==83){if(pos<end)pos++;continue;}
   if(c==63){out[len]=0;strcpy(t,out);fputs("@\r\n",trans);len=pos=ins=0;continue;}
   if(c==59 || c==61 || c==60 || c==62){
    int stop=c==59?pos+1:end;
    if(c==60 || c==62){int target=gc();char *p=strchr(t+pos,target);if(!p)continue;stop=(int)(p-t);}
    while(pos<stop && pos<end){if(c!=62 && len<cap){out[len++]=t[pos];echo((unsigned char)t[pos]);}pos++;}
    ins=0;
   }continue;
  }
  if(c==22){c=gc();c=toupper(c)&31;}
  if(len<cap){out[len++]=(char)c;echo(c);if(!ins && t[pos])pos++;}else fputc(7,trans);
 }
}
static void add(int at,const char *s){memmove(v+at+1,v+at,(size_t)(n-at)*sizeof *v);v[at]=strdup(s);n++;used+=(int)strlen(s)+1;}
static void del(int a,int b){for(int i=a;i<b;i++){used-=(int)strlen(v[i])+1;free(v[i]);}memmove(v+a,v+b,(size_t)(n-b)*sizeof *v);n-=b-a;}
static void load(int count){while(next<on && count-- && used+(int)strlen(orig[next])+1<=LIMIT){add(n,orig[next++]);}if(next==on)fputs("End of input file\r\n",trans);}
static void append_default(void){while(next<on && used<LIMIT*3/4){add(n,orig[next++]);}if(next==on)fputs("End of input file\r\n",trans);}
static int address(char **p){char *s=*p;int x,rel=0;while(*s==' ')s++;if(*s=='.'){s++;x=cur;}else if(*s=='#'){s++;x=n+1;}else {if(*s=='+'||*s=='-')rel=1;if(!isdigit((unsigned char)*s)&&!rel){*p=s;return -1;}char *e;x=(int)strtol(s,&e,10);if(e==s)die("bad address");s=e;if(rel)x+=cur;}*p=s;if(x<1)x=1;return x;}
static int yes(void){fputs("O.K.? ",trans);int c=gc();if(c!=13)echo(c);fputs("\r\n",trans);return c=='y'||c=='Y'||c==13;}
static void diskline(FILE *f,const char *s){fwrite(s,1,strlen(s),f);fputs("\r\n",f);}
static void model(const scenario *sc){
 if(!strcmp(sc->name,"bakname")){fputs("Cannot edit .BAK file--rename file\r\n",trans);return;}
 char command[254],edit[254],temp[254];FILE *out=NULL;int done=0;
 cur=1;n=on=next=used=0;needle[0]=replacement[0]=0;
 if(!strcmp(sc->name,"robackup"))fputs("WARNING! Backup file is read-only -- backup\r\nwill not be made\r\n",trans);
 FILE *src=fopen("IN.TXT","rb");if(src){char *s=NULL;size_t z=0;ssize_t k;
  while((k=getline(&s,&z,src))>=0){if(k && s[k-1]=='\n')s[--k]=0;if(k&&s[k-1]=='\r')s[--k]=0;
   if(!strstr(sc->tail,"/B")){char *p=strchr(s,26);if(p){*p=0;if(*s)orig[on++]=strdup(s);break;}}
   orig[on++]=strdup(s);
  }free(s);fclose(src);append_default();
 }else fputs("New file\r\n",trans);
 while(!done){fputc('*',trans);if(line(command,"",253)<0)continue;
  char *p=command;do {
   int a[4]={-1,-1,-1,-1},ac=0,ask=0;
   for(;;){if(ac==4)die("too many args");a[ac++]=address(&p);int space=0;while(*p==' '){space=1;p++;}if(*p==','){p++;continue;}if(space&&(isdigit((unsigned char)*p)||*p=='.'||*p=='#'||*p=='+'||*p=='-'))continue;break;}
   if(*p=='?'){ask=1;p++;}while(*p==' ')p++;
   int cmd=toupper((unsigned char)*p);if(cmd && cmd!=';')p++;else cmd=0;
   if(cmd!='A'&&cmd!='W')for(int i=0;i<3;i++)if(a[i]>n+1)a[i]=n+1;
   int first=a[0]<0?cur:a[0],last=a[1]<0?first:a[1];
   if(cmd=='I'){int at=first-1,old=n;for(;;){prefix(at+1,1);if(line(edit,at? v[at-1]:"",253)<0)break;add(at++,edit);}cur=first<=old?at+1:(at?at:1);}
   else if(cmd=='D'){if(first>n||last<first){fputs("Entry error\r\n",trans);}else {del(first-1,last);cur=first;}}
   else if(cmd=='C'||cmd=='M'){int dest=a[2];if(dest<1||first>n||last>n||last<first||(dest>=first&&dest<=last)){fputs("Entry error\r\n",trans);}else {
    int cnt=last-first+1,times=a[3]<0?1:a[3];char **copy=malloc((size_t)cnt*sizeof *copy);for(int i=0;i<cnt;i++)copy[i]=strdup(v[first-1+i]);
    if(cmd=='M'){del(first-1,last);if(dest>last)dest-=cnt;times=1;}
    for(int t=0;t<times;t++)for(int i=0;i<cnt;i++)add(dest-1+t*cnt+i,copy[i]);
    cur=dest;for(int i=0;i<cnt;i++)free(copy[i]);free(copy);
   }}
   else if(cmd=='L'||cmd=='P'){int st=a[0]<0?(cmd=='L'?(cur>11?cur-11:1):cur+1):a[0];int end=a[1]<0?st+22:a[1];if(end>n)end=n;for(int i=st;i<=end;i++)show(i,v[i-1]);if(cmd=='P')cur=end;}
   else if(cmd=='S'||cmd=='R'){
    char *z=strchr(p,26);if(z)*z=0;if(*p)strcpy(needle,p);if(cmd=='R'&&z)strcpy(replacement,z+1);p+=strlen(p);if(z)p=z+1+strlen(z+1);
    int st=a[0]<0?cur+1:a[0],end=a[1]<0?n:a[1],found=0;if(end>n)end=n;
    if(!*needle)continue;
    for(int i=st;i<=end;i++){
     if(cmd=='S'){if(strstr(v[i-1],needle)){show(i,v[i-1]);if(!ask||yes()){cur=i;found=1;break;}}}
     else {size_t off=0;char *hit;while((hit=strstr(v[i-1]+off,needle))){
      size_t at=(size_t)(hit-v[i-1]);snprintf(temp,sizeof temp,"%.*s%s%s",(int)at,v[i-1],replacement,hit+strlen(needle));show(i,temp);found=1;
      if(!ask||yes()){used+=(int)strlen(temp)-(int)strlen(v[i-1]);free(v[i-1]);v[i-1]=strdup(temp);off=at+strlen(replacement);}else off=at+strlen(needle);
     }}
    }if(!found)fputs("Not found\r\n",trans);
   }
   else if(cmd=='T'){while(*p==' ')p++;FILE *f=fopen(p,"rb");if(!f)die("transfer open");char *s=NULL;size_t z=0;int at=first-1;while(getline(&s,&z,f)>=0){s[strcspn(s,"\r\n")]=0;add(at++,s);}free(s);fclose(f);p+=strlen(p);}
   else if(cmd=='A'){if(a[0]<0)append_default();else load(a[0]);}
   else if(cmd=='W'||cmd=='E'){
    if(!out)out=fopen("OUTPUT.TXT","wb");
    if(!out)die("output open");
    int count;
    if(cmd=='E')count=n;else if(a[0]>=0)count=a[0]<n?a[0]:n;else {count=0;int remain=used;while(remain>LIMIT*3/4){remain-=(int)strlen(v[count++])+1;}}
    for(int i=0;i<count;i++)diskline(out,v[i]);
    del(0,count);cur=1;
    if(cmd=='E'){while(next<on)diskline(out,orig[next++]);fputc(26,out);fclose(out);out=NULL;done=1;}
   }
   else if(cmd=='Q'){fputs("Abort edit (Y/N)?",trans);line(edit,"",253);if(toupper((unsigned char)edit[0])=='Y')done=1;}
   else if(!cmd){int k=a[0]<0?cur+1:a[0];if(k>n)fputs("Entry error\r\n",trans);else {cur=k;show(k,v[k-1]);prefix(k,1);if(line(edit,v[k-1],253)>0){used+=(int)strlen(edit)-(int)strlen(v[k-1]);free(v[k-1]);v[k-1]=strdup(edit);}}}
   else die("unknown command");
   if(*p==';')p++;
  }while(*p&&!done);
 }
 if(out){fclose(out);unlink("OUTPUT.TXT");}
 for(int i=0;i<n;i++)free(v[i]);
 for(int i=0;i<on;i++)free(orig[i]);
}
static void seedfile(const char *path,const char *s){FILE *f=fopen(path,"wb");if(!f)die("seed open");for(;*s;s++){if(*s=='\n')fputc(13,f);fputc(*s,f);}fclose(f);}
static void qcode(FILE *f,int c,int *first){const char *s=NULL;char b[32];
 if(!*first)fputc(',',f);
 *first=0;
 if(!c){c=gc();switch(c){case 59:s="f1";break;case 60:s="f2";break;case 61:s="f3";break;case 62:s="f4";break;case 63:s="f5";break;case 82:s="insert";break;case 83:s="delete";break;default:die("scan");}}
 else if(c>=1&&c<=26&&c!=13&&c!=8&&c!=9){snprintf(b,sizeof b,"ctrl-%c",c+'a'-1);s=b;}
 else if(c>='A'&&c<='Z'){snprintf(b,sizeof b,"shift-%c",tolower(c));s=b;}
 else {switch(c){case 13:s="ret";break;case 27:s="esc";break;case 8:s="backspace";break;case 9:s="tab";break;case ' ':s="spc";break;case ',':s="comma";break;case '.':s="dot";break;case '/':s="slash";break;case '\\':s="backslash";break;case '?':s="shift-slash";break;case ':':s="shift-semicolon";break;case ';':s="semicolon";break;case '#':s="shift-3";break;case '+':s="shift-equal";break;case '-':s="minus";break;default:b[0]=(char)c;b[1]=0;s=b;}}
 fputs(s,f);
}
int main(int argc,char **argv){if(argc!=3)return 2;int ix=-1;for(unsigned i=0;i<sizeof cases/sizeof *cases;i++)if(!strcmp(argv[1],cases[i].name))ix=(int)i;if(ix<0)die("case");
 if(mkdir(argv[2],0700)&&access(argv[2],F_OK))die("mkdir");
 if(chdir(argv[2]))die("chdir");
 const scenario *sc=cases+ix;
 if(sc->seed){if(!strcmp(sc->seed,"@large")){FILE*f=fopen("IN.TXT","wb");for(int i=0;i<300;i++){fprintf(f,"%03d ",i);for(int j=4;j<199;j++)fputc('a'+i%26,f);fputs("\r\n",f);}fclose(f);}else {seedfile("IN.TXT",sc->seed);if(!strcmp(sc->name,"commands")||!strcmp(sc->name,"move")){FILE*f=fopen("IN.TXT","ab");for(const char*s=motto;*s;s++){if(*s=='\n')fputc(13,f);fputc(*s,f);}fclose(f);}}}
 seedfile("IN.BAK","old backup\n");seedfile("irsharpe.mem","Sharpe Office Supplies\nThe World Leader in Office Sharpeware\n");
 keys=(const unsigned char*)sc->script;keyend=keys+scriptlen[ix];FILE*f=fopen("keys.bin","wb");fwrite(keys,1,scriptlen[ix],f);fclose(f);
 f=fopen("keys.qmp","w");int first=1;while(keys<keyend)qcode(f,gc(),&first);fputs(",e,x,i,t,ret",f);fclose(f);
 f=fopen("tail","w");fprintf(f,"%s",sc->tail);fclose(f);
 trans=fopen("expected.txt","wb");keys=(const unsigned char*)sc->script;model(sc);if(keys!=keyend)die("unconsumed keys");fclose(trans);
 /* Independent printed-result anchors. Full transcripts come from the model.
  * MS-DOS 3.3 User's Guide p.188 printed result truncated; rule p.187 governs (PM 2026-10-16)
  * line 8 must retain "we".
  * MS-DOS 3.3 User's Guide p.188 printed result truncated; rule p.187 governs (PM 2026-10-16)
  * line 10 must retain "trying".
  */
 if(!strcmp(sc->name,"replace")){FILE*g=fopen("OUTPUT.TXT","rb");char b[2048];size_t k=fread(b,1,sizeof b-1,g);b[k]=0;fclose(g);if(!strstr(b,"As a result of ythe accident, we\r\n")||!strstr(b,"warn the customers against trying\r\n"))die("p.187 replacement rule");}
 if(!strcmp(sc->name,"repeat")){FILE*g=fopen("OUTPUT.TXT","rb");char b[32]={0};size_t k=fread(b,1,sizeof b-1,g);b[k]=0;fclose(g);if(strcmp(b,"ours ours\r\n\032"))die("p.187 all occurrences");}
 if(!strcmp(sc->name,"edit")){FILE*g=fopen("OUTPUT.TXT","rb");char b[512];size_t k=fread(b,1,sizeof b-1,g);b[k]=0;fclose(g);if(!strstr(b,"shock from our X-1000 Automatic\r\n"))die("p.178 printed result");}
 /* Complete independent printed file results, without displayed line numbers.
  * p.178: six-line edited letter. p.184: entire eighteen-line moved letter.
  * DOS EOF marker follows the printed text (p.171).
  */
 if(!strcmp(sc->name,"edit")||!strcmp(sc->name,"move")){
  FILE*g=fopen("MANUAL.TXT","wb");
  if(!strcmp(sc->name,"move"))for(const char*s=motto;*s;s++){if(*s=='\n')fputc(13,g);fputc(*s,g);}
  const char *s=!strcmp(sc->name,"move")?letter:"Dear Mr. Dimm,\n\nI was sorry to hear of your recent\nhospitalization due to electrical\nshock from our X-1000 Automatic\nPencil Sharpener.\n";
  for(;*s;s++){if(*s=='\n')fputc(13,g);fputc(*s,g);}fputc(26,g);fclose(g);
  g=fopen("MANUAL.TXT","rb");FILE*h=fopen("OUTPUT.TXT","rb");int a,b;do{a=fgetc(g);b=fgetc(h);if(a!=b)die("printed file result");}while(a!=EOF);fclose(g);fclose(h);
 }
 printf("GREEN EDLIN model %s (manual p.%d)\n",sc->name,sc->page);return 0;
}
