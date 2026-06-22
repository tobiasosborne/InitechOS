/*
 * test_mech_policy.c -- the C-8 SOURCE-conformance scanner (THE ORACLE).
 *
 * beads: initech-6bq2 (C-8 MECHANISM/POLICY enforcement); epic initech-qipc.
 * Ref:   ADR-0004-AMENDMENT-DEC-09 Sec 3.1 (DEC-09-D1, the C-8 cut-line),
 *        Sec 3.10 (DEC-09-D4 #1, this oracle), Sec 5.1 (constraint C-8);
 *        CLAUDE.md Law 2 (the oracle is the truth), Rule 6 (mutation-proven),
 *        Rule 11 (deterministic), Rule 12 (ASCII-clean).
 *
 * WHAT IT GRADES (structure, NOT values): it READS the declared FLAIR
 * mechanism + decoration source files (below the C-8 cut-line) and asserts
 * each ships:
 *   - ZERO 0xRRGGBB / 0xRRGGBBu hex COLOR literal,
 *   - ZERO INITECH_*_RGB macro reference,
 *   - ZERO index->RGB switch (a `switch` whose body returns/yields raw
 *     0xRRGGBB color literals -- the five-drifted-switch heresy DEC-09 kills).
 * The ALLOWLIST is EXACTLY the two sites permitted to turn an index into a
 * color: the device CLUT mechanism (spec/assets/clut.h) and the flair_look
 * resolver TU (os/flair/flair_look.c).  Those are NOT scanned.
 *
 * It grades SOURCE STRUCTURE (it reads the files; it never renders), so it
 * CANNOT self-pass / cannot be valid-by-construction (HER-02 boundary): the
 * artifact and the grader read different things.  This is the cheapest gate in
 * the vector -- no emulator, no golden.
 *
 * COMMENT/STRING AWARE: C comments (block + line) and string/char literals are
 * stripped before scanning, so a macro NAME or a hex value mentioned in prose
 * (e.g. "byte-identical to the prior named-constant site") is never a false
 * positive.  Only real code tokens are graded.
 *
 * MUTATION (Rule 6): built with -DMECH_POLICY_MUTANT, the scanner ALSO scans
 * harness/proptest/fixtures/mech_policy_mutant.c -- a mechanism-shaped fixture
 * carrying one planted 0xRRGGBB literal -- so the mutant build MUST go RED.
 * Restore = the normal build does not scan the fixture.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "test_assert.h"   /* TEST_HARNESS / CHECK / TEST_SUMMARY (-Iseed) */

TEST_HARNESS();

/* ---------------------------------------------------------------------------
 * The declared scan set (below the C-8 cut-line).  surface/blitter/window/
 * event/desktop are MECHANISM (imaging + geometry).  chrome/control/dialog are
 * DECORATION (they keep geometry + the cfill/crect/cframe span engine but name
 * a PART, not a color) -- they sit below the cut-line too and must be equally
 * literal-free, so the scanner grades them as well (DEC-09 Sec 3.10: the span
 * engine "wherever it lives").  All eight must be color-literal-free.
 * ------------------------------------------------------------------------- */
static const char *const MECH_FILES[] = {
    "os/flair/surface.c",   /* MECHANISM: the ONE pixel writer                */
    "os/flair/blitter.c",   /* MECHANISM: region-clipped span fill/blit       */
    "os/flair/window.c",    /* MECHANISM: window geometry / z-order           */
    "os/flair/event.c",     /* MECHANISM: event pump (no color)               */
    "os/flair/desktop.c",   /* MECHANISM: compositor / repaint geometry       */
    "os/flair/chrome.c",    /* DECORATION: chrome look + cfill/crect/cframe    */
    "os/flair/control.c",   /* DECORATION: control look                       */
    "os/flair/dialog.c"     /* DECORATION: dialog look                        */
};
enum { MECH_FILE_COUNT = (int)(sizeof MECH_FILES / sizeof MECH_FILES[0]) };

/* The mutant fixture, scanned ONLY under -DMECH_POLICY_MUTANT (Rule 6). */
#if defined(MECH_POLICY_MUTANT)
static const char *const MUTANT_FIXTURE =
    "harness/proptest/fixtures/mech_policy_mutant.c";
#endif

/* ---------------------------------------------------------------------------
 * slurp -- read an entire file into a heap buffer (NUL-terminated).  Returns
 * NULL on open failure (the caller fails the check loudly).  Deterministic.
 * ------------------------------------------------------------------------- */
