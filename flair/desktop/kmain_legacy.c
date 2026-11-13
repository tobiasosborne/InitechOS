/* Unbuilt production quarry from legacy-final kmain.c.
 * Only the unextracted film scene, hidden mat and text-app handoff remain.
 * These fragments retain their surrounding context dependencies; they are not
 * an entry point or another product personality. See README.md.
 * Boot scenarios, overlays and mutation personalities have been discarded. */

/* Film modal dragging; original kmain.c lines 2179-2278. */

/* Text application handoff; original kmain.c lines 2995-3149. */
/* ===========================================================================
 * O-7 (ADR-0013 Sec 3.2 class-2): SAMIR -- the FIRST TEXT TENANT.
 * ---------------------------------------------------------------------------
 * SAMIR (InitechBase) is a character-mode .COM, not a FLAIR GUI tenant: it owns
 * the WHOLE screen as a DOS text program (its dot-prompt REPL writes 80x25 text
 * via INT 21h). The App Contract launches it as a TEXT tenant by SUSPENDING the
 * live desktop, running SAMIR full-screen, and REBUILDING the desktop on return
 * -- the classic System-7/MultiFinder "switch to a full-screen app" gesture.
 *
 * DEC-LAUNCH-1: THE SYSTEM HOTKEY is Ctrl+Alt+backslash, SET-1 make 0x2B
 * with BOTH controlKey and optionKey in EventRecord.modifiers. The Event
 * Manager folds left/right Ctrl and Alt into those flags (event.c;
 * spec/event_model.h Sec 2), so either side works. Harness token ctrl-alt-bsl.
 * Ref: Lotus 1-2-3 R2.2 Reference Manual p. 1-17 (PDF 33, local manuals
 * corpus): backslash is the repeating-label prefix. Plain backslash MUST
 * reach the foreground tenant (ADR-0013 Sec 3.3); the previous bare-key
 * launcher stole worksheet text. Ctrl+Alt is an authored system reservation,
 * outside the printable label alphabet on that page; both modifiers are
 * required, leaving plain/Shift/Ctrl-only/Alt-only text to the tenant.
 * The shared system_key.h predicate is used before flair_app_dispatch and
 * by the host routing oracle; FLAIR_MUT_PLAIN_BSL_LAUNCH restores the bug.
 *
 * INPUT OWNERSHIP (DAS L002): the GUI receives raw input exclusively.
 * At the cooked launch chord, transfer the queued raw suffix to DOS in order,
 * preserving its decoder state and later typeahead. During SAMIR the raw hook
 * only records state to be cooked without GUI delivery on resume.
 *
 * REBUILD = RE-PRESENT THE PRESERVED OFFSCREEN (Option A, stateless; the ADR's
 * "re-render the SAME scene + re-present"). DEC-03a.5, initech-jarzu:
 * SAMIR's module/reserve/native heap/guard/stack occupy [0x600000,0x800000);
 * its PSP/environment use owned conventional controls. The locked map proves
 * all of these DISJOINT from the preserved FLAIR heap [0x100000,0x500000).
 * ctx->off STILL holds the EXACT pre-suspend co-resident desktop after SAMIR
 * returns; flair_desktop_present re-blits it to the LFB BIT-IDENTICALLY (the same
 * bytes SAMIR's text overwrote). No heap re-init, no tenant relaunch needed --
 * the tenants' WindowRecords/content are heap-resident and untouched.
 *
 * Ref: ADR-0013 Sec 3.2 (class-2 text tenants); spec/memory_map.h (DEC-03a locked
 *      high-application / low-control vs FLAIR-heap disjointness proof); os/milton/kmain.c loader_exec_by_name
 *      (the PSP/JFT/CWD save+restore wrapper this reuses); os/milton/kbd.c (the
 *      exclusive input owner). CLAUDE.md Law 2 (pre==post is real, not by luck), Law 4
 *      (full-screen app then back to the live desktop), Rule 2 (fail loud),
 *      Rule 11 (deterministic), Rule 12 (ASCII-clean).
 * ===========================================================================*/
#define FLAIR_TEXT_TENANT_NAME         "SAMIR.EXE"  /* 8.3 name on the FAT volume (InitechMZ + SAMIR.OVL beside it, initech-5etom.1) */

