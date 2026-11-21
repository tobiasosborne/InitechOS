/* Fixture for tests/console-symbols.sh: the shape of kernel/console.c and
 * kernel/startup.c in build/system/system.elf. The page is a static
 * 4000-byte g_console_page; g_int21_con is a 4-byte global pointer. */
static unsigned char g_console_page[80 * 25 * 2];
void *g_int21_con;
void *fixture_page(void) { g_int21_con = g_console_page; return g_console_page; }
