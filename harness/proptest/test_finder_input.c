/* Ref: tdnl.65, MTE Event Manager SelectWindow; tdnl.88, IM Control
 * Manager TrackControl (system7-decomp/specs/toolbox/control-manager.md).
 * Reuse the manager scene, never its expectations. */
#define main legacy_process_main
#include "test_process.c"
#undef main

int main(void)
{
    static win_store_t p, q;
    static mgr_store_t m;
    static FlairApp a, b;
    FlairProcessList list;
    EventRecord ev = mk_event(mouseDown, 20, 70, 0);
    rgn_rect_t frame = {0, 0, 64, 160};
    mgr_attach(&m, frame);
    win_attach(&q); win_attach(&p);
    NewWindow(&m.wm, &q.rec, (rgn_rect_t){8,56,30,94},
              (rgn_rect_t){11,57,29,93}, documentKind, documentProc, 1);
    NewWindow(&m.wm, &p.rec, (rgn_rect_t){8,6,30,44},
              (rgn_rect_t){11,7,29,43}, documentKind, documentProc, 1);
    a.name="P"; a.procs=&g_pq_procs; a.windows=&p.rec;
    b.name="Q"; b.procs=&g_pq_procs; b.windows=&q.rec;
    g_appP=&a; g_appQ=&b;
    FlairProcessList_init(&list);
    FlairProcess_register(&list,&b); FlairProcess_register(&list,&a);
    g_log_n=0;
    flair_app_dispatch(&list,&m.wm,&ev);
    CHECK(list.head==&b && m.wm.front==&q.rec,
          "ACTIVATE_ONLY raises the inactive window");
    CHECK(g_log_n==2 && g_log[0].what==activateEvt && g_log[1].what==activateEvt,
          "ACTIVATE_ONLY consumes the first content click");
    flair_app_dispatch(&list,&m.wm,&ev);
    CHECK(g_log_n==3 && g_log[2].what==mouseDown,
          "ACTIVATE_ONLY delivers the second content click");
    return TEST_SUMMARY("test-finder-input");
}
