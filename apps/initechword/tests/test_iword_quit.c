/* IM VI6-36/6-37, WP5.1 Reference193/211: a native Exit may still be asking,
 * or be cancelled; callback return is not an answer. Explicit reply transport
 * is independently recorded by the SDK fake. */
#define main old_face_main
#include "test_iword_face.c"
#undef main
static void native_exit(void)
{ tbx_event_t ev={.what=TBX_EVT_MENU,.message=TBX_MENU_WORD(129,3)};tbx_app_event(&ev); }
static void fresh(int dirty)
{
 m_dir=0;tbx_host_reset();g_mode=M_EDIT;g_exiting=0;
 CHECK(tbx_app_main()==0,"quit","register fresh tenant");if(dirty)tks("Keep unsaved text.");
}
int main(void)
{
 static int counters[2];g_ctr=counters;
 fresh(0);native_exit();CHECK(m_exit_calls==1,"quit","clean native Exit completes immediately");
 fresh(1);native_exit();CHECK(g_mode==M_SAVEQ&&tbx_host.quit_replies==1&&tbx_host.quit_reply_state==1&&!m_exit_calls,"quit","dirty native Exit explicitly awaits save answer");
 tk(0x1b);CHECK(g_mode==M_EDIT&&tbx_host.quit_reply_state==0&&!m_exit_calls&&g_ed.doc.len==18,"quit","Escape cancels request and preserves unsaved text");
 native_exit();tk('n');CHECK(g_mode==M_EXITQ&&tbx_host.quit_reply_state==1,"quit","No save still awaits Exit answer");
 tk('y');CHECK(m_exit_calls==1,"quit","confirmed discard completes native Exit");
 fresh(1);native_exit();tk('y');CHECK(g_mode==M_SAVE_NAME&&tbx_host.quit_reply_state==1,"quit","Yes waits for filename");
 tk(0x1b);CHECK(g_mode==M_EDIT&&tbx_host.quit_reply_state==0&&!m_exit_calls&&g_ed.doc.len==18,"quit","filename Escape cancels while retaining unsaved text");
 native_exit();tk('y');
 tks("BAD NAME.WP5");tk('\r');CHECK(g_mode==M_ERROR&&tbx_host.quit_reply_state==0&&!m_exit_calls&&g_ed.doc.len==18,"quit","failed save cancels power request and retains text");
 fresh(1);native_exit();tk('y');tks("SAVED.WP5");tk('\r');
 CHECK(g_mode==M_EXITQ&&tbx_host.quit_reply_state==1&&!m_exit_calls,"quit","successful save still waits for the exit confirmation");
 tk('y');CHECK(m_exit_calls==1,"quit","confirmed saved exit accepts the request");
 fresh(1);g_mode=M_SAVE_NAME;native_exit();CHECK(tbx_host.quit_reply_state==0&&g_mode==M_SAVE_NAME,"quit","already occupied prompt explicitly declines another Exit");
 printf("test_iword_quit: %d checks, %d failures\n",counters[0],counters[1]);return counters[1]?1:0;
}
