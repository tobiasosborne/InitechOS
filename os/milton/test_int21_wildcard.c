/* test_int21_wildcard.c -- host unit oracle for the DOS 8.3 FCB-style wildcard
 * engine behind AH=4Eh/4Fh FINDFIRST/FINDNEXT (beads initech-80k).
 *
 * Strategy: drive the REAL artifact int21.c matcher (build_pattern +
 * pattern_match) directly, through the INT21_WILDCARD_TESTSEAM wrappers, so
 * there is no second matcher to drift (Law 2). A row is (spec, on-disk name) ->
 * expected match/no-match; build the FCB template from the spec, render the
 * on-disk name into its 11-byte space-padded form (exactly as entry_template
 * does from a dir entry), and assert.
 *
 * GROUND TRUTH (Law 1) -- the MS-DOS / CP/M FCB matching algorithm, as
 * documented by Raymond Chen, "How did wildcards work in MS-DOS?", The Old New
 * Thing, 2007-12-17, and the DOS 3.3 Programmer's Reference Manual AH=4Eh:
 *
 *   1. The spec is converted to an 11-byte template (8 name + 3 ext), starting
 *      from ELEVEN SPACES. A '.' jumps the cursor to the ext field (pos 9). A
 *      '*' fills the REST OF ITS CURRENT FIELD with '?'. Any other char is
 *      copied (upper-cased) and the cursor advances. Short fields stay
 *      space-padded.
 *   2. Matching is position-by-position across all 11 bytes: a '?' in the
 *      template matches ANY byte INCLUDING the trailing space pad; a non-'?'
 *      byte must compare equal. The on-disk name is itself space-padded to 11.
 *
 * Consequence (the load-bearing, counter-intuitive fact): because '?' matches
 * the pad space and the on-disk name is space-padded, a pattern with a trailing
 * '?' MATCHES A SHORTER NAME. So:
 *   - "FOO?"  MATCHES "FOO"   (template pos 3 '?' matches the pad space)
 *   - "*.TXT" MATCHES "A.TXT" (name = '?'*8, pos 1..7 '?' match pad spaces)
 *   - "?.BAR" MATCHES "A.BAR" (pos 0 '?' = 'A', pos 1..7 spaces = pad spaces)
 *   - "?"     MATCHES the empty/1-char name (pos 0 '?' matches the pad space)
 * The grounding-brief guess that "FOO?.* does NOT match FOO.TXT" is WRONG under
 * real DOS semantics; this oracle encodes the CORRECT behavior with the
 * citation above.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11 / Law 3).
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "test_assert.h"

TEST_HARNESS();

/* The seam wrappers (int21.c, compiled with -DINT21_WILDCARD_TESTSEAM). */
extern void int21_wildcard_build(const char *spec, uint8_t out[11]);
extern int  int21_wildcard_match(const uint8_t pat[11], const uint8_t name[11]);

/* Render an on-disk 8.3 name ("FOO", "FOO.TXT", "A.BAR", "") into the 11-byte
 * space-padded, upper-cased form the directory entry carries (this is exactly
 * what entry_template() produces from a dir_entry's filename[8]/extension[3],
 * which spec/dos_structs.h documents as already space-padded upper-case). The
 * name field gets the part before '.', the ext field the part after; both are
 * padded with spaces and truncated to 8 / 3. */
static void render_name(const char *name, uint8_t out[11])
{
    for (int i = 0; i < 11; i++) {
        out[i] = ' ';
    }
    int field = 0;          /* 0 = name (0..7), 1 = ext (8..10) */
    int pos   = 0;
    for (const char *p = name; *p; p++) {
        char c = *p;
        if (c == '.') { field = 1; pos = 0; continue; }
        int base = (field == 0) ? 0 : 8;
        int cap  = (field == 0) ? 8 : 3;
        if (pos < cap) {
            if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
            out[base + pos++] = (uint8_t)c;
        }
    }
}

