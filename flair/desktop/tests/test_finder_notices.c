/* tdnl.62/.66/.72/.57: expected words are the operator/bead wording.
 * No native text capture is claimed. Chrome is graded in QEMU against
 * system7-decomp/goldens/captures/s8_alert_modal.png. */
#define main legacy_windows_main
#include "test_finder_windows.c"
#undef main
int main(void)
{
    char lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX], joined[400];
    const struct { finder_win_status_t code; const char *words; } cases[]={
        {FINDER_WIN_ERR_DISKFULL,"The disk is full"},
        {FINDER_WIN_ERR_FULL,"folder could not be opened"},
        {FINDER_WIN_ERR_MEMORY,"not enough memory"},
        {FINDER_WIN_ERR_CYCLE,"inside itself"},
        {FINDER_WIN_ERR_SERVICE,"used by the Finder"},
        {FINDER_WIN_ERR_MOVE,"could not be moved"},
        {FINDER_WIN_ERR_EXISTS,"already exists"},
        {FINDER_WIN_ERR_LAUNCH,"could not be opened"},
        {FINDER_WIN_ERR_CRASH,"unexpectedly quit"},
        /* bead initech-vnl4 (audit 2026-10-23 desktop L007): the unavailable
         * Trash names the Trash, never the move wording. */
        {FINDER_WIN_ERR_NOTRASH,"The Trash could not be opened"}
    };
    for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++) {
        int n=finder_error_alert_text(cases[i].code,"README.TXT",lines);
        joined[0]=0;
        for(int k=0;k<n;k++) { if(k) strcat(joined," "); strcat(joined,lines[k]); }
        CHECK(n>0 && strstr(joined,cases[i].words),cases[i].words);
        if(cases[i].code==FINDER_WIN_ERR_NOTRASH)
            CHECK(!strstr(joined,"could not be moved"),"NOTRASH_WORDING no move was asked for");
        if(cases[i].code==FINDER_WIN_ERR_EXISTS)
            CHECK(!strcmp(joined,"An item named README.TXT already exists in this location. Do you want to replace it with the one you're moving?"),
                  "REPLACE_WORDING full operator-specified prompt survives wrapping");
    }
    {
        /* bead initech-vnl4 (audit L001): the Delete Now confirm, whole. */
        int n=finder_delete_now_alert_text("README.TXT",lines);
        joined[0]=0;
        for(int k=0;k<n;k++) { if(k) strcat(joined," "); strcat(joined,lines[k]); }
        CHECK(n>1 && !strcmp(joined,"There is no room on the disk for the Trash. Are you sure you want to remove README.TXT permanently?"),
              "DELETE_NOW_WORDING the full confirm survives wrapping");
    }
    {
        static scene_t s;
        int slot=-1;
        scene_init(&s,MOCK_DIRS,2);
        CHECK(finder_win_open(&s.sh,0,0,"",1,&slot,0)==FINDER_WIN_OK,
              "DISK_FULL_ROUTE opens real model");
        s.mock.mkdir_rc=FINDER_WIN_ERR_DISKFULL;
        CHECK(finder_win_new_folder(&s.sh,slot,0,0)==FINDER_WIN_ERR_DISKFULL,
              "DISK_FULL_ROUTE mkdir error reaches the alert mapper unchanged");
    }
    return TEST_SUMMARY("test-finder-notices");
}
