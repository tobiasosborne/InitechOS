/* Authored safety policy: README.md's operator failed-save ruling;
 * GM3/GM4; CREATE_NEW ownership: sdk/include/toolbox_gate.h Sec 10b.
 * Run the real tenant over sdk/host. The shell independently compares the
 * exported disk files with its pre-save snapshot, never Calc serialization.
 * FILE COPY completion is a fixture of its owner's stage/commit contract;
 * this suite checks Calc's routing, ownership, handles and error handling. */
#include <stdio.h>
#include <string.h>
#include "tbx_host.h"

static const char *open_fault;
static int copy_rejected;
static unsigned reserve_calls, copy_calls;
static int32_t fault_open(const char *path, uint32_t mode)
{
    if(open_fault && !strcmp(path,open_fault)) {
        open_fault=0;
        return TBX_ERR_DOS(5);
    }
    return tbx_file_open(path,mode);
}
static int32_t count_reserve(const char *path)
{
    reserve_calls++;
    return tbx_file_create_new(path);
}
static int32_t fault_copy(const char *caption, const char *src, const char *dst)
{
    copy_calls++;
    if(copy_rejected) return TBX_ERR_BUSY;
    return tbx_file_copy(caption,src,dst);
}
#define tbx_file_open fault_open
#define tbx_file_create_new count_reserve
#define tbx_file_copy fault_copy
#include "../calc.c"
#undef tbx_file_open
#undef tbx_file_create_new
#undef tbx_file_copy

