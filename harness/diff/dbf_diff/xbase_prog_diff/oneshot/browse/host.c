/* Factory replay using the real BROWSE, field kit and mutation/index engine. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "samir/interp.h"
#include "samir/workarea.h"
#include "samir/nav.h"
#include "samir/dbf.h"
#include "ui_apps.h"

struct pal_host_cfg { uint8_t yy, mm, dd; uint32_t heap; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(samir_pal_t *);
extern int mutate_register(xb_interp *);
extern int set_register(xb_interp *);
extern int query_register(xb_interp *);
static const unsigned char *keys;
static int32_t input(samir_pal_t *p) { (void)p; return *keys ? *keys++ : -1; }
static int escape(samir_pal_t *p) { (void)p; return 0; }
static void quiet(samir_pal_t *p, const char *s, uint32_t n) { (void)p; (void)s; (void)n; }
static void need(int condition, const char *message)
{ if (!condition) { fprintf(stderr, "FAIL: %s\n", message); exit(1); } }
static void fixture(const char *out)
{
    char bytes[4096]; size_t n;
    FILE *src = fopen("build/CLIENTS.DBF", "rb"), *dst = fopen(out, "wb");
    need(src && dst, "fixture opens");
    while ((n = fread(bytes, 1, sizeof(bytes), src)) != 0)
        need(fwrite(bytes, 1, n, dst) == n, "fixture copy");
    fclose(src); fclose(dst);
}
static void browse(xb_interp *ip, const char *args, const char *script)
{
    int ec = 0, rc;
    keys = (const unsigned char *)script;
    rc = ui_browse(ip, args, &ec);
    if (rc) fprintf(stderr, "BROWSE rc=%d ec=%d\n", rc, ec);
    need(rc == 0, "BROWSE returns success");
    need(!*keys, "all replay keys consumed");
}
static void city(xb_interp *ip, unsigned rec, const char *text, int deleted)
{
    xb_val values[3]; int del;
    need(dbf_read_rec(wa_table(xb_interp_env(ip), 1), rec, values, &del) == 0, "read record");
    need(!memcmp(values[1].u.c.p, text, strlen(text)), "CITY value");
    need(del == deleted, "delete mark");
}
int main(void)
{
    samir_pal_t *pal = pal_host_make((struct pal_host_cfg){86, 7, 24, 1024*1024});
    xb_interp *ip; wa_env *env; int ec = 0;
    need(pal != NULL, "host PAL");
    pal->conin_char = input; pal->conin_escape = escape; pal->conout = quiet;
    ip = xb_interp_make(pal); need(ip != NULL, "interpreter");
    env = xb_interp_env(ip);
    need(mutate_register(ip) == 0 && set_register(ip) == 0 && query_register(ip) == 0, "engine modules");
    need(ui_browse(ip, "", &ec) < 0 && ec == 52, "no database catalog error");
    (void)mkdir("build/oneshot/browse", 0775);
    (void)mkdir("build/oneshot/browse/host", 0775);
    fixture("build/oneshot/browse/host/CLIENTS.DBF");
    need(wa_set_open_rw(env, 1, "build/oneshot/browse/host/CLIENTS.DBF", "CLIENTS", NULL) == 0, "USE fixture");
    need(ui_browse(ip, "FIELDS MISSING", &ec) < 0 && ec == 48, "unknown field catalog error");
    browse(ip, "", "\030\004berlin\027");
    city(ip, 1, "HONOLULU", 0); city(ip, 2, "berlin", 0); city(ip, 3, "DALLAS", 0);
    need(wa_nrec(env, 1) == 3, "record count unchanged");
    browse(ip, "FIELDS CITY", "discard\033"); city(ip, 2, "berlin", 0);
    browse(ip, "FIELDS CITY", "\025\027"); city(ip, 2, "berlin", 1);
    browse(ip, "FIELDS CITY", "\025\027"); city(ip, 2, "berlin", 0);
    browse(ip, "FIELDS CITY FREEZE CITY NOAPPEND", "\003\030\027");
    need(wa_nrec(env, 1) == 3 && wa_recno(env, 1) == 3, "NOAPPEND at last record");
    browse(ip, "FIELDS CITY", "\030yrome\027");
    need(wa_nrec(env, 1) == 4, "append past last record"); city(ip, 4, "rome", 0);
    (void)remove("build/oneshot/browse/host/CITY.NDX");
    need(samir_do(ip, "INDEX ON CITY TO build/oneshot/browse/host/CITY") == 0, "create and attach CITY index");
    need(wa_index_count(env, 1) == 1, "index attached");
    need(wa_nav_goto(env, 1, 2) == 0, "position indexed record");
    browse(ip, "FIELDS CITY", "\031oslo\027"); city(ip, 2, "oslo", 0);
    need(samir_do(ip, "SEEK 'oslo'") == 0 && wa_found(env, 1) && wa_recno(env, 1) == 2,
         "edited index key seeks to record 2");
    browse(ip, "FIELDS CITY NOFOLLOW", "\031AAA\027");
    need(wa_recno(env, 1) == 1, "NOFOLLOW keeps the old sorted position after key change");
    need(wa_nav_goto(env, 1, 2) == 0, "position record for follow");
    browse(ip, "FIELDS CITY", "\031oslo\027");
    need(wa_recno(env, 1) == 2, "default follows the edited record to its new position");
    need(wa_close_all(env) == 0, "flush and close");
    xb_interp_free(ip); pal_host_free(pal);
    puts("PASS: BROWSE navigation, editing, Esc, delete/recall, options, append, no-database and NDX maintenance");
    return 0;
}
