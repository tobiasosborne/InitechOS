/* Host integration driver for ASSIST/HELP while the shared EXE is linking.
 * Factory code: libc allowed. Uses the real engine, kit, and UI modules. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "samir/interp.h"
#include "samir/pal.h"

struct pal_host_cfg { uint8_t yy, mm, dd; uint32_t heap; };
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void pal_host_free(samir_pal_t *p);

static char screen[25][81];
static int row, col, failures, step, total, line;
static const char *destination;
static const char *const lines[] = {"ASSIST", "HELP USE", "HELP", "HELP NONSENSE", "QUIT"};
struct event { int key; const char *expect, *dump; };
static struct event events[300];

static void snapshot(const char *name)
{
    char path[512];
    FILE *f;
    int r;
    snprintf(path, sizeof(path), "%s/%s.txt", destination, name);
    f = fopen(path, "w");
    if (!f) { perror(path); exit(2); }
    for (r = 0; r < 25; r++) fprintf(f, "%s\n", screen[r]);
    fclose(f);
}
static int contains(const char *s)
{
    int r;
    for (r = 0; r < 25; r++) if (strstr(screen[r], s)) return 1;
    return 0;
}
static int32_t read_key(samir_pal_t *p)
{
    struct event e;
    (void)p;
    if (step == total) return -1;
    e = events[step++];
    if (e.dump) snapshot(e.dump);
    if (e.expect && !contains(e.expect)) {
        fprintf(stderr, "event %d: missing <%s>\n", step-1, e.expect);
        snapshot("failure"); failures++;
    }
    return e.key;
}
static int32_t read_line(samir_pal_t *p, char *b, uint32_t cap)
{
    uint32_t n;
    (void)p;
    if (line == (int)(sizeof(lines)/sizeof(lines[0]))) return -1;
    n = (uint32_t)strlen(lines[line]);
    if (n >= cap) return -1;
    memcpy(b, lines[line++], n+1);
    return (int32_t)n;
}
static void cursor(samir_pal_t *p, uint8_t r, uint8_t c)
{ (void)p; row = r; col = c; }
static void attribute(samir_pal_t *p, uint8_t a)
{ (void)p; (void)a; }
static int paint(samir_pal_t *p, uint8_t r, uint8_t c, const char *s, uint32_t n)
{
    (void)p;
    if (r >= 25 || c+n > 80) { failures++; return -1; }
    memcpy(screen[r]+c, s, n);
    return 0;
}
static void output(samir_pal_t *p, const char *s, uint32_t n)
{
    uint32_t i;
    (void)p;
    for (i = 0; i < n; i++) {
        if (s[i] == '\n') { row++; col = 0; }
        else if (s[i] == '\r') col = 0;
        else {
            if (row >= 0 && row < 25 && col >= 0 && col < 80) screen[row][col] = s[i];
            if (++col == 80) { col = 0; row++; }
        }
    }
}
static void add(int key, const char *expect, const char *dump)
{ events[total++] = (struct event){key,expect,dump}; }
static void type(const char *text)
{ while (*text) add((unsigned char)*text++, NULL, NULL); }

int main(int argc, char **argv)
{
    samir_pal_t *p;
    xb_interp *ip;
    int i, rc;
    if (argc != 2) return 2;
    destination = argv[1];
    for (i = 0; i < 25; i++) { memset(screen[i], ' ', 80); screen[i][80] = 0; }
    add(256|0x3b, "Database file", "menu1");
    add(27,"USE [<file>]", "assisthelp");
    add(13, "Database file", NULL); type("CLIENTS"); add(13,NULL,NULL);
    add('N', "Is the file indexed?", "indexed");
    add(4, "CLIENTS", "opened"); add(4,NULL,NULL); add(4,NULL,NULL);
    add(256|0x4f,NULL,NULL); add(13,NULL,NULL); type("2"); add(13,NULL,NULL);
    add(19,"Rec: 2/3", "goto");
    add(24,NULL,NULL); add(24,NULL,NULL); add(24,NULL,NULL); add(13,NULL,NULL);
    add(23,"BROWSE", "browse"); add(4,NULL,NULL); add(4,NULL,NULL);
    add(13,"List",NULL); add(13,"Execute the command", "command");
    add(27,"PESTON", "list");
    add(3,"USE [<file>]", "help1");
    add(18,"LIST [<scope>]", "next");
    add(27,"USE [<file>]", "previous");
    add(24,"MAIN MENU", "help2"); add(13,NULL,NULL);
    add(27,"LIST [<scope>]", "selected");
    add(27,"No help available for", "unknown");
    p = pal_host_make((struct pal_host_cfg){93,7,14,1048576});
    if (!p) return 2;
    p->conin_char=read_key; p->conin_line=read_line;
    p->conout=output; p->paint=paint; p->gotoxy=cursor; p->set_attr=attribute;
    ip = xb_interp_make(p);
    if (!ip) return 2;
    rc = samir_repl(p,ip);
    snapshot("after");
    if (rc || step != total || failures) {
        fprintf(stderr,"host check failed: rc=%d keys=%d/%d failures=%d\n",rc,step,total,failures);
        failures++;
    }
    xb_interp_free(ip); pal_host_free(p);
    if (failures) return 1;
    puts("GREEN: real-engine ASSIST/HELP host keyboard integration");
    return 0;
}
