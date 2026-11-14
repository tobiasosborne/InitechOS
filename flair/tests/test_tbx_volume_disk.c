/* Sec 10 volume/root contract, real independent A/C disks and mtools. */
#define main filecopy_disk_main
#include "test_filecopy_disk.c"
#undef main
static void expect_file(const char *command_line,const char *want)
{
    char out[80]; command(command_line); FILE *f=fopen("observed.dat","rb");
    CHECK(f!=0,"mtools volume output");
    if(f) { size_t n=fread(out,1,sizeof out,f); CHECK(n==strlen(want) && !memcmp(out,want,n),"independent volume/path bytes"); fclose(f); }
}
int main(void)
{
    fat12_volume_t cvol; blockdev_t cdev={0,rd,wr};
    static psp_t caller;
    static char name[]="X.TXT", foreign[]="C:X.TXT", missing[]="MISSING.DAT", replacement[]="NEW A ROOT";
    FILE *f=fopen("old.dat","wb"); fwrite("OLD A ROOT",1,10,f); fclose(f);
    f=fopen("c.dat","wb"); fwrite("OLD C ROOT",1,10,f); fclose(f);
    f=fopen("adoc.dat","wb"); fputs("A DOC DECOY",f); fclose(f);
    f=fopen("cdoc.dat","wb"); fputs("C DOC DECOY",f); fclose(f);
    command("mformat -C -i base.img -f 1440 ::"); command("mcopy -i base.img old.dat ::X.TXT");
    command("mmd -i base.img ::DOCS"); command("mcopy -i base.img adoc.dat ::DOCS/X.TXT");
    command("mformat -C -i c.img -f 1440 ::"); command("mcopy -i c.img c.dat ::X.TXT");
    command("mmd -i c.img ::OTHER"); command("mcopy -i c.img cdoc.dat ::OTHER/X.TXT");
    setup(); cdev.ctx=fopen("c.img","r+b"); CHECK(cdev.ctx!=0,"open C volume");
    CHECK(fat12_mount(&cvol,&cdev,sec)==0,"mount C"); CHECK(fileio_fat_add_drive(2,&cvol)==0,"bind C");
    flair_platform_bind_files(0); int21_set_psp(&caller);
    for(unsigned current=0;current<=2;current+=2) for(unsigned io=0;io<=2;io+=2) {
        int21_cwd_snapshot_t before={0},after;
        before.current_drive=(uint8_t)current; before.io_drive=(uint8_t)io;
        /* Each independently formatted volume allocates X.TXT at cluster 2,
         * then its directory at 3. Both snapshots name actual directories. */
        before.starts[0]=3; before.starts[2]=3;
        strcpy(before.paths[0],"DOCS"); strcpy(before.paths[2],"OTHER");
        before.start_cluster=before.starts[io]; strcpy(before.path,before.paths[io]);
        int21_cwd_restore(&before);
        int32_t h=tbx_file_open(name,TBX_FILE_READ_ONLY);
        CHECK(h>=5,"bound A root opens");
        if(h>=5) { int32_t n=tbx_file_read(h,g_tbx_save_chunk,80); CHECK(n==10 && !memcmp(g_tbx_save_chunk,"OLD A ROOT",10),"bound A root exact bytes"); CHECK(tbx_file_close(h)==0,"close A"); }
        h=tbx_file_open(foreign,TBX_FILE_READ_ONLY);
        CHECK(h>=5,"explicit C root opens");
        if(h>=5) { int32_t n=tbx_file_read(h,g_tbx_save_chunk,80); CHECK(n==10 && !memcmp(g_tbx_save_chunk,"OLD C ROOT",10),"explicit C root bytes"); CHECK(tbx_file_close(h)==0,"close C"); }
        CHECK(tbx_file_open(missing,0)<0,"error restoration control");
        after=int21_cwd_save(); CHECK(!memcmp(&before,&after,sizeof before),"all caller drive/CWD state restored");
        CHECK(int21_get_psp()==&caller,"caller PSP restored");
    }
    CHECK(tbx_file_save(name,replacement,10)==10,"save uses bound A root");
    expect_file("mcopy -o -i copy.img ::X.TXT observed.dat","NEW A ROOT");
    expect_file("mcopy -o -i copy.img ::DOCS/X.TXT observed.dat","A DOC DECOY");
    expect_file("mcopy -o -i c.img ::X.TXT observed.dat","OLD C ROOT");
    expect_file("mcopy -o -i c.img ::OTHER/X.TXT observed.dat","C DOC DECOY");
    fclose(cdev.ctx); fclose(disk);
    return TEST_SUMMARY("test-tbx-volume-disk");
}
