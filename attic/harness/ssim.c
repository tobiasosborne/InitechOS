/* Factory-only SSIM guide, initech-w06 (PRD Sec 3/8/14; Law 3/4).
 * Wang, Bovik, Sheikh, Simoncelli, "Image Quality Assessment: From Error
 * Visibility to Structural Similarity", IEEE TIP 13(4), 2004, pp.600-612,
 * DOI 10.1109/TIP.2003.819861; eq.13 p.605, eqs.14-17 p.606.
 * Local mathematical source: docs/research/ssim-wang2004.md.
 * Independent C implementation of those equations, not a MATLAB port.
 * 11x11 Gaussian sigma1.5, population moments, K1=.01 K2=.03 L=255.
 * Valid windows only; no padding/resizing/downsampling/gamma adjustment.
 * RGB -> full-range BT.601 luma. Reports mean + local-map summary.
 * Historical SSIM NEVER gates: errors mean invalid input, not a low score.
 * Codec patterns: tools/ppm_flair_grow_box_check.c, harness/render/.
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ssim_params.h"

#define MAX_PIXELS (16u * 1024u * 1024u)
typedef struct { unsigned w, h; double *luma; } Image;
typedef struct { unsigned x,y,w,h; } Rect;
typedef struct {
    double mean, min, max, sd;
    unsigned w,h, worst_x,worst_y;
} Summary;

static int error(const char *why, const char *path)
{
    fprintf(stderr,"ssim: %s%s%s\n",why,path ? ": " : "",path ? path : "");
    return 0;
}

/* Consume exactly one separator after a token, not arbitrary whitespace:
 * a P6 raster may itself start with a space, LF, or '#'. */
static int token(FILE *f, char *s, size_t cap)
{
    int c; size_t n=0;
    do {
        c=fgetc(f);
        if(c=='#') { while(c!=EOF && c!='\n') c=fgetc(f); }
    } while(c!=EOF && isspace((unsigned char)c));
    if(c==EOF) return 0;
    do {
        if(n+1>=cap) return 0;
        s[n++]=(char)c; c=fgetc(f);
    } while(c!=EOF && !isspace((unsigned char)c));
    if(c=='\r') { c=fgetc(f); if(c!=EOF && c!='\n') ungetc(c,f); }
    s[n]=0; return 1;
}

static int number(const char *s, unsigned *out)
{
    char *end; unsigned long v;
    if(!*s) return 0;
    for(const char *p=s;*p;p++) if(!isdigit((unsigned char)*p)) return 0;
    errno=0; v=strtoul(s,&end,10);
    if(errno || *end || v>UINT_MAX) return 0;
    *out=(unsigned)v; return 1;
}

static int ppm_number(FILE *f,unsigned *out)
{
    char s[32]; return token(f,s,sizeof s) && number(s,out);
}

static unsigned char *read_ppm(FILE *f, unsigned *w, unsigned *h)
{
    char magic[4]; unsigned max;
    if(!token(f,magic,sizeof magic) || (strcmp(magic,"P6") && strcmp(magic,"P3")) ||
       !ppm_number(f,w) || !ppm_number(f,h) || !ppm_number(f,&max) ||
       !*w || !*h || max!=255 || *w>MAX_PIXELS / *h) return NULL;
    size_t n=(size_t)*w * *h * 3;
    unsigned char *rgb=malloc(n);
    if(!rgb) return NULL;
    if(!strcmp(magic,"P6")) {
        if(fread(rgb,1,n,f)!=n) {free(rgb);return NULL;}
    } else {
        for(size_t i=0;i<n;i++) {
            unsigned v;
            if(!ppm_number(f,&v) || v>255) {free(rgb);return NULL;}
            rgb[i]=(unsigned char)v;
        }
    }
    return rgb;
}

