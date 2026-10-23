/* XCOPY and TREE, native flat .COM-equivalent programs (initech-zuvo).
 *
 * Law 1: MS-DOS_3.3_Users_Guide_198707.pdf printed pp.122,127-129
 * (PDF 207,212-214): operands, /A /M /D /S /E /P /V /W, exit codes;
 * p.324 (PDF409): F/D prompt; pp.314,341: cyclic/parameter diagnostics;
 * p.54 (PDF139): DATE separators and the 1980-2079 year window.
 * IBM ZR23-6845-01 printed D-6 (PDF62): pre-DOS-4 TREE column layout,
 * continued subdirectories have a colon at column 16, Files names at 9.
 * IBM 6138519 Reference printed 7-184 (PDF289): volume heading and None.
 * ADR-0003 Appendix A + DEC-08 / DEC-04a: flat native INT21 ABI;
 * spec/find_data.h: DTA layout. mcopy -s on identical images grades the
 * results; mdir -/ supplies the independent TREE inventory.
 *
 * All enumeration, wildcard matching, paths, and file transfers use the
 * SAME INT21 services as COPY/DIR. No FAT decoder or second FAT walker.
 * Only ordinary DOS path text and DTA find results enter these programs.
 * Re-start a find by ordinal after recursion: this kernel has one active
 * find context (int21.c g_find), rather than DOS's opaque DTA resume state.
 * This keeps parent searches intact without interpreting private DTA bytes.
 */
#include "xcopy_tree.h"
#include "find_data.h"
#include "dos_messages.h" /* DEC-13/App C; MS-DOS 3.3 pp.29,308,330,361. */
#include "dospath.h"     /* q45q: drive-qualified operands (DEC-18). */

#define CAP 128
#define S_RECURSE 1
#define E_EMPTY 2
#define A_ARCHIVE 4
#define M_CLEAR 8
#define P_PROMPT 16
#define V_VERIFY 32
#define W_WAIT 64
static find_data_t dta;
static char pattern[13];
static unsigned flags, copied, date_min, default_drive;
static int error;

static unsigned length(const char *s) { unsigned n=0; while (s[n]) n++; return n; }
static int equal(const char *a, const char *b)
{ while (*a && *a == *b) { a++; b++; } return *a == *b; }
static int put(char *to, const char *from)
{
    unsigned n=length(from);
    if (n >= CAP) { error=4; return 0; }
    for (unsigned i=0; i<=n; i++) to[i]=from[i];
    return 1;
}
static char upper(char c) { return c >= 'a' && c <= 'z' ? (char)(c-32) : c; }
static int call(unsigned a, unsigned b, unsigned c, uintptr_t d)
{
    du_regs r = {a,b,c,d,0};
    return du_call(&r) ? -(int)(r.a & 65535) : (int)r.a;
}
static void text(const char *s)
{
    unsigned n=length(s);
    if (call(0x4000,1,n,(uintptr_t)s) != (int)n) error=5;
}
static void line(const char *s) { text(s); text("\r\n"); }
static int key(void)
{
    /* AH=07h deliberately leaves ^C to XCOPY, so /W returns its documented
     * code 2 instead of the kernel's default INT23 abort code. IBM DOS 3.30
     * Technical Reference p.6-59; MS-DOS 3.3 Reference p.128. */
#ifdef DU_MUT_CTRL_C
    int c = call(0x0800,0,0,0) & 255;
#else
    int c = call(0x0700,0,0,0) & 255;
#endif
    if (c == 3) error=2;
    return upper((char)c);
}
static int yes(void)
{
    int c;
    do { c=key(); } while (c != 'Y' && c != 'N' && !error);
    text(c == 'Y' ? "Y\r\n" : "N\r\n");
    return c == 'Y' && !error;
}
static void number(unsigned n)
{
    char buf[12]; unsigned at=11;
    buf[at]=0;
    do { buf[--at]=(char)('0'+n%10); n/=10; } while (n);
    text(buf+at);
}

/* Canonical absolute DOS text "D:\\PATH" (no links in FAT), the dospath.h
 * resolver ATTRIB also uses. Dot/dotdot resolve before the self/cyclic guard,
 * including aliases through a drive's working directory. The drive is ALWAYS
 * explicit: audit L032, a D: target written under A:'s tree, came from the
 * former drive-stripping resolver. CHDIR validates existing directories;
 * file services validate leaf names. MS-DOS 3.3 Reference pp4-11, 127. */
