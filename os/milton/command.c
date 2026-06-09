/* command.c -- InitechDOS COMMAND.COM-alike interactive shell (the A:\> REPL).
 *
 * beads: initech-7pc. M2 capstone (PRD Sec 6.1). See command.h for the full
 *        citation block + the host-testability seam rationale.
 * Ref:   ADR-0003 DEC-11/DEC-12 + Appendix D; DOS 3.3 COMMAND.COM behaviour;
 *        spec/dos_messages.json (MSG-DOS-0002/0003); spec/find_data.h (the
 *        43-byte DTA find-record); spec/int21h_calling_convention.json (DEC-04a
 *        flat ABI: AH=func, EBX=handle, ECX=count, EDX=flat ptr, EAX=ret, CF).
 *        CLAUDE.md Law 1/2/3/4, Rule 2 (fail loud), Rule 11, Rule 12.
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only. The
 * pure logic (cmd_*) carries NO asm + NO I/O so the SAME TU compiles HOSTED for
 * os/milton/test_command.c. The REPL + its `int $0x21` wrappers are kernel-only,
 * guarded by COMMAND_KERNEL_REPL so the host build never sees inline asm.
 */

#include "command.h"
#include "dos_structs.h"   /* DIR_ATTR_DIRECTORY -- the DIR formatter's attr bit */

/* ---- PURE shell logic (host-testable; the test_command.c oracle drives these) */

