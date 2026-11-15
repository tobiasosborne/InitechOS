/* factory/emu/console_symbols.h on the built system ELF: the page is the
 * 4000-byte g_console_page, never int21.c's 4-byte pointer of the same
 * name. The expected addresses come from nm -S (passed in), not this code. */
#include "../emu/console_symbols.h"
int main(int argc, char **argv)
{
    uint32_t page, con;
    if (argc != 4) return 2;
    unsigned long want_page = strtoul(argv[2], 0, 16), want_con = strtoul(argv[3], 0, 16);
    if (console_symbols(argv[1], &page, &con) != 0 || page != want_page || con != want_con) {
        printf("FAIL console symbols: page=%08x con=%08x want %08lx %08lx\n", page, con, want_page, want_con);
        printf("INITECH_CHECKS 1 failures=1\n");
        return 1;
    }
    printf("INITECH_CHECKS 1 failures=0\n");
    return 0;
}
