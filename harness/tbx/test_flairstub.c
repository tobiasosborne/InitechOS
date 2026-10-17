/*
 * harness/tbx/test_flairstub.c -- THE MARSHALLING ORACLE of the Turbo Initech
 * FLAIR binding (factory C, i386 freestanding; bead initech-tdnl.22 R5.2).
 *
 *   test_flairstub <os/tps/FLAIR.PAS>
 *
 * Calls every stub of seed/rt/flairstub.asm DIRECTLY, the way compiled
 * Pascal calls it (cdecl, argument i at [ebp+8+4i], var = address,
 * ShortStrings), with sentinel values in EBX/ESI/EDI, and catches the REAL
 * `int 0x81` it executes (tbx32_rt.c). Every expectation comes from
 * spec/toolbox_gate.h, compiled in (never typed a second time):
 *   - AX is the header's code for the verb;
 *   - the argument block holds exactly the header's arity: the trapped ESP
 *     plus 4 * TBX_ARGC_* is the stub's frame floor (EBP - 12, its three
 *     saved registers), and every dword is the argument the verb owes, in
 *     the header's order (strings as NUL-terminated copies INSIDE the stub's
 *     image storage; menu graphs decoded through the Sec 9 offsets);
 *   - the stub returns the gate's EAX unchanged and preserves EBX/ESI/EDI/
 *     EBP and ESP (the fileio.asm discipline);
 *   - the stub's local refusals (a name, title or label its storage cannot
 *     hold; a ninth menu; a thirty-third item) answer TBX_ERR_BADARG and
 *     never trap;
 *   - the link-synthesized record (Sec 5) and the event trampoline (Sec 6):
 *     an event written into evBufPtr reaches the Pascal handler, field for
 *     field, through eventProc;
 *   - FLAIR.PAS's constants equal the header's values, and its
 *     FlairMaxMenus / FlairMaxItems equal the stub's measured caps.
 * One PASS/FAIL line per check, prefixed by its verb class (LIFECYCLE,
 * WINDOW, DRAW, MENU, ALERT, SCRAP, EVENT, CONSTANTS): each FLAIRSTUB_MUT_*
 * build of the stub must turn its own class RED (test-flairstub-mutant).
 *
 * Exit 0 = all PASS, 1 = a FAIL, 2 = usage. ASCII-clean (Rule 12).
 */
#include "tbx32_rt.h"
#include "tbxmodel.h"
#include "event_model.h"

/* ---- the stub's entries (cdecl; FLAIR.PAS signatures) ---- */
extern char pf_flairregister[], pf_flairexit[], pf_flairnewwindow[],
            pf_flairsetwtitle[], pf_flaircontentorigin[], pf_flairglobaltolocal[],
            pf_flairtextdraw[], pf_flairfillrect[], pf_flairdrawcells[],
            pf_flairnewmenu[], pf_flairappenditem[], pf_flairsetmenubar[],
            pf_flairdrawmenubar[], pf_flairalert[], pf_flairscrapput[],
            pf_flairscrapget[];
extern char __executable_start[];

/* what the stub's entry and trampoline call: the test plays the program */
void pas_main(void);
void pas_main(void) { }
static uint32_t g_handler_ptr;
static int32_t  g_handler_ev[6];
static int      g_handler_calls;
void pf_flaireventhandler(uint32_t ev);
void pf_flaireventhandler(uint32_t ev)
{
    int k;
    g_handler_ptr = ev;
    for (k = 0; k < 6; k++) g_handler_ev[k] = *(int32_t *)(uintptr_t)(ev + 4u * (uint32_t)k);
    g_handler_calls++;
}

