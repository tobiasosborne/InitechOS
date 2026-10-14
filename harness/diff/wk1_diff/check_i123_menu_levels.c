/* Factory C: expected entry screens come only from the RI-checked golden,
 * not artifact menu tables. Locked traces and their visit order are separate
 * spec data. initech-uqah. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {char path[256],items[512],desc[512];} level_t;
static level_t levels[128];static unsigned count;
static void panel(char *out,const char *s)
{ unsigned n=0;while(s[n]&&n<77){out[n]=s[n];n++;}while(n<77)out[n++]=' ';out[n]=0; }
int main(int argc,char **argv)
{
    if(argc!=5)return 2;
    FILE *f=fopen(argv[2],"r");if(!f)return 2;
    char line[1024];
    while(fgets(line,sizeof line,f)) {
        char *a=strchr(line,'\t');if(!a)return 2;*a++=0;
        char *b=strchr(a,'\t');if(!b)return 2;*b++=0;b[strcspn(b,"\n")]=0;
        unsigned k;for(k=0;k<count;k++)if(!strcmp(levels[k].path,line))break;
        if(k==count) {if(count==128)return 2;strcpy(levels[k].path,line);strcpy(levels[k].desc,!strcmp(b,"\"\"")?"":b);count++;}
        if(levels[k].items[0])strcat(levels[k].items,"  ");
        if(strlen(levels[k].items)+strlen(a)>=sizeof levels[k].items)return 2;
        strcat(levels[k].items,a);
    }
    fclose(f);f=fopen(argv[3],"r");FILE *serial=fopen(argv[1],"r");if(!f||!serial)return 2;
    unsigned fragment=(unsigned)atoi(argv[4]),id,visits=0;char path[256];unsigned char seen[128]={0};
    while(fscanf(f,"%u %255s",&id,path)==2) {
        unsigned k;for(k=0;k<count;k++)if(!strcmp(levels[k].path,path))break;
        if(k==count)return 2;
        seen[k]=1;if(id!=fragment)continue;
        char expected[512],text[78];int found=0;
        panel(text,levels[k].items);snprintf(expected,sizeof expected,"x=1 y=16 s=\"%s\" fg=1 bg=0 -> 0",text);
        while(fgets(line,sizeof line,serial))if(strstr(line,expected)){found=1;break;}
        if(!found){fprintf(stderr,"missing menu entry: %s\n",path);return 1;}
        panel(text,levels[k].desc);snprintf(expected,sizeof expected,"x=1 y=32 s=\"%s\" fg=1 bg=0 -> 0",text);found=0;
        while(fgets(line,sizeof line,serial))if(strstr(line,expected)){found=1;break;}
        if(!found){fprintf(stderr,"missing first description: %s\n",path);return 1;}
        visits++;
    }
    fclose(f);fclose(serial);
    for(unsigned k=0;k<count;k++)if(!seen[k]){fprintf(stderr,"unvisited menu: %s\n",levels[k].path);return 1;}
    if(!visits)return 2;
    printf("i123-menu-levels: fragment %u, %u menu entry screens verified\n",fragment,visits);return 0;
}
