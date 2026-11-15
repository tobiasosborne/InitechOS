/* Replay of every pinned real III+ forms capture (initech-nic17).
 *
 * Each case in tests/forms_replay.cases is a directory under
 * tests/xbase_prog_diff/screen-mint minted twice by `make mint-forms` from
 * real DBASE.EXE 1.1 on DOS 3.30. The replay copies its SETUP files into a
 * scratch directory, runs the product's own dot prompt (ibase_repl) on the
 * host PAL, and types the case's KEYS stream: command lines go to the dot
 * prompt, single keys to READ. A DUMP is taken when the product next asks
 * for input, as the TSR took it once the keyboard queue had drained. Every
 * dump is compared whole (2000 cells of text and attribute plus the cursor)
 * with the captured .screen, and every EXPORTS file with the captured
 * readback: exact bytes, except a .DBF header's date (1-3) and reserved
 * bytes (12-31). Expected bytes come only from the captures.
 *
 * The capture programs hold before some READs with an INKEY loop that the
 * mint released with F2 (scaffolding, design-base-forms section 3). The
 * captures prove only that real INKEY() is non-zero for F2; its value is
 * uncaptured and the product's INKEY returns 0 for it. So the replay gives
 * each barrier's F2 to INKEY as the byte 'F' (a non-zero INKEY value) and
 * never to READ. The programs and other keys are replayed unchanged.
 *
 * A known disagreement is a `differs` line naming exactly what may differ:
 * for a dump `rows:A-B` (text and attribute cells of those rows) and
 * `cursor`, for an export `bytes:N` or `bytes:A-B` (lengths must agree),
 * `lines:N` (CR/LF lines, same count) or `absent` (the product writes no
 * such file); parts join with `+`. Everything outside the spec is still
 * compared exactly, the exempt part must still differ (a fix that makes it
 * agree fails until the record is removed), and every run proves the
 * comparator rejects corruption outside a spec (initech-nic17 review: a
 * whole-item exemption accepted a missing OUT.MEM). A case whose program
 * cannot run here is an `elsewhere` line naming the test that grades it;
 * nothing of it counts as agreeing in this replay. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include "test_assert.h"
#include "screen_file.h"
#include "w2_host.h"
#include "ibase/screen.h"
TEST_HARNESS();
struct pal_host_cfg {uint8_t yy,mm,dd;uint32_t heap;};
extern ibase_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(ibase_pal_t *);

#define BASE "tests/xbase_prog_diff/screen-mint"
#define MAX_EV 4096
#define MAX_DIFFERS 128

/* ---- page model: the DOS PAL's console (pal/pal_dos.c) on the host ---- */
static screen_file page;
static ibase_screen model;
static void draw(void *u, uint8_t r, uint8_t c, uint8_t a, const char *s, uint32_t n)
{
    (void)u;
    if (r >= 25 || c + n > 80) return;
    for (uint32_t i = 0; i < n; i++) { page.bytes[2*(r*80+c+i)] = (uint8_t)s[i]; page.bytes[2*(r*80+c+i)+1] = a; }
}
static void scroll(void *u, uint8_t top, uint8_t bottom)
{
    (void)u;
    for (uint32_t r = top; r < bottom; r++) memcpy(page.bytes + r*160, page.bytes + (r+1)*160, 160);
    for (uint32_t c = 0; c < 80; c++) { page.bytes[bottom*160+c*2] = ' '; page.bytes[bottom*160+c*2+1] = 7; }
}
static void page_init(void)
{
    for (int i = 0; i < 2000; i++) { page.bytes[2*i] = ' '; page.bytes[2*i+1] = 7; }
    ibase_screen_init(&model, draw, NULL);
    model.scroll = scroll;
}
static int h_paint(ibase_pal_t *p, uint8_t r, uint8_t c, const char *s, uint32_t n)
{
    (void)p;
    if (!n) { if (r > 25 || c >= 80) return -PAL_EIO; model.row = r; model.col = c; return 0; }
    if (r >= 25 || c + n > 80) return -PAL_EIO;
    memcpy(model.text[r] + c, s, n); draw(NULL, r, c, model.attr, s, n);
    model.row = r; model.col = (uint8_t)(c + n < 80 ? c + n : 79);
    return 0;
}
static int h_say(ibase_pal_t *p, uint8_t r, uint8_t c, const char *s, uint32_t n)
{ (void)p; ibase_screen_at(&model, r, c, s, n); return 0; }
static void h_gotoxy(ibase_pal_t *p, uint8_t r, uint8_t c) { (void)p; model.row = r; model.col = c; }
static void h_attr(ibase_pal_t *p, uint8_t a) { (void)p; model.attr = a; }
/* As pal_dos.c dos_conout: BEL goes to CON and draws no cell. */
static void h_conout(ibase_pal_t *p, const char *s, uint32_t n)
{
    (void)p;
    for (uint32_t i = 0, start = 0; i <= n; i++)
        if (i == n || s[i] == '\a') { ibase_screen_write(&model, s + start, i - start); start = i + 1; }
}
static void h_prompt(ibase_pal_t *p, int scoreboard, int cleared)
{ (void)p; ibase_screen_prompt(&model, scoreboard, cleared); }
static void h_status(ibase_pal_t *p, int on, const char *path, uint32_t rec, uint32_t count)
{ (void)p; (void)on; (void)path; (void)rec; (void)count; }