/* g_flair_kbd_ring (the SPSC raw ring the FLAIR pump drains) is defined earlier
 * in this TU's BOOT_FLAIR_LIVE block; flair_launch_text_tenant drains its stale
 * events after the suspend (the scancode hook stays installed through SAMIR). */

/* flair_launch_text_tenant -- suspend the live desktop, run a character-mode .COM
 * TEXT tenant full-screen, then rebuild the desktop BIT-IDENTICALLY on return.
 * Reuses the EXISTING loader_exec_by_name wrapper (PSP/JFT/CWD save+restore) and
 * the EXISTING flair_desktop_present blit (BC-1: no new load/geometry logic). */
static void flair_launch_text_tenant(const boot_info_t *bi, flair_live_ctx_t *ctx,
                                     const char *name83)
{
    serial_puts("FLAIR-SUSPEND\n");

#ifdef KMAIN_INPUT_HANDOFF_GATE
    /* ADR-0013 O-7 typeahead: model a delayed cooperative task while IRQ1
     * queues the launch release (3 bytes) and QUIT+Return (10 bytes).
     * Gate images only. This makes the no-replay mutation independent of
     * host scheduling; the production path has no wait. */
    {
        uint32_t start = flair_tick_count();
        while (g_flair_kbd_ring.head - g_flair_kbd_ring.tail < 13u &&
               flair_tick_count() - start < 200u)
            __asm__ __volatile__("hlt");
        if (g_flair_kbd_ring.head - g_flair_kbd_ring.tail >= 13u)
            serial_puts("FLAIR-HANDOFF-QUEUED\n");
    }
#endif

    /* Transfer only raw bytes AFTER the cooked launch chord to DOS, using
     * the decoder snapshot at that boundary. IRQ1's state is already at
     * head; reusing it would misread releases and extended prefixes. Mask
     * IRQs so live DOS input cannot overtake this bounded replay. */
    {
        unsigned long flags;
        kbd_state_t replay = g_flair_consumed_keys;
        __asm__ __volatile__("pushf; pop %0; cli" : "=r"(flags) : : "memory");
        kbd_set_input_owner(KBD_OWNER_DOS);
        int21_conin_handoff();
        uint32_t end = g_flair_kbd_ring.head;
        for (uint32_t i = g_flair_kbd_ring.tail; i != end; ++i) {
            const flair_raw_event_t *raw = &g_flair_kbd_ring.slots[i & FLAIR_RAW_RING_MASK];
            if (raw->kind == FLAIR_RAW_KEYBOARD)
                kbd_console_replay(&replay, (uint8_t)raw->payload);
        }
        __asm__ __volatile__("push %0; popf" : : "r"(flags) : "memory", "cc");
    }

    /* Prepare the screen for SAMIR's text I/O: clear the LFB to the console paper
     * + home the cursor. The kernel console came up at boot and is already bound
     * to the INT 21h CON sink (g_int21_con), so SAMIR's AH=09h/AH=40h output fans
     * to LFB + serial with no extra wiring (reuse, not re-init). */
    if (g_int21_con) {
        console_clear(g_int21_con);
        console_set_cursor(g_int21_con, 0u, 0u);
    }

    /* Run the .COM through the EXISTING saw wrapper: load_program_from_fat off the
     * booted volume, then restore the kernel-context exit-hook/PSP/CWD (the loader
     * rebinds all three during the run). loader_exec_by_name returns 0 on success
     * (else a DOS EXEC error code). */
    {
        uint8_t  rc   = 0;
        uint16_t derr = loader_exec_by_name(name83, 0u /* root dir */,
                                            (const char *)0, 0u,
                                            0u /* env: inherit-empty */, &rc);
        if (derr == 0u) {
            serial_puts("FLAIR-TEXT-EXIT rc=");
            serial_putu((uint32_t)rc);
            serial_putc('\n');
        } else {
            serial_puts("FLAIR-TEXT-FAIL err=");
            serial_putu((uint32_t)derr);
            serial_putc('\n');
        }
    }

    /* Drain SAMIR's stale input without GUI dispatch. DEC-LAUNCH-1 requires
     * cooking releases too: Ctrl/Alt came down before suspend and came up
     * during SAMIR. A raw tail=head drop would leave them latched on resume.
     * Keep the Event Manager's tick/cursor/lock state (event_model.h Sec 2/5). */
    {
        unsigned long flags;
        __asm__ __volatile__("pushf; pop %0; cli" : "=r"(flags) : : "memory");
        flair_text_tenant_drain(&g_flair_kbd_ring);
        kbd_set_input_owner(KBD_OWNER_GUI);
        int21_conin_handoff();
        __asm__ __volatile__("push %0; popf" : : "r"(flags) : "memory", "cc");
    }

    /* REBUILD: the offscreen survived SAMIR (< 1 MiB vs FLAIR-heap disjoint, see
     * the header), so re-present it to restore the desktop BIT-IDENTICALLY. */
    flair_desktop_present(bi, &ctx->off);

    serial_puts("FLAIR-RESUME\n");
}

