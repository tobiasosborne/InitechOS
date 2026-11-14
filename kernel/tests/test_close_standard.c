/* Real DOS3.30 CLOSE3 probe: tests/fixtures/close3-dos33. */
#define main original_fileio_main
#include "test_fileio.c"
#undef main
static int_frame_t call_close(unsigned ax,unsigned bx,unsigned cx,unsigned dx)
{ int_frame_t f=fresh_frame();f.eax=ax;f.ebx=bx;f.ecx=cx;f.edx=dx;int21_dispatch(&f);return f; }
int main(void)
{
    int21_set_break_flag(0);uint32_t name=low_dup("HELLO.TXT");
    for(unsigned standard=0;standard<5;standard++) {
        bind_standard_process();mock_reset_dir();int21_set_file_backend(&g_mock_backend);
        unsigned idx=g_test_psp.jft[standard];sft_entry_t saved=g_sft[idx];
        int_frame_t f=call_close(0x3e00,standard,0,0);
        CHECK(!frame_cf(&f) && g_test_psp.jft[standard]==JFT_CLOSED,"CLOSE standard releases JFT slot");
        CHECK(!memcmp(&saved,&g_sft[idx],sizeof saved),"CLOSE standard preserves pinned resident device SFT");
        f=call_close(0x3e00,standard,0,0);CHECK(frame_cf(&f) && (f.eax&65535)==6,"second CLOSE standard returns invalid handle");
        f=call_close(0x3d00,0,0,name);unsigned h=f.eax&65535;
        CHECK(!frame_cf(&f) && h==standard,"OPEN reuses lowest closed standard slot (genuine CLOSE3-OK)");
        sft_entry_t *e=sft_from_handle(&g_test_psp,h);unsigned file_idx=g_test_psp.jft[h];
        CHECK(e && e->ref_count==1,"opened file owns one reference");
        f=call_close(0x3e00,h,0,0);
        CHECK(!frame_cf(&f) && g_test_psp.jft[h]==JFT_CLOSED && g_sft[file_idx].kind==SFT_KIND_FREE,
              "file in standard-number slot closes its real SFT reference");
    }
    return TEST_SUMMARY("close-standard");
}
