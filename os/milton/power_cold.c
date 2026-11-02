/* Ref: ADR-0014 DEC-14-8 ("Shutdown, exit, power: once per session") and
 * DEC-14-10 ("Then: ... shutdown and exit"); bead initech-qshpn.
 * Special > Restart / Shut Down / Exit to InitechDOS, section COLD of
 * C:\FLAIR.OVL. Moved verbatim from kmain.c (behaviour, markers and mutant
 * knobs unchanged); each knob's mutant kernel rebuilds THIS object with its
 * own -D (harness/proptest/power_overlay.mk), so a mutant perturbs the body it
 * names. What stays resident (DEC-14-6): the desktop's IRQ hook teardown and
 * console reset (flair_exit_teardown: an overlay never imports a hook
 * setter, DEC-14-6.5),
 * the bounded-life halt (finder_shutdown_life_check: its tick and the
 * FLAIR_LIVE_INTERACTIVE build switch are kmain's), the locked banner lines
 * (exitdos_banner), the private boot-volume read shared with the overlay
 * manager (boot_root_find), and every code pointer stored in resident data
 * (int21_exit_hook, loader_exec_by_name, the IDT and absolute-disk thunks:
 * resident functions, so no resident word ever points into the window).
 * Reached only from finder_outcome_run (COLD) through the event pump. */
#include "finder_actions.h"
#include "power_cold.h"
#include "tbxgate.h"
#include "power.h"
#include "power_notice.h"
#include "console.h"
#include "int21.h"
#include "kbd.h"
#include "transient.h"
#include "memory_map.h"
#include "dos_messages.h"
#include "color_canon.h"

/* AH=09h DISPLAY STRING through a literal int 0x21, as kmain.c dos_puts. */
static inline void dos_puts(const char *dollar_terminated)
{
    __asm__ __volatile__(
        "int $0x21"
        :
        : "a"(0x0900u), "d"((uint32_t)(uintptr_t)dollar_terminated)
        : "cc", "memory");
}

static rgn_rect_t fta_rect(int l, int t, int r, int b)
{
    rgn_rect_t x;
    x.left = (int16_t)l; x.top = (int16_t)t; x.right = (int16_t)r; x.bottom = (int16_t)b;
    return x;
}