/* Jump-to-Conclusions activation; original kmain.c lines 5133-5176. */
static __attribute__((cold,noinline)) int flair_live_jtc_trigger(
    flair_live_ctx_t *ctx, const boot_info_t *bi, const EventRecord *ev)
{
    static unsigned clicks;
    int fire = egg_jtc_trigger(&clicks,ev);
    if (fire) {
        FlairApp *prev = ctx->plist->head, *a = prev;
        while (a && a->procs != &egg_jtc_procs) a = a->nextApp;
        if (a) (void)FlairProcess_activate(ctx->plist,ctx->wm,ev,a);
        else a = FlairProcess_launch(ctx->plist,ctx->wm,&ctx->off,ctx->master,
            &egg_jtc_procs,"",fta_rect(0,0,1,1),16384u,16384u);
        if (!a) {
            serial_puts("PANIC JTC launch\n");
            for (;;) { __asm__ __volatile__("cli; hlt"); }
        }
        flair_live_finish_tenant_switch(ctx,bi,ctx->plist,prev,g_ten_barport);
        serial_puts("EGG-JTC-OPEN click="); serial_putu(clicks); serial_putc('\n');
    }
    return ev->what == mouseDown && clicks != 0;
}

static __attribute__((cold,noinline)) int flair_live_jtc_pad(
    flair_live_ctx_t *ctx, const boot_info_t *bi, EventRecord *ev)
{
    int pad = egg_jtc_take_pad();
    if (pad < 0) return 0;
    rgn_rect_t r = egg_jtc_pad(region_get_bbox(ctx->plist->head->windows->contRgn),pad);
    ev->where.h = (int16_t)((r.left+r.right)/2);
    ev->where.v = (int16_t)((r.top+r.bottom)/2);
    flair_event_warp(ev->where.h,ev->where.v);
    flair_live_cursor_track(ev); /* Cursor Manager erases save-under and jumps */
    serial_puts("EGG-JTC-JUMP pad="); serial_putu((uint32_t)pad);
    serial_puts(" x="); serial_puti(ev->where.h);
    serial_puts(" y="); serial_puti(ev->where.v); serial_putc('\n');
    const char *text[1] = {egg_jtc_label(pad)};
    static const char *const ok[1] = {"OK"};
    flair_live_alert_open(ctx,bi,text,1,ok,1,1);
    serial_puts("EGG-JTC-ALERT up\n");
    (void)flair_live_alert_run(ctx,bi);
    serial_puts("EGG-JTC-ALERT closed\n");
    return 1;
}

/* Returns 1 when `sel` was an Apple-menu choice (handled here). */

