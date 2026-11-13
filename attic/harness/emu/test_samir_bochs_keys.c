/* initech-hdlb: independent RFC 6143 KeyEvent wire oracle, without a live
 * socket. Ref: docs/research/samir-bochs-rfb.md; ADR-0009 DEC-08. Include
 * the actual driver, replacing only the send syscall with a byte sink. */
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <sys/socket.h>
static ssize_t capture_send(int fd, const void *buf, size_t len, int flags);
#define send capture_send
#include "bochs.c"
#undef send

static unsigned char wire[32];
static size_t used;
static int interrupt_once, broken;

static void check(int ok, const char *why)
{
    if (!ok) { fprintf(stderr, "RED samir-bochs-keys: %s\n", why); exit(1); }
}

static ssize_t capture_send(int fd, const void *buf, size_t len, int flags)
{
    (void)fd; (void)flags;
    if (broken) { errno = EPIPE; return -1; }
    if (interrupt_once) { interrupt_once = 0; errno = EINTR; return -1; }
    if (len > 3) len = 3; /* Force short writes through the real sender. */
    check(used + len <= sizeof wire, "packet overflow");
    memcpy(wire + used, buf, len); used += len;
    return (ssize_t)len;
}

int main(void)
{
    RfbShadow r = { .fd = -1 };
    /* RFC 6143 Sec 7.5.4: type, edge, padding, big-endian X keysym. */
    static const unsigned char expected[] = {
        4,1,0,0,0,0,255,13, 4,0,0,0,0,0,255,13,
        4,1,0,0,0,0,0,115, 4,0,0,0,0,0,0,115
    };
    check(rfb_key("ret",3) == 0xff0d, "Return keysym must be ff0d");
    check(rfb_key("spc",3) == 32 && rfb_key("dot",3) == 46,
          "space and period keysyms");
    check(rfb_key("s",1) == 115 && rfb_key("0",1) == 48,
          "ASCII letter and digit keysyms");
    check(!rfb_key("",0) && !rfb_key("rets",4) && !rfb_key("shift",5),
          "unknown tokens rejected");
    /* initech-wuxo0: literal X11 keysyms, independent of the parser map. */
    static const struct { const char *token; unsigned sym; } extended[] = {
        {"home",0xff50}, {"up",0xff52}, {"pgup",0xff55}, {"left",0xff51},
        {"right",0xff53}, {"end",0xff57}, {"down",0xff54}, {"pgdn",0xff56},
        {"insert",0xff63}, {"delete",0xffff}, {"f1",0xffbe}, {"f10",0xffc7},
        {"kp_home",0xffb7}, {"kp_up",0xffb8}, {"kp_pgup",0xffb9},
        {"kp_left",0xffb4}, {"kp_right",0xffb6}, {"kp_end",0xffb1},
        {"kp_down",0xffb2}, {"kp_pgdn",0xffb3}, {"kp_insert",0xffb0},
        {"kp_delete",0xffae}
    };
    for (unsigned i = 0; i < sizeof extended / sizeof extended[0]; i++)
        check(rfb_key(extended[i].token, strlen(extended[i].token)) == extended[i].sym,
              "extended keysym, including unprefixed kp_ tokens");
    BochsTraceStep step;
    check(bochs_trace_parse("shift-f10",9,&step) && step.shift && !step.ctrl &&
          !step.alt && step.key == 0xffc7, "Shift function chord");
    check(bochs_trace_parse("alt-q",5,&step) && step.alt && !step.ctrl &&
          !step.shift && step.key == 'q', "Alt-letter chord");
    check(bochs_trace_parse("ctrl-kp_pgup",12,&step) && step.ctrl &&
          step.key == 0xffb9, "Ctrl keypad chord");
    interrupt_once = 1;
    check(rfb_key_event(&r,rfb_key("ret",3),true), "Return press sent");
    check(rfb_key_event(&r,rfb_key("ret",3),false), "Return release sent");
    check(rfb_key_event(&r,rfb_key("s",1),true), "ASCII press sent");
    check(rfb_key_event(&r,rfb_key("s",1),false), "ASCII release sent");
    check(used == sizeof expected && !memcmp(wire,expected,sizeof expected),
          "press/release wire bytes after interrupted and partial writes");
    used = 0;
    static const unsigned char pointer[] = {5,1,1,64,1,16};
    interrupt_once = 1;
    check(rfb_pointer_event(&r, 320, 272, 1), "pointer packet sent");
    check(used == sizeof pointer && !memcmp(wire,pointer,sizeof pointer),
          "PointerEvent wire bytes after interrupted and partial writes");
    broken = 1;
    check(!rfb_key_event(&r,115,true) && strstr(r.err,"cannot send KeyEvent"),
          "transport failure is loud");
    puts(">>> test-samir-bochs-keys: green (RFC KeyEvent bytes, both edges, partial writes, EINTR, broken transport)");
    return 0;
}