/* ---------------------------------------------------------------------------
 * bead initech-tdnl.78: SPECIAL > RESTART AND SPECIAL > SHUT DOWN.
 *
 * THE BEHAVIOUR (Law 1):
 *   - both "first sav[e] any necessary information" (Restart / Shut Down,
 *     Apple_Macintosh_System_Software_Users_Guide_V6.0.pdf PDF p. 141, book
 *     p. 121) -- "put everything away safely and update directory
 *     information" (same guide PDF p. 222, book p. 202). The Finder shell
 *     has already noted every open window's view (finder_windows.c
 *     fw_do_power); here the kernel commits DESKTOP.DB. That is the ONLY
 *     deferred state: the FAT backend commits every write before it returns
 *     (os/milton/fileio_fat.c "commits to disk per call (no deferred
 *     flush)", and its CLOSE is a no-op for that reason). The Trash is NOT
 *     emptied (System 7: Apple_Whats_New_In_System_7_Z030-3935-A.pdf PDF
 *     p. 23). Running disk applications are first asked to quit, and one
 *     that declines cancels the whole command (finder_power_ask_apps).
 *   - Restart then restarts the machine: the 8042 reset pulse
 *     (os/milton/power.h kernel_reset); a pulse that does not reset is a
 *     PANIC with the controller status, never a silent hang (Rule 2).
 *   - Shut Down, on a machine with no software power switch (a PC of the
 *     period, like the Macintosh Plus / SE): "the screen will become dark,
 *     and a dialog box will appear with a Restart button" (V6 guide PDF
 *     p. 215, book p. 195; "click the Restart button on the screen", the same
 *     page) -- the Macintosh User's Guide for desktop Macintosh computers
 *     (Z030-1751-A, PDF p. 25, book p. 6): switch off "when a message on
 *     screen indicates that it's safe to do so".
 *
 * THE SCREEN -- the System Error Handler's alert table, resource 'DSAT' 2 of
 * the System file on the System 7.0.1 boot disk (../system7-decomp/goldens/
 * work/boot701.dsk; System 7.5.3's boot753.img carries the same entries).
 * Alert 20001, the switch-off alert with a Restart button:
 *   text 0xB176 "It is now safe to switch off your Macintosh./If you wish to
 *        continue using your Macintosh,/press Restart." ('/' = new line),
 *        pen (v 94, h 114);
 *   icon 0xBFF1, a 32x32 ICON at rect (81,60)-(113,92) (the bytes below);
 *   buttons 0xAA0F, ONE button: string 0xA628 "Restart" at rect
 *        (154,404)-(174,463).
 * Those are 512x342 (compact Macintosh) screen coordinates; this 640x480
 * screen centres that frame, (+64,+69).
 * STATED DEVIATIONS:
 *   - "Macintosh" -> "computer": this is not a Macintosh; FLAIR is Initech's
 *     own platform (ADR-0012), and the house rule prefers the generic
 *     designation (ADR-0003 DEC-12; the Finder already says "About This
 *     Computer"). The rest of the text is verbatim.
 *   - the BOX rectangle is AUTHORED: the DSAT draws into the low-memory
 *     DSAlertRect, which the resource does not carry (no local reference).
 *     (96,133)..(546,259) = DSAT-space (32,64)..(482,190), enclosing the
 *     three DSAT elements.
 *   - the frame and face are the existing dBoxProc alert's (the Empty Trash
 *     confirm's, Platinum face; the Platinum alert frame is bead 81ft's).
 *   - line pitch 16 = the Chicago 12 cell (CHICAGO_CELL_H); the DSAT carries
 *     only the first pen position (baseline v 94 -> cell top 94 - 12 + 69).
 * Returns 1 when the Restart button (or Return, the default item) was
 * chosen; in a BOUNDED image the screen holds until the image's life ends
 * and then halts (kernel_halt) -- the interactive image waits forever, which
 * is what a switched-off-but-not-yet-unplugged machine does.
 * MUTANT KMAIN_MUT_SHUTDOWN_RETURNS (Rule 6; test-flair-shutdown-mutant):
 * returns 0 straight after the screen is up, so the desktop takes input
 * again. NEVER in a real build.
 * ------------------------------------------------------------------------- */
#define FSD_DX       64
#define FSD_DY       69
#define FSD_L        (32 + FSD_DX)
#define FSD_T        (64 + FSD_DY)
#define FSD_R        (482 + FSD_DX)
#define FSD_B        (190 + FSD_DY)
#define FSD_ICON_X   (60 + FSD_DX)
#define FSD_ICON_Y   (81 + FSD_DY)
#define FSD_TEXT_X   (114 + FSD_DX)
#define FSD_TEXT_Y   (94 - 12 + FSD_DY)
#define FSD_BTN_L    (404 + FSD_DX)
#define FSD_BTN_T    (154 + FSD_DY)
#define FSD_BTN_R    (463 + FSD_DX)
#define FSD_BTN_B    (174 + FSD_DY)

