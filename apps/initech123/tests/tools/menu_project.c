/* Factory generator. Independent acceptance is project.awk's RI subtraction
 * and the separate byte reader; this tool never supplies their expectations. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define NODES 512
#define LEVELS 128
struct node {char parent[512],name[128],desc[512],path[640];int level,child,ordinal,omit,keep,ns,ds;};
struct level {char path[512];int count,live,newid,first;uint32_t mask;};
static struct node nodes[NODES];static struct level levels[LEVELS];
static unsigned nn,nl,ns,nb;static char strings[32768];static unsigned offsets[1024];
static void bad(const char *message){fprintf(stderr,"menu projection: %s\n",message);exit(2);}
static void copy(char *out,unsigned cap,const char *s){if(strlen(s)>=cap)bad("field too long");strcpy(out,s);}
static int level_id(const char *path)
{
    for(unsigned i=0;i<nl;i++)if(!strcmp(levels[i].path,path))return (int)i;
    if(nl==LEVELS)bad("too many levels");
    copy(levels[nl].path,sizeof levels[nl].path,path);levels[nl].live=-1;return (int)nl++;
}
static void read_full(const char *path)
{
    FILE *f=fopen(path,"r");if(!f)bad("missing full reference");char line[2048];
    while(fgets(line,sizeof line,f)) {
        if(nn==NODES)bad("too many nodes");
        char *a=strchr(line,'\t');if(!a)bad("missing reference name");*a++=0;
        char *b=strchr(a,'\t');if(!b)bad("missing reference description");*b++=0;b[strcspn(b,"\r\n")]=0;
        struct node *n=&nodes[nn++];copy(n->parent,sizeof n->parent,line);copy(n->name,sizeof n->name,a);
        copy(n->desc,sizeof n->desc,!strcmp(b,"\"\"")?"":b);
        n->level=level_id(line);n->ordinal=levels[n->level].count++;
        int k=snprintf(n->path,sizeof n->path,"%s%s%s",line,!strcmp(line,"/")?"":"/",a);if(k<0||(unsigned)k>=sizeof n->path)bad("path too long");
        n->child=-1;
    }
    if(ferror(f)||fclose(f))bad("reference read failure");
    for(unsigned i=0;i<nn;i++)for(unsigned j=0;j<nl;j++)if(!strcmp(nodes[i].path,levels[j].path))nodes[i].child=(int)j;
    if(!nl||strcmp(levels[0].path,"/"))bad("missing root");
}
static void read_policy(const char *path,int descriptions)
{
    FILE *f=fopen(path,"r");if(!f)bad("missing reviewed policy");char line[2048];
    while(fgets(line,sizeof line,f)) {
        line[strcspn(line,"\r\n")]=0;char *name=strchr(line,'\t');if(!name)bad("invalid policy row");*name++=0;
        char *desc=strchr(name,'\t');if(desc)*desc++=0;
        unsigned found=0;
        for(unsigned i=0;i<nn;i++)if(!strcmp(nodes[i].parent,line)&&!strcmp(nodes[i].name,name)) {
            if(descriptions){if(!desc)bad("missing reviewed description");copy(nodes[i].desc,sizeof nodes[i].desc,desc);}
            else {if(nodes[i].child>=0)bad("omission is not a leaf");nodes[i].omit=1;}
            found++;
        }
        if(found!=1)bad("unknown or duplicate policy node");
    }
    if(ferror(f)||fclose(f))bad("policy read failure");
}
static int live(int level)
{
    if(levels[level].live>=0)return levels[level].live;
    if(levels[level].live==-2)bad("reference cycle");
    levels[level].live=-2;int result=0;
    for(unsigned i=0;i<nn;i++)if(nodes[i].level==level&&!nodes[i].omit) {
        if(nodes[i].child>=0){if(live(nodes[i].child))result=1;}
        else if(strcmp(nodes[i].name,"Quit"))result=1;
    }
    levels[level].live=result;return result;
}
static unsigned intern(const char *s)
{
    for(unsigned i=0;i<ns;i++)if(!strcmp(strings+offsets[i],s))return i;
    unsigned len=(unsigned)strlen(s)+1;if(ns==1024||nb+len>sizeof strings)bad("string capacity");
    offsets[ns]=nb;memcpy(strings+nb,s,len);nb+=len;return ns++;
}
static void quoted(const char *s)
{
    putchar('"');for(;*s;s++){if(*s=='"'||*s=='\\')putchar('\\');putchar(*s);}putchar('"');
}
int project_menu(const char *reference,const char *omissions,const char *descriptions)
{
    read_full(reference);read_policy(omissions,0);read_policy(descriptions,1);
    unsigned nlevels=0,nitems=0;
    for(unsigned i=0;i<nl;i++)if(live((int)i))levels[i].newid=(int)nlevels++;
    for(unsigned l=0;l<nl;l++)if(levels[l].live) {
        levels[l].first=(int)nitems;levels[l].count=0;levels[l].mask=0;
        for(unsigned i=0;i<nn;i++)if(nodes[i].level==(int)l&&!nodes[i].omit&&(nodes[i].child<0||levels[nodes[i].child].live)) {
            struct node *n=&nodes[i];n->keep=1;n->ns=(int)intern(n->name);n->ds=(int)intern(n->desc);
            if(n->ordinal>=32)bad("action mask capacity");
            levels[l].mask|=(uint32_t)1<<n->ordinal;
            levels[l].count++;nitems++;
        }
    }
    puts("/* Generated from the full RI tree and reviewed omissions; factory-only inputs. */");
    printf("#define I123_MENU_STR_BYTES %uu\n#define I123_MENU_STRING_COUNT %uu\n#define I123_MENU_ITEM_COUNT %uu\n#define I123_MENU_LEVEL_COUNT %uu\n",nb,ns,nitems,nlevels);
    puts("static const uint32_t menu_action_masks[] = {");
    for(unsigned i=0;i<nl;i++)if(levels[i].live)printf("0x%08xu, /* %s */\n",levels[i].mask,levels[i].path);
    puts("};\n#ifndef I123_MENU_DISK\nstatic const char menu_template_strings[] = {");
    for(unsigned i=0;i<nb;i++){printf("0x%02x,",(unsigned char)strings[i]);if(i%16==15)putchar('\n');}puts("\n};\nstatic const uint16_t menu_template_offsets[] = {");
    for(unsigned i=0;i<ns;i++)printf("%u,%s",offsets[i],i%16==15?"\n":"");
    puts("\n};\nstatic const i123_menu_item_t menu_template_items[] = {");
    for(unsigned i=0;i<nn;i++)if(nodes[i].keep)printf("{%d,%d,%u,%d}, /* %s */\n",nodes[i].ns,nodes[i].ds,nodes[i].child<0?65535u:(unsigned)levels[nodes[i].child].newid,nodes[i].ordinal,nodes[i].path);
    puts("};\nstatic const i123_menu_level_t menu_template_levels[] = {");
    for(unsigned i=0;i<nl;i++)if(levels[i].live)printf("{%d,%d}, /* %s */\n",levels[i].first,levels[i].count,levels[i].path);
    puts("};\n#ifdef I123_MENU_HOST\nconst char *const i123_menu_paths[] = {");
    for(unsigned i=0;i<nl;i++)if(levels[i].live){quoted(levels[i].path);puts(",");}
    puts("};\n#endif\n#endif");return ferror(stdout)?2:0;
}
#ifdef MENU_PROJECT_MAIN
int main(int argc,char **argv){return argc==4?project_menu(argv[1],argv[2],argv[3]):2;}
#endif