/* Film scene construction; original kmain.c lines 6498-6608. */
#ifdef FLAIR_FILM_SCENE
    /* M024/M025: real disk tenant, existing one-slot host and WM verbs.
     * The scene authors placement; Calc alone paints its live content. */
    {
        dir_entry_t apps;
        int apps_rc=FAT12_ERR_NOT_FOUND, load_rc=TBX_OK;
        if (g_finder_vol)
            apps_rc=fat12_find(g_finder_vol,g_finder_secbuf,"APPS",&apps);
        if (apps_rc==FAT12_OK && !(apps.attribute & DIR_ATTR_DIRECTORY))
            apps_rc=FAT12_ERR_NOT_FOUND;
        if (g_film_volume_status==FAT12_OK && apps_rc==FAT12_OK)
            load_rc=tbx_launch("CALC.EXE",apps.start_cluster);
        panic_reason_t reason=film_startup_reason(g_film_volume_status,apps_rc,load_rc);
        if (reason!=PANIC_REASON_NONE) {
            /* DAS L001: carry the actual mount/find/launch statuses to serial;
             * show a short accurate cause before the terminal HALTED marker. */
            serial_puts("PANIC film-startup volume="); serial_puti(g_film_volume_status);
            serial_puts(" apps="); serial_puti(apps_rc);
            serial_puts(" load="); serial_puti(load_rc); serial_putc('\n');
            panic_show_reason(reason,reason==PANIC_REASON_DATA_VOLUME ? "DATA DISK" :
                                     reason==PANIC_REASON_APPS ? "APPS" : "CALC.EXE");
            serial_puts("HALTED\n");
            for (;;) { __asm__ __volatile__("cli; hlt"); }
        }
        FlairApp *calc=tbx_take_affirm();
        if (!calc || !calc->windows || !calc->windows->nextWindow) {
            panic_show_reason(PANIC_REASON_CALC_WINDOWS,"CALC.EXE");
            serial_puts("PANIC film-startup CALC windows\nHALTED\n");
            for (;;) { __asm__ __volatile__("cli; hlt"); }
        }
        EventRecord ev0 = {0};
        (void)FlairProcess_activate(&ten_plist,ctx.wm,&ev0,calc);
        flair_film_arrange(ctx.wm,calc->windows->nextWindow,calc->windows);
        /* shell_render retains the authored Photoshop bar for the tableau. */
        g_ten_plist=&ten_plist;
        shell_render(ctx.scene,&ctx.off);
        desktop_validate_all(ctx.wm);
        for (WindowPtr w=ctx.wm->front;w;w=w->nextWindow)
            if (tbx_owns_window(w)) WindowMgr_invalidate(ctx.wm,w,region_get_bbox(w->contRgn));
        flair_route_updates(&ten_plist,ctx.wm);
        flair_film_cursor(ctx.scene->modal_up);
        flair_desktop_present(&b,&ctx.off);
        serial_puts("FLAIR-FILM-SCENE CALC ledger+chart\n");
    }
