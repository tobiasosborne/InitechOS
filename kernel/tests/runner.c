/* Factory adapter: isolated helper aliases, then the POSIX shell oracle. */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

/* Preserve the shell adapter's complete helper set and per-case directories,
 * without spawning one ln process for every helper in every gate. */
static int link_tools(const char *source, const char *destination)
{
    DIR *dir=opendir(source);
    if(!dir) { perror(source);return 2; }
    struct dirent *entry;
    int rc=0;
    while((entry=readdir(dir))!=NULL) {
        if(strncmp(entry->d_name,"tool_",5))continue;
        size_t a=strlen(source)+strlen(entry->d_name)+2;
        size_t b=strlen(destination)+strlen(entry->d_name+5)+2;
        char *from=malloc(a),*to=malloc(b);
        if(!from || !to) { free(from);free(to);rc=2;break; }
        snprintf(from,a,"%s/%s",source,entry->d_name);
        snprintf(to,b,"%s/%s",destination,entry->d_name+5);
        struct stat info;
        if(stat(from,&info)) { perror(from);rc=2; }
        else if(S_ISREG(info.st_mode) && symlink(from,to)) { perror(to);rc=2; }
        free(from);free(to);
        if(rc)break;
    }
    if(closedir(dir))rc=2;
    return rc;
}
int main(int argc, char **argv)
{
    if(argc==4 && !strcmp(argv[1],"--link-tools"))return link_tools(argv[2],argv[3]);
    if(argc<2)return 2;
    argv[0]="sh";
    execvp("sh",argv);
    perror("sh");
    return 2;
}
