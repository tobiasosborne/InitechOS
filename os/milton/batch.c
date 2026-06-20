/* batch.c -- InitechDOS COMMAND.COM .BAT interpreter: pure batch-language
 *            parser and parameter/variable expander.
 *
 * beads: initech-xw1 (batch interpreter -- LOGIC half; REPL/AUTOEXEC
 *        integration into command.c is a separate later bead).
 * Ref:   DOS 3.3 COMMAND.COM batch-file processing (Ralf Brown's Interrupt
 *        List; Peter Norton "DOS Programmer's Reference" 3rd ed.; Microsoft
 *        MS-DOS 3.3 Technical Reference, Chapter 3 "Batch Files"):
 *          - Parameter/variable expansion rules: %0..%9, %VAR%, %%.
 *          - Directive set: REM, ECHO, @, GOTO, :label, IF, FOR, SHIFT,
 *            CALL, PAUSE.
 *          - Case-insensitive directive recognition.
 *          - Label matching: GOTO is case-insensitive; only first 8 chars
 *            of a label name are significant in real DOS 3.3, but we match
 *            on the full name (BATCH_LABEL_MAX cap) for clarity.
 *        CLAUDE.md Law 1 (cite source), Law 2 (oracle is truth), Law 3
 *        (artifact = C), Rule 2 (fail loud), Rule 11 (deterministic),
 *        Rule 12 (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only.
 * No libc: every string helper is hand-rolled (same discipline as env.c and
 * command.c).
 *
 * MUTATION hooks (CLAUDE.md Rule 6):
 *   BATCH_MUTATE_NO_PCT_PCT    -- %% does NOT collapse to '%'; the %% test
 *                                 goes RED.  NEVER in a real build.
 *   BATCH_MUTATE_ECHO_OFF_MISS -- ECHO OFF classification is skipped so the
 *                                 ECHO OFF case falls through to BL_ECHO_TEXT;
 *                                 the ECHO OFF test goes RED.  NEVER in a
 *                                 real build.
 *   BATCH_MUTATE_GOTO_NOLABEL  -- batch_goto_target always copies 0 chars
 *                                 into the output (the GOTO target test goes
 *                                 RED).  NEVER in a real build.
 */

#include "batch.h"

/* ---- freestanding helpers (no libc) ------------------------------------- */

