/* Independent RFC6143 byte contract: each requested edge exactly once. */
#define main desktop_driver_main
#include "desktop-bochs.c"
#undef main
/* A slow receiver acknowledges each controller byte independently. The next
 * event must not overtake that acknowledgement, even when send() succeeded.
 * Literal set-1 bytes: IBM PC/AT Technical Reference keyboard scan codes.
 * Bochs 2.7 keyboard.cc logs READ(60) only on a nonempty keyboard buffer. */
static int acknowledged_tap(void)
{
    int fd[2];if(socketpair(AF_UNIX,SOCK_STREAM,0,fd))return 2;
    if(fcntl(fd[0],F_SETFL,O_NONBLOCK))return 2;
    char path[]="/tmp/initech-input-XXXXXX";
    int logfd=mkstemp(path);if(logfd<0)return 2;
    FILE *reader=fopen(path,"r");if(!reader)return 2;
    unlink(path);
    pid_t child=fork();if(child<0)return 2;
    if(!child) {
        close(fd[0]);fclose(reader);
        FILE *log=fdopen(logfd,"w");if(!log)_exit(2);
        const unsigned char scans[]={0x1d,0x18,0x98,0x9d};
        const unsigned keys[]={0xffe3,'o','o',0xffe3};
        for(unsigned i=0;i<sizeof scans;i++) {
            unsigned char packet[8];
            if(recv(fd[1],packet,sizeof packet,MSG_WAITALL)!=sizeof packet)_exit(2);
            if(packet[0]!=4 || packet[1]!=(i<2) || packet[2] || packet[3] ||
               be32(packet+4)!=keys[i])_exit(1);
            struct pollfd pending={.fd=fd[1],.events=POLLIN};
            if(poll(&pending,1,40)!=0) {
                fputs("FAIL desktop key: next event outran controller acknowledgement\n",stderr);
                _exit(1);
            }
            fprintf(log,"0d[KBD   ] gen_scancode(): BX_KEY_%s %s\n",
                    i==0||i==3?"CTRL_L":"O",packet[1]?"pressed":"released");
            fprintf(log,"0d[KBD   ] gen_scancode(): writing translated %02x\n",scans[i]);
            fprintf(log,"0d[KBD   ] READ(60) = %02x\n",scans[i]);
            fflush(log);
        }
        fclose(log);close(fd[1]);_exit(0);
    }
    close(fd[1]);close(logfd);
    RfbShadow rfb={.fd=fd[0],.input_log=reader,.input_deadline=mono_ms()+2000};
    BochsTraceStep step={.kind=BX_KEY,.ctrl=true};bool down=false;
    int ok=bochs_key_step(&rfb,&step,'o',&down,true)&&!down;
    int status=0;if(waitpid(child,&status,0)!=child)ok=0;
    fclose(reader);close(fd[0]);
    return ok&&WIFEXITED(status)&&WEXITSTATUS(status)==0?0:1;
}
static int rejected_stream(const char *records, unsigned key, bool down,
                            const char *error)
{
    FILE *log=tmpfile();if(!log)return 2;
    fputs(records,log);rewind(log);
    RfbShadow rfb={.fd=-1,.input_log=log,.input_deadline=mono_ms()};
    int ok=!rfb_input_wait(&rfb,key,down) && strstr(rfb.err,error);
    fclose(log);
    if(!ok)fprintf(stderr,"FAIL desktop key: expected rejection %s; got %s\n",error,rfb.err);
    return ok?0:1;
}
int main(void)
{
    int fd[2];if(socketpair(AF_UNIX,SOCK_STREAM,0,fd))return 2;
    RfbShadow rfb={.fd=fd[0]};
    BochsTraceStep step={.kind=BX_KEY,.shift=true};
    bool down=false;
    if(!bochs_key_step(&rfb,&step,'a',&down,BOCHS_TRACE_TAP_KEYS) || down) {
        puts("FAIL desktop key: tap left held across a host wait");return 1;
    }
    const unsigned char expected[]={
        4,1,0,0,0,0,255,225, 4,1,0,0,0,0,0,97,
        4,0,0,0,0,0,0,97, 4,0,0,0,0,0,255,225};
    unsigned char got[sizeof expected];size_t n=0;
    while(n<sizeof got) {ssize_t r=read(fd[1],got+n,sizeof got-n);if(r<=0)return 2;n+=(size_t)r;}
    if(memcmp(got,expected,sizeof got)) {puts("FAIL desktop key: modifier/tap wire ordering");return 1;}
    /* Ordinary Bochs callers still hold until their second replay step. */
    down=false;
    if(!bochs_key_step(&rfb,&step,'a',&down,false) || !down)return 1;
    if(!bochs_key_step(&rfb,&step,'a',&down,false) || down)return 1;
    close(fd[0]);close(fd[1]);
    if(acknowledged_tap())return 1;
    /* The sentinel observed a complete extra O press/release before 1/2/3.
     * Device records here use literal expected scan bytes, not the encoder. */
    if(rejected_stream(
        "0d[KBD   ] gen_scancode(): BX_KEY_O pressed\n"
        "0d[KBD   ] gen_scancode(): writing translated 18\n"
        "0d[KBD   ] READ(60) = 18\n"
        "0d[KBD   ] gen_scancode(): BX_KEY_O released\n"
        "0d[KBD   ] gen_scancode(): writing translated 98\n"
        "0d[KBD   ] READ(60) = 98\n",'o',true,"duplicated"))return 1;
    if(rejected_stream(
        "0d[KBD   ] gen_scancode(): BX_KEY_O released\n",'o',true,"out-of-order"))return 1;
    if(rejected_stream(
        "0d[KBD   ] gen_scancode(): BX_KEY_O pressed\n"
        "0d[KBD   ] gen_scancode(): writing translated 19\n",'o',true,"wrong"))return 1;
    if(rejected_stream(
        "0d[KBD   ] gen_scancode(): BX_KEY_O pressed\n"
        "0d[KBD   ] gen_scancode(): writing translated 18\n",'o',true,"missing"))return 1;
    FILE *log=tmpfile();if(!log)return 2;
    fputs("0d[KBD   ] gen_scancode(): BX_KEY_UP pressed\n"
          "0d[KBD   ] gen_scancode(): writing translated e0\n"
          "0d[KBD   ] gen_scancode(): writing translated 48\n"
          "0d[KBD   ] READ(60) = e0\n"
          "0d[KBD   ] READ(60) = 48\n",log);rewind(log);
    rfb=(RfbShadow){.fd=-1,.input_log=log,.input_deadline=mono_ms()};
    if(!rfb_input_wait(&rfb,0xff52,true))return 1;
    fclose(log);
    log=tmpfile();if(!log)return 2;
    fputs("0d[KBD   ] gen_scancode(): BX_KEY_O pressed\n",log);rewind(log);
    rfb=(RfbShadow){.fd=-1,.input_log=log};
    if(rfb_input_quiet(&rfb)||!strstr(rfb.err,"replayed"))return 1;
    fclose(log);
    puts("ok desktop key: exact taps, consumed-byte pacing, duplicate/missing/wrong/order rejection; two-step keys preserved");return 0;
}
