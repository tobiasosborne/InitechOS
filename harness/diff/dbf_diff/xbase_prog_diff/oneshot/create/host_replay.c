/* Factory replay when the full SAMIRX resident exceeds its loader envelope.
 * Exercises the same command hooks and PAL key stream as oneshot KEYS. */
#include "samir/kit.h"
#include "samir/rt.h"
struct pal_host_cfg { uint8_t date_yy, date_mm, date_dd; uint32_t heap_size; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
extern void pal_host_free(samir_pal_t *p);
static samir_pal_t *host;
static char cells[25][81];
static const char *keys;
static unsigned keypos, keylen, linepos, capture;
static const char *const *lines;
static const char *const create_lines[] = {"CREATE WIDGETS", "LIST STRUCTURE", "QUIT", 0};
static const char *const modcmd_lines[] = {"MODIFY COMMAND HELLO", "DO HELLO", "QUIT", 0};
static const char create_keys[] = "part\rc\r10\rqty\rn\r5\r0\r~due\rd\rok\rl\r\027\rn";
static const char modcmd_keys[] = "? \"HI\"\rreturn~\027";
static int paint(samir_pal_t *p, uint8_t r, uint8_t c, const char *s, uint32_t n)
{
    (void)p;
    if (r >= 25 || c >= 80) return 0;
    if (n > 80u - c) n = 80u - c;
    rt_memcpy(cells[r] + c, s, n); return 0;
}
static int32_t raw_key(samir_pal_t *p)
{
    int c;
    (void)p;
    if (keypos >= keylen) return -1;
    c = (unsigned char)keys[keypos++];
    if (c == '~') {
        char name[] = "host-screen0.txt"; pal_fd f;
        name[11] = (char)('0' + capture++);
        f = host->open(host, name, PAL_WR | PAL_CREATE | PAL_TRUNC);
        if (f >= 0) {
            for (int r = 0; r < 25; r++) { cells[r][80] = '\n'; host->write(host, f, cells[r], 81); }
            host->close(host, f);
        }
        return raw_key(p);
    }
    return c;
}
static int32_t command_line(samir_pal_t *p, char *out, uint32_t cap)
{
    const char *s = lines[linepos]; unsigned n;
    (void)p;
    if (!s) return -1;
    linepos++; n = rt_strlen(s);
    if (n >= cap) return -1;
    rt_memcpy(out, s, n + 1); return (int32_t)n;
}
static void cursor(samir_pal_t *p, uint8_t r, uint8_t c) { (void)p; (void)r; (void)c; }
static void attr(samir_pal_t *p, uint8_t a) { (void)p; (void)a; }
int main(int argc, char **argv)
{
    struct pal_host_cfg cfg = {93, 1, 1, 1024u * 1024u};
    xb_interp *ip; int rc;
    int modcmd = argc > 1 && argv[1][0] == 'm';
    keys = modcmd ? modcmd_keys : create_keys;
    keylen = modcmd ? sizeof modcmd_keys - 1 : sizeof create_keys - 1;
    lines = modcmd ? modcmd_lines : create_lines;
    rt_memset(cells, ' ', sizeof cells);
    host = pal_host_make(cfg);
    if (!host) return 2;
    host->paint = paint; host->conin_char = raw_key; host->conin_line = command_line;
    host->gotoxy = cursor; host->set_attr = attr; host->conin_escape = 0;
    ip = xb_interp_make(host);
    if (!ip) return 3;
    rc = samir_repl(host, ip);
    xb_interp_free(ip); pal_host_free(host);
    return rc < 0 || keypos != keylen;
}
