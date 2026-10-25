/* NDX PM review: callback-only metadata, PAL drive, name provenance.
 * GOLD.OUT pins INDEX ON ... TO T1 -> C:T1.ndx. Explicit extension
 * preservation and equivalent SET INDEX/USE defaulting are inferred rules,
 * tested here independently of the period golden pending a follow-up mint. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_assert.h"
#include "samir/interp.h"

TEST_HARNESS();
struct pal_host_cfg { uint8_t yy, mm, dd; uint32_t heap; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(samir_pal_t *);
static unsigned line, snap;
static const char *const commands[] = {
    "USE BASE", "SET SAFETY OFF", "INDEX ON NAME TO bare", "SNAP",
    "CLOSE INDEX", "SET INDEX TO BARE", "SNAP", "USE BASE INDEX BARE", "SNAP",
    "CLOSE INDEX", "SET INDEX TO BARE.NDX", "SNAP", "USE BASE INDEX BARE", "SNAP",
    "CLOSE INDEX", "INDEX ON NAME TO explicit.nDx", "SNAP",
    "CLOSE INDEX", "SET INDEX TO EXPLICIT.nDx", "SNAP",
    "USE BASE INDEX EXPLICIT.nDx", "SNAP", "DRIVED", "SNAP", "DRIVENONE", "SNAP",
    "USE BASE", "INDEX ON NAME TO upper.NDX", "SNAP",
    "USE BASE", "INDEX ON NAME TO other.IDX", "SNAP", "QUIT"
};
static const char *const expected[] = {
    "C:BARE.ndx", "C:BARE.ndx", "C:BARE.ndx", "C:BARE.NDX", "C:BARE.ndx",
    "C:EXPLICIT.nDx", "C:EXPLICIT.nDx", "C:EXPLICIT.nDx",
    "D:EXPLICIT.nDx", "EXPLICIT.nDx", "UPPER.NDX", "OTHER.IDX"
};
static int drive_d(samir_pal_t *p) { (void)p; return 'D'; }
static int no_drive(samir_pal_t *p) { (void)p; return -1; }
static void output(samir_pal_t *p, const char *s, uint32_t n)
{ (void)p; (void)s; (void)n; }
static int32_t input(samir_pal_t *p, char *s, uint32_t cap)
{
    const char *v = line < sizeof commands / sizeof commands[0] ? commands[line++] : "QUIT";
    (void)p;
    snprintf(s, cap, "%s", v);
    return (int32_t)strlen(s);
}
static int check_name(void *u, xb_interp *ip, const char *verb, const char *args, int *ec)
{
    xb_val v;
    (void)u; (void)args;
    if (!strcmp(verb, "DRIVED")) { xb_interp_pal(ip)->curdrive = drive_d; return CMD_OK; }
    if (!strcmp(verb, "DRIVENONE")) { xb_interp_pal(ip)->curdrive = no_drive; return CMD_OK; }
    if (strcmp(verb, "SNAP")) return CMD_UNKNOWN;
    CHECK(snap < sizeof expected / sizeof expected[0], "snapshot within fixed expectations");
    CHECK(xb_interp_eval(ip, "NDX(1)", &v, ec) == INTERP_OK, "NDX evaluated through cursor callback");
    if (snap < sizeof expected / sizeof expected[0])
        CHECK(v.t == XB_C && v.u.c.len == strlen(expected[snap]) &&
              !memcmp(v.u.c.p, expected[snap], strlen(expected[snap])),
              "default extension and explicit spelling follow stored provenance and PAL drive");
    snap++;
    CHECK(xb_interp_eval(ip, "NDX(2)", &v, ec) == INTERP_OK &&
          v.t == XB_C && v.u.c.len == 0, "unused cursor slot returns empty");
    return CMD_OK;
}
static int fake_index(void *u, int slot, char *buf, uint32_t cap)
{
    size_t n = strlen(u);
    if (slot != 1 || n >= cap) return -1;
    memcpy(buf, u, n + 1);
    return (int)n;
}
int main(void)
{
    char cwd[2048], dir[] = "build/ndx-names-XXXXXX", path[256], scratch[512];
    char bytes[4096];
    size_t n;
    FILE *src, *dst;
    samir_pal_t *pal;
    xb_interp *ip;
    CHECK(getcwd(cwd, sizeof cwd) != NULL && mkdtemp(dir) != NULL, "fresh factory working directory");
    snprintf(path, sizeof path, "%s/BASE.DBF", dir);
    src = fopen("build/CLIENTS.DBF", "rb"); dst = fopen(path, "wb");
    CHECK(src && dst, "independent database fixture opened");
    if (!src || !dst) return TEST_SUMMARY("test-samir-ndx-names");
    while ((n = fread(bytes, 1, sizeof bytes, src)) != 0)
        CHECK(fwrite(bytes, 1, n, dst) == n, "fixture copied");
    fclose(src); fclose(dst);
    pal = pal_host_make((struct pal_host_cfg){86, 7, 24, 2*1024*1024});
    CHECK(pal && pal->curdrive && pal->curdrive(pal) == 'C', "host PAL pins a deterministic C drive");
    ip = pal ? xb_interp_make(pal) : NULL;
    CHECK(ip != NULL, "interpreter constructed");
    if (!ip) return TEST_SUMMARY("test-samir-ndx-names");
    CHECK(xb_interp_ctx(ip)->file_pal == pal, "interpreter binds its PAL");
    CHECK(xb_interp_add_cmd_hook(ip, check_name, NULL) == INTERP_OK, "name assertions registered");
    pal->conout = output; pal->conin_line = input;
    CHECK(chdir(dir) == 0, "entered isolated fixture directory");
    CHECK(samir_repl(pal, ip) == INTERP_OK && snap == sizeof expected / sizeof expected[0],
          "all real INDEX ON, SET INDEX and USE INDEX snapshots executed");
    CHECK(chdir(cwd) == 0, "restored working directory");
    {
        xb_dbcursor cursor = {.index_name = fake_index};
        xb_ctx ctx = {0};
        xb_val arg = xb_n(1), value;
        int ec;
        ctx.dbcur = &cursor; ctx.dbcur_user = "Q:KEEP.NDX"; ctx.file_pal = pal;
        ctx.scratch = scratch; ctx.scratch_cap = sizeof scratch;
        pal->curdrive = drive_d;
        CHECK(xb_call_builtin("NDX", 3, &arg, 1, &ctx, &value, &ec) == 0 &&
              value.t == XB_C && value.u.c.len == 10 && !memcmp(value.u.c.p, "Q:KEEP.NDX", 10),
              "explicit drive qualifier is preserved without rewriting extension");
    }
    xb_interp_free(ip); pal_host_free(pal);
    return TEST_SUMMARY("test-samir-ndx-names");
}
