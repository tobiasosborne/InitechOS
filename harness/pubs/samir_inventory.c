/* PUB-REF-0001 GC-01 / CP-11 (bead initech-ipu5.8): the shipped InitechBase
 * surface, asked of the ACTUAL dot-prompt dispatcher rather than copied into
 * a second list.  The same single-route pattern as dos_surface_inventory.c:
 * candidate words are read from stdin, one per line (the Makefile feeds every
 * upper-case word of the string literals and packed "A\0B\0" grammar tables of
 * os/samir/samir_main.c, the cmd and core sources and the samir headers:
 * harness/pubs/samir_candidates.sh, initech-u01i), and a word is shipped iff
 * the real code accepts it, under its full name (a four-letter-or-longer
 * shortening of another accepted candidate is that name; see the end of main):
 *
 *   NAME|command   samir_repl (samir_main.c: the REPL-owned verbs, then the
 *                  registered hook chain query/mutate/set/proc/report behind
 *                  the flow.c spine) given the line "NAME" -- or "NAME X" --
 *                  does NOT answer "*** Unrecognized command verb." (#16);
 *                  failing both, "NAME WORD" for every candidate WORD, since
 *                  a verb may exist only with its keyword (COPY FILE,
 *                  mutate.c d_file; CLEAR MEMORY, flow.c exec_command:
 *                  "COPY" and "CLEAR" alone are #16, Using III+ U5-59/U5-51)
 *   NAME|function  the line "? NAME()" does not answer #31
 *                  "Invalid function name." (fn_builtins.c xb_call_builtin
 *                  and the IIF special form of eval.c)
 *   NAME|set       "SET NAME <probe>" is accepted by the SET module for one
 *                  of the probe operands ON, OFF, TO, TO 0, TO 2, AMERICAN, TO X:
 *                  the line is answered with neither #16 nor the code the
 *                  shipped REPL prints for an option set.c does not know
 *                  (set_cmd_hook returns -INTERP_ERR_SYNTAX with no catalog
 *                  ordinal, and samir_main.c renders that engine code 7 as
 *                  "7  File already exists."; observed, not endorsed -- the
 *                  books never print it)
 *
 * Every probe is a fresh interpreter over the host PAL (pal_host.c, the
 * factory twin of pal_milton.c) running samir_repl -- the shipped dot-prompt
 * loop -- inside a scratch directory, and is asked twice if need be: with the
 * probe line alone, and after "USE PROBE", a one-field, one-record database
 * the probe writes there with the shipped writer (dbf_create), since some
 * options (SET ORDER) answer only with a database in USE.  The database is
 * written afresh before each such probe and removed at the end, so no probe
 * sees what another changed and nothing is left behind.  Symbol commands are
 * candidates too: the Makefile adds ? ?? @ ! & = and the comment forms.
 * Output: NAME|KIND lines (the Makefile sorts them).
 *
 * `samir_inventory messages` prints the shipped message catalogue instead,
 * n|text for every non-empty decoded catalogue entry -- the very table
 * repl_render_error prints from (GC-02).  samir_main.c is included whole for
 * that (the single-TU pattern of int21_inventory.c); the rest of the engine
 * is linked.  Host C (Law 3). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "samir/interp.h"
#include "samir/dbf.h"
#include "../../os/samir/samir_main.c"
struct pal_host_cfg { uint8_t yy,mm,dd; uint32_t heap_size; };
samir_pal_t *pal_host_make(struct pal_host_cfg);
void pal_host_free(samir_pal_t *);

static char g_out[16384];
static size_t g_outn;
static const char *g_lines[2];
static int g_nlines, g_fed;

static void cap_out(samir_pal_t *p,const char *s,uint32_t n)
{
    (void)p;
    if (g_outn + n >= sizeof g_out) n = (uint32_t)(sizeof g_out - 1 - g_outn);
    memcpy(g_out + g_outn, s, n); g_outn += n; g_out[g_outn] = 0;
}
static int32_t feed_line(samir_pal_t *p,char *buf,uint32_t cap)
{
    (void)p;
    if (g_fed >= g_nlines) return -1;
    const char *l = g_lines[g_fed++];
    size_t n = strlen(l);
    if (n >= cap) n = cap - 1;
    memcpy(buf, l, n); buf[n] = 0;
    return (int32_t)n;
}
static int32_t no_key(samir_pal_t *p) { (void)p; return -1; }

/* The probe database: PROBE.DBF, one character field, one record. */
static void probe_table(void)
{
    samir_pal_t *p = pal_host_make((struct pal_host_cfg){92,3,1,1048576});
    dbf_field_spec f[1] = { { "NAME", 'C', 10u, 0u } };
    dbf_table *t = NULL;
    xb_val v[1];
    if (!p || dbf_create(p, "PROBE.DBF", f, 1, &t) != DBF_OK) { fprintf(stderr, "!!! samir_inventory: PROBE.DBF\n"); exit(2); }
    v[0] = xb_c("PROBE", 5);
    if (dbf_append_rec(t, v, 0) != DBF_OK || dbf_flush(t) != DBF_OK) { fprintf(stderr, "!!! samir_inventory: PROBE.DBF\n"); exit(2); }
    dbf_close(t); pal_host_free(p);
}
static const char *run(int nlines, const char *a, const char *b);
/* Run one line through a fresh shipped REPL, without and then with a
 * database in USE; the first answer g_accept takes is returned. */
