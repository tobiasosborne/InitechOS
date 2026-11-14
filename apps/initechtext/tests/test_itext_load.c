/* Load completion/conservation, initech-gxb4z. Authorities: GOLDEN-MASTER
 * item 4, the operator's phase-2 original-name ruling, and the documented
 * TEXT capacity/line-end contract (README). T01..T05 are independent authored
 * bytes from the desktop file-reader study, commit 4fb96275.
 *
 * Real app and TextEdit, recording SDK UI, disk-backed file bindings. Only
 * the read-side adapter injects faults. The independent stdio reader below
 * opens the actual pathname and compares against input/literal expectations;
 * it never uses doc_load, TextEdit or the adapter's short-read policy.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tbx_host.h"

static unsigned limit, read_bytes, seek_calls, opened, closed;
enum { NORMAL, OPEN_ERROR, SEEK_END_ERROR, SEEK_START_ERROR, READ_ERROR,
       PREFIX_ERROR, PREFIX_EOF, CLOSE_ERROR, SEEK_START_WRONG, READ_OVERRUN,
       GROW, EMPTY_GROW, SHRINK, TAIL_ERROR, TAIL_OVERRUN };
static int fault, source_changed;
static const char growth_suffix[] = "-GROWN";

/* Review reproducer size-change.c: a separate host descriptor appends six
 * literal bytes after the first read, or during an empty file's rewind.
 * No scheduling race or product-derived expected bytes. */
static void mutate_source_file(void)
{
    if (fault == SHRINK) {
        if (truncate("CASE.TXT", 4)) exit(2);
    } else {
        int h = open("CASE.TXT", O_WRONLY | O_APPEND);
        if (h < 0 || write(h, growth_suffix, sizeof growth_suffix - 1) !=
            (ssize_t)sizeof growth_suffix - 1 || close(h)) exit(2);
    }
    source_changed = 1;
}

static int32_t disk_open(const char *path, uint32_t mode)
{
    int h;
    if (fault == OPEN_ERROR) return TBX_ERR_DOS(5);
    h = open(path, mode == TBX_FILE_READ_ONLY ? O_RDONLY : O_RDWR);
    if (h < 0) return TBX_ERR_DOS(errno == ENOENT ? 2 : 5);
    opened++;
    return h;
}
static int32_t disk_create_new(const char *path)
{
    int h = open(path, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (h < 0) return TBX_ERR_DOS(errno == EEXIST ? 80 : 5);
    opened++;
    return h;
}
static int32_t disk_read(int32_t h, void *buf, uint32_t n)
{
    ssize_t got;
    if (fault == READ_ERROR || (fault == PREFIX_ERROR && read_bytes >= 4))
        return TBX_ERR_DOS(5);
    if (fault == TAIL_ERROR && read_bytes >= 20) return TBX_ERR_DOS(5);
    if (fault == TAIL_OVERRUN && read_bytes >= 20) return (int32_t)n + 1;
    if (fault == PREFIX_EOF && read_bytes >= 4) return 0;
    if (fault == READ_OVERRUN) return (int32_t)n + 1;
    if (limit && n > limit) n = limit;
    got = read(h, buf, n);
    if (got < 0) return TBX_ERR_DOS(5);
    read_bytes += (unsigned)got;
    if ((fault == GROW || fault == SHRINK) && !source_changed) mutate_source_file();
    return (int32_t)got;
}
static int32_t disk_write(int32_t h, const void *buf, uint32_t n)
{
    ssize_t done = write(h, buf, n);
    return done < 0 ? TBX_ERR_DOS(5) : (int32_t)done;
}
static int32_t disk_seek(int32_t h, int32_t delta, uint32_t origin)
{
    off_t pos;
    seek_calls++;
    if ((fault == SEEK_END_ERROR && seek_calls == 1) ||
        (fault == SEEK_START_ERROR && seek_calls == 2)) return TBX_ERR_DOS(5);
    pos = lseek(h, delta, origin == TBX_FILE_FROM_END ? SEEK_END : SEEK_SET);
    if (fault == EMPTY_GROW && seek_calls == 2 && !source_changed) mutate_source_file();
    if (fault == SEEK_START_WRONG && seek_calls == 2) return 1;
    return pos < 0 ? TBX_ERR_DOS(5) : (int32_t)pos;
}
static int32_t disk_close(int32_t h)
{
    int rc = close(h);
    closed++;
    return rc || fault == CLOSE_ERROR ? TBX_ERR_DOS(5) : TBX_OK;
}
static int32_t disk_delete(const char *path)
{
    if (unlink(path) == 0) return TBX_OK;
    return TBX_ERR_DOS(errno == ENOENT ? 2 : 5);
}
static int32_t disk_rename(const char *from, const char *to)
{
    if (access(to, F_OK) == 0) return TBX_ERR_DOS(5);
    return rename(from, to) == 0 ? TBX_OK : TBX_ERR_DOS(5);
}
#define tbx_file_open disk_open
#define tbx_file_create_new disk_create_new
#define tbx_file_read disk_read
#define tbx_file_write disk_write
#define tbx_file_seek disk_seek
#define tbx_file_close disk_close
#define tbx_file_delete disk_delete
#define tbx_file_rename disk_rename
#include "../itext.c"

static unsigned checks, failures;
static const char *scenario;
#define CHECK(c, why) do { checks++; if (!(c)) { failures++; \
    printf("FAIL %s: %s (line %d)\n", scenario, why, __LINE__); } } while (0)