char cmd_upcase_char(char c)
{
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

void cmd_upcase_str(char *s)
{
    if (s == 0) {
        return;
    }
    for (; *s; s++) {
        *s = cmd_upcase_char(*s);
    }
}

static int is_space(char c)
{
    return c == ' ' || c == '\t';
}

cmd_kind_t cmd_classify(const char *upper_command)
{
    if (upper_command == 0 || upper_command[0] == '\0') {
        return CMD_EMPTY;
    }
    /* A tiny hand-rolled strcmp keeps this freestanding (no libc). */
    struct { const char *name; cmd_kind_t kind; } table[] = {
        { "DIR",   CMD_DIR  },
        { "TYPE",  CMD_TYPE },
        { "CD",    CMD_CD   },
        { "CHDIR", CMD_CD   },   /* CHDIR is the long form of CD (DOS) */
        { "CLS",   CMD_CLS  },
        { "VER",   CMD_VER  },
        { "ECHO",  CMD_ECHO },
        { "EXIT",  CMD_EXIT },
    };
    for (unsigned i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        const char *a = upper_command;
        const char *b = table[i].name;
        while (*a && *b && *a == *b) {
            a++; b++;
        }
        if (*a == '\0' && *b == '\0') {
            return table[i].kind;
        }
    }
#ifdef CMD_MUTATE_BADCMD_BUILTIN
    /* MUTANT (Rule 6; make test-command-mutant only): treat an unknown command
     * word as a built-in (CMD_DIR) instead of CMD_EXTERNAL, so "badcmd" would
     * never reach EXEC / the Bad command diagnostic -> the classify oracle goes
     * RED. NEVER in a real build. */
    return CMD_DIR;
#else
    return CMD_EXTERNAL;
#endif
}

void cmd_parse(const char *line, cmd_line_t *out)
{
    int i;

    /* Defensive init (Rule 2): never leave the output uninitialized. */
    out->command[0] = '\0';
    out->arg[0]     = '\0';
    out->kind       = CMD_EMPTY;

    if (line == 0) {
        return;
    }

    const char *p = line;
    while (is_space(*p)) {           /* skip leading whitespace */
        p++;
    }

    /* Copy the command word (up to the first space), upper-casing as we go.
     * Clamp to CMD_TOKEN_MAX-1 so an absurdly long word cannot overflow. */
    i = 0;
    while (*p && !is_space(*p) && i < CMD_TOKEN_MAX - 1) {
#ifdef CMD_MUTATE_NO_UPCASE
        /* MUTANT (Rule 6; make test-command-mutant only): copy the command word
         * WITHOUT upper-casing, so a lowercase "dir" no longer matches the
         * upper-case dispatch table -> the parse oracle goes RED. NEVER in a
         * real build. */
        out->command[i++] = *p;
#else
        out->command[i++] = cmd_upcase_char(*p);
#endif
        p++;
    }
    out->command[i] = '\0';
    /* If the word was longer than the clamp, swallow the rest of it so it does
     * not bleed into arg (it is already a non-recognized command anyway). */
    while (*p && !is_space(*p)) {
        p++;
    }

    while (is_space(*p)) {           /* skip the spaces after the command */
        p++;
    }

    /* Copy the rest verbatim (case preserved -- ECHO needs it). */
    i = 0;
    while (*p && i < CMD_LINE_MAX - 1) {
        out->arg[i++] = *p++;
    }
    out->arg[i] = '\0';

    out->kind = cmd_classify(out->command);
}

int cmd_first_token(const char *arg, char *out)
{
    int i = 0;
    const char *p = arg;

    out[0] = '\0';
    if (arg == 0) {
        return 0;
    }
    while (is_space(*p)) {            /* skip any leading spaces */
        p++;
    }
    while (*p && !is_space(*p) && i < CMD_TOKEN_MAX - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';
    return i;
}

int cmd_append_com(const char *name, char *out)
{
    int n = 0;
    int has_dot = 0;
    const char *p;

    if (name == 0) {
        out[0] = '\0';
        return 0;
    }
    for (p = name; *p; p++) {
        if (*p == '.') {
            has_dot = 1;
        }
    }

    /* Copy the name. */
    for (p = name; *p && n < CMD_TOKEN_MAX - 1; p++) {
        out[n++] = *p;
    }
    if (*p != '\0') {
        /* The name itself did not fit -- never truncate into a wrong file. */
        out[0] = '\0';
        return 0;
    }

#ifdef CMD_MUTATE_COM_ALWAYS
    /* MUTANT (Rule 6; make test-command-mutant only): append ".COM" ALWAYS, even
     * when the name already carries an extension, so "GREET.COM" becomes
     * "GREET.COM.COM" -> the appender oracle goes RED. NEVER in a real build. */
    has_dot = 0;
#endif
    if (!has_dot) {
        /* Append ".COM" only if it fits (n + 4 chars + NUL). */
        const char *ext = ".COM";
        if (n + 4 >= CMD_TOKEN_MAX) {
            out[0] = '\0';
            return 0;
        }
        for (const char *e = ext; *e; e++) {
            out[n++] = *e;
        }
    }
    out[n] = '\0';
    return 1;
}

/* Append an unsigned decimal to out[*pos], right-justified in `width` columns
 * (space-padded). Pure formatting helper for the DIR line. */
static void put_u_field(char *out, int *pos, uint32_t v, int width)
{
    char tmp[10];
    int n = 0;
    int pad;

    if (v == 0) {
        tmp[n++] = '0';
    } else {
        while (v > 0 && n < (int)sizeof(tmp)) {
            tmp[n++] = (char)('0' + (v % 10u));
            v /= 10u;
        }
    }
    for (pad = width - n; pad > 0; pad--) {
        out[(*pos)++] = ' ';
    }
    while (n > 0) {
        out[(*pos)++] = tmp[--n];
    }
}

int cmd_format_dir_line(const char *fname, uint32_t fsize, uint8_t attr,
                        char *out)
{
    int pos = 0;
    int i;

    /* Left-justify the 8.3 name in a 13-column field (NAME.EXT, max 12 + pad).
     * This is a plausible period-DOS DIR layout (Law 4), not byte-exact DOS. */
    for (i = 0; fname[i] != '\0' && i < 12; i++) {
        out[pos++] = fname[i];
    }
    for (; i < 13; i++) {
        out[pos++] = ' ';
    }

    if (attr & DIR_ATTR_DIRECTORY) {
        const char *d = "      <DIR>";   /* right-ish DIR marker in the size col */
        for (const char *q = d; *q; q++) {
            out[pos++] = *q;
        }
    } else {
        put_u_field(out, &pos, fsize, 11);
    }
    out[pos++] = '\n';
    out[pos]   = '\0';
    return pos;
}

/* ===========================================================================
 * The REPL (kernel-only). Everything below dogfoods the INT 21h API via real
 * `int $0x21` calls -- the authentic COMMAND.COM design + proof the OS API is a
 * usable surface (the brief's central design point). Compiled out of the host
 * build (test_command.c never defines COMMAND_KERNEL_REPL) so the pure logic
 * above stays asm-free + host-testable (Law 3).
 * ===========================================================================*/
#ifdef COMMAND_KERNEL_REPL

#include "find_data.h"   /* find_data_t (43-byte DTA record; offsets DIR reads) */

/* DEC-04a flat ABI (spec/int21h_calling_convention.json): AH=function in EAX,
 * EBX=handle, ECX=count, EDX=flat ptr, EAX=return, CF=error in EFLAGS. The
 * wrappers below issue a literal `int $0x21` honoring exactly that contract --
 * the SAME path a real DOS COMMAND.COM uses (it is itself a .COM that calls the
 * INT 21h services). We capture CF with `sbb` (carry -> 0xFFFFFFFF/0). */

/* AH=09h DISPLAY STRING: EDX -> '$'-terminated string -> CON. */
static void dos_print(const char *dollar_terminated)
{
    __asm__ __volatile__(
        "int $0x21"
        :
        : "a"(0x0900u), "d"((uint32_t)(uintptr_t)dollar_terminated)
        : "cc", "memory");
}

/* Print an ASCIIZ string by writing it to stdout (handle 1) via AH=40h. Used
 * for output that is not naturally '$'-terminated (it may contain '$'). */
static void dos_write(const char *s, uint32_t len)
{
    __asm__ __volatile__(
        "int $0x21"
        :
        : "a"(0x4000u), "b"(1u), "c"(len), "d"((uint32_t)(uintptr_t)s)
        : "cc", "memory");
}

static uint32_t str_len(const char *s)
{
    uint32_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

static void dos_puts_raw(const char *s)
{
    dos_write(s, str_len(s));
}

/* AH=0Ah BUFFERED INPUT: EDX -> the buffered-input block (byte0=max incl CR,
 * byte1=count written, bytes2..=chars+CR). Blocks until Enter (the keyboard
 * IRQ wakes the kernel's blocking conin; --keys injection drives it in the
 * oracle). */
static void dos_getline(uint8_t *buf)
{
    __asm__ __volatile__(
        "int $0x21"
        :
        : "a"(0x0A00u), "d"((uint32_t)(uintptr_t)buf)
        : "cc", "memory");
}

/* AH=1Ah SET DTA: EDX -> the new Disk Transfer Area (where FINDFIRST/NEXT write
 * the 43-byte find-record). */
static void dos_setdta(void *dta)
{
    __asm__ __volatile__(
        "int $0x21"
        :
        : "a"(0x1A00u), "d"((uint32_t)(uintptr_t)dta)
        : "cc", "memory");
}

/* AH=4Eh FINDFIRST: EDX -> ASCIIZ file spec, ECX = attribute mask. Returns 1 on
 * a hit (CF clear), 0 on no-match (CF set). */
static int dos_findfirst(const char *spec, uint16_t attr)
{
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %0, %0"
        : "=r"(carry)
        : "a"(0x4E00u), "c"((uint32_t)attr), "d"((uint32_t)(uintptr_t)spec)
        : "cc", "memory");
    return carry == 0u;
}

/* AH=4Fh FINDNEXT: continue the active search. Returns 1 on a hit, 0 when done. */
static int dos_findnext(void)
{
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %0, %0"
        : "=r"(carry)
        : "a"(0x4F00u)
        : "cc", "memory");
    return carry == 0u;
}

/* AH=3Dh OPEN: EDX -> ASCIIZ path, AL = mode (0 = read). Returns the handle
 * (>=0) on success, or -(error) on failure (CF set). */
static int dos_open(const char *path)
{
    uint32_t ax = 0x3D00u;   /* AH=3Dh, AL=00 (read) */
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "d"((uint32_t)(uintptr_t)path)
        : "cc", "memory");
    if (carry != 0u) {
        return -(int)(ax & 0xFFFFu);
    }
    return (int)(ax & 0xFFFFu);
}

