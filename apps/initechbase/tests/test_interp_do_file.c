/* initech-104j3, PRD 6.6: replay real III+ multi-file inputs through the
 * shipped REPL and PAL. do_file_test.sh compares the entire alternate file,
 * including CR/LF, spaces, error messages and DOS EOF, against GOLD.OUT. */
#include <stdio.h>
#include <unistd.h>
#include "samir/interp.h"

struct pal_host_cfg { uint8_t yy, mm, dd; uint32_t heap; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(samir_pal_t *);
static unsigned line;
static int32_t input(samir_pal_t *pal, char *s, uint32_t cap)
{
    const char *command = line++ == 0 ? "DO MINT" : "QUIT";
    (void)pal;
    return snprintf(s, cap, "%s", command);
}
static void output(samir_pal_t *pal, const char *s, uint32_t n)
{
    (void)pal;
    if (fwrite(s, 1, n, stdout) != n) _exit(2);
}
int main(int argc, char **argv)
{
    samir_pal_t *pal;
    xb_interp *ip;
    int rc;
    if (argc != 2 || chdir(argv[1])) return 2;
    pal = pal_host_make((struct pal_host_cfg){86, 7, 24, 2 * 1024 * 1024});
    if (!pal) return 2;
    pal->conin_line = input;
    pal->conout = output;
    ip = xb_interp_make(pal);
    if (!ip) return 2;
    rc = samir_repl(pal, ip);
    xb_interp_free(ip);
    pal_host_free(pal);
    return rc ? 2 : 0;
}
