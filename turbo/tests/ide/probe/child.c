/* Position-independent flat child for the EXEC probe: compiled -fpie and
 * linked at zero, so it runs wherever the loader places a nested image.
 * Prints its load address and exits with code 7 (AH=4Ch). */
static const char text[] = "PROBE-CHILD-RAN at=00000000\r\n";

void _start(void)
{
    char line[sizeof text];
    for (unsigned i = 0; i < sizeof text; i++) line[i] = text[i];
    unsigned here = (unsigned)&_start;
    for (int i = 26; i >= 19; i--) { line[i] = "0123456789ABCDEF"[here & 15]; here >>= 4; }
    unsigned ax = 0x4000, bx = 1, cx = sizeof text - 1, dx = (unsigned)line;
    __asm__ volatile("int $0x21" : "+a"(ax), "+b"(bx), "+c"(cx), "+d"(dx) : : "cc", "memory");
    ax = 0x4c07;
    __asm__ volatile("int $0x21" : "+a"(ax) : : "cc", "memory");
    for (;;) {}
}