/* AH=3Fh READ: EBX=handle, ECX=count, EDX=buffer. Returns bytes read (0=EOF). */
static uint32_t dos_read(int handle, uint8_t *buf, uint32_t count)
{
    uint32_t ax = 0x3F00u;
    __asm__ __volatile__(
        "int $0x21"
        : "+a"(ax)
        : "b"((uint32_t)handle), "c"(count), "d"((uint32_t)(uintptr_t)buf)
        : "cc", "memory");
    return ax;   /* EAX = bytes read */
}

/* AH=3Eh CLOSE: EBX=handle. */
static void dos_close(int handle)
{
    __asm__ __volatile__(
        "int $0x21"
        :
        : "a"(0x3E00u), "b"((uint32_t)handle)
        : "cc", "memory");
}

/* AH=4Bh EXEC (AL=00 load+execute): EDX -> ASCIIZ program path. Returns 0 on a
 * clean run (CF clear), or the DOS error code (CF set; e.g. 0x0002 not found). */
static uint16_t dos_exec(const char *path)
{
    uint32_t ax = 0x4B00u;
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "d"((uint32_t)(uintptr_t)path)
        : "cc", "memory");
    if (carry != 0u) {
        return (uint16_t)(ax & 0xFFFFu);
    }
    return 0u;
}

