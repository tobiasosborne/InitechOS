#include "ibase/lifetime.h"
#include "ibase/screen.h"
#include "ibase/rt.h"
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
void ibase_screen_init(ibase_screen *s,ibase_screen_draw draw,void *user)
{
    IBASE_RETAIN(s); IBASE_RETAIN(draw); IBASE_RETAIN(user);
    rt_memset(s,0,sizeof(*s)); rt_memset(s->text,' ',sizeof(s->text));
    s->draw=draw;s->user=user;
    s->scoreboard=1;s->attr=7;
}
void ibase_screen_status(ibase_screen *s,int on,char drive,const char *db,uint32_t rec,uint32_t count)
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
    if(on || s->active) {
        s->draw(s->user,22,0,attr,s->status,80);
        s->draw(s->user,23,0,0x07,s->guidance,80);
    }
    s->active=(uint8_t)(on!=0);
}
static void scroll(ibase_screen *s,uint8_t bottom)
{
    uint32_t r,top=s->scoreboard?1u:0u;
    for(r=top;r<bottom;r++) rt_memcpy(s->text[r],s->text[r+1],80);
    rt_memset(s->text[bottom],' ',80);
    if(s->scroll) s->scroll(s->user,(uint8_t)top,bottom);
    else if(s->draw) for(r=top;r<=bottom;r++) s->draw(s->user,(uint8_t)r,0,0x07,s->text[r],80);
}
static void next_row(ibase_screen *s)
{
    uint8_t bottom=s->active?21:24;
    s->col=0;
    if(++s->row<=bottom) return;
    s->row=bottom;
    scroll(s,bottom);
}
/* Real III+ at-dot d1..d8, at-dot-c e1/e4/e5/e7 and at-to-b TEd.
 * Enter has only returned the cursor to column zero; only this prompt scrolls.
 * CLEAR's scoreboard cleanup is deferred until here, once per CLEAR. */
void ibase_screen_prompt(ibase_screen *s,int scoreboard,int cleared)
{
    uint32_t n=79;
    if(s->scoreboard!=(uint8_t)scoreboard) {
        uint32_t r;
        rt_memset(s->text,' ',sizeof s->text);
        if(s->draw) for(r=0;r<25;r++) s->draw(s->user,(uint8_t)r,0,0x07,s->text[r],80);
        s->row=s->col=0;
    }
    s->scoreboard=(uint8_t)scoreboard;
    if(cleared && scoreboard) {
        rt_memset(s->text[0]+39,' ',41);
        if(s->draw) s->draw(s->user,0,39,0x07,s->text[0]+39,41);
    }
    if(s->active) return;
    if(s->row>=24) { s->row=24;next_row(s); }
    else {
        rt_memset(s->text[24],' ',n);
        if(s->draw) s->draw(s->user,24,0,0x07,s->text[24],n);
    }
    s->row=24;s->col=0;
}
void ibase_screen_write(ibase_screen *s,const char *text,uint32_t len)
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
            if(s->row>24) next_row(s);
            /* next_row also resets col: bound the native draw AFTER it. */
            col=s->col;
            while(i<len && s->col<80 && text[i]!='\n' && text[i]!='\r' && text[i]!='\b')
                s->text[s->row][s->col++]=text[i++];
            s->draw(s->user,s->row,(uint8_t)col,s->attr,text+start,i-start);
            if(s->col==80) next_row(s);
        }
    }
}

/* SAY paints through the PAL without terminal wrap.
 * Ref: ../dbase3-decomp/specs/full-screen/say-get-read.md sections 1, 2.2:
 * @SAY addresses all 25 rows and leaves the cursor after its value. The
 * row-23 golden and test_ibase_say require cells beyond the 22-row viewport.
 * Using III+ U5-16/PDF128 reserves the status rows for normal output: neither
 * next_row nor ibase_screen_write changes policy after this operation. */
static void say_next_row(ibase_screen *s)
{
    s->col=0;
    /* Real III+ at-say-edge/edge: bottom overflow advances the BIOS
     * cursor to (25,3), but neither paints hidden bytes nor scrolls. */
    if(s->row<25) s->row++;
}
void ibase_screen_at(ibase_screen *s,uint8_t row,uint8_t col,const char *text,uint32_t n)
{
    uint32_t i;
    s->row=row;s->col=col;
    /* Real III+ at-say-edge-controls/controls: CR/LF are literal cells,
     * just like printable bytes. Positioned output never scrolls the page. */
    for(i=0;i<n;i++) {
        {
            uint32_t start=i,col=s->col;
            while(i<n && s->col<80) {
                if(s->row<25) s->text[s->row][s->col]=text[i];
                s->col++;i++;
            }
            if(s->draw && s->row<25) s->draw(s->user,s->row,(uint8_t)col,s->attr,text+start,i-start);
            i--;
            if(s->col==80)say_next_row(s);
        }
    }
}