static int drive_cwd(unsigned drive, char out[64])
{
#ifdef DU_MUT_REL_ROOT
    (void)drive; out[0]=0; return 0;
#else
    du_regs r={0x4700,0,0,drive+1,(uintptr_t)out};
    out[0]=0;
    return du_call(&r) ? -1 : 0;
#endif
}
static int absolute(const char *in, char out[CAP])
{
#ifdef DU_MUT_DRIVE
    if (in[0] && in[1] == ':') in+=2;   /* Rule 6: the pre-q45q resolver. */
#endif
    int rc=dospath_full(in,default_drive,drive_cwd,out,CAP);
    if (rc == DOSPATH_TOO_LONG) error=4;
    return rc == DOSPATH_OK;
}
static int join(char out[CAP], const char *dir, const char *leaf)
{
    unsigned n=length(dir), k=length(leaf);
    if (n+k+2 > CAP) { error=4; return 0; }
    if (!put(out,dir)) return 0;
    if (n && out[n-1] != '\\') out[n++]='\\';
    for (unsigned i=0; i<=k; i++) out[n+i]=leaf[i];
    return 1;
}
/* Probe by CHDIR, then restore THAT drive's directory: CHDIR d:path moves
 * only drive d's working directory (MS-DOS 3.3 Reference p.41). */
static int directory(const char *p)
{
    char back[CAP]={p[0],':',0};
    if (!absolute(back,back)) return 0;
    int ok=call(0x3b00,0,0,(uintptr_t)p) >= 0;
    if (ok && call(0x3b00,0,0,(uintptr_t)back) < 0) error=4;
    return ok;
}
static int ensure_dir(const char *p)
{
    if (directory(p)) return 1;
    char parent[CAP];
    if (!put(parent,p)) return 0;
    dospath_parent(parent);
    if (equal(parent,p) || !ensure_dir(parent)) return 0;
    if (call(0x3900,0,0,(uintptr_t)p) < 0) {
        line(MSG_DOS_0017); error=5; return 0;
    }
    return 1;
}

/* Same AH=1A/4E/4F DTA contract as dir_program.asm. Error propagation matters:
 * a disk error must never masquerade as an empty subdirectory. */
static int nth(const char *spec, unsigned attr, unsigned n, find_data_t *out)
{
    call(0x1a00,0,0,(uintptr_t)&dta);
    int rc=call(0x4e00,0,attr,(uintptr_t)spec);
    while (rc >= 0 && n--) rc=call(0x4f00,0,0,0);
    if (rc < 0) {
        if (rc != -2 && rc != -18) error=5;
        return 0;
    }
    *out=dta;
    return 1;
}
static int visible(const find_data_t *fd)
{
#ifdef DU_MUT_HIDDEN
    return !(fd->attr & 8) && !equal(fd->fname,".") && !equal(fd->fname,"..");
#else
    return !(fd->attr & 14) && !equal(fd->fname,".") && !equal(fd->fname,"..");
#endif
}

/* /D is inclusive, with the US DOS 3.3 mm-dd-yy[yy] DATE format. FAT dates
 * already compare chronologically as packed unsigned integers (AH=57h/DTA).
 * DOS country formats belong to the kernel's country setting. */
static int parse_date(const char *s)
{
    unsigned val[3]={0,0,0}, part=0, digits=0;
    for (; *s; s++) {
        if (*s >= '0' && *s <= '9') {
            if (++digits > 4) return 0;
            val[part]=val[part]*10+(unsigned)(*s-'0');
        } else if ((*s == '-' || *s == '/') && part < 2 && digits) {
            part++; digits=0;
        } else return 0;
    }
    if (part != 2 || !digits) return 0;
    unsigned m=val[0], d=val[1], y=val[2];
#ifdef DU_MUT_DATE_RANGE
    if (y < 100) y+=1900;
    if (y < 1980 || y > 2099 || m < 1 || m > 12) return 0;
#else
    if (y < 100) y+=y < 80 ? 2000 : 1900;
    if (y < 1980 || y > 2079 || m < 1 || m > 12) return 0;
#endif
    static const unsigned char days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    unsigned max=days[m-1]+(m == 2 && y%4 == 0);
    if (!d || d > max) return 0;
    date_min=((y-1980)<<9)|(m<<5)|d;
    return 1;
}
static int selected(const find_data_t *fd)
{
    if (!visible(fd) || (fd->attr & 16)) return 0;
#ifndef DU_MUT_A
    if ((flags & A_ARCHIVE) && !(fd->attr & 32)) return 0;
#endif
#ifndef DU_MUT_D
    if (fd->fdate < date_min) return 0;
#endif
    return 1;
}