/* DSAT 2 ICON 0xBFF1, 32 rows x 4 bytes, MSB = leftmost pixel, 1 = ink. */
static const uint8_t FSD_ICON[128] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x7F, 0xFF, 0xFF,
    0x80, 0x7F, 0xFF, 0xFF, 0x80, 0x7F, 0xFF, 0xFF,
    0x80, 0x7F, 0xFF, 0xFF, 0x80, 0x7F, 0xC0, 0xFF,
    0x88, 0x7F, 0x00, 0x3F, 0x88, 0x7E, 0x00, 0x1F,
    0x88, 0x7C, 0x00, 0x0F, 0x80, 0x78, 0x00, 0x07,
    0x80, 0x78, 0x00, 0x07, 0x80, 0x70, 0x00, 0x03,
    0x80, 0x71, 0xDD, 0xC3, 0x80, 0x70, 0x00, 0x03,
    0x80, 0x70, 0x00, 0x03, 0x80, 0x71, 0xDD, 0x43,
    0x80, 0x70, 0x00, 0x03, 0x80, 0x70, 0x00, 0x03,
    0x80, 0x71, 0xD7, 0x03, 0x80, 0x70, 0x00, 0x03,
    0x87, 0xF0, 0x00, 0x03, 0x81, 0xF1, 0xEE, 0xC3,
    0x81, 0xF0, 0x00, 0x07, 0x81, 0xF0, 0x00, 0x07,
    0x81, 0xF0, 0x00, 0x0F, 0x81, 0xE0, 0x00, 0x1F,
    0x8F, 0x80, 0x00, 0x7F, 0x81, 0xFF, 0xFF, 0xFF,
    0x81, 0xFF, 0xFF, 0xFF, 0x81, 0xFF, 0xFF, 0xFF,
    0x81, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static __attribute__((cold,noinline)) int finder_shutdown_screen(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    static const char *const text[3] = {
        "It is now safe to switch off your computer.",
        "If you wish to continue using your computer,",
        "press Restart."
    };
    static DialogRecord     dr;
    static DialogItem       it[4];
    static ControlRecord    restart_c;
    static finder_win_rgn_t rs, rc, ru;
    const bitmap_t *off = &ctx->off;
    uint16_t hit = 0u;
    int k;

    /* the screen becomes dark */
    for (uint32_t y = 0u; y < off->height; y++)
        surface_fill_span(off, 0u, y, off->width, (uint32_t)CIDX_BLACK);

    control_init(&restart_c, pushButton,
                 fta_rect(FSD_BTN_L, FSD_BTN_T, FSD_BTN_R, FSD_BTN_B),
                 0, 0, 1, 1, "Restart");
    for (k = 0; k < 4; k++) {
        it[k].type    = (k == 0) ? ctrlItem : statText;
        it[k].ctrl    = (k == 0) ? &restart_c : (ControlRecord *)0;
        it[k].rect    = (k == 0) ? restart_c.contrlRect
                                 : fta_rect(FSD_TEXT_X, FSD_TEXT_Y + 16 * (k - 1),
                                            FSD_BTN_R, FSD_TEXT_Y + 16 * k);
        it[k].text    = (k == 0) ? (const char *)0 : text[k - 1];
        it[k].enabled = (uint8_t)((k == 0) ? 1 : 0);
        it[k]._pad[0] = it[k]._pad[1] = it[k]._pad[2] = 0u;
    }
    fta_rgn(&rs); fta_rgn(&rc); fta_rgn(&ru);
    (void)NewDialog(&dr, fta_rect(FSD_L, FSD_T, FSD_R, FSD_B), "", it, 4u,
                    1u /* Restart = default = Return */, 0u,
                    (WindowMgr *)0, &rs.r, &rc.r, &ru.r);
    dr.window.port.portBits.bm = ctx->off;
    dr.window.port.visRgn  = (region_t *)0;
    dr.window.port.clipRgn = (region_t *)0;
    dr.window.port.portRect = region_get_bbox(dr.window.strucRgn);
    DrawDialog(&dr);
    /* the DSAT icon, srcCopy (ink black, clear white) */
    for (int r = 0; r < 32; r++)
        for (int c = 0; c < 32; c++)
            surface_put_pixel(off,
                              (uint32_t)(FSD_ICON_Y + r) * off->pitch +
                              (uint32_t)(FSD_ICON_X + c) * off->bytes_per_pixel,
                              (FSD_ICON[r * 4 + (c >> 3)] & (0x80u >> (c & 7)))
                                  ? (uint32_t)CIDX_BLACK : (uint32_t)CIDX_WHITE);
    flair_desktop_present(bi, &ctx->off);
    serial_puts("FLAIR-SHUTDOWN-SAFE\n");
#if defined(KMAIN_MUT_SHUTDOWN_RETURNS)
    return 0;
#endif

    for (;;) {
        EventRecord ev;
        int g = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &ev, 3u);
        flair_live_cursor_track(&ev);
        if (g) {
            flair_live_emit_evt(&ev);
            if (DialogHandleEvent(&dr, &ev, (dialog_filter_fn)0, &hit) &&
                hit == 1u)
                return 1;
        }
        finder_shutdown_life_check();   /* resident: kmain owns the life */
    }
}