/* ---- the key stream ---- */
enum { EV_BYTE, EV_DUMP, EV_RELEASE };
static struct { int kind; int byte; char name[64]; } ev[MAX_EV];
static int nev, iev;
static const char *case_name;
static int dumps_seen, dumps_bad, phase_setup;
static char setup_cmd[] = "DO SETUP\r";
static int setup_i;

struct differ { char dir[64], item[80], spec[200]; int seen; };
static struct differ differs[MAX_DIFFERS];
static int ndiffers;
static const char *recorded(const char *dir, const char *item)
{
    for (int i = 0; i < ndiffers; i++)
        if (!strcmp(differs[i].dir, dir) && !strcmp(differs[i].item, item)) { differs[i].seen++; return differs[i].spec; }
    return NULL;
}
/* Does a spec cover value v of part `kind`? Parts are kind:N or kind:A-B
 * joined by '+'; a bare kind ("cursor", "absent") covers all of its kind. */
static int spec_covers(const char *spec, const char *kind, long v)
{
    size_t kl = strlen(kind);
    for (const char *p = spec; p && *p; ) {
        const char *e = strchr(p, '+'); size_t n = e ? (size_t)(e - p) : strlen(p);
        if (n >= kl && !strncmp(p, kind, kl)) {
            if (n == kl) return 1;
            if (p[kl] == ':') {
                long a = strtol(p + kl + 1, NULL, 10), b = a;
                const char *dash = memchr(p + kl + 1, '-', n - kl - 1);
                if (dash) b = strtol(dash + 1, NULL, 10);
                if (v >= a && v <= b) return 1;
            }
        }
        p = e ? e + 1 : NULL;
    }
    return 0;
}
/* 1 when every cell outside the spec agrees; *inside when a part inside differs. */
static int page_agrees(const unsigned char *got, const unsigned char *want, const char *spec, int *inside)
{
    int ok = 1; *inside = 0;
    for (int i = 0; i < 4000; i++) {
        if (got[i] == want[i]) continue;
        if (spec && spec_covers(spec, "rows", i / 160)) *inside = 1; else ok = 0;
    }
    if (got[4000] != want[4000] || got[4001] != want[4001]) {
        if (spec && spec_covers(spec, "cursor", 0)) *inside = 1; else ok = 0;
    }
    return ok;
}
/* The same for an export (gn < 0: the product wrote no such file). */
static int file_agrees(const unsigned char *got, long gn, const unsigned char *want, long wn, const char *spec, int *inside)
{
    *inside = 0;
    if (gn < 0) { if (spec && spec_covers(spec, "absent", 0)) { *inside = 1; return 1; } return 0; }
    if (spec && strstr(spec, "lines:")) {
        long gi = 0, wi = 0; int line = 1, ok = 1;
        while (gi < gn || wi < wn) {
            long ge = gi, we = wi;
            if ((gi >= gn) != (wi >= wn)) return 0;   /* line counts differ */
            while (ge < gn && !(got[ge] == '\r' && ge + 1 < gn && got[ge+1] == '\n')) ge++;
            while (we < wn && !(want[we] == '\r' && we + 1 < wn && want[we+1] == '\n')) we++;
            if (ge - gi != we - wi || memcmp(got + gi, want + wi, (size_t)(ge - gi))) {
                if (spec_covers(spec, "lines", line)) *inside = 1; else ok = 0;
            }
            gi = ge + 2; wi = we + 2; line++;
        }
        return ok;
    }
    if (gn != wn) return 0;
    int ok = 1;
    for (long i = 0; i < wn; i++) {
        if (got[i] == want[i]) continue;
        if (spec && spec_covers(spec, "bytes", i)) *inside = 1; else ok = 0;
    }
    return ok;
}
/* Agree, or differ only inside the recorded spec, and still there. */
static void judge(const char *item, int agrees, int inside, const char *spec, const char *detail)
{
    char msg[300];
    if (spec) {
        snprintf(msg, sizeof msg, "%s %s agrees outside its recorded spec %s", case_name, item, spec);
        if (!agrees) fprintf(stderr, "RED %s %s: differs outside %s: %s\n", case_name, item, spec, detail);
        CHECK(agrees, msg);
        snprintf(msg, sizeof msg, "%s %s: recorded disagreement still differs", case_name, item);
        if (agrees && !inside) fprintf(stderr, "STALE %s %s: now agrees; remove its differs line\n", case_name, item);
        CHECK(!agrees || inside, msg);
    } else {
        snprintf(msg, sizeof msg, "%s %s equals the real III+ capture", case_name, item);
        if (!agrees) fprintf(stderr, "RED %s %s: %s\n", case_name, item, detail);
        CHECK(agrees, msg);
    }
}