static int load(const char *path,Image *im)
{
    FILE *f=fopen(path,"rb"); unsigned char sig[8], *rgb=NULL;
    if(!f) return error("cannot open image",path);
    size_t n=fread(sig,1,sizeof sig,f);
    rewind(f);
    if(n==8 && !png_sig_cmp(sig,0,8)) {
        png_image png; memset(&png,0,sizeof png); png.version=PNG_IMAGE_VERSION;
        if(png_image_begin_read_from_stdio(&png,f)) {
            png.format=PNG_FORMAT_RGB;
            if(png.width && png.height && png.width<=MAX_PIXELS/png.height) {
                im->w=png.width; im->h=png.height;
                rgb=malloc(PNG_IMAGE_SIZE(png));
                png_color bg={255,255,255};
                if(rgb && !png_image_finish_read(&png,&bg,rgb,0,NULL)) {free(rgb);rgb=NULL;}
            }
        }
        png_image_free(&png);
    } else rgb=read_ppm(f,&im->w,&im->h);
    fclose(f);
    if(!rgb) return error("malformed/unsupported 8-bit PNG or PPM (maxval255, <=16M pixels)",path);
    size_t count=(size_t)im->w*im->h;
    im->luma=malloc(count*sizeof *im->luma);
    if(!im->luma) {free(rgb);return error("out of memory",path);}
    for(size_t i=0;i<count;i++) {
#ifdef SSIM_MUT_LUMA
        im->luma[i]=(rgb[3*i]+rgb[3*i+1]+rgb[3*i+2])/3.0;
#else
        im->luma[i]=0.299*rgb[3*i]+0.587*rgb[3*i+1]+0.114*rgb[3*i+2];
#endif
    }
    free(rgb); return 1;
}

static unsigned kernel_size(void)
{
#ifdef SSIM_MUT_WINDOW_9
    return 9;
#else
    return FLAIR_SSIM_WINDOW_SIZE;
#endif
}

static int inside(const Image *im,Rect r)
{
    return r.w && r.h && r.x<=im->w && r.y<=im->h &&
           r.w<=im->w-r.x && r.h<=im->h-r.y;
}

static int compare(const Image *a,Rect ra,const Image *b,Rect rb,Summary *s)
{
    unsigned k=kernel_size();
    if(!inside(a,ra) || !inside(b,rb)) return error("crop outside image",NULL);
    if(ra.w!=rb.w || ra.h!=rb.h) return error("crops must have equal dimensions (no resize)",NULL);
    if(ra.w<k || ra.h<k) return error("crop smaller than Gaussian window",NULL);
    double weights[11*11], norm=0;
    double sigma=FLAIR_SSIM_SIGMA_TENTHS/10.0;
    double k1=FLAIR_SSIM_K1, k2=FLAIR_SSIM_K2;
#ifdef SSIM_MUT_SIGMA
    sigma=2.0;
#endif
#ifdef SSIM_MUT_K1
    k1=0.02;
#endif
#ifdef SSIM_MUT_K2
    k2=0.06;
#endif
    double c1=k1*k1*FLAIR_SSIM_DATA_RANGE*FLAIR_SSIM_DATA_RANGE;
    double c2=k2*k2*FLAIR_SSIM_DATA_RANGE*FLAIR_SSIM_DATA_RANGE;
    for(unsigned y=0;y<k;y++) for(unsigned x=0;x<k;x++) {
        double dx=(double)x-(k-1)/2.0, dy=(double)y-(k-1)/2.0;
        weights[y*k+x]=exp(-(dx*dx+dy*dy)/(2*sigma*sigma));
        norm+=weights[y*k+x];
    }
    for(unsigned i=0;i<k*k;i++) weights[i]/=norm;
    s->w=ra.w-k+1; s->h=ra.h-k+1; s->min=INFINITY; s->max=-INFINITY;
    double total=0, m2=0, running=0; size_t count=0;
    for(unsigned y=0;y<s->h;y++) for(unsigned x=0;x<s->w;x++) {
        double ma=0,mb=0,aa=0,bb=0,ab=0;
        for(unsigned j=0;j<k;j++) for(unsigned i=0;i<k;i++) {
            double av=a->luma[(size_t)(ra.y+y+j)*a->w+ra.x+x+i];
            double bv=b->luma[(size_t)(rb.y+y+j)*b->w+rb.x+x+i];
            double weight=weights[j*k+i];
            ma+=weight*av; mb+=weight*bv;
            aa+=weight*av*av; bb+=weight*bv*bv; ab+=weight*av*bv;
        }
        /* Cancellation can produce a tiny negative variance on flat fields. */
        double va=fmax(0,aa-ma*ma), vb=fmax(0,bb-mb*mb), cov=ab-ma*mb;
        double v=((2*ma*mb+c1)*(2*cov+c2))/((ma*ma+mb*mb+c1)*(va+vb+c2));
        if(!isfinite(v)) return error("non-finite SSIM",NULL);
        if(v<s->min) {s->min=v;s->worst_x=x;s->worst_y=y;}
        if(v>s->max) s->max=v;
        total+=v; count++;
        double delta=v-running; running+=delta/(double)count; m2+=delta*(v-running);
    }
    s->mean=total/count; s->sd=sqrt(fmax(0,m2/count)); return 1;
}

