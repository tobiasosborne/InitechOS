/* Second reader of the authored v2 resource. No menu.h, product tables,
 * loader/exporter calls or product shape constants are used here. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint8_t bytes[32768];
static const char *text[1024];
static unsigned name[512],desc[512],child[512],action[512],first[128],count[128];
static char paths[128][640];
static unsigned get16(unsigned at){return bytes[at]+256u*bytes[at+1];}
static int invalid(void){fputs("invalid menu resource\n",stderr);return 1;}
int inspect_menu(const char *path)
{
    FILE *f=fopen(path,"rb");if(!f){perror(path);return 2;}
    unsigned len=(unsigned)fread(bytes,1,sizeof bytes,f);
    int extra=fgetc(f),read_error=ferror(f),close_error=fclose(f);
    if(read_error||close_error)return 2;
    if(extra!=EOF)return invalid();
    if(len<16||memcmp(bytes,"I123",4)||get16(4)!=2)return invalid();
    unsigned chars=get16(6),strings=get16(8),items=get16(10);
    if(!chars||!strings||strings>1024||!items||items>512)return invalid();
    unsigned table=12+chars,nodes=table+2*strings,levels=nodes+8*items;
    if(levels>=len-4||(len-4-levels)%4)return invalid();
    unsigned nlevels=(len-4-levels)/4;if(!nlevels||nlevels>128)return invalid();
    uint32_t checksum=2166136261u,stored=0;
    for(unsigned i=0;i<len-4;i++)checksum=(checksum^bytes[i])*16777619u;
    for(unsigned i=0;i<4;i++)stored|=(uint32_t)bytes[len-4+i]<<(8*i);
    if(checksum!=stored)return invalid();
    unsigned position=0;
    for(unsigned i=0;i<strings;i++) {
        if(get16(table+2*i)!=position||position>=chars)return invalid();
        const void *end=memchr(bytes+12+position,0,chars-position);if(!end)return invalid();
        text[i]=(const char *)bytes+12+position;position=(unsigned)((const uint8_t *)end-(bytes+12))+1;
    }
    if(position!=chars)return invalid();
    uint8_t used_string[1024]={0},used_item[512]={0},seen[128]={0};
    for(unsigned i=0;i<items;i++) {
        unsigned at=nodes+8*i;
        name[i]=get16(at);desc[i]=get16(at+2);child[i]=get16(at+4);action[i]=get16(at+6);
        if(name[i]>=strings||desc[i]>=strings||!*text[name[i]]||action[i]>=32||
           (child[i]!=65535&&child[i]>=nlevels))return invalid();
        used_string[name[i]]=used_string[desc[i]]=1;
    }
    for(unsigned i=0;i<strings;i++)if(!used_string[i])return invalid();
    for(unsigned l=0;l<nlevels;l++) {
        first[l]=get16(levels+4*l);count[l]=get16(levels+4*l+2);
        if(!count[l]||first[l]+count[l]>items)return invalid();
        uint32_t actions=0;
        for(unsigned i=first[l];i<first[l]+count[l];i++) {
            if(used_item[i]++)return invalid();
            uint32_t bit=(uint32_t)1<<action[i];if(actions&bit)return invalid();actions|=bit;
            for(unsigned j=first[l];j<i;j++)if(!strcmp(text[name[j]],text[name[i]]))return invalid();
        }
    }
    for(unsigned i=0;i<items;i++)if(!used_item[i])return invalid();
    unsigned queue[128]={0},tail=1;strcpy(paths[0],"/");seen[0]=1;
    for(unsigned head=0;head<tail;head++) {
        unsigned l=queue[head];
        for(unsigned i=first[l];i<first[l]+count[l];i++)if(child[i]!=65535) {
            unsigned c=child[i];if(seen[c]||tail==128)return invalid();seen[c]=1;queue[tail++]=c;
            char built[640];int n=snprintf(built,sizeof built,"%s%s%s",paths[l],l?"/":"",text[name[i]]);
            if(n<0||(unsigned)n>=sizeof built)return invalid();
            memcpy(paths[c],built,(unsigned)n+1);
        }
    }
    if(tail!=nlevels)return invalid();
    for(unsigned l=0;l<nlevels;l++)for(unsigned i=first[l];i<first[l]+count[l];i++)
        printf("%s\t%s\t%s\t%u\n",paths[l],text[name[i]],*text[desc[i]]?text[desc[i]]:"\"\"",action[i]);
    return ferror(stdout)?2:0;
}
