/* Extracted production start; reference: legacy-final kmain.c. */
#include "desktop_internal.h"

void kernel_graphical_environment(void)
{
    boot_info_t b=*boot_info();
    idt_gate_t saved_gate=idt_get_gate(TBX_GATE_VECTOR);
    g_desktop_exit=0;
    g_vga_dac_shift=b.lfb_dac_bits==8?0:2;
    flair_boot_screen(&b);
    flair_tenant_mount_volume();
    flair_live_ctx_t ctx;
    FlairProcessList ten_plist;
    FlairApp *ten_finder = (FlairApp *)0;
    flair_desktop_run(&b, &ctx);
    serial_puts("FLAIR-DESKTOP\n");
    FlairProcessList_init(&ten_plist);
    {
        rgn_rect_t db;
        db.top = (int16_t)(SHELL_MENUBAR2_TOP + FLAIR_MENUBAR_H);
        db.left = 0;
        db.bottom = (int16_t)FLAIR_SCREEN_H;
        db.right = (int16_t)FLAIR_SCREEN_W;
        ten_finder = FlairProcess_launch(&ten_plist, ctx.wm, &ctx.off, ctx.master,
                                         &finder_shell_procs, "FINDER", db,
                                         (uint32_t)(64u * 1024u),
                                         (uint32_t)(256u * 1024u));
        if (ten_finder == (FlairApp *)0) {
            serial_puts("PANIC flair-tenants: FINDER launch returned NULL "
                        "(budget/heap exhausted)\nHALTED\n");
            while (!g_desktop_exit) { __asm__ __volatile__("cli; hlt"); }
        }
        g_finder_shell = finder_shell_of(ten_finder);
        g_finder_desk = finder_shell_desk(g_finder_shell);
        if (g_finder_desk == (finder_desk_t *)0) {
            serial_puts("PANIC flair-tenants: FINDER has no desktop model\n"
                        "HALTED\n");
            while (!g_desktop_exit) { __asm__ __volatile__("cli; hlt"); }
        }
        finder_click_reset(&g_finder_click);
        {
            int vidx = finder_desk_find_kind(g_finder_desk, FINDER_ICON_VOLUME);
            if (vidx >= 0)
                finder_desk_set_name(g_finder_desk, vidx, g_finder_vol_label);
        }
        finder_desk_load_positions();
        finder_desk_install_underlay(g_finder_desk, ctx.wm);
        finder_shell_bind_fs(g_finder_shell, &g_finder_fs_binding);
        if (finder_shell_recount_trash(g_finder_shell)) {
            g_finder_shell->trash_icon_dirty = 0u;
            serial_puts("DESK-TRASH-ICON full\n");
        }
        g_finder_ctx.trace = finder_trace_serial;
        g_finder_ctx.trace_user = (void *)0;
        finder_shell_bind_ctx(g_finder_shell, &g_finder_ctx);
        finder_desk_sync_ctx();
        ten_finder->menubar = finder_menu_bar();
        g_ten_plist = &ten_plist;
        g_ten_finder = ten_finder;
        serial_puts("FINDER-DESKTOP-ICONS n=");
        serial_putu((uint32_t)g_finder_desk->n);
        serial_putc('\n');
    }
    ctx.plist = &ten_plist;
    {
        tbx_host_t th;
        th.list = &ten_plist;
        th.wm = ctx.wm;
        th.master = ctx.master;
        th.surface = &ctx.off;
        th.puts = serial_puts;
        th.cellfont = (const uint8_t *)(uintptr_t)b.font_addr;
        th.scrap = (FlairScrap *)flair_alloc(ctx.master, FLAIR_CLASS_GENERAL,
                                             (uint32_t)sizeof(FlairScrap));
        if (th.scrap == (FlairScrap *)0) flair_desktop_oom("desk scrap");
        FlairScrap_init(th.scrap);
        serial_puts("FLAIR-SCRAP-READY bytes=");
        serial_putu((uint32_t)sizeof(FlairScrap));
        serial_putc('\n');
        tbx_bind(&th);
        tbx_preferences_boot(1024u+b.ext_mem_kb);
        idt_set_gate((uint8_t)TBX_GATE_VECTOR, (void *)tbx_gate_entry,
                     (uint16_t)TBX_GATE_SELECTOR, (uint8_t)TBX_GATE_TYPE_ATTR);
        panic_set_fault_hook(tbx_fault_triage);
        serial_puts("TBX-GATE-READY vec=0x81\n");
    }
    shell_render_with_menubar(ctx.scene, &ctx.off, finder_menu_bar());
    desktop_validate_all(ctx.wm);
    flair_desktop_present(&b, &ctx.off);
    flair_event_init(&g_flair_kbd_ring);
    kbd_state_init(&g_flair_consumed_keys);
    flair_event_set_keyboard_observer(flair_consumed_key);
    g_flair_mouse_activity = 0u;
    g_flair_cursor_visible = 0;
    g_flair_cursor_x = (int)(FLAIR_SCREEN_W / 2);
    g_flair_cursor_y = (int)(FLAIR_SCREEN_H / 2);
    flair_cursor_show(g_flair_cursor_x, g_flair_cursor_y);
    g_flair_cursor_visible = 1;
    serial_puts("FLAIR-CURSOR x=");
    serial_puti((int32_t)g_flair_cursor_x);
    serial_puts(" y=");
    serial_puti((int32_t)g_flair_cursor_y);
    serial_putc('\n');
    __asm__ volatile("cli" : : : "memory");
    pit_set_tick_hook(flair_tick_advance);
    kbd_set_scancode_hook(flair_live_kbd_post);
    idt_install_irq(0x34u, (void *)irq12_entry);
    mouse_init();
    mouse_set_event_hook(flair_live_mouse_post);
    serial_puts("FLAIR-HOOK-SET\n");
    __asm__ volatile("sti");
    kbd_set_input_owner(KBD_OWNER_GUI);
    int21_conin_handoff();
    pic_unmask_irq12();
    flair_event_set_yield(flair_live_yield);
    serial_puts("FLAIR-LIVE-READY\n");
    {
        enum { FLAIR_TEN_TICK_ANNOUNCE = 4 };
        enum { FLAIR_TEN_WNE_SLICE = 3 };
        uint32_t start = flair_tick_count();
        uint32_t last = start;
        uint32_t seen = 0u;
        ((void)(start + (uint32_t)250));
        bitmap_t ten_bar2_view;
        GrafPort ten_barport;
        {
            make_offset_view(&ten_bar2_view, &ctx.off, (uint32_t)SHELL_MENUBAR2_TOP);
            rgn_rect_t whole = { 0, 0, (int16_t)ten_bar2_view.height,
                                 (int16_t)ten_bar2_view.width };
            ten_barport.portBits.bm = ten_bar2_view;
            ten_barport.portBits.bounds = whole;
            ten_barport.portRect = whole;
            ten_barport.visRgn = (region_t *)0;
            ten_barport.clipRgn = (region_t *)0;
            ten_barport.pnLoc.v = 0; ten_barport.pnLoc.h = 0;
            ten_barport.pnSize.v = 1; ten_barport.pnSize.h = 1;
            ten_barport.pnVis = 0;
            ten_barport.grafProcs = (QDProcs *)0;
        }
        g_ten_barport = &ten_barport;
        MenuClock_attach(&ctx.scene->bar_sys);
        uint32_t clock_last_poll = 0;
        int clock_polled = 0;
        while (!g_desktop_exit) {
            if(g_finder_io_error) {
                finder_win_status_t st=g_finder_io_error;
                g_finder_io_error=FINDER_WIN_OK;
                (void)finder_notice(&ctx,&b,st,"");
            }
            EventRecord ev;
            int got = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &ev,
                                    (uint32_t)FLAIR_TEN_WNE_SLICE);
            flair_live_cursor_track(&ev);
            tbx_preferences_tick();
            if(tbx_preferences_dirty()) {
                for(WindowPtr w=ctx.wm->front;w;w=w->nextWindow)
                    WindowMgr_invalidate(ctx.wm,w,region_get_bbox(w->contRgn));
                shell_render_with_menubar(ctx.scene,&ctx.off,
                                          flair_live_tenant_bar(&ctx,&ten_plist));
                flair_live_content_phase(&ctx);
                flair_desktop_present(&b,&ctx.off);
            }
            uint32_t now = flair_tick_count();
            if (!clock_polled || now - clock_last_poll >= 100u) {
                clock_polled = 1;
                clock_last_poll = now;
                int_frame_t time = { 0 };
                time.eax = 0x2C00u;
                int21_dispatch(&time);
                serial_puts("FLAIR-CLOCK-POLL\n");
                if (MenuClock_set((time.ecx >> 8) & 255u, time.ecx & 255u)) {
                    GrafPort clock_port = ten_barport;
                    clock_port.portBits.bm = ctx.off;
                    MenuClock_draw(&clock_port, (const region_t *)0);
                    flair_desktop_present(&b, &ctx.off);
                    serial_puts("FLAIR-CLOCK ");
                    serial_puts(MenuClock_text());
                    serial_putc('\n');
                }
            }
            if (now != last) {
                last = now;
                seen++;
                if (seen <= (uint32_t)FLAIR_TEN_TICK_ANNOUNCE) {
                    serial_puts("FLAIR-TICK n=");
                    serial_putu(now);
                    serial_putc('\n');
                }
            }
            if (!got) {
                continue;
            }
            flair_live_emit_evt(&ev);

            if (flair_live_finder_key(&ctx, &b, &ev)) {
                continue;
            }
            if (flair_live_tenant_key(&ctx, &b, &ev)) {
                continue;
            }
            WindowPtr chrome_w = (WindowPtr)0;
            flair_part_code_t chrome_pc = inDesk;
            int chrome_was_active = 0;
            if (ev.what == (uint16_t)mouseDown) {
                chrome_pc = FindWindow(ctx.wm, ev.where, &chrome_w);
                chrome_was_active = chrome_w != (WindowPtr)0 && chrome_w->hilited;
            }
            FlairApp *ten_prev_fg = ten_plist.head;
            flair_app_dispatch(&ten_plist, ctx.wm, &ev);
            flair_live_finish_tenant_switch(&ctx, &b, &ten_plist,
                                            ten_prev_fg, &ten_barport);
            if (ev.what == (uint16_t)mouseDown) {
                if (chrome_pc == inDrag && chrome_w != (WindowPtr)0) {
                    flair_live_do_drag(&ctx, &b, chrome_w, ev.where);
                } else if (chrome_pc == inGoAway && chrome_w != (WindowPtr)0) {
                    if (!flair_live_track_box(&ctx, &b, chrome_w, chrome_pc)) {
                    } else {
                    int fslot = (g_finder_shell != (finder_shell_t *)0)
                        ? finder_win_slot_of(g_finder_shell, chrome_w) : -1;
                    if (fslot >= 0)
                        flair_live_do_finder_close(&ctx, &b, fslot);
                    else
                        flair_live_do_close(&ctx, &b, chrome_w, &ten_barport);
                    }
                } else if ((chrome_pc == inZoomIn || chrome_pc == inZoomOut) &&
                           chrome_w != (WindowPtr)0) {
                    if (flair_live_track_box(&ctx, &b, chrome_w, chrome_pc))
                        flair_live_do_zoom(&ctx, &b, chrome_w);
                } else if (chrome_pc == inGrow && chrome_w != (WindowPtr)0) {
                    flair_live_do_grow(&ctx, &b, chrome_w, ev.where);
                } else if (chrome_pc == inCollapse &&
                           chrome_w != (WindowPtr)0) {
                    if (flair_live_track_box(&ctx, &b, chrome_w, chrome_pc))
                        flair_live_do_collapse(&ctx, &b, chrome_w);
                } else if (ev.where.v >= 0 &&
                           ev.where.v < (int16_t)FLAIR_MENUBAR_H) {
                    (void)flair_live_apple_result(&ctx, &b,
                        flair_live_do_menu_at(&ctx, &b, &ctx.scene->bar_sys,
                                              ev.where, 0u));
                } else if (ev.where.v >= (int16_t)SHELL_MENUBAR2_TOP &&
                           ev.where.v < (int16_t)(SHELL_MENUBAR2_TOP +
                                                  FLAIR_MENUBAR_H)) {
                    MenuBar *menu2_bar = flair_live_tenant_bar(&ctx, &ten_plist);
                    uint32_t menu2_sel =
                        flair_live_do_menu_at(&ctx, &b, menu2_bar, ev.where,
                                              (uint32_t)SHELL_MENUBAR2_TOP);
                    if (!flair_live_apple_result(&ctx, &b, menu2_sel)) {
                        flair_live_finder_menu_result(&ctx, &b, menu2_bar,
                                                      menu2_sel);
                        flair_live_tenant_menu_result(&ctx, &b, menu2_bar,
                                                      menu2_sel, &ev);
                    }
                }
                if (chrome_pc == inDesk &&
                    ev.where.v >= (int16_t)(SHELL_MENUBAR2_TOP +
                                            FLAIR_MENUBAR_H)) {
                    flair_live_desk_activate(&ctx, &b, &ev);
                    flair_live_do_desk(&ctx, &b, &ev);
                }
                if (chrome_pc == inContent && chrome_w != (WindowPtr)0 &&
                    WindowScrollBand(chrome_w, ev.where) >= 0) {
                    if (chrome_was_active)
                        flair_live_track_scroll(&ctx, &b, chrome_w, ev.where);
                } else if (chrome_pc == inContent && chrome_w != (WindowPtr)0 &&
                    g_finder_shell != (finder_shell_t *)0) {
                    int fslot = finder_win_slot_of(g_finder_shell, chrome_w);
                    if (fslot >= 0 && chrome_was_active)
                        flair_live_do_finder_content(&ctx, &b, &ev, fslot);
                }
                flair_live_sync_foreground(&ctx, &b, &ev);
            }
            flair_live_tenant_service(&ctx, &b);
        }
    }
    flair_cursor_hide();
    pit_set_tick_hook(0);
    kbd_set_scancode_hook(0);
    mouse_set_event_hook(0);
    panic_set_fault_hook(0);
    kbd_set_input_owner(KBD_OWNER_DOS);
    int21_conin_handoff();
    flair_event_set_yield(0);
    idt_set_gate(TBX_GATE_VECTOR,
        (void *)(uintptr_t)((uint32_t)saved_gate.offset_lo | ((uint32_t)saved_gate.offset_hi<<16)),
        saved_gate.selector, saved_gate.type_attr);
    g_finder_shell=0; g_finder_desk=0;
    g_ten_plist=0; g_ten_finder=0; g_ten_barport=0;
    desktop_console_restore();
    serial_puts("FLAIR-EXIT-TO-DOS\n");
}

char g_finder_notice_lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];

char g_finder_trash_confirm_lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];

char g_finder_delete_now_confirm_lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];
