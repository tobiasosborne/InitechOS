/* Standalone WP5 stream/property reader. No product headers or objects.
 * Format authority: real MINT01..32 and wp5_ref.py in the pinned corpus;
 * Replace authority: WP5.1 Reference pp.551-553,697-698.
 * A confirmed, length-preserving single letter change changes exactly that
 * letter; deleting an attribute token changes exactly that token. All other
 * semantic tokens must survive. Geometry in Tab/Indent and hidden bookkeeping
 * may be rewritten, and SRt/space are semantically equivalent. Plain documents
 * additionally retain their exact captured wrapping (same character widths).
 * Trial enumeration is checked here too, independently of iw_unit_len. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t source[65536],actual[65536],expected[65536],a[65536],b[65536];
static unsigned u16(const uint8_t *s){return s[0]+256u*s[1];}
static unsigned u32(const uint8_t *s){return u16(s)+65536u*u16(s+2);}
static void require(int ok,const char *why){if(!ok){fprintf(stderr,"replace-ref: %s\n",why);exit(1);}}
static unsigned unit(const uint8_t *s,unsigned n,unsigned p)
{
    static const unsigned fixed[]={4,9,11,3,3,5,6,7,4,5,6,6,8,10,10,12};
    require(p<n,"unit starts outside stream");
    unsigned c=s[p],k=1;
    if(c>=0xc0&&c<0xd0){k=fixed[c-0xc0];require(k<=n-p&&s[p+k-1]==c,"bad fixed trailer");}
    else if(c>=0xd0){
        require(n-p>=8,"short variable token");unsigned len=u16(s+p+2);
        k=0;unsigned choices[]={len,len+4,len+2};
        for(unsigned i=0;i<3;i++){
            unsigned x=choices[i];
            if(x>=8&&x<=n-p&&s[p+x-1]==c&&s[p+x-2]==s[p+1]&&u16(s+p+x-4)==len){k=x;break;}
        }
        require(k!=0,"bad mirrored variable trailer");
    }
    return k;
}
static unsigned semantic(const uint8_t *s,unsigned n,uint8_t *out)
{
    unsigned w=0;
    for(unsigned p=0;p<n;){
        unsigned k=unit(s,n,p),c=s[p];
        if(c==0xc6||(c==0xd4&&s[p+1]==0)){} /* unshown geometry */
        else if(c==0xc2){out[w++]=0xc2;}
        else if(c==0xc1&&s[p+1]==2){out[w++]=0xc1;out[w++]=2;}
        else if(c==13){out[w++]=' ';}
        else{memcpy(out+w,s+p,k);w+=k;}
        p+=k;
    }
    return w;
}
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    FILE *f=fopen(argv[1],"rb");require(f!=0,"missing source");
    unsigned n=(unsigned)fread(source,1,sizeof source,f);fclose(f);
    require(n>=16&&!memcmp(source,"\xffWPC",4),"not a WP5 document");
    unsigned start=u32(source+4);require(start>=16&&start<=n,"bad docstart");
    memmove(source,source+start,n-start);n-=start;
    int plain=1;for(unsigned p=0;p<n;){unsigned k=unit(source,n,p);if(k!=1||source[p]>=128)plain=0;p+=k;}
    f=fopen(argv[2],"rb");require(f!=0,"missing trials");
    unsigned checks=0;
    for(unsigned p=0;p<n;){
        unsigned k=unit(source,n,p),c=source[p];
        int code=c==0xc3&&(source[p+1]==12||source[p+1]==14);
        if((k==1&&c>='a'&&c<='z')||code){
            uint32_t hdr[2];require(fread(hdr,sizeof hdr,1,f)==1,"missing eligible replacement trial");
            require(hdr[0]==p&&hdr[1]<=sizeof actual,"wrong offset or output length");
            require(fread(actual,1,hdr[1],f)==hdr[1],"short trial");
            memcpy(expected,source,p);
            unsigned en=p;
            if(!code)expected[en++]=(uint8_t)(c-32);
            memcpy(expected+en,source+p+k,n-p-k);en+=n-p-k;
            unsigned an=semantic(actual,hdr[1],a),bn=semantic(expected,en,b);
            require(an==bn&&!memcmp(a,b,bn),"confirmed Replace changed unrelated text/codes or missed target");
            if(plain)require(en==hdr[1]&&!memcmp(expected,actual,en),"length-preserving Replace changed captured wrapping");
            checks++;
        }
        p+=k;
    }
    require(fgetc(f)==EOF,"unexpected extra trials");fclose(f);
    printf("replace-ref: %u independent replacement properties\n",checks);
    return 0;
}
