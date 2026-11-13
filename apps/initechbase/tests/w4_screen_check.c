/* L2 structural oracle. Independent literal assertions, native screen_capture
 * decoder; PPM raster is checked separately with ppm_text_check --cells.
 * No artifact includes. initech-9nzpr, DEC-03a.10. All coords zero-based.
 * TEXT row col literal; BLANK row col length; ATTR row col length hex;
 * CURSOR row col. Each noncomment line is one named assertion class.
 * --mutants changes captured behavior once per assertion, not expectations.
 */
#include <ctype.h>
#include "screen_file.h"
typedef struct { char kind[16],text[256]; int row,col,n,attr; } assertion;
static assertion assertions[256];
static int count;
static void load_assertions(const char *path)
{
    FILE *f=screen_open(path,"r"); char line[512];
    while (fgets(line,sizeof line,f)) {
        if (!strchr(line,'\n')) screen_die("assertion missing newline/too long");
        line[strcspn(line,"\r\n")]=0;
        if (!*line || line[0]=='#') continue;
        if (count==256) screen_die("too many assertions");
        assertion *a=&assertions[count]; int used=0;
        if (sscanf(line,"%15s %d %d%n",a->kind,&a->row,&a->col,&used)!=3 ||
            a->row<0 || a->row>=25 || a->col<0 || a->col>=80) screen_die("bad assertion origin");
        char *tail=line+used; int end=0;
        if (!strcmp(a->kind,"TEXT")) {
            if (*tail++!=' ' || !*tail || strlen(tail)>=sizeof a->text) screen_die("bad TEXT");
            strcpy(a->text,tail); a->n=(int)strlen(tail);
        } else if (!strcmp(a->kind,"CURSOR")) {
            if (*tail) screen_die("bad CURSOR tail");
        } else if (!strcmp(a->kind,"BLANK")) {
            if (sscanf(tail," %d%n",&a->n,&end)!=1 || tail[end]) screen_die("bad BLANK");
        } else if (!strcmp(a->kind,"ATTR")) {
            unsigned attr;
            if (sscanf(tail," %d %x%n",&a->n,&attr,&end)!=2 || tail[end] || attr>255) screen_die("bad ATTR");
            a->attr=(int)attr;
        } else screen_die("unknown structural assertion class");
        if (strcmp(a->kind,"CURSOR") && (a->n<=0 || a->n>80-a->col)) screen_die("assertion outside row");
        count++;
    }
    if (ferror(f) || !count) screen_die("empty/unreadable assertions");
    fclose(f);
}
static int check(const screen_file *s,int verbose)
{
    int bad=0;
    for (int i=0;i<count;i++) {
        assertion *a=&assertions[i]; int ok=1;
        if (!strcmp(a->kind,"CURSOR")) ok=s->bytes[4000]==a->row && s->bytes[4001]==a->col;
        else for (int j=0;j<a->n;j++) {
            unsigned char ch=s->bytes[2*(a->row*80+a->col+j)];
            if (!strcmp(a->kind,"TEXT")) ok &= ch==(unsigned char)a->text[j];
            else if (!strcmp(a->kind,"BLANK")) ok &= ch==' ';
            else ok &= s->bytes[2*(a->row*80+a->col+j)+1]==a->attr;
        }
        if (!ok) { bad++; if (verbose) fprintf(stderr,"RED: L2 %s assertion %d at (%d,%d)\n",a->kind,i+1,a->row,a->col); }
    }
    return bad;
}
int main(int argc,char **argv)
{
    int mutant=argc==5 && !strcmp(argv[1],"--mutants");
    if (argc!=3 && !mutant) { fputs("usage: w4_screen_check SCREEN ASSERTIONS | --mutants SCREEN ASSERTIONS OUT\n",stderr); return 2; }
    screen_file s; screen_read(argv[mutant?2:1],&s); load_assertions(argv[mutant?3:2]);
    if (check(&s,1)) return 1;
    if (mutant) for (int i=0;i<count;i++) {
        screen_file m=s; assertion *a=&assertions[i];
        if (!strcmp(a->kind,"CURSOR")) m.bytes[4001]=(unsigned char)((m.bytes[4001]+1)%80);
        else if (!strcmp(a->kind,"ATTR")) m.bytes[2*(a->row*80+a->col)+1]^=0x70;
        else m.bytes[2*(a->row*80+a->col)]^=1;
        char path[1024];
        if (snprintf(path,sizeof path,"%s/mutant-%d-%s.screen",argv[4],i+1,a->kind)>=(int)sizeof path) screen_die("mutant output path");
        screen_write(path,&m);
        /* Read the written artifact back through the same public checker. */
        screen_read(path,&m);
        if (!check(&m,1)) screen_die("structural mutant survived");
        printf("RED proven: L2 %s assertion %d mutated captured behavior\n",a->kind,i+1);
    }
    printf("GREEN: L2 %d structural assertions%s\n",count,mutant?", each mutant RED":""); return 0;
}
