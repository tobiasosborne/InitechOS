/* MS-DOS 3.3 Programmer's Reference, functions 3Dh, 3Fh, 45h, 46h:
 * https://www.pcjs.org/documents/books/mspl13/msdos/dosref33/
 * Literal results; existing in-memory fixture supplies only file transport.
 * Missing service refusal: CLAUDE.md, How the tree is organised.
 */
#define main original_fileio_main
#include "test_fileio.c"
#undef main

static int_frame_t invoke(unsigned ax, unsigned bx, unsigned cx, unsigned dx)
{
    int_frame_t f = fresh_frame();
    f.eax=ax; f.ebx=bx; f.ecx=cx; f.edx=dx;
    int21_dispatch(&f); return f;
}
static void reset(void)
{
    bind_standard_process(); mock_reset_dir(); int21_cwd_reset();
    int21_set_file_backend(&g_mock_backend);
}
int main(void)
{
    int21_set_sink(sink_capture); int21_set_break_flag(0);
    uint32_t name=low_dup("HELLO.TXT"), buf=(uint32_t)(uintptr_t)alloc_low(64);
    CHECK(name && buf, "low buffers available");
    reset();
    int_frame_t f=invoke(0x3d00,0,0,name);
    CHECK(!frame_cf(&f) && f.eax==5, "fixture OPEN returns handle 5");
    f=invoke(0x4500,5,0,0);
    CHECK(!frame_cf(&f) && (f.eax&65535)==6, "7ex7t DUP returns AX=6, not 4506");
    for(unsigned n=0;n<23;n++) {
        unsigned bad=n<20 ? 0x100+n : n==20 ? 0x10005 : n==21 ? 0x80000001 : 0xffffffff;
        for(unsigned op=0;op<3;op++) {
            reset(); (void)invoke(0x3d00,0,0,name);
            psp_t before=g_test_psp; sft_entry_t table[SFT_MAX_ENTRIES];
            memcpy(table,g_sft,sizeof table);
            f=invoke(op==0 ? 0x4500 : 0x4600,op==2 ? 5 : bad,op==2 ? bad : 1,0);
            CHECK(frame_cf(&f) && (f.eax&65535)==6, "7ex7t invalid full handle -> error 6");
            CHECK(!memcmp(&before,&g_test_psp,sizeof before) && !memcmp(table,g_sft,sizeof table),
                  "7ex7t invalid DUP leaves JFT/SFT unchanged");
        }
    }
    for(unsigned mode=0;mode<256;mode++) {
        if((mode&7)<3 && !(mode&8) && (mode&0x70)<=0x40) continue;
        reset(); f=invoke(0x3d00|mode,0,0,name);
        CHECK(frame_cf(&f) && (f.eax&65535)==12, "bcg.14 invalid OPEN mode -> error 12");
        CHECK(!sft_file_slots_in_use(), "bcg.14 invalid OPEN commits no slot");
    }
    reset(); f=invoke(0x3d81,0,0,name); unsigned h=f.eax;
    memcpy((void *)(uintptr_t)buf,"xyz",3);
    f=invoke(0x4000,h,3,buf);
    CHECK(!frame_cf(&f) && f.eax==3 && !memcmp(g_mock[0].data,"xyz",3), "bcg.14 AL=81 writes three literal bytes");
    CHECK(sft_from_handle(&g_test_psp,h)->file_offset==3, "bcg.14 write advances exactly three bytes");
    for(unsigned mode=1;mode<=0x81;mode+=0x80) {
        reset(); f=invoke(0x3d00|mode,0,0,name); h=f.eax;
        memset((void *)(uintptr_t)buf,0x5a,64);
        f=invoke(0x3f00,h,1,buf);
        CHECK(frame_cf(&f) && (f.eax&65535)==5, "vwo7d write-only READ -> access denied");
        CHECK(*(uint8_t *)(uintptr_t)buf==0x5a && sft_from_handle(&g_test_psp,h)->file_offset==0,
              "vwo7d denied READ preserves buffer and offset");
    }
    const char *nonfiles[]={"SUB","DISK"};
    for(unsigned i=0;i<2;i++) {
        reset(); f=invoke(0x3d00,0,0,low_dup(nonfiles[i]));
        CHECK(frame_cf(&f) && (f.eax&65535)==5 && !sft_file_slots_in_use(),
              "vwo7d directory/volume OPEN denies without slot commit");
    }
    reset(); f=invoke(0x3d00,0,0,name); h=f.eax;
    int21_file_backend_t missing=g_mock_backend; missing.read_at=0;
    int21_set_file_backend(&missing);
    f=invoke(0x3f00,h,1,buf);
    CHECK(frame_cf(&f) && (f.eax&65535)==5, "j8kmv missing READ service fails visibly");
    f=invoke(0x3f00,h,0,0);
    CHECK(!frame_cf(&f) && f.eax==0, "j8kmv zero-length READ needs no service");
    int21_set_file_backend(&g_mock_backend);
    (void)invoke(0x4202,h,0,0); f=invoke(0x3f00,h,1,buf);
    CHECK(!frame_cf(&f) && f.eax==0, "j8kmv genuine EOF remains successful");
    return TEST_SUMMARY("review-handles");
}
