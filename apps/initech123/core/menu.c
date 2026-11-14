/* Shipped R2.2 sub-tree. Full RI and reviewed omissions remain factory inputs.
 * Version 2 appends the original sibling/action identity to each item. This
 * authored framing is documented in menu.h, not a Lotus file format. */
#include "menu.h"
#include "menu_data.h"
#define MENU_STRINGS I123_MENU_STR_BYTES
#define MENU_NAMES I123_MENU_STRING_COUNT
#define MENU_ITEMS I123_MENU_ITEM_COUNT
#define MENU_LEVELS I123_MENU_LEVEL_COUNT
static char menu_strings[MENU_STRINGS];
static uint16_t menu_offsets[MENU_NAMES];
i123_menu_item_t i123_menu_items[MENU_ITEMS];
i123_menu_level_t i123_menu_levels[MENU_LEVELS];
static uint16_t rd(const uint8_t *p){return (uint16_t)(p[0]|(p[1]<<8));}
static uint32_t hash(const uint8_t *b,uint32_t n)
{uint32_t h=2166136261u;for(uint32_t i=0;i<n;i++)h=(h^b[i])*16777619u;return h;}
static int equal(const char *a,const char *b)
{while(*a&&*a==*b){a++;b++;}return *a==*b;}
#ifndef I123_MENU_DISK
_Static_assert(sizeof menu_template_items/sizeof menu_template_items[0]==MENU_ITEMS,"menu items");
_Static_assert(sizeof menu_template_levels/sizeof menu_template_levels[0]==MENU_LEVELS,"menu levels");
_Static_assert(sizeof menu_template_offsets/sizeof menu_template_offsets[0]==MENU_NAMES,"menu strings");
_Static_assert(sizeof menu_template_strings==MENU_STRINGS,"menu string bytes");
void i123_menu_init(void)
{
    for(uint32_t i=0;i<MENU_STRINGS;i++)menu_strings[i]=menu_template_strings[i];
    for(uint32_t i=0;i<MENU_NAMES;i++)menu_offsets[i]=menu_template_offsets[i];
    for(uint32_t i=0;i<MENU_ITEMS;i++)i123_menu_items[i]=menu_template_items[i];
    for(uint32_t i=0;i<MENU_LEVELS;i++)i123_menu_levels[i]=menu_template_levels[i];
}
static void wr(uint8_t *p,uint16_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
uint32_t i123_menu_export(uint8_t *b,uint32_t cap)
{
    uint32_t n=12+MENU_STRINGS+2*MENU_NAMES+8*MENU_ITEMS+4*MENU_LEVELS+4;
    if(cap<n)return 0;
    b[0]='I';b[1]='1';b[2]='2';b[3]='3';wr(b+4,2);wr(b+6,MENU_STRINGS);wr(b+8,MENU_NAMES);wr(b+10,MENU_ITEMS);
    uint32_t p=12;
    for(uint32_t i=0;i<MENU_STRINGS;i++)b[p++]=(uint8_t)menu_strings[i];
    for(uint32_t i=0;i<MENU_NAMES;i++){wr(b+p,menu_offsets[i]);p+=2;}
    for(uint32_t i=0;i<MENU_ITEMS;i++) {
        const i123_menu_item_t *it=&i123_menu_items[i];
        wr(b+p,it->name);wr(b+p+2,it->desc);wr(b+p+4,it->child);wr(b+p+6,it->action);p+=8;
    }
    for(uint32_t i=0;i<MENU_LEVELS;i++){wr(b+p,i123_menu_levels[i].first);wr(b+p+2,i123_menu_levels[i].count);p+=4;}
    uint32_t h=hash(b,p);for(unsigned i=0;i<4;i++)b[p++]=(uint8_t)(h>>(8*i));return p;
}
#else
void i123_menu_init(void){}
#endif
int i123_menu_load(const uint8_t *b,uint32_t n)
{
    uint32_t want=12+MENU_STRINGS+2*MENU_NAMES+8*MENU_ITEMS+4*MENU_LEVELS+4;
    if(n!=want||b[0]!='I'||b[1]!='1'||b[2]!='2'||b[3]!='3'||rd(b+4)!=2||
       rd(b+6)!=MENU_STRINGS||rd(b+8)!=MENU_NAMES||rd(b+10)!=MENU_ITEMS)return -1;
    uint32_t h=0;for(unsigned i=0;i<4;i++)h|=(uint32_t)b[n-4+i]<<(8*i);
    if(hash(b,n-4)!=h)return -1;
    uint32_t p=12;
    for(uint32_t i=0;i<MENU_STRINGS;i++)menu_strings[i]=(char)b[p++];
    uint32_t end=0;
    for(uint32_t i=0;i<MENU_NAMES;i++) {
        menu_offsets[i]=rd(b+p);p+=2;
        if(menu_offsets[i]!=end)return -1;
        while(end<MENU_STRINGS&&menu_strings[end])end++;
        if(end==MENU_STRINGS)return -1;
        end++;
    }
    if(end!=MENU_STRINGS)return -1;
    uint8_t used[MENU_NAMES]={0},parents[MENU_LEVELS]={0},seen[MENU_LEVELS]={0};
    for(uint32_t i=0;i<MENU_ITEMS;i++) {
        i123_menu_item_t *it=&i123_menu_items[i];
        it->name=rd(b+p);it->desc=rd(b+p+2);it->child=rd(b+p+4);it->action=rd(b+p+6);p+=8;
        if(it->name>=MENU_NAMES||it->desc>=MENU_NAMES||!menu_strings[menu_offsets[it->name]]||it->action>=32)return -1;
        used[it->name]=used[it->desc]=1;
        if(it->child!=65535) {
            if(!it->child||it->child>=MENU_LEVELS||++parents[it->child]!=1)return -1;
        }
    }
    for(uint32_t i=0;i<MENU_NAMES;i++)if(!used[i])return -1;
    uint32_t covered=0;
    for(uint32_t i=0;i<MENU_LEVELS;i++) {
        i123_menu_level_t *lv=&i123_menu_levels[i];lv->first=rd(b+p);lv->count=rd(b+p+2);p+=4;
        if(!lv->count||lv->first!=covered||lv->first+lv->count>MENU_ITEMS)return -1;
        covered+=lv->count;uint32_t actions=0;
        for(uint32_t j=0;j<lv->count;j++) {
            const i123_menu_item_t *it=&i123_menu_items[lv->first+j];uint32_t bit=(uint32_t)1<<it->action;
            if(!(menu_action_masks[i]&bit)||(actions&bit))return -1;
            actions|=bit;
            for(uint32_t k=0;k<j;k++)if(equal(i123_menu_string(it->name),i123_menu_string(i123_menu_items[lv->first+k].name)))return -1;
        }
        if(actions!=menu_action_masks[i])return -1;
    }
    if(covered!=MENU_ITEMS)return -1;
    for(uint32_t i=1;i<MENU_LEVELS;i++)if(parents[i]!=1)return -1;
    seen[0]=1;
    for(uint32_t pass=0;pass<MENU_LEVELS;pass++)for(uint32_t i=0;i<MENU_LEVELS;i++)if(seen[i]) {
        const i123_menu_level_t *lv=&i123_menu_levels[i];
        for(uint32_t j=0;j<lv->count;j++){uint16_t child=i123_menu_items[lv->first+j].child;if(child!=65535)seen[child]=1;}
    }
    for(uint32_t i=0;i<MENU_LEVELS;i++)if(!seen[i])return -1;
    return 0;
}
const char *i123_menu_string(uint16_t id){return menu_strings+menu_offsets[id];}
uint32_t i123_menu_count(void){return MENU_LEVELS;}
