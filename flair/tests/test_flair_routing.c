/* test_flair_routing.c -- the R3.7 O-1-style HOST routing suite over
 * disk_tenant_event.
 *
 * WHY THIS EXISTS. R3.7 (,  item 8) landed the
 * disk-tenant host with an emulator gate only; two guest bugs were found and
 * fixed on the real guest WITHOUT a host oracle or a mutant pinning them:
 *   (1) chrome.c drew a window's TITLE TEXT UNCLIPPED, so repainting a partly
 *       covered window's title bar stamped its Chicago cell run over the
 *       window in front (the app-launch RESTORE leg caught 176 wrong pixels);
 *   (2) a tenant that REGISTERs without opening a window must NOT become the
 *       foreground: FlairProcess_admit links it BEHIND the head, and the first
 *       NEWWINDOW is affirmed through FlairProcess_activate once the tenant has
 *       returned (disk tenant lifecycle).
 * The reconciliation's disk tenant lifecycle names the oracle this file is: "O-1-style
 * host routing suite over disk_tenant_event"
 * (Toolbox gate records in tbxgate.h).
 *
 * WHAT RUNS. The REAL os/flair/tbxgate.c (disk_tenant_event, the INT 81h
 * dispatcher tbx_gate_dispatch, tbx_launch / tbx_take_affirm / tbx_reap), the
 * REAL Layer-5 spine (process.c flair_app_dispatch / FlairProcess_admit /
 * FlairProcess_activate / teardown), window.c, heap.c, the region engine, and
 * the REAL DOS psp.c/sft.c the teardown reaches. Three seams are host fakes,
 * each the narrowest that can stand in for the guest:
 *   - flair_platform_load: carves the block through the kernel's own carve
 *     callback and writes a link-synthesized FlairTenantRec into the image
 *     (spec/toolbox_gate.h Sec 5) instead of reading TENANTFX.EXE off FAT;
 *   - flair_platform_tenant_call / flair_platform_tenant_abort (os/initechdos/tbx_gate.asm): call the
 *     host "tenant program" below for the entry / eventProc address, and
 *     unwind a triaged fault with longjmp (the asm unwinds the stack);
 *   - the tenant's `int 0x81` is a 68-byte int_frame_t image with EAX at +28
 *     and the cdecl args at +68 (spec/toolbox_gate.h Sec 2), handed to the
 *     REAL tbx_gate_dispatch.
 * The kmain pump step flair_live_tenant_service (os/initechdos/kmain.c) is
 * mirrored by service: affirm (FlairProcess_activate) FIRST, then tbx_reap.
 *
 * INDEPENDENT GOLDEN (independent checks; the test_process idiom). The tenant program
 * appends what it READ OUT OF ITS OWN FlairEvent buffer (what / message /
 * modifiers / where, the widened int32 slots of spec/toolbox_gate.h Sec 6) to
 * the delivery log; the resident Finder stub appends the EventRecord it was
 * handed. Every expected sequence below is HAND-AUTHORED from tenant lifecycle
 * / 3.4 and disk tenant lifecycle/-4 -- never read back out of the router.
 *
 * THE ADDRESS MODEL. tbxgate.c holds tenant addresses as uint32_t (the flat-32
 * guest). This suite links -no-pie and keeps the master heap -- hence every
 * tenant block -- in static storage, so every image address is < 4 GiB on the
 * 64-bit host; scene check S0 asserts it before any leg runs (fail loud, never
 * a silent truncation).
 *
 * LEGS:
 *   A  windowless REGISTER: the Finder stays foreground; no pair; keys reach
 *      the Finder (disk tenant lifecycle).                     regression patches ADMIT_IS_FOREGROUND
 *   B  first NEWWINDOW: no switch while the tenant's own entry runs; the pump
 *      affirms with exactly one deactivate THEN one activate, the activate
 *      delivered through disk_tenant_event into the tenant's FlairEvent.
 *   C  routing: keyDown to the foreground tenant regardless of the cursor;
 *      content clicks with GLOBAL where; title clicks never delivered;
 *      click-to-activate both ways (tenant lifecycle).
 *   L  plain backslash reaches the tenant; Ctrl+Alt+backslash launches with
 *      either physical modifier side; resume cooks releases without delivery.
 *                                        regression patches plain bsl launch patch
 *   G  a second launch while resident: TENANT-SLOT-BUSY, nothing installed.
 *   D  EXIT inside a handler: the dying tenant hears nothing more; teardown
 *      promotes the Finder with ONE activate; the next key is the Finder's.
 *   E  a fault inside the eventProc: FlairProcess_kill, no further delivery;
 *      a fault OUTSIDE the image is not triaged as the tenant's.
 *   H  an entry that never REGISTERs: TENANT-UNREGISTERED, nothing installed.
 *   P  an event aimed at a non-resident app is a fail-loud PANIC.
 *   T  the title run of a partly covered window writes NO pixel inside the
 *      window in front (chrome.c).                     regression patches TITLE_UNCLIPPED
 *   X  the INT 81h dispatcher itself (disk tenant lifecycle's oracle line): the AX demux
 *      is total over all 65536 codes, AX is EAX's low word, args come from
 *      frame+68 under a poisoned frame, errors are negative in EAX, a stray
 *      int 81h is NOTTENANT, the TENANT-GATE trace is exact.
 *                              regression patches ax full eax patch, arg offset patch
 *   M  SETMBAR refuses a graph outside the image, installs the tenant's own
 *      record; DRAWMENUBAR asks one redraw; a choice in the tenant's bar is
 *      delivered as TBX_EVT_MENU.   MBAR_NO_BOUNDS, MBAR_NOT_SWAPPED,
 *                                   DRAWMBAR_NOOP, MENU_NOT_DELIVERED
 *   R  launch/exit cycles leave the master heap stable (O-3).   EXIT_LEAK
 *   F  TEXTDRAW in Geneva 12: the [MEAS] 39 x 16 run box.       GENEVA_IGNORED
 *   U  DRAWCELLS underline on row B + 1 = 13.                   CELL_UL_LOW
 *   K  SETTINGS applies the Mouse record live (record.json).    MOUSE_INERT
 *
 * Ref: tenant lifecycle/3.4/7 (O-1); docs/design/GUI-remediation-ADR-
 *      reconciliation.md disk tenant lifecycle/-4; spec/toolbox_gate.h Sec 2/3/5/6;
 *      os/flair/tbxgate.c; os/flair/process.c; os/initechdos/kmain.c
 *      flair_live_tenant_service; harness/proptest/test_process.c (style).
 *      independent checks, Rules 1/2/6/11/12.
 */
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "region_algebra.h"
#include "region.h"
#include "window_record.h"
#include "window.h"
#include "event_model.h"
#include "heap.h"
#include "event.h"           /* FLAIR_SCREEN_W / FLAIR_SCREEN_H */
#include "system_key.h"
#include "process.h"
#include "tbxgate.h"
#include "menu.h"            /* MenuBar: leg M installs the tenant's own bar */
#include "toolbox_gate.h"
#include "flair_platform.h"
// #include "sft.h"             /* sft_init: the boot-time SFT the teardown reaps */
#include "render.h"
#include "chrome.h"
#include "chrome_metrics.h"
#include "flair_look.h"
#include "text.h"
#include "test_assert.h"
#include "expect_abort.h"

TEST_HARNESS();

/* ===========================================================================
 * 1. ARENA SCAFFOLDING (the test_process rgn_store/win_store/mgr_store idiom)
 * ===========================================================================*/
typedef struct rgn_store {
    region_t  r;
    rgn_row_t rows[RGN_ROWS_CAP];
    int16_t   pool[RGN_X_POOL_CAP];
} rgn_store_t;

static void store_attach(rgn_store_t *s)
{
    memset(s, 0, sizeof *s);
    s->r.rows = s->rows; s->r.cap_rows = RGN_ROWS_CAP;
    s->r.x_pool = s->pool; s->r.x_pool_cap = RGN_X_POOL_CAP;
    region_set_empty(&s->r);
}

typedef struct win_store {
    WindowRecord rec;
    rgn_store_t  struc, cont, upd;
} win_store_t;

static void win_attach(win_store_t *w)
{
    memset(&w->rec, 0, sizeof w->rec);
    store_attach(&w->struc); store_attach(&w->cont); store_attach(&w->upd);
    w->rec.strucRgn = &w->struc.r;
    w->rec.contRgn = &w->cont.r;
    w->rec.updateRgn = &w->upd.r;
}

static rgn_rect_t R(int top, int left, int bottom, int right)
{
    rgn_rect_t r;
    r.top = (int16_t)top; r.left = (int16_t)left;
    r.bottom = (int16_t)bottom; r.right = (int16_t)right;
    return r;
}

/* Half-open [top,bottom) x [left,right) membership, independent of window.c. */
static int rect_has(rgn_rect_t r, int h, int v)
{
    return v >= r.top && v < r.bottom && h >= r.left && h < r.right;
}

/* ===========================================================================
 * 2. THE SCENE (hand-authored geometry, 640x480; all values from the spec)
 * ===========================================================================*/
/* The resident Finder's window (frame, global). */
#define F_TOP 60
#define F_LEFT 20
#define F_BOTTOM 300
#define F_RIGHT 280
/* The tenant's NEWWINDOW frame: l,t,r,b (the gate's argument order). */
#define T_L 340
#define T_T 100
#define T_R 600
#define T_B 320
/* Probes: content probes sit >= 30 px inside the frame on every side -- more
 * than the 22-px title band (FLAIR_CHROME_TITLEBAR_H) and the 16-px bars
 * (FLAIR_CHROME_SCROLLBAR_W) -- so they are content by arithmetic alone. The
 * title probe sits 10 rows below the frame top, mid-width, clear of the boxes. */
#define F_PROBE_H 150
#define F_PROBE_V 180
#define T_PROBE_H 470
#define T_PROBE_V 210
#define T_TITLE_H 470
#define T_TITLE_V (T_T + 10)

/* The tenant image layout (offsets from load_base; spec/toolbox_gate.h Sec 5:
 * the FlairTenantRec and everything it points at lie inside the image). */
#define IMG_REC   0x000u
#define IMG_NAME  0x020u
#define IMG_EVB   0x040u
#define IMG_ENTRY 0x080u
#define IMG_PROC  0x0C0u
#define IMG_LEN   0x200u

enum { WHO_F = 1, WHO_T = 2 };

typedef struct ent {
    int     who;
    int32_t what, msg, mods, h, v;
} ent_t;

enum { LOG_CAP = 64 };
static ent_t g_log[LOG_CAP];
static int   g_n;