static void plant(const char *path, const void *data, size_t n)
{
    FILE *f = fopen(path, "wb");
    if (!f || fwrite(data, 1, n, f) != n || fclose(f)) exit(2);
}
static int disk_is(const char *path, const void *expected, size_t n)
{
    unsigned char data[8192];
    FILE *f = fopen(path, "rb");
    size_t got;
    int good;
    if (!f) return 0;
    got = fread(data, 1, sizeof data, f);
    good = !ferror(f) && feof(f) && got == n && !memcmp(data, expected, n);
    return fclose(f) == 0 && good;
}
static void event(int what, int32_t message)
{
    tbx_event_t e = { .what = what, .message = message };
    tbx_host.nops = 0;
    if (!tbx_host_push_event(&e)) exit(2);
    (void)tbx_host_dispatch(tbx_app_event);
}
static void open_named(const char *path)
{
    event(TBX_EVT_MENU, TBX_MENU_WORD(129, 2));
    snprintf(tbx_host.sf_reply, TBX_SF_REPLY_BYTES, "%s", path);
    event(TBX_EVT_SFREPLY, 1);
}
static void configure(unsigned cap, int error)
{
    limit = cap; fault = error;
    read_bytes = seek_calls = opened = closed = 0;
    source_changed = 0;
}
static void launch(void)
{
    configure(0, NORMAL);
    tbx_host_reset();
    memset(&g_te, 0, sizeof g_te);
    g_win = 0; g_dirty = 0; g_active = 1; g_scroll = 0; g_anchor = 0;
    g_scrap_has = 0; g_after = 0; g_sf_kind = 0; g_path[0] = 0;
    g_undo_ok = 0; g_undo_run = 0;
    CHECK(tbx_app_main() == 0, "launch");
}
static void success(const char *name, const unsigned char *input, size_t n,
                    const unsigned char *expected, size_t want, unsigned cap)
{
    scenario = name;
    plant("CASE.TXT", input, n);
    launch(); configure(cap, NORMAL);
    open_named("CASE.TXT");
    CHECK(tbx_host.alerts == 0, "complete file opens without an alert");
    CHECK(g_te.teLength == (int)want && !memcmp(g_te.text, expected, want),
          "all expected text is loaded");
    CHECK(!strcmp(g_path, "CASE.TXT") && !g_dirty, "complete document is published");
    CHECK(opened == closed, "load closes its handle");
    configure(0, NORMAL);
    event(TBX_EVT_MENU, TBX_MENU_WORD(129, 5));
    CHECK(disk_is("CASE.TXT", expected, want), "stdio reads exact saved bytes by original name");
    open_named("CASE.TXT");
    CHECK(g_te.teLength == (int)want && !memcmp(g_te.text, expected, want),
          "saved file reopens completely");
    printf("checked %s\n", name);
}
static void refusal(const char *name, const unsigned char *input, size_t n,
                    unsigned cap, int error)
{
    static const char prior[] = "prior\rwork", edited[] = "Zprior\rwork";
    FlairTE before;
    int dirty, undo_len, undo_ok, undo_dirty, scroll;
    char undo[4096];
    unsigned char changed[8192];
    const unsigned char *expected = input;
    size_t disk_n = n;
    if (error == GROW || error == EMPTY_GROW) {
        if (n + sizeof growth_suffix - 1 > sizeof changed) exit(2);
        memcpy(changed, input, n);
        memcpy(changed + n, growth_suffix, sizeof growth_suffix - 1);
        expected = changed; disk_n += sizeof growth_suffix - 1;
    } else if (error == SHRINK) {
        disk_n = 4;
    }
    scenario = name;
    plant("CASE.TXT", input, n); plant("PRIOR.TXT", prior, sizeof prior - 1);
    launch(); open_named("PRIOR.TXT");
    event(keyDown, 'Z');
    before = g_te; dirty = g_dirty; scroll = g_scroll;
    undo_len = g_undo_len; undo_ok = g_undo_ok; undo_dirty = g_undo_dirty;
    memcpy(undo, g_undo, sizeof undo);
    /* Standard File is normally reached after the unsaved-work guard.
     * Answer Don't Save, then fail the requested load: existing work remains. */
    event(TBX_EVT_MENU, TBX_MENU_WORD(129, 2));
    event(TBX_EVT_ALERT, TBX_ALERT_OTHER);
    snprintf(tbx_host.sf_reply, TBX_SF_REPLY_BYTES, "%s", "CASE.TXT");
    configure(cap, error);
    event(TBX_EVT_SFREPLY, 1);
    CHECK(tbx_host.alerts == 2 && !strcmp(tbx_host.alert_btn[0], "OK"),
          "load refusal uses an OK alert");
    CHECK(strstr(tbx_host.alert_text, "could not be opened") ||
          strstr(tbx_host.alert_text, "too large"), "load refusal explains the error");
    CHECK(!memcmp(&g_te, &before, sizeof before) && g_dirty == dirty && g_scroll == scroll,
          "prior text, selection, dirty state and scroll survive");
    CHECK(!strcmp(g_path, "PRIOR.TXT") && !strcmp(tbx_host.windows[0].title, "PRIOR.TXT"),
          "prior document keeps its own name");
    CHECK(undo_len == g_undo_len && undo_ok == g_undo_ok && undo_dirty == g_undo_dirty &&
          !memcmp(undo, g_undo, sizeof undo), "prior undo survives");
    CHECK(opened == closed, "failed load closes any acquired handle");
    CHECK(disk_is("CASE.TXT", expected, disk_n) && disk_is("PRIOR.TXT", prior, sizeof prior - 1),
          "independent reader opens both original files after the reported failure");
    configure(0, NORMAL);
    event(TBX_EVT_ALERT, TBX_ALERT_DEFAULT);
    event(TBX_EVT_MENU, TBX_MENU_WORD(129, 5));
    CHECK(disk_is("CASE.TXT", expected, disk_n), "Save after failed Open never overwrites the refused file");
    CHECK(disk_is("PRIOR.TXT", edited, sizeof edited - 1), "Save retains the previous document's name");
    open_named("PRIOR.TXT");
    CHECK(g_te.teLength == (int)sizeof edited - 1 &&
          !memcmp(g_te.text, edited, sizeof edited - 1), "previous file still reopens under its own name");
    printf("checked %s\n", name);
}
int main(int argc, char **argv)
{
    unsigned char fixture[5][8192];
    size_t sizes[5];
    char work[] = "/tmp/initechtext-load-XXXXXX";
    static const unsigned char normalized[] = "alpha\rbravo\r\rcharlie\r";
    unsigned char growing[4096];
    if (argc != 2) return 2;
    for (int i = 0; i < 5; i++) {
        char path[1024]; FILE *f;
        snprintf(path, sizeof path, "%s/T%02d.TXT", argv[1], i + 1);
        f = fopen(path, "rb"); if (!f) return 2;
        sizes[i] = fread(fixture[i], 1, sizeof fixture[i], f);
        if (ferror(f) || !feof(f) || fclose(f)) return 2;
    }
    if (!mkdtemp(work) || chdir(work)) return 2;
    success("T04 normal control", fixture[3], sizes[3], fixture[3], sizes[3], 0);
    success("T04 four-byte reads", fixture[3], sizes[3], fixture[3], sizes[3], 4);
    success("T04 one-byte reads", fixture[3], sizes[3], fixture[3], sizes[3], 1);
    success("T04 three-byte reads and final remainder", fixture[3], sizes[3], fixture[3], sizes[3], 3);
    success("T01 CRLF across reads", fixture[0], sizes[0], normalized, sizeof normalized - 1, 1);
    success("T02 full capacity", fixture[1], sizes[1], fixture[1], sizes[1], 4);
    success("empty file", fixture[3], 0, fixture[3], 0, 4);
    refusal("open error", fixture[3], sizes[3], 4, OPEN_ERROR);
    refusal("seek-end error", fixture[3], sizes[3], 4, SEEK_END_ERROR);
    refusal("rewind error", fixture[3], sizes[3], 4, SEEK_START_ERROR);
    refusal("empty file rewind error", fixture[3], 0, 4, SEEK_START_ERROR);
    refusal("empty file wrong rewind position", fixture[3], 0, 4, SEEK_START_WRONG);
    refusal("initial read error", fixture[3], sizes[3], 4, READ_ERROR);
    refusal("read error after prefix", fixture[3], sizes[3], 4, PREFIX_ERROR);
    refusal("early EOF after prefix", fixture[3], sizes[3], 4, PREFIX_EOF);
    refusal("invalid read count", fixture[3], sizes[3], 4, READ_OVERRUN);
    refusal("EOF probe read error", fixture[3], sizes[3], 4, TAIL_ERROR);
    refusal("EOF probe invalid count", fixture[3], sizes[3], 4, TAIL_OVERRUN);
    refusal("empty file EOF probe error", fixture[3], 0, 4, READ_ERROR);
    refusal("close error", fixture[3], sizes[3], 4, CLOSE_ERROR);
    refusal("T03 oversize with short reads", fixture[2], sizes[2], 4, NORMAL);
    refusal("T05 oversize normalized input", fixture[4], sizes[4], 4, NORMAL);
    memset(growing, 'A', sizeof growing);
    refusal("grow after first read", growing, 20, 4, GROW);
    refusal("empty grows during rewind", growing, 0, 4, EMPTY_GROW);
    refusal("capacity grows after first read", growing, sizeof growing, 4, GROW);
    refusal("shrink after first read", growing, 20, 4, SHRINK);
    /* TE also limits line count: a byte-sized input can still be too large. */
    memset(fixture[0], '\r', 2048);
    refusal("TextEdit line capacity", fixture[0], 2048, 4, NORMAL);
    unlink("CASE.TXT"); unlink("PRIOR.TXT");
    if (chdir("/") || rmdir(work)) return 2;
    printf("test_itext_load: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
