/*
 * pubs_msgtable.c -- emit the InitechDOS message list for the manuals from
 * the LOCKED catalogue spec/dos_messages.json (factory, C-only).
 *
 * Bead initech-ipu5.5.  INITECH-DOCUMENTATION-SET.md Sec 0 rule 3: message
 * appendices are generated from spec/ so a manual cannot drift from the
 * software.  ADR-0003 DEC-13 makes the catalogue a controlled vocabulary; this
 * tool copies each text verbatim (only escaping it for TeX) and sorts the list
 * alphabetically, as the period message appendices were.
 *
 * Entries whose text begins with a space or '<' are DIR display fragments
 * (MSG-DOS-0021..0026: " Volume in drive A is ", "<DIR>", ...), not messages a
 * user looks up; they are left out of the list and counted on stderr.
 * '%c' (a drive-letter substitution, per the catalogue's _comment) prints as
 * the placeholder \ph{d}.
 *
 * Two CHKDSK messages are completed on the screen by a value the command
 * supplies (os/milton/chkdsk.c).  A text beginning with ',' is the tail of
 * the message of the catalogue id just before it, which ends in a space:
 * MSG-DOS-0044 "Cannot CHDIR to " + path + MSG-DOS-0045 ", tree past this
 * point not processed".  The tail is joined to its head as ONE list entry
 * under the head's id, the path printed as \ph{path} (the form of the IBM
 * DOS 3.10 Reference, p.7-48 CHKDSK: "Cannot CHDIR to d:path, tree past this
 * point not processed"), and counted with the display fragments.  A message
 * that still ends in a space is followed by a number (MSG-DOS-0043 "on
 * cluster " n; IBM 3.10 Reference p.A-46 "on cluster xx"), printed \ph{n}.
 * A tail with no head that ends in a space fails loud (Rule 2).
 *
 * Usage: pubs_msgtable <dos_messages.json> [all|device|other] > out.tex
 * The optional selector splits the list the way the IBM Messages appendix is
 * split (PUB-REF-0001 AN-03; IBM 3.10 Reference App. A): `device` = the
 * device error messages (text containing "reading drive %c:" or "writing
 * drive %c:") and their response prompt "Abort, Retry, Fail?"; `other` =
 * everything else; `all` (the default) = both.  Bead initech-ipu5.7.
 * Exit 0 ok; 1 malformed catalogue (fail loud, Rule 2); 2 usage / I/O.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXMSG 256

struct msg { char id[32]; char text[256]; };

static int cmp(const void *a, const void *b)
{
    const struct msg *x = a, *y = b;
    int c = strcmp(x->text, y->text);
    return c ? c : strcmp(x->id, y->id);
}

/* Read a JSON string starting at *p (which points at the opening quote). */
static int read_string(const char **p, char *out, size_t cap)
{
    const char *s = *p;
    size_t n = 0;
    if (*s != '"') return -1;
    s++;
    while (*s && *s != '"') {
        char c = *s++;
        if (c == '\\') {
            c = *s++;
            /* The catalogue is ASCII (Rule 12); the only escapes are the JSON
             * quote/backslash/slash and the two-line CHKDSK messages' CR LF
             * (MSG-DOS-0043 "File is cross-linked:" / "on cluster n", MS-DOS
             * 3.3 pp.318-319), which the book prints as a line break. */
            if (c == 'r') c = '\r';
            else if (c == 'n') c = '\n';
            else if (c != '"' && c != '\\' && c != '/') return -1;
        }
        if (c != '\r' && c != '\n' && ((unsigned char)c < 0x20 || (unsigned char)c > 0x7E)) return -1; /* ASCII only, Rule 12 */
        if (n + 1 >= cap) return -1;
        out[n++] = c;
    }
    if (*s != '"') return -1;
    out[n] = '\0';
    *p = s + 1;
    return 0;
}

static void skip_ws(const char **p)
{
    while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r') (*p)++;
}