/* AH=30h GET VERSION: AL=major, AH=minor. */
static void dos_getver(uint8_t *major, uint8_t *minor)
{
    uint32_t ax = 0x3000u;
    __asm__ __volatile__(
        "int $0x21"
        : "+a"(ax)
        :
        : "cc", "memory");
    *major = (uint8_t)(ax & 0xFFu);
    *minor = (uint8_t)((ax >> 8) & 0xFFu);
}

/* The shell's DTA: a dedicated 43-byte find-record buffer FINDFIRST/NEXT write
 * into (bound via AH=1Ah). File scope so it outlives each DIR invocation. */
static find_data_t g_shell_dta;

/* A scratch line buffer for output formatting (DIR lines, etc.). */
static char g_out[64];

/* ---- the controlled diagnostics (spec/dos_messages.json, ADR-0003 App C) ----
 * Transcribed VERBATIM; the controlled vocabulary (DEC-13) -- no rewording. */
#define MSG_BAD_COMMAND   "Bad command or file name"   /* MSG-DOS-0002 */
#define MSG_FILE_NOT_FND  "File not found"              /* MSG-DOS-0003 */

/* ---- the built-in command handlers ---------------------------------------- */

/* DIR: FINDFIRST "*.*" then loop FINDNEXT, formatting each find-record into a
 * DOS-like line. A header ("Directory of A:\") + a file-count footer frame it
 * (Law 4 -- plausibly period DOS). The find-record fields are read at the
 * spec/find_data.h offsets (fname@0x15, fsize@0x11, attr@0x0C). */
