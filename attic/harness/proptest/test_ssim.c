/* Independent analytic oracle, written before harness/ssim.c (initech-w06).
 * Ref: docs/research/ssim-wang2004.md, Wang 2004 pp.604-606 eqs.7,13-17.
 * Exercises the public CLI, codecs, crops and map; imports no SSIM code/spec.
 * Sparse impulse expectations use its closed-form weighted moments, not a
 * second sliding-window implementation. Numerical bounds test mathematics
 * ONLY: historical UI similarity is never a gate (PRD Sec 3/8, Law 4).
 */
#define _POSIX_C_SOURCE 200809L
#include <math.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int failures;
static const char *tool;
static char a[512], b[512], p[512], bad[512];
static unsigned char pixels[32 * 32 * 3];

static void check(int yes, const char *rule)
{
    printf("%s %s\n", yes ? "PASS" : "RED", rule);
    if (!yes) failures++;
}

static void ppm(const char *path, int w, int h)
{
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(2); }
    fprintf(f, "P6\n# SSIM analytic fixture\n%d %d\n255\n", w, h);
    if (fwrite(pixels, 3, (size_t)w*h, f) != (size_t)w*h || fclose(f)) exit(2);
}

/* No shell or command interpolation; capture just the public score record. */
static int run(const char *left, const char *right, int crop, double *mean,
               unsigned *mw, unsigned *mh)
{
    int fd[2], status;
    char buf[4096]; size_t n = 0;
    if (pipe(fd)) exit(2);
    pid_t child = fork();
    if (child < 0) exit(2);
    if (!child) {
        close(fd[0]);
        if (dup2(fd[1], STDOUT_FILENO) < 0) _exit(126);
        close(fd[1]);
        if (crop == 1)
            execl(tool, tool, left, right, "--crop", "2", "3", "11", "11",
                  "--reference-crop", "4", "5", "11", "11", (char *)NULL);
        else if (crop == 2)
            execl(tool, tool, left, right, "--crop", "30", "30", "11", "11", (char *)NULL);
        else execl(tool, tool, left, right, (char *)NULL);
        _exit(127);
    }
    close(fd[1]);
    ssize_t got;
    while (n < sizeof buf-1 && (got = read(fd[0], buf+n, sizeof buf-1-n)) > 0) n += (size_t)got;
    close(fd[0]); buf[n] = 0;
    if (waitpid(child, &status, 0) < 0 || !WIFEXITED(status)) exit(2);
    int rc = WEXITSTATUS(status);
    if (!rc && (sscanf(buf, "SSIM mean=%lf map=%ux%u", mean, mw, mh) != 3 || !isfinite(*mean))) return 3;
    return rc;
}

static double impulse(int delta)
{
    /* One nonzero pixel at the centre of the only valid 11x11 window.
     * p is its Gaussian mass. E[X]=d*p, Var[X]=d*d*p*(1-p). */
    double sum = 0;
    for (int i = -5; i <= 5; i++) sum += exp(-(double)(i*i)/4.5);
    double weight = 1/(sum*sum), mu = delta*weight;
    double variance = delta*(double)delta*weight*(1-weight);
    return 6.5025/(mu*mu+6.5025) * 58.5225/(variance+58.5225);
}