static char *slurp(const char *path, long *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long n = ftell(f);
    if (n < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[got] = '\0';
    if (out_len) {
        *out_len = (long)got;
    }
    return buf;
}

/* ---------------------------------------------------------------------------
 * strip_comments_and_strings -- replace C comments (block + line) and string /
 * char literals with spaces, IN PLACE.  Newlines preserved (for line numbers).
 * After this pass only real code tokens remain, so a macro name or hex value in
 * prose can never be a false positive (Law 2 -- a scanner that misfires on a
 * comment is not an oracle).
 * ------------------------------------------------------------------------- */
static void strip_comments_and_strings(char *s)
{
    enum { CODE, BLOCK, LINE, STR, CHR } st = CODE;
    for (size_t i = 0; s[i] != '\0'; i++) {
        char c = s[i];
        char n = s[i + 1];
        switch (st) {
        case CODE:
            if (c == '/' && n == '*') { s[i] = ' '; s[i + 1] = ' '; i++; st = BLOCK; }
            else if (c == '/' && n == '/') { s[i] = ' '; s[i + 1] = ' '; i++; st = LINE; }
            else if (c == '"') { s[i] = ' '; st = STR; }
            else if (c == '\'') { s[i] = ' '; st = CHR; }
            break;
        case BLOCK:
            if (c == '*' && n == '/') { s[i] = ' '; s[i + 1] = ' '; i++; st = CODE; }
            else if (c != '\n') { s[i] = ' '; }
            break;
        case LINE:
            if (c == '\n') { st = CODE; }
            else { s[i] = ' '; }
            break;
        case STR:
            if (c == '\\' && n != '\0') { s[i] = ' '; s[i + 1] = ' '; i++; }
            else if (c == '"') { s[i] = ' '; st = CODE; }
            else if (c != '\n') { s[i] = ' '; }
            break;
        case CHR:
            if (c == '\\' && n != '\0') { s[i] = ' '; s[i + 1] = ' '; i++; }
            else if (c == '\'') { s[i] = ' '; st = CODE; }
            else if (c != '\n') { s[i] = ' '; }
            break;
        }
    }
}

/* line number (1-based) of byte offset `off` in `s`. */
static int line_of(const char *s, size_t off)
{
    int line = 1;
    for (size_t i = 0; i < off && s[i] != '\0'; i++) {
        if (s[i] == '\n') {
            line++;
        }
    }
    return line;
}

static int is_hex(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int is_ident(char c)
{
    return isalnum((unsigned char)c) || c == '_';
}

/* ---------------------------------------------------------------------------
 * count_color_hex -- count 0xRRGGBB (exactly 6 hex digits, optional u/U/l/L
 * suffix, not part of a longer hex run) color literals in stripped code.  A
 * 0xRRGGBB is the canonical packed-color literal C-8 forbids below the cut.
 * 8-digit (0xAARRGGBB) and 2-digit (0xFFu mask) literals are NOT colors and
 * are not counted (the 6-digit run is the discriminator).  Reports first hit.
 * ------------------------------------------------------------------------- */
static int count_color_hex(const char *s, int *first_line)
{
    int hits = 0;
    *first_line = 0;
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
            /* must not be preceded by an ident char (e.g. part of a name) */
            if (i > 0 && is_ident(s[i - 1])) {
                continue;
            }
            size_t j = i + 2;
            size_t start = j;
            while (is_hex(s[j])) {
                j++;
            }
            size_t ndig = j - start;
            /* after the hex digits, allow integer suffix u/U/l/L (any order) */
            size_t k = j;
            while (s[k] == 'u' || s[k] == 'U' || s[k] == 'l' || s[k] == 'L') {
                k++;
            }
            /* a color literal is EXACTLY 6 hex digits not followed by another
             * ident char (so 0x00RRGGBB(8) and 0xFFu(2) are excluded). */
            if (ndig == 6 && !is_ident(s[k])) {
                hits++;
                if (*first_line == 0) {
                    *first_line = line_of(s, i);
                }
            }
            i = j - 1;
        }
    }
    return hits;
}

/* ---------------------------------------------------------------------------
 * count_initech_rgb -- count INITECH_*_RGB macro references in stripped code.
 * The macro family the canon header exposes for compile-time color constants;
 * forbidden below the cut-line (only the allowlisted resolver may use them).
 * ------------------------------------------------------------------------- */
static int count_initech_rgb(const char *s, int *first_line)
{
    int hits = 0;
    *first_line = 0;
    const char *pfx = "INITECH_";
    size_t plen = strlen(pfx);
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (i > 0 && is_ident(s[i - 1])) {
            continue;
        }
        if (strncmp(&s[i], pfx, plen) != 0) {
            continue;
        }
        /* walk the identifier; flag if it ends with _RGB */
        size_t j = i;
        while (is_ident(s[j])) {
            j++;
        }
        size_t idlen = j - i;
        if (idlen >= 4 && strncmp(&s[j - 4], "_RGB", 4) == 0) {
            hits++;
            if (*first_line == 0) {
                *first_line = line_of(s, i);
            }
        }
        i = j - 1;
    }
    return hits;
}

/* ---------------------------------------------------------------------------
 * count_index_rgb_switch -- detect an index->RGB switch: a `switch` whose body
 * (up to the matching close brace) contains a 0xRRGGBB color literal.  This is
 * the precise shape of the five drifted index->RGB switches DEC-09 kills (a
 * switch on an index producing raw color values).  We already forbid bare
 * color hex above, so this is a redundancy tooth that also fires on a switch
 * that smuggled a color in -- defence in depth.
 * ------------------------------------------------------------------------- */
