/* Capture the live SAMIR text console, without OCR or artifact changes.
 * Ref: pubs/screens/capture.sh (QMP), os/milton/console.{c,h} shadow page,
 * kmain.c g_int21_con, spec/tsr_contract.h (4002-byte save-under layout).
 * Read the matching kernel ELF32 symbols. Flat identity mapping, ADR-0001.
 * Stop QEMU around all reads; restore its original running/paused state.
 */
#define _POSIX_C_SOURCE 200809L
#include <elf.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>
#include "screen_file.h"

static int sock=-1, resume_guest, temp_created;
static char tempdir[]="/tmp/samir-screen.XXXXXX", mempath[128];
static void cleanup(void)
{
    if (sock>=0 && resume_guest) {
        const char cmd[]="{\"execute\":\"cont\",\"id\":999}\n";
        ssize_t sent=write(sock,cmd,sizeof cmd-1);
        if (sent!=(ssize_t)(sizeof cmd-1)) fputs("screen: emergency QMP resume failed\n",stderr);
    }
    if (sock>=0) close(sock);
    if (*mempath) unlink(mempath);
    if (temp_created) rmdir(tempdir);
}
static void response(char *b,size_t cap)
{
    size_t n=0;
    while (n+1<cap) {
        struct pollfd ready={sock,POLLIN,0};
        int polled;
        do { polled=poll(&ready,1,5000); } while (polled<0 && errno==EINTR);
        if (polled!=1) screen_die("QMP read timeout");
        ssize_t rc=read(sock,b+n,1);
        if (rc<0 && (errno==EINTR || errno==EAGAIN)) continue;
        if (rc!=1) screen_die("QMP read/timeout");
        if (b[n++]=='\n') { b[n]=0; return; }
    }
    screen_die("QMP response too long");
}
static void request(const char *cmd,char *reply,size_t cap)
{
    char buf[1024];
    int len=snprintf(buf,sizeof buf,"{%s,\"id\":7}\n",cmd);
    if (len<0 || (size_t)len>=sizeof buf) screen_die("QMP command too long");
    size_t sent=0;
    while (sent<(size_t)len) {
        ssize_t rc=write(sock,buf+sent,(size_t)len-sent);
        if (rc<0 && errno==EINTR) continue;
        if (rc<=0) screen_die("QMP write");
        sent+=(size_t)rc;
    }
    for (;;) {
        response(reply,cap);
        if (!strstr(reply,"\"id\"")) continue; /* asynchronous events */
        if (strstr(reply,"\"error\"")) { fputs(reply,stderr); screen_die("QMP error"); }
        if (!strstr(reply,"\"return\"")) screen_die("bad QMP response");
        return;
    }
}
static void memory(uint32_t addr,size_t size,void *out)
{
    char cmd[512],reply[8192];
    unlink(mempath);
    snprintf(cmd,sizeof cmd,"\"execute\":\"human-monitor-command\",\"arguments\":{"
             "\"command-line\":\"pmemsave 0x%x %zu \\\"%s\\\"\"}",addr,size,mempath);
    request(cmd,reply,sizeof reply);
    FILE *f=screen_open(mempath,"rb");
    if (fread(out,1,size,f)!=size || fgetc(f)!=EOF) screen_die("QMP memory size mismatch");
    fclose(f);
}
static uint32_t le32(const unsigned char *p)
{
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void symbols(const char *path,uint32_t *page,uint32_t *con)
{
    FILE *f=screen_open(path,"rb");
    if (fseek(f,0,SEEK_END)) screen_die("ELF seek");
    long size=ftell(f);
    if (size<(long)sizeof(Elf32_Ehdr) || size>64*1024*1024) screen_die("ELF size");
    rewind(f);
    unsigned char *b=malloc((size_t)size);
    if (!b || fread(b,1,(size_t)size,f)!=(size_t)size) screen_die("ELF read");
    fclose(f);
    Elf32_Ehdr eh; memcpy(&eh,b,sizeof eh);
    if (memcmp(eh.e_ident,ELFMAG,SELFMAG) || eh.e_ident[EI_CLASS]!=ELFCLASS32 ||
        eh.e_ident[EI_DATA]!=ELFDATA2LSB || eh.e_machine!=EM_386 ||
        eh.e_shentsize!=sizeof(Elf32_Shdr) ||
        (uint64_t)eh.e_shoff+(uint64_t)eh.e_shnum*sizeof(Elf32_Shdr)>(uint64_t)size)
        screen_die("need the matching unstripped i386 kernel ELF");
    *page=*con=0;
    for (unsigned i=0;i<eh.e_shnum;i++) {
        Elf32_Shdr sh,st;
        memcpy(&sh,b+eh.e_shoff+i*sizeof sh,sizeof sh);
        if (sh.sh_type!=SHT_SYMTAB) continue;
        if (sh.sh_link>=eh.e_shnum || sh.sh_entsize!=sizeof(Elf32_Sym) ||
            sh.sh_size%sizeof(Elf32_Sym) || (uint64_t)sh.sh_offset+sh.sh_size>(uint64_t)size)
            screen_die("bad ELF symbols");
        memcpy(&st,b+eh.e_shoff+sh.sh_link*sizeof st,sizeof st);
        if ((uint64_t)st.sh_offset+st.sh_size>(uint64_t)size) screen_die("bad ELF strings");
        for (unsigned j=0;j<sh.sh_size;j+=sizeof(Elf32_Sym)) {
            Elf32_Sym sym; memcpy(&sym,b+sh.sh_offset+j,sizeof sym);
            if (sym.st_name>=st.sh_size ||
                !memchr(b+st.sh_offset+sym.st_name,0,st.sh_size-sym.st_name))
                screen_die("bad ELF symbol name");
            const char *name=(char *)b+st.sh_offset+sym.st_name;
            if (!strcmp(name,"g_console_page")) *page=sym.st_value;
            if (!strcmp(name,"g_int21_con")) *con=sym.st_value;
        }
    }
    free(b);
    if (!*page || !*con) screen_die("kernel ELF lacks console symbols");
}
int main(int argc,char **argv)
{
    int inherited=argc==5 && !strcmp(argv[1],"--fd");
    if (argc!=4 && !inherited) {
        fputs("usage: screen_capture QMP-SOCKET MATCHING-KERNEL.elf OUTPUT.screen\n"
              "       screen_capture --fd HANDSHAKED-QMP-FD MATCHING-KERNEL.elf OUTPUT.screen\n",stderr); return 2;
    }
    const char *elf=argv[inherited?3:2], *output=argv[inherited?4:3];
    uint32_t page,con;
    symbols(elf,&page,&con);
    struct sockaddr_un addr; memset(&addr,0,sizeof addr); addr.sun_family=AF_UNIX;
    if (inherited) {
        char *end; long fd=strtol(argv[2],&end,10);
        if (!*argv[2] || *end || fd<0 || fd>1024) screen_die("bad inherited QMP fd");
        sock=(int)fd;
    } else {
        if (strlen(argv[1])>=sizeof addr.sun_path) screen_die("QMP socket path too long");
        strcpy(addr.sun_path,argv[1]);
        sock=socket(AF_UNIX,SOCK_STREAM,0);
        if (sock<0 || connect(sock,(struct sockaddr *)&addr,sizeof addr)) screen_die("QMP connect");
    }
    signal(SIGPIPE,SIG_IGN);
    atexit(cleanup);
    char reply[8192];
    if (!inherited) {
        response(reply,sizeof reply);
        if (!strstr(reply,"\"QMP\"")) screen_die("missing QMP greeting");
        request("\"execute\":\"qmp_capabilities\"",reply,sizeof reply);
    }
    request("\"execute\":\"query-status\"",reply,sizeof reply);
    char *running=strstr(reply,"\"running\"");
    if (!running || !(running=strchr(running,':'))) screen_die("missing QMP run state");
    do { running++; } while (*running==' ' || *running=='\t');
    if (!strncmp(running,"true",4)) {
        resume_guest=1;
        request("\"execute\":\"stop\"",reply,sizeof reply);
    } else if (strncmp(running,"false",5)) screen_die("bad QMP run state");
    if (!mkdtemp(tempdir)) screen_die("capture temporary directory");
    temp_created=1;
    snprintf(mempath,sizeof mempath,"%s/memory",tempdir);
    unsigned char ptr[4],head[44]; screen_file s;
    memory(con,4,ptr);
    if (!le32(ptr)) screen_die("console not initialized");
    memory(le32(ptr),sizeof head,head);
    /* i386 console_t prefix: lfb,pitch,bpp,width,height,bytes_per_pixel,font,
     * cols,rows,cur_col,cur_row. Fail closed on a different grid/ABI. */
    if (le32(head+28)!=80 || le32(head+32)!=25 ||
        le32(head+36)>=80 || le32(head+40)>=25) screen_die("console geometry/ABI mismatch");
    memory(page,4000,s.bytes);
    s.bytes[4000]=(unsigned char)le32(head+40);
    s.bytes[4001]=(unsigned char)le32(head+36);
    if (resume_guest) {
        request("\"execute\":\"cont\"",reply,sizeof reply);
        resume_guest=0;
    }
    screen_write(output,&s);
    puts("GREEN: live SAMIR console captured (2000 characters, 2000 attributes, cursor)");
    return 0;
}
