#include "samir/screen.h"
#include "samir/rt.h"
/* Fig 5-17: mode columns 0..15, drive 17..21, database 23..44,
 * record 46..64, scoreboard cells 66..71 and 73..79; separators 16/22/45/65/72.
 * The scan locates these cell boundaries, not native framebuffer RGB. */
static void put(char *row,uint32_t col,uint32_t end,const char *text)
{
    while(*text && col<end) row[col++]=*text++;
}
static uint32_t number(char *out,uint32_t n)
{
    char reverse[10];uint32_t i=0,j;
    do { reverse[i++]=(char)('0'+n%10);n/=10; } while(n);
    for(j=0;j<i;j++) out[j]=reverse[i-1-j];
    return i;
}
void samir_screen_init(samir_screen *s,samir_screen_draw draw,void *user)
{
    rt_memset(s,0,sizeof(*s)); rt_memset(s->text,' ',sizeof(s->text));
    s->draw=draw;s->user=user;
}
void samir_screen_status(samir_screen *s,int on,char drive,const char *db,uint32_t rec,uint32_t count)
{
    char value[24];uint32_t n;uint8_t attr=on?0x70:0x07;
    rt_memset(s->status,' ',80);rt_memset(s->guidance,' ',80);
    if(on) {
        put(s->status,0,16,"Command Line");
        s->status[16]=s->status[22]=s->status[45]=s->status[65]=s->status[72]='|';
        s->status[17]='(';s->status[18]=drive;s->status[19]=':';s->status[20]='>';s->status[21]=')';
        put(s->status,23,45,db);
        put(s->status,46,65,"Rec: ");
        n=number(value,rec);value[n++]='/';n+=number(value+n,count);value[n]=0;
        put(s->status,51,65,value);
        put(s->guidance,24,80,"Enter a dBASE III PLUS command.");
    }
#ifdef SAMIR_MUTATE_STATUS
    rt_memset(s->status,' ',80);rt_memset(s->guidance,' ',80);attr=0x07;
#endif
    if(on || s->active) {
        s->draw(s->user,22,0,attr,s->status,80);
        s->draw(s->user,23,0,0x07,s->guidance,80);
    }
    s->active=(uint8_t)(on!=0);
}
static void next_row(samir_screen *s)
{
    uint32_t r;
    s->col=0;
    if(++s->row<22) return;
    s->row=21;
    for(r=0;r<21;r++) rt_memcpy(s->text[r],s->text[r+1],80);
    rt_memset(s->text[21],' ',80);
    for(r=0;r<22;r++) s->draw(s->user,(uint8_t)r,0,0x07,s->text[r],80);
}
void samir_screen_write(samir_screen *s,const char *text,uint32_t len)
{
    uint32_t i=0;
    while(i<len) {
        char c=text[i];
        if(c=='\n') { next_row(s);i++; }
        else if(c=='\r') { s->col=0;i++; }
        else if(c=='\b') {
            if(s->col) s->col--; else if(s->row) { s->row--;s->col=79; }
            i++;
        } else {
            uint32_t start=i,col=s->col;
            while(i<len && s->col<80 && text[i]!='\n' && text[i]!='\r' && text[i]!='\b')
                s->text[s->row][s->col++]=text[i++];
            s->draw(s->user,s->row,(uint8_t)col,0x07,text+start,i-start);
            if(s->col==80) next_row(s);
        }
    }
}

/* SAY-only cursor/cell synchronization after the PAL paints CON.
 * Ref: ../dbase3-decomp/specs/full-screen/say-get-read.md sections 1, 2.2:
 * @SAY addresses all 25 rows and leaves the cursor after its value. The
 * row-23 golden and test_samir_say require cells beyond the 22-row viewport.
 * Using III+ U5-16/PDF128 reserves the status rows for normal output: neither
 * next_row nor samir_screen_write changes policy after this operation. */
static void say_next_row(samir_screen *s)
{
    uint32_t r;
    s->col=0;
    if(s->row<24) { s->row++;return; }
    for(r=0;r<24;r++)rt_memcpy(s->text[r],s->text[r+1],80);
    rt_memset(s->text[24],' ',80);
}
void samir_screen_at(samir_screen *s,uint8_t row,uint8_t col,const char *text,uint32_t n)
{
    uint32_t i;
    s->row=row;s->col=col;
    /* Only SAY takes the terminal's control motion here (Milton console.c
     * console_putc and ansi.c BEL); the shared dot-prompt writer stays intact.
     * This also preserves test_samir_say's embedded-control expectations. */
    for(i=0;i<n;i++) {
        char c=text[i];
        if(c=='\n')say_next_row(s);
        else if(c=='\r')s->col=0;
        else if(c=='\b') { if(s->col)s->col--; }
        else if(c=='\a') { }
        else if(c=='\f') { rt_memset(s->text,' ',sizeof s->text);s->row=s->col=0; }
        else if(c=='\t') {
            uint8_t next=(uint8_t)((s->col/8+1)*8);
            if(next>=80)say_next_row(s);else s->col=next;
        } else {
            s->text[s->row][s->col++]=c;
            if(s->col==80)say_next_row(s);
        }
    }
}
