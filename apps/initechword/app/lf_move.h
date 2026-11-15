/* WP5.1 List Files Move/Rename (Reference347-353 and fixtures
 * iword-listfiles-ops). Same-directory replacement uses FILE_REPLACE.
 * Across directories DOS Move is copy then delete: the verified replacement
 * commits first; source removal is cleanup. Operator ruling2026-11-15:
 * precommit errors preserve both originals, postcommit cleanup errors name
 * the complete source left behind. A second I/O error never rolls back a
 * committed replacement. */
static int32_t lf_move_file(int *source_left)
{
    int32_t rc=tbx_file_rename(g_lf_src,g_lf_dst);
    *source_left=0;
    if(rc==TBX_OK)return rc;
    if(rc!=TBX_ERR_DOS(17)&&rc!=TBX_ERR_DOS(5))return rc;
    rc=tbx_file_replace(g_lf_src,g_lf_dst);
    if(rc==TBX_OK)return rc;
    if(rc!=TBX_ERR_DOS(17)&&rc!=TBX_ERR_DOS(2))return rc;
    rc=tbx_file_copy_save(g_lf_src,g_lf_dst);
    if(rc<0)return rc;
    if(tbx_file_delete(g_lf_src)<0)*source_left=1;
    return TBX_OK;
}
