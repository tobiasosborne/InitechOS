/* Feasibility probe (turbo/README.md "IDE"): can an extended InitechMZ
 * program stay resident while it EXECs a child and regain control?
 * Linked exactly like TURBO.COM (start_extended.asm, seed_extended.ld,
 * mzlink --extended) but with this C body as pas_main. Authority for the
 * calls: kernel/int21.c do_exec (flat AH=4Bh: EDX path, EBX parameter
 * block), AH=4Dh, AH=40h; spec in kernel/include/dos_structs.h. */
struct exec_block { unsigned env, tail, fcb1, fcb2; };

static unsigned length(const char *s) { unsigned n = 0; while (s[n]) n++; return n; }

static void say(const char *s)
{
    unsigned ax = 0x4000, bx = 1, cx = length(s), dx = (unsigned)s;
    __asm__ volatile("int $0x21" : "+a"(ax), "+b"(bx), "+c"(cx), "+d"(dx) : : "cc", "memory");
}

static void hex(char *p, unsigned v) { for (int i = 3; i >= 0; i--) { p[i] = "0123456789ABCDEF"[v & 15]; v >>= 4; } }

static void attempt(const char *name)
{
    static const char tail[2] = { 0, '\r' };
    struct exec_block block = { 0, (unsigned)tail, 0, 0 };
    unsigned ax = 0x4b00, bx = (unsigned)&block, dx = (unsigned)name, carry;
    __asm__ volatile("int $0x21\n\tsbb %%ecx,%%ecx" : "+a"(ax), "+b"(bx), "+d"(dx), "=c"(carry) : : "cc", "memory");
    unsigned status = 0;
    if (!carry) {
        status = 0x4d00;
        __asm__ volatile("int $0x21" : "+a"(status) : : "cc", "memory");
    }
    char line[] = "PROBE-EXEC cf=0 ax=0000 status=0000 name=";
    line[14] = carry ? '1' : '0';
    hex(line + 19, carry ? ax & 0xffff : 0);
    hex(line + 31, status & 0xffff);
    say(line);
    say(name);
    say("\r\n");
}

static char resident[] = "PROBE-PARENT-RESIDENT\r\n";

void pas_main(void)
{
    say("PROBE-PARENT-START\r\n");
    attempt("PIC.COM");
    say(resident);
    attempt("EXT.EXE");
    say(resident);
    attempt("PIC.COM");
    say(resident);
    attempt("FLAT.COM");
    say(resident);
    say("PROBE-PARENT-END\r\n");
}