/* ---- the call shim: sentinels in, checks out ---- */
uint32_t stub_call(void *fn, int n, const uint32_t *args);
uint32_t g_clobbered, g_esp_before, g_ebp_before;
__asm__(".text\n"
        ".globl stub_call\n"
        "stub_call:\n"
        "    push %ebp\n"
        "    mov  %esp, %ebp\n"
        "    push %ebx\n"
        "    push %esi\n"
        "    push %edi\n"
        "    mov  12(%ebp), %ecx\n"
        "    mov  16(%ebp), %edx\n"
        "1:  test %ecx, %ecx\n"
        "    jz   2f\n"
        "    dec  %ecx\n"
        "    pushl (%edx,%ecx,4)\n"
        "    jmp  1b\n"
        "2:  mov  %esp, g_esp_before\n"
        "    mov  %ebp, g_ebp_before\n"
        "    mov  $0x0B0B0B0B, %ebx\n"
        "    mov  $0x05050505, %esi\n"
        "    mov  $0x0D0D0D0D, %edi\n"
        "    call *8(%ebp)\n"
        "    xor  %edx, %edx\n"
        "    cmp  $0x0B0B0B0B, %ebx\n"
        "    je   3f\n"
        "    or   $1, %edx\n"
        "3:  cmp  $0x05050505, %esi\n"
        "    je   4f\n"
        "    or   $2, %edx\n"
        "4:  cmp  $0x0D0D0D0D, %edi\n"
        "    je   5f\n"
        "    or   $4, %edx\n"
        "5:  cmp  g_ebp_before, %ebp\n"
        "    je   6f\n"
        "    or   $8, %edx\n"
        "6:  cmp  g_esp_before, %esp\n"
        "    je   7f\n"
        "    or   $16, %edx\n"
        "7:  mov  %edx, g_clobbered\n"
        "    lea  -12(%ebp), %esp\n"
        "    pop  %edi\n"
        "    pop  %esi\n"
        "    pop  %ebx\n"
        "    pop  %ebp\n"
        "    ret\n");

/* ---- the trap recorder ---- */
static int      g_traps;
static uint32_t g_ax, g_a[8], g_esp, g_ebp;
static int32_t  g_ret;        /* what the "gate" answers */
static uint8_t  g_get_payload[300];
static int      g_get_len;    /* SCRAP_GET: bytes the gate copies, or <0 */

static int32_t on_trap(uint32_t ax, const uint32_t *a, uint32_t esp, uint32_t ebp)
{
    int i;
    g_traps++;
    g_ax = ax;
    for (i = 0; i < 8; i++) g_a[i] = a[i];
    g_esp = esp;
    g_ebp = ebp;
    if (ax == TBX_SCRAP_GET && g_get_len >= 0) {
        for (i = 0; i < g_get_len; i++)
            *(uint8_t *)(uintptr_t)(a[0] + (uint32_t)i) = g_get_payload[i];
        return g_get_len;
    }
    return g_ret;
}

/* ---- reporting ---- */
static int g_fail;

static void report(const char *cls, const char *what, int ok)
{
    tm_puts(ok ? "PASS " : "FAIL ");
    tm_puts(cls);
    tm_puts(" ");
    tm_puts(what);
    tm_puts("\n");
    if (!ok) g_fail = 1;
}