/* The reset itself: never returns. A reset that does not land is a PANIC with
 * the 8042 status (0x100 set = the input buffer never drained), Rule 2. */
static void finder_restart_now(void)
{
    uint32_t st;

    serial_puts("FLAIR-RESTART\n");
    st = kernel_reset();
    serial_puts("PANIC kernel_reset: the 8042 pulse did not reset the machine st=");
    serial_puthex32(st);
    serial_puts("\nHALTED\n");
    kernel_halt();
}

/* FIRST, THE RUNNING APPLICATIONS ARE ASKED TO QUIT (System 7: the Finder
 * sends each the Quit Application event on Restart / Shut Down, Inside
 * Macintosh Vol VI PDF pp. 340-341 / 345; the application answers it with
 * "the same routine" as its File > Quit, which saves documents or returns
 * userCanceledErr when "the Quit request was canceled by the user", PDF
 * p. 345, Listing 6-8). A cancel cancels the shut-down (PM directive
 * 2026-10-15, with the Initech 123 exit guard initech-fvnj). The one
 * resident disk tenant hears its OWN File > Quit (tbxgate.c tbx_ask_quit) and
 * the ordinary tenant service runs: it may EXIT (reaped), ask its question
 * (on a tree with TBX_ALERT the service runs the alert and delivers the
 * answer), or decline. Still resident afterwards == declined: the shut-down
 * is CANCELLED, loudly, and the tenant is NEVER terminated (its work would
 * be lost). Returns 1 to go on. The kernel-linked demo tenants (HELLO, NOTES)
 * hold no documents and are not asked: the reset or the halt ends them.
 * MUTANT KMAIN_MUT_POWER_NO_ASK (Rule 6; test-flair-shutdown-mutant): the
 * application is not asked. NEVER in a real build. */
static __attribute__((cold,noinline)) int finder_power_ask_apps(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    const char *still;
    int r;

#if defined(KMAIN_MUT_POWER_NO_ASK)
    return 1;
#endif
    r = tbx_ask_quit();
    if (r == 0) return 1;                  /* no application running          */
    if (r > 0) flair_live_tenant_heard(ctx, bi);
    still = tbx_resident_name();
    if (still == (const char *)0) return 1;
    serial_puts("FINDER-POWER-CANCEL app=");
    serial_puts(still);
    serial_puts(r > 0 ? " declined\n" : " cannot-ask\n");
    /* B04 / audit DAS L022: serial alone leaves the user guessing. The
     * notice is modal, restores the desktop, and never terminates the app. */
    {
        static const char *const ok[] = { "OK" };
        const char *lines[3];
        unsigned n = finder_power_notice(r > 0, still, lines);
        if (n) {
            flair_live_alert_open(ctx, bi, lines, (int)n, ok, 1, 1);
            serial_puts("FINDER-POWER-NOTICE\n");
            for (unsigned i = 0; i < n; i++) {
                serial_puts("FINDER-ALERT-LINE ");
                serial_puts(lines[i]); serial_putc('\n');
            }
            (void)flair_live_alert_run(ctx, bi);
        }
    }
    return 0;
}

/* Special > Restart / Shut Down after the shell noted the open views
 * (`noted`, finder_windows.c fw_do_power): ask the applications to quit,
 * commit, then reset or shut down.
 * MUTANT KMAIN_MUT_POWER_NO_FLUSH (Rule 6; test-flair-shutdown-mutant): the
 * DESKTOP.DB commit is skipped. NEVER in a real build. */
void finder_power(flair_live_ctx_t *ctx, const boot_info_t *bi,
                         finder_cmd_id id, int16_t noted)
{
    serial_puts(id == FCMD_RESTART ? "FINDER-POWER name=RESTART views="
                                   : "FINDER-POWER name=SHUTDOWN views=");
    serial_puti((int32_t)noted);
    serial_putc('\n');
    if (!finder_power_ask_apps(ctx, bi))
        return;                         /* an application cancelled          */
#if !defined(KMAIN_MUT_POWER_NO_FLUSH)
    g_finder_db_dirty = 1;
    finder_desk_persist();
#endif
    if (id == FCMD_SHUTDOWN && !finder_shutdown_screen(ctx, bi))
        return;                         /* only KMAIN_MUT_SHUTDOWN_RETURNS */
    finder_restart_now();
}

