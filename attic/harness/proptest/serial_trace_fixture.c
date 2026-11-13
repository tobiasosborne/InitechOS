/* Ref: PRD Sec 8 / Rule 6. Independent original C formatter oracle for the
 * storage-only serial transform; cover both numeric limits and string syntax. */
#include <stdio.h>
#include <stdint.h>
static void serial_putc(char c) { putchar((unsigned char)c); }
static void serial_puts(const char *s) { fputs(s, stdout); }
static void serial_putu(uint32_t v) { printf("%u", v); }
static void serial_puti(int32_t v) { printf("%d", v); }
static void serial_puthex32(uint32_t v) { printf("%08X", v); }
static unsigned next_value(void) { static unsigned v=76; return ++v; }
int main(void)
{
    const char *s = "quoted \" \\ string";
    for (unsigned i = 0; i < 20; ++i) {
        serial_puts("FLAIR-TEST value="); serial_putu(i);
        serial_puts(" name="); serial_puts(s); serial_putc('\n');
    }
    serial_puts("limits "); serial_putu(UINT32_MAX);
    serial_putc(' '); serial_puti(INT32_MIN);
    serial_puts(" hex="); serial_puthex32(0xABCDEF01u);
    serial_puts(" adjacent" " literals\n");
    if (s) serial_puts("one\n"); else serial_puts("two\n");
    serial_puts("zero="); serial_putu(0u); serial_putc('\n');
    { serial_puts("FLAIR-REPEATED-ONE value="); serial_putu(1); }
    { serial_puts("FLAIR-REPEATED-TWO value="); serial_putu(2); }
    { serial_puts("FLAIR-REPEATED-THREE value="); serial_putu(3); }
    { serial_puts("FLAIR-REPEATED-FOUR value="); serial_putu(4); }
    if (s) serial_puts("controlled=");
    serial_putu(5);
    serial_putc('\n');
    serial_puts("side effect="); serial_putu(next_value());
    serial_putc('\n');
    return 0;
}
