/* initech-0xs6: refused-power notice. Reuse the existing independently
 * captured alert chrome/colors/PPM reader; keep all Trash gates unchanged.
 * Exact heading words are checked below as well as by test_power_notice
 * and the emulator transcript; all three lines, the OK label and default
 * ring must be visible. No assumptions about documents behind the notice.
 * Golden: system7-decomp s8_alert_modal.png (see included checker). */
#define main trash_checker_main
#include "ppm_flair_trash_check.c"
#undef main
#include "chicago12.h"
#include "../spec/chicago12_nfnt_golden.h"

/* Authored operation-neutral wording (REVIEW.md): the same notice serves
 * Exit to InitechDOS, Shut Down and Restart. Match the locked Chicago ART
 * at independent NFNT 5478 metrics, as ppm_flair_app_launch_check does.
 * The expected literal is independent of power_notice.h. */
static void notice_heading(void)
{
    const char *text = "The operation was cancelled.";
    int bad = 0;
    for (int r = 0; r < CHICAGO_CELL_H; r++)
        for (int x = 0; x < TXT_X1 - TXT_X0; x++) {
            int pen = 0, set = 0;
            for (const char *s = text; *s; s++) {
                int c = (unsigned char)*s, aw = nfnt5478_aw(c);
                if (x >= pen && x < pen + aw) {
                    int col = x - pen - nfnt5478_lb(c);
                    if (col >= 0 && col < 16)
                        set = !!(chicago12_rows[chicago12_index(c)][r] & (0x8000u >> col));
                    break;
                }
                pen += aw;
            }
            uint32_t want = flair_canon_rgb((unsigned char)(set ? CIDX_BLACK : CIDX_PLAT_FACE)) & 0xFFFFFFu;
            bad += alert_capture_rgb(g_buf, TXT_X0 + x, TXT_Y0 + r) != want;
        }
    if (bad) {
        fprintf(stderr, "FAIL power-notice heading: %d pixels differ from \"%s\"\n", bad, text);
        g_fail = 1;
    }
}

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
    notice_heading();
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
