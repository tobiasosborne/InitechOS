/* Native execution of the artifact through a DOS-I/O seam; NOT the oracle.
 * The independent model is model.c. Ref: ADR-0003 Appendix A. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "edlin.h"
static int err(int r) { return r < 0 ? -(errno == ENOENT ? 2 : 5) : r; }
static const char *path(const char *p){static char b[128];unsigned n=0;if(p[0]&&p[1]==':')p+=2;while(*p=='\\')p++;while(*p&&n<127){b[n++]=*p=='\\'?'/':*p;p++;}b[n]=0;return b;}
static int op(const char *p) { return err(open(path(p),O_RDONLY)); }
static int cr(const char *p) { return err(open(path(p),O_CREAT|O_TRUNC|O_WRONLY,0600)); }
static int rd(int h,void *p,unsigned n) { return err((int)read(h,p,n)); }
static int wr(int h,const void *p,unsigned n) { return err((int)write(h,p,n)); }
static int cl(int h) { return err(close(h)); }
static int rmfile(const char *p) { struct stat s;p=path(p);if(!stat(p,&s)&&!(s.st_mode&0200))return -5;return err(unlink(p)); }
static int rn(const char *a,const char *b) { char c[128];snprintf(c,sizeof c,"%s",path(a));return err(rename(c,path(b))); }
static int at(const char *p) { struct stat s; return stat(path(p),&s) ? -2 : (s.st_mode&0200 ? 0:1); }
static int gc(void) { int c=getchar(); if(c==EOF) { fputs("RED EDLIN-input exhausted\n",stderr); exit(1); } return c; }
#ifdef EDLIN_ABSENT
int edlin_run(const edlin_io *io,const char *p) { (void)io;(void)p; fputs("RED EDLIN implementation missing\n",stderr);return 1; }
#endif
int main(int argc,char **argv) {
    edlin_io io={gc,op,cr,rd,wr,cl,rmfile,rn,at};
    return argc==2 ? edlin_run(&io,argv[1]) : 2;
}
