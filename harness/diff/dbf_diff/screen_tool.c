/* Factory C, PRD 6.6 / W4.0: compile timed BIOS events and grade real pages. */
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include "screen_file.h"

/* DOS names are case-insensitive. Validate before copying so aliases, links,
 * nested payloads and supplied captures cannot replace the oracle/driver. */
static void setup_check(const char *path)
{
    DIR *dir=opendir(path);
    if (!dir) screen_die("cannot open SETUP directory");
    struct dirent *e;
    while ((e=readdir(dir))) {
        if (!strcmp(e->d_name,".") || !strcmp(e->d_name,"..")) continue;
        char upper[13], child[4096];
        size_t n=strlen(e->d_name), base=0, ext=0;
        if (!n || n>=sizeof upper) screen_die("SETUP requires DOS 8.3 names");
        int dot=0;
        for (size_t i=0;i<n;i++) {
            unsigned char c=(unsigned char)e->d_name[i];
            upper[i]=(char)toupper(c);
            if (c=='.') { if (dot++ || !base) screen_die("bad SETUP name"); }
            else {
                if (!isalnum(c) && c!='_' && c!='-') screen_die("bad SETUP name");
                if (dot) ext++; else base++;
            }
        }
        upper[n]=0;
        if (base>8 || ext>3 || (dot && !ext)) screen_die("SETUP requires DOS 8.3 names");
        if (!strncmp(upper,"DBASE.",6) || !strncmp(upper,"DBASEINL.",9) ||
            !strncmp(upper,"DUMP",4) || !strcmp(upper,"CAPTURE.OK") ||
            !strcmp(upper,"SCREEN.COM") || !strcmp(upper,"EVENTS.BIN") ||
            !strcmp(upper,"MINT.PRG") || !strcmp(upper,"AUTOEXEC.BAT") ||
            !strcmp(upper,"CONFIG.SYS")) {
            fprintf(stderr,"screen: reserved SETUP name: %s\n",e->d_name); exit(2);
        }
        int len=snprintf(child,sizeof child,"%s/%s",path,e->d_name);
        if (len<0 || (size_t)len>=sizeof child) screen_die("SETUP path too long");
        struct stat st;
        if (lstat(child,&st)) screen_die("cannot stat SETUP entry");
        if (S_ISDIR(st.st_mode)) setup_check(child);
        else if (!S_ISREG(st.st_mode)) screen_die("SETUP links/special files forbidden");
    }
    closedir(dir);
}

