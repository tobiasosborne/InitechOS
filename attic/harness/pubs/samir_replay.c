/* PUB-REF-0001 GC-03. Same host PAL/interpreter route as dbf_diff.
 * stdout is the interpreter's bytes, without trimming or normalisation.
 *
 * Two routes, one tool (bead initech-ipu5.8 extends the ipu5.6 specimen
 * route rather than forking it):
 *
 *   samir_replay <file.prg>
 *       the program route (ipu5.6): samir_do over the query and SET modules,
 *       the source as one program; the InitechBase replay specimen.
 *   samir_replay session <dir> <file.dot>
 *       the dot-prompt route (ipu5.8, the InitechBase manuals, NT-17): the
 *       SHIPPED dot-prompt loop samir_repl (os/samir/samir_main.c, every
 *       module registered exactly as SAMIR.COM registers them) reads the
 *       lines of <file.dot> as the typed lines, after chdir(<dir>), the
 *       scratch copy of the sample diskette the caller prepared.  The cooked
 *       console of the shipped PAL echoes each typed line (pal.h conin_line:
 *       "line-edited, echoed"); the host PAL does not, so this route echoes
 *       the line and its newline after the ". " prompt, as the screen shows
 *       it.  At the end of the input the REPL has printed one more ". "
 *       prompt waiting for a line that never comes; that final prompt is not
 *       part of the transcript and is dropped (only it: anything else at the
 *       end is kept and compared).  A session that ends with QUIT has no
 *       such prompt and is kept whole.  A line read by ACCEPT, INPUT or WAIT is
 *       the next line of the file, echoed the same way.  The injected date
 *       is 03/01/92 (the edition date, Rule 11), as in the program route.
 *       Line ends: the shipped commands end their lines with CR LF (LIST,
 *       DISPLAY, REPORT FORM), which the screen shows exactly as LF; CR LF
 *       is therefore written as LF, and a CR before another CR (REPORT FORM
 *       ends a page with CR, then starts the next line with CR LF) moves
 *       the cursor nowhere and is dropped, as is a CR at the start of a
 *       line (LABEL FORM ends each row of labels with CR after its blank
 *       separating lines, os/samir/cmd/report.c lbl_run: the cursor is
 *       already in column 0).  Any other control byte (a CR after text on
 *       its line, a form feed, a tab) is RED: a transcript cannot show
 *       it. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "samir/interp.h"
struct pal_host_cfg { uint8_t yy,mm,dd; uint32_t heap_size; };
samir_pal_t *pal_host_make(struct pal_host_cfg);
void pal_host_free(samir_pal_t *);
int query_register(xb_interp *);
int set_register(xb_interp *);
static void output(samir_pal_t *p,const char *s,uint32_t n)
{ (void)p; if(fwrite(s,1,n,stdout)!=n) _Exit(2); }

/* ---- the dot-prompt route --------------------------------------------- */
static char *g_out; static size_t g_outn, g_outcap;
static FILE *g_in;
static int g_eof;
static void sess_out(samir_pal_t *p,const char *s,uint32_t n)
{
    (void)p;
    if (g_outn + n + 1 > g_outcap) {
        g_outcap = (g_outn + n + 1) * 2;
        g_out = realloc(g_out, g_outcap);
        if (!g_out) _Exit(2);
    }
    memcpy(g_out + g_outn, s, n); g_outn += n;
}
static int32_t sess_line(samir_pal_t *p,char *buf,uint32_t cap)
{
    if (cap == 0 || !fgets(buf, (int)cap, g_in)) { g_eof = 1; return -1; }
    size_t n = strcspn(buf, "\n");
    if (buf[n] != '\n' && !feof(g_in)) {
        fprintf(stderr, "!!! test-pubs RED SAMIR-REPLAY: typed line longer than %u bytes\n", cap - 1);
        _Exit(1);
    }
    buf[n] = 0;
    sess_out(p, buf, (uint32_t)n); sess_out(p, "\n", 1);
    return (int32_t)n;
}
static int32_t sess_key(samir_pal_t *p)
{
    char line[256];
    /* A single key (WAIT) is the first byte of the next line. */
    if (!fgets(line, sizeof line, g_in)) return -1;
    line[strcspn(line, "\n")] = 0;
    sess_out(p, line, (uint32_t)strlen(line)); sess_out(p, "\n", 1);
    return line[0] ? (unsigned char)line[0] : '\r';
}
static int session(const char *dir,const char *file)
{
    if (!(g_in = fopen(file, "rb")) || chdir(dir)) return 2;
    samir_pal_t *p = pal_host_make((struct pal_host_cfg){92,3,1,1048576});
    xb_interp *ip = p ? xb_interp_make(p) : NULL;
    if (!ip) return 2;
    p->conout = sess_out; p->conin_line = sess_line; p->conin_char = sess_key;
    int rc = samir_repl(p, ip);
    xb_interp_free(ip); pal_host_free(p); fclose(g_in);
    if (rc) { fprintf(stderr, "!!! test-pubs RED SAMIR-REPLAY: %s: REPL returned %d\n", file, rc); return 1; }
    if (g_eof) {
        if (g_outn < 2 || memcmp(g_out + g_outn - 2, ". ", 2)) {
            fprintf(stderr, "!!! test-pubs RED SAMIR-REPLAY: %s: input ended away from the dot prompt\n", file);
            return 1;
        }
        g_outn -= 2;
    }
    size_t w = 0;
    for (size_t r = 0; r < g_outn; r++) {
        unsigned char c = (unsigned char)g_out[r];
        if (c == '\r' && r + 1 < g_outn && (g_out[r + 1] == '\n' || g_out[r + 1] == '\r')) continue;
        if (c == '\r' && (w == 0 || g_out[w - 1] == '\n')) continue;
        if ((c < 32 && c != '\n') || c > 126) {
            fprintf(stderr, "!!! test-pubs RED SAMIR-REPLAY: %s: byte %u at offset %zu cannot be printed\n", file, c, r);
            return 1;
        }
        g_out[w++] = (char)c;
    }
    g_outn = w;
    return fwrite(g_out, 1, g_outn, stdout) == g_outn ? 0 : 2;
}

int main(int argc,char **argv)
{
    char src[65536]; size_t n; FILE *f;
    if(argc==4 && !strcmp(argv[1],"session")) return session(argv[2],argv[3]);
    if(argc!=2 || !(f=fopen(argv[1],"rb"))) return 2;
    n=fread(src,1,sizeof(src)-1,f);
    if(ferror(f) || !feof(f)) { fclose(f); return 2; }
    fclose(f);src[n]=0;
    samir_pal_t *p=pal_host_make((struct pal_host_cfg){92,3,1,1048576});
    xb_interp *ip=p?xb_interp_make(p):NULL;
    if(!ip) return 2;
    p->conout=output;
    int rc=query_register(ip);
    if(!rc) rc=set_register(ip);
    if(!rc) rc=samir_do(ip,src);
    if(rc) fprintf(stderr,"!!! test-pubs RED SAMIR-REPLAY: %s error=%d\n",argv[1],samir_last_error(ip));
    xb_interp_free(ip);pal_host_free(p);
    return rc?1:0;
}