static int (*g_accept)(const char *);
static const char *probe(const char *line)
{
    const char *out = run(1, line, NULL);
    if (g_accept(out)) return out;
    probe_table();
    return run(2, "USE PROBE", line);
}
static const char *run(int nlines, const char *a, const char *b)
{
    samir_pal_t *p = pal_host_make((struct pal_host_cfg){92,3,1,1048576});
    if (!p) { fprintf(stderr, "!!! samir_inventory: PAL\n"); exit(2); }
    p->conout = cap_out; p->conin_line = feed_line; p->conin_char = no_key;
    xb_interp *ip = xb_interp_make(p);
    if (!ip) { fprintf(stderr, "!!! samir_inventory: interp\n"); exit(2); }
    g_outn = 0; g_out[0] = 0; g_lines[0] = a; g_lines[1] = b; g_nlines = nlines; g_fed = 0;
    if (samir_repl(p, ip) != INTERP_OK) { fprintf(stderr, "!!! samir_inventory: REPL\n"); exit(2); }
    xb_interp_free(ip); pal_host_free(p);
    return g_out;
}
static int unrecognized(const char *out) { return strstr(out, "16  *** Unrecognized command verb.") != NULL; }
/* An unknown SET option is "10  Syntax error." (real III+ 1.1 SF-unknown
 * capture, fns-mint/unknown); before initech-ipu5.8.1 SAMIR leaked "7". */
static int set_refused(const char *out) { return unrecognized(out) || strstr(out, "10  Syntax error.") != NULL; }
static int accepted_cmd(const char *out) { return !unrecognized(out); }
static int accepted_fn(const char *out) { return !strstr(out, "31  Invalid function name."); }
static int accepted_set(const char *out) { return !set_refused(out); }