static void builtin_dir(void)
{
    int n;
    uint32_t files = 0;

    dos_setdta(&g_shell_dta);
    dos_print("\r\n Directory of A:\\\r\n\r\n$");

    /* Search attribute 0x10 (DIRECTORY) so subdir entries are listed too; the
     * dispatcher already includes plain files. */
    if (!dos_findfirst("*.*", DIR_ATTR_DIRECTORY)) {
        dos_print(MSG_FILE_NOT_FND "\r\n$");
        return;
    }
    do {
        n = cmd_format_dir_line(g_shell_dta.fname, g_shell_dta.fsize,
                                g_shell_dta.attr, g_out);
        /* cmd_format_dir_line ends the line with '\n'; the CON console + serial
         * treat '\n' as newline (the banner uses '\n' too). */
        dos_write(g_out, (uint32_t)n);
        files++;
    } while (dos_findnext());

    /* File-count footer. */
    {
        int pos = 0;
        put_u_field(g_out, &pos, files, 0);
        g_out[pos] = '\0';
        dos_print("\r\n$");
        dos_puts_raw(g_out);
        dos_print(" file(s)\r\n$");
    }
}

/* TYPE <file>: OPEN the named file, READ it in chunks, WRITE each chunk to
 * stdout (handle 1), CLOSE. File-not-found => the controlled diagnostic. */
static void builtin_type(const char *arg)
{
    char name[CMD_TOKEN_MAX];
    int handle;
    uint8_t chunk[128];
    uint32_t got;

    if (cmd_first_token(arg, name) == 0) {
        /* No filename given -- MSG-DOS-0011 "Required parameter missing". */
        dos_print("Required parameter missing\r\n$");
        return;
    }
    cmd_upcase_str(name);   /* DOS upcases 8.3 names */

    handle = dos_open(name);
    if (handle < 0) {
        dos_print(MSG_FILE_NOT_FND "\r\n$");
        return;
    }
    for (;;) {
        got = dos_read(handle, chunk, sizeof(chunk));
        if (got == 0u) {
            break;          /* EOF */
        }
        dos_write((const char *)chunk, got);
    }
    dos_close(handle);
    dos_print("\r\n$");     /* ensure a trailing newline after the file body */
}

/* CD/CHDIR: no arg => print the current directory (A:\). "CD \" => root (the
 * only directory this milestone). A subdirectory arg => an honest "not yet
 * supported" message (real traversal is initech-ti8) -- present + loud (Rule 2),
 * never silently wrong. */
static void builtin_cd(const char *arg)
{
    char tok[CMD_TOKEN_MAX];
    int len = cmd_first_token(arg, tok);

    if (len == 0) {
        dos_print("A:\\\r\n$");           /* print the current directory */
        return;
    }
    if (tok[0] == '\\' && tok[1] == '\0') {
        dos_print("A:\\\r\n$");           /* CD \ -> already at the root */
        return;
    }
    /* Any other path: subdirectory traversal is deferred (initech-ti8). */
    dos_print("Subdirectory traversal not yet supported\r\n$");
}

/* CLS: clear the screen. The console exposes no clear hook this milestone, so we
 * emit a form feed (0x0C) -- a period-authentic DOS CLS signal -- followed by a
 * page of blank lines as a simple, deterministic fallback. Kept minimal. */
static void builtin_cls(void)
{
    /* Form feed first (a real terminal/console would clear on this). */
    dos_print("\f$");
    for (int i = 0; i < 25; i++) {
        dos_print("\r\n$");
    }
}

/* VER: print the InitechDOS version line. Source the numbers from AH=30h
 * GETVER (3.30; ADR-0003 DEC-12) rather than hard-coding, so a GETVER change
 * is reflected. */
static void builtin_ver(void)
{
    uint8_t major = 0, minor = 0;
    int pos = 0;

    dos_getver(&major, &minor);
    dos_print("\r\nInitechDOS Version $");
    put_u_field(g_out, &pos, (uint32_t)major, 0);
    g_out[pos++] = '.';
    /* minor is the two-digit fractional part (30 -> ".30"); pad to 2 digits. */
    if (minor < 10u) {
        g_out[pos++] = '0';
    }
    put_u_field(g_out, &pos, (uint32_t)minor, 0);
    g_out[pos] = '\0';
    dos_puts_raw(g_out);
    dos_print("\r\n$");
}

/* ECHO <text>: print the text. With no arg, print the on/off status (minimal --
 * DOS prints "ECHO is on"). */