static void log_add(int who, int32_t what, int32_t msg, int32_t mods,
                    int32_t h, int32_t v)
{
    if (g_n >= LOG_CAP) return;
    g_log[g_n].who = who; g_log[g_n].what = what; g_log[g_n].msg = msg;
    g_log[g_n].mods = mods; g_log[g_n].h = h; g_log[g_n].v = v;
    g_n++;
}

/* active < 0: compare where (h,v); active >= 0: compare the activate flag. */
static int is(int i, int who, int32_t what, int active, int h, int v)
{
    const ent_t *e;
    if (i < 0 || i >= g_n) return 0;
    e = &g_log[i];
    if (e->who != who || e->what != what) return 0;
    if (active >= 0)
        return ((e->mods & (int32_t)FLAIR_EVT_MOD_ACTIVE_FLAG) ? 1 : 0) == active;
    return e->h == h && e->v == v;
}

static int count_who(int from, int who)
{
    int n = 0;
    for (int i = from; i < g_n; i++) if (g_log[i].who == who) n++;
    return n;
}

/* The serial sink: the TENANT-* vocabulary the guest prints. */
static char g_serial[16384];
static size_t g_serial_n;
static int g_serial_fd = -1;    /* a forked probe streams to its pipe */

static void serial_cap(const char *s)
{
    size_t k = strlen(s);
    if (g_serial_fd >= 0) { ssize_t w = write(g_serial_fd, s, k); (void)w; }
    if (g_serial_n + k < sizeof g_serial) {
        memcpy(g_serial + g_serial_n, s, k);
        g_serial_n += k;
        g_serial[g_serial_n] = '\0';
    }
}

static int serial_has(const char *needle) { return strstr(g_serial, needle) != NULL; }

/* The world: one master heap, one Window Manager, the resident Finder. */
static uint8_t g_master_mem[192u * 1024u] __attribute__((aligned(16)));

static struct world {
    flair_heap_t     master;
    WindowMgr        wm;
    rgn_store_t      desk, sa, sb, sc;
    win_store_t      fw;
    FlairApp         finder;
    FlairProcessList plist;
} W;

static void finder_event(FlairApp *self, const EventRecord *ev)
{
    if (self != &W.finder) { fprintf(stderr, "  FAIL finder_event: wrong self\n"); abort(); }
    log_add(WHO_F, ev->what, (int32_t)ev->message, ev->modifiers,
            ev->where.h, ev->where.v);
}

static const FlairAppProcs g_finder_procs = { NULL, finder_event, NULL, NULL };

/* ===========================================================================
 * 3. THE TENANT PROGRAM (the host stand-in for TENANTFX.EXE)
 * ===========================================================================*/
static struct tprog {
    int      do_register, do_newwindow;
    int      exit_on, crash_on, stray_on;     /* keyDown chars that trigger */
    /* what the tenant observed from inside its own code: */
    int32_t  reg_rc, win_tok, exit_rc;
    FlairApp *head_in_entry;
    int      log_n_in_entry;
    int      entry_runs, proc_runs, stray_returned;
    void   (*script)(void);       /* leg X: a scripted entry instead */
} TP;

static uint32_t g_base;           /* the current image's load_base           */
static uint32_t g_psp;            /* the current block's PSP address         */
static jmp_buf  g_unwind;

/* One `int 0x81`: the 68-byte int_frame_t image with EAX at +28, the cdecl
 * args from +68 up, every OTHER dword (the rest of the frame and the stack
 * above the args) filled with `poison`. Returns the EAX the dispatcher left. */
static uint32_t gate_raw(uint32_t eax, uint32_t argc, const uint32_t *args,
                         uint32_t poison)
{
    uint32_t frame[(TBX_GATE_ARG_OFFSET / 4u) + TBX_ARGC_MAX + 2u];
    for (uint32_t i = 0; i < sizeof frame / sizeof frame[0]; i++) frame[i] = poison;
    frame[28u / 4u] = eax;                                 /* EAX at +28 */
    for (uint32_t i = 0; i < argc; i++)
        frame[TBX_GATE_ARG_OFFSET / 4u + i] = args[i];     /* cdecl at +68 */
    tbx_gate_dispatch((uint8_t *)frame);
    return frame[28u / 4u];
}

static int32_t gate(uint32_t ax, uint32_t argc, const uint32_t *args)
{
    return (int32_t)gate_raw(ax, argc, args, 0u);
}

static void tenant_entry(void)
{
    TP.entry_runs++;
    if (TP.script != NULL) {
        TP.script();
        return;
    }
    if (TP.do_register) {
        uint32_t a[1] = { g_base + IMG_REC };
        TP.reg_rc = gate(TBX_REGISTER, 1u, a);
    }
    if (TP.do_newwindow) {
        uint32_t a[5] = { T_L, T_T, T_R, T_B, 1u };
        TP.win_tok = gate(TBX_NEWWINDOW, 5u, a);
    }
    TP.head_in_entry = W.plist.head;
    TP.log_n_in_entry = g_n;
}

static void tenant_eventproc(void)
{
    const volatile int32_t *eb =
        (const volatile int32_t *)(uintptr_t)(g_base + IMG_EVB);
    int32_t what = eb[FLAIR_EVBUF_WHAT_OFF / 4u];
    int32_t msg = eb[FLAIR_EVBUF_MSG_OFF / 4u];

    TP.proc_runs++;
    log_add(WHO_T, what, msg, eb[FLAIR_EVBUF_MODS_OFF / 4u],
            eb[FLAIR_EVBUF_WHEREH_OFF / 4u], eb[FLAIR_EVBUF_WHEREV_OFF / 4u]);
    if (what != keyDown) return;
    if (TP.exit_on && (msg & 0xFF) == TP.exit_on) {
        uint32_t a[1] = { 7u };
        TP.exit_rc = gate(TBX_EXIT, 1u, a);
    }
    if (TP.stray_on && (msg & 0xFF) == TP.stray_on) {
        /* A #GP whose EIP is OUTSIDE the image: a kernel fault during a
         * trap, never the tenant's -- triage must return (the kernel HALTs). */
        tbx_fault_triage(13u, 0x00001000u);
        TP.stray_returned = 1;
    }
    if (TP.crash_on && (msg & 0xFF) == TP.crash_on) {
        /* A #GP at an EIP inside the eventProc: the triage unwinds. */
        tbx_fault_triage(13u, g_base + IMG_PROC + 5u);
        fprintf(stderr, "  FAIL tenant: tbx_fault_triage returned for an in-image fault\n");
        abort();
    }
}

/* os/initechdos/tbx_gate.asm, host form. */
int flair_platform_tenant_call(uint32_t fn, uint32_t psp)
{
    if (psp != g_psp) { fprintf(stderr, "  FAIL flair_platform_tenant_call: psp != the block's PSP\n"); abort(); }
    if (setjmp(g_unwind) != 0) return 1;                 /* crashed */
    if (fn == g_base + IMG_ENTRY) tenant_entry();
    else if (fn == g_base + IMG_PROC) tenant_eventproc();
    else { fprintf(stderr, "  FAIL flair_platform_tenant_call: fn +0x%x is no tenant entry point\n",
                   (unsigned)(fn - g_base)); abort(); }
    return 0;
}

void flair_platform_tenant_abort(void)
{
    longjmp(g_unwind, 1);
}

static void put32(uint32_t addr, uint32_t v)
{
    *(volatile uint32_t *)(uintptr_t)addr = v;
}

/* os/initechdos/loader.c flair_platform_load, host form: carve through the KERNEL's
 * callback, then lay down the link-synthesized FlairTenantRec. */
int flair_platform_load(const char *name83, uint16_t dir_start,
                                   flair_platform_carve carve, void *user,
                                   void **out_block, flair_platform_plan *out)
{
    uint32_t blen = TBX_TENANT_PSP_BYTES + IMG_LEN;
    uint8_t *blk;
    (void)name83; (void)dir_start;
    blk = (uint8_t *)carve(blen, user);
    *out_block = blk;
    if (blk == NULL) return FLAIR_PLATFORM_LOAD_NOMEM;
    memset(blk, 0, blen);
    memset(out, 0, sizeof *out);
    out->psp_addr = (uint32_t)(uintptr_t)blk;
    out->load_base = out->psp_addr + TBX_TENANT_PSP_BYTES;
    out->module_len = IMG_LEN;
    out->entry = out->load_base + IMG_ENTRY;
    out->block_len = blen;
    g_psp = out->psp_addr;
    g_base = out->load_base;
    put32(g_base + IMG_REC + FLAIR_TENANT_REC_TAG_OFF, FLAIR_TENANT_REC_TAG);
    put32(g_base + IMG_REC + FLAIR_TENANT_REC_NAME_OFF, g_base + IMG_NAME);
    put32(g_base + IMG_REC + FLAIR_TENANT_REC_BASE_OFF, g_base);
    put32(g_base + IMG_REC + FLAIR_TENANT_REC_LEN_OFF, IMG_LEN);
    put32(g_base + IMG_REC + FLAIR_TENANT_REC_EVB_OFF, g_base + IMG_EVB);
    put32(g_base + IMG_REC + FLAIR_TENANT_REC_PROC_OFF, g_base + IMG_PROC);
    memcpy((void *)(uintptr_t)(g_base + IMG_NAME), "TENANTFX", 9);
    return FLAIR_PLATFORM_LOAD_OK;
}

/* ===========================================================================
 * 4. THE PUMP (kmain flair_live_tenant_service, host form)
 * ===========================================================================*/
static const bitmap_t *probe_surface;
static const uint8_t *probe_cellfont;   /* leg U: a hand-authored 8x16 strike */
static void world_init(void)
{
    tbx_host_t host;

    memset(&W, 0, sizeof W);
    memset(&TP, 0, sizeof TP);
    g_n = 0;
    g_serial_n = 0; g_serial[0] = '\0';
    /* No open files in the routing scenario. */     /* kmain does this at boot; tbxf_reap walks the JFT */
    flair_heap_init(&W.master, g_master_mem, (uint32_t)sizeof g_master_mem);
    store_attach(&W.desk); store_attach(&W.sa);
    store_attach(&W.sb); store_attach(&W.sc);
    WindowMgr_init(&W.wm, R(0, 0, FLAIR_SCREEN_H, FLAIR_SCREEN_W),
                   &W.desk.r, &W.sa.r, &W.sb.r, &W.sc.r);
    win_attach(&W.fw);
    NewDocumentWindow(&W.wm, &W.fw.rec, R(F_TOP, F_LEFT, F_BOTTOM, F_RIGHT),
                      documentKind, 1);
    W.finder.name = "FINDER";
    W.finder.procs = &g_finder_procs;
    W.finder.windows = &W.fw.rec;
    FlairProcessList_init(&W.plist);
    FlairProcess_register(&W.plist, &W.finder);

    memset(&host, 0, sizeof host);
    host.list = &W.plist;
    host.wm = &W.wm;
    host.master = &W.master;
    host.puts = serial_cap;
    host.surface = probe_surface;
    host.cellfont = probe_cellfont;
    tbx_bind(&host);
}

