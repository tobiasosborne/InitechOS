/* Finder label properties. Reference: Mac OS 8.1 mint-008
 * s8_icons_floppy_window.png (one centered line below each sprite), and
 * Apple, Macintosh Human Interface Guidelines (1992), p.221 (middle
 * ellipsis preserves both ends). See README.md for the compact FLAIR grid.
 * Expected geometry is a disjoint-cell invariant, never a rendered golden.
 */
#include <stdio.h>
#include <string.h>
#include "finder_desktop.h"
#include "finder_windows.h"
#include "geneva9.h"
#include "test_assert.h"
TEST_HARNESS();
static const char *const names[] = {
    "123.EXE", "123.MNU", "CALC.EXE", "CALCULAT.EXE", "CPANELS.EXE",
    "INITBASE.EXE", "INITBASE.OVL", "LEDGER.TXT", "MICHAEL.CF",
    "NOTEPAD.EXE", "PAINT.EXE", "SCRAPBK.EXE", "TEXT.EXE", "WORD.EXE",
    "WWWWWWWW.WWW", "iiiiiiii.iii"
};
static void layout_properties(void)
{
    finder_desk_t fd;
    finder_desk_icon_t icons[16];
    static unsigned char pixels[640*1024];
    bitmap_t bm = {0};
    bm.base=pixels; bm.width=640; bm.height=1024; bm.pitch=640;
    bm.bpp=8; bm.bytes_per_pixel=1;
    for (int width=68; width<=640; width+=13) {
        finder_desk_init(&fd,icons,16,(rgn_rect_t){20,0,1024,(int16_t)width});
        for (int i=0;i<16;i++) {
            int16_t x,y;
            finder_win_grid_origin(fd.bounds,i,&x,&y);
            finder_desk_add(&fd,FINDER_ICON_FILE,names[i],x,y,1);
        }
        int disjoint=1, bounded=1, painted=1;
        for (int i=0;i<16;i++) {
            rgn_rect_t a=finder_desk_label_rect(&fd,i);
            bounded &= a.left>=0 && a.right<=width && a.right-a.left<=66;
            for (int j=0;j<i;j++)
                disjoint &= !finder_rect_intersects(a,finder_desk_label_rect(&fd,j));
            /* Isolate each label on a sentinel surface: every written label
             * pixel, including glyph overhang, must lie inside its hit band.
             * Exercise selected and unselected paint without borrowing metrics. */
            finder_desk_t one=fd;
            one.icons=&icons[i]; one.n=1;
            for (int selected=0;selected<2;selected++) {
                icons[i].selected=(uint8_t)selected;
                memset(pixels,0x7e,sizeof pixels);
                finder_desk_paint(&one,&bm,NULL);
                for (int y=a.top;y<a.bottom;y++) for (int x=0;x<640;x++)
                    if(pixels[y*640+x]!=0x7e && (x<a.left || x>=a.right)) painted=0;
            }
        }
        CHECK(disjoint,"grid labels have disjoint pixel extents");
        CHECK(bounded,"label bands stay within their 66-pixel cells and view");
        CHECK(painted,"label ink and selection never escape the hit band");
        for (int scroll=1;scroll<=200;scroll+=17) {
            for (int i=0;i<16;i++) icons[i].x-=17;
            disjoint=1;
            for (int i=0;i<16;i++) for (int j=0;j<i;j++)
                disjoint &= !finder_rect_intersects(finder_desk_label_rect(&fd,i),
                                                    finder_desk_label_rect(&fd,j));
            CHECK(disjoint,"scrolling does not pile offscreen labels at the view edge");
        }
    }
}
/* Sum the published strike data directly, without calling the formatter's
 * measuring function. This grades fitting against the font, not itself. */
static int width_of(const char *s)
{
    int width=0;
    for (;*s;s++) width+=geneva9_advance[(unsigned char)*s-32];
    return width;
}
static void spelling_properties(void)
{
    finder_desk_t fd;
    finder_desk_icon_t icon;
    char shown[FINDER_DESK_NAME_MAX+3];
    int fits=1, unchanged=1, middle=1, identity=1;
    for (int budget=0;budget<=60;budget++) for (unsigned i=0;i<16;i++) {
        int total=budget+6;
        finder_desk_init(&fd,&icon,1,(rgn_rect_t){20,0,480,(int16_t)total});
        finder_desk_add(&fd,FINDER_ICON_FILE,names[i],total/2-16,30,1);
        finder_desk_label_name(&fd,0,shown);
        fits &= width_of(shown)<=budget;
        identity &= strcmp(icon.name,names[i])==0;
        if(width_of(names[i])<=budget) unchanged &= strcmp(shown,names[i])==0;
        else if (budget<12) middle &= shown[0]=='\0';
        else {
            const char *ellipsis=strstr(shown,"...");
            if(!ellipsis) { middle=0; continue; }
            size_t prefix=(size_t)(ellipsis-shown), suffix=strlen(ellipsis+3);
            middle &= prefix+suffix<strlen(names[i]) &&
                !strncmp(shown,names[i],prefix) &&
                !strcmp(ellipsis+3,names[i]+strlen(names[i])-suffix);
            /* A 30-pixel budget always fits at least a letter at either
             * end and the 12-pixel ellipsis, even for two W glyphs. */
            if(budget>=30) middle &= prefix>0 && suffix>0;
        }
    }
    CHECK(fits,"every display spelling fits its pixel budget");
    CHECK(unchanged,"names that fit are displayed without abbreviation");
    CHECK(middle,"overflow replaces a contiguous middle with ellipsis, preserving ends");
    CHECK(identity,"display abbreviation never changes the file's real name");
}
int main(void)
{
    layout_properties();
    spelling_properties();
    return TEST_SUMMARY("test_finder_labels");
}