static void put_tex(const char *t)
{
    for (; *t; t++) {
        if (t[0] == '%' && t[1] == 'c') { fputs("\\ph{d}", stdout); t++; continue; }
        if (t[0] == '\001') { fputs(" \\ph{path}", stdout); continue; }
        if (t[0] == ' ' && t[1] == '\0') { fputs(" \\ph{n}", stdout); continue; }
        if (t[0] == '\r') { continue; }
        if (t[0] == '\n') { fputs("\\\\", stdout); continue; }
        switch (*t) {
        case '\\': fputs("\\textbackslash{}", stdout); break;
        case '{': case '}': case '$': case '&': case '#': case '_': case '%':
            printf("\\%c", *t); break;
        case '^': fputs("\\textasciicircum{}", stdout); break;
        case '~': fputs("\\textasciitilde{}", stdout); break;
        default: putchar(*t);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 3) { fprintf(stderr, "usage: pubs_msgtable <dos_messages.json> [all|device|other]\n"); return 2; }
    int select = 0; /* 0 all, 1 device, 2 other */
    if (argc == 3) {
        if (strcmp(argv[2], "all") == 0) select = 0;
        else if (strcmp(argv[2], "device") == 0) select = 1;
        else if (strcmp(argv[2], "other") == 0) select = 2;
        else { fprintf(stderr, "pubs_msgtable: unknown selector %s\n", argv[2]); return 2; }
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { fprintf(stderr, "pubs_msgtable: cannot open %s\n", argv[1]); return 2; }
    static char buf[1 << 16];
    size_t len = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[len] = '\0';

    const char *p = strstr(buf, "\"messages\"");
    if (!p) { fprintf(stderr, "pubs_msgtable: FAIL no \"messages\" object\n"); return 1; }
    p += strlen("\"messages\"");
    skip_ws(&p);
    if (*p++ != ':') { fprintf(stderr, "pubs_msgtable: FAIL malformed \"messages\"\n"); return 1; }
    skip_ws(&p);
    if (*p++ != '{') { fprintf(stderr, "pubs_msgtable: FAIL \"messages\" is not an object\n"); return 1; }

    static struct msg m[MAXMSG];
    int n = 0, fragments = 0;
    for (;;) {
        skip_ws(&p);
        if (*p == '}') break;
        struct msg cur;
        if (read_string(&p, cur.id, sizeof(cur.id)) != 0) {
            fprintf(stderr, "pubs_msgtable: FAIL bad message id near entry %d\n", n + fragments + 1); return 1;
        }
        skip_ws(&p);
        if (*p++ != ':') { fprintf(stderr, "pubs_msgtable: FAIL missing ':' after %s\n", cur.id); return 1; }
        skip_ws(&p);
        if (read_string(&p, cur.text, sizeof(cur.text)) != 0) {
            fprintf(stderr, "pubs_msgtable: FAIL bad text for %s\n", cur.id); return 1;
        }
        if (strncmp(cur.id, "MSG-DOS-", 8) != 0) {
            fprintf(stderr, "pubs_msgtable: FAIL unexpected id %s\n", cur.id); return 1;
        }
        if (cur.text[0] == ' ' || cur.text[0] == '<') fragments++;
        else if (cur.text[0] == ',') {
            /* The tail of the previous id's message (see the header). */
            size_t hl = n ? strlen(m[n - 1].text) : 0;
            if (hl == 0 || m[n - 1].text[hl - 1] != ' ' ||
                hl + 1 + strlen(cur.text) >= sizeof(m[0].text)) {
                fprintf(stderr, "pubs_msgtable: FAIL %s continues no message that ends in a space\n", cur.id); return 1;
            }
            m[n - 1].text[hl - 1] = '\001';   /* the path the command prints */
            strcpy(m[n - 1].text + hl, cur.text);
            fragments++;
        } else {
            if (n == MAXMSG) { fprintf(stderr, "pubs_msgtable: FAIL too many messages\n"); return 1; }
            m[n++] = cur;
        }
        skip_ws(&p);
        if (*p == ',') { p++; continue; }
        if (*p == '}') break;
        fprintf(stderr, "pubs_msgtable: FAIL expected ',' or '}' after %s\n", cur.id); return 1;
    }
    if (n == 0) { fprintf(stderr, "pubs_msgtable: FAIL empty catalogue\n"); return 1; }
    qsort(m, (size_t)n, sizeof(m[0]), cmp);

    printf("%% Generated by tools/pubs_msgtable.c from spec/dos_messages.json.\n"
           "%% Do not edit: change the catalogue (ADR-0003 DEC-13) instead.\n");
    int printed = 0;
    for (int i = 0; i < n; i++) {
        int device = strstr(m[i].text, "reading drive %c:") != NULL ||
                     strstr(m[i].text, "writing drive %c:") != NULL ||
                     strcmp(m[i].text, "Abort, Retry, Fail?") == 0;
        if ((select == 1 && !device) || (select == 2 && device)) continue;
        printed++;
        printf("\\MessageEntry{%s}{", m[i].id);
        put_tex(m[i].text);
        printf("}\n");
    }
    if (printed == 0) { fprintf(stderr, "pubs_msgtable: FAIL selector printed no messages\n"); return 1; }
    fprintf(stderr, "pubs_msgtable: %d of %d messages, %d display fragments left out\n", printed, n, fragments);
    return 0;
}