static void service(void)
{
    FlairApp *app = tbx_take_affirm();
    if (app != NULL) {
        EventRecord ev0;
        memset(&ev0, 0, sizeof ev0);
        ev0.what = (uint16_t)nullEvent;
        (void)FlairProcess_activate(&W.plist, &W.wm, &ev0, app);
    }
    (void)tbx_reap();
}

static EventRecord mk(uint16_t what, uint32_t msg, int h, int v)
{
    EventRecord ev;
    memset(&ev, 0, sizeof ev);
    ev.what = what;
    ev.message = msg;
    ev.where.h = (int16_t)h;
    ev.where.v = (int16_t)v;
    return ev;
}

static void post(EventRecord ev) { flair_app_dispatch(&W.plist, &W.wm, &ev); }
static unsigned launches;
static void pump(EventRecord ev)
{
    /* Same pre-dispatch predicate as kmain; a launch is the host sink. */
    if (flair_text_tenant_hotkey(&ev)) launches++;
    else post(ev);
    service();
}

/* Ref: R2.2 Reference Manual p. 1-17 (PDF 33): backslash repeats a label.
 * tenant lifecycle keys belong to the foreground; event_model.h Sec 2/5
 * and event.c SET-1 decoder unify left/right Ctrl and Alt (E0 1D/38).
 * All expected deliveries/counts here are literal, independent of routing. */
static void leg_launchkey(void)
{
    static const uint8_t plain[] = {0x2B, 0xAB};
    static const uint8_t left[] = {0x1D,0x38,0x2B,0xAB,0xB8,0x9D};
    static const uint8_t right[] = {0xE0,0x1D,0xE0,0x38,0x2B,0xAB,
                                    0xE0,0xB8,0xE0,0x9D};
    static const uint8_t lr[] = {0x1D,0xE0,0x38,0x2B,0xAB,0xE0,0xB8,0x9D};
    static const uint8_t rl[] = {0xE0,0x1D,0x38,0x2B,0xAB,0xB8,0xE0,0x9D};
    static const uint8_t ctrl[] = {0x1D,0x2B,0xAB,0x9D};
    static const uint8_t alt[] = {0x38,0x2B,0xAB,0xB8};
    static const uint8_t shift[] = {0x2A,0x2B,0xAB,0xAA};
    const uint8_t *traces[] = {plain,left,right,lr,rl,ctrl,alt,shift,plain};
    const unsigned lens[] = {sizeof plain,sizeof left,sizeof right,sizeof lr,
                            sizeof rl,sizeof ctrl,sizeof alt,sizeof shift,sizeof plain};
    const uint16_t mods[] = {0,0x1800,0x1800,0x1800,0x1800,0x1000,0x0800,0x0200,0};
    flair_raw_ring_t ring;
    flair_event_init(&ring);
    launches = 0;
    for (unsigned t = 0; t < sizeof lens / sizeof lens[0]; t++) {
        int b = g_n;
        unsigned l = launches;
        for (unsigned i = 0; i < lens[t]; i++) {
            flair_raw_event_t raw = {FLAIR_RAW_KEYBOARD, 0u, traces[t][i]};
            CHECK(flair_raw_post(&ring, &raw), "leg(L0): raw trace fits ring");
        }
        EventRecord ev;
        unsigned down = 0;
        for (unsigned i = 0; i < lens[t]; i++) {
            (void)GetNextEvent(&ring, everyEvent, &ev);
            if (ev.what != keyDown) continue;
            down++;
            CHECK((ev.modifiers & 0x1A00u) == mods[t],
                  "leg(L0b): physical modifier sides cook to the locked flags");
            pump(ev);
            if (t >= 1 && t <= 4) flair_text_tenant_drain(&ring);
        }
        if (t >= 1 && t <= 4) {
            CHECK(down == 1 && launches == l + 1 && g_n == b,
                  "leg(L2): either Ctrl plus either Alt launches, never reaches tenant");
        } else {
            CHECK(down == 1 && launches == l && g_n == b + 1 &&
                  g_log[b].who == WHO_T && g_log[b].what == keyDown &&
                  g_log[b].msg == (t == 7 ? 0x2B7C : 0x2B5C),
                  "leg(L1): plain backslash and non-launch modifiers reach tenant");
        }
        (void)GetNextEvent(&ring, everyEvent, &ev);
        CHECK((ev.modifiers & 0x1A00u) == 0,
              "leg(L5): suspend drain cooks modifier releases before sheet resumes");
    }
    EventRecord ev = mk(keyUp, 0x2B5Cu, 0, 0);
    ev.modifiers = 0x1800u;
    CHECK(!flair_text_tenant_hotkey(&ev), "leg(L3): chord keyUp never launches");
    ev.what = autoKey;
    CHECK(!flair_text_tenant_hotkey(&ev), "leg(L4): chord autoKey never launches");
}

static FlairApp *tenant_app(void)
{
    for (FlairApp *a = W.plist.head; a != NULL; a = a->nextApp)
        if (a != &W.finder) return a;
    return NULL;
}

static int windows_in_zorder(void)
{
    int n = 0;
    for (WindowPtr w = W.wm.front; w != NULL; w = w->nextWindow) n++;
    return n;
}

static int launch(void)
{
    return tbx_launch("TENANTFX.EXE", 0u);
}

/* ===========================================================================
 * 5. THE LEGS
 * ===========================================================================*/
static void scene_checks(void)
{
    world_init();
    CHECK((uintptr_t)(void *)g_master_mem + sizeof g_master_mem <= 0xFFFFFFFFu,
          "S0: the master heap (every tenant block) is below 4 GiB -- the "
          "flat-32 address model holds on the host (link -no-pie)");
    CHECK(rect_has(R(F_TOP + 30, F_LEFT + 30, F_BOTTOM - 30, F_RIGHT - 30),
                   F_PROBE_H, F_PROBE_V) &&
          rect_has(R(T_T + 30, T_L + 30, T_B - 30, T_R - 30), T_PROBE_H, T_PROBE_V),
          "S1: both content probes lie >= 30 px inside their frames (content by arithmetic)");
    CHECK(T_TITLE_V - T_T >= 1 && T_TITLE_V - T_T < FLAIR_CHROME_TITLEBAR_H - 1 &&
          T_TITLE_H - T_L > 60 && T_R - T_TITLE_H > 60,
          "S2: the tenant title probe lies inside the title band, clear of the boxes");
    CHECK(F_RIGHT < T_L, "S3: the Finder and tenant windows are disjoint in x");
    CHECK(W.plist.head == &W.finder && W.wm.front == &W.fw.rec && W.fw.rec.hilited,
          "S4: the resident Finder is the foreground app with the one active window");
}

/* --- leg A: a WINDOWLESS registration never takes the foreground --------- */
static void leg_admit(void)
{
    FlairApp *t;
    int rc;

    world_init();
    TP.do_register = 1;
    rc = launch();
    service();
    t = tenant_app();
    CHECK(rc == TBX_OK && TP.reg_rc == TBX_OK && TP.entry_runs == 1 &&
          serial_has("TENANT-REGISTER ok app=TENANTFX\n"),
          "leg(A0): the windowless tenant's REGISTER succeeds (TENANT-REGISTER ok)");
    CHECK(t != NULL && t->windows == NULL,
          "leg(A0b): the tenant is resident and owns no window");
    CHECK(W.plist.head == &W.finder,
          "leg(A1): a tenant that REGISTERs without a window does NOT become the "
          "foreground -- the Finder stays the head (DEC-AC3-4)");
    CHECK(t != NULL && W.finder.state == FLAIR_APP_FG && t->state == FLAIR_APP_BG &&
          W.finder.nextApp == t,
          "leg(A1b): it is admitted BEHIND the head as a background app");
    CHECK(g_n == 0,
          "leg(A1c): no activateEvt is synthesized for a windowless registration");
    CHECK(W.wm.front == &W.fw.rec && W.fw.rec.hilited,
          "leg(A1d): the Finder's window stays front and active");
    pump(mk(keyDown, 'x', T_PROBE_H, T_PROBE_V));
    CHECK(g_n == 1 && is(0, WHO_F, keyDown, -1, T_PROBE_H, T_PROBE_V) &&
          g_log[0].msg == 'x',
          "leg(A2): the next keystroke reaches the foreground Finder");
    CHECK(count_who(0, WHO_T) == 0 && TP.proc_runs == 0,
          "leg(A2b): disk_tenant_event delivers nothing to the windowless tenant");
}

/* --- leg B: the FIRST window is affirmed after the entry returns ---------- */
static FlairApp *leg_affirm(void)
{
    FlairApp *t;
    int rc;

    world_init();
    TP.do_register = 1;
    TP.do_newwindow = 1;
    rc = launch();
    CHECK(rc == TBX_OK && TP.reg_rc == TBX_OK && TP.win_tok == 1,
          "leg(B0): REGISTER then NEWWINDOW inside the entry -> window token 1");
    CHECK(TP.head_in_entry == &W.finder && TP.log_n_in_entry == 0,
          "leg(B1): while the tenant's own entry runs NOTHING switches -- the Finder "
          "is the head and no event was delivered (tenant code is never re-entered "
          "from inside its own trap)");
    CHECK(W.plist.head == &W.finder && g_n == 0 && TP.proc_runs == 0,
          "leg(B1b): the entry has returned; until the pump services it, still no switch");
    service();
    t = tenant_app();
    CHECK(g_n == 2 && is(0, WHO_F, activateEvt, 0, 0, 0) &&
          is(1, WHO_T, activateEvt, 1, 0, 0),
          "leg(B2): the pump's affirmation emits EXACTLY one deactivate to the Finder "
          "THEN one activate to the tenant, the activate read out of the tenant's "
          "own FlairEvent (disk_tenant_event)");
    CHECK(t != NULL && W.plist.head == t && t->state == FLAIR_APP_FG &&
          W.finder.state == FLAIR_APP_BG,
          "leg(B2b): the tenant is now the foreground head");
    CHECK(t != NULL && t->windows != NULL && W.wm.front == t->windows &&
          t->windows->hilited && !W.fw.rec.hilited,
          "leg(B2c): its window is front and the one active window");
    CHECK(serial_has("TENANT-EVT what=8\n"),
          "leg(B2d): the delivery is on the serial trace (TENANT-EVT what=8)");
    return t;
}

