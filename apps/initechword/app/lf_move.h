/* List Files Move/Rename, initech-evky3. Visible prompts/selection come from
 * real WP5.1 fixtures/iword-listfiles-ops/grids/{13_move_replace,
 * 17_move_success,33_rename_selection} (Reference pp.347-353). The reported-
 * failure ruling additionally protects every prior destination through source
 * removal AND list refresh. No crash-atomicity claim. */
static char g_lf_move_old[TBX_FILE_PATH_MAX];
static char g_lf_move_stage[TBX_FILE_PATH_MAX];
typedef struct lf_move { int old,copied,published,removed; } lf_move_t;

static int32_t lf_move_residual(int existed)
{
    /* A second I/O failure refused rollback. Retain all complete streams and
     * name the preceding destination exactly; block writes until recovery.
     * Known residual closes with kernel replace primitive initech-7mb66. */
    scopy(g_word_original, existed ? g_lf_move_old : g_lf_dst);
    scopy(g_word_recovery, "ERROR: Original retained in ");
    scopy(g_word_recovery+slen(g_word_recovery),g_word_original);
    g_word_blocked=1;
    return TBX_ERR_BADARG;
}

static int32_t lf_move_undo(lf_move_t *s,int32_t failed)
{
    /* A failed refresh may follow source removal. Restore the source first;
     * the old destination remains separately retained throughout. */
    if(s->removed){
        int32_t rc=s->copied?tbx_file_copy_save(g_lf_dst,g_lf_src):
                             tbx_file_rename(g_lf_dst,g_lf_src);
        if(rc<0)return lf_move_residual(s->old);
        s->removed=0;
        if(!s->copied)s->published=0;
    }
    /* Use metadata, not DELETE, to get a complete copy out of the way:
     * a persistent source-delete error must not prevent restoration. */
    if(s->published&&tbx_file_rename(g_lf_dst,g_lf_move_stage)<0)
        return lf_move_residual(s->old);
    if(s->old&&tbx_file_rename(g_lf_move_old,g_lf_dst)<0)
        return lf_move_residual(s->old);
    s->old=0;
    if(s->published)(void)tbx_file_delete(g_lf_move_stage);
    return failed;
}

static int32_t lf_move_file(lf_move_t *s)
{
    /* New same-directory names need neither data space nor source DELETE. */
    int32_t rc=tbx_file_rename(g_lf_src,g_lf_dst);
    if(rc==TBX_OK){s->published=s->removed=1;return rc;}
    if(rc!=TBX_ERR_DOS(17)&&rc!=TBX_ERR_DOS(5))return rc;
    /* Cross-directory rollback after a failed refresh must be able to copy
     * back to the source. Match the shared helper's recovery-path preflight
     * before moving either predecessor (removed with initech-7mb66). */
    if(rc==TBX_ERR_DOS(17)&&
       (!word_extension(g_lf_move_stage,g_lf_src,"$$$")||slen(g_lf_move_stage)>60u))
        return TBX_ERR_BADARG;

    int existed=0;
    int32_t h=tbx_file_open(g_lf_dst,TBX_FILE_READ_ONLY);
    if(h>=0){
        existed=1;rc=tbx_file_close(h);if(rc<0)return rc;
    }else if(h!=TBX_ERR_DOS(2))return h;
    /* The second name receives a completed copy during rollback. Skip both
     * operands and all occupied candidates; never truncate an unowned file. */
    rc=word_reserve_path(g_lf_move_stage,g_lf_dst,"N00",g_lf_src);
    if(rc<0)return rc;
    if(existed){
        rc=word_reserve_path(g_lf_move_old,g_lf_dst,"M00",g_lf_src);
        if(rc<0)return rc;
        rc=tbx_file_rename(g_lf_dst,g_lf_move_old);
        if(rc<0)return rc;
        s->old=1;
    }

    rc=tbx_file_rename(g_lf_src,g_lf_dst);
    if(rc==TBX_OK)s->published=s->removed=1;
    if(rc==TBX_ERR_DOS(17)||rc==TBX_ERR_DOS(5)){
        /* The helper sees an absent destination. Its own successful cleanup
         * therefore cannot release our retained destination predecessor. */
        rc=tbx_file_copy_save(g_lf_src,g_lf_dst);
        if(rc==TBX_OK){
            s->copied=s->published=1;rc=tbx_file_delete(g_lf_src);
            if(rc==TBX_OK)s->removed=1;
        }
    }
    return rc<0?lf_move_undo(s,rc):rc;
}

static void lf_move_cleanup(lf_move_t *s)
{
    /* Source removal and refresh completed: Move committed. Failed cleanup
     * retains an extra predecessor, never reports a failed Move. */
    if(s->old)(void)tbx_file_delete(g_lf_move_old);
}