static int transfer(const char *src, const char *dst, const find_data_t *fd)
{
    unsigned char buf[2048], verify[2048];
#ifndef DU_MUT_SELF
    if (equal(src,dst)) { line(MSG_DOS_0020); error=4; return 0; }
#endif
    if (flags & P_PROMPT) {
#ifndef DU_MUT_P
        text(src+2); text(" (Y/N?)");
        if (!yes()) return 0;
#else
        (void)yes;
#endif
    }
    char parent[CAP];
    if (!put(parent,dst)) return 0;
    dospath_parent(parent);
    if (!ensure_dir(parent)) return 0;
    int in=call(0x3d00,0,0,(uintptr_t)src);
    if (in < 0) { error=5; return 0; }
    int out=call(0x3c00,0,0,(uintptr_t)dst);
    if (out < 0) { call(0x3e00,(unsigned)in,0,0); error=5; return 0; }
    for (;;) {
        int n=call(0x3f00,(unsigned)in,sizeof(buf),(uintptr_t)buf);
        if (n < 0) { error=5; break; }
        if (!n) break;
        if (call(0x4000,(unsigned)out,(unsigned)n,(uintptr_t)buf) != n) { error=5; break; }
    }
    /* Source date/time survives the last WRITE and CLOSE (initech-qqrz).
     * Ref: MS-DOS 3.3 Reference p.50 (PDF135), mcopy -m on identical images;
     * IBM DOS 3.30 Tech Ref p.6-208 (PDF331), AH=57h CX=time, DX=date. */
    du_regs r={0x5701,(unsigned)out,fd->ftime,fd->fdate,0};
#ifdef DU_MUT_TIME
    r.c=0; r.d=0;
#endif
    if (du_call(&r)) error=5;
    if (call(0x3e00,(unsigned)out,0,0) < 0) error=5;
    if (call(0x3e00,(unsigned)in,0,0) < 0) error=5;
    if (error) return 0;
    if (flags & V_VERIFY) {
#ifndef DU_MUT_V
        in=call(0x3d00,0,0,(uintptr_t)src);
        out=call(0x3d00,0,0,(uintptr_t)dst);
        if (in < 0 || out < 0) error=5;
        while (!error) {
            int n=call(0x3f00,(unsigned)in,sizeof(buf),(uintptr_t)buf);
            int k=call(0x3f00,(unsigned)out,sizeof(verify),(uintptr_t)verify);
            if (n < 0 || k != n) { error=5; break; }
            for (int i=0; i<n; i++) if (buf[i] != verify[i]) { error=5; break; }
            if (!n) break;
        }
        if (in >= 0) call(0x3e00,(unsigned)in,0,0);
        if (out >= 0) call(0x3e00,(unsigned)out,0,0);
#else
        (void)verify;
#endif
    }
    if (error) return 0;
    /* mcopy oracle: copied files have archive set; source read-only survives. */
    if (call(0x4301,0,(fd->attr & 1)|32,(uintptr_t)dst) < 0) { error=5; return 0; }
    if (flags & M_CLEAR) {
#ifndef DU_MUT_M
        if (call(0x4301,0,fd->attr & ~32u,(uintptr_t)src) < 0) { error=5; return 0; }
#endif
    }
    copied++;
    if (!(flags & P_PROMPT)) line(src+2);   /* pre-q45q listing text */
    return 1;
}

/* Empty directories are created eagerly ONLY for /S /E. Otherwise directories
 * appear lazily when an accepted file needs them, pruning whole empty trees
 * and directories whose files fail the wildcard/archive/date filters. */
static void copy_dir(const char *src, const char *dst, const char *target_file, unsigned depth)
{
    if (depth > 32) { error=4; return; }
    char spec[CAP], from[CAP], to[CAP]; find_data_t fd;
    if (!join(spec,src,pattern)) return;
    for (unsigned n=0; !error && nth(spec,0,n,&fd); n++) {
        if (!selected(&fd)) continue;
        if (!join(from,src,fd.fname)) return;
        if (target_file) { if (!put(to,target_file)) return; }
        else if (!join(to,dst,fd.fname)) return;
        transfer(from,to,&fd);
    }
    if (!(flags & S_RECURSE) || error) return;
    if (!join(spec,src,"*.*")) return;
    for (unsigned n=0; !error && nth(spec,16,n,&fd); n++) {
        if (!visible(&fd) || !(fd.attr & 16)) continue;
        if (!join(from,src,fd.fname) || !join(to,dst,fd.fname)) return;
        if ((flags & E_EMPTY) && !ensure_dir(to)) return;
        copy_dir(from,to,0,depth+1);
    }
}

