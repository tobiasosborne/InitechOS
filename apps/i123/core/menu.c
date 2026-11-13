/* THE ARTIFACT: R2.2 menu resource. Ref: 123.RI offsets in
 * spec/i123/menu_ri_offsets.tsv. Separate resource storage follows the real
 * 123.RI disk layout; the I123.MNU binary framing is authored, not a Lotus
 * file format. No timestamps or host paths. Missing/damaged data is refused. */
#include "menu.h"
#define MENU_STRINGS 14062u
#define MENU_NAMES 768u
#define MENU_ITEMS 381u
#define MENU_LEVELS 82u
static char menu_strings[MENU_STRINGS];
static uint16_t menu_offsets[MENU_NAMES];
i123_menu_item_t i123_menu_items[MENU_ITEMS];
i123_menu_level_t i123_menu_levels[MENU_LEVELS];
static uint32_t menu_nstrings;
static uint16_t rd(const uint8_t *p) { return (uint16_t)(p[0]|(p[1]<<8)); }
static uint32_t hash(const uint8_t *b,uint32_t n)
{ uint32_t h=2166136261u;for(uint32_t i=0;i<n;i++)h=(h^b[i])*16777619u;return h; }
#ifndef I123_MENU_DISK
#include "menu_data.h"
_Static_assert(sizeof menu_template_items / sizeof menu_template_items[0] == MENU_ITEMS, "menu item count");
_Static_assert(sizeof menu_template_levels / sizeof menu_template_levels[0] == MENU_LEVELS, "menu level count");
_Static_assert(sizeof menu_template_offsets / sizeof menu_template_offsets[0] <= MENU_NAMES, "menu string capacity");
_Static_assert(I123_MENU_STR_BYTES == MENU_STRINGS, "menu string bytes");
void i123_menu_init(void)
{
    for(uint32_t i=0;i<MENU_STRINGS;i++)menu_strings[i]=menu_template_strings[i];
    /* Source array is separately named in the generated data. */
    menu_nstrings=sizeof menu_template_offsets/sizeof menu_template_offsets[0];
    for(uint32_t i=0;i<menu_nstrings;i++)menu_offsets[i]=menu_template_offsets[i];
    for(uint32_t i=0;i<MENU_ITEMS;i++)i123_menu_items[i]=menu_template_items[i];
    for(uint32_t i=0;i<MENU_LEVELS;i++)i123_menu_levels[i]=menu_template_levels[i];
#ifdef I123_MUT_MENU_SPELLING
    menu_strings[menu_offsets[i123_menu_items[0].name]]='V';
#endif
}
static void wr(uint8_t *b,uint16_t n) { b[0]=(uint8_t)n;b[1]=(uint8_t)(n>>8); }
uint32_t i123_menu_export(uint8_t *b,uint32_t cap)
{
    uint32_t n=12+MENU_STRINGS+2*menu_nstrings+6*MENU_ITEMS+4*MENU_LEVELS+4;
    if(cap<n)return 0;
    b[0]='I';b[1]='1';b[2]='2';b[3]='3';wr(b+4,1);wr(b+6,MENU_STRINGS);wr(b+8,(uint16_t)menu_nstrings);wr(b+10,MENU_ITEMS);
    uint32_t p=12;
    for(uint32_t i=0;i<MENU_STRINGS;i++)b[p++]=(uint8_t)menu_strings[i];
    for(uint32_t i=0;i<menu_nstrings;i++){wr(b+p,menu_offsets[i]);p+=2;}
    for(uint32_t i=0;i<MENU_ITEMS;i++) {
        wr(b+p,i123_menu_items[i].name);wr(b+p+2,i123_menu_items[i].desc);wr(b+p+4,i123_menu_items[i].child);p+=6;
    }
    for(uint32_t i=0;i<MENU_LEVELS;i++){wr(b+p,i123_menu_levels[i].first);wr(b+p+2,i123_menu_levels[i].count);p+=4;}
    uint32_t h=hash(b,p);for(unsigned i=0;i<4;i++)b[p++]=(uint8_t)(h>>(8*i));return p;
}
#else
void i123_menu_init(void) { }
#endif
int i123_menu_load(const uint8_t *b,uint32_t n)
{
    if(n<16||b[0]!='I'||b[1]!='1'||b[2]!='2'||b[3]!='3'||rd(b+4)!=1||rd(b+6)!=MENU_STRINGS||rd(b+10)!=MENU_ITEMS)return -1;
    uint32_t ns=rd(b+8),want=12+MENU_STRINGS+2*ns+6*MENU_ITEMS+4*MENU_LEVELS+4;
    if(ns>MENU_NAMES||n!=want)return -1;
    uint32_t h=0;for(unsigned i=0;i<4;i++)h|=(uint32_t)b[n-4+i]<<(8*i);
    if(hash(b,n-4)!=h)return -1;
    uint32_t p=12;
    for(uint32_t i=0;i<MENU_STRINGS;i++)menu_strings[i]=(char)b[p++];
    if(menu_strings[MENU_STRINGS-1])return -1;
    for(uint32_t i=0;i<ns;i++){menu_offsets[i]=rd(b+p);p+=2;if(menu_offsets[i]>=MENU_STRINGS)return -1;}
    for(uint32_t i=0;i<MENU_ITEMS;i++) {
        i123_menu_item_t *it=&i123_menu_items[i];it->name=rd(b+p);it->desc=rd(b+p+2);it->child=rd(b+p+4);p+=6;
        if(it->name>=ns||it->desc>=ns||(it->child!=65535&&it->child>=MENU_LEVELS))return -1;
    }
    for(uint32_t i=0;i<MENU_LEVELS;i++) {
        i123_menu_level_t *lv=&i123_menu_levels[i];lv->first=rd(b+p);lv->count=rd(b+p+2);p+=4;
        if(!lv->count||lv->first+lv->count>MENU_ITEMS)return -1;
    }
    menu_nstrings=ns;
#ifdef I123_MUT_MENU_SPELLING
    menu_strings[menu_offsets[i123_menu_items[0].name]]='V';
#endif
    return 0;
}
const char *i123_menu_string(uint16_t id) { return menu_strings+menu_offsets[id]; }
uint32_t i123_menu_count(void) { return MENU_LEVELS; }
