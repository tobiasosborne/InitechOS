/* Caller-authored complete input is the independent contract. Exercise the
 * actual QMP transport; neither product rendering nor a computed golden is used. */
#include "../emu/qemu.c"
static int failed;
static void check(int ok,const char *what)
{
    if(!ok) {fprintf(stderr,"FAIL QEMU trace: %s\n",what);failed=1;}
}
int main(void)
{
    char trace[8000];int fd[2];
    if(socketpair(AF_UNIX,SOCK_STREAM,0,fd))return 2;
    const size_t lengths[]={4095,4096,6001};
    for(unsigned i=0;i<sizeof lengths/sizeof *lengths;i++) {
        size_t n=lengths[i];int unknown=0;
        memset(trace,',',n);trace[n-1]='a';trace[n]=0;
        check(qmp_inject_keys(fd[0],trace,&unknown,NULL,NULL,NULL)==1 && !unknown,
              "complete key at 4095/4096/6001 boundary");
        memset(trace,',',n);strcpy(trace+n-4,"m1:0");
        check(qmp_inject_mouse(fd[0],trace,NULL,NULL,NULL,NULL)==1,
              "complete mouse event at 4095/4096/6001 boundary");
    }
    /* Bisect's exact impossible required suffix, with/without 4100 commas.
     * No serial fixture can print it. A dropped assertion must never pass. */
    check(qmp_inject_mouse(fd[0],"w1,WUNREACHABLE-SUFFIX-ASSERTION",NULL,NULL,NULL,NULL)<0,
          "unreachable mandatory assertion control");
    strcpy(trace,"w1,");memset(trace+3,',',4100);
    strcpy(trace+4103,",WUNREACHABLE-SUFFIX-ASSERTION");
    check(qmp_inject_mouse(fd[0],trace,NULL,NULL,NULL,NULL)<0,
          "mandatory assertion after byte 4103 was dropped");
    close(fd[0]);close(fd[1]);
    const char *bad[]={"m1:2garbage","mno:0","m2147483648:0","w1bad",
        "w30001","l2","Kbogus:0","Kctrl:2","kctrl-no-such-key","garbage","W","D","S"};
    for(unsigned i=0;i<sizeof bad/sizeof *bad;i++) {
        if(socketpair(AF_UNIX,SOCK_STREAM,0,fd))return 2;
        snprintf(trace,sizeof trace,"m1:0,%s",bad[i]);
        check(qmp_inject_mouse(fd[0],trace,NULL,NULL,NULL,NULL)<0,"invalid suffix accepted");
        char b;
        check(recv(fd[1],&b,1,MSG_DONTWAIT)<0 && (errno==EAGAIN || errno==EWOULDBLOCK),
              "invalid suffix delivered its valid prefix");
        int unknown=0;
        check(qmp_inject_keys(fd[0],"a,no-such-key",&unknown,NULL,NULL,NULL)<0,
              "invalid key suffix accepted");
        check(recv(fd[1],&b,1,MSG_DONTWAIT)<0 && (errno==EAGAIN || errno==EWOULDBLOCK),
              "invalid key suffix delivered its valid prefix");
        close(fd[0]);close(fd[1]);
    }
    if(!failed)puts("ok QEMU trace: 4095/4096/6001 bytes, required suffix retained; invalid input rejected before delivery");
    return failed;
}