static void builtin_echo(const char *arg)
{
    char first[CMD_TOKEN_MAX];
    if (cmd_first_token(arg, first) == 0) {
        dos_print("ECHO is on\r\n$");
        return;
    }
    dos_puts_raw(arg);
    dos_print("\r\n$");
}

/* External command: append .COM (if no extension), upcase, AH=4Bh EXEC. On a
 * not-found / load failure => the controlled "Bad command or file name". On a
 * clean run control simply returns to the prompt. */
static void run_external(const char *command, const char *arg)
{
    char name[CMD_TOKEN_MAX];
    char prog[CMD_TOKEN_MAX];
    uint16_t err;
    (void)arg;   /* command-tail args to the child are deferred (no PSP tail yet) */

    /* `command` is already upper-cased by cmd_parse. Append .COM if needed. */
    if (!cmd_append_com(command, prog)) {
        dos_print(MSG_BAD_COMMAND "\r\n$");
        return;
    }
    /* Defensive: upcase again (cmd_append_com preserves what it was given). */
    (void)name;
    cmd_upcase_str(prog);

    err = dos_exec(prog);
    if (err != 0u) {
        /* Not found / bad format / nested -> the canonical DOS diagnostic. */
        dos_print(MSG_BAD_COMMAND "\r\n$");
        return;
    }
    /* Clean run: control is back; the next prompt follows. */
}

/* Read one line from the keyboard via AH=0Ah into `out` (ASCIIZ, CR stripped).
 * The AH=0Ah block: byte0 = max incl CR (we preset it), byte1 = count written,
 * bytes2.. = chars + CR. We copy the `count` chars out and NUL-terminate. */
static void read_line(char *out)
{
    /* +2 for the AH=0Ah header (max + count) byte slots. */
    uint8_t blk[CMD_LINE_MAX + 2];
    uint8_t count;
    uint8_t i;

    blk[0] = (uint8_t)CMD_LINE_MAX;   /* max length including the CR */
    blk[1] = 0;
    dos_getline(blk);

    count = blk[1];
    for (i = 0; i < count && i < (uint8_t)(CMD_LINE_MAX - 1); i++) {
        out[i] = (char)blk[2 + i];
    }
    out[i] = '\0';
}

void command_repl(void)
{
    char line[CMD_LINE_MAX];
    cmd_line_t parsed;

    /* A serial marker the oracle gates key-injection on (mirrors KBD-ECHO-READY
     * / CONIN-PROG-READY). The kernel's CON sink also fans this to serial via
     * the AH=09h banner, but a distinct grep-able marker keeps the gate robust;
     * kmain prints SHELL-READY before calling us, so we don't duplicate it. */
    for (;;) {
        /* $P$G prompt = current drive + path + '>' (ADR-0003 DEC-11). Root only
         * this milestone => "A:\>". */
        dos_print("A:\\>$");

        read_line(line);
        cmd_parse(line, &parsed);

        switch (parsed.kind) {
            case CMD_EMPTY:
                break;                       /* blank line -> just re-prompt */
            case CMD_DIR:
                builtin_dir();
                break;
            case CMD_TYPE:
                builtin_type(parsed.arg);
                break;
            case CMD_CD:
                builtin_cd(parsed.arg);
                break;
            case CMD_CLS:
                builtin_cls();
                break;
            case CMD_VER:
                builtin_ver();
                break;
            case CMD_ECHO:
                builtin_echo(parsed.arg);
                break;
            case CMD_EXIT:
                /* Grep-able clean-exit marker. Plain '\n' (no CR) so the serial
                 * line is exactly "SHELL-EXIT" -- the oracle's ^SHELL-EXIT$
                 * anchored match would miss a trailing CR (the same serial-clean
                 * discipline kmain uses for its markers). */
                dos_print("SHELL-EXIT\n$");
                return;
            case CMD_EXTERNAL:
            default:
                run_external(parsed.command, parsed.arg);
                break;
        }
    }
}

#endif /* COMMAND_KERNEL_REPL */
