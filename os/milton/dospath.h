/* Drive-qualified DOS path text, shared by the external commands
 * (initech-q45q; audit 2026-10-23-dos L012/L014/L032).
 *
 * Law 1: MS-DOS 3.3 User's Reference pp4-11 (PDF89-96): a file
 * specification is [d:][path]filename, every drive keeps its OWN current
 * directory, and "." / ".." name the current/parent directory; p.42
 * (PDF127): CHDIR d: reports drive d's directory. IBM DOS 3.30 Technical
 * Reference p6-188 (PDF311): AH=47h GETCWD takes DL=drive (0=default,
 * 1=A:) and returns the path without drive or leading '\'. ADR-0003 DEC-18
 * + spec/drive_letters.json: A, C and D are independently mounted volumes.
 *
 * The result is always "D:\DIR\LEAF" (uppercase, drive explicit, root =
 * "D:\"), so every later INT 21h call names its own volume and nothing
 * silently inherits the default drive's volume or directory -- the
 * L032 defect, where a D: target was resolved under A:'s directory tree.
 * Header-only: each flat .COM includes it; no shared object, no host code.
 */
#ifndef INITECH_DOSPATH_H
#define INITECH_DOSPATH_H

#define DOSPATH_OK         0
#define DOSPATH_BAD_DRIVE (-1)   /* letter not A..Z, or GETCWD refused it   */
#define DOSPATH_TOO_LONG  (-2)   /* would not fit the caller's buffer       */

/* GETCWD for a zero-based drive (0=A:): root-relative text into out[64],
 * "" at the root; negative when the drive is invalid (DOS error 15). */
typedef int (*dospath_cwd_fn)(unsigned drive, char out[64]);

/* The default drive's letter index when `in` carries no "d:" prefix. */
static inline int dospath_drive(const char *in, unsigned def)
{
    if (!in[0] || in[1] != ':') return (int)def;
    char c = in[0];
    if (c >= 'a' && c <= 'z') c = (char)(c - 32);
    return c >= 'A' && c <= 'Z' ? c - 'A' : DOSPATH_BAD_DRIVE;
}

static inline int dospath_full(const char *in, unsigned def,
                               dospath_cwd_fn cwd, char *out, unsigned cap)
{
    int drive = dospath_drive(in, def);
    char rel[64], raw[192];
    unsigned n = 0, end = 3;
    if (drive < 0) return DOSPATH_BAD_DRIVE;
    if (in[0] && in[1] == ':') in += 2;
    /* Ask the named drive even for an absolute path: an absent letter
     * fails here, before any file service could alias another volume. */
    if (cwd((unsigned)drive, rel) < 0) return DOSPATH_BAD_DRIVE;
    if (cap < 4) return DOSPATH_TOO_LONG;
    if (*in != '\\') {
        for (unsigned i = 0; rel[i] && i < 63; i++) raw[n++] = rel[i];
        raw[n++] = '\\';
    }
    for (; *in; in++) {
        if (n + 1 >= sizeof raw) return DOSPATH_TOO_LONG;
        char c = *in;
        raw[n++] = c >= 'a' && c <= 'z' ? (char)(c - 32) : c;
    }
    raw[n] = 0;
    out[0] = (char)('A' + drive); out[1] = ':'; out[2] = '\\'; out[3] = 0;
    for (unsigned pos = 0; pos < n;) {
        while (raw[pos] == '\\') pos++;
        unsigned start = pos;
        while (raw[pos] && raw[pos] != '\\') pos++;
        unsigned len = pos - start;
        if (!len || (len == 1 && raw[start] == '.')) continue;
        if (len == 2 && raw[start] == '.' && raw[start + 1] == '.') {
            while (end > 3 && out[end - 1] != '\\') end--;
            if (end > 3) end--;
            out[end] = 0;
            continue;
        }
        if (end + (end > 3) + len >= cap) return DOSPATH_TOO_LONG;
        if (end > 3) out[end++] = '\\';
        for (unsigned i = 0; i < len; i++) out[end++] = raw[start + i];
        out[end] = 0;
    }
    return DOSPATH_OK;
}

/* Cut "D:\A\B" to its parent "D:\A"; the root "D:\" is its own parent. */
static inline void dospath_parent(char *p)
{
    unsigned n = 0;
    while (p[n]) n++;
    while (n > 3 && p[n - 1] != '\\') n--;
    p[n > 3 ? n - 1 : 3] = 0;
}

#endif