static void tree_dir(const char *path, int files, unsigned depth)
{
    if (depth > 32) { error=4; return; }
    char spec[CAP], child[CAP]; find_data_t fd; unsigned count=0;
    text("\r\nPath: "); line(path+2);   /* IBM D-6: no drive in Path: */
#ifdef DU_MUT_TREE_LAYOUT
    text("\r\nSubdirectories:");
#else
    text("\r\nSub-directories:");
#endif
    if (!join(spec,path,"*.*")) return;
    for (unsigned n=0; !error && nth(spec,16,n,&fd); n++) {
        if (!visible(&fd) || !(fd.attr & 16)) continue;
        text(count++ ? "               : " : " "); line(fd.fname);
    }
    if (!count) line(" None");
    if (files) {
        text("\r\nFiles:\r\n"); count=0;
        for (unsigned n=0; !error && nth(spec,0,n,&fd); n++) {
            if (!visible(&fd)) continue;
            text("        "); line(fd.fname); count++;
        }
        if (!count) line("        None");
    }
    for (unsigned n=0; !error && nth(spec,16,n,&fd); n++) {
        if (!visible(&fd) || !(fd.attr & 16)) continue;
        if (!join(child,path,fd.fname)) return;
        tree_dir(child,files,depth+1);
    }
}
static void volume_heading(char drive)
{
    find_data_t fd;
    char root[8]={drive,':'};   /* the walked drive's label (L014) */
    put(root+2,"\\*.*");
    text("DIRECTORY PATH LISTING FOR VOLUME ");
    for (unsigned n=0; nth(root,8,n,&fd); n++) {
        if (fd.attr & 8) {
            text(fd.fname);
            break;
        }
    }
    text("\r\n");
}

int du_main(int tree, char *tail)
{
    char source[CAP]="", target[CAP]="", src[CAP], dst[CAP];
    unsigned operands=0; int files=0;
    flags=0; copied=0; date_min=0; error=0;
    du_regs r={0x1900,0,0,0,0};   /* AH=19h: the default drive (0=A:) */
    du_call(&r);
    default_drive=r.a & 255;
    /* Tokenize operands and adjacent switches (/S/E is valid period syntax).
     * Date slashes, if used, are consumed inside /D's token. */
    char *p=tail;
    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        if (*p == '/') {
            p++; char opt=upper(*p++);
            if (tree) {
                if (opt != 'F') goto invalid;
                files=1;
            } else if (opt == 'D' && *p == ':') {
                char date[24]; unsigned n=0; p++;
                while (*p && *p != ' ' && *p != '\t') {
                    if (*p == '/' && (p[1] < '0' || p[1] > '9')) break;
                    if (n+1 >= sizeof(date)) goto invalid;
                    date[n++]=*p++;
                }
                date[n]=0;
                if (!parse_date(date)) goto invalid;
            } else {
                switch (opt) {
                    case 'S': flags|=S_RECURSE; break;
                    case 'E': flags|=E_EMPTY; break;
                    case 'A': flags|=A_ARCHIVE; flags&=~M_CLEAR; break;
                    case 'M': flags|=A_ARCHIVE|M_CLEAR; break;
                    case 'P': flags|=P_PROMPT; break;
                    case 'V': flags|=V_VERIFY; break;
                    case 'W': flags|=W_WAIT; break;
                    default: goto invalid;
                }
            }
            if (*p && *p != '/' && *p != ' ' && *p != '\t') goto invalid;
        } else {
            char *out=operands == 0 ? source : target; unsigned n=0;
            if (++operands > (tree ? 1u : 2u)) goto parameters;
            while (*p && *p != '/' && *p != ' ' && *p != '\t') {
                if (n+1 >= CAP) goto invalid;
                out[n++]=*p++;
            }
            out[n]=0;
        }
    }
    if (tree) {
#ifdef DU_MUT_TREE_F
        files=0;
#endif
        /* TREE [d:] lists the whole drive (MS-DOS 3.3 Reference p.122):
         * an empty operand or a bare "d:" names that drive's root. */
        char root[4]={'\\',0,0,0};
        if (*source && source[1] == ':' && !source[2]) {
            root[0]=source[0]; root[1]=':'; root[2]='\\';
        }
        if (!absolute(*source && root[1] != ':' ? source : root,src) ||
            !directory(src)) goto path_error;
        volume_heading(src[0]); tree_dir(src,files,0);
        return error;
    }
    if (!operands) goto parameters;