/* --- leg C: routing through disk_tenant_event ----------------------------- */
static void leg_route(FlairApp *t)
{
    int b;

    /* C1: keyDown with the cursor over the FINDER's window. */
    b = g_n;
    pump(mk(keyDown, 'k', F_PROBE_H, F_PROBE_V));
    CHECK(g_n == b + 1 && is(b, WHO_T, keyDown, -1, F_PROBE_H, F_PROBE_V) &&
          g_log[b].msg == 'k',
          "leg(C1): keyDown reaches the FOREGROUND tenant, not the window under the "
          "cursor; its FlairEvent carries what/message/where intact");

    /* C2: a content click on the tenant's own window. */
    b = g_n;
    pump(mk(mouseDown, 0u, T_PROBE_H, T_PROBE_V));
    CHECK(g_n == b + 1 && is(b, WHO_T, mouseDown, -1, T_PROBE_H, T_PROBE_V),
          "leg(C2): a content mouseDown reaches the tenant in GLOBAL coordinates");

    /* C3: a title click on the foreground tenant's window. */
    b = g_n;
    pump(mk(mouseDown, 0u, T_TITLE_H, T_TITLE_V));
    CHECK(g_n == b,
          "leg(C3): a title mouseDown is shell-owned chrome -- never delivered");

    /* C4: click the background Finder's content. */
    b = g_n;
    pump(mk(mouseDown, 0u, F_PROBE_H, F_PROBE_V));
    /* pinned inputs re-key 2026-10-20 (, merged with the Finder alerts
     * bundle): SelectWindow CONSUMES a document's activating content click
     * (IM: Macintosh Toolbox Essentials, Window Manager, Activating a window;
     * process.c ). The click used to be delivered after the activate
     * pair, the audit G11/ defect, so the leg now expects exactly the
     * deactivate/activate pair and NO mouseDown to the newly active app. */
    CHECK(g_n == b + 2 && is(b, WHO_T, activateEvt, 0, 0, 0) &&
          is(b + 1, WHO_F, activateEvt, 1, 0, 0),
          "leg(C4): click-to-activate away from the tenant: deactivate the tenant, "
          "activate the Finder; the activating click is consumed (tdnl.65)");
    CHECK(W.plist.head == &W.finder && !t->windows->hilited && W.fw.rec.hilited,
          "leg(C4b): the Finder is foreground; the tenant's window is drawn inactive");

    /* C5: keys now belong to the Finder, cursor over the tenant. */
    b = g_n;
    pump(mk(keyDown, 'z', T_PROBE_H, T_PROBE_V));
    CHECK(g_n == b + 1 && is(b, WHO_F, keyDown, -1, T_PROBE_H, T_PROBE_V),
          "leg(C5): the background tenant hears no keystroke");

    /* C6: click the background tenant's content. */
    b = g_n;
    pump(mk(mouseDown, 0u, T_PROBE_H, T_PROBE_V));
    CHECK(g_n == b + 2 && is(b, WHO_F, activateEvt, 0, 0, 0) &&
          is(b + 1, WHO_T, activateEvt, 1, 0, 0),
          "leg(C6): click-to-activate back: deactivate the Finder, activate the "
          "tenant; the activating click is consumed (tdnl.65)");
    CHECK(W.plist.head == t && t->windows->hilited && !W.fw.rec.hilited,
          "leg(C6b): the tenant is foreground again with the active window");
}

/* --- leg G: a second launch while one tenant is resident ------------------ */
static void leg_busy(FlairApp *t)
{
    int b = g_n, entries = TP.entry_runs;
    uint32_t avail = flair_heap_avail(&W.master);
    int rc = launch();
    CHECK(rc == TBX_ERR_BUSY && serial_has("TENANT-SLOT-BUSY name=TENANTFX.EXE\n"),
          "leg(G1): a second launch while resident is refused (TENANT-SLOT-BUSY)");
    CHECK(TP.entry_runs == entries && g_n == b && W.plist.head == t &&
          flair_heap_avail(&W.master) == avail,
          "leg(G2): nothing ran, nothing was delivered, nothing was carved");
}

/* --- leg D: EXIT inside a handler ----------------------------------------- */
static void leg_exit(void)
{
    int b;

    TP.exit_on = 'q';
    b = g_n;
    post(mk(keyDown, 'q', T_PROBE_H, T_PROBE_V));
    CHECK(g_n == b + 1 && is(b, WHO_T, keyDown, -1, T_PROBE_H, T_PROBE_V) &&
          TP.exit_rc == TBX_OK,
          "leg(D1): the tenant hears its 'q' and EXITs from inside the handler (-> 0)");

    b = g_n;
    post(mk(keyDown, 'w', T_PROBE_H, T_PROBE_V));
    CHECK(g_n == b,
          "leg(D2): a tenant that asked to EXIT hears nothing more -- the event to "
          "the still-linked head is dropped, never run");

    b = g_n;
    service();
    CHECK(g_n == b + 1 && is(b, WHO_F, activateEvt, 1, 0, 0),
          "leg(D3): teardown promotes the Finder with ONE activate; the dying tenant "
          "receives no deactivate");
    CHECK(W.plist.head == &W.finder && tenant_app() == NULL &&
          W.wm.front == &W.fw.rec && windows_in_zorder() == 1 && W.fw.rec.hilited,
          "leg(D3b): the tenant and its window are gone; the Finder's window is active");
    CHECK(serial_has("TENANT-EXIT rc=7 via=exit\n"),
          "leg(D3c): TENANT-EXIT rc=7 via=exit");

    b = g_n;
    pump(mk(keyDown, 'e', T_PROBE_H, T_PROBE_V));
    CHECK(g_n == b + 1 && is(b, WHO_F, keyDown, -1, T_PROBE_H, T_PROBE_V),
          "leg(D4): the next keystroke reaches the Finder (the CLOSE_STALE_FOCUS "
          "shape, on the host)");
}

/* --- leg E: a fault inside the tenant ------------------------------------ */
static void leg_crash(void)
{
    int b;

    world_init();
    TP.do_register = 1;
    TP.do_newwindow = 1;
    TP.crash_on = 'c';
    TP.stray_on = 'o';
    (void)launch();
    service();

    b = g_n;
    pump(mk(keyDown, 'o', T_PROBE_H, T_PROBE_V));
    CHECK(TP.stray_returned == 1 && tenant_app() != NULL && g_n == b + 1 &&
          !serial_has("TENANT-CRASH"),
          "leg(E0): a fault whose EIP lies OUTSIDE the image is not triaged as the "
          "tenant's (the kernel halts); the tenant lives");

    b = g_n;
    pump(mk(keyDown, 'c', T_PROBE_H, T_PROBE_V));
    CHECK(serial_has("TENANT-CRASH vec=13 eip=+0x00c5\n"),
          "leg(E1): a fault inside the eventProc is triaged (TENANT-CRASH eip=+0x00c5)");
    CHECK(g_n == b + 2 && is(b, WHO_T, keyDown, -1, T_PROBE_H, T_PROBE_V) &&
          is(b + 1, WHO_F, activateEvt, 1, 0, 0),
          "leg(E2): the dead tenant is killed between pump iterations -- nothing more "
          "reaches it; the Finder gets the one activate");
    CHECK(W.plist.head == &W.finder && tenant_app() == NULL &&
          windows_in_zorder() == 1 && serial_has("TENANT-EXIT rc=-1 via=crash\n"),
          "leg(E2b): FlairProcess_kill removed the tenant and its window (via=crash)");
    b = g_n;
    pump(mk(keyDown, 'n', T_PROBE_H, T_PROBE_V));
    CHECK(g_n == b + 1 && is(b, WHO_F, keyDown, -1, T_PROBE_H, T_PROBE_V),
          "leg(E3): the next keystroke reaches the Finder");
}

/* --- leg H: an entry that never registers -------------------------------- */
static void leg_noreg(void)
{
    world_init();
    (void)launch();
    service();
    CHECK(TP.entry_runs == 1 && serial_has("TENANT-UNREGISTERED name=TENANTFX.EXE\n"),
          "leg(H1): an entry that returns without REGISTER is reaped (TENANT-UNREGISTERED)");
    CHECK(W.plist.head == &W.finder && W.finder.nextApp == NULL && g_n == 0 &&
          windows_in_zorder() == 1,
          "leg(H2): nothing was installed and nothing delivered");
    {
        int rc;
        TP.do_register = 1;
        rc = launch();
        service();
        CHECK(rc == TBX_OK && tenant_app() != NULL &&
              serial_has("TENANT-REGISTER ok app=TENANTFX\n"),
              "leg(H3): the slot is free again -- the next launch is admitted");
    }
}

/* --- leg P: the fail-loud guard of disk_tenant_event ---------------------- */
static void leg_panic(void)
{
    int fds[2];
    pid_t pid;
    int status = 0;
    char buf[512];
    ssize_t got;
    size_t n = 0;

    world_init();
    TP.do_register = 1;
    TP.do_newwindow = 1;
    (void)launch();
    service();
    if (pipe(fds) != 0) { CHECK(0, "leg(P) pipe"); return; }
    (void)fflush(NULL);
    pid = fork();
    if (pid == 0) {
        EventRecord ev = mk(keyDown, 'p', 0, 0);
        FlairApp *t = tenant_app();
        close(fds[0]);
        g_serial_fd = fds[1];
        alarm(10);
        t->procs->event(&W.finder, &ev);   /* not the resident disk tenant */
        _exit(0);
    }
    close(fds[1]);
    while (n < sizeof buf - 1 && (got = read(fds[0], buf + n, sizeof buf - 1 - n)) > 0)
        n += (size_t)got;
    buf[n] = '\0';
    close(fds[0]);
    if (pid > 0) waitpid(pid, &status, 0);
    CHECK(pid > 0 && WIFSIGNALED(status) &&
          strstr(buf, "PANIC tbx: event for an app that is not the resident disk tenant\n") != NULL,
          "leg(P1): an event aimed at a non-resident app PANICs fail-loud (Rule 2), "
          "never runs tenant code");
}

/* --- leg T: the clipped title run (chrome.c) ------------------------------ */
/* The back window (inactive) and the window in front of it. The front window
 * covers the right part of the back window's title band, INCLUDING part of
 * the centred title run, so the title is partly covered. */
#define TB_TOP 100
#define TB_LEFT 100
#define TB_BOTTOM 260
#define TB_RIGHT 340
#define TF_TOP 80
#define TF_LEFT 220
#define TF_BOTTOM 300
#define TF_RIGHT 420

static int g_t_clip;              /* 1: visRgn = port minus the front window */
static rgn_store_t g_t_front, g_t_all;