static unsigned checks, failures;
#define CHECK(c,s) do { checks++; if(!(c)) { failures++; fprintf(stderr,"FAIL save safety: %s\n",s); } } while(0)
static void event(int32_t what, int32_t message)
{
    tbx_event_t e={what,message,0,0,0,0};
    tbx_app_event(&e);
}
static unsigned handles(void)
{
    unsigned n=0;
    for(unsigned i=0;i<TBX_HOST_HANDLES;i++) n+=!!tbx_host.handles[i].used;
    return n;
}
static unsigned calls(const char *name)
{
    unsigned n=0;
    for(unsigned i=0;i<tbx_host.nops;i++) n+=!strcmp(tbx_host.ops[i].name,name);
    return n;
}
static void complete(int32_t result)
{
    if(result>=0) {
        const tbx_host_file_t *f=tbx_host_file(tbx_host.copy_src);
        CHECK(f!=0,"FILE COPY has a readable source");
        if(f) CHECK(tbx_host_put_file(tbx_host.copy_dst,f->data,f->size),"FILE COPY success fixture");
    }
    event(TBX_EVT_FILECOPY,result);
}
int main(int argc, char **argv)
{
    (void)count_reserve; /* Also compile against the pre-CREATE_NEW tenant. */
    if(argc!=3) return 2;
    const char *scenario=argv[1];
    int probe=!strcmp(scenario,"destination-open-error") || !strcmp(scenario,"destination-close-error");
    int cancel=!strcmp(scenario,"replace-no");
    int copy_error=!strcmp(scenario,"copy-error") || !strcmp(scenario,"copy-cancel");
    int success=!strcmp(scenario,"stage-open-error") || !strcmp(scenario,"collision");
    tbx_host_reset();
    CHECK(tbx_host_import(argv[2]),"import independent on-disk fixture");
    CHECK(tbx_app_main()==0,"Calc launches");
    event(keyDown,'7'); event(keyDown,'3'); event(keyDown,'\r');
    /* Enter through Quit > Save so failures also prove that unsaved work
     * remains open instead of fulfilling a stale deferred Quit. */
    event(TBX_EVT_MENU,TBX_MENU_WORD(129,2));
    if(!strcmp(scenario,"destination-open-error")) open_fault="APPS\\TABLES.TXT";
    if(!strcmp(scenario,"destination-close-error")) tbx_host.fail_close=1;
    event(TBX_EVT_ALERT,TBX_ALERT_DEFAULT);
    if(probe) {
        CHECK(copy_calls==0,"destination probe failure never starts FILE COPY");
        CHECK(g_ask==ASK_NOTICE && !g_quit_after_save,"destination probe error is reported and cancels deferred Quit");
    } else {
        CHECK(g_ask==ASK_REPLACE && copy_calls==0,"existing destination asks before replacement");
        if(!strcmp(scenario,"stage-open-error")) open_fault="APPS\\TABLES.TMP";
        if(!strcmp(scenario,"create-error")) tbx_host.file_error[TBX_FILE_CREATE_NEW]=5;
        if(!strcmp(scenario,"write-error")) tbx_host.file_error[TBX_FILE_WRITE]=5;
        if(!strcmp(scenario,"write-zero")) tbx_host.write_budget=0;
        if(!strcmp(scenario,"write-short")) tbx_host.write_budget=11;
        if(!strcmp(scenario,"stage-close-error")) tbx_host.fail_close=1;
        if(!strcmp(scenario,"copy-rejected")) copy_rejected=1;
        event(TBX_EVT_ALERT,cancel?TBX_ALERT_CANCEL:TBX_ALERT_DEFAULT);
        CHECK(handles()==0,"every acquired handle is closed, including failed writes");
        if(success || copy_error) {
            CHECK(copy_calls==1 && g_copying,"completed stage goes through FILE COPY");
            CHECK(!strcmp(tbx_host.copy_caption,"Saving tables to disk..."),"canon FILE COPY caption");
            CHECK(!strcmp(tbx_host.copy_src,success?"APPS\\TABLE~1.TMP":"APPS\\TABLES.TMP"),"copy uses exclusively owned stage");
            complete(copy_error?(!strcmp(scenario,"copy-cancel")?TBX_ERR_BUSY:TBX_ERR_DOS(39)):0);
        } else if(cancel) {
            CHECK(copy_calls==0 && reserve_calls==0,"declining replace creates nothing");
        } else {
            CHECK(g_ask==ASK_NOTICE && !g_quit_after_save,"save failure is reported and cancels deferred Quit");
            CHECK(copy_calls==(!strcmp(scenario,"copy-rejected")?1u:0u),"failed stage never reaches FILE COPY");
        }
        if(!strcmp(scenario,"create-error")) CHECK(reserve_calls==1,"only file-exists advances to another candidate");
        if(!strcmp(scenario,"all-collisions")) CHECK(reserve_calls==4,"all four names reserved exclusively before safe refusal");
    }
    CHECK(handles()==0,"no open handles after completion/error");
    CHECK(!g_staged && !g_copying,"no stage ownership or copy left after terminal result");
    CHECK(calls("tbx_file_create")==0,"no truncating CREATE in Calc");
    if(success) {
        CHECK(tbx_host.exit_calls==1 && !g_dirty,"only successful FILE COPY completes deferred Quit");
    } else {
        CHECK(tbx_host.exit_calls==0 && g_dirty,"failure/cancel retains unsaved work and session");
        /* Clear fixture faults, dismiss the notice, and retry. Reaching a
         * new copy proves the failed save did not wedge the live app. */
        open_fault=0; copy_rejected=0; tbx_host.fail_close=0;
        memset(tbx_host.file_error,0,sizeof tbx_host.file_error);
        tbx_host.write_budget=-1;
    }
    CHECK(tbx_host_export(argv[2]),"export disk for independent shell readback");
    if(!success && strcmp(scenario,"all-collisions")) {
        event(TBX_EVT_ALERT,TBX_ALERT_DEFAULT);
        unsigned before=copy_calls;
        event(TBX_EVT_MENU,TBX_MENU_WORD(129,1));
        event(TBX_EVT_ALERT,TBX_ALERT_DEFAULT);
        CHECK(copy_calls==before+1 && g_copying,"retry can start another FILE COPY");
        complete(0);
        CHECK(!g_dirty && tbx_host.exit_calls==0,"retry succeeds without stale deferred Quit");
        CHECK(handles()==0,"retry closes all handles");
    }
    printf("test_calc_save_safety: %u checks, %u failures (%s)\n",checks,failures,scenario);
    return failures?1:0;
}
