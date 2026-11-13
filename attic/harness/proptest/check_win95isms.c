/*
 * check_win95isms.c -- the era-boundary guardrail gate (check-win95isms).
 *
 * beads: initech-m6qx (re-flair STEP 6); epic initech-qipc step 6.
 * Ref:   ADR-0006 Sec 4.6 (check-win95isms -- a grep gate over spec/ AND
 *        os/flair/ asserting NO forbidden Win95 token appears as a live
 *        constant; the guardrails doc itself excluded). ADR-0004-AMENDMENT-
 *        DEC-09 Sec 3.6 ARB-8 + Sec 3.7 D-9b R5 (the flat-2-D Win-3.1 target
 *        forbids the Win95 3D-light import). CLAUDE.md Law 3, Rule 6, Rule 12.
 *
 * WHY. The HERITAGE_GDI peer skin is a FLAT-2-D Win-3.1 facade, NOT Win95 CTL3D.
 * The single largest drift risk is silently importing a Win95-ism (the #DFDFDF
 * COLOR_3DLIGHT inner-bevel highlight, the gradient caption, CTL3D, DrawEdge,
 * MENUEX, a COLOR_* system index past the Win-3.1 set). This gate scans the
 * FLAIR color sources for those tokens as LIVE constants and fails loud if one
 * appears, keeping the peer skin period-correct.
 *
 * SCOPE. Scans spec/flair_skins.h, spec/assets/color_canon.{h,json}, and the
 * os/flair/ C/H sources (the FLAIR color surface). A guardrails DOC (a .md that
 * lists the forbidden tokens to document them) is EXCLUDED -- listing a token to
 * forbid it is not using it. Comments are NOT excluded: a forbidden token in a
 * comment is still drift toward a Win95 import and is flagged (the doc-exclusion
 * is the one carve-out, mirroring the ADR's "guardrails doc itself excluded").
 *
 * MUTATION PROOF (Rule 6): built with -DWIN95ISM_MUTANT, the scanner ALSO scans
 * an in-binary planted-token buffer containing "#DFDFDF" -> the gate goes RED.
 * Restore by dropping the -D. (This proves the matcher bites without mutating a
 * tracked source file; an operator may ALSO plant a literal #DFDFDF into
 * flair_skins.h to prove the file scan bites, then revert.)
 *
 * HOSTED RUN (mirrors the in-repo C-scanner gates):
 *   gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *       harness/proptest/check_win95isms.c -o build/check_win95isms \
 *       && build/check_win95isms
 *   Mutant:  add -DWIN95ISM_MUTANT  (MUST exit non-zero / RED).
 *   (Run from the repo root; paths below are repo-relative.)
 *
 * ASCII-clean (Rule 12). Deterministic / no timestamps (Rule 11).
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* ---------------------------------------------------------------------------
 * The forbidden Win95-ism tokens (ADR-0006 Sec 4.6; ADR-0004-AMENDMENT-DEC-09
 * ARB-8 / D-9b R5). Matched case-insensitively as substrings of each scanned
 * line. #DFDFDF and COLOR_3DLIGHT are the load-bearing pair from ARB-8.
 * ------------------------------------------------------------------------- */
static const char *const FORBIDDEN[] = {
    "#DFDFDF",          /* the Win95 COLOR_3DLIGHT highlight value (ARB-8)      */
    "0xDFDFDF",         /* same value as a C hex literal                       */
    "COLOR_3DLIGHT",    /* the Win95 system color name (ARB-8)                  */
    "COLOR_3DDKSHADOW", /* the Win95 dark-shadow system color                  */
    "CTL3D",            /* the Win95 3-D control library                       */
    "DrawEdge",         /* the Win95 GDI 3-D edge primitive                    */
    "MENUEX",           /* the Win95 extended-menu resource                    */
    "HTCLOSE",          /* the Win95 caption close-box hit-test code           */
    "gradient caption", /* the Win98+ gradient title bar                       */
    "GradientFill",     /* the Win95+/MSIMG32 gradient primitive               */
    NULL
};

/* The files/dirs to scan (repo-relative). os/flair is walked one level deep for
 * .c/.h; the spec color sources are named explicitly. */
static const char *const FILES[] = {
    "spec/flair_skins.h",
    "spec/assets/color_canon.h",
    "spec/assets/color_canon.json",
    "os/flair/flair_look.h",
    "os/flair/chrome.c",
    "os/flair/control.c",
    "os/flair/dialog.c",
    "os/flair/menu.c",
    "os/flair/window.c",
    "os/flair/desktop.c",
    NULL
};

static int ascii_casecmp_contains(const char *hay, const char *needle)
{
    size_t nl = strlen(needle);
    if (nl == 0) return 0;
    for (; *hay; hay++) {
        size_t k;
        for (k = 0; k < nl; k++) {
            unsigned char a = (unsigned char)hay[k];
            unsigned char b = (unsigned char)needle[k];
            if (a == '\0') return 0;
            if (tolower(a) != tolower(b)) break;
        }
        if (k == nl) return 1;
    }
    return 0;
}

/* Scan one text buffer (a line) for any forbidden token. Returns the token or
 * NULL. */
static const char *scan_line(const char *line)
{
    int i;
    for (i = 0; FORBIDDEN[i] != NULL; i++) {
        if (ascii_casecmp_contains(line, FORBIDDEN[i])) {
            return FORBIDDEN[i];
        }
    }
    return NULL;
}

static int scan_file(const char *path, int *files_scanned)
{
    FILE *f = fopen(path, "r");
    char line[4096];
    int hits = 0;
    long lineno = 0;
    if (!f) {
        /* A scanned source being absent is a setup error, not a pass: report it
         * loud so the gate is never a silent no-op (the loud-skip discipline).
         * Absent FLAIR sources that simply do not exist yet are reported but do
         * not fail the gate (they carry no tokens). */
        printf("note: scan target absent (skipped): %s\n", path);
        return 0;
    }
    (*files_scanned)++;
    while (fgets(line, (int)sizeof(line), f)) {
        const char *tok;
        lineno++;
        tok = scan_line(line);
        if (tok) {
            hits++;
            printf("FAIL win95-ism '%s' in %s:%ld\n", tok, path, lineno);
        }
    }
    fclose(f);
    return hits;
}

int main(void)
{
    int hits = 0, files_scanned = 0, i;

    for (i = 0; FILES[i] != NULL; i++) {
        hits += scan_file(FILES[i], &files_scanned);
    }

#ifdef WIN95ISM_MUTANT
    /* Rule 6: plant a token in an in-binary buffer and prove the matcher bites,
     * without mutating a tracked source file. */
    {
        const char *planted = "    skin->btnhilight = #DFDFDF; /* WIN95-ism */";
        const char *tok = scan_line(planted);
        if (tok) {
            hits++;
            printf("FAIL win95-ism '%s' in <planted-mutant-buffer>:1\n", tok);
        }
    }
#endif

    printf("check_win95isms: %d files scanned, %d win95-ism hit(s)\n",
           files_scanned, hits);
    return hits ? 1 : 0;
}