#else
    /* (3) Each tenant's OWN menu is its menubar; the live app-switch loop swaps the
     * FOREGROUND tenant's menu into the SECOND (Photoshop-chimera) band (ref beads
     * initech-4w15 / initech-7tjp: band 1 == the SHELL-OWNED static System-7 bar,
     * retained by shell_render, never replaced by a foreground switch). HELLO's
     * menu IS the Photoshop bar; NOTES owns the no-Apple File/Edit/Notes fixture
     * above. Because HELLO is the boot FOREGROUND (2), shell_render's fixed band-2
     * composition (bar_photoshop) ALREADY equals the foreground's menu at boot --
     * so the resting scene stays the distinct chimera with no extra draw, and the
     * O-5 switch makes band 2 swap Photoshop -> NOTES while remaining distinct
     * from band 1 (PRD Sec 1.1 / Sec 3 / Sec 6.3; Law 4). */
    ten_hello->menubar = &ctx.scene->bar_photoshop;
    ten_notes->menubar = &flair_ten_notes_bar;

    /* (4) Recomposite the offscreen: teal desktop + the two tenants' window chrome
     * (back-to-front) + the two menu bars (shell_render; modal_up==0 => no dialog).
     * The compositor owns chrome; the tenant CONTENT is (re)drawn next. shell_render
     * composes band 1 = bar_sys (System-7) and band 2 = bar_photoshop (Photoshop) --
     * the distinct chimera. UNTOUCHED by initech-4w15 (Do NOT touch shell_render's
     * initial draw): with HELLO the boot foreground, band 2 already equals the
     * foreground app's menu, so NO extra boot-time band-2 draw is needed (an earlier
     * fix attempt added one and produced two identical System-7 bars at rest -- the
     * Law-4 regression this arrangement avoids by construction). */
    shell_render(ctx.scene, &ctx.off);

    /* (4b) DQ1: the from-scratch composite serviced ALL pending damage --
     * including the STALE launch-time seeds. Between the two FlairProcess_launch
     * calls NOTES was briefly the FRONT window, so its 0->1 activation seed
     * (initech-rqz5) covered its FULL structure; HELLO then launched on top,
     * making that pending damage a superset of what NOTES still owns. The
     * updateEvt paint path clips to contRgn INTERSECT updateRgn (ref_tenant.c)
     * and TRUSTS updateRgn to be within the window's visible region, so routing
     * that stale seed would let background NOTES stomp foreground HELLO's
     * overlap (caught live by test-flair-appswitch TIER-A when this validate
     * was missing). A scene build is not a pump cycle; the composite + this
     * validate close it, and the explicit per-tenant seeds below (clipped to
     * CURRENT visibility by WindowMgr_invalidate) rebuild exactly the content
     * damage that is really owed. */
    desktop_validate_all(ctx.wm);

    /* (5) Repaint each tenant's content in z-order through the updateEvt spine.
     * WindowMgr_invalidate clips the seed to each window's VISIBLE region, so the
     * background NOTES does NOT overdraw the front HELLO in the overlap; then
     * flair_route_updates delivers the updateEvt to each owning tenant front-to-
     * back. Result: HELLO full content (white), NOTES exposed sliver (gray), overlap
     * = HELLO_FILL -- the PRE app-switch state the O-5 grader expects (ref bead
     * initech-4w15: HELLO is the boot foreground). */
    if (ten_hello->windows != (WindowPtr)0)
        WindowMgr_invalidate(ctx.wm, ten_hello->windows,
                             region_get_bbox(ten_hello->windows->contRgn));
    if (ten_notes->windows != (WindowPtr)0)
        WindowMgr_invalidate(ctx.wm, ten_notes->windows,
                             region_get_bbox(ten_notes->windows->contRgn));
    flair_route_updates(&ten_plist, ctx.wm);

    /* (6) BC-4: present the co-resident tenant desktop BEFORE arming the pump. */
    flair_desktop_present(&b, &ctx.off);
    serial_puts("FLAIR-TENANTS-READY hello+notes co-resident; HELLO foreground\n");

#endif /* FLAIR_FILM_SCENE */

/* Film scene pump branch; original kmain.c lines 6910-6940. */
#ifdef FLAIR_FILM_SCENE
            /* The Saving scene is staged, not a fake I/O operation. Escape
             * dismisses it to let the operator use/drag the real windows. */
            if (ctx.scene->modal_up && flair_film_dismiss(&ev)) {
                ctx.scene->modal_up=0;
                shell_sync_overlay(ctx.scene);
                flair_film_cursor(0);
                shell_render(ctx.scene,&ctx.off);
                for (WindowPtr w=ctx.wm->front;w;w=w->nextWindow)
                    if (tbx_owns_window(w)) WindowMgr_invalidate(ctx.wm,w,region_get_bbox(w->contRgn));
                flair_live_content_phase(&ctx);
                DrawMenuBar(g_ten_barport,flair_live_tenant_bar(&ctx,&ten_plist),-1,
                            (const region_t *)0);
                flair_desktop_present(&b,&ctx.off);
                serial_puts("FLAIR-FILM-MODAL dismissed\n");
                continue;
            }
            if (ctx.scene->modal_up && ctx.scene->dlg) {
                flair_modal_disposition_t md=DialogModalDispatch(ctx.scene->dlg,&ev,
                    flair_live_modal_block_marker,(void *)0);
                if (md!=FLAIR_MODAL_PASSTHROUGH) {
                    if (md==FLAIR_MODAL_DRAG && ev.what==mouseDown)
                        flair_live_do_modal_drag(&ctx,&b,ev.where);
                    else if (md==FLAIR_MODAL_CAPTURE) {
                        uint16_t hit=0;
                        (void)DialogHandleEvent(ctx.scene->dlg,&ev,(dialog_filter_fn)0,&hit);
                    }
                    continue;
                }
            }
#endif
