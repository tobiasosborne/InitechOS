/*
 * harness/tbx/tbx32_rt.c -- the freestanding i386 Linux run-time of the
 * binding oracles (see tbx32_rt.h). Factory C. ASCII-clean (Rule 12).
 *
 * Linux i386 ABI facts used (Law 1: the kernel's own uapi headers,
 * asm/sigcontext.h + asm/ucontext.h for i386, asm/unistd_32.h):
 *   syscalls: exit 1, read 3, write 4, open 5, close 6, old_mmap 90
 *             (one pointer to the six arguments), rt_sigreturn 173,
 *             rt_sigaction 174, exit_group 252;
 *   struct ucontext { u32 uc_flags; u32 uc_link; stack_t uc_stack (12);
 *                     struct sigcontext uc_mcontext; ... } -> the
 *   sigcontext at byte 20, as u32 words: gs fs es ds edi esi ebp esp ebx
 *   edx ecx eax trapno err eip ... (indices 0..14);
 *   SA_SIGINFO 4, SA_RESTORER 0x04000000; SIGSEGV 11.
 */
#include "tbx32_rt.h"
#include "tbxmodel.h"

tbx32_trap_fn   tbx32_on_trap;
tbx32_serial_fn tbx32_on_serial;

/* ------------------------------------------------------------- syscalls */
static long sys3(long n, long a, long b, long c)
{
    long r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c)
                     : "memory");
    return r;
}

static long sys4(long n, long a, long b, long c, long d)
{
    long r;
    __asm__ volatile("int $0x80" : "=a"(r)
                     : "a"(n), "b"(a), "c"(b), "d"(c), "S"(d) : "memory");
    return r;
}

/* --------------------------------------------------------------- output */
static char g_out[65536];
static int  g_outn;

void tbx32_flush(void)
{
    int p = 0;
    while (p < g_outn) {
        long w = sys3(4, 1, (long)(g_out + p), g_outn - p);
        if (w <= 0) break;
        p += (int)w;
    }
    g_outn = 0;
}

void tm_write(const char *s, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        if (g_outn == (int)sizeof g_out) tbx32_flush();
        g_out[g_outn++] = s[i];
    }
}

void tbx32_exit(int rc)
{
    tbx32_flush();
    sys3(252, rc, 0, 0);
    for (;;) sys3(1, rc, 0, 0);
}

void tbx32_die(const char *msg)
{
    tm_puts("TBX32-FATAL ");
    tm_puts(msg);
    tm_puts("\n");
    tbx32_exit(2);
}

int tbx32_read_file(const char *path, char *buf, int cap)
{
    long fd = sys3(5, (long)path, 0, 0), got, n = 0;
    if (fd < 0) return -1;
    while (n < cap) {
        got = sys3(3, fd, (long)(buf + n), cap - n);
        if (got < 0) { sys3(6, fd, 0, 0); return -1; }
        if (got == 0) break;
        n += got;
    }
    if (n == cap) {               /* a file that fills the buffer is too big */
        char c;
        got = sys3(3, fd, (long)&c, 1);
        if (got != 0) { sys3(6, fd, 0, 0); return -1; }
    }
    sys3(6, fd, 0, 0);
    return (int)n;
}

void *tbx32_map_fixed(uint32_t addr, uint32_t len)
{
    /* old_mmap: addr, len, PROT_READ|WRITE|EXEC = 7,
     * MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS = 0x32, fd -1, offset 0 */
    uint32_t a[6];
    long r;
    a[0] = addr; a[1] = len; a[2] = 7u; a[3] = 0x32u; a[4] = 0xFFFFFFFFu; a[5] = 0u;
    r = sys3(90, (long)a, 0, 0);
    if ((uint32_t)r != addr) return 0;
    return (void *)(uintptr_t)addr;
}

/* --------------------------------------------------------- the catcher */
struct ksigaction {
    void    *handler;
    uint32_t flags;
    void    *restorer;
    uint32_t mask[2];
};

extern void tbx32_restorer(void);
__asm__(".text\n"
        ".globl tbx32_restorer\n"
        "tbx32_restorer:\n"
        "    mov $173, %eax\n"
        "    int $0x80\n");

static void put_hex8(const char *k, uint32_t v)
{
    tm_puts(k);
    tm_putx(v, 8);
}

