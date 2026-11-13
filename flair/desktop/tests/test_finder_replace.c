/* tdnl.57: Replace preserves the displaced item through the SAME Trash
 * staging mechanism, then commits the incoming item. Nothing unlinked first. */
#define main legacy_ops_main
#include "test_finder_ops.c"
#undef main
static int move_step;
static int failed_rollback(void *u,const char *src,uint16_t sd,const char *dst,uint16_t dd)
{
    if(++move_step>1) return FINDER_WIN_ERR_MOVE;
    return m_move(u,src,sd,dst,dd);
}
int main(void)
{
    finder_tgt_t t;
    finder_move_result_t r;
    scene_init(&S);
    mv_add(&S.mock.d[1],"README.TXT",0x20u,8);
    t=finder_ops_resolve(&S.sh,ROOT,0,148,122);
    CHECK(finder_ops_drop(&S.sh,ROOT,0,&t,0,0,&r)==FINDER_WIN_ERR_EXISTS,"REPLACE first asks permission");
    CHECK(finder_ops_replace(&S.sh,ROOT,0,&t,0,0,&r)==FINDER_WIN_OK,"REPLACE confirmed move succeeds");
    CHECK(!mv_has(&S.mock,CL_ROOT,"README.TXT") && mv_has(&S.mock,CL_APPS,"README.TXT"),"REPLACE incoming item reaches destination");
    CHECK(mv_has(&S.mock,CL_TRASH,"README.TXT") && S.sh.n_origins==1,"REPLACE displaced item preserved with origin");
    CHECK(S.mock.d[1].e[mv_find(&S.mock.d[1],"README.TXT")].cluster==5,
          "REPLACE destination contains incoming bytes, not displaced bytes");
    scene_init(&S);
    mv_add(&S.mock.d[1],"README.TXT",0x20u,8);
    S.sh.fs.move=failed_rollback;move_step=0;
    t=finder_ops_resolve(&S.sh,ROOT,0,148,122);
    CHECK(finder_ops_replace(&S.sh,ROOT,0,&t,0,0,&r)==FINDER_WIN_ERR_MOVE,
          "REPLACE_ROLLBACK reports failed incoming move");
    CHECK(mv_has(&S.mock,CL_ROOT,"README.TXT") && mv_has(&S.mock,CL_TRASH,"README.TXT"),
          "REPLACE_ROLLBACK preserves both payloads");
    CHECK(S.sh.n_origins==1 && r.db_dirty,
          "REPLACE_ROLLBACK displaced item remains recoverable when rollback fails");
    return TEST_SUMMARY("test-finder-replace");
}