static void draw_back(GrafPort *port)
{
    if (g_t_clip) {
        store_attach(&g_t_front); store_attach(&g_t_all);
        region_set_rect(&g_t_front.r, R(TF_TOP, TF_LEFT, TF_BOTTOM, TF_RIGHT));
        region_set_rect(&g_t_all.r, R(0, 0, FLAIR_SCREEN_H, FLAIR_SCREEN_W));
        region_op(port->visRgn, &g_t_all.r, &g_t_front.r, RGN_OP_DIFF);
    }
    flair_draw_document_window(port, flair_look_default_skin(),
                               R(TB_TOP, TB_LEFT, TB_BOTTOM, TB_RIGHT),
                               "NOTES", 0, FLAIR_WINDOW_WIDGET_ALL);
}

static void leg_title(void)
{
    static uint32_t pre[FLAIR_SCREEN_H][FLAIR_SCREEN_W];
    static uint32_t full[FLAIR_SCREEN_H][FLAIR_SCREEN_W];
    render_boot_info_t boot = { 0u, 0u, 32u, FLAIR_SCREEN_W, FLAIR_SCREEN_H };
    render_ctx_t a, b;
    rgn_rect_t front = R(TF_TOP, TF_LEFT, TF_BOTTOM, TF_RIGHT);
    int text_top = TB_TOP + FLAIR_CHROME_TITLE_TEXT_TOP_OFF;
    long title_under = 0, wrote_under = 0, title_wrote_under = 0, differ_out = 0;

    if (render_ctx_init(&a, &boot) != 0 || render_ctx_init(&b, &boot) != 0) {
        CHECK(0, "leg(T) render contexts");
        return;
    }
    for (int y = 0; y < FLAIR_SCREEN_H; y++)
        for (int x = 0; x < FLAIR_SCREEN_W; x++)
            pre[y][x] = render_pixel_rgb(&a, (uint32_t)x, (uint32_t)y);

    g_t_clip = 0;                 /* reference: nothing in front */
    render_run(&b, draw_back);
    for (int y = 0; y < FLAIR_SCREEN_H; y++)
        for (int x = 0; x < FLAIR_SCREEN_W; x++)
            full[y][x] = render_pixel_rgb(&b, (uint32_t)x, (uint32_t)y);

    g_t_clip = 1;                 /* the repaint of the partly covered window */
    render_run(&a, draw_back);

    for (int y = 0; y < FLAIR_SCREEN_H; y++) {
        for (int x = 0; x < FLAIR_SCREEN_W; x++) {
            uint32_t got = render_pixel_rgb(&a, (uint32_t)x, (uint32_t)y);
            int in_front = rect_has(front, x, y);
            int text_row = y >= text_top && y < text_top + CHICAGO_CELL_H;
            if (in_front) {
                if (text_row && full[y][x] != pre[y][x]) title_under++;
                if (got != pre[y][x]) {
                    wrote_under++;
                    if (text_row) title_wrote_under++;
                }
            } else if (got != full[y][x]) {
                differ_out++;
            }
        }
    }
    CHECK(title_under > 0,
          "leg(T0) scene: unclipped, the title run DOES extend under the front "
          "window (the scene covers part of the title)");
    CHECK(title_wrote_under == 0,
          "leg(T1): the covered title run writes NO pixel inside the front window "
          "(chrome_ctext clips to visRgn; bead initech-tdnl.14)");
    CHECK(wrote_under == 0,
          "leg(T2): no chrome pixel at all lands inside the front window");
    CHECK(differ_out == 0,
          "leg(T3): outside the front window the clipped repaint equals the "
          "unclipped one (the clip removes nothing visible)");
    render_ctx_free(&a);
    render_ctx_free(&b);
}

/* --- leg X: the INT 81h dispatcher itself (disk tenant lifecycle's oracle line) ------
 * docs/design/GUI-remediation-ADR-reconciliation.md disk tenant lifecycle *Oracle*: "host
 * property suite over the gate dispatcher (AX demux, frame-relative arg reads,
 * negative-error convention) + the TENANT-GATE trace". The callable-code list
 * below is HAND-COPIED from spec/toolbox_gate.h Sec 3/9-16 and spec/
 * cp_dialog.h (the rows marked V1..V4 plus the Sec 10-16 additions, and
 * CONTENT_SIZE 0x0024 of ); the
 * RESERVED / DISSOLVED rows (0x0002, 0x0010, 0x0021, 0x0032, 0x0051) are
 * deliberately absent -- Sec 3: "Every other code... returns TBX_ERR_BADCODE
 * (the demux is total, never a silent no-op)". */
static const uint16_t k_callable[] = {
    0x0001u, 0x0020u, 0x0022u, 0x0023u, 0x0024u, 0x0030u, 0x0031u, 0x0033u,
    0x0050u, 0x0052u, 0x0070u,
    0x0080u, 0x0081u, 0x0082u, 0x0083u, 0x0084u, 0x0085u, 0x0086u,
    0x0087u, 0x0088u,   /* Sec 10a FILE_FIND / DISK_FREE */
    0x0089u, 0x008Au, 0x008Bu,   /* Sec 10b FILE_RENAME / FILE_CREATE_NEW */
    0x0090u, 0x0091u, 0x0092u, 0x0093u, 0x0094u, 0x0095u, 0x0096u,
    0x00A0u, 0x00A1u, 0x00B0u, 0x00B1u,
    0x00C0u, 0x00C1u, 0x00C2u, 0x00C3u
};
#define POISON 0xA5A5A5A5u

static struct xres {
    uint32_t notreg_eax;                 /* NEWWINDOW before REGISTER        */
    uint32_t reserved_eax[5];            /* the five reserved/dissolved rows */
    long     sweep_codes, sweep_bad;     /* every other 16-bit AX            */
    uint32_t badtag_eax, outside_eax;    /* REGISTER refusals                */
    uint32_t reg_hiword_eax;             /* REGISTER with EAX = 0xBEEF0001   */
    uint32_t reg_again_eax;              /* a second REGISTER                */
    uint32_t badarg_eax;                 /* NEWWINDOW above the menu bands   */
    uint32_t win1_eax, win2_eax, win3_eax;
    char     win1_line[128];
    int      n_after_sweep, n_before_sweep;
    FlairApp *head_after_sweep;
} X;

static int callable(uint32_t ax)
{
    for (size_t i = 0; i < sizeof k_callable / sizeof k_callable[0]; i++)
        if (k_callable[i] == ax) return 1;
    return 0;
}

/* The last TENANT-GATE line on the serial trace, into `out`. */
static void last_gate_line(char *out, size_t cap)
{
    const char *p = NULL, *q = g_serial;
    size_t k = 0;
    while ((q = strstr(q, "TENANT-GATE ")) != NULL) { p = q; q++; }
    out[0] = '\0';
    if (p == NULL) return;
    while (p[k] != '\0' && p[k] != '\n' && k + 2 < cap) { out[k] = p[k]; k++; }
    out[k++] = '\n';
    out[k] = '\0';
}

static void script_gate(void)
{
    const uint32_t win[5] = { T_L, T_T, T_R, T_B, 1u };
    /* Operator 2026-11-13: one 20-row bar, then usable window rows. */
    const uint32_t lowwin[5] = { T_L, 19u, T_R, T_B, 1u };
    const uint32_t reclaimed[5] = { T_L, 20u, T_R, T_B, 1u };
    const uint16_t reserved[5] = { 0x0002u, 0x0010u, 0x0021u, 0x0032u, 0x0051u };
    uint32_t rec = g_base + IMG_REC, a[1];

    /* X4a: a window before registration. */
    X.notreg_eax = gate_raw(TBX_NEWWINDOW, 5u, win, 0u);

    /* X1a: the reserved / dissolved rows. */
    for (int i = 0; i < 5; i++)
        X.reserved_eax[i] = gate_raw(reserved[i], 0u, NULL, 0u);

    /* X1b: EVERY other 16-bit AX, args poisoned -- the demux is total. */
    X.n_before_sweep = g_n;
    for (uint32_t ax = 0u; ax <= 0xFFFFu; ax++) {
        if (callable(ax)) continue;
        X.sweep_codes++;
        if (gate_raw(ax, 0u, NULL, POISON) != (uint32_t)TBX_ERR_BADCODE) X.sweep_bad++;
    }
    X.n_after_sweep = g_n;
    X.head_after_sweep = W.plist.head;
    g_serial_n = 0; g_serial[0] = '\0';   /* the sweep's 65k lines are graded */

    /* X4b: REGISTER refusals (record tag, record outside the image). */
    put32(rec + FLAIR_TENANT_REC_TAG_OFF, 0x58585858u);
    a[0] = rec;
    X.badtag_eax = gate_raw(TBX_REGISTER, 1u, a, 0u);
    put32(rec + FLAIR_TENANT_REC_TAG_OFF, FLAIR_TENANT_REC_TAG);
    a[0] = g_base + IMG_LEN;                 /* one past the image */
    X.outside_eax = gate_raw(TBX_REGISTER, 1u, a, 0u);

    /* X2: AX is the LOW WORD of EAX -- garbage in the high word is ignored. */
    a[0] = rec;
    X.reg_hiword_eax = gate_raw(0xBEEF0000u | TBX_REGISTER, 1u, a, POISON);
    X.reg_again_eax = gate_raw(TBX_REGISTER, 1u, a, 0u);

    /* X4c / X3: a refused window, then two windows with the whole frame and
     * the stack above the five args poisoned, then the cap. */
    X.badarg_eax = gate_raw(TBX_NEWWINDOW, 5u, lowwin, 0u);
    X.win1_eax = gate_raw(TBX_NEWWINDOW, 5u, win, POISON);
    last_gate_line(X.win1_line, sizeof X.win1_line);
    X.win2_eax = gate_raw(TBX_NEWWINDOW, 5u, reclaimed, POISON);
    X.win3_eax = gate_raw(TBX_NEWWINDOW, 5u, win, POISON);
}