static void on_segv(int sig, void *info, void *ucv)
{
    uint32_t *mc = (uint32_t *)((char *)ucv + 20);
    uint32_t eip = mc[14];
    const uint8_t *ip = (const uint8_t *)(uintptr_t)eip;
    (void)sig; (void)info;
    if (ip[0] == 0xCD && ip[1] == 0x81 && tbx32_on_trap) {
        mc[11] = (uint32_t)tbx32_on_trap(mc[11] & 0xFFFFu,
                                         (const uint32_t *)(uintptr_t)mc[7],
                                         mc[7], mc[6]);
        mc[14] = eip + 2u;
        return;
    }
    if (ip[0] == 0xEE) {           /* out dx, al: a COM1 byte */
        if (tbx32_on_serial && (mc[9] & 0xFFFFu) == 0x3F8u)
            tbx32_on_serial((uint8_t)(mc[11] & 0xFFu));
        mc[14] = eip + 1u;
        return;
    }
    tm_puts("TBX CRASH");
    put_hex8(" eip=0x", eip);
    put_hex8(" trapno=0x", mc[12]);
    put_hex8(" err=0x", mc[13]);
    put_hex8(" bytes=0x", (uint32_t)ip[0] | ((uint32_t)ip[1] << 8));
    tm_puts("\n");
    tbx32_exit(3);
}

void tbx32_install(void)
{
    struct ksigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.handler = (void *)on_segv;
    sa.flags = 0x04000000u | 4u;              /* SA_RESTORER | SA_SIGINFO */
    sa.restorer = (void *)tbx32_restorer;
    if (sys4(174, 11, (long)&sa, 0, 8) != 0)
        tbx32_die("rt_sigaction(SIGSEGV) failed");
}

/* tbx32_call(fn, ebx): the os/milton/tbx_gate.asm tbx_tenant_call contract
 * (EBX = PSP, callee-saved registers preserved for the caller, DF = 0). */
__asm__(".text\n"
        ".globl tbx32_call\n"
        "tbx32_call:\n"
        "    push %ebp\n"
        "    mov  %esp, %ebp\n"
        "    push %ebx\n"
        "    push %esi\n"
        "    push %edi\n"
        "    mov  8(%ebp), %eax\n"
        "    mov  12(%ebp), %ebx\n"
        "    cld\n"
        "    call *%eax\n"
        "    lea  -12(%ebp), %esp\n"
        "    pop  %edi\n"
        "    pop  %esi\n"
        "    pop  %ebx\n"
        "    pop  %ebp\n"
        "    ret\n");

/* ------------------------------------------------------------- the entry */
__asm__(".text\n"
        ".globl _start\n"
        "_start:\n"
        "    xor  %ebp, %ebp\n"
        "    mov  %esp, %eax\n"
        "    and  $-16, %esp\n"
        "    sub  $12, %esp\n"
        "    push %eax\n"
        "    call tbx32_start\n"
        "    hlt\n");

void tbx32_start(uint32_t *sp);
void tbx32_start(uint32_t *sp)
{
    int argc = (int)sp[0];
    char **argv = (char **)(sp + 1);
    tbx32_exit(tbx32_main(argc, argv));
}

/* --------------------------------------------- what gcc may call itself */
void *memcpy(void *d, const void *s, uint32_t n)
{
    uint8_t *dp = (uint8_t *)d;
    const uint8_t *sp = (const uint8_t *)s;
    while (n--) *dp++ = *sp++;
    return d;
}

void *memmove(void *d, const void *s, uint32_t n)
{
    uint8_t *dp = (uint8_t *)d;
    const uint8_t *sp = (const uint8_t *)s;
    if (dp < sp) { while (n--) *dp++ = *sp++; }
    else { dp += n; sp += n; while (n--) *--dp = *--sp; }
    return d;
}

void *memset(void *d, int c, uint32_t n)
{
    uint8_t *dp = (uint8_t *)d;
    while (n--) *dp++ = (uint8_t)c;
    return d;
}

int memcmp(const void *a, const void *b, uint32_t n)
{
    const uint8_t *x = (const uint8_t *)a, *y = (const uint8_t *)b;
    uint32_t i;
    for (i = 0; i < n; i++)
        if (x[i] != y[i]) return (int)x[i] - (int)y[i];
    return 0;
}
