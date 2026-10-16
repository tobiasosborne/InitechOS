/* ATTRIB.COM / LABEL.COM -- freestanding DOS 3.3 utilities (gyvx/f88h).
 * Law 1, local initech-manuals-ref/dos33 sources:
 * MS-DOS_3.3_Users_Guide_198707.pdf p.33 (PDF118): R/A, wildcards, /S,
 * directory operands forbidden; pp.338/341/348 (PDF423/426/433): diagnostics;
 * pp.86-87 (PDF171-172): label grammar, spaces,
 * forbidden characters, deletion only on Y; p.126 (PDF211): VOL.
 * 6138519_DOS_3.10_Reference_Feb85.pdf pp.7-17/18 (PDF122-123): attribute
 * letters then full path, blanks for unset fields; pp.7-127/128 (PDF232-233):
 * exact IBM dialogue, uppercase examples, first eleven characters retained.
 * ADR-0003 Appendix A/B.1, DEC-08; DEC-15; spec/find_data.h/dos_structs.h.
 * Disk effects are diffed against mattrib/mlabel on identical input images.
 * Only R/A are exposed. No host runtime, clock, paths or metadata.
 */
#include "dosutil.h"
#include "dos_messages.h" /* DEC-13/App C; MS-DOS 3.3 pp.308,340. */
#define PATH_CAP 80
static unsigned slen(const char *s) { unsigned n=0; while (s[n]) n++; return n; }
static char upper(char c) { return c>='a' && c<='z' ? c-'a'+'A' : c; }
static int space(char c) { return c==' ' || c=='\t'; }
static int equal(const char *a,const char *b)
{
    while (*a && *a==*b) { a++; b++; }
    return *a==*b;
}
static void copy(char *d,const char *s) { while ((*d++=*s++)) {} }
static int error(const char *s) { du_puts(s); du_puts("\r\n"); return 1; }

/* DOS path limit: MS 3.3 User's Reference p.8. GETCWD supplies live CWD;
 * normalize dot/parent components before printing a full path. */
static int fullpath(const char *src,char out[PATH_CAP])
{
    char raw[160],cwd[64];
    unsigned n=0;
    int drive=du_drive();
    if (src[0] && src[1]==':') { drive=upper(src[0])-'A'; src+=2; }
    if (drive!=du_drive()) return -1;
    out[0]=(char)('A'+drive); out[1]=':'; out[2]='\\'; out[3]=0;
    if (*src!='\\') {
        if (du_cwd(cwd)<0) return -1;
        for (unsigned i=0;cwd[i];i++) raw[n++]=cwd[i];
        if (n) raw[n++]='\\';
    }
    if (n+slen(src)>=sizeof(raw)) return -1;
    while (*src) raw[n++]=upper(*src++);
    raw[n]=0;
    unsigned end=3,pos=0;
    while (pos<n) {
        while (raw[pos]=='\\') pos++;
        unsigned start=pos;
        while (raw[pos] && raw[pos]!='\\') pos++;
        unsigned count=pos-start;
        if (!count || (count==1 && raw[start]=='.')) continue;
        if (count==2 && raw[start]=='.' && raw[start+1]=='.') {
            while (end>3 && out[end-1]!='\\') end--;
            if (end>3) end--;
            out[end]=0; continue;
        }
        if (end>3) out[end++]='\\';
        if (end+count>=67) return -1;
        for (unsigned i=0;i<count;i++) out[end++]=raw[start+i];
        out[end]=0;
    }
    return 0;
}

/* DOS padded 8.3 wildcard template (Appendix A FINDFIRST): '*' fills the
 * rest of its component, '?' matches a padded byte too. */