/* ---------------------------------------------------------------------------
 * bead initech-3p1a: SPECIAL > EXIT TO InitechDOS -- the chimera's own way out.
 *
 * THE BEHAVIOUR (Law 1):
 *   - The Macintosh leaves the Finder only by Restart / Shut Down (above;
 *     tdnl.78 graded against ../system7-decomp). InitechOS runs OVER
 *     InitechDOS (PRD Sec 1/4: the DOS-3.3 personality fused with the
 *     Toolbox), so it also offers the way back DOWN to the prompt, the way a
 *     period GUI shell over DOS did. The TPS Report Generator runs ONLY from
 *     that prompt (operator 2026-06-14 / 2026-10-16, bead initech-8479.1).
 *   - It asks first, in the existing alert (flair_live_alert_open: the
 *     s8_alert_modal.png geometry, default ringed, Escape = Cancel), with the
 *     operator's words "Are you sure you want to exit to InitechDOS?" --
 *     Exit (default, Return) / Cancel (Escape).
 *   - Exit puts everything away through the SAME path Shut Down uses
 *     (finder_power_ask_apps: a running disk application is asked to quit
 *     and may cancel; then the DESKTOP.DB commit), then tears the desktop
 *     down -- the pump's tick / keyboard / mouse hooks are taken out and the
 *     Toolbox fault triage is unbound -- and hands the SAME 640x480 LFB back
 *     to the InitechDOS text console the boot path built (there is no BIOS
 *     text mode to return to: the console is the VGA 8x16 font blitted into
 *     the LFB, os/milton/console.h). The console is RE-INITIALISED from
 *     boot_info and must come back CONSOLE_OK, or the hand-off PANICs (Rule
 *     2: a known console state or nothing).
 *   - The InitechDOS banner (ADR-0003 Appendix D.1, the locked
 *     spec/dos_banner.txt lines) is printed through INT 21h, then COMMAND.COM
 *     takes the console as the PERMANENT command processor (/P) with the
 *     flagship data volume, already mounted and bound, as drive A:.
 *   - COMMAND.COM is the TRANSIENT (os/milton/transient.h): read from the boot
 *     disk now, validated, never trusted blindly. A missing or foreign
 *     transient is ADR-0003's MSG-DOS-0014 on the console and a PANIC on
 *     serial -- the machine halts, as DOS did.
 * Serial: FINDER-EXIT-DOS-ALERT (alert up) ; FINDER-EXIT-DOS-CANCEL |
 *   FINDER-POWER name=EXITDOS views=<n> ; DESKTOP-DB-SAVE ... ;
 *   FLAIR-EXIT-TO-DOS ; COMMAND-TRANSIENT-OK bytes=<n> ; SHELL-READY.
 * MUTANT KMAIN_MUT_EXIT_NO_TEARDOWN (Rule 6; test-flair-exit-to-dos-mutant):
 * goes straight to the prompt WITHOUT the teardown and the console reset --
 * the desktop stays painted under the prompt. NEVER in a real build.
 * ------------------------------------------------------------------------- */
static __attribute__((cold,noinline)) void exitdos_fail(const char *why, uint32_t v)
{
    /* Initial interpreter activation: MS-DOS 3.3 User's Guide printed p310
     * (PDF395), ADR-0003 Appendix C/DEC-13. The post-EXEC overwrite/retry
     * message on printed p338 is a different path. Halt is Rule 2 policy. */
#ifndef KMAIN_MUT_COMMAND_MESSAGE
    dos_puts("\n" MSG_DOS_0014 "\n$");
#else
    dos_puts("\n" MSG_DOS_0002 "\n$");
#endif
    serial_puts("PANIC exit-to-dos: ");
    serial_puts(why);
    serial_puts(" v=");
    serial_puthex32(v);
    serial_puts("\nHALTED\n");
    kernel_halt();
}