static void leg_gate(void)
{
    const uint32_t win[5] = { T_L, T_T, T_R, T_B, 1u };
    uint32_t stray;
    int wins_before;
    FlairApp *t;

    world_init();
    memset(&X, 0, sizeof X);
    TP.script = script_gate;
    (void)launch();
    t = tenant_app();

    CHECK(X.notreg_eax == 0xFFFFFFFEu,
          "leg(X4a): NEWWINDOW before REGISTER -> TBX_ERR_NOTREG, sign-extended "
          "in EAX (0xFFFFFFFE)");
    CHECK(X.reserved_eax[0] == 0xFFFFFFFFu && X.reserved_eax[1] == 0xFFFFFFFFu &&
          X.reserved_eax[2] == 0xFFFFFFFFu && X.reserved_eax[3] == 0xFFFFFFFFu &&
          X.reserved_eax[4] == 0xFFFFFFFFu,
          "leg(X1a): the reserved / DISSOLVED rows 0x0002 0x0010 0x0021 0x0032 "
          "0x0051 answer TBX_ERR_BADCODE");
    CHECK(X.sweep_codes == 65536L - (long)(sizeof k_callable / sizeof k_callable[0]) &&
          X.sweep_bad == 0,
          "leg(X1b): EVERY 16-bit AX outside the spec's callable rows answers "
          "TBX_ERR_BADCODE (the demux is total)");
    CHECK(X.n_after_sweep == X.n_before_sweep && X.head_after_sweep == &W.finder,
          "leg(X1c): ... and none of them delivered an event or touched the "
          "process list");
    CHECK(X.badtag_eax == 0xFFFFFFFAu && X.outside_eax == 0xFFFFFFFAu &&
          serial_has("TENANT-REGISTER-BAD why=tag\n") &&
          serial_has("TENANT-REGISTER-BAD why=rec-outside-image\n"),
          "leg(X4b): a lying record is refused TBX_ERR_BADREC and reported "
          "(TENANT-REGISTER-BAD why=tag / why=rec-outside-image)");
    CHECK(X.reg_hiword_eax == 0u && t != NULL &&
          serial_has("TENANT-GATE ax=0x0001 rec=+0x0000 -> 0\n"),
          "leg(X2): AX is the LOW WORD of EAX -- REGISTER with EAX=0xBEEF0001 "
          "registers (TENANT-GATE ax=0x0001 rec=+0x0000 -> 0)");
    CHECK(X.reg_again_eax == 0xFFFFFFFCu,
          "leg(X4c): a second REGISTER -> TBX_ERR_BUSY");
    CHECK(X.badarg_eax == 0xFFFFFFFDu,
          "leg(X4d): a window above the menu bands -> TBX_ERR_BADARG");
    CHECK(X.win1_eax == 1u && X.win2_eax == 2u &&
          strcmp(X.win1_line,
                 "TENANT-GATE ax=0x0020 l=340 t=100 r=600 b=320 goaway=1 -> 1\n") == 0,
          "leg(X3): with the frame and the stack above the args POISONED, NEWWINDOW "
          "reads exactly its five args from frame+68 (TENANT-GATE ax=0x0020 l=340 "
          "t=100 r=600 b=320 goaway=1 -> 1)");
    CHECK(X.win3_eax == 0xFFFFFFFCu,
          "leg(X4e): a third window past TBX_TENANT_WINDOWS -> TBX_ERR_BUSY");

    /* X5: a stray int 81h from KERNEL context (no tenant code executing). */
    wins_before = windows_in_zorder();
    stray = gate_raw(TBX_NEWWINDOW, 5u, win, 0u);
    CHECK(stray == 0xFFFFFFF9u && windows_in_zorder() == wins_before &&
          serial_has("TENANT-GATE ax=0x0020 l=340 t=100 r=600 b=320 goaway=1 -> -7\n"),
          "leg(X5): a stray int 81h with no tenant executing -> TBX_ERR_NOTTENANT, "
          "nothing created, still traced");
    service();
    CHECK(t != NULL && W.plist.head == t,
          "leg(X6): the scripted tenant is affirmed like any other once it returns");
}

/* SDK Sec 7 content-local rectangles. Literal captured document insets give
 * content [346,579) x [122,299). The reader subtracts that origin in a wide
 * domain, never repeats the gate's narrowing/addition. */
static uint32_t fill_pixels[FLAIR_SCREEN_W*FLAIR_SCREEN_H];
static void script_fill_geometry(void)
{
    uint32_t reg[]={g_base+IMG_REC};
    uint32_t win[]={T_L,T_T,T_R,T_B,1};
    uint32_t second[]={20,20,200,220,1};
    CHECK(gate(TBX_REGISTER,1,reg)==0 && gate(TBX_NEWWINDOW,5,win)==1,
          "raster probe registers and creates a real gate window");
    CHECK(gate(TBX_NEWWINDOW,5,second)==2,"second raster window has its own token");
    const int box[2][4]={{346,122,579,299},{26,42,179,199}};
    const struct { int32_t l,t,r,b,rc; } cases[]={
        {-65536,-65536,-65526,-65526,0},
        {65536,65536,65546,65546,0},
        {-1,-1,10,10,0},
        {INT32_MIN,INT32_MIN,INT32_MAX,INT32_MAX,0},
        {INT32_MIN,0,0,10,0},
        {231,175,INT32_MAX,INT32_MAX,0},
        {INT32_MAX-10,0,INT32_MAX,10,0},
        {10,10,0,0,TBX_ERR_BADARG}
    };
    for(unsigned token=1;token<=2;token++) for(unsigned c=0;c<sizeof cases/sizeof cases[0];c++) {
        for(unsigned i=0;i<sizeof fill_pixels/sizeof fill_pixels[0];i++) fill_pixels[i]=0xB2D3E5F7u;
        uint32_t a[]={token,(uint32_t)cases[c].l,(uint32_t)cases[c].t,
                      (uint32_t)cases[c].r,(uint32_t)cases[c].b,TBX_COLOR_BLACK};
        CHECK(gate(TBX_FILLRECT,6,a)==cases[c].rc,"wide rectangle result follows SDK validation");
        unsigned bad=0,painted=0;
        for(int y=0;y<FLAIR_SCREEN_H;y++) for(int x=0;x<FLAIR_SCREEN_W;x++) {
            int64_t lx=(int64_t)x-box[token-1][0],ly=(int64_t)y-box[token-1][1];
            int want=cases[c].rc==0 && x>=box[token-1][0] && x<box[token-1][2] &&
                     y>=box[token-1][1] && y<box[token-1][3] &&
                     lx>=cases[c].l && lx<cases[c].r && ly>=cases[c].t && ly<cases[c].b;
            bad += (fill_pixels[y*FLAIR_SCREEN_W+x]!=0xB2D3E5F7u)!=want;
            painted += want;
        }
        CHECK(!bad,"every changed pixel equals the independent wide-domain clipped set");
        int drew=tbx_take_drew();
        if(painted) CHECK(drew==1,"actual drawing requests presentation");
        CHECK(tbx_take_drew()==0,"presentation request is consumed once");
    }
}
static void leg_fill_geometry(void)
{
    static const bitmap_t bm={.base=(volatile uint8_t *)fill_pixels,
        .pitch=FLAIR_SCREEN_W*4,.bpp=32,.bytes_per_pixel=4,
        .width=FLAIR_SCREEN_W,.height=FLAIR_SCREEN_H};
    probe_surface=&bm; world_init(); TP.script=script_fill_geometry;
    CHECK(launch()==0,"raster tenant launches through the real dispatcher");
    probe_surface=0;
}

/* SDK Sec 5: every endpoint is in the screen domain before any difference.
 * Failed requests must preserve both window count and the records arena. */
static unsigned extreme_case;
static void script_window_extremes(void)
{
    static const int32_t invalid[][4]={
        {INT32_MAX-100,60,INT32_MIN+100,280},
        {20,INT32_MAX-100,300,INT32_MIN+100},
        {INT32_MIN,60,300,280},{20,INT32_MIN,300,280},
        {INT32_MAX,60,INT32_MAX,280},{20,INT32_MAX,300,INT32_MAX},
        {20,60,INT32_MIN,280},{20,60,300,INT32_MIN}
    };
    uint32_t reg[]={g_base+IMG_REC};
    CHECK(gate(TBX_REGISTER,1,reg)==0,"extreme-window tenant registers");
    FlairApp *app=tenant_app();
    uint32_t avail=flair_heap_avail(&app->records_arena);
    int windows=windows_in_zorder();
    uint32_t a[5];
    for(unsigned k=0;k<4;k++) a[k]=(uint32_t)invalid[extreme_case][k];
    a[4]=1;
    CHECK(gate(TBX_NEWWINDOW,5,a)==TBX_ERR_BADARG,"extreme or inverted window is refused");
    CHECK(flair_heap_avail(&app->records_arena)==avail && windows_in_zorder()==windows,
          "refusal consumes no record storage or window slot");
    const uint32_t good[]={20,40,200,180,1};
    CHECK(gate(TBX_NEWWINDOW,5,good)==1,"first valid window still receives token one");
}
static void leg_window_extremes(void)
{
    for(extreme_case=0;extreme_case<8;extreme_case++) {
        EXPECT_SUCCESS("wide window rejection returns without side effects", {
            world_init(); TP.script=script_window_extremes;
            CHECK(launch()==0,"extreme-window fixture launches");
        });
    }
}


/* CHECK with a formatted reason (the shared macro takes one string). */
#define CHECKF(cond, ...) do { char m_[512]; \
    snprintf(m_, sizeof m_, __VA_ARGS__); CHECK((cond), m_); } while (0)

/* ===========================================================================
 * 6. MENU BAR, EXIT, TEXT AND SETTINGS LEGS (initech-qpslo)
 *
 * The legacy emulator gates test-flair-app-launch-mutant and
 * test-flair-tenant-menu-mutant proved these verbs from screendumps; the host
 * forms below drive the same REAL dispatcher and grade literal values that
 * come from spec/toolbox_gate.h Sec 6a/8/9, the Geneva 12 [MEAS] widths and
 * spec/cpanels/record.json, never from tbxgate.c.
 * ===========================================================================*/

/* A MenuBar graph laid out by hand at `at` (spec Sec 9 offsets, literal):
 * one menu, ID 128, title "File", items "Open" and "Quit" (both enabled). */
#define MB_BAR   0x000u
#define MB_MENU  0x010u
#define MB_ITEMS 0x020u
#define MB_TITLE 0x040u
#define MB_OPEN  0x048u
#define MB_QUIT  0x050u
#define MB_LEN   0x060u
static void lay_menubar(uint32_t at)
{
    memset((void *)(uintptr_t)at, 0, MB_LEN);
    put32(at + MB_BAR + 0u, at + MB_MENU);                  /* menus @0     */
    *(volatile uint16_t *)(uintptr_t)(at + MB_BAR + 4u) = 1u; /* n_menus @4 */
    *(volatile uint16_t *)(uintptr_t)(at + MB_MENU + 0u) = 128u; /* menuID @0 */
    put32(at + MB_MENU + 4u, at + MB_TITLE);                /* title @4     */
    put32(at + MB_MENU + 8u, at + MB_ITEMS);                /* items @8     */
    *(volatile uint16_t *)(uintptr_t)(at + MB_MENU + 12u) = 2u; /* n_items @12 */
    put32(at + MB_ITEMS + 0u, at + MB_OPEN);                /* item 1 text  */
    *(volatile uint8_t *)(uintptr_t)(at + MB_ITEMS + 7u) = 1u;  /* enabled @7 */
    put32(at + MB_ITEMS + 12u, at + MB_QUIT);               /* item 2 text  */
    *(volatile uint8_t *)(uintptr_t)(at + MB_ITEMS + 12u + 7u) = 1u;
    memcpy((void *)(uintptr_t)(at + MB_TITLE), "File", 5);
    memcpy((void *)(uintptr_t)(at + MB_OPEN), "Open", 5);
    memcpy((void *)(uintptr_t)(at + MB_QUIT), "Quit", 5);
}