static int template83(const char *s,char out[11])
{
    for (int i=0;i<11;i++) out[i]=' ';
    unsigned i=0,limit=8;
    while (*s) {
        char c=upper(*s++);
        if (c=='.' && limit==8) { i=8; limit=11; continue; }
        if (c=='*') {
            while (i<limit) out[i++]='?';
            while (*s && *s!='.') s++;
            continue;
        }
        if (i>=limit || c<33 || c==':' || c=='\\' || c=='/' || c=='+' || c=='"') return -1;
        out[i++]=c;
    }
    return 0;
}
static int matches(const char pat[11],const char *name)
{
    char raw[11];
    if (template83(name,raw)) return 0;
#ifdef DU_MUTATE_WILD
    for (int i=0;i<11;i++) if (pat[i]=='?') return 1;
#endif
    for (int i=0;i<11;i++) if (pat[i]!='?' && pat[i]!=raw[i]) return 0;
    return 1;
}
typedef struct attr_options {
    char pat[11];
    unsigned mask,bits,recursive,found,failed;
} attr_options;
static int attrib_walk(char dir[PATH_CAP],attr_options *opt)
{
    du_entry entry;
    unsigned end=slen(dir);
    /* Reissue the parent search by ordinal after descending. InitechDOS keeps
     * one FINDNEXT cursor, so saving only the DTA cannot resume a parent. No
     * fixed match-count cap, and no dependency on the child's search state. */
    for (unsigned i=0;;i++) {
        int rc=du_list(dir,i,&entry);
        if (rc<0) return -1;
        if (!rc) break;
        if (entry.attr&8 || equal(entry.name,".") || equal(entry.name,"..")) continue;
        char path[PATH_CAP];
        copy(path,dir);
        unsigned p=end;
        if (path[p-1]!='\\') path[p++]='\\';
        if (p+slen(entry.name)>=67) return -1;
        copy(path+p,entry.name);
        if (entry.attr&16) {
#ifndef DU_MUTATE_RECURSE
            if (opt->recursive && attrib_walk(path,opt)<0) return -1;
#endif
            continue;
        }
        if (!matches(opt->pat,entry.name)) continue;
        opt->found++;
        uint8_t attr=0;
        if (du_attr(path,0,&attr)<0) return -1;
        if (opt->mask) {
            attr=(uint8_t)((attr&~opt->mask)|opt->bits);
#ifdef DU_MUTATE_PRESERVE
            attr&=0x21;
#endif
            if (du_attr(path,1,&attr)<0) { error(MSG_DOS_0009); opt->failed=1; }
        } else {
            /* IBM 7-18: R in column 1, full path in column 10. MS 3.3 p.33
             * adds A, but its exact display column still needs a period
             * golden: this next-field placement is not independently proven. */
            char fields[]="         ";
            if (attr&1) fields[0]='R';
            if (attr&32) fields[3]='A';
            du_puts(fields);
#ifdef DU_MUTATE_DISPLAY
            du_puts(entry.name);
#else
            du_puts(path);
#endif
            du_puts("\r\n");
        }
    }
    return 0;
}
int attrib_main(const char *tail)
{
    attr_options opt={{0},0,0,0,0,0};
    char operand[128]={0},token[128],path[PATH_CAP];
    while (*tail) {
        while (space(*tail)) tail++;
        unsigned n=0;
        while (*tail && !space(*tail) && n<127) token[n++]=upper(*tail++);
        token[n]=0;
        if (!n) break;
        if (equal(token,"/S")) { opt.recursive=1; continue; }
        if ((token[0]=='+' || token[0]=='-') && n==2 && (token[1]=='R' || token[1]=='A')) {
            unsigned bit=token[1]=='R' ? 1u : 32u;
#ifdef DU_MUTATE_R
            if (bit==1) bit=0;
#endif
#ifdef DU_MUTATE_A
            if (bit==32) bit=0;
#endif
            opt.mask|=bit;
            if (token[0]=='+') opt.bits|=bit; else opt.bits&=~bit;
            continue;
        }
        if (operand[0] || token[0]=='/' || token[0]=='+' || token[0]=='-') {
#ifdef DU_MUTATE_SYNTAX
            return 0;
#else
            return error(MSG_DOS_0038);
#endif
        }
        copy(operand,token);
    }
    if (!operand[0]) return error(MSG_DOS_0038);
    if (fullpath(operand,path)) return error(MSG_DOS_0039);
    unsigned split=slen(path);
    while (split>3 && path[split-1]!='\\') split--;
    if (!path[split] || template83(path+split,opt.pat)) return error(MSG_DOS_0039);
    uint8_t attr=0;
    if (!du_attr(path,0,&attr) && (attr&16)) return error(MSG_DOS_0039);
    if (split>3) split--;
    path[split]=0;
    if (attrib_walk(path,&opt)<0 || !opt.found) return error(MSG_DOS_0039);
    return opt.failed ? 1 : 0;
}

