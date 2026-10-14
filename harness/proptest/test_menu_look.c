/* tdnl.93/.94/.95/.70: independent captured anatomy + original NFNT check.
 * Charcoal and the command glyph remain the tdnl.69 decision; grade geometry
 * around Chicago, never compare those two substituted letter strikes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include "render.h"
#include "menu.h"
#include "menu_look_golden.h"
#include "test_assert.h"
TEST_HARNESS();
static MenuItem items[]={{"New Folder",0,'N',0,1,0},{"by Icons",18,0,0,1,0}};
static MenuInfo menus[]={{512,"File",items,2,0}};
static MenuBar bar={menus,1,1};
static void draw(GrafPort *p){HiliteMenu(p,&bar,0,NULL);flair_draw_menu_panel(p,&bar,0,-1,NULL);}
static int be(const unsigned char *b,int p){return b[p]*256+b[p+1];}
static int original(const char *png,const char *nfnt)
{
    png_image im;memset(&im,0,sizeof im);im.version=PNG_IMAGE_VERSION;
    if(!png_image_begin_read_from_file(&im,png))return 1;
    im.format=PNG_FORMAT_RGB;unsigned char *p=malloc(PNG_IMAGE_SIZE(im));
    if(!p || !png_image_finish_read(&im,0,p,0,0)){free(p);return 1;}
    int bad=im.width!=640 || im.height!=480;
    if(!bad) for(int y=0;y<20;y++) {
        const unsigned char *q=p+3*(y*640+34);
        bad+=(unsigned)(q[0]*65536+q[1]*256+q[2])!=ML_source_band[y];
    }
    /* Independent geometry crops: first New Folder ink x53/y23; last
     * shortcut ink216; right frame230. Never query the renderer for them. */
    if(!bad) {
        int l=640,t=480,r=-1;
        for(int y=23;y<32;y++)for(int x=35;x<229;x++) {
            const unsigned char *q=p+3*(y*640+x);
            if(q[0]==0 && q[1]==0 && q[2]==0) {
                if(x<l)l=x;
                if(x>r)r=x;
                if(y<t)t=y;
            }
        }
        bad+=l!=53 || t!=23 || r!=216;
        /* Historical pointer occupies x55..61 on rows21/22. The first
         * cap column53 is clear on22; crop only the documented cap band. */
        const unsigned char *above=p+3*(22*640+53);
        bad+=above[0]!=231 || above[1]!=231 || above[2]!=231;
        for(int y=21;y<35;y++) {
            const unsigned char *q=p+3*(y*640+230);
            bad+=q[0]!=0 || q[1]!=0 || q[2]!=0;
        }
    }
    free(p);png_image_free(&im);
    unsigned char b[8192];FILE *f=fopen(nfnt,"rb");if(!f)return 1;
    size_t n=fread(b,1,sizeof b,f);fclose(f);if(n!=3098)return 1;
    int h=be(b,14),rw=be(b,24),loc=26+2*rw*h,ow=16+2*be(b,16);
    if(h!=15 || rw!=70 || loc!=2126 || ow!=2612)return 1;
    int start=be(b,loc+36),end=be(b,loc+38);
    bad+=b[ow+36]!=1 || b[ow+37]!=11 || end-start!=9;
    for(int y=0;y<16;y++)for(int x=0;x<11;x++) {
        int k=start+x-1;
        int ink=y<h && x>=1 && k<end && (b[26+y*2*rw+k/8]&(128>>(k%8)));
        bad+=!!ink!=!!(ML_check[y]&(0x8000u>>x));
    }
    start=be(b,loc+140);end=be(b,loc+142);
    bad+=b[ow+140]!=1 || b[ow+141]!=7;
    for(int y=0;y<9;y++)for(int x=0;x<7;x++) {
        int k=start+x-1;
        int ink=x>=1 && k<end && (b[26+(3+y)*2*rw+k/8]&(128>>(k%8)));
        bad+=!!ink!=!!(ML_file_F[y]&(0x8000u>>x));
    }
    return bad;
}
int main(int argc,char **argv)
{
    if(argc!=3){fprintf(stderr,"usage: test_menu_look historical.png NFNT.bin\n");return 2;}
    CHECK(!original(argv[1],argv[2]),"ML0 crops equal historical PNG and NFNT bytes (missing corpus FAILS)");
    for(int bpp=8;bpp<=32;bpp+=24) {
        render_ctx_t r;render_boot_info_t b={0,0,(unsigned)bpp,640,480};
        if(render_ctx_init(&r,&b))return 2;
        render_run(&r,draw);
        int bad=0;for(int y=0;y<20;y++)bad+=render_pixel_rgb(&r,34,y)!=ml_band_rgb(y,bpp);
        CHECK(!bad,"ML1 title band matches independent four-class crop");
        bad=0;for(int y=0;y<9;y++)for(int x=0;x<7;x++) {
            uint32_t want=(ML_file_F[y]&(0x8000u>>x))?0xFFFFFF:ml_band_rgb(y+5,bpp);
            bad+=render_pixel_rgb(&r,42+x,5+y)!=want;
        }
        CHECK(!bad,"ML9 white File F equals independent NFNT crop");
        rgn_rect_t panel=MenuInfo_panel_rect(&bar,0);
        CHECK(panel.left==33 && panel.top==19,"ML2 panel origin33/19 (capture)");
        int lx=640,ty=480,rx=-1;
        for(int y=21;y<36;y++)for(int x=panel.left+2;x<panel.right-2;x++)
            if(render_pixel_rgb(&r,x,y)==0){if(x<lx)lx=x;if(y<ty)ty=y;if(x>rx)rx=x;}
        CHECK(lx-panel.left==20 && ty-panel.top==4,"ML3 item ink offset20/4 (x53-33,y23-19)");
        CHECK(panel.right-1-rx==14,"ML4 shortcut ink-to-frame14 (x230-216)");
        bad=0;int count=0;
        for(int y=0;y<16;y++)for(int x=0;x<11;x++) {
            int ink=render_pixel_rgb(&r,panel.left+4+x,36+y)==0;
            count+=ink;bad+=ink!=!!(ML_check[y]&(0x8000u>>x));
        }
        CHECK(!bad && count==18,"ML5 mark equals NFNT check:9x8,18 pixels,advance11");
        /* Idle title and Apple placement, then repeat with an effective clip.
         * Compare a clipped draw to the complete draw, including unchanged
         * outside pixels; this detects text that bypasses the clip. */
        DrawMenuBar(&r.port,&bar,-1,NULL);lx=640;
        for(int y=2;y<18;y++)for(int x=33;x<74;x++)
            if(render_pixel_rgb(&r,x,y)==0 && x<lx)lx=x;
        CHECK(lx==43,"ML6 File first ink43 (capture)");
        lx=640;for(int y=2;y<18;y++)for(int x=10;x<33;x++)
            if(render_pixel_rgb(&r,x,y)==0 && x<lx)lx=x;
        CHECK(lx==17,"ML7 retained Apple first ink17 (capture)");
        render_ctx_t c;if(render_ctx_init(&c,&b))return 2;
        region_t *clip=&c.clipRgn;
        rgn_rect_t cr={7,45,50,80};region_set_rect(clip,cr);
        draw(&r.port);HiliteMenu(&c.port,&bar,0,clip);flair_draw_menu_panel(&c.port,&bar,0,-1,clip);
        bad=0;for(int y=0;y<80;y++)for(int x=0;x<200;x++) {
            uint32_t want=(x>=45&&x<80&&y>=7&&y<50)?render_pixel_rgb(&r,x,y):0x8DDCDC;
            bad+=render_pixel_rgb(&c,x,y)!=want;
        }
        CHECK(!bad,"ML8 band, title, item and check respect effective clip");
        render_ctx_free(&c);render_ctx_free(&r);
    }
    return TEST_SUMMARY("test-menu-look");
}
