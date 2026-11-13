/* Independent genuine MS-DOS 3.30 results; fixtures/search33/README.md. */
#define main original_fileio_main
#include "test_fileio.c"
#undef main
static uint16_t entries(uint32_t index,uint16_t dir,dir_entry_t *de,int *found)
{
    (void)dir;*found=index<7;
    if(*found)fill_dir_entry(&g_mock[index],de);
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    FILE *ref=fopen(argv[1],"rb");if(!ref)return 2;
    unsigned char expected[896];
    if(fread(expected,1,sizeof expected,ref)!=sizeof expected || fgetc(ref)!=EOF)return 2;
    fclose(ref);
    static const uint8_t attrs[]={0x20,0x22,0x24,0x26,0x10,8,0x16};
    static const char names[][9]={"A0      ","A1      ","A2      ","A3      ","A4      ","A5      ","A6      "};
    bind_standard_process(); int21_set_sink(sink_capture); mock_reset_dir();
    int21_cwd_reset(); int21_file_backend_t backend=g_mock_backend;
    backend.dir_entry=entries;int21_set_file_backend(&backend);
    memset(g_mock,0,sizeof g_mock);
    for(unsigned i=0;i<7;i++) {
        g_mock[i].name8=names[i];g_mock[i].ext3="   ";g_mock[i].attr=attrs[i];g_mock[i].present=1;
    }
    int_frame_t f=fresh_frame(); f.eax=0x1a00;f.edx=(uint32_t)(uintptr_t)alloc_low(64);int21_dispatch(&f);
    unsigned at=0;
    for(unsigned mask=0;mask<64;mask++)for(unsigned i=0;i<7;i++)for(unsigned wild=0;wild<2;wild++) {
        char path[20];snprintf(path,sizeof path,"A%u%s",i,wild?".*":"");
        f=fresh_frame();f.eax=0x4e00;f.ecx=mask;f.edx=low_dup(path);int21_dispatch(&f);
        if(frame_cf(&f)!=expected[at])fprintf(stderr,"search33 mask=%02x attr=%02x wild=%u expectedCF=%u gotCF=%d\n",mask,attrs[i],wild,expected[at],frame_cf(&f));
        CHECK(frame_cf(&f)==expected[at],"g8x3 exact/wild result vs genuine DOS 3.30 capture");at++;
    }
    return TEST_SUMMARY("search33");
}