/* One match expectation: does FCB-template(spec) match on-disk name? */
static void expect_match(const char *spec, const char *name, int want, const char *msg)
{
    uint8_t pat[11];
    uint8_t nm[11];
    int21_wildcard_build(spec, pat);
    render_name(name, nm);
    int got = int21_wildcard_match(pat, nm);
    CHECK(got == want, msg);
}

/* Assert the 11-byte template build matches an expected literal (11 chars). */
static void expect_template(const char *spec, const char *want11, const char *msg)
{
    uint8_t pat[11];
    int21_wildcard_build(spec, pat);
    int ok = (memcmp(pat, want11, 11) == 0);
    CHECK(ok, msg);
}

int main(void)
{
    /* ---- (A) template build: '*' fills its OWN field; '?' is stored literally;
     * input is upper-cased; '.' switches fields; short fields space-pad. ---- */
    /* "*.*" -> all '?' (match all). */
    expect_template("*.*",      "???????????", "build: *.* -> 11 '?'");
    /* "A*.*": name = A + 7 '?'; ext = 3 '?'. The '*' fills only the NAME field. */
    expect_template("A*.*",     "A??????????", "build: A*.* -> A + 10 '?' (name '*' stays in name)");
    /* "*.TXT": name = 8 '?'; ext = TXT. The name '*' does NOT bleed into ext. */
    expect_template("*.TXT",    "????????TXT", "build: *.TXT -> 8 '?' + TXT (ext NOT over-filled)");
    /* "FOO?.*": name = FOO?  (literal '?' at pos 3) + 4 pad spaces; ext = 3 '?'. */
    expect_template("FOO?.*",   "FOO?    ???", "build: FOO?.* -> FOO? + pad + 3 '?' ('?' literal)");
    /* "FOO?": name = FOO? + pad; NO dot -> ext stays all spaces (not '?'). */
    expect_template("FOO?",     "FOO?       ", "build: FOO? (no dot) -> ext stays spaces");
    /* lower-case input is upper-cased. */
    expect_template("foo.txt",  "FOO     TXT", "build: foo.txt upper-cased to FOO.TXT");
    /* exact 8.3 name, short, space-padded. */
    expect_template("A.B",      "A       B  ", "build: A.B -> space-padded");
    /* "?.BAR": name = ? + 7 spaces; ext = BAR. */
    expect_template("?.BAR",    "?       BAR", "build: ?.BAR -> ? + pad + BAR");
    /* "A*" (NO dot): the name '*' fills the NAME field only; the ext stays all
     * spaces (blank). This is the row that distinguishes a per-field '*' fill
     * (correct: ext stays "   ") from a '*' that bleeds to the end of the
     * 11-byte template (wrong: ext becomes "???"). */
    expect_template("A*",       "A???????   ", "build: A* (no dot) -> A + 7 '?' name, ext BLANK (no bleed)");

    /* ---- (B) the canonical match table (ground truth: Old New Thing 2007). -- */

    /* exact match, and exact non-match. */
    expect_match("HELLO.TXT", "HELLO.TXT", 1, "exact: HELLO.TXT matches HELLO.TXT");
    expect_match("HELLO.TXT", "HELLO.DAT", 0, "exact: HELLO.TXT does NOT match HELLO.DAT");
    expect_match("README",    "README",    1, "exact: README (no ext) matches README");
    expect_match("README",    "READ",      0, "exact: README does NOT match READ");

    /* "*.*" matches everything (regular names). */
    expect_match("*.*", "HELLO.TXT", 1, "*.* matches HELLO.TXT");
    expect_match("*.*", "README",    1, "*.* matches README (no ext)");
    expect_match("*.*", "A",         1, "*.* matches a 1-char name");

    /* "*.TXT": the SHARP ground-truth case. Name '*' becomes 8 '?', which match
     * the pad spaces of a short name -> "A.TXT" MATCHES. Ext must be exactly TXT. */
    expect_match("*.TXT", "A.TXT",     1, "*.TXT MATCHES A.TXT (?-over-pad; ground truth)");
    expect_match("*.TXT", "HELLO.TXT", 1, "*.TXT matches HELLO.TXT");
    expect_match("*.TXT", "HELLO.DAT", 0, "*.TXT does NOT match HELLO.DAT (ext differs)");
    expect_match("*.TXT", "README",    0, "*.TXT does NOT match README (ext is blank, not TXT)");

    /* "A*.*": name starts with A, any ext. */
    expect_match("A*.*", "APPLE.TXT", 1, "A*.* matches APPLE.TXT");
    expect_match("A*.*", "A.TXT",     1, "A*.* matches A.TXT (short name, ? over pad)");
    expect_match("A*.*", "BANANA.TXT",0, "A*.* does NOT match BANANA.TXT");
    expect_match("A*.*", "README",    0, "A*.* does NOT match README");

    /* "A*" (NO dot): name starts with A, ext must be BLANK. This is the row that
     * BITES a name-field '*' bleeding into the ext field -- with a bleed, "A*"
     * would match "APPLE.TXT" (wrong); correct DOS keeps the ext blank. */
    expect_match("A*", "APPLE",     1, "A* (no dot) matches APPLE (blank ext)");
    expect_match("A*", "A",         1, "A* (no dot) matches A (blank ext)");
    expect_match("A*", "APPLE.TXT", 0, "A* (no dot) does NOT match APPLE.TXT (ext must be blank; bleed-catcher)");
    expect_match("A*", "README",    0, "A* (no dot) does NOT match README");

    /* "?.BAR": exactly one name char (or zero -> pad), ext BAR. */
    expect_match("?.BAR", "A.BAR", 1, "?.BAR MATCHES A.BAR (ground truth)");
    expect_match("?.BAR", "X.BAR", 1, "?.BAR matches X.BAR");
    expect_match("?.BAR", "AB.BAR",0, "?.BAR does NOT match AB.BAR (2-char name)");
    expect_match("?.BAR", "A.BAZ", 0, "?.BAR does NOT match A.BAZ (ext differs)");

    /* "FOO?.*": THE counter-intuitive case the brief got wrong. A trailing '?'
     * matches the pad space, so "FOO" (3 chars) MATCHES, and "FOO.TXT" matches.
     * "FOOD" (4 chars, pos 3 == 'D') also matches; "FOOBAR" (5+) does not. */
    expect_match("FOO?.*", "FOO.TXT", 1, "FOO?.* MATCHES FOO.TXT (?-over-pad; brief was WRONG)");
    expect_match("FOO?.*", "FOO",     1, "FOO?.* MATCHES FOO (trailing ? over pad space)");
    expect_match("FOO?.*", "FOOD.TXT",1, "FOO?.* matches FOOD.TXT (? == D)");
    expect_match("FOO?.*", "FOOBAR",  0, "FOO?.* does NOT match FOOBAR (5-char name)");
    expect_match("FOO?.*", "FXO.TXT", 0, "FOO?.* does NOT match FXO.TXT (literal O mismatch)");

    /* "?": a single name '?' (no dot) -> ext stays blank. Matches a 0/1-char
     * name with a BLANK extension only. */
    expect_match("?", "A",     1, "? matches a 1-char name A");
    expect_match("?", "AB",    0, "? does NOT match a 2-char name AB");
    expect_match("?", "A.TXT", 0, "? does NOT match A.TXT (ext must be blank)");

    /* lower-case spec must still match an (upper-cased) on-disk name -- the
     * build upper-cases the input. */
    expect_match("hello.txt", "HELLO.TXT", 1, "lower-case spec hello.txt matches HELLO.TXT");

    return TEST_SUMMARY("test_int21_wildcard");
}