static Rect rect(flair_ssim_rect_t r)
{
    Rect out={r.x,r.y,r.w,r.h};return out;
}

static int guide(const char *frame,const char *build,const char *goldens)
{
    unsigned rows=0; int ok=1;
    printf("SSIM GUIDE frame=%s; native pixels, no threshold (Law 4)\n",frame);
    printf("window | dump crop x,y,w,h | golden crop x,y,w,h | SSIM | map summary (min/max/sd; worst local centre)\n");
    for(size_t i=0;i<FLAIR_SSIM_CROP_COUNT;i++) {
        const flair_ssim_crop_t *c=&flair_ssim_crops[i];
        if(strcmp(c->frame,frame)) continue;
        rows++;
        char left[4096],right[4096]; Image a={0},b={0}; Summary s={0};
        int paths=snprintf(left,sizeof left,"%s/%s",build,c->dump)<(int)sizeof left &&
                  snprintf(right,sizeof right,"%s/%s",goldens,c->golden)<(int)sizeof right;
        if(!paths || !load(left,&a) || !load(right,&b) || !compare(&a,rect(c->native),&b,rect(c->fixture),&s)) {
            fprintf(stderr,"ssim: %s needs its real dump: INITECH_QMP_STDIO=1 make %s\n",c->name,c->gate);
            ok=0;
        } else {
            printf("%s | %s %u,%u,%u,%u | %s %u,%u,%u,%u | %.9f | %ux%u min=%.9f max=%.9f sd=%.9f worst=(%u,%u)\n",
                   c->name,c->dump,c->native.x,c->native.y,c->native.w,c->native.h,
                   c->golden,c->fixture.x,c->fixture.y,c->fixture.w,c->fixture.h,
                   s.mean,s.w,s.h,s.min,s.max,s.sd,s.worst_x+kernel_size()/2,s.worst_y+kernel_size()/2);
        }
        free(a.luma); free(b.luma);
    }
    if(!rows) return error("unknown FRAME (desktop, grow-box, scroll-look, alert, finder, icons, controls)",frame);
    return ok;
}

static int parse_rect(char **argv,Rect *r)
{
    return number(argv[0],&r->x) && number(argv[1],&r->y) &&
           number(argv[2],&r->w) && number(argv[3],&r->h);
}

int main(int argc,char **argv)
{
    if(argc>=3 && !strcmp(argv[1],"--frame")) {
        const char *build="build", *goldens="../system7-decomp/goldens/captures";
        for(int i=3;i<argc;i+=2) {
            if(i+1>=argc) goto usage;
            if(!strcmp(argv[i],"--build")) build=argv[i+1];
            else if(!strcmp(argv[i],"--goldens")) goldens=argv[i+1];
            else goto usage;
        }
        return guide(argv[2],build,goldens) ? 0 : 2;
    }
    if(argc<3) goto usage;
    Rect ra={0},rb={0}; int ca=0,cb=0;
    for(int i=3;i<argc;i+=5) {
        if(i+4>=argc) goto usage;
        if(!strcmp(argv[i],"--crop")) {if(!parse_rect(argv+i+1,&ra)) goto usage;ca=1;}
        else if(!strcmp(argv[i],"--reference-crop")) {if(!parse_rect(argv+i+1,&rb)) goto usage;cb=1;}
        else goto usage;
    }
    Image a={0},b={0}; Summary s={0};
    int ok=load(argv[1],&a) && load(argv[2],&b);
    if(ok) {
        if(!ca) {ra.w=a.w;ra.h=a.h;}
        if(!cb) {if(ca) rb=ra;else {rb.w=b.w;rb.h=b.h;}}
        ok=compare(&a,ra,&b,rb,&s);
    }
    if(ok) printf("SSIM mean=%.12f map=%ux%u windows=%zu min=%.12f max=%.12f sd=%.12f worst=(%u,%u) GUIDE ONLY\n",
                  s.mean,s.w,s.h,(size_t)s.w*s.h,s.min,s.max,s.sd,s.worst_x+kernel_size()/2,s.worst_y+kernel_size()/2);
    free(a.luma);free(b.luma);return ok ? 0 : 2;
usage:
    fprintf(stderr,"usage: ssim actual.{ppm,png} reference.{ppm,png} [--crop x y w h] [--reference-crop x y w h]\n"
                   "       ssim --frame name [--build directory] [--goldens directory]\n");
    return 2;
}