int main(int argc, char **argv)
{
    if (argc != 3) { fprintf(stderr, "usage: test_ssim tool fixture-directory\n"); return 2; }
    tool = argv[1];
    if (snprintf(a,sizeof a,"%s/ssim-a.ppm",argv[2]) >= (int)sizeof a ||
        snprintf(b,sizeof b,"%s/ssim-b.ppm",argv[2]) >= (int)sizeof b ||
        snprintf(p,sizeof p,"%s/ssim.png",argv[2]) >= (int)sizeof p ||
        snprintf(bad,sizeof bad,"%s/ssim-bad.ppm",argv[2]) >= (int)sizeof bad) return 2;
    double score = 0; unsigned w = 0, h = 0;
    memset(pixels,0,sizeof pixels); ppm(a,11,11); ppm(b,11,11);
    check(!run(a,b,0,&score,&w,&h) && fabs(score-1)<1e-12 && w==1 && h==1,
          "IDENTITY black: SSIM=1, valid map=1x1");
    pixels[0]=pixels[1]=pixels[2]=1;
    memset(pixels,1,11*11*3); ppm(b,11,11);
    check(!run(a,b,0,&score,&w,&h) && fabs(score-6.5025/7.5025)<1e-10,
          "K1 constant-patch analytic value");
    for (int d=1; d<=255; d+=254) {
        memset(pixels,0,sizeof pixels);
        pixels[3*(5*11+5)]=pixels[3*(5*11+5)+1]=pixels[3*(5*11+5)+2]=(unsigned char)d;
        ppm(b,11,11);
        int ok=!run(a,b,0,&score,&w,&h) && score<1 && fabs(score-impulse(d))<1e-10 && w==1 && h==1;
        check(ok, d==1 ? "GAUSSIAN one-pixel perturbation analytic value <1" : "K2 impulse255 analytic value");
        printf("  impulse%d mean=%.12f expected=%.12f\n",d,score,impulse(d));
    }
    for (int i=0;i<32*32;i++) pixels[3*i]=pixels[3*i+1]=pixels[3*i+2]=(i+i/32)%2 ? 255 : 0;
    ppm(a,32,32);
    for (int i=0;i<32*32*3;i++) pixels[i]=255-pixels[i];
    ppm(b,32,32);
    check(!run(a,b,0,&score,&w,&h) && score < -0.99 && w==22 && h==22,
          "INVERSION negative covariance retained; map=22x22");
    printf("  inversion mean=%.12f\n",score);
    memset(pixels,32,sizeof pixels); ppm(a,32,32);
    png_image im; memset(&im,0,sizeof im); im.version=PNG_IMAGE_VERSION;
    im.width=32; im.height=32; im.format=PNG_FORMAT_RGB;
    if (!png_image_write_to_file(&im,p,0,pixels,0,NULL)) return 2;
    check(!run(a,p,0,&score,&w,&h) && fabs(score-1)<1e-12,"PNG/PPM identical decode; raster starts with whitespace byte");
    memset(pixels,0,sizeof pixels); ppm(a,32,32);
    for (int i=0;i<32*32;i++) pixels[i*3]=255;
    ppm(b,32,32);
    double red=0.299*255;
    check(!run(a,b,0,&score,&w,&h) && fabs(score-6.5025/(red*red+6.5025))<1e-10,"LUMA RGB red uses BT.601");
    memset(pixels,0,sizeof pixels);
    for (int y=3;y<14;y++) for(int x=2;x<13;x++)
        pixels[3*(32*y+x)]=pixels[3*(32*y+x)+1]=pixels[3*(32*y+x)+2]=100;
    ppm(a,32,32); memset(pixels,255,sizeof pixels);
    for (int y=5;y<16;y++) for(int x=4;x<15;x++)
        pixels[3*(32*y+x)]=pixels[3*(32*y+x)+1]=pixels[3*(32*y+x)+2]=100;
    ppm(b,32,32);
    check(!run(a,b,1,&score,&w,&h) && fabs(score-1)<1e-12 && w==1 && h==1,
          "CROP independent offsets exclude surrounding pixels");
    check(run(a,b,2,&score,&w,&h)==2,"INPUT out-of-bounds crop rejected");
    ppm(b,10,10);
    check(run(a,b,0,&score,&w,&h)==2,"INPUT unequal dimensions rejected");
    ppm(a,10,10);
    check(run(a,b,0,&score,&w,&h)==2,"INPUT smaller-than-kernel rejected");
    FILE *f=fopen(bad,"wb"); if(!f) return 2;
    fputs("P6\n11 11\n255\nshort",f); fclose(f);
    check(run(bad,bad,0,&score,&w,&h)==2,"INPUT truncated raster rejected");
    printf("test-ssim: %s (%d failures)\n",failures ? "RED" : "green",failures);
    return failures ? 1 : 0;
}
