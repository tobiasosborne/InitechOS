/* Independent filesystem oracle for the adapter: every executable helper gets
 * its exact basename alias; directories and unrelated files do not. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "test_assert.h"
TEST_HARNESS();
static void file(const char *path) { FILE *f=fopen(path,"w");if(!f || fclose(f))exit(2); }
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    char root[]="/tmp/initech-adapter.XXXXXX",src[256],dst[256],a[280],b[280],link[280],got[512];
    if(!mkdtemp(root))return 2;
    snprintf(src,sizeof src,"%s/source",root);snprintf(dst,sizeof dst,"%s/destination",root);
    if(mkdir(src,0700) || mkdir(dst,0700))return 2;
    snprintf(a,sizeof a,"%s/tool_first",src);snprintf(b,sizeof b,"%s/tool_last",src);file(a);file(b);
    snprintf(link,sizeof link,"%s/tool_directory",src);if(mkdir(link,0700))return 2;
    snprintf(link,sizeof link,"%s/unrelated",src);file(link);
    pid_t pid=fork();if(pid<0)return 2;
    if(!pid) { execl(argv[1],argv[1],"--link-tools",src,dst,(char *)0);_exit(2); }
    int status;if(waitpid(pid,&status,0)!=pid)return 2;
    CHECK(WIFEXITED(status) && WEXITSTATUS(status)==0,"helper setup completes normally");
    snprintf(link,sizeof link,"%s/first",dst);ssize_t n=readlink(link,got,sizeof got-1);
    CHECK(n>0 && (got[n]=0,!strcmp(got,a)),"first helper aliases exact independent source path");
    unlink(link);
    snprintf(link,sizeof link,"%s/last",dst);n=readlink(link,got,sizeof got-1);
    CHECK(n>0 && (got[n]=0,!strcmp(got,b)),"last helper aliases exact independent source path");
    unlink(link);
    snprintf(link,sizeof link,"%s/directory",dst);CHECK(access(link,F_OK)!=0,"build directories are not helper aliases");
    snprintf(link,sizeof link,"%s/unrelated",dst);CHECK(access(link,F_OK)!=0,"nonhelper files are not aliases");
    unlink(a);unlink(b);snprintf(link,sizeof link,"%s/tool_directory",src);rmdir(link);
    snprintf(link,sizeof link,"%s/unrelated",src);unlink(link);rmdir(src);rmdir(dst);rmdir(root);
    return TEST_SUMMARY("runner-setup");
}