static void dump_now(const char *name)
{
    char path[512], detail[200] = "", item[80];
    screen_file want;
    snprintf(path, sizeof path, BASE "/%s/%s.screen", case_name, name);
    screen_read(path, &want);
    page.bytes[4000] = model.row; page.bytes[4001] = model.col;
    snprintf(item, sizeof item, "dump:%s", name);
    const char *spec = recorded(case_name, item);
    int inside, same = !memcmp(page.bytes, want.bytes, 4002), agrees = page_agrees(page.bytes, want.bytes, spec, &inside);
    if (!same) {
        int i;
        for (i = 0; i < 4000 && page.bytes[i] == want.bytes[i]; i++) {}
        if (i < 4000)
            snprintf(detail, sizeof detail, "first difference row %d col %d %s: got %02x want %02x",
                     i/160, (i%160)/2, i%2 ? "attr" : "char", page.bytes[i], want.bytes[i]);
        else
            snprintf(detail, sizeof detail, "cursor got %d,%d want %d,%d",
                     page.bytes[4000], page.bytes[4001], want.bytes[4000], want.bytes[4001]);
        if (getenv("REPLAY_KEEP")) {
            snprintf(path, sizeof path, "build/replay-%s-%s.screen", case_name, name);
            screen_write(path, &page);
        }
    }
    judge(item, agrees, inside, spec, detail);
    dumps_seen++; if (!same) dumps_bad++;
}
static void drain_dumps(void)
{
    while (iev < nev && ev[iev].kind == EV_DUMP) { dump_now(ev[iev].name); iev++; }
}
static int32_t next_byte(void)
{
    if (phase_setup) return setup_cmd[setup_i] ? (int32_t)(uint8_t)setup_cmd[setup_i++] : -1;
    drain_dumps();
    if (iev >= nev) return -1;
    if (ev[iev].kind == EV_RELEASE) {
        fprintf(stderr, "RED %s: a barrier release reached the keyboard, not INKEY\n", case_name);
        iev++; return 0;
    }
    return ev[iev++].byte;
}
static int32_t h_conin_char(ibase_pal_t *p) { (void)p; return next_byte(); }
static unsigned polls;
static int32_t h_conin_poll(ibase_pal_t *p)
{
    (void)p;
    if (phase_setup) return -1;
    drain_dumps();
    if (iev < nev && ev[iev].kind == EV_RELEASE) { iev++; return 'F'; }
    if (++polls > 1000000u) { fprintf(stderr, "RED %s: program polls INKEY forever\n", case_name); exit(1); }
    return -1;
}
static int h_escape(ibase_pal_t *p) { (void)p; return 0; }
/* pal_dos.c dos_terminal_line, echo mode 2 (the dot prompt). */
static int32_t h_command(ibase_pal_t *p, char *buf, uint32_t cap)
{
    uint32_t n = 0; int32_t c;
    if (phase_setup && !setup_cmd[setup_i]) {
        /* SETUP has run; the mint's RUN SCREEN left a cleared page with the
         * cursor on the last row, where `. DO ...` appears in every case. */
        phase_setup = 0; page_init(); model.row = 24; model.col = 0;
        model.scoreboard = 0;   /* every SETUP.PRG sets SCOREBOARD OFF */
        h_conout(p, ". ", 2);   /* the prompt the REPL printed before asking */
    }
    for (;;) {
        c = next_byte();
        if (c < 0) return -1;
        if (c == 0) { if (next_byte() < 0) return -1; continue; }
        if (c == '\r' || c == '\n') break;
        if (c == '\b') { if (n) { n--; h_conout(p, "\b \b", 3); } continue; }
        if (c < 32 || c > 126) continue;
        if (n + 1 < cap) { buf[n++] = (char)c; char ch = (char)c; h_conout(p, &ch, 1); }
    }
    h_conout(p, "\r", 1);
    buf[n] = 0;
    return (int32_t)n;
}

