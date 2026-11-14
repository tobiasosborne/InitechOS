/* GM3: real modal + Event Manager + file engine + DOS/FAT; mtools oracle.
 * Only presentation/serial/app-completion bindings are hosted. Set-1 input
 * reaches the same finder_filecopy_run loop as the booted INT 81h path. */
#define main disk_suite_main
#include "test_filecopy_disk.c"
#undef main
static void modal_answer(int32_t result);
#define tbx_filecopy_answer modal_answer
#include "desktop/finder_actions.c"
#undef tbx_filecopy_answer
DialogRecord g_fc_dr;
DialogItem g_fc_it[2];
ControlRecord g_fc_bar;
finder_win_rgn_t g_fc_rs,g_fc_rc,g_fc_ru;
flair_raw_ring_t g_flair_kbd_ring;
static unsigned replies,ticks,mode;
static int32_t answer;
static void modal_answer(int32_t result) { replies++; answer=result; }
void serial_putc(char c) { (void)c; }
void serial_puti(int32_t n) { (void)n; }
void serial_putu(uint32_t n) { (void)n; }
void serial_puts(const char *s) { (void)s; }
void flair_live_emit_evt(const EventRecord *ev) { (void)ev; }
void flair_live_cursor_track(const EventRecord *ev) { (void)ev; }
void flair_desktop_present(const boot_info_t *bi,const bitmap_t *off) { (void)bi; (void)off; }
void finder_desk_repaint(flair_live_ctx_t *ctx,const boot_info_t *bi) { (void)ctx; (void)bi; }
void flair_live_tenant_service(flair_live_ctx_t *ctx,const boot_info_t *bi) { (void)ctx; (void)bi; }
void fta_rgn(finder_win_rgn_t *s)
{
 memset(s,0,sizeof *s); s->r.rows=s->rows; s->r.cap_rows=FINDER_WIN_RGN_ROWS;
 s->r.x_pool=s->pool; s->r.x_pool_cap=FINDER_WIN_RGN_POOL; region_set_empty(&s->r);
}
static void raw(flair_raw_kind_t kind,uint32_t payload)
{
 flair_raw_event_t ev={kind,ticks,payload};
 CHECK(flair_raw_post(&g_flair_kbd_ring,&ev),"post physical cancel trace");
}
static void tick(void)
{
 flair_tick_advance();
 if(++ticks!=50) return;
 if(mode<2) raw(FLAIR_RAW_KEYBOARD,1); /* physical Escape, ASCII zero */
 else if(mode==2) { raw(FLAIR_RAW_KEYBOARD,0x1d); raw(FLAIR_RAW_KEYBOARD,0x34); }
 else {
  flair_event_warp(mode==3?450:150,mode==3?288:210);
  raw(FLAIR_RAW_MOUSE,1); raw(FLAIR_RAW_MOUSE,0);
 }
}
int main(void)
{
 FILE *f;
 for(unsigned i=0;i<sizeof source;i++) source[i]=(unsigned char)(i*31u+7u);
 f=fopen("old.dat","wb"); CHECK(f!=0,"old fixture"); fwrite(old,1,sizeof old-1,f); fclose(f);
 f=fopen("source.dat","wb"); CHECK(f!=0,"source fixture"); fwrite(source,1,sizeof source,f); fclose(f);
 command("mformat -C -i base.img -f 1440 ::");
 command("mcopy -i base.img old.dat ::TARGET.DAT");
 command("mcopy -i base.img source.dat ::SOURCE.DAT");
 static uint8_t pixels[640*480];
 static finder_win_rgn_t backing[4];
 WindowMgr wm;
 flair_live_ctx_t ctx={0};
 for(unsigned i=0;i<4;i++) fta_rgn(&backing[i]);
 WindowMgr_init(&wm,(rgn_rect_t){0,0,480,640},&backing[0].r,&backing[1].r,&backing[2].r,&backing[3].r);
 ctx.wm=&wm; ctx.off=(bitmap_t){.base=pixels,.pitch=640,.width=640,.height=480,.bpp=8,.bytes_per_pixel=1};
 for(mode=0;mode<5;mode++) {
  setup();
  if(mode==1) {
   int32_t h=fc_dos(0x3c00,0,0,flair_file_ptr("SOURCE.DAT"));
   CHECK(h>=0 && fc_dos(0x3e00,(uint32_t)h,0,0)>=0,"empty modal source");
  }
  fclose(disk);
  f=fopen("rate.cfg","wb"); CHECK(f!=0,"rate fixture"); fputs("PROGRESS_BAR_RATE=100\r\n",f); fclose(f);
  command("mmd -i copy.img ::APPS"); command("mcopy -i copy.img rate.cfg ::APPS/MICHAEL.CFG");
  disk=fopen("copy.img","r+b"); CHECK(fileio_fat_bind(&vol)==0,"reload modal disk");
  replies=ticks=0; answer=0;
  flair_event_init(&g_flair_kbd_ring); flair_event_set_yield(tick);
  finder_filecopy_run(&ctx,0,"Copying test data...","SOURCE.DAT","TARGET.DAT");
  CHECK(replies==1 && answer==TBX_ERR_BUSY,"all live cancel routes deliver exactly one cancelled completion");
  if(mode==1) {
   recovered_stream("mcopy -o -i copy.img ::TARGET.DAT recovered.dat",old,sizeof old-1);
   recovered_stream("mcopy -o -i copy.img ::SOURCE.DAT recovered.dat","",0);
  } else verify(old,sizeof old-1);
  CHECK(tbxf_live(&g_slot.files)==0,"modal cancel releases file handles");
  fclose(disk);
 }
 return TEST_SUMMARY("test-copy-modal");
}