/* Read + validate the transient. Returns its entry; never returns on failure. */
static void (*exitdos_load_transient(void))(int)
{
    dir_entry_t command;
    transient_hdr_t *h = (transient_hdr_t *)(uintptr_t)TRANSIENT_BASE;
    /* EVERY kernel import the transient's code uses, from the GENERATED table
     * (build/transient_want.asm; os/milton/transient.h). The review of
     * initech-13ig found the former hand-kept list checked five of ten. */
    const uint32_t n_want = transient_import_count;
    const uint32_t hdr_bytes = (uint32_t)sizeof *h + n_want * 4u;
    uint32_t bytes, k, v;
    int rc;

    if (kernel_image_end() > TRANSIENT_BASE)
        exitdos_fail("the kernel overlaps the transient window",
                     kernel_image_end());
#ifdef KMAIN_MUT_COMMAND_VOLUME
    g_boot_slave = 1;
#endif
    rc = boot_root_find("COMMAND.COM", &command, &v);
    if (rc == 1) exitdos_fail("boot FAT mount failed", v);
    if (rc == 2) exitdos_fail("boot FAT cluster too large", v);
    if (rc != 0) exitdos_fail("COMMAND.COM not found", v);
    if ((command.attribute & (DIR_ATTR_DIRECTORY | DIR_ATTR_VOLLABEL)) != 0u ||
        command.file_size <= sizeof *h ||
        command.file_size > TRANSIENT_CEIL - TRANSIENT_BASE)
        exitdos_fail("COMMAND.COM file extents", command.file_size);
    rc = fat12_read_partial(&g_boot_vol, 0, 0u, &command, 0u, sizeof *h,
                           h, g_boot_cluster, &bytes);
    if (rc != FAT12_OK || bytes != sizeof *h)
        exitdos_fail("boot FAT read of the header failed", (uint32_t)rc);
#ifndef KMAIN_MUT_COMMAND_MAGIC
    for (k = 0u; k < TRANSIENT_MAGIC_LEN; k++)
        if (h->magic[k] != TRANSIENT_MAGIC[k])
            exitdos_fail("no COMMAND3 transient on the boot disk", k);
#endif
    if (h->n_imports != n_want)
        exitdos_fail("transient import count != the kernel's", h->n_imports);
    if (h->load_end <= TRANSIENT_BASE + hdr_bytes ||
        h->load_end != TRANSIENT_BASE + command.file_size ||
        h->bss_end < h->load_end || h->bss_end > TRANSIENT_CEIL)
        exitdos_fail("transient header extents", h->bss_end);
    rc = fat12_read_file(&g_boot_vol, 0, 0u, &command, h,
                         TRANSIENT_CEIL - TRANSIENT_BASE, g_boot_cluster, &bytes);
    if (rc != FAT12_OK || bytes != command.file_size)
        exitdos_fail("boot FAT read of the transient failed", (uint32_t)rc);
    for (k = 0u; k < n_want; k++)
        if (h->imports[k] != transient_import_want[k])
            exitdos_fail("transient linked against another kernel build", k);
    if (h->entry < TRANSIENT_BASE + hdr_bytes || h->entry >= h->load_end)
        exitdos_fail("transient entry outside its text", h->entry);
    if (h->exec_secondary < TRANSIENT_BASE + hdr_bytes ||
        h->exec_secondary >= h->load_end)
        exitdos_fail("transient exec_secondary outside its text", h->exec_secondary);
    for (uint8_t *z = (uint8_t *)(uintptr_t)h->load_end;
         z < (uint8_t *)(uintptr_t)h->bss_end; z++)
        *z = 0u;
    serial_puts("COMMAND-TRANSIENT-OK bytes=");
    serial_putu(h->load_end - TRANSIENT_BASE);
    serial_putc('\n');
    /* From here COMMAND.COM is resident: a program's EXEC of it runs the
     * secondary processor (loader_exec_by_name, initech-13ig). */
    g_transient_exec_secondary =
        (void (*)(const char *, uint32_t))(uintptr_t)h->exec_secondary;
    return (void (*)(int))(uintptr_t)h->entry;
}