static void event(FILE *f, int op, unsigned value)
{
    fputc(op, f); fputc(value & 255, f); fputc(value >> 8, f);
}
static unsigned key(const char *s)
{
    static const struct { const char *name; unsigned value; } keys[] = {
        {"ENTER",0x1c0d}, {"ESC",0x011b}, {"DOWN",0x5000}, {"UP",0x4800},
        {"LEFT",0x4b00}, {"RIGHT",0x4d00}, {"HOME",0x4700}, {"END",0x4f00},
        {"PGUP",0x4900}, {"PGDN",0x5100}, {"TAB",0x0f09}, {"BACKSPACE",0x0e08},
        {"DELETE",0x5300}, {"CTRL-Y",0x1519}, {"CTRL-END",0x7500},
        {"CTRL-HOME",0x7700}, {"F1",0x3b00}, {"F2",0x3c00}, {"F10",0x4400}
    };
    for (unsigned i=0; i<sizeof keys/sizeof keys[0]; i++)
        if (!strcmp(s,keys[i].name)) return keys[i].value;
    screen_die("unknown KEY"); return 0;
}
static unsigned text_key(unsigned char ch)
{
    const char *plain="1234567890-=qwertyuiop[]asdfghjkl;'`\\zxcvbnm,./";
    const char *shift="!@#$%^&*()_+QWERTYUIOP{}ASDFGHJKL:\"~|ZXCVBNM<>?";
    const unsigned char scans[]={2,3,4,5,6,7,8,9,10,11,12,13,
        16,17,18,19,20,21,22,23,24,25,26,27,
        30,31,32,33,34,35,36,37,38,39,40,41,43,
        44,45,46,47,48,49,50,51,52,53};
    if (ch==' ') return 0x3920;
    const char *p=strchr(plain,ch);
    if (p) return ch|((unsigned)scans[p-plain]<<8);
    p=strchr(shift,ch);
    if (p) return ch|((unsigned)scans[p-shift]<<8);
    screen_die("TEXT has no US BIOS key mapping"); return 0;
}
static int dump_names(const char *path, char names[8][33])
{
    FILE *f = screen_open(path,"r"); char b[128]; int n=0;
    while (fgets(b,sizeof b,f)) {
        b[strcspn(b,"\r\n")] = 0;
        if (!*b || *b=='#') continue;
        if (n==8 || strlen(b)>32) screen_die("DUMPS: at most eight names of <=32 bytes");
        for (unsigned j=0;b[j];j++)
            if (!isalnum((unsigned char)b[j]) && b[j]!='-' && b[j]!='_')
                screen_die("unsafe DUMP name");
        for (int j=0;j<n;j++) if (!strcmp(names[j],b)) screen_die("duplicate DUMP");
        strcpy(names[n++],b);
    }
    fclose(f); if (!n) screen_die("empty DUMPS"); return n;
}
static void compile(const char *keys, const char *dumps, const char *output)
{
    char names[8][33], b[512]; int n=dump_names(dumps,names), used[8]={0};
    FILE *in=screen_open(keys,"r"), *out=screen_open(output,"wb");
    while (fgets(b,sizeof b,in)) {
        if (!strchr(b,'\n') && !feof(in)) screen_die("KEYS line too long");
        b[strcspn(b,"\r\n")]=0;
        if (!*b || *b=='#') continue;
        if (!strncmp(b,"WAIT ",5)) {
            char *end; unsigned long v=strtoul(b+5,&end,10);
            if (!isdigit((unsigned char)b[5]) || *end || !v || v>65535)
                screen_die("WAIT needs 1..65535 BIOS ticks");
            event(out,2,(unsigned)v);
        } else if (!strncmp(b,"KEY ",4)) event(out,1,key(b+4));
        else if (!strncmp(b,"TEXT ",5)) {
            for (char *p=b+5;*p;p++) {
                if ((unsigned char)*p<32 || (unsigned char)*p>126) screen_die("TEXT is printable ASCII");
                event(out,1,text_key((unsigned char)*p));
            }
        } else if (!strncmp(b,"DUMP ",5)) {
            int i; for (i=0;i<n;i++) if (!strcmp(b+5,names[i])) break;
            if (i==n || used[i]++) screen_die("DUMP not listed, or used twice");
            event(out,3,(unsigned)i);
        } else screen_die("unknown KEYS directive");
        if (ftell(out)>8187) screen_die("script exceeds 8192 bytes");
    }
    for (int i=0;i<n;i++) if (!used[i]) screen_die("listed DUMP never used");
    event(out,0,0); fclose(in);
    if (fclose(out)) screen_die("event write failed");
}
int main(int argc,char **argv)
{
    screen_file g,a;
    if (argc==3 && !strcmp(argv[1],"check-setup")) { setup_check(argv[2]); return 0; }
    if (argc==4 && !strcmp(argv[1],"check-save")) {
        char names[8][33]; int n=dump_names(argv[2],names);
        FILE *f=screen_open(argv[3],"rb");
        int lo=fgetc(f),hi=fgetc(f),extra=fgetc(f); fclose(f);
        if (lo<0 || hi<0 || extra!=EOF || (lo|(hi<<8))!=(1<<n)-1)
            screen_die("save marker does not cover all declared dumps");
        return 0;
    }
    if (argc==3 && !strcmp(argv[1],"codec-test")) {
        for (int i=0;i<2000;i++) { g.bytes[2*i]=(unsigned char)i; g.bytes[2*i+1]=(unsigned char)(i/3); }
        g.bytes[4000]=24; g.bytes[4001]=79;
        screen_write(argv[2],&g); screen_read(argv[2],&a);
        if (memcmp(&g,&a,sizeof g)) screen_die("CP437/attribute round-trip failed");
        puts("GREEN: screen codec round-trips all 256 character and attribute bytes");
        return 0;
    }
    if (argc==5 && !strcmp(argv[1],"compile")) { compile(argv[2],argv[3],argv[4]); return 0; }
    if (argc==4 && !strcmp(argv[1],"encode")) { screen_raw(argv[2],&g); screen_write(argv[3],&g); return 0; }
    if (argc==4 && !strcmp(argv[1],"decode")) {
        screen_read(argv[2],&g); FILE *f=screen_open(argv[3],"wb");
        if (fwrite(g.bytes,1,4002,f)!=4002 || fclose(f)) screen_die("raw write failed");
        return 0;
    }
    if (argc==4 && (!strcmp(argv[1],"compare") || !strcmp(argv[1],"compare-raw"))) {
        screen_read(argv[2],&g);
        if (!strcmp(argv[1],"compare-raw")) screen_raw(argv[3],&a); else screen_read(argv[3],&a);
        return screen_compare(&g,&a);
    }
    if (argc==4 && !strncmp(argv[1],"mutate-",7)) {
        screen_read(argv[2],&g);
        if (!strcmp(argv[1],"mutate-cell")) g.bytes[0]^=1;
        else if (!strcmp(argv[1],"mutate-attr")) g.bytes[1]^=1;
        else if (!strcmp(argv[1],"mutate-cursor")) g.bytes[4001]=(g.bytes[4001]+1)%80;
        else screen_die("unknown mutation");
        screen_write(argv[3],&g); return 0;
    }
    fprintf(stderr,"usage: screen_tool compile KEYS DUMPS EVENTS | encode RAW SCREEN | decode SCREEN RAW\n"
                   "       screen_tool compare GOLDEN SCREEN | compare-raw GOLDEN RAW\n"
                   "       screen_tool mutate-cell|mutate-attr|mutate-cursor SCREEN OUTPUT\n");
    return 2;
}