int main(int argc,char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "messages")) {
        char m[256];
        for (int code = 0; code <= REPL_MSG_COUNT; code++) {
            (void)samir_message((unsigned)code, m, sizeof m);
            if (code && *m) printf("%d|%s\n", code, m);
        }
        return ferror(stdout) ? 1 : 0;
    }
    char word[64], line[160];
    static const char *const ops[] = { "ON", "OFF", "TO", "TO 0", "TO 2", "AMERICAN", "TO X" };
    /* Calibration (Rule 2): the probe must tell a shipped word from an
     * unshipped one, or every answer below is noise. */
    int bad;
    g_accept = accepted_cmd;
    bad = !unrecognized(probe("XYZZY")) || unrecognized(probe("LIST"));
    g_accept = accepted_fn;
    bad |= !strstr(probe("? XYZZY()"), "31  Invalid function name.") || strstr(probe("? UPPER()"), "31  Invalid function name.") != NULL;
    g_accept = accepted_set;
    bad |= !set_refused(probe("SET XYZZY ON")) || set_refused(probe("SET EXACT ON")) || set_refused(probe("SET ORDER TO 0"));
    if (bad) {
        fprintf(stderr, "!!! samir_inventory: probe calibration failed\n");
        return 2;
    }
    /* Every candidate is also a keyword candidate for the two-word form. */
    static char words[1024][64]; int nwords = 0;
    enum { K_CMD = 1, K_FN = 2, K_SET = 4 };
    static unsigned char kinds[1024];
    while (fgets(word, sizeof word, stdin)) {
        word[strcspn(word, "\r\n")] = '\0';
        if (!word[0]) continue;
        if (nwords == 1024) { fprintf(stderr, "!!! samir_inventory: too many candidates\n"); return 2; }
        snprintf(words[nwords++], sizeof words[0], "%s", word);
    }
    for (int wi = 0; wi < nwords; wi++) {
        snprintf(word, sizeof word, "%s", words[wi]);
        /* SET is the verb of the SET <option> entries (NAME|set below); the
         * bare verb has no form of its own to document. */
        if (!strcmp(word, "SET")) goto options;
        g_accept = accepted_cmd;
        if (!unrecognized(probe(word))) kinds[wi] |= K_CMD;
        else {
            snprintf(line, sizeof line, "%s X", word);
            if (!unrecognized(probe(line))) kinds[wi] |= K_CMD;
            else if (word[0] >= 'A' && word[0] <= 'Z')
                for (int ki = 0; ki < nwords; ki++) {
                    if (words[ki][0] < 'A' || words[ki][0] > 'Z') continue;
                    snprintf(line, sizeof line, "%s %s", word, words[ki]);
                    if (!unrecognized(probe(line))) { kinds[wi] |= K_CMD; break; }
                }
        }
    options:
        if (word[0] >= 'A' && word[0] <= 'Z') {
            g_accept = accepted_fn;
            snprintf(line, sizeof line, "? %s()", word);
            if (!strstr(probe(line), "31  Invalid function name.")) kinds[wi] |= K_FN;
            g_accept = accepted_set;
            for (size_t i = 0; i < sizeof ops / sizeof ops[0]; i++) {
                snprintf(line, sizeof line, "SET %s %s", word, ops[i]);
                if (!set_refused(probe(line))) { kinds[wi] |= K_SET; break; }
            }
        }
    }
    remove("PROBE.DBF");
    /* Full names only.  Since initech-13d3 (9261bac8) the dispatcher takes
     * any prefix of four or more letters of a command, keyword or SET option
     * (rt_kw_match, os/samir/core/rt.c), as real dBASE III PLUS 1.1 does
     * (../dbase3-decomp specs/runtime/interpreter-and-execution-model.md
     * Sec 3.1: the verb table "holds the full spellings"; graded by the real
     * abbrev/abbrfull mints).  So a candidate that is such a prefix of
     * another candidate accepted as the same kind -- REST of RESTORE, SET
     * DELETE of SET DELETED, PARAMETER of PARAMETERS -- is that name typed
     * short, not a name of its own; the books document the rule once
     * (Using InitechBase u2-commands), never each shortening. */
    static const char *const kname[] = { "command", "function", "set" };
    for (int wi = 0; wi < nwords; wi++)
        for (int k = 0; k < 3; k++) {
            size_t n = strlen(words[wi]);
            int shortening = 0;
            if (!(kinds[wi] & (1 << k))) continue;
            for (int vi = 0; vi < nwords && !shortening && n >= 4; vi++)
                shortening = vi != wi && (kinds[vi] & (1 << k)) && strlen(words[vi]) > n &&
                             !strncmp(words[vi], words[wi], n);
            if (!shortening) printf("%s|%s\n", words[wi], kname[k]);
        }
    return ferror(stdout) ? 1 : 0;
}
