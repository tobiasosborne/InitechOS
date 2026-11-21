/* Fixture for tests/console-symbols.sh: kernel/int21.c's 4-byte function
 * pointer that is also named g_console_page. Linked after page.c so that a
 * reader taking the last symbol of that name reads the wrong one. */
static void (*g_console_page)(void);
void fixture_set(void (*service)(void)) { g_console_page = service; }
void fixture_call(void) { if (g_console_page) g_console_page(); }