/* BIOS words from tests/screen_tool.c's KEY table, as DOS AH=07h returns them:
 * the ASCII byte, or 0 then the scan byte for an extended key. */
static const struct { const char *name; unsigned word; } keytab[] = {
    {"ENTER",0x1c0d},{"ESC",0x011b},{"DOWN",0x5000},{"UP",0x4800},{"LEFT",0x4b00},
    {"RIGHT",0x4d00},{"HOME",0x4700},{"END",0x4f00},{"PGUP",0x4900},{"PGDN",0x5100},
    {"TAB",0x0f09},{"BACKSPACE",0x0e08},{"DELETE",0x5300},{"CTRL-Y",0x1519},
    {"CTRL-END",0x7500},{"CTRL-HOME",0x7700},{"F1",0x3b00},{"F2",0x3c00},{"F10",0x4400},
    {"INSERT",0x5200},{"SHIFT-TAB",0x0f00},{"CTRL-A",0x1e01},{"CTRL-C",0x2e03},
    {"CTRL-D",0x2004},{"CTRL-E",0x1205},{"CTRL-F",0x2106},{"CTRL-G",0x2207},
    {"CTRL-Q",0x1011},{"CTRL-R",0x1312},{"CTRL-S",0x1f13},{"CTRL-V",0x2f16},
    {"CTRL-W",0x1117},{"CTRL-X",0x2d18}
};
static void add_byte(int b) { if (nev >= MAX_EV) screen_die("key stream too long"); ev[nev].kind = EV_BYTE; ev[nev++].byte = b; }
static int load_keys(const char *dir, int barriers)
{
    char path[512], line[512]; int f2 = 0;
    snprintf(path, sizeof path, BASE "/%s/KEYS", dir);
    FILE *f = screen_open(path, "rb");
    nev = iev = 0;
    while (fgets(line, sizeof line, f)) {
        size_t n = strlen(line);
        while (n && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = 0;
        if (!strncmp(line, "WAIT ", 5) || !n) continue;
        if (!strncmp(line, "TEXT ", 5)) { for (char *s = line + 5; *s; s++) add_byte((uint8_t)*s); continue; }
        if (!strncmp(line, "DUMP ", 5)) {
            if (nev >= MAX_EV) screen_die("key stream too long");
            ev[nev].kind = EV_DUMP; snprintf(ev[nev].name, sizeof ev[nev].name, "%s", line + 5); nev++;
            continue;
        }
        if (!strncmp(line, "KEY ", 4)) {
            unsigned w = 0; int found = 0;
            if (!strcmp(line + 4, "F2")) {   /* barrier release */
                if (nev >= MAX_EV) screen_die("key stream too long");
                ev[nev++].kind = EV_RELEASE; f2++; continue;
            }
            for (size_t k = 0; k < sizeof keytab / sizeof keytab[0]; k++)
                if (!strcmp(line + 4, keytab[k].name)) { w = keytab[k].word; found = 1; }
            if (!found) { fprintf(stderr, "RED %s: unknown key %s\n", dir, line + 4); return -1; }
            if (w & 0xff) add_byte((int)(w & 0xff)); else { add_byte(0); add_byte((int)(w >> 8)); }
            continue;
        }
        fprintf(stderr, "RED %s: unknown KEYS line %s\n", dir, line); return -1;
    }
    fclose(f);
    if (f2 != barriers) { fprintf(stderr, "RED %s: %d F2 keys for %d INKEY barriers\n", dir, f2, barriers); return -1; }
    return 0;
}

/* Copy SETUP unchanged, counting each INKEY barrier loop in the programs. */
static int copy_setup(const char *dir, const char *work)
{
    char path[1024], out[1024];
    int barriers = 0;
    snprintf(path, sizeof path, BASE "/%s/SETUP", dir);
    DIR *d = opendir(path);
    if (!d) return -1;
    struct dirent *e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        snprintf(path, sizeof path, BASE "/%s/SETUP/%s", dir, e->d_name);
        snprintf(out, sizeof out, "%s/%s", work, e->d_name);
        FILE *a = screen_open(path, "rb"), *b = screen_open(out, "wb");
        size_t len = strlen(e->d_name);
        int prg = len > 4 && !strcasecmp(e->d_name + len - 4, ".PRG");
        if (!prg) { int c; while ((c = fgetc(a)) != EOF) fputc(c, b); }
        else {
            char lines[400][256]; int n = 0;
            while (n < 400 && fgets(lines[n], sizeof lines[n], a)) n++;
            for (int i = 0; i < n; i++) {
                if (i + 2 < n && !strncmp(lines[i], "DO WHILE H=0", 12) &&
                    !strncmp(lines[i+1], "H=INKEY()", 9) && !strncmp(lines[i+2], "ENDDO", 5))
                    barriers++;
                fputs(lines[i], b);
            }
        }
        fclose(a); fclose(b);
    }
    closedir(d);
    return barriers;
}

