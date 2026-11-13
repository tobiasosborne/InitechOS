/* initech-tdnl.91: emulator fixture for the file ABI, authored, no local
 * reference for fixture bytes. Ref: spec/toolbox_gate.h Sec 10.
 * Independent mtools grader expects exactly "Gate file bytes\r\n". */
#include "tbx.h"
const char tbx_app_name[] = "FILETEN";
static char name[] TBX_IN_IMAGE = "KEEP.DAT";
static char payload[] TBX_IN_IMAGE = "Gate file bytes\r\n";
static char got[32];
static int32_t win;
static int32_t fcall(uint32_t code, uint32_t a, uint32_t b, uint32_t c)
{
    switch (code) {
    case TBX_FILE_CREATE: return tbx_file_create((const char *)(uintptr_t)a);
    case TBX_FILE_OPEN: return tbx_file_open((const char *)(uintptr_t)a,b);
    case TBX_FILE_READ: return tbx_file_read((int32_t)a,(void *)(uintptr_t)b,c);
    case TBX_FILE_WRITE: return tbx_file_write((int32_t)a,(const void *)(uintptr_t)b,c);
    case TBX_FILE_SEEK: return tbx_file_seek((int32_t)a,(int32_t)b,c);
    case TBX_FILE_CLOSE: return tbx_file_close((int32_t)a);
    case TBX_FILE_DELETE: return tbx_file_delete((const char *)(uintptr_t)a);
    default: return TBX_ERR_BADCODE;
    }
}
#define PATH ((uint32_t)(uintptr_t)name)
static void say(const char *s)
{ (void)tbx_draw_cells(win,8,40,s,TBX_COLOR_BLACK,TBX_COLOR_WHITE); }
static void roundtrip(void)
{
    /* DOS reserves device basenames even with an extension; a file CREATE
     * must not make a disk file that OPEN will resolve as a device. */
    if (tbx_file_create("nul.txt") != TBX_ERR_BADARG) { say("FILE DEVICE FAILED"); return; }
    int32_t h = fcall(TBX_FILE_CREATE,PATH,0,0);
    if (h < 5 || fcall(TBX_FILE_WRITE,h,(uint32_t)(uintptr_t)payload,17) != 17 ||
        fcall(TBX_FILE_CLOSE,h,0,0) != 0) { say("FILE WRITE FAILED"); return; }
    h = fcall(TBX_FILE_OPEN,PATH,0,0);
    if (h < 5 || fcall(TBX_FILE_SEEK,h,5,0) != 5 ||
        fcall(TBX_FILE_READ,h,(uint32_t)(uintptr_t)got,12) != 12) { say("FILE READ FAILED"); return; }
    for (uint32_t i = 0; i < 12; i++)
        if (got[i] != payload[i+5]) { say("FILE BYTES FAILED"); return; }
    if (fcall(TBX_FILE_CLOSE,h,0,0)) { say("FILE CLOSE FAILED"); return; }
    name[0]='D'; name[1]='E'; name[2]='L'; name[3]='E';
    h = fcall(TBX_FILE_CREATE,PATH,0,0);
    if (h < 5 || fcall(TBX_FILE_WRITE,h,(uint32_t)(uintptr_t)payload,17) != 17 ||
        fcall(TBX_FILE_CLOSE,h,0,0) || fcall(TBX_FILE_DELETE,PATH,0,0)) { say("FILE DELETE FAILED"); return; }
    say("FILE ROUNDTRIP OK");
}
static void open_all(char how)
{
    for (uint32_t i = 0; i < 15; i++) {
        if (fcall(TBX_FILE_OPEN,PATH,0,0) < 5) { say("FILE OPEN LIMIT FAILED"); return; }
    }
    say("FILE OPEN FIFTEEN OK");
    if (how == 'o') (void)tbx_exit(0);
    if (how == 'x') {
        extern char __image_start[];
        /* Crash after scribbling the image PSP's JFT: cleanup cannot trust it.
         * ADR-0013 Sec 3.4/AC-2 (death independent of damaged tenant data). */
        uintptr_t base;
        __asm__ __volatile__("" : "=r"(base) : "0"((uintptr_t)__image_start));
        volatile uint8_t *psp = (volatile uint8_t *)(base - 256u);
        for (uint32_t i = 0; i < 20; i++) psp[0x18+i] = 0xee;
        __asm__ __volatile__("ud2");
    }
}
int tbx_app_main(void)
{
    if (tbx_register()) return 1;
    win = tbx_new_window(180,170,470,330,1);
    (void)tbx_set_wtitle(win,"File Gate"); return 0;
}
void tbx_app_event(const tbx_event_t *ev)
{
    if (ev->what == updateEvt) say("FILE READY");
    if (ev->what == keyDown) {
        char ch = (char)ev->message;
        if (ch == 'q') roundtrip();
        if (ch == 'o' || ch == 'c' || ch == 'x') open_all(ch);
    }
}