#ifndef DU_MUT_E_REQUIRES_S
    if ((flags & E_EMPTY) && !(flags & S_RECURSE)) goto invalid;
#endif
#ifdef DU_MUT_DEFAULT
    flags|=S_RECURSE;
#endif
#ifdef DU_MUT_S
    flags&=~S_RECURSE;
#endif
#ifdef DU_MUT_E
    flags&=~E_EMPTY;
#endif
    if (flags & W_WAIT) {
#ifndef DU_MUT_W
        line("Press any key when ready to start"); line("copying files");
        key(); if (error) return error;
#endif
    }
    if (!absolute(source,src) || !absolute(*target ? target : ".",dst)) goto invalid;
#ifdef DU_MUT_OMIT
    if (!*target) put(dst,"\\");
#endif
    if (directory(src)) {
        pattern[0]='*'; pattern[1]='.'; pattern[2]='*'; pattern[3]=0;
    } else {
        unsigned n=length(src), split=n;
        while (split && src[split-1] != '\\') split--;
        if (n-split >= sizeof(pattern)) goto invalid;
        for (unsigned i=0; i<=n-split; i++) pattern[i]=src[split+i];
        dospath_parent(src);
        if (!directory(src)) goto path_error;
    }
#ifdef DU_MUT_WILD
    for (char *q=pattern; *q; q++) if (*q == '?') *q='@';
#endif
    char spec[CAP], target_file[CAP]; find_data_t fd;
    if (!join(spec,src,pattern)) goto invalid;
    if (!nth(spec,0,0,&fd)) {
        if (error) return error;
#ifdef DU_MUT_DESCEND
        line("0" MSG_DOS_0024 "copied"); return 1;
#endif
        if (!(flags & S_RECURSE)) { line("0" MSG_DOS_0024 "copied"); return 1; }
    }
    int isdir=directory(dst);
    unsigned tlen=length(target);
    int trailing=tlen && (target[tlen-1] == '\\' || target[tlen-1] == ':');
    int exists=call(0x4300,0,0,(uintptr_t)dst) >= 0;
    if (!isdir && !exists && !trailing) {
        text("Does "); text(target); text(" specify a file name or directory name on the target (F = file, D = directory)?");
        int c;
        do { c=key(); } while (!error && c != 'F' && c != 'D');
        text("\r\n"); if (error) return error;
        isdir=c == 'D';
    } else if (trailing) isdir=1;
#ifdef DU_MUT_TARGET
    isdir=1;
#endif
    if ((flags & S_RECURSE) && isdir) {
        unsigned n=length(src);
        if (equal(src,dst) || n == 3 ||   /* the "D:\\" root */
            (length(dst)>n && dst[n] == '\\')) {
            int prefix=1;
            for (unsigned i=0; i<n && dst[i]; i++) if (dst[i] != src[i]) prefix=0;
            if (prefix) {
#ifdef DU_MUT_CYCLIC
                return 0;
#else
                line(MSG_DOS_0050); return 4;
#endif
            }
        }
    }
    if (!isdir && (flags & S_RECURSE)) goto invalid;
    if (isdir && (flags & E_EMPTY) && !ensure_dir(dst)) return error;
    if (!isdir) {
        if (!put(target_file,dst)) return error;
        /* Multiple sources cannot be collapsed onto one file. */
        if (nth(spec,0,1,&fd)) goto invalid;
    }
    copy_dir(src,dst,isdir ? 0 : target_file,0);
    if (error == 5) line(MSG_DOS_0009);
#ifdef DU_MUT_COUNT
    copied++;
#endif
    number(copied); line(MSG_DOS_0024 "copied");
    return error ? error : copied ? 0 : 1;
invalid:
    line(MSG_DOS_0051); return 4;
parameters:
    line(MSG_DOS_0038); return 4;
path_error:
    line(MSG_DOS_0039); return 4;
}
