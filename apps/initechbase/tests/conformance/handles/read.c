/* Independent DOS 3.30 PSP/JFT and SFT-name reader. Never links product code.
 * PSP: INT 20h signature, parent +16h, JFT count +32h/far pointer +34h.
 * DOS 3.30 SFT: refcount +0, 8.3 name +20h, owner PSP +31h, 35h bytes.
 * The MCB owner preceding a PSP and the SFT owner constrain each projection.
 * Raw one-MiB reference memory is disposable; retain only these metadata. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static unsigned char memory[1048576];
static unsigned word(unsigned at){return memory[at]+256u*memory[at+1u];}
int main(int argc,char **argv)
{
    FILE *f;unsigned process=0;
    if(argc!=2 || !(f=fopen(argv[1],"rb")))return 2;
    if(fread(memory,1,sizeof memory,f)!=sizeof memory){fclose(f);return 2;}
    fclose(f);
    for(unsigned at=16;at<0xa0000;at+=16){
        unsigned count,pos,used=0;
        if(memory[at]!=0xcd || memory[at+1u]!=0x20 || word(at-15u)!=at/16u)continue;
        count=word(at+0x32);pos=word(at+0x34)+16u*word(at+0x36);
        if(count<20 || count>255 || pos+count>sizeof memory)continue;
        for(unsigned i=0;i<count;i++)used+=memory[pos+i]!=255;
        printf("PSP %04x parent %04x JFT %u used %u\n",at/16u,word(at+0x16),count,used);
        for(unsigned i=0;i<count;i++)printf("%02x%c",memory[pos+i],i+1u==count?'\n':' ');
        if(used==count)process=at/16u;
    }
    if(!process)return 1;
    for(unsigned at=32;at+21u<process*16u;at++){
        static const char *const names[]={"DBASE   OVL","MINT    PRG","RESULT  OUT","BASE    DBF","PROBE   PRG"};
        if(word(at+17u)!=process || !word(at-32u))continue;
        for(unsigned i=0;i<sizeof names/sizeof names[0];i++)if(!memcmp(memory+at,names[i],11))
            printf("SFT name %.11s references %u owner %04x\n",memory+at,word(at-32u),process);
    }
    return 0;
}