static long file_bytes(const char *path, unsigned char **out)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    *out = malloc((size_t)n + 1);
    if (fread(*out, 1, (size_t)n, f) != (size_t)n) n = -1;
    fclose(f);
    return n;
}
/* Bytes real III+ fills from RAM: a DBF's header date and reserved bytes
 * and each field descriptor's address (12-15) and reserved bytes (18-31),
 * as the conformance comparator (tests/conformance/qualify.sh) normalises
 * them; each MEM entry's address (12-15); an NDX header's bytes after its
 * key expression's terminator and each node page's unused tail. Nothing
 * else is touched. */
static void normalise(const char *name, unsigned char *b, long n)
{
    size_t l = strlen(name);
    const char *ext = l > 4 ? name + l - 4 : "";
    if (!strcasecmp(ext, ".DBF") && n >= 32) {
        memset(b + 1, 0, 3); memset(b + 12, 0, 20);
        for (long o = 32; o + 32 <= n && b[o] != 0x0d; o += 32) { memset(b + o + 12, 0, 4); memset(b + o + 18, 0, 14); }
    } else if (!strcasecmp(ext, ".MEM")) {
        for (long o = 0; o + 32 <= n && b[o] != 0x1a; ) {
            long len = b[o+16] | (long)b[o+17] << 8;
            if ((b[o+11] & 0x7f) == 'N' || (b[o+11] & 0x7f) == 'D') len = 8;
            memset(b + o + 12, 0, 4);
            o += 32 + len;
        }
    } else if (!strcasecmp(ext, ".NDX") && n >= 512) {
        long z = 24, esize = b[18] | (long)b[19] << 8;
        while (z < 512 && b[z]) z++;
        if (z < 512) memset(b + z, 0, (size_t)(512 - z));
        /* Each node page: a 4-byte count, its entries and a final pointer;
         * the rest of the 512 bytes is unused. */
        for (long pg = 512; esize > 0 && pg + 512 <= n; pg += 512) {
            long cnt = b[pg] | (long)b[pg+1] << 8 | (long)b[pg+2] << 16 | (long)b[pg+3] << 24;
            long used = 4 + cnt * esize + 4;
            if (cnt >= 0 && used < 512) memset(b + pg + used, 0, (size_t)(512 - used));
        }
    }
}
static void compare_exports(const char *dir, const char *work)
{
    char path[1024], line[128];
    snprintf(path, sizeof path, BASE "/%s/EXPORTS", dir);
    FILE *f = fopen(path, "rb");
    if (!f) return;
    while (fgets(line, sizeof line, f)) {
        size_t n = strlen(line);
        while (n && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = 0;
        if (!n || line[0] == '#') continue;
        char item[160], detail[200] = "", got_path[1024], want_path[1024];
        unsigned char *got = NULL, *want = NULL;
        snprintf(item, sizeof item, "export:%s", line);
        snprintf(want_path, sizeof want_path, BASE "/%s/exports/%s.MISSING", dir, line);
        snprintf(got_path, sizeof got_path, "%s/%s", work, line);
        long gn = file_bytes(got_path, &got);
        if (gn < 0) {   /* host PALs may keep the program's spelling */
            char low[128]; for (size_t i = 0; i <= n; i++) low[i] = (char)tolower((unsigned char)line[i]);
            snprintf(got_path, sizeof got_path, "%s/%s", work, low);
            gn = file_bytes(got_path, &got);
        }
        const char *spec = recorded(dir, item);
        int inside;
        if (access(want_path, F_OK) == 0) {
            snprintf(detail, sizeof detail, "real III+ wrote no %s; product wrote %ld bytes", line, gn);
            judge(item, gn < 0, 0, NULL, detail);
            free(got); continue;
        }
        snprintf(want_path, sizeof want_path, BASE "/%s/exports/%s", dir, line);
        long wn = file_bytes(want_path, &want);
        if (wn < 0) screen_die("pinned export missing");
        if (gn >= 0) normalise(line, got, gn);
        normalise(line, want, wn);
        int agrees = file_agrees(got, gn, want, wn, spec, &inside);
        if (gn != wn || memcmp(got, want, (size_t)wn)) {
            long i = 0;
            if (gn >= 0) while (i < gn && i < wn && got[i] == want[i]) i++;
            snprintf(detail, sizeof detail, "%ld bytes vs %ld; first difference at %ld", gn, wn, i);
        }
        judge(item, agrees, inside, spec, detail);
        free(got); free(want);
    }
    fclose(f);
}


static void replay(const char *dir)
{
    char root[2048], temp[128], work[4200];
    snprintf(temp, sizeof temp, "build/replay-%s.XXXXXX", dir);
    if (!getcwd(root, sizeof root) || !mkdtemp(temp)) screen_die("scratch directory");
    snprintf(work, sizeof work, "%s/%s", root, temp);
    case_name = dir;
    int barriers = copy_setup(dir, work);
    CHECK(barriers >= 0, "case SETUP copied");
    if (barriers < 0 || load_keys(dir, barriers)) { CHECK(0, "case key stream compiled"); return; }
    int planned = 0;
    for (int i = 0; i < nev; i++) planned += ev[i].kind == EV_DUMP;
    ibase_pal_t *pal = pal_host_make((struct pal_host_cfg){92,3,2,4*1024*1024});
    struct w2_host_env env;
    struct w2_host_config cfg = {work, work, 16*1024*1024, 16*1024*1024, 'C'};
    CHECK(pal && w2_host_bind(&env, pal, &cfg) == 0, "replay PAL bound");
    pal->paint = h_paint; pal->say_screen = h_say; pal->gotoxy = h_gotoxy; pal->set_attr = h_attr;
    pal->conout = h_conout; pal->prompt = h_prompt; pal->status = h_status;
    pal->conin_char = h_conin_char; pal->conin_poll = h_conin_poll; pal->conin_escape = h_escape;
    pal->conin_command = h_command; pal->conin_line = h_command;
    page_init();
    phase_setup = 1; setup_i = 0; dumps_seen = dumps_bad = 0; polls = 0;
    xb_interp *ip = xb_interp_make(pal);
    CHECK(ip != NULL, "replay interpreter");
    if (!ip) return;
    (void)ibase_repl(pal, ip);
    char msg[160];
    snprintf(msg, sizeof msg, "%s: every DUMP reached (%d of %d)", dir, dumps_seen, planned);
    if (dumps_seen != planned) fprintf(stderr, "RED %s\n", msg);
    CHECK(dumps_seen == planned, msg);
    xb_interp_free(ip); w2_host_unbind(&env); pal_host_free(pal);
    compare_exports(dir, work);
    if (getenv("REPLAY_VERBOSE")) fprintf(stderr, "%s: %d dumps, %d differ\n", dir, dumps_seen, dumps_bad);
}

/* The comparator rejects what it must, every run: a missing export, a byte
 * or line outside a spec, a length change, a page cell outside the recorded
 * rows and a moved cursor; and it accepts exactly the recorded part
 * (initech-nic17 review controls). */
static void comparator_controls(void)
{
    unsigned char a[64], b[64]; static unsigned char p1[4002], p2[4002]; int in;
    for (int i = 0; i < 64; i++) a[i] = b[i] = (unsigned char)i;
    CHECK(!file_agrees(a, -1, b, 64, "bytes:18", &in), "control: a missing export fails a bytes spec");
    CHECK(file_agrees(a, -1, b, 64, "absent", &in) && in, "control: absent accepts a missing file");
    CHECK(!file_agrees(a, 64, b, 63, "absent", &in), "control: absent accepts nothing else");
    b[18] ^= 1;
    CHECK(file_agrees(a, 64, b, 64, "bytes:18", &in) && in, "control: the recorded byte may differ");
    b[40] ^= 1;
    CHECK(!file_agrees(a, 64, b, 64, "bytes:18", &in), "control: a byte outside the spec fails");
    CHECK(!file_agrees(a, 63, b, 64, "bytes:0-63", &in), "control: a length change fails a bytes spec");
    memcpy(a, "AB\r\nCD\r\nEF", 10); memcpy(b, "AB\r\nCX\r\nEF", 10);
    CHECK(file_agrees(a, 10, b, 10, "lines:2", &in) && in, "control: the recorded line may differ");
    CHECK(!file_agrees(a, 10, b, 10, "lines:3", &in), "control: a line outside the spec fails");
    CHECK(!file_agrees(a, 6, b, 10, "lines:2", &in), "control: a missing line fails");
    memset(p1, 7, sizeof p1); memset(p2, 7, sizeof p2);
    p2[160*20] = 'X';
    CHECK(page_agrees(p1, p2, "rows:20-24", &in) && in, "control: a cell in the recorded rows may differ");
    p2[3999] ^= 1;
    CHECK(page_agrees(p1, p2, "rows:20-24", &in), "control: row 24 is inside rows:20-24");
    p2[5] ^= 1;
    CHECK(!page_agrees(p1, p2, "rows:20-24", &in), "control: a cell outside the rows fails");
    p2[5] ^= 1; p2[4000] = 3;
    CHECK(!page_agrees(p1, p2, "rows:20-24", &in), "control: a moved cursor fails without cursor");
    CHECK(page_agrees(p1, p2, "rows:20-24+cursor", &in), "control: cursor exempts the cursor");
}

int main(void)
{
    FILE *f = screen_open("tests/forms_replay.cases", "rb");
    char line[512], cases[96][64], elsewhere[16][64]; int ncases = 0, nelse = 0;
    const char *only = getenv("REPLAY_CASE");
    if (only && !*only) only = NULL;
    while (fgets(line, sizeof line, f)) {
        char a[64], b[64], c[96];
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "case %63s", a) == 1) { if (ncases < 96) snprintf(cases[ncases++], 64, "%s", a); continue; }
        char sp[200];
        if (sscanf(line, "differs %63s %95s %199s", b, c, sp) == 3 && ndiffers < MAX_DIFFERS) {
            if (strncmp(sp, "rows:", 5) && strncmp(sp, "cursor", 6) && strncmp(sp, "bytes:", 6) &&
                strncmp(sp, "lines:", 6) && strncmp(sp, "absent", 6)) screen_die("differs line without a spec");
            snprintf(differs[ndiffers].dir, sizeof differs[0].dir, "%s", b);
            snprintf(differs[ndiffers].item, sizeof differs[0].item, "%s", c);
            snprintf(differs[ndiffers].spec, sizeof differs[0].spec, "%s", sp);
            ndiffers++; continue;
        }
        if (sscanf(line, "elsewhere %63s", a) == 1) { if (nelse < 16) snprintf(elsewhere[nelse++], 64, "%s", a); continue; }
        screen_die("bad forms_replay.cases line");
    }
    fclose(f);
    /* Every pinned forms case is replayed: none is silently left out. */
    DIR *d = opendir(BASE);
    struct dirent *e; int pinned = 0;
    while (d && (e = readdir(d))) {
        if (strncmp(e->d_name, "forms-", 6)) continue;
        int listed = 0;
        for (int i = 0; i < ncases; i++) listed |= !strcmp(cases[i], e->d_name);
        for (int i = 0; i < nelse; i++) listed |= !strcmp(elsewhere[i], e->d_name);
        if (!listed) fprintf(stderr, "RED %s is pinned but not replayed\n", e->d_name);
        CHECK(listed, "every pinned forms case is listed for replay");
        pinned++;
    }
    if (d) closedir(d);
    CHECK(pinned == ncases + nelse, "replay list names only pinned cases");
    comparator_controls();
    for (int i = 0; i < ncases; i++) if (!only || !strcmp(only, cases[i])) replay(cases[i]);
    if (!only) for (int i = 0; i < ndiffers; i++) {
        if (!differs[i].seen) fprintf(stderr, "RED differs %s %s names no replayed item\n", differs[i].dir, differs[i].item);
        CHECK(differs[i].seen, "each recorded disagreement names a replayed item");
    }
    return TEST_SUMMARY("test-ibase-forms-replay");
}