/* Return the byte-length of an ASCIIZ string (NUL not counted). */
static int bat_strlen(const char *s)
{
    int n = 0;
    if (s == 0) {
        return 0;
    }
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

/* Upper-case one ASCII byte (a-z -> A-Z); other bytes pass through.
 * DOS is case-insensitive for commands and directive keywords. */
static char bat_upcase(char c)
{
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

/* Return 1 if c is an ASCII space or horizontal tab, 0 otherwise. */
static int bat_isspace(char c)
{
    return (c == ' ' || c == '\t');
}

/* Case-insensitive comparison of two ASCIIZ strings.
 * Returns 0 if equal, non-zero if different. */
static int bat_stricmp(const char *a, const char *b)
{
    if (a == 0 || b == 0) {
        return (a != b);
    }
    while (*a != '\0' && *b != '\0') {
        if (bat_upcase(*a) != bat_upcase(*b)) {
            return 1;
        }
        a++;
        b++;
    }
    return (*a != *b);    /* both NUL -> 0 (equal); one still non-NUL -> 1 */
}

/* Copy at most `n`-1 characters from `src` into `dst` and NUL-terminate.
 * Returns the number of characters written (not counting NUL).
 * n must be >= 1.  Safe even when src is NULL (copies 0 chars). */
static int bat_strlcpy(char *dst, const char *src, int n)
{
    int i = 0;
    if (n <= 0) {
        return 0;
    }
    if (src == 0) {
        dst[0] = '\0';
        return 0;
    }
    while (i < n - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return i;
}

/* Append one character to `out` at position `*pos`, ensuring the total
 * stays below `cap` (including a final NUL).  Returns 1 on success, 0 if
 * there is no room (the character is dropped and overflow is signalled via
 * the return value -- callers check and propagate). */
static int bat_emit(char *out, int *pos, int cap, char c)
{
    if (*pos >= cap - 1) {
        return 0;   /* no room (Rule 2: never overflow) */
    }
    out[(*pos)++] = c;
    return 1;
}

/* Emit a NUL-terminated string `s` into `out` starting at `*pos`.
 * Returns the number of characters emitted on success.  Returns -1 and
 * leaves `*pos` unchanged if `s` does not fit within `cap`-1 chars
 * (overflow detected before writing, so `out` stays consistent). */
static int bat_emit_str(char *out, int *pos, int cap, const char *s)
{
    int slen = bat_strlen(s);
    if (*pos + slen >= cap) {
        return -1;  /* would not fit (including the final NUL slot) */
    }
    for (int i = 0; i < slen; i++) {
        out[(*pos)++] = s[i];
    }
    return slen;
}

/* Return 1 if c is a valid DOS environment variable name character:
 * alphanumeric, underscore, or any non-NUL non-'%' non-space byte.
 * Real DOS is liberal about what can appear in %NAME%; we allow anything
 * except NUL, '%', and ASCII control chars (< 0x20) to match DOS 3.3. */
static int bat_is_varname_char(char c)
{
    if (c == '\0' || c == '%') {
        return 0;
    }
    if ((unsigned char)c < 0x20) {
        return 0;
    }
    return 1;
}

/* ---- batch_expand -------------------------------------------------------- */

int batch_expand(const char *line,
                 const char *const argv[], int argc,
                 batch_env_lookup_fn env, void *envctx,
                 char *out, int cap)
{
    int pos = 0;    /* write cursor into `out` */
    int i   = 0;    /* read cursor into `line` */

    if (out == 0 || cap <= 0) {
        return -1;
    }
    if (line == 0) {
        out[0] = '\0';
        return 0;
    }

    while (line[i] != '\0') {
        if (line[i] != '%') {
            /* Ordinary character -- emit verbatim. */
            if (!bat_emit(out, &pos, cap, line[i])) {
                out[0] = '\0';
                return -1;  /* overflow (Rule 2) */
            }
            i++;
            continue;
        }

        /* line[i] == '%': look ahead. */
        i++;    /* skip the first '%' */

        if (line[i] == '\0') {
            /* Lone '%' at end of line -- emit verbatim (DOS behaviour). */
            if (!bat_emit(out, &pos, cap, '%')) {
                out[0] = '\0';
                return -1;
            }
            break;
        }

        if (line[i] == '%') {
            /* "%%" -> literal '%' (DOS escape for percent sign).
             * MUTANT BATCH_MUTATE_NO_PCT_PCT: skip collapsing; emit '%'
             * twice so the test that checks for a single '%' goes RED. */
#ifdef BATCH_MUTATE_NO_PCT_PCT
            /* Emit both percent signs (the mutant deliberately breaks this). */
            if (!bat_emit(out, &pos, cap, '%')) { out[0] = '\0'; return -1; }
            if (!bat_emit(out, &pos, cap, '%')) { out[0] = '\0'; return -1; }
#else
            if (!bat_emit(out, &pos, cap, '%')) {
                out[0] = '\0';
                return -1;
            }
#endif
            i++;    /* skip the second '%' */
            continue;
        }

        if (line[i] >= '0' && line[i] <= '9') {
            /* %0..%9 -- positional parameter substitution. */
            int idx = line[i] - '0';
            i++;    /* skip the digit */

            const char *param = 0;
            if (argv != 0 && idx < argc) {
                param = argv[idx];
            }
            /* Out-of-range or NULL argv entry -> empty string (DOS 3.3). */
            if (param == 0) {
                continue;   /* emit nothing */
            }
            if (bat_emit_str(out, &pos, cap, param) < 0) {
                out[0] = '\0';
                return -1;  /* overflow */
            }
            continue;
        }

        /* Might be %VARNAME% -- scan forward for a closing '%'. */
        if (bat_is_varname_char(line[i])) {
            /* Collect the variable name into a local scratch buffer. */
            char vname[BATCH_LINE_MAX];
            int  vlen = 0;
            int  j    = i;

            while (line[j] != '\0' && line[j] != '%' &&
                   bat_is_varname_char(line[j]) &&
                   vlen < (int)(sizeof(vname) - 1)) {
                vname[vlen++] = line[j++];
            }
            vname[vlen] = '\0';

            if (line[j] == '%' && vlen > 0) {
                /* Closed %VARNAME% -- look up. */
                i = j + 1;     /* advance past the closing '%' */
                const char *val = 0;
                if (env != 0) {
                    val = env(vname, envctx);
                }
                if (val != 0) {
                    /* Defined: emit the value. */
                    if (bat_emit_str(out, &pos, cap, val) < 0) {
                        out[0] = '\0';
                        return -1;
                    }
                } else {
                    /* Undefined: emit the literal "%VARNAME%" (DOS 3.3). */
                    if (!bat_emit(out, &pos, cap, '%')) {
                        out[0] = '\0'; return -1;
                    }
                    if (bat_emit_str(out, &pos, cap, vname) < 0) {
                        out[0] = '\0'; return -1;
                    }
                    if (!bat_emit(out, &pos, cap, '%')) {
                        out[0] = '\0'; return -1;
                    }
                }
                continue;
            }
            /* Unclosed '%NAME' (no closing '%') -- fall through: emit
             * the leading '%' verbatim and restart the scan from `i`
             * (the character after the first '%'). */
        }

        /* Unrecognized '%' sequence -- emit the '%' verbatim. */
        if (!bat_emit(out, &pos, cap, '%')) {
            out[0] = '\0';
            return -1;
        }
        /* Do NOT advance i further -- restart from where the name scan left
         * off (the character that broke the varname scan). */
    }

    out[pos] = '\0';
    return pos;
}

/* ---- Internal helpers for batch_classify -------------------------------- */

/* Skip leading spaces and tabs in `s`, return a pointer to the first
 * non-whitespace character (or the NUL terminator). */
static const char *bat_skip_ws(const char *s)
{
    while (bat_isspace(*s)) {
        s++;
    }
    return s;
}

/* Copy at most BATCH_LABEL_MAX-1 characters of a label name from `src` into
 * `dst` (ASCIIZ).  A label name is terminated by whitespace, NUL, or ':'.
 * Returns the number of characters copied. */
static int bat_copy_label(char *dst, const char *src)
{
    int n = 0;
    while (*src != '\0' && !bat_isspace(*src) && *src != ':' &&
           n < BATCH_LABEL_MAX - 1) {
        dst[n++] = *src++;
    }
    dst[n] = '\0';
    return n;
}

/* Extract the GOTO target label from the argument portion of a GOTO line
 * (i.e. the text after "GOTO" and its trailing whitespace).  The leading ':'
 * is optional in real DOS 3.3 ("GOTO FOO" and "GOTO :FOO" both work); we
 * strip a leading ':' if present.
 *
 * Writes the label name (without ':') into `dst` (BATCH_LABEL_MAX).
 * Returns the number of characters written.
 *
 * MUTANT BATCH_MUTATE_GOTO_NOLABEL: always writes 0 characters so the
 * oracle that checks the extracted target goes RED.  NEVER in a real build. */
static int bat_goto_target(char *dst, const char *arg)
{
    const char *p = bat_skip_ws(arg);
#ifdef BATCH_MUTATE_GOTO_NOLABEL
    /* Mutant: write nothing -- the GOTO target test goes RED. */
    dst[0] = '\0';
    (void)p;
    return 0;
#else
    if (*p == ':') {
        p++;    /* strip the optional leading ':' */
    }
    return bat_copy_label(dst, p);
#endif
}

/* Match a directive keyword at the start of `s` (after '@' + whitespace
 * have been stripped) against `kw` (upper-case), case-insensitively.
 * The keyword must be followed by whitespace, NUL, or (for ECHO) a special
 * secondary check.  Returns 1 on match, 0 otherwise.
 *
 * `rest` is set to a pointer to the first character after the keyword
 * (and its trailing whitespace) on a match; undefined on no-match. */
static int bat_match_kw(const char *s, const char *kw, const char **rest)
{
    int klen = bat_strlen(kw);
    int i;
    for (i = 0; i < klen; i++) {
        if (bat_upcase(s[i]) != kw[i]) {
            return 0;
        }
    }
    /* The keyword must be followed by a word boundary (space/tab/NUL). */
    if (s[klen] != '\0' && !bat_isspace(s[klen])) {
        return 0;
    }
    *rest = bat_skip_ws(s + klen);
    return 1;
}

/* ---- batch_classify ------------------------------------------------------ */

batch_line_kind_t batch_classify(const char *line, batch_parsed_t *out)
{
    /* Zero-initialise all output fields. */
    out->kind          = BL_BLANK;
    out->at_suppressed = 0;
    out->label_name[0] = '\0';
    out->echo_text[0]  = '\0';
    out->goto_target[0] = '\0';

    if (line == 0) {
        return BL_BLANK;
    }

    /* Skip leading whitespace. */
    const char *p = bat_skip_ws(line);

    /* Check for '@' prefix (suppress echo of this line). */
    if (*p == '@') {
        out->at_suppressed = 1;
        p++;
        p = bat_skip_ws(p);
    }

    /* Blank line (or all whitespace / just '@'). */
    if (*p == '\0') {
        out->kind = BL_BLANK;
        return BL_BLANK;
    }

    /* Label definition: starts with ':'. */
    if (*p == ':') {
        p++;    /* skip ':' */
        bat_copy_label(out->label_name, p);
        out->kind = BL_LABEL;
        return BL_LABEL;
    }

    /* Match directive keywords (case-insensitive). */
    const char *rest = 0;

    /* REM -- comment line (no execution, no echo even when ECHO is ON). */
    if (bat_match_kw(p, "REM", &rest)) {
        out->kind = BL_REM;
        return BL_REM;
    }

    /* ECHO -- three sub-kinds: ECHO ON, ECHO OFF, ECHO <text> (or bare ECHO).
     *
     * Ref: MS-DOS 3.3 Technical Reference Chapter 3: "ECHO ON" and "ECHO OFF"
     * are matched case-insensitively; any other argument (including empty) is
     * BL_ECHO_TEXT (bare ECHO prints the current state -- the REPL handles
     * that; we just classify it as ECHO_TEXT with empty echo_text).
     *
     * MUTANT BATCH_MUTATE_ECHO_OFF_MISS: skip the OFF check so "ECHO OFF"
     * falls into BL_ECHO_TEXT; the ECHO OFF classify test goes RED. */
    if (bat_match_kw(p, "ECHO", &rest)) {
#ifndef BATCH_MUTATE_ECHO_OFF_MISS
        if (bat_stricmp(rest, "OFF") == 0) {
            out->kind = BL_ECHO_OFF;
            return BL_ECHO_OFF;
        }
#endif
        if (bat_stricmp(rest, "ON") == 0) {
            out->kind = BL_ECHO_ON;
            return BL_ECHO_ON;
        }
        /* ECHO <text> (including bare ECHO with empty rest). */
        bat_strlcpy(out->echo_text, rest, BATCH_LINE_MAX);
        out->kind = BL_ECHO_TEXT;
        return BL_ECHO_TEXT;
    }

    /* GOTO -- extract the target label. */
    if (bat_match_kw(p, "GOTO", &rest)) {
        bat_goto_target(out->goto_target, rest);
        out->kind = BL_GOTO;
        return BL_GOTO;
    }

    /* IF -- conditional (REPL evaluates; we just classify). */
    if (bat_match_kw(p, "IF", &rest)) {
        out->kind = BL_IF;
        return BL_IF;
    }

    /* FOR -- loop (REPL expands set and iterates; we just classify). */
    if (bat_match_kw(p, "FOR", &rest)) {
        out->kind = BL_FOR;
        return BL_FOR;
    }

    /* SHIFT -- shift positional parameters left by one. */
    if (bat_match_kw(p, "SHIFT", &rest)) {
        out->kind = BL_SHIFT;
        return BL_SHIFT;
    }

    /* CALL -- invoke a nested batch file. */
    if (bat_match_kw(p, "CALL", &rest)) {
        out->kind = BL_CALL;
        return BL_CALL;
    }

    /* PAUSE -- display "Strike a key..." and wait for a keystroke. */
    if (bat_match_kw(p, "PAUSE", &rest)) {
        out->kind = BL_PAUSE;
        return BL_PAUSE;
    }

    /* Anything else: an external or internal DOS command to be executed. */
    out->kind = BL_COMMAND;
    return BL_COMMAND;
}

/* ---- batch_label_matches ------------------------------------------------ */

int batch_label_matches(const char *line, const char *target)
{
    if (line == 0 || target == 0) {
        return 0;
    }

    /* Skip leading whitespace. */
    const char *p = bat_skip_ws(line);

    /* Must start with ':'. */
    if (*p != ':') {
        return 0;
    }
    p++;    /* skip ':' */

    /* Extract the label name from the line into a local buffer. */
    char lname[BATCH_LABEL_MAX];
    bat_copy_label(lname, p);

    /* Compare case-insensitively (DOS 3.3 GOTO is case-insensitive). */
    return (bat_stricmp(lname, target) == 0) ? 1 : 0;
}