static unsigned word(const uint8_t *p) { return p[0]|((unsigned)p[1]<<8); }
static void bytes(uint8_t *d,const uint8_t *s,unsigned n) { while (n--) *d++=*s++; }
typedef struct label_disk {
    uint8_t boot[512],sector[512];
    unsigned drive,root,entries;
    int label_slot,free_slot;
    char current[12];
} label_disk;
static int label_read(label_disk *d)
{
    if (du_sector(0,d->drive,0,d->boot)<0) return -1;
    d->root=word(d->boot+14)+d->boot[16]*word(d->boot+22);
    d->entries=word(d->boot+17);
    unsigned total=word(d->boot+19);
    /* DEC-15 is the DOS3.3 16-bit absolute-sector interface. No FAT32 or
     * DOS4 packet form, and no writes on malformed geometry. */
    if (word(d->boot+11)!=512 || !d->entries || !word(d->boot+14) ||
        !d->boot[16] || !word(d->boot+22) || !total ||
        d->root+(d->entries+15)/16>total) return -1;
    d->label_slot=d->free_slot=-1;
    d->current[0]=0;
    unsigned loaded=~0u;
    for (unsigned i=0;i<d->entries;i++) {
        if (loaded!=i/16) {
            loaded=i/16;
            if (du_sector(0,d->drive,d->root+loaded,d->sector)<0) return -1;
        }
        uint8_t *e=d->sector+32*(i%16);
        if (!e[0] || e[0]==0xe5) {
            if (d->free_slot<0) d->free_slot=(int)i;
            if (!e[0]) break;
        } else if (e[11]==8) { /* exact VOLLABEL, never VFAT LFN=0Fh */
            d->label_slot=(int)i;
            for (int j=0;j<11;j++) d->current[j]=(char)e[j];
            d->current[11]=0;
            break;
        }
    }
    if (d->label_slot<0 && d->boot[38]==0x29) {
        for (int i=0;i<11;i++) d->current[i]=(char)d->boot[43+i];
        d->current[11]=0;
        if (equal(d->current,"NO NAME    ")) d->current[0]=0;
    }
    unsigned n=slen(d->current);
    while (n && d->current[n-1]==' ') d->current[--n]=0;
    return 0;
}
static int label_write(label_disk *d,const char *name)
{
    unsigned n=slen(name);
    uint8_t padded[11];
    for (unsigned i=0;i<11;i++) padded[i]=i<n ? (uint8_t)name[i] : ' ';
    int slot=d->label_slot>=0 ? d->label_slot : d->free_slot;
    if (n && slot<0) return -2;
    /* Read-modify-write one root slot; never change FATs or file timestamps. */
    if (slot>=0 && (n || d->label_slot>=0)) {
        unsigned lba=d->root+(unsigned)slot/16;
        if (du_sector(0,d->drive,lba,d->sector)<0) return -1;
        uint8_t *e=d->sector+32*((unsigned)slot%16);
        if (!n) e[0]=0xe5;
        else {
            for (int i=0;i<32;i++) e[i]=0;
            bytes(e,padded,11); e[11]=8;
        }
#ifndef DU_MUTATE_ROOT
        if (du_sector(1,d->drive,lba,d->sector)<0) return -1;
#endif
    }
    /* Only extended BPBs own a label field; older DOS3.3 boot code must not
     * be overwritten. mformat/FORMAT labels already use this exact layout. */
    if (d->boot[38]==0x29) {
        bytes(d->boot+43,n ? padded : (const uint8_t *)"NO NAME    ",11);
#ifndef DU_MUTATE_BPB
        if (du_sector(1,d->drive,0,d->boot)<0) return -1;
#endif
    }
    return 0;
}
static int label_commit(label_disk *disk,const char *name)
{
    int rc=label_write(disk,name);
    if (!rc) return 0;
#ifndef DU_MUTATE_ROOM
    if (rc==-2) return error(MSG_DOS_0040);
#endif
    return error(MSG_DOS_0009);
}
int label_main(const char *tail)
{
    label_disk disk;
    char input[128],name[12];
    while (space(*tail)) tail++;
    disk.drive=(unsigned)du_drive();
    if (tail[0] && tail[1]==':') { disk.drive=(unsigned)(upper(tail[0])-'A'); tail+=2; }
    if (disk.drive!=(unsigned)du_drive()) return error(MSG_DOS_0004);
    if (label_read(&disk)<0) return error(MSG_DOS_0009);
    while (*tail==' ') tail++;
    if (!*tail) {
        char drive[]="A"; drive[0]=(char)('A'+disk.drive);
        du_puts("Volume in drive "); du_puts(drive);
        if (disk.current[0]) { du_puts(" is "); du_puts(disk.current); }
        else du_puts(" has no label");
        du_puts("\r\n");
#ifdef DU_MUTATE_PROMPT
        du_puts("Volume label? ");
#else
        du_puts("Volume label (11 characters, ENTER for none)?");
#endif
        if (du_line(input)<0) return 1;
        tail=input;
        if (!*tail) {
            if (!disk.current[0]) return 0;
            du_puts("Delete current volume label (Y/N)?");
            if (du_line(input)<0) return 1;
#ifndef DU_MUTATE_CONFIRM
            if (upper(input[0])!='Y') return 0;
#endif
            return label_commit(&disk,"");
        }
    }
    unsigned n=0;
    for (;*tail;tail++) {
        char c=upper(*tail);
#ifndef DU_MUTATE_INVALID
        const char *bad="*?/\\|.,;:+=<>[]()&^\"";
        if ((unsigned char)c<32) return error(MSG_DOS_0041);
        for (;*bad;bad++) if (*bad==c) return error(MSG_DOS_0041);
#endif
#ifdef DU_MUTATE_TRUNCATE
        if (n<10) name[n++]=c;
#else
        if (n<11) name[n++]=c;
#endif
    }
    while (n && name[n-1]==' ') n--;
    name[n]=0;
    if (!n) return error(MSG_DOS_0041);
    return label_commit(&disk,name);
}
