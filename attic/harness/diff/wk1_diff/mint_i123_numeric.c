/* Input-only builder for genuine Lotus R2.2 numeric mints. The output is
 * NOT an oracle: real 123.EXE must execute its automatic macro and save it.
 * Macro keys follow the existing corpus mint/MINT02.steps; \0 automatic
 * execution is exercised by the real executable, not implemented here.
 */
#include <stdio.h>
#include <string.h>
#include "i123.h"
static unsigned char mem[1048576], buf[65536];
static i123_sheet_t sh;
static unsigned line;
static void command(const char *s)
{
    i123_set_label(&sh, 10, (uint16_t)line++, '\'', s, (uint32_t)strlen(s));
}
int main(int argc, char **argv)
{
    unsigned n;
    char s[241];
    if (argc != 3) return 2;
    i123_sheet_init(&sh, mem, sizeof mem, 2048);
    if (!strcmp(argv[1], "entry")) {
        static const char *const entries[] = {"$12.34", "-$12.34", "+$12.34", "$-12.34", "$+12.34",
            "$12.5%", "$1.25E2", "$$12", "12$34", "$", "$A$1", "+$A$1", "$0", "$.5"};
        for (unsigned i = 0; i < sizeof entries / sizeof entries[0]; i++) {
            snprintf(s, sizeof s, "{GOTO}A%u~%s~{ESC}{ESC}", i + 1, entries[i]);
            command(s);
        }
        command("/fsNUMENTRY~~{GOTO}A1~");
    } else if (!strcmp(argv[1], "format")) {
        unsigned row = 0;
        command("{GOTO}A1~/wcs30~{GOTO}B1~/wcs40~");
        for (unsigned type = 0; type < 4; type++) {
            const char *formats[] = {"f", "c", ",", "p"};
            for (unsigned dec = type == 3 ? 10 : 12; dec <= 15; dec++) {
                row++;
                snprintf(s, sizeof s, "{GOTO}A%u~1234.5678912345~/rf%s%u~~{GOTO}B%u~@STRING(1234.5678912345%s,%u)~",
                    row, formats[type], dec, row, type == 3 ? "*100" : "", dec);
                command(s);
            }
        }
        static const char *const values[] = {"-1234.5678912345", "0", "9.999999999999996", "-0.0000000000000001",
            "1234.625", "-1234.625", "999999999999999", "1E20"};
        for (unsigned i = 0; i < sizeof values / sizeof values[0]; i++) {
            row++;
            snprintf(s, sizeof s, "{GOTO}A%u~%s~/rff15~~{GOTO}B%u~@STRING(%s,15)~", row, values[i], row, values[i]);
            command(s);
        }
        command("/fsNUMFMT~~{GOTO}A1~");
    } else return 2;
    i123_name_set(&sh, "\\0", (i123_rect_t){10, 0, 10, (uint16_t)(line - 1)});
    if (i123_wk1_write(&sh, buf, sizeof buf, &n)) return 1;
    FILE *f = fopen(argv[2], "wb");
    if (!f) return 1;
    int ok = fwrite(buf, 1, n, f) == n;
    return fclose(f) || !ok;
}