/* The same graph OUTSIDE the tenant image, in readable static storage (below
 * 4 GiB, -no-pie), so a missing image-range check would install it rather
 * than fault: the refusal is the only correct outcome. */
static uint8_t g_foreign_bar[MB_LEN] __attribute__((aligned(16)));

#define IMG_MBAR 0x100u      /* the in-image MenuBar graph (0x100..0x160) */
static struct mres {
    int32_t reg, win, outside_bar, outside_menus, set, draw;
    int     redraw_before_draw;
    MenuBar *installed_after_set;
} M;

static void script_menubar(void)
{
    uint32_t reg[1] = { g_base + IMG_REC };
    uint32_t win[5] = { T_L, T_T, T_R, T_B, 1u };
    uint32_t a[1];
    uint32_t inbar = g_base + IMG_MBAR;
    uint32_t foreign = (uint32_t)(uintptr_t)g_foreign_bar;

    M.reg = gate(TBX_REGISTER, 1u, reg);
    M.win = gate(TBX_NEWWINDOW, 5u, win);
    lay_menubar(foreign);
    a[0] = foreign;
    M.outside_bar = gate(TBX_SETMBAR, 1u, a);
    /* An in-image bar whose menus array lies outside the image. */
    lay_menubar(inbar);
    put32(inbar + MB_BAR + 0u, foreign + MB_MENU);
    a[0] = inbar;
    M.outside_menus = gate(TBX_SETMBAR, 1u, a);
    lay_menubar(inbar);
    M.set = gate(TBX_SETMBAR, 1u, a);
    M.installed_after_set = tenant_app() != NULL ? tenant_app()->menubar : NULL;
    M.redraw_before_draw = tbx_take_mbar_redraw();
    M.draw = gate(TBX_DRAWMENUBAR, 0u, NULL);
}

static void leg_menubar(void)
{
    FlairApp *t;
    MenuBar *want;
    int b;
    EventRecord src;

    world_init();
    memset(&M, 0, sizeof M);
    TP.script = script_menubar;
    CHECKF(launch() == TBX_OK, "leg(M0): the menu-bar tenant launches");
    t = tenant_app();
    want = (MenuBar *)(uintptr_t)(g_base + IMG_MBAR);
    CHECKF(M.reg == TBX_OK && M.win == 1, "leg(M0b): it registers and opens window 1");
    CHECKF(M.outside_bar == TBX_ERR_BADARG &&
          serial_has("TENANT-SETMBAR-BAD why=bar-outside-image\n"),
          "leg(M1): a MenuBar record outside the image is refused BADARG "
          "(spec Sec 9: the WHOLE graph lies in the image; why=bar-outside-image)");
    CHECKF(M.outside_menus == TBX_ERR_BADARG &&
          serial_has("TENANT-SETMBAR-BAD why=menus-outside-image\n"),
          "leg(M2): an in-image bar whose menus array is outside the image is "
          "refused BADARG (why=menus-outside-image)");
    CHECKF(M.set == TBX_OK && M.installed_after_set == want,
          "leg(M3): a valid in-image bar is INSTALLED: the app's menubar is the "
          "tenant's own record at load_base+0x100 (SetMenuBar, spec Sec 9)");
    CHECKF(M.redraw_before_draw == 0,
          "leg(M4): SETMBAR alone requests no band-2 redraw (SetMenuBar, not DrawMenuBar)");
    CHECKF(M.draw == TBX_OK && tbx_take_mbar_redraw() == 1 && tbx_take_mbar_redraw() == 0,
          "leg(M5): DRAWMENUBAR asks the pump for ONE band-2 redraw once the "
          "tenant returns (spec Sec 9 V3), consumed once");
    service();
    CHECKF(t != NULL && W.plist.head == t && t->menubar == want,
          "leg(M6): the affirmed foreground tenant keeps its installed bar");

    /* Sec 6a: a choice made in the tenant's OWN bar reaches its eventProc as
     * TBX_EVT_MENU with Message (menuID << 16) | item. Item 2 of menu 128. */
    memset(&src, 0, sizeof src);
    src.what = mouseDown; src.where.h = 30; src.where.v = 10;
    b = g_n;
    CHECKF(tbx_menu_choice(want, (128u << 16) | 2u, &src, "mouse") == 1,
          "leg(M7): the shell reports the tenant's own menu choice as delivered");
    CHECKF(g_n == b + 1 && g_log[b].who == WHO_T &&
          g_log[b].what == (int32_t)TBX_EVT_MENU &&
          g_log[b].msg == (int32_t)((128u << 16) | 2u) &&
          g_log[b].h == 30 && g_log[b].v == 10,
          "leg(M8): the tenant's eventProc reads TBX_EVT_MENU, message "
          "0x00800002 and the source where out of its own FlairEvent");
    CHECKF(serial_has("TENANT-MENU menu=128 item=2 src=mouse\n"),
          "leg(M9): the delivery is on the trace (TENANT-MENU menu=128 item=2 src=mouse)");
    b = g_n;
    CHECKF(tbx_menu_choice((const MenuBar *)(const void *)g_foreign_bar,
                          (128u << 16) | 2u, &src, "mouse") == 0 && g_n == b,
          "leg(M10): a choice made in a bar that is not the tenant's is not delivered");
}

/* --- leg R: EXIT returns the code block (the O-3 avail-stable invariant) -- */
static uint32_t heapavail_line(int nth)
{
    const char *p = g_serial;
    for (int i = 0; p != NULL; i++) {
        p = strstr(p, "TENANT-HEAPAVAIL n=");
        if (p == NULL) break;
        if (i == nth) return (uint32_t)strtoul(p + 19, NULL, 10);
        p++;
    }
    return 0xFFFFFFFFu;
}

/* heap.h: flair_heap_avail is the untouched bump tail; a freed block goes to
 * its class free list and the next request of that class and size reuses it.
 * So the first cycle may lower the tail once, and every later launch/exit
 * cycle must leave it exactly where the first left it (the legacy O-3
 * avail-stable invariant of test-flair-app-launch). */
static void leg_reclaim(void)
{
    enum { CYCLES = 3 };
    uint32_t avail0, avail[CYCLES];

    world_init();
    avail0 = flair_heap_avail(&W.master);
    for (int cycle = 0; cycle < CYCLES; cycle++) {
        memset(&TP, 0, sizeof TP);
        TP.do_register = 1;
        TP.do_newwindow = 1;
        TP.exit_on = 'q';
        CHECKF(launch() == TBX_OK, "leg(R0): launch cycle %d", cycle);
        service();
        CHECKF(tenant_app() != NULL && W.plist.head == tenant_app(),
               "leg(R1): the tenant is resident and foreground (cycle %d)", cycle);
        pump(mk(keyDown, 'q', T_PROBE_H, T_PROBE_V));
        avail[cycle] = flair_heap_avail(&W.master);
        CHECKF(tenant_app() == NULL && W.plist.head == &W.finder &&
               serial_has("TENANT-EXIT rc=7 via=exit\n"),
               "leg(R2): the tenant exited and was reaped (cycle %d)", cycle);
        CHECKF(heapavail_line(cycle) == avail[cycle],
               "leg(R3): TENANT-HEAPAVAIL reports the master heap's free tail "
               "(cycle %d: line %u, heap %u)", cycle,
               (unsigned)heapavail_line(cycle), (unsigned)avail[cycle]);
    }
    CHECKF(avail[0] <= avail0,
           "leg(R4): the first cycle may only lower the tail (%u -> %u)",
           (unsigned)avail0, (unsigned)avail[0]);
    for (int cycle = 1; cycle < CYCLES; cycle++)
        CHECKF(avail[cycle] == avail[0],
               "leg(R5): launch/exit cycle %d leaves the master heap exactly as "
               "cycle 0 did -- the code block and every tenant record are reused, "
               "never leaked (cycle 0 %u, now %u)", cycle,
               (unsigned)avail[0], (unsigned)avail[cycle]);
}

/* --- leg F: TEXTDRAW honours txFont/txSize (Geneva 12) -------------------- */
static uint32_t text_pixels[FLAIR_SCREEN_W * FLAIR_SCREEN_H];
#define TEXT_SENTINEL 0xB2D3E5F7u
static uint32_t g_text_fg;
static int32_t g_text_rc;
static void script_textdraw(void)
{
    uint32_t reg[1] = { g_base + IMG_REC };
    uint32_t win[5] = { T_L, T_T, T_R, T_B, 1u };
    uint32_t s = g_base + IMG_MBAR;
    memcpy((void *)(uintptr_t)s, "tables", 7);
    CHECKF(gate(TBX_REGISTER, 1u, reg) == 0 && gate(TBX_NEWWINDOW, 5u, win) == 1,
          "leg(F0): the text tenant registers and opens window 1");
    for (unsigned i = 0; i < sizeof text_pixels / sizeof text_pixels[0]; i++)
        text_pixels[i] = TEXT_SENTINEL;
    {
        uint32_t a[6] = { 1u, 10u, 10u, s, g_text_fg, TBX_COLOR_GRAY };
        g_text_rc = gate(TBX_TEXTDRAW, 6u, a);
    }
}

/* The opaque run box: bbox of every pixel the call changed. */
static void changed_box(int *x0, int *y0, int *x1, int *y1, long *n)
{
    *x0 = FLAIR_SCREEN_W; *y0 = FLAIR_SCREEN_H; *x1 = -1; *y1 = -1; *n = 0;
    for (int y = 0; y < FLAIR_SCREEN_H; y++)
        for (int x = 0; x < FLAIR_SCREEN_W; x++)
            if (text_pixels[y * FLAIR_SCREEN_W + x] != TEXT_SENTINEL) {
                if (x < *x0) *x0 = x;
                if (y < *y0) *y0 = y;
                if (x > *x1) *x1 = x;
                if (y > *y1) *y1 = y;
                (*n)++;
            }
}