static int count_index_rgb_switch(const char *s, int *first_line)
{
    int hits = 0;
    *first_line = 0;
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (i > 0 && is_ident(s[i - 1])) {
            continue;
        }
        if (strncmp(&s[i], "switch", 6) != 0 || is_ident(s[i + 6])) {
            continue;
        }
        /* find the body's opening brace */
        size_t j = i + 6;
        while (s[j] != '\0' && s[j] != '{' && s[j] != ';') {
            j++;
        }
        if (s[j] != '{') {
            continue;
        }
        /* scan to the matching close brace, looking for a 6-digit hex color */
        int depth = 0;
        size_t body_start = j;
        for (; s[j] != '\0'; j++) {
            if (s[j] == '{') {
                depth++;
            } else if (s[j] == '}') {
                depth--;
                if (depth == 0) {
                    j++;
                    break;
                }
            }
        }
        /* extract [body_start, j) and count color hex in it */
        size_t blen = j - body_start;
        char *body = (char *)malloc(blen + 1);
        if (body) {
            memcpy(body, &s[body_start], blen);
            body[blen] = '\0';
            int fl = 0;
            if (count_color_hex(body, &fl) > 0) {
                hits++;
                if (*first_line == 0) {
                    *first_line = line_of(s, i);
                }
            }
            free(body);
        }
        i = j - 1;
    }
    return hits;
}

/* ---------------------------------------------------------------------------
 * scan_one -- run all three teeth over one file; CHECK each is zero.  `tag`
 * labels the file in failure messages.  `must_be_clean` is 1 for the real scan
 * set (expect zero hits) and 0 for the mutant fixture (expect NON-zero -> the
 * scanner counts the violation, and the mutant test asserts the file is dirty).
 * Returns the total number of violations found in this file.
 * ------------------------------------------------------------------------- */
static int scan_one(const char *path, int must_be_clean)
{
    long len = 0;
    char *raw = slurp(path, &len);
    char msg[256];
    if (!raw) {
        snprintf(msg, sizeof msg, "[%s] could not be opened for scanning", path);
        CHECK(0, msg);
        return 0;
    }
    strip_comments_and_strings(raw);

    int hex_line = 0, mac_line = 0, sw_line = 0;
    int hex = count_color_hex(raw, &hex_line);
    int mac = count_initech_rgb(raw, &mac_line);
    int sw  = count_index_rgb_switch(raw, &sw_line);
    int total = hex + mac + sw;

    if (must_be_clean) {
        snprintf(msg, sizeof msg,
                 "[%s] must ship ZERO 0xRRGGBB color literal (found %d, first @ line %d)",
                 path, hex, hex_line);
        CHECK(hex == 0, msg);
        snprintf(msg, sizeof msg,
                 "[%s] must ship ZERO INITECH_*_RGB macro (found %d, first @ line %d)",
                 path, mac, mac_line);
        CHECK(mac == 0, msg);
        snprintf(msg, sizeof msg,
                 "[%s] must ship ZERO index->RGB switch (found %d, first @ line %d)",
                 path, sw, sw_line);
        CHECK(sw == 0, msg);
    }

    free(raw);
    return total;
}

int main(void)
{
    printf("test-mech-policy: C-8 source-conformance scanner "
           "(no color literal below the cut-line)\n");

    /* The real scan set: every declared file must be color-literal-free. */
    for (int i = 0; i < MECH_FILE_COUNT; i++) {
        (void)scan_one(MECH_FILES[i], 1 /* must_be_clean */);
    }

#if defined(MECH_POLICY_MUTANT)
    /* Rule 6: the mutant fixture carries one planted 0xRRGGBB literal.  The
     * scanner MUST count it (>0) -- if the fixture scanned clean, the scanner
     * is blind and this check fails the build RED. */
    {
        int viol = scan_one(MUTANT_FIXTURE, 0 /* report only */);
        char msg[256];
        snprintf(msg, sizeof msg,
                 "[MUTANT] %s MUST carry a detectable C-8 violation (found %d)",
                 MUTANT_FIXTURE, viol);
        CHECK(viol > 0, msg);
        /* Force the build RED so the mutant target observes a non-zero exit
         * even though the planted-literal detection above is itself a PASS:
         * the mutant's PURPOSE is to prove the scanner BITES, so under
         * MECH_POLICY_MUTANT the test deliberately fails iff the fixture was
         * NOT flagged.  We assert that the fixture WAS flagged, then exit
         * non-zero to signal "this is the mutant build" to the Makefile gate. */
        printf("test-mech-policy[MUTANT]: planted-literal detection = %d violation(s)\n",
               viol);
        if (viol > 0) {
            /* The scanner correctly bit the planted literal -> the mutant
             * build is RED ON PURPOSE (Rule 6: the oracle bites). */
            fprintf(stderr,
                "test-mech-policy[MUTANT]: RED on purpose -- scanner bit the "
                "planted 0xRRGGBB color literal in the fixture\n");
            return 1;
        }
    }
#endif

    return TEST_SUMMARY("test-mech-policy");
}
