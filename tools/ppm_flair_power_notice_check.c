/* initech-0xs6: refused-power notice. Reuse the existing independently
 * captured alert chrome/colors/PPM reader; keep all Trash gates unchanged.
 * Message words are graded by test_power_notice and the emulator transcript;
 * these pixels prove that all three lines, the OK label and default ring
 * are visible. No assumptions about documents behind the modal notice.
 * Golden: system7-decomp s8_alert_modal.png (see included checker). */
#define main trash_checker_main
#include "ppm_flair_trash_check.c"
#undef main

int main(int argc, char **argv)
{
    FILE *f;
    long w,h,maxv;
    char magic[2];
    size_t n=(size_t)SCRW*SCRH*3;
    if(argc!=2) {fprintf(stderr,"usage: %s <notice.ppm>\n",argv[0]);return 2;}
    g_leg="power-refused";
    f=fopen(argv[1],"rb");
    if(!f || fread(magic,1,2,f)!=2 || magic[0]!='P' || magic[1]!='6' ||
       read_uint(f,&w) || read_uint(f,&h) || read_uint(f,&maxv) ||
       w!=SCRW || h!=SCRH || maxv!=255) {
        fprintf(stderr,"FAIL power-notice: missing or invalid PPM\n");
        if(f) fclose(f);
        return 2;
    }
    g_buf=malloc(n);
    if(!g_buf || fread(g_buf,1,n,f)!=n) {fclose(f);free(g_buf);return 2;}
    fclose(f);
    alert_chrome(0);
    want(300,150,CIDX_PLAT_FACE,"alert face between message and buttons");
    for(int k=0;k<3;k++)
        want_ink(TXT_X0,TXT_Y0+16*k,TXT_X1,TXT_Y0+16*(k+1),1,"message line");
    want_ink(TXT_X0,TXT_Y0+48,CA_L-3,BT_B,0,"no fourth line");
    want_ink(AL_L+AL_BORDER+1,TXT_Y0,TXT_X0-2,TXT_Y0+48,0,"no icon or stray text");
    want_ink(OK_L+4,BT_T+3,OK_R-4,BT_B-2,1,"OK label");
    want_ink(CA_L+4,BT_T+3,CA_R-4,BT_B-2,0,"no Cancel button");
    want((OK_L+OK_R)/2,BT_T-3,CIDX_BLACK,"OK default ring");
    want((CA_L+CA_R)/2,BT_T-3,CIDX_PLAT_FACE,"no Cancel ring");
    free(g_buf);
    puts(g_fail?"FAIL power-notice pixels":"GREEN power-notice pixels");
    return g_fail?1:0;
}
