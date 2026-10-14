/* Independent Mac crop oracle for BOTH actual scroll-bar renderers.
 * Law 2: expected structure is the literal historical PNG transcription;
 * expected colours are an explicit authored palette substitution. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include "render.h"
#include "chrome.h"
#include "control.h"
#include "flair_look.h"
#include "scrollbar_look_golden.h"
#include "test_assert.h"
TEST_HARNESS();
static void draw(GrafPort *p)
{
    WindowScroll ws;
    memset(&ws,0,sizeof ws);
    ws.axis[0].max=100; ws.axis[1].max=100;
    ws.axis[0].drag_pos=-1; ws.axis[1].drag_pos=-1;
    /* Frame is the Finder default. V thumb (360,97), H thumb (41,260). */
    rgn_rect_t f={60,20,280,380};
    flair_draw_document_window_scroll(p,flair_look_default_skin(),f,"",1,
                                     FLAIR_WINDOW_WIDGET_ALL,&ws);
    ControlRecord ctrl;
    rgn_rect_t r={81,400,260,416};
    control_init(&ctrl,scrollBar,r,0,0,100,1,"");
    DrawControl(p,&ctrl);
}
static int grade(const render_ctx_t *r,const scroll_look_crop *c,int x,int y)
{
    int bad=0;
    for(int yy=0;yy<c->h;yy++) for(int xx=0;xx<c->w;xx++)
        bad += render_pixel_rgb(r,(unsigned)(x+xx),(unsigned)(y+yy)) !=
               sl_mapped_rgb(c->pixels[yy*c->w+xx],(int)r->boot.lfb_bpp);
    if(bad) printf("FAIL crop %s at %d,%d: %d/%d differences depth=%u\n",
                  c->name,x,y,bad,c->w*c->h,r->boot.lfb_bpp);
    return bad;
}
static int original(const char *path)
{
    png_image im; memset(&im,0,sizeof im); im.version=PNG_IMAGE_VERSION;
    if(!png_image_begin_read_from_file(&im,path)) {
        fprintf(stderr,"FAIL missing/unreadable independent PNG: %s\n",path); return 1;
    }
    im.format=PNG_FORMAT_RGB;
    unsigned char *p=malloc(PNG_IMAGE_SIZE(im));
    if(!p || !png_image_finish_read(&im,0,p,0,0)) {
        fprintf(stderr,"FAIL independent PNG decode\n"); free(p); return 1;
    }
    if(im.width!=640 || im.height!=480) {
        fprintf(stderr,"FAIL independent PNG dimensions\n");
        free(p); png_image_free(&im); return 1;
    }
    int bad=0;
    for(unsigned i=0;i<6;i++) {
        const scroll_look_crop *c=&SL_crops[i];
        for(int y=0;y<c->h;y++) for(int x=0;x<c->w;x++) {
            const unsigned char *q=p+3*((c->y+y)*im.width+c->x+x);
            unsigned rgb=q[0]*65536u+q[1]*256u+q[2];
            bad += rgb!=sl_source_rgb(c->pixels[y*c->w+x]);
        }
    }
    free(p); png_image_free(&im); return bad;
}
int main(int argc,char **argv)
{
    if(argc!=2) {fprintf(stderr,"usage: test_scrollbar_look <independent.png>\n");return 2;}
    CHECK(!original(argv[1]),"SL0 independent crops equal the historical PNG, pixel for pixel");
    int n[256]={0};
    for(const char *p=SL_thumb_v;*p;p++) n[(unsigned char)*p]++;
    CHECK(n['H']==49 && n['P']==102 && n['S']==26 && n['R']==28 && n['f']==5,
          "SL1 source shade populations 49/102/26/28/5 (Mac crop)");
    for(int bpp=8;bpp<=32;bpp+=24) {
        render_ctx_t r; render_boot_info_t b={0,0,(unsigned)bpp,640,480};
        if(render_ctx_init(&r,&b)) return 2;
        render_run(&r,draw);
        CHECK(!grade(&r,&SL_crops[0],360,97) && !grade(&r,&SL_crops[1],41,260),
              "SL2 chrome thumb ramp + four seven-pixel dark grips + highlights match crops");
        CHECK(!grade(&r,&SL_crops[2],360,82) && !grade(&r,&SL_crops[3],360,245) &&
              !grade(&r,&SL_crops[4],26,260) && !grade(&r,&SL_crops[5],345,260),
              "SL3 all four chrome arrow tiles: zero of 196 differences each");
        CHECK(!grade(&r,&SL_crops[0],401,97) && !grade(&r,&SL_crops[2],401,82) &&
              !grade(&r,&SL_crops[3],401,245),
              "SL4 Control Manager thumb and arrows match the same independent crops");
        int white=0,face=0,dark=0,cap=0,grips=0;
        for(int y=0;y<15;y++) for(int x=0;x<14;x++) {
            uint32_t rgb=render_pixel_rgb(&r,360u+(unsigned)x,97u+(unsigned)y);
            white+=rgb==0xFFFFFF; face+=rgb==0x8DDCDC;
            dark+=rgb==(bpp==8?0x3F3F3Fu:0x4E9BA3u); cap+=rgb==0xF3F3F3;
            if(x>=4 && x<=10 && y>=3 && y<=10)
                grips+=rgb==(bpp==8?0x3F3F3Fu:0x4E9BA3u);
        }
        CHECK(white==49 && face==102 && dark==54 && cap==5 && grips==28,
              "SL5 mapped shade counts 49/102/(26+28)/5; exactly28 dark grip pixels");
        render_ctx_free(&r);
    }
    return TEST_SUMMARY("test-scrollbar-look");
}