/* The hand-off itself. Never returns. */
static __attribute__((cold,noinline)) void flair_exit_to_dos(const boot_info_t *bi)
{
    void (*repl)(int);

    /* Input ownership follows the handoff itself, including the existing
     * no-teardown pixel mutant: that mutant must still run DIR at the prompt
     * so its oracle can grade the deliberately surviving desktop pixels. */
    kbd_set_input_owner(KBD_OWNER_DOS);
    int21_conin_handoff();

#if !defined(KMAIN_MUT_EXIT_NO_TEARDOWN)
    /* TEAR DOWN: the desktop's input + clock hooks out, the Toolbox fault
     * triage unbound (a DOS program's fault is the ordinary PC LOAD LETTER
     * panic, not a tenant death). IRQ12 stays live with NO hook: the mouse
     * ISR keeps draining the PS/2 AUX stream, so a mouse moved at the prompt
     * can never leave a byte in the 8042 output buffer ahead of a keystroke. */
    /* A KNOWN CONSOLE STATE: re-initialise the boot console from boot_info
     * (bounds, bpp, font) and clear the whole LFB to its paper, or panic.
     * Both halves are resident (kmain.c flair_exit_teardown): the hook
     * setters may not be overlay imports (DEC-14-6.5) and the console's
     * seafoam DAC values keep their one home in kmain.c. */
    if (!flair_exit_teardown(bi))
        exitdos_fail("no boot console to hand back", 0u);
#else
    (void)bi;
#endif
    /* The ownership handoff already discarded the desktop's cooked input. */
    serial_puts("FLAIR-EXIT-TO-DOS\n");
    /* The kernel context COMMAND.COM runs in: the kernel PSP and terminate
     * hook (the loader rebinds both per EXEC and restores them), the root of
     * drive A: as the current directory. */
    int21_set_exit(int21_exit_hook);
    int21_set_psp(&g_kernel_psp);
    int21_cwd_reset();
    /* COMMAND.COM's DOS personality: the seams the boot-to-prompt kernel binds
     * after ITS mount (kernel_main's FAT-MOUNT block) and the desktop never
     * needed -- AH=4Bh EXEC through the loader, AH=25h/35h on the live IDT,
     * INT 25h/26h on the flagship volume's device (CHKDSK.COM lives in APPS). */
    int21_set_exec_backend(loader_exec_by_name);
    int21_set_vectortable(int21_setvect_idt, int21_getvect_idt);
    if (g_finder_vol != (const fat12_volume_t *)0 && g_absdisk_vol.dev != 0) {
        int21_absdisk_backend_t absdisk;
        absdisk.read          = int21_absdisk_read;
        absdisk.write         = int21_absdisk_write;
        absdisk.total_sectors = g_finder_vol->bpb.total_sectors_16 != 0u
                                    ? (uint32_t)g_finder_vol->bpb.total_sectors_16
                                    : g_finder_vol->bpb.total_sectors_32;
        int21_set_blockdev(&absdisk);
    }
    repl = exitdos_load_transient();
    exitdos_banner();       /* resident: the locked banner lines' home */
    dos_puts("\n$");
    serial_puts("SHELL-READY\n");
    repl(1);          /* the permanent command processor never returns */
    exitdos_fail("COMMAND.COM returned", 0u);
}

/* Special > Exit to InitechDOS after the shell noted the open views. */
void finder_exit_dos(flair_live_ctx_t *ctx, const boot_info_t *bi,
                            int16_t noted)
{
    static const char *const labels[2] = { "Exit", "Cancel" };
    static char lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];
    const char *text[FINDER_ALERT_LINES];
    int n = finder_exit_dos_alert_text(lines), k;

    for (k = 0; k < n; k++) text[k] = lines[k];
    flair_live_alert_open(ctx, bi, text, n, labels, 2, 1);
    serial_puts("FINDER-EXIT-DOS-ALERT\n");
    if (flair_live_alert_run(ctx, bi) != 1u) {
        serial_puts("FINDER-EXIT-DOS-CANCEL\n");
        return;
    }
    serial_puts("FINDER-POWER name=EXITDOS views=");
    serial_puti((int32_t)noted);
    serial_putc('\n');
    if (!finder_power_ask_apps(ctx, bi))
        return;                         /* an application cancelled          */
    g_finder_db_dirty = 1;
    finder_desk_persist();
    flair_exit_to_dos(bi);
}
