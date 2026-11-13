/* tdnl.57: Trash rename survives encode/decode, Put Away uses original name
 * AND directory. Mutable mock volume is independent of the origin codec. */
#define main legacy_ops_main
#include "test_finder_ops.c"
#undef main
int main(void)
{
    /* Independent IDB1 fixture: two 24-byte records, kind=5 staged spelling
     * followed by kind=6 original spelling. Every byte is hand-authored. */
    static const uint8_t golden_origin[56]={
        'I','D','B','1',1,0,2,0,
        5,1,0,0,'R','E','A','D','M','0','0','1',0,0,0,0,0,0,0,0,0,0,0,0,
        6,0,0,0,'R','E','A','D','M','E','.','T','X','T',0,0,0,0,0,0,0,0,0,0
    };
    finder_move_result_t r;
    finder_tgt_t t;
    FinderCtx ctx={0};
    uint8_t db[4096];
    uint32_t len;
    int slot=-1,idx;
    scene_init(&S);
    mv_add(&S.mock.d[2],"README.TXT",0x20u,99);
    t=finder_ops_resolve(&S.sh,ROOT,0,600,420);
    CHECK(finder_ops_drop(&S.sh,ROOT,0,&t,0,0,&r)==FINDER_WIN_OK && r.renamed,
          "ORIGINAL_NAME collision stages a suffixed file");
    len=finder_desk_db_encode_full(&S.sh.desk,0,0,S.sh.origins,S.sh.n_origins,db,sizeof db);
    CHECK(len==104 && db[6]==4 && db[7]==0 &&
          !memcmp(db+56,golden_origin+8,48),
          "ORIGINAL_NAME encoded origin pair equals independent byte golden");
    CHECK(finder_shell_load_origins(&S.sh,golden_origin,sizeof golden_origin)==1,
          "ORIGINAL_NAME independent origin pair decodes");
    CHECK(finder_win_open_trash(&S.sh,&slot,0)==FINDER_WIN_OK,"PUT_AWAY opens Trash");
    idx=idx_of(&S.sh.windows[slot].view,r.as);
    CHECK(idx>=0,"PUT_AWAY staged item is visible");
    if(idx>=0) S.sh.windows[slot].view.icons[idx].selected=1;
    finder_shell_bind_ctx(&S.sh,&ctx);
    CHECK(ctx.exec(ctx.shell,FCMD_PUT_AWAY),"PUT_AWAY command is implemented");
    CHECK(mv_has(&S.mock,CL_ROOT,"README.TXT") && !mv_has(&S.mock,CL_TRASH,r.as),
          "ORIGINAL_NAME Put Away restores README.TXT in original root");
    CHECK(S.sh.n_origins==0,"PUT_AWAY consumes successful origin only");
    scene_init(&S);
    mv_add(&S.mock.d[2],"APPS",0x10u,99);
    t=finder_ops_resolve(&S.sh,ROOT,1,600,420);
    CHECK(finder_ops_drop(&S.sh,ROOT,1,&t,0,0,&r)==FINDER_WIN_OK && r.renamed,
          "FOLDER_ORIGIN collision stages the open folder");
    CHECK(finder_win_open_trash(&S.sh,&slot,0)==FINDER_WIN_OK,"FOLDER_ORIGIN opens Trash");
    idx=idx_of(&S.sh.windows[slot].view,r.as);
    CHECK(idx>=0 && finder_ops_put_away(&S.sh,slot,idx)==FINDER_WIN_OK,
          "FOLDER_ORIGIN restores the folder");
    CHECK(S.sh.windows[APPW].parent==CL_ROOT && !strcmp(S.sh.windows[APPW].name83,"APPS"),
          "FOLDER_ORIGIN open view follows restored identity");
    return TEST_SUMMARY("test-finder-restore");
}
