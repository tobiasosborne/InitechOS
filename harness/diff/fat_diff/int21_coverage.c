/* Factory coverage certificate: the locked register is the sole scope source.
 * Compare it bidirectionally with the real ah_is_listed() inventory, then
 * require a witness in the Makefile gate vector for every registered AH.
 * Ref: ADR-0003 Appendix A (2026-10-17 DEC-16 reconciliation); PRD 6.1/8;
 * initech-40oq. The dispatcher and witnesses are independent evidence. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *read_file(const char *name)
{
    FILE *f=fopen(name,"rb");
    if(!f) { perror(name); exit(2); }
    if(fseek(f,0,SEEK_END))exit(2);
    long n=ftell(f); if(n<0 || n>2000000)exit(2);
    rewind(f); char *s=calloc((size_t)n+1,1); if(!s)exit(2);
    if(fread(s,1,(size_t)n,f)!=(size_t)n)exit(2);
    fclose(f); return s;
}
/* Expand the locked register's ASCII AH notation at a string boundary. */
static void expand(const char *p,const char *end,unsigned char bits[256])
{
    while(p<end) {
        if(!isxdigit((unsigned char)*p)) { p++; continue; }
        char *q; unsigned long lo=strtoul(p,&q,16);
        if(q>=end || *q!='h' || lo>255) { p++; continue; }
        p=q+1;
        while(p<end && *p==' ')p++;
        unsigned long hi=lo;
        if(p<end && *p=='-') {
            p++;
            while(p<end && *p==' ')p++;
            hi=strtoul(p,&q,16);
            if(q>=end || *q!='h' || hi<lo || hi>255)exit(2);
            p=q+1;
        }
        for(unsigned long i=lo;i<=hi;i++)bits[i]=1;
    }
}
static int has_gate(const char *vector,const char *gate)
{
    size_t n=strlen(gate);
    for(const char *p=vector;(p=strstr(p,gate));p++)
        if((p==vector || isspace((unsigned char)p[-1])) &&
           (!p[n] || isspace((unsigned char)p[n])))return 1;
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=5)return 2;
    char *json=read_file(argv[1]);
    char *vector=read_file(argv[2]),*witness=read_file(argv[3]);
    char *inventory=read_file(argv[4]);
    unsigned char registered[256]={0},actual[256]={0},covered[256]={0};
    char *p;
    for(p=json;(p=strstr(p,"\"ah\""));p++) {
        char *colon=strchr(p,':'),*begin=colon?strchr(colon,'"'):0;
        char *finish=begin?strchr(begin+1,'"'):0; if(!finish)return 2;
        expand(begin+1,finish,registered); p=finish;
    }
    for(p=inventory;(p=strstr(p,"INT21:"));p+=6) {
        unsigned ah; if(sscanf(p+6,"%2x",&ah)!=1)return 2;
        actual[ah]=1;
    }
    unsigned n=0;
    for(unsigned ah=0;ah<256;ah++) {
        if(actual[ah] && !registered[ah]) {
            fprintf(stderr,"RED kernel-coverage register dropped dispatcher AH=%02X: restore the function in spec/int21h_register.json\n",ah);return 1;
        }
        if(registered[ah] && !actual[ah]) {
            fprintf(stderr,"RED kernel-coverage dispatcher register omits AH=%02X\n",ah);return 1;
        }
        n+=registered[ah];
    }
    if(!n) { fputs("RED kernel-coverage empty scope\n",stderr);return 1; }
    /* Each row supplies an exercised function/range and its real named gate.
     * A missing gate cannot be papered over with an unreferenced target. */
    for(char *line=strtok(witness,"\n");line;line=strtok(0,"\n")) {
        if(*line=='#' || !*line)continue;
        char field[80],gate[120]; if(sscanf(line,"%79s %119s",field,gate)!=2)return 2;
        unsigned char row[256]={0}; expand(field,field+strlen(field),row);
        for(unsigned ah=0;ah<256;ah++)if(row[ah] && has_gate(vector,gate)) {
            covered[ah]=1;
            printf("AH=%02X gate=%s\n",ah,gate);
        }
    }
    for(unsigned ah=0;ah<256;ah++)if(registered[ah]) {
        if(!covered[ah]) { fprintf(stderr,"RED kernel-coverage AH=%02X has no implemented gate in TEST vectors\n",ah);return 1; }
    }
    printf("GREEN kernel-coverage: %u Appendix-A AHs registered and exercised by named TEST-vector gates\n",n);
    free(json);free(vector);free(witness);free(inventory);return 0;
}