static void leg_geneva(void)
{
    static const bitmap_t bm = { .base = (volatile uint8_t *)text_pixels,
        .pitch = FLAIR_SCREEN_W * 4, .bpp = 32, .bytes_per_pixel = 4,
        .width = FLAIR_SCREEN_W, .height = FLAIR_SCREEN_H };
    int x0, y0, x1, y1;
    long n;

    probe_surface = &bm;
    world_init();
    TP.script = script_textdraw;
    g_text_fg = TBX_COLOR_BLACK | TBX_TEXT_GENEVA12;
    CHECKF(launch() == TBX_OK && g_text_rc == TBX_OK,
          "leg(F1): TEXTDRAW with applFont at txSize 12 is accepted");
    changed_box(&x0, &y0, &x1, &y1, &n);
    /* Content origin (346,122) as leg fill_geometry; the run starts at local
     * (10,10). Geneva 12 "tables" is 39 px wide ([MEAS] SimpleText capture,
     * specs/sys8/simpletext-stdfile.md Sec 1.2) and the line is 16 rows
     * (12 + 3 + 1, the same capture); the run box is opaque. */
    CHECKF(x0 == 356 && y0 == 132 && x1 - x0 + 1 == 39 && y1 - y0 + 1 == 16 &&
          n == 39L * 16L,
          "leg(F2): the Geneva 12 run box is exactly the measured 39 x 16 at "
          "(356,132) (got x[%d,%d] y[%d,%d], %ld px)", x0, x1, y0, y1, n);
    probe_surface = NULL;
}

/* --- leg U: DRAWCELLS underline row (spec Sec 7a STYLE) ------------------- */
/* A hand-authored strike: glyph 'A' is a 2-px stem on rows 2..11 (the ROM
 * strike's capitals end on row 11, spec Sec 7a), every other glyph blank. */
static uint8_t g_cellfont[256u * 16u];
static int32_t g_cells_rc[2];
static void script_cells(void)
{
    uint32_t reg[1] = { g_base + IMG_REC };
    uint32_t win[5] = { T_L, T_T, T_R, T_B, 1u };
    uint32_t s = g_base + IMG_MBAR;
    memcpy((void *)(uintptr_t)s, "A A", 4);
    CHECKF(gate(TBX_REGISTER, 1u, reg) == 0 && gate(TBX_NEWWINDOW, 5u, win) == 1,
          "leg(U0): the cell tenant registers and opens window 1");
    for (unsigned i = 0; i < sizeof text_pixels / sizeof text_pixels[0]; i++)
        text_pixels[i] = TEXT_SENTINEL;
    {
        uint32_t plain[6] = { 1u, 8u, 8u, s, TBX_COLOR_BLACK, TBX_COLOR_GRAY };
        uint32_t ul[6] = { 1u, 8u, 40u, s, TBX_COLOR_BLACK | TBX_TEXT_UNDERLINE,
                           TBX_COLOR_GRAY };
        g_cells_rc[0] = gate(TBX_DRAWCELLS, 6u, plain);
        g_cells_rc[1] = gate(TBX_DRAWCELLS, 6u, ul);
    }
}

static void leg_cells(void)
{
    static const bitmap_t bm = { .base = (volatile uint8_t *)text_pixels,
        .pitch = FLAIR_SCREEN_W * 4, .bpp = 32, .bytes_per_pixel = 4,
        .width = FLAIR_SCREEN_W, .height = FLAIR_SCREEN_H };
    /* Global cell tops: content origin (346,122) + local (8,8) / (8,40). */
    const int gx = 354, yplain = 130, yul = 162, ncols = 24;
    uint32_t ink, paper;
    int last_ink_row = -1, bad13 = 0, bad_other = 0, ul_row = -1, ul_rows = 0;

    memset(g_cellfont, 0, sizeof g_cellfont);
    for (int r = 2; r <= 11; r++) g_cellfont['A' * 16 + r] = 0x18u;
    probe_surface = &bm;
    probe_cellfont = g_cellfont;
    world_init();
    TP.script = script_cells;
    CHECKF(launch() == TBX_OK && g_cells_rc[0] == TBX_OK && g_cells_rc[1] == TBX_OK,
          "leg(U1): plain and underlined DRAWCELLS runs are accepted");
    ink = text_pixels[(yplain + 5) * FLAIR_SCREEN_W + gx + 3];
    paper = text_pixels[yplain * FLAIR_SCREEN_W + gx];
    CHECKF(ink != paper && ink != TEXT_SENTINEL && paper != TEXT_SENTINEL,
          "leg(U2): the plain run inks the stem in fg on an opaque bg cell");
    /* Measure B on the plain run (the legacy checker's leg BASELINE). */
    for (int r = 0; r < 16; r++)
        for (int c = 0; c < ncols; c++)
            if (text_pixels[(yplain + r) * FLAIR_SCREEN_W + gx + c] == ink) last_ink_row = r;
    CHECKF(last_ink_row == 11,
          "leg(U3): the plain letters end on row 11, so the baseline row B is 12");
    /* UNDERLINE: one 1-px line on row B + 1 = 13 under the whole run; no ink
     * of the strike touches row 13, so no halo gap. */
    for (int r = 12; r < 16; r++) {
        int full = 1;
        for (int c = 0; c < ncols; c++) {
            uint32_t px = text_pixels[(yul + r) * FLAIR_SCREEN_W + gx + c];
            if (px != ink) full = 0;
            if (r == 13 && px != ink) bad13++;
            if (r != 13 && px != paper) bad_other++;
        }
        if (full) { ul_row = r; ul_rows++; }
    }
    CHECKF(bad13 == 0 && bad_other == 0 && ul_rows == 1 && ul_row == 13,
          "leg(U4): the underline is the single row B + 1 = 13 of the cell, "
          "across all 24 columns; rows 12, 14 and 15 stay bg (found row %d, "
          "%d rows; %d gaps on row 13, %d stray)", ul_row, ul_rows, bad13, bad_other);
    probe_surface = NULL;
    probe_cellfont = NULL;
}

/* --- leg K: Control Panels SETTINGS applies Mouse live -------------------- */
/* spec/cpanels/record.json: "CP01", version 1, pattern, tracking (0 slow,
 * 1 normal, 2 fast), double click (0 slow 40 ticks, 1 normal 20 ticks,
 * 2 fast 10 ticks), sound, volume, clock, reserved 0 x3, sum of bytes 0..13
 * little endian. Literal: pattern 0, tracking 2, double 2, sound 0,
 * volume 2, clock 1. Sum = 67+80+48+49+1+0+2+2+0+2+1 = 252. */
static const uint8_t k_cp_fast[16] = {
    'C','P','0','1', 1, 0, 2, 2, 0, 2, 1, 0, 0, 0, 252, 0
};
static int32_t g_settings_rc;
extern unsigned routing_desktop_pattern, routing_desktop_pattern_calls;
extern int routing_menu_clock;
static void script_settings(void)
{
    uint32_t reg[1] = { g_base + IMG_REC };
    uint32_t rec = g_base + IMG_MBAR;
    memcpy((void *)(uintptr_t)rec, k_cp_fast, sizeof k_cp_fast);
    CHECKF(gate(TBX_REGISTER, 1u, reg) == 0, "leg(K0): the panel tenant registers");
    {
        uint32_t a[2] = { 1u, rec };
        g_settings_rc = gate(TBX_SETTINGS, 2u, a);
    }
}

/* One mouseDown after a +10 px horizontal delta from the centre: its where.h. */
static int click_after_delta(void)
{
    flair_raw_ring_t ring;
    flair_raw_event_t raw = { FLAIR_RAW_MOUSE, 0u,
                              0x01u | ((uint32_t)(uint8_t)(int8_t)10 << 8) };
    flair_raw_event_t up = { FLAIR_RAW_MOUSE, 1u, 0u };
    EventRecord ev;
    int h = -1;
    flair_event_init(&ring);
    (void)flair_raw_post(&ring, &raw);
    (void)flair_raw_post(&ring, &up);
    if (GetNextEvent(&ring, everyEvent, &ev) && ev.what == mouseDown) h = ev.where.h;
    (void)GetNextEvent(&ring, everyEvent, &ev);
    return h;
}

static void leg_settings(void)
{
    unsigned saved_tracking = flair_mouse_tracking, saved_ticks = flair_double_click_ticks;
    int normal_h, fast_h;

    flair_mouse_tracking = 1u;
    normal_h = click_after_delta();
    CHECKF(normal_h == FLAIR_SCREEN_W / 2 + 10,
          "leg(K1): normal tracking is 1:1 -- a +10 delta from the centre clicks at "
          "x=330 (got %d)", normal_h);
    world_init();
    TP.script = script_settings;
    routing_desktop_pattern_calls = 0u;
    CHECKF(launch() == TBX_OK && g_settings_rc == 0,
          "leg(K2): SETTINGS write of a valid CP01 record succeeds");
    CHECKF(serial_has("CPANELS-APPLY pattern=0 tracking=2 double=2 ticks=10\n"),
          "leg(K3): the applied trace reports the fast interval of 10 ticks "
          "(record.json byte 7 value 2)");
    CHECKF(flair_double_click_ticks == 10u,
          "leg(K4): the Event/Finder double-click interval is live at 10 ticks "
          "(got %u)", flair_double_click_ticks);
    fast_h = click_after_delta();
    CHECKF(fast_h > normal_h,
          "leg(K5): fast tracking is live: the same +10 delta moves the cursor "
          "farther than normal (normal x=%d, fast x=%d)", normal_h, fast_h);
    CHECKF(routing_desktop_pattern_calls == 1u && routing_desktop_pattern == 0u &&
          routing_menu_clock == 1,
          "leg(K6): the pattern and menu clock of the record are applied once");
    flair_mouse_tracking = saved_tracking;
    flair_double_click_ticks = saved_ticks;
}

int main(void)
{
    FlairApp *t;

    scene_checks();
    leg_admit();
    t = leg_affirm();
    if (t != NULL && t->windows != NULL) {
        leg_route(t);
        leg_launchkey();
        leg_busy(t);
        /* A failed singleton check leaves the tenant graph unfit for EXIT.
         * Finish its assertions instead of dispatching into that graph. */
        if (g_fails) return TEST_SUMMARY("test-flair-routing");
        leg_exit();
    } else {
        CHECK(0, "leg(B) produced no foreground tenant -- legs C/G/D not run");
    }
    leg_crash();
    leg_noreg();
    leg_panic();
    leg_title();
    leg_gate();
    leg_fill_geometry();
    leg_window_extremes();
    leg_menubar();
    leg_reclaim();
    leg_geneva();
    leg_cells();
    leg_settings();
    return TEST_SUMMARY("test-flair-routing");
}
