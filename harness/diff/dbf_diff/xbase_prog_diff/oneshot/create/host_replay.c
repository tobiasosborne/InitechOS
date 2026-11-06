/* Factory replay when the full SAMIRX resident exceeds its loader envelope.
 * Exercises the same command hooks and PAL key stream as oneshot KEYS. */
#include "samir/kit.h"
#include "samir/rt.h"
struct pal_host_cfg { uint8_t date_yy, date_mm, date_dd; uint32_t heap_size; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
extern void pal_host_free(samir_pal_t *p);
extern int ui_modify_file(xb_interp *ip, const char *args, int *ec);
static samir_pal_t *host;
static char cells[25][81];
static const char *keys;
static unsigned keypos, keylen, linepos, capture;
static const char *const *lines;
static const char *const create_lines[] = {"CREATE WIDGETS", "LIST STRUCTURE", "QUIT", 0};
static const char *const modcmd_lines[] = {"MODIFY COMMAND HELLO", "DO HELLO", "QUIT", 0};
static const char *const memo_lines[] = {"CREATE MEMOS", "QUIT", 0};
static const char *const abort_lines[] = {"CREATE ABORTED", "QUIT", 0};
static const char *const prompt_lines[] = {"CREATE", "QUIT", 0};
static const char *const structure_lines[] = {
    "USE WIDGETS", "APPEND BLANK",
    "REPLACE PART WITH 'ALPHABETA', QTY WITH 42, DUE WITH CTOD('11/06/93'), OK WITH .T.",
    "DELETE", "MODIFY STRUCTURE", "QUIT", 0};
static const char *const text_lines[] = {"MODIFY COMMAND EDITING", "QUIT", 0};
static const char *const abandon_lines[] = {"MODIFY COMMAND EDITING", "QUIT", 0};
static const char *const empty_lines[] = {"MODIFY COMMAND EMPTY", "QUIT", 0};
static const char *const keepmemo_lines[] = {"USE WIDGETS", "GO 1",
    "REPLACE NOTES WITH 'memo survives'", "MODIFY STRUCTURE", "QUIT", 0};
static const char create_keys[] = "part\rc\r10\rqty\rn\r5\r0\r~due\rd\rok\rl\r\027\rn";
static const char modcmd_keys[] = "? \"HI\"\rreturn~\027";
static const char memo_keys[] = "note\rm\r\027\rn";
static const char abort_keys[] = "part\rc\r0\033";
static const char prompt_keys[] = "prompted\rname\rc\r12\r\r\rn";
static const char structure_keys[] = "\r\r5\rcount\r\r\r\r\r\r\r\rnotes\rm\r~\027\r";
/* End, backspace across LF, Home, overwrite, Ctrl-N, type, Ctrl-Y,
 * insert toggle, Esc/N, save. Extended DOS keys are 0 followed by scan. */
static const char text_keys[] = "\0O\0P\0G\010\0G\0RZ\016temp\031\0R!~\033n\027";
static const char abandon_keys[] = "changed\033y";
static const char empty_keys[] = "\027";
static const char keepmemo_keys[] = "\r\r7\r\027\r";
static const char file_keys[] = "plain text~\027";
static char replay[7000];
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
    char mode = argc > 1 ? argv[1][0] : 'c';
    keys = create_keys; keylen = sizeof create_keys - 1; lines = create_lines;
#define MODE(c, k, l) if (mode == (c)) { keys = (k); keylen = sizeof(k) - 1; lines = (l); }
    MODE('m', modcmd_keys, modcmd_lines)
    MODE('b', memo_keys, memo_lines)
    MODE('a', abort_keys, abort_lines)
    MODE('p', prompt_keys, prompt_lines)
    MODE('s', structure_keys, structure_lines)
    MODE('t', text_keys, text_lines)
    MODE('x', abandon_keys, abandon_lines)
    MODE('e', empty_keys, empty_lines)
    MODE('k', keepmemo_keys, keepmemo_lines)
    MODE('f', file_keys, empty_lines)
#undef MODE
    rt_memset(cells, ' ', sizeof cells);
    host = pal_host_make(cfg);
    if (!host) return 2;
    host->paint = paint; host->conin_char = raw_key; host->conin_line = command_line;
    host->gotoxy = cursor; host->set_attr = attr; host->conin_escape = 0;
    if (mode == 'r') {
        pal_fd fd = host->open(host, "KEYS.RAW", PAL_RD);
        int32_t n;
        if (fd < 0) return 4;
        n = host->read(host, fd, replay, sizeof replay);
        host->close(host, fd);
        if (n < 0) return 5;
        keys = replay; keylen = (unsigned)n; lines = text_lines;
    }
    ip = xb_interp_make(host);
    if (!ip) return 3;
    if (mode == 'f') { int ec = 0; rc = ui_modify_file(ip, "PLAIN", &ec); }
    else rc = samir_repl(host, ip);
    xb_interp_free(ip); pal_host_free(host);
    return rc < 0 || keypos != keylen;
}
