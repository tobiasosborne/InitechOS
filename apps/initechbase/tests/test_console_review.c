/* PRD 6.6; initech-0spyf/fzsvo. Native page bytes, never framebuffer guesses.
 * controls is projected directly from the pinned real III+ control capture.
 * No SET COLOR screen-mint capture exists: color expectations are host CGA
 * attribute checks from HELP.DBS 1390-1398 / test_samir_set_color.c, NOT a
 * claim of independent real SET COLOR screen parity. */
#include "screen_file.h"
static int bad;
static void check(int ok,const char *why)
{
    if(!ok) { fprintf(stderr,"RED console-review: %s\n",why);bad=1; }
}
int main(int argc,char **argv)
{
    screen_file a,g;
    if(argc!=3) return 2;
    screen_read(argv[2],&a);
    if(!strcmp(argv[1],"continuation")) {
        for(unsigned c=0;c<80;c++)
            check(a.bytes[23*160+c*2]=='X',"off-page continuation completed 80 cells");
    } else if(!strcmp(argv[1],"keys")) {
        check(a.bytes[24*160]=='.' && a.bytes[24*160+2]==' ',"dot prompt present");
        check(a.bytes[24*160+4]==' ' && a.bytes[4001]==2,
              "Left pair is navigation, never text K");
    } else if((!strcmp(argv[1],"color") || !strcmp(argv[1],"color-state"))) {
        check(a.bytes[2*160+3*2]=='S' && a.bytes[2*160+3*2+1]==0x1f,
              "native SAY selected attribute 1F");
        check(a.bytes[2*160+4*2]=='O' && a.bytes[2*160+4*2+1]==0x1f,
              "modeled output selected attribute 1F");
    } else if(!strcmp(argv[1],"controls")) {
        screen_read("harness/diff/dbf_diff/xbase_prog_diff/screen-mint/at-say-edge-controls/controls.screen",&g);
        for(unsigned i=0;i<5;i++)
            check(a.bytes[24*160+i*2]==g.bytes[24*160+i*2],
                  "redirected SAY paints real Z 0D Q 0A R cells");
        check(a.bytes[4000]==24 && a.bytes[4001]==5,"redirected SAY real cursor 24,5");
    } else return 2;
    if(!bad) printf("GREEN console-review: %s\n",argv[1]);
    return bad;
}