static int streq(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static void ss(uint8_t *dst, const char *s)     /* C string -> ShortString */
{
    int n = 0;
    while (s[n]) { dst[1 + n] = (uint8_t)s[n]; n++; }
    dst[0] = (uint8_t)n;
}

static uint32_t P(const void *p) { return (uint32_t)(uintptr_t)p; }

/* A call that must trap once with code ax and arity argc; returns EAX. */
static uint32_t trap_call(const char *cls, const char *what, void *fn, int n,
                          const uint32_t *args, uint32_t ax, uint32_t argc,
                          int32_t answer)
{
    uint32_t r;
    int before = g_traps;
    g_ret = answer;
    r = stub_call(fn, n, args);
    report(cls, what, g_traps == before + 1 && g_ax == ax &&
                      g_esp + 4u * argc == g_ebp - 12u &&
                      g_clobbered == 0u && r == (uint32_t)answer);
    return r;
}

/* A call the stub must refuse locally: BADARG, no trap. */
static void refuse_call(const char *cls, const char *what, void *fn, int n,
                        const uint32_t *args)
{
    int before = g_traps;
    uint32_t r = stub_call(fn, n, args);
    report(cls, what, g_traps == before && (int32_t)r == TBX_ERR_BADARG &&
                      g_clobbered == 0u);
}

static int in_image(uint32_t p);
/* a pointer the stub handed the gate, read only when it lies in the image
 * (a wrong one reads as "" and fails the comparison instead of faulting) */
static const char *cstr(uint32_t p) { return in_image(p) ? (const char *)(uintptr_t)p : ""; }
static uint32_t rd32(uint32_t a) { return *(const uint32_t *)(uintptr_t)a; }
static uint16_t rd16(uint32_t a) { return *(const uint16_t *)(uintptr_t)a; }
static uint8_t  rd8(uint32_t a)  { return *(const uint8_t *)(uintptr_t)a; }

/* the stub's image storage lies inside this executable, past the code */
static int in_image(uint32_t p)
{
    extern char _end[];
    return p >= P(__executable_start) && p < P(_end);
}

/* ======================================================================= */
static uint8_t S1[256], S2[256], S3[256], S4[256];
static uint32_t g_rec;

static void t_lifecycle(void)
{
    uint32_t a[4];
    ss(S1, "SMOKETPS");
    a[0] = P(S1);
    trap_call("LIFECYCLE", "REGISTER traps 0x0001 with one argument", pf_flairregister, 1, a,
              TBX_REGISTER, TBX_ARGC_REGISTER, TBX_OK);
    g_rec = g_a[0];
    report("LIFECYCLE", "REGISTER record: tag, imageBase, imageLen (Sec 5)",
           in_image(g_rec) &&
           rd32(g_rec + FLAIR_TENANT_REC_TAG_OFF) == FLAIR_TENANT_REC_TAG &&
           rd32(g_rec + FLAIR_TENANT_REC_BASE_OFF) == P(__executable_start) &&
           rd32(g_rec + FLAIR_TENANT_REC_LEN_OFF) == 0x13572468u);
    report("LIFECYCLE", "REGISTER namePtr: the name, NUL-terminated, in the image",
           in_image(rd32(g_rec + FLAIR_TENANT_REC_NAME_OFF)) &&
           streq(cstr(rd32(g_rec + FLAIR_TENANT_REC_NAME_OFF)), "SMOKETPS"));
    ss(S1, "ELEVENCHARS");                    /* 11 + NUL = the bound */
    trap_call("LIFECYCLE", "REGISTER accepts an 11-character name", pf_flairregister, 1, a,
              TBX_REGISTER, TBX_ARGC_REGISTER, TBX_ERR_BUSY);
    ss(S1, "TWELVECHARSX");
    refuse_call("LIFECYCLE", "REGISTER refuses a name past FLAIR_TENANT_NAME_MAX", pf_flairregister, 1, a);
    a[0] = 1234u;
    trap_call("LIFECYCLE", "EXIT traps 0x0070 with one argument", pf_flairexit, 1, a,
              TBX_EXIT, TBX_ARGC_EXIT, TBX_OK);
    report("LIFECYCLE", "EXIT passes the program's code", g_a[0] == 1234u);
    a[0] = (uint32_t)-5;
    trap_call("LIFECYCLE", "EXIT returns the gate's answer", pf_flairexit, 1, a,
              TBX_EXIT, TBX_ARGC_EXIT, TBX_ERR_NOTREG);
    report("LIFECYCLE", "EXIT passes a negative code", g_a[0] == (uint32_t)-5);
}

static void t_window(void)
{
    uint32_t a[6];
    a[0] = 10; a[1] = 20; a[2] = 300; a[3] = 400; a[4] = TBX_GOAWAY_CLOSE | TBX_WDEF_RDOC;
    trap_call("WINDOW", "NEWWINDOW traps 0x0020 with five arguments", pf_flairnewwindow, 5, a,
              TBX_NEWWINDOW, TBX_ARGC_NEWWINDOW, 1);
    report("WINDOW", "NEWWINDOW block = (l, t, r, b, goAway)",
           g_a[0] == 10u && g_a[1] == 20u && g_a[2] == 300u && g_a[3] == 400u &&
           g_a[4] == (TBX_GOAWAY_CLOSE | TBX_WDEF_RDOC));
    ss(S1, "Calculator");
    a[0] = 2; a[1] = P(S1);
    trap_call("WINDOW", "SETWTITLE traps 0x0022 with two arguments", pf_flairsetwtitle, 2, a,
              TBX_SETWTITLE, TBX_ARGC_SETWTITLE, TBX_OK);
    report("WINDOW", "SETWTITLE block = (win, ASCIZ copy in the image)",
           g_a[0] == 2u && in_image(g_a[1]) && g_a[1] != P(S1 + 1) &&
           streq(cstr(g_a[1]), "Calculator"));
    a[0] = 7;
    trap_call("WINDOW", "CONTENTORIGIN traps 0x0023 with one argument", pf_flaircontentorigin, 1, a,
              TBX_CONTENTORIGIN, TBX_ARGC_CONTENTORIGIN, 0x00710109);
    report("WINDOW", "CONTENTORIGIN block = (win)", g_a[0] == 7u);
    a[0] = 1; a[1] = (uint32_t)-7; a[2] = 9;
    trap_call("WINDOW", "GLOBALTOLOCAL traps 0x00C3 with three arguments", pf_flairglobaltolocal, 3, a,
              TBX_GLOBALTOLOCAL, TBX_ARGC_GLOBALTOLOCAL, (int32_t)0xFFF90009);
    report("WINDOW", "GLOBALTOLOCAL block = (win, h, v)",
           g_a[0] == 1u && g_a[1] == (uint32_t)-7 && g_a[2] == 9u);
}

static void t_draw(void)
{
    uint32_t a[6];
    int i;
    char long255[256];
    ss(S1, "Hello, Initech");
    a[0] = 1; a[1] = 2; a[2] = 3; a[3] = P(S1); a[4] = 1u | TBX_TEXT_BOLD; a[5] = 2;
    trap_call("DRAW", "TEXTDRAW traps 0x0030 with six arguments", pf_flairtextdraw, 6, a,
              TBX_TEXTDRAW, TBX_ARGC_TEXTDRAW, TBX_OK);
    report("DRAW", "TEXTDRAW block = (win, x, y, ASCIZ copy, fg, bg)",
           g_a[0] == 1u && g_a[1] == 2u && g_a[2] == 3u && in_image(g_a[3]) &&
           streq(cstr(g_a[3]), "Hello, Initech") &&
           g_a[4] == (1u | TBX_TEXT_BOLD) && g_a[5] == 2u);
    for (i = 0; i < 255; i++) long255[i] = (char)('A' + i % 26);
    long255[255] = 0;
    ss(S1, long255);
    trap_call("DRAW", "TEXTDRAW copies a full 255-byte string", pf_flairtextdraw, 6, a,
              TBX_TEXTDRAW, TBX_ARGC_TEXTDRAW, TBX_OK);
    report("DRAW", "TEXTDRAW copy is all 255 bytes + NUL", streq(cstr(g_a[3]), long255));
    ss(S1, "");
    trap_call("DRAW", "TEXTDRAW of the empty string", pf_flairtextdraw, 6, a,
              TBX_TEXTDRAW, TBX_ARGC_TEXTDRAW, TBX_OK);
    report("DRAW", "TEXTDRAW empty copy is the NUL alone", streq(cstr(g_a[3]), ""));
    a[0] = 1; a[1] = (uint32_t)-4; a[2] = 5; a[3] = 600; a[4] = 7; a[5] = 3;
    trap_call("DRAW", "FILLRECT traps 0x0031 with six arguments", pf_flairfillrect, 6, a,
              TBX_FILLRECT, TBX_ARGC_FILLRECT, TBX_ERR_BADARG);
    report("DRAW", "FILLRECT block = (win, l, t, r, b, colour)",
           g_a[0] == 1u && g_a[1] == (uint32_t)-4 && g_a[2] == 5u &&
           g_a[3] == 600u && g_a[4] == 7u && g_a[5] == 3u);
    ss(S1, "A1 '123");
    a[0] = 2; a[1] = 16; a[2] = 32; a[3] = P(S1); a[4] = 1; a[5] = 0;
    trap_call("DRAW", "DRAWCELLS traps 0x0033 with six arguments", pf_flairdrawcells, 6, a,
              TBX_DRAWCELLS, TBX_ARGC_DRAWCELLS, TBX_OK);
    report("DRAW", "DRAWCELLS block = (win, x, y, ASCIZ copy, fg, bg)",
           g_a[0] == 2u && g_a[1] == 16u && g_a[2] == 32u && in_image(g_a[3]) &&
           streq(cstr(g_a[3]), "A1 '123") && g_a[4] == 1u && g_a[5] == 0u);
}

static int g_max_menus, g_max_items;

static void t_menu(void)
{
    uint32_t a[4], bar, menus, items, it;
    int i, ok;
    ss(S1, "Orphan");
    a[0] = P(S1); a[1] = 'O'; a[2] = 1;
    refuse_call("MENU", "an item before any menu is refused", pf_flairappenditem, 3, a);
    ss(S1, "File");
    a[0] = 129; a[1] = P(S1);
    i = g_traps;
    report("MENU", "NewMenu builds locally (OK, no trap)",
           (int32_t)stub_call(pf_flairnewmenu, 2, a) == TBX_OK && g_traps == i && g_clobbered == 0u);
    ss(S1, "Close");
    a[0] = P(S1); a[1] = 'W'; a[2] = 1;
    report("MENU", "AppendItem builds locally (OK, no trap)",
           (int32_t)stub_call(pf_flairappenditem, 3, a) == TBX_OK && g_traps == i && g_clobbered == 0u);
    ss(S1, "-");
    a[0] = P(S1); a[1] = 0; a[2] = 0;
    (void)stub_call(pf_flairappenditem, 3, a);
    ss(S1, "About Smoke");
    a[0] = P(S1); a[1] = 0; a[2] = 1;
    (void)stub_call(pf_flairappenditem, 3, a);
    ss(S1, "Edit");
    a[0] = 130; a[1] = P(S1);
    (void)stub_call(pf_flairnewmenu, 2, a);
    ss(S1, "Copy");
    a[0] = P(S1); a[1] = 'C'; a[2] = 1;
    (void)stub_call(pf_flairappenditem, 3, a);
    ss(S1, "0123456789012345678901234567890X");     /* 32 + NUL > STR_MAX */
    a[0] = P(S1); a[1] = 0; a[2] = 1;
    refuse_call("MENU", "an item text past TBX_MBAR_STR_MAX is refused", pf_flairappenditem, 3, a);
    a[0] = 131; a[1] = P(S1);
    refuse_call("MENU", "a title past TBX_MBAR_STR_MAX is refused", pf_flairnewmenu, 2, a);
    a[0] = 1;
    trap_call("MENU", "SETMBAR traps 0x0050 with one argument", pf_flairsetmenubar, 1, a,
              TBX_SETMBAR, TBX_ARGC_SETMBAR, TBX_OK);
    bar = g_a[0];
    menus = rd32(bar + TBX_MBAR_MENUS_OFF);
    ok = in_image(bar) && in_image(menus) && rd16(bar + TBX_MBAR_COUNT_OFF) == 2u &&
         rd8(bar + TBX_MBAR_APPLE_OFF) == 1u;
    report("MENU", "MenuBar: two menus, the Apple slot (Sec 9 offsets)", ok);
    ok = (int16_t)rd16(menus + TBX_MINFO_ID_OFF) == 129 &&
         streq(cstr(rd32(menus + TBX_MINFO_TITLE_OFF)), "File") &&
         rd16(menus + TBX_MINFO_COUNT_OFF) == 3u &&
         (int16_t)rd16(menus + TBX_MINFO_WIDTH_OFF) == 0 &&
         (int16_t)rd16(menus + TBX_MINFO_BYTES + TBX_MINFO_ID_OFF) == 130 &&
         streq(cstr(rd32(menus + TBX_MINFO_BYTES + TBX_MINFO_TITLE_OFF)), "Edit") &&
         rd16(menus + TBX_MINFO_BYTES + TBX_MINFO_COUNT_OFF) == 1u;
    report("MENU", "MenuInfo: ids, titles, item counts", ok);
    items = rd32(menus + TBX_MINFO_ITEMS_OFF);
    it = items;
    ok = in_image(items) && streq(cstr(rd32(it + TBX_MITEM_TEXT_OFF)), "Close") &&
         rd8(it + TBX_MITEM_CMD_OFF) == 'W' && rd8(it + TBX_MITEM_ENABLED_OFF) == 1u &&
         rd8(it + TBX_MITEM_DIVIDER_OFF) == 0u && rd8(it + TBX_MITEM_MARK_OFF) == 0u &&
         rd8(it + TBX_MITEM_STYLE_OFF) == 0u;
    report("MENU", "MenuItem Close: text, command key W, enabled", ok);
    it = items + TBX_MITEM_BYTES;
    ok = streq(cstr(rd32(it + TBX_MITEM_TEXT_OFF)), "-") &&
         rd8(it + TBX_MITEM_DIVIDER_OFF) == 1u && rd8(it + TBX_MITEM_ENABLED_OFF) == 0u;
    report("MENU", "MenuItem '-': a dividing line (IM AppendMenu)", ok);
    it = rd32(menus + TBX_MINFO_BYTES + TBX_MINFO_ITEMS_OFF);
    ok = it == items + 3u * TBX_MITEM_BYTES &&
         streq(cstr(rd32(it + TBX_MITEM_TEXT_OFF)), "Copy") && rd8(it + TBX_MITEM_CMD_OFF) == 'C';
    report("MENU", "Edit's items follow File's (contiguous storage)", ok);
    trap_call("MENU", "DRAWMENUBAR traps 0x0052 with no argument", pf_flairdrawmenubar, 0, a,
              TBX_DRAWMENUBAR, TBX_ARGC_DRAWMENUBAR, TBX_OK);
    /* the caps: menus until refused, then items until refused */
    g_max_menus = 2;
    for (;;) {
        ss(S1, "M");
        a[0] = 200u + (uint32_t)g_max_menus; a[1] = P(S1);
        if ((int32_t)stub_call(pf_flairnewmenu, 2, a) != TBX_OK) break;
        if (++g_max_menus > 64) break;
    }
    g_max_items = 4;
    for (;;) {
        ss(S1, "I");
        a[0] = P(S1); a[1] = 0; a[2] = 1;
        if ((int32_t)stub_call(pf_flairappenditem, 3, a) != TBX_OK) break;
        if (++g_max_items > 256) break;
    }
}

static void t_alert(void)
{
    uint32_t a[4];
    ss(S1, "Save changes?\rThe document is open.");
    ss(S2, "Save");
    ss(S3, "Cancel");
    ss(S4, "Don't Save");
    a[0] = P(S1); a[1] = P(S2); a[2] = P(S3); a[3] = P(S4);
    trap_call("ALERT", "ALERT traps 0x0090 with four arguments", pf_flairalert, 4, a,
              TBX_ALERT, TBX_ARGC_ALERT, TBX_OK);
    report("ALERT", "ALERT block = (text, default, cancel, other) ASCIZ copies",
           in_image(g_a[0]) && in_image(g_a[1]) && in_image(g_a[2]) && in_image(g_a[3]) &&
           streq(cstr(g_a[0]), "Save changes?\rThe document is open.") &&
           streq(cstr(g_a[1]), "Save") && streq(cstr(g_a[2]), "Cancel") &&
           streq(cstr(g_a[3]), "Don't Save"));
    ss(S3, "");
    ss(S4, "");
    trap_call("ALERT", "ALERT with one button", pf_flairalert, 4, a,
              TBX_ALERT, TBX_ARGC_ALERT, TBX_ERR_BUSY);
    report("ALERT", "an empty cancel and other label pass 0",
           g_a[2] == 0u && g_a[3] == 0u && streq(cstr(g_a[1]), "Save"));
    ss(S3, "Cancel");
    trap_call("ALERT", "ALERT with two buttons", pf_flairalert, 4, a,
              TBX_ALERT, TBX_ARGC_ALERT, TBX_OK);
    report("ALERT", "cancel set, other empty -> (cancel, 0)",
           streq(cstr(g_a[2]), "Cancel") && g_a[3] == 0u);
    ss(S2, "SixteenCharsLong");                      /* 16 + NUL > LABEL_MAX */
    refuse_call("ALERT", "a label past TBX_ALERT_LABEL_MAX is refused", pf_flairalert, 4, a);
}

static void t_scrap(void)
{
    uint32_t a[2];
    uint32_t r;
    int i, ok;
    ss(S1, "Smoke on the Scrap");
    a[0] = P(S1);
    trap_call("SCRAP", "SCRAP_PUT traps 0x00A0 with two arguments", pf_flairscrapput, 1, a,
              TBX_SCRAP_PUT, TBX_ARGC_SCRAP_PUT, TBX_OK);
    ok = in_image(g_a[0]) && g_a[1] == 18u;
    for (i = 0; ok && i < 18; i++) ok = rd8(g_a[0] + (uint32_t)i) == S1[1 + i];
    report("SCRAP", "SCRAP_PUT block = (image copy, the full length)", ok);
    for (i = 0; i < 5; i++) g_get_payload[i] = (uint8_t)"hello"[i];
    g_get_len = 5;
    ss(S2, "unchanged");
    a[0] = P(S2);
    r = stub_call(pf_flairscrapget, 1, a);
    report("SCRAP", "SCRAP_GET traps 0x00A1 with (image buffer, 255)",
           g_ax == TBX_SCRAP_GET && g_esp + 4u * TBX_ARGC_SCRAP_GET == g_ebp - 12u &&
           in_image(g_a[0]) && g_a[1] == 255u && g_clobbered == 0u);
    report("SCRAP", "SCRAP_GET answers the length and fills the string",
           r == 5u && S2[0] == 5u && S2[1] == 'h' && S2[5] == 'o');
    g_get_len = -1;
    ss(S2, "unchanged");
    r = trap_call("SCRAP", "SCRAP_GET of an empty Scrap", pf_flairscrapget, 1, a,
                  TBX_SCRAP_GET, TBX_ARGC_SCRAP_GET, TBX_ERR_NOTYPE);
    report("SCRAP", "an error leaves the string untouched",
           (int32_t)r == TBX_ERR_NOTYPE && S2[0] == 9u && S2[1] == 'u');
}

static void t_event(void)
{
    uint32_t evb, proc;
    int k, ok;
    evb  = rd32(g_rec + FLAIR_TENANT_REC_EVB_OFF);
    proc = rd32(g_rec + FLAIR_TENANT_REC_PROC_OFF);
    report("EVENT", "the record's evBufPtr and eventProc lie in the image",
           in_image(evb) && in_image(proc));
    for (k = 0; k < 6; k++) *(int32_t *)(uintptr_t)(evb + 4u * (uint32_t)k) = 100 * (k + 1) + k;
    *(int32_t *)(uintptr_t)(evb + FLAIR_EVBUF_WHAT_OFF) = mouseDown;
    g_handler_calls = 0;
    (void)tbx32_call(proc, 0u);
    ok = g_handler_calls == 1 && g_handler_ptr == evb &&
         g_handler_ev[0] == mouseDown && g_handler_ev[1] == 201 &&
         g_handler_ev[2] == 302 && g_handler_ev[3] == 403 &&
         g_handler_ev[4] == *(int32_t *)(uintptr_t)(evb + FLAIR_EVBUF_WHEREH_OFF) &&
         g_handler_ev[5] == *(int32_t *)(uintptr_t)(evb + FLAIR_EVBUF_WHEREV_OFF);
    report("EVENT", "eventProc hands FlairEventHandler the buffer, field for field", ok);
}

/* ---- FLAIR.PAS constants vs the headers ---- */
static const struct { const char *name; int32_t value; } k_const[] = {
    { "FlairOK", TBX_OK }, { "FlairErrBadCode", TBX_ERR_BADCODE },
    { "FlairErrNotReg", TBX_ERR_NOTREG }, { "FlairErrBadArg", TBX_ERR_BADARG },
    { "FlairErrBusy", TBX_ERR_BUSY }, { "FlairErrNoMem", TBX_ERR_NOMEM },
    { "FlairErrBadRec", TBX_ERR_BADREC }, { "FlairErrNotTenant", TBX_ERR_NOTTENANT },
    { "FlairErrNoType", TBX_ERR_NOTYPE },
    { "FlairWhite", TBX_COLOR_WHITE }, { "FlairBlack", TBX_COLOR_BLACK },
    { "FlairGray", TBX_COLOR_GRAY }, { "FlairNavy", TBX_COLOR_NAVY },
    { "FlairBold", (int32_t)TBX_TEXT_BOLD }, { "FlairItalic", (int32_t)TBX_TEXT_ITALIC },
    { "FlairUnderline", (int32_t)TBX_TEXT_UNDERLINE }, { "FlairGeneva9", (int32_t)TBX_TEXT_GENEVA9 },
    { "FlairGoAwayNone", TBX_GOAWAY_NONE }, { "FlairGoAwayClose", TBX_GOAWAY_CLOSE },
    { "FlairGoAwayAsk", TBX_GOAWAY_ASK }, { "FlairRDocProc", (int32_t)TBX_WDEF_RDOC },
    { "FlairNullEvent", nullEvent }, { "FlairMouseDown", mouseDown },
    { "FlairMouseUp", mouseUp }, { "FlairKeyDown", keyDown }, { "FlairKeyUp", keyUp },
    { "FlairAutoKey", autoKey }, { "FlairUpdateEvt", updateEvt },
    { "FlairActivateEvt", activateEvt },
    { "FlairMenuEvt", TBX_EVT_MENU }, { "FlairGoAwayEvt", TBX_EVT_GOAWAY },
    { "FlairAlertEvt", TBX_EVT_ALERT },
    { "FlairAlertDefault", TBX_ALERT_DEFAULT }, { "FlairAlertCancel", TBX_ALERT_CANCEL },
    { "FlairAlertOther", TBX_ALERT_OTHER },
};
#define N_CONST ((int)(sizeof k_const / sizeof k_const[0]))

static char g_pas[65536];

static int ieq(const char *a, int n, const char *b)
{
    int i;
    for (i = 0; i < n; i++) {
        char x = a[i], y = b[i];
        if (x >= 'A' && x <= 'Z') x = (char)(x - 'A' + 'a');
        if (y >= 'A' && y <= 'Z') y = (char)(y - 'A' + 'a');
        if (y == 0 || x != y) return 0;
    }
    return b[n] == 0;
}

/* Find "Name = <int>;" in the const part of FLAIR.PAS (before `type`). */
static int pas_const(const char *text, int len, const char *name, int32_t *out)
{
    int p = 0;
    while (p < len) {
        int s = p, e;
        while (p < len && ((text[p] >= 'A' && text[p] <= 'Z') || (text[p] >= 'a' && text[p] <= 'z') ||
                           (text[p] >= '0' && text[p] <= '9') || text[p] == '_')) p++;
        e = p;
        if (e > s && (s == 0 || text[s - 1] == ' ' || text[s - 1] == '\n') && ieq(text + s, e - s, name)) {
            int q = e, neg = 0, any = 0;
            int32_t v = 0;
            while (q < len && text[q] == ' ') q++;
            if (q < len && text[q] == '=') {
                q++;
                while (q < len && text[q] == ' ') q++;
                if (q < len && text[q] == '-') { neg = 1; q++; }
                while (q < len && text[q] >= '0' && text[q] <= '9') { v = v * 10 + (text[q] - '0'); q++; any = 1; }
                if (any && q < len && text[q] == ';') { *out = neg ? -v : v; return 1; }
            }
        }
        if (p == s) p++;
    }
    return 0;
}

static void t_constants(const char *path)
{
    int len = tbx32_read_file(path, g_pas, (int)sizeof g_pas), i;
    int32_t v;
    if (len < 0) { report("CONSTANTS", "FLAIR.PAS readable", 0); return; }
    for (i = 0; i < N_CONST; i++) {
        int ok = pas_const(g_pas, len, k_const[i].name, &v) && v == k_const[i].value;
        report("CONSTANTS", k_const[i].name, ok);
    }
    report("CONSTANTS", "FlairMaxMenus == the stub's menu cap",
           pas_const(g_pas, len, "FlairMaxMenus", &v) && v == g_max_menus &&
           g_max_menus <= (int)TBX_MBAR_MAX_MENUS);
    report("CONSTANTS", "FlairMaxItems == the stub's item cap",
           pas_const(g_pas, len, "FlairMaxItems", &v) && v == g_max_items);
}

int tbx32_main(int argc, char **argv)
{
    if (argc != 2) tbx32_die("usage: test_flairstub <FLAIR.PAS>");
    tbx32_on_trap = on_trap;
    tbx32_install();
    g_get_len = -1;
    t_lifecycle();
    t_window();
    t_draw();
    t_menu();
    t_alert();
    t_scrap();
    t_event();
    t_constants(argv[1]);
    tm_puts(g_fail ? "test-flairstub: RED\n" : "test-flairstub: green\n");
    return g_fail ? 1 : 0;
}
