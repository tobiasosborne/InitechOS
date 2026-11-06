/* Factory replay for the real field kit + editor + mutation engine.
 * Run host.sh from the repository root; emulator KEYS remain the acceptance. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "samir/interp.h"
#include "samir/dbf.h"
#include "samir/workarea.h"
#include "samir/nav.h"
#include "samir/rt.h"
#include "ui_apps.h"

struct pal_host_cfg { uint8_t yy, mm, dd; uint32_t heap; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(samir_pal_t *);
extern int mutate_register(xb_interp *);
extern int set_register(xb_interp *);
extern int query_register(xb_interp *);
static const unsigned char *keys;
static int eof_reads;
static int32_t input(samir_pal_t *p)
{ (void)p; if (*keys) return *keys++; ++eof_reads; return -1; }
static int poll_escape(samir_pal_t *p) { (void)p; return 0; }
static void quiet(samir_pal_t *p, const char *s, uint32_t n) { (void)p; (void)s; (void)n; }
static void need(int ok, const char *message)
{ if (!ok) { fprintf(stderr, "FAIL: %s\n", message); exit(1); } }
static void fixture(const char *out)
{
    char b[4096]; size_t n;
    FILE *src = fopen("build/CLIENTS.DBF", "rb"), *dst = fopen(out, "wb");
    need(src && dst, "fixture opens");
    while ((n = fread(b, 1, sizeof b, src)) != 0) need(fwrite(b, 1, n, dst) == n, "fixture copy");
    fclose(src); fclose(dst);
}
static void edit(xb_interp *ip, int mode, const char *args, const char *script)
{
    int ec = 0, rc;
    keys = (const unsigned char *)script; eof_reads = 0;
    rc = ui_edit(ip, mode, args, &ec);
    if (rc) fprintf(stderr, "editor rc=%d ec=%d\n", rc, ec);
    need(rc == 0, "editor succeeds"); need(!*keys, "all replay keys consumed");
    need(eof_reads == 0, "editor exits on the requested key without exhausting input");
}
static void open_table(xb_interp *ip, const char *path)
{
    wa_env *env = xb_interp_env(ip);
    (void)wa_close(env, 1);
    fixture(path);
    need(wa_set_open_rw(env, 1, path, "CLIENTS", NULL) == 0, "fixture USE");
}
static void city(xb_interp *ip, unsigned recno, const char *want, int deleted)
{
    xb_val values[3]; int del;
    need(dbf_read_rec(wa_table(xb_interp_env(ip), 1), recno, values, &del) == 0, "read record");
    need(!memcmp(values[1].u.c.p, want, strlen(want)), "CITY matches");
    need(del == deleted, "delete flag matches");
}
int main(void)
{
    samir_pal_t *pal = pal_host_make((struct pal_host_cfg){86, 7, 24, 1024*1024});
    xb_interp *ip; int ec = 0;
    need(pal != NULL, "host PAL");
    pal->conin_char = input; pal->conin_escape = poll_escape; pal->conout = quiet;
    ip = xb_interp_make(pal); need(ip != NULL, "interpreter");
    need(mutate_register(ip) == 0 && set_register(ip) == 0 && query_register(ip) == 0, "engine modules");
    need(ui_edit(ip, UI_EDIT_EDIT, "", &ec) < 0 && ec == 52, "no database error");
    (void)mkdir("build/oneshot/edit", 0775);
    (void)mkdir("build/oneshot/edit/host", 0775);
    open_table(ip, "build/oneshot/edit/host/append.dbf");
    edit(ip, UI_EDIT_APPEND, "", "\r");
    need(wa_nrec(xb_interp_env(ip), 1) == 3, "untouched append stays empty");
    edit(ip, UI_EDIT_APPEND, "", "aborted\033");
    need(wa_nrec(xb_interp_env(ip), 1) == 3, "Esc cancels append");
    edit(ip, UI_EDIT_APPEND, "", "smythe\rberlin\r125.50\027");
    need(wa_nrec(xb_interp_env(ip), 1) == 4, "append count"); city(ip, 4, "berlin", 0);
    open_table(ip, "build/oneshot/edit/host/edit.dbf");
    {
        xb_val sentinel = xb_c("retained", 8);
        need(xb_interp_store(ip, "ZED000", &sentinel) == 0, "seed caller variable");
    }
    edit(ip, UI_EDIT_EDIT, "2 FIELDS CITY", "\031discard\033"); city(ip, 2, "AUSTIN", 0);
    edit(ip, UI_EDIT_EDIT, "2", "\340\120\031berlin\340\165"); city(ip, 2, "berlin", 0);
    edit(ip, UI_EDIT_EDIT, "2", "\025\027"); city(ip, 2, "berlin", 1);
    edit(ip, UI_EDIT_EDIT, "2", "\025\033"); city(ip, 2, "berlin", 1);
    edit(ip, UI_EDIT_EDIT, "2", "\025\027"); city(ip, 2, "berlin", 0);
    {
        xb_val sentinel;
        need(xb_interp_get_memvar(ip, "ZED000", &sentinel) == 0 && sentinel.t == XB_C &&
             sentinel.u.c.len == 8 && !memcmp(sentinel.u.c.p, "retained", 8), "caller variable survives private editor scratch");
    }
    open_table(ip, "build/oneshot/edit/host/insert.dbf");
    need(wa_nav_goto(xb_interp_env(ip), 1, 1) == 0, "position before INSERT");
    edit(ip, UI_EDIT_INSERT, "", "inserted\rberlin\r20\027");
    need(wa_nrec(xb_interp_env(ip), 1) == 4, "insert count");
    city(ip, 2, "berlin", 0); city(ip, 3, "AUSTIN", 0); city(ip, 4, "DALLAS", 0);
    open_table(ip, "build/oneshot/edit/host/paging.dbf");
    edit(ip, UI_EDIT_EDIT, "FIELDS CITY", "\031first\003\031discard\033");
    city(ip, 1, "first", 0); city(ip, 2, "AUSTIN", 0);
    need(wa_nav_goto(xb_interp_env(ip), 1, 1) == 0, "position NEXT scope");
    edit(ip, UI_EDIT_EDIT, "NEXT 2 FIELDS CITY", "\031one\003\031two\003\027");
    need(wa_recno(xb_interp_env(ip), 1) == 2, "NEXT limits paging");
    city(ip, 1, "one", 0); city(ip, 2, "two", 0); city(ip, 3, "DALLAS", 0);
    edit(ip, UI_EDIT_EDIT, "REST FIELDS CITY", "\022\027");
    need(wa_recno(xb_interp_env(ip), 1) == 2, "REST limits backward paging");
    open_table(ip, "build/oneshot/edit/host/indexed.dbf");
    (void)remove("build/oneshot/edit/host/CITY.NDX");
    (void)remove("build/oneshot/edit/host/CITY.ndx");
    need(samir_do(ip, "INDEX ON CITY TO build/oneshot/edit/host/CITY.NDX") == 0, "create CITY index");
    edit(ip, UI_EDIT_EDIT, "2 FIELDS CITY", "\031berlin\027");
    need(samir_do(ip, "SEEK \"berlin\"") == 0 && wa_found(xb_interp_env(ip), 1) &&
         wa_recno(xb_interp_env(ip), 1) == 2, "EDIT maintains index key");
    edit(ip, UI_EDIT_APPEND, "", "newindex\rzurich\r1\027");
    need(samir_do(ip, "SEEK \"zurich\"") == 0 && wa_found(xb_interp_env(ip), 1) &&
         wa_recno(xb_interp_env(ip), 1) == 4, "APPEND maintains index key");
    need(wa_nav_go_top(xb_interp_env(ip), 1) == 0, "indexed scope starts at top");
    edit(ip, UI_EDIT_EDIT, "NEXT 2 FIELDS BAL", "\003\003\027");
    need(wa_recno(xb_interp_env(ip), 1) == 1, "NEXT counts records in index order");
    need(ui_edit(ip, UI_EDIT_INSERT, "", &ec) < 0 && ec == 147, "indexed INSERT refused before mutation");
    need(wa_nrec(xb_interp_env(ip), 1) == 4, "indexed INSERT leaves records unchanged");
    {
        dbf_table *table;
        const dbf_field_spec fields[] = {{"STAMP", 'D', 8, 0}, {"FLAG", 'L', 1, 0}, {"LONG", 'C', 100, 0}};
        xb_val values[3]; int del;
        char replay[128];
        (void)wa_close(xb_interp_env(ip), 1);
        need(dbf_create(pal, "build/oneshot/edit/host/date.dbf", fields, 3, &table) == 0, "date fixture");
        need(dbf_append_blank_commit(table) == 0, "date blank record");
        need(wa_adopt_table(xb_interp_env(ip), 1, table, NULL, "DATES", "DATES.DBF", NULL, 0) == 0, "adopt date table");
        edit(ip, UI_EDIT_APPEND, "", "\r");
        need(dbf_nrec(table) == 1, "blank date mask does not keep APPEND open");
        memcpy(replay, "023186 \031022886y", 15);
        memset(replay + 15, 'x', 100); replay[115] = 23; replay[116] = 0;
        edit(ip, UI_EDIT_EDIT, "1", replay);
        need(dbf_read_rec(table, 1, values, &del) == 0, "date readback");
        need(values[0].t == XB_D && values[0].u.d == jdn_from_ymd(1986, 2, 28), "invalid date rejected and valid date saved");
        need(values[1].t == XB_L && values[1].u.l, "logical input normalized");
        need(values[2].u.c.len == 100 && values[2].u.c.p[99] == 'x', "long field scrolls and saves tail");
    }
    (void)wa_close_all(xb_interp_env(ip)); xb_interp_free(ip); pal_host_free(pal);
    puts("PASS: EDIT/APPEND/INSERT; scopes, FIELDS, Esc, paging, delete/recall, NDX maintenance, date/logical validation and long fields");
    return 0;
}
