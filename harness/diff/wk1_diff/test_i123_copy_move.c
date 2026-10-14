/* Factory C independent coordinate model, R2.2 Reference /Copy pp. 3-2..6
 * and /Move pp. 7-2..4. Text expectations never use artifact ref helpers. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include "i123.h"
static uint8_t mem[2][131072], cm[I123_CALC_BYTES(256)];
static i123_calc_t cx;
static unsigned rng=0x123a55,checks,failures;
static unsigned random_(unsigned n) { rng=rng*1664525u+1013904223u; return (rng>>8)%n; }
#define CHECK(x,why) do {checks++; if (!(x)) {failures++; if(failures<8) fprintf(stderr,"RED-REASON %s line %d\n",why,__LINE__);}}while(0)
static void ref(char *out,int c,int r,int ac,int ar)
{ sprintf(out,"%s%c%s%d",ac?"$":"",'A'+c,ar?"$":"",r+1); }
static void text(i123_sheet_t *s,int c,int r,char *out)
{ const i123_cell_t *q=i123_find(s,c,r); out[0]=0; if(q) CHECK(i123_formula_text(s->pool+q->off,q->len,c,r,out,512)==0,"decompile"); }
int main(int argc,char **argv)
{
    if(argc==3&&!strcmp(argv[1],"--corpus")) {
        char path[1024];snprintf(path,sizeof path,"%s/mint",argv[2]);
        DIR *dir=opendir(path);if(!dir){perror("mint corpus");return 2;}
        struct dirent *de;unsigned copies=0,moves=0;
        while((de=readdir(dir))) {
            if(!strstr(de->d_name,".steps"))continue;
            snprintf(path,sizeof path,"%s/mint/%s",argv[2],de->d_name);
            FILE *f=fopen(path,"r");if(!f){closedir(dir);return 2;}
            char line[512];while(fgets(line,sizeof line,f)) {
                if(!strncmp(line,"type /c",7)||!strncmp(line,"type /C",7))copies++;
                if(!strncmp(line,"type /m",7)||!strncmp(line,"type /M",7))moves++;
            }
            fclose(f);
        }
        closedir(dir);
        fprintf(stderr,"!!! test-i123-copy-move: corpus evidence unavailable: %u /Copy and %u /Move mint commands; real command goldens required\n",copies,moves);
        return 2;
    }
    if(argc!=1)return 2;
    i123_sheet_t sh,dst;
    CHECK(i123_calc_init(&cx,cm,sizeof cm,256,0)==0,"init");
    for(unsigned trial=0;trial<4000;trial++) {
        int moving=trial&1, sc=4+random_(4),sr=4+random_(6), dc=1+random_(10),dr=1+random_(12);
        int tc[2]={ (int)random_(16),(int)random_(22) },tr[2]={ (int)random_(20),(int)random_(22) };
        int ac[2]={ (int)random_(2),(int)random_(2) },ar[2]={ (int)random_(2),(int)random_(2) };
        char a[32],b[32],formula[100],expected[512],got[512];
        i123_rect_t rg={(uint16_t)sc,(uint16_t)sr,(uint16_t)(sc+1),(uint16_t)(sr+1)};
        if(trial%3==0) {tc[0]=sc;tr[0]=sr+1;} /* moved precedent */
        if(trial%5==0) {tc[1]=dc;tr[1]=dr+1;} /* moved-over precedent */
        CHECK(i123_sheet_init(&sh,mem[0],sizeof mem[0],256)==0,"init");
        CHECK(i123_sheet_init(&dst,mem[1],sizeof mem[1],256)==0,"init");
        sh.calcmode=0;
        for(int r=0;r<24;r++) for(int c=0;c<10;c++) CHECK(i123_set_number(&sh,c,r,c+r)==0,"number");
        ref(a,tc[0],tr[0],ac[0],ar[0]);ref(b,tc[1],tr[1],ac[1],ar[1]);
        if(trial%4==2)sprintf(formula,"@SUM(%s..%s)",a,b);else sprintf(formula,"+%s+%s",a,b);
        CHECK(i123_enter_formula(&cx,&sh,sc,sr,formula,strlen(formula))==0,"formula");
        CHECK(i123_enter_formula(&cx,&sh,23,23,formula,strlen(formula))==0,"formula");
        CHECK(i123_transfer(&sh,&dst,rg,dc,dr,moving)==0,"transfer");
        for(int outside=0;outside<2;outside++) {
            int nc=outside?23:dc,nr=outside?23:dr;
            char parts[2][32];
            for(int k=0;k<2;k++) {
                int c=tc[k],r=tr[k],err=0;
                if(moving) {
                    if(c>=sc&&c<=sc+1&&r>=sr&&r<=sr+1) {c+=dc-sc;r+=dr-sr;}
                    else if(c>=dc&&c<=dc+1&&r>=dr&&r<=dr+1) err=1;
                } else if(!outside) {if(!ac[k])c+=dc-sc;if(!ar[k])r+=dr-sr;}
                /* Lotus relative columns wrap modulo 256; this generator
                 * stays at A..X, except negative copied references. */
                if(c<0)c+=256;
                if(r<0||r>=8192)err=1;
                if(err)strcpy(parts[k],"@ERR");
                else if(c>=26) { /* independent A..IV spelling */
                    sprintf(parts[k],"%s%c%c%s%d",ac[k]?"$":"",'A'+c/26-1,'A'+c%26,ar[k]?"$":"",r+1);
                } else ref(parts[k],c,r,ac[k],ar[k]);
            }
            if(trial%4==2) {
                if(!strcmp(parts[0],"@ERR")||!strcmp(parts[1],"@ERR"))strcpy(expected,"@SUM(@ERR)");
                else sprintf(expected,"@SUM(%s..%s)",parts[0],parts[1]);
            } else sprintf(expected,"%s%s+%s",(parts[0][0]=='$'||parts[0][0]=='@')?"":"+",parts[0],parts[1]);
            text(&dst,nc,nr,got);
            CHECK(!strcmp(got,expected),outside?"move-referrers":"absolute-relative");
        }
    }
    CHECK(i123_sheet_init(&sh,mem[0],sizeof mem[0],256)==0,"init");
    CHECK(i123_sheet_init(&dst,mem[1],sizeof mem[1],256)==0,"init");
    i123_rect_t rg={0,0,1,0},found;
    CHECK(i123_name_set(&sh,"TOTAL",rg)==0,"name-create");
    CHECK(i123_name_find(&sh,"total",5,&found)==0&&found.c1==1,"name-find");
    CHECK(i123_set_number(&sh,0,0,3)==0&&i123_set_number(&sh,1,0,4)==0,"numbers");
    CHECK(i123_enter_formula(&cx,&sh,4,0,"@SUM(TOTAL)",11)==0,"name-formula");
    i123_val_t v; i123_cell_value(&sh,4,0,&v); CHECK(v.t==I123_V_NUM&&v.n==7,"name-value");
    CHECK(i123_transfer(&sh,&dst,rg,0,2,1)==0,"named-move");
    CHECK(i123_name_find(&dst,"TOTAL",5,&found)==0&&found.r0==2&&found.r1==2,"name-move");
    CHECK(i123_recalc(&cx,&dst)==0,"recalc"); i123_cell_value(&dst,4,0,&v);
    CHECK(v.t==I123_V_NUM&&v.n==7,"name-referrer");
    CHECK(i123_name_delete(&dst,"TOTAL")==0&&i123_name_find(&dst,"TOTAL",5,&found)!=0,"name-delete");
    CHECK(i123_name_set(&dst,"ZZZ",rg)==0&&i123_name_set(&dst,"AAA",rg)==0,"name-create");
    CHECK(i123_name_table(&dst,10,10)==0,"name-table");
    char table[512];i123_cell_content(&dst,i123_find(&dst,10,10),table,sizeof table);
    CHECK(!strcmp(table,"'AAA"),"name-table-alphabetical");
    i123_cell_content(&dst,i123_find(&dst,10,11),table,sizeof table);
    CHECK(!strcmp(table,"'ZZZ"),"name-table-alphabetical");
    CHECK(!i123_find(&dst,10,12)&&!i123_find(&dst,11,12),"name-table-blank-row");
    CHECK(i123_name_set(&sh,"TOTAL",(i123_rect_t){3,3,4,3})==I123_E_ARG,"name-rebind-refused");
    CHECK(i123_sheet_init(&dst,mem[1],sizeof mem[1],1)==0,"init");
    CHECK(i123_transfer(&sh,&dst,rg,0,2,0)==I123_E_FULL,"capacity-refusal");
    i123_cell_value(&sh,0,0,&v);CHECK(v.t==I123_V_NUM&&v.n==3,"source-intact");
    CHECK(i123_transfer(&sh,&sh,rg,0,2,0)==I123_E_ARG,"alias-refusal");
    printf("i123-copy-move: %u checks, %u failures (4000 random sheets)\n",checks,failures);
    return failures?1:0;
}
