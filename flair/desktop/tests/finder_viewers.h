/* Independent viewer/gesture invariants, alongside the exact menu manifest. */
static int empty_metadata(void *u,uint16_t d,finder_enum_cb cb,void *cu)
{(void)u;(void)d;(void)cb;(void)cu;return 0;}
static __attribute__((unused)) void info_contexts(void)
{
 static scene_t s;FinderCtx ctx={0};finder_file_info_t info;int slot=-1;
 scene_init(&s,MOCK_DIRS,2);finder_shell_bind_ctx(&s.sh,&ctx);
 CHECK(!finder_file_info(&s.sh,&info),"desktop volume/Trash have no file Info");
 CHECK(finder_win_open(&s.sh,0,0,"",1,&slot,NULL)==0,"Info fixture opens");
 finder_desk_t *v=&s.sh.windows[slot].view;
 CHECK(!finder_file_info(&s.sh,&info),"no selection has no Info");
 v->icons[0].selected=v->icons[2].selected=1;
 CHECK(!finder_file_info(&s.sh,&info),"multiple files have no Info");
 v->icons[0].selected=v->icons[2].selected=0;v->icons[1].selected=1;
 CHECK(!finder_file_info(&s.sh,&info),"folder has no Info");v->icons[1].selected=0;
 for(int index=0;index<=2;index+=2){
  v->icons[index].selected=1;finder_shell_sync_ctx(&s.sh);finder_menu_refresh_enables(&ctx);
  CHECK(finder_file_info(&s.sh,&info)&&MenuKey(finder_menu_bar(),'I')==MenuResult(512,5),"document and EXE have Info");
  finder_dispatch(&ctx,MenuKey(finder_menu_bar(),'I'),"key");
  const finder_cmd_outcome_t *o=finder_shell_take_outcome(&s.sh);
  CHECK(o&&o->id==FCMD_GET_INFO&&o->status==0,"Info shortcut returns a bounded record");
  CHECK(v->icons[index].selected,"Info preserves selection");v->icons[index].selected=0;
 }
 v->icons[0].selected=1;s.sh.fs.enumerate=empty_metadata;
 CHECK(!finder_file_info(&s.sh,&info),"missing metadata has no Info");
 finder_shell_sync_ctx(&s.sh);finder_menu_refresh_enables(&ctx);
 CHECK(MenuKey(finder_menu_bar(),'I')==0,"missing record disables Info shortcut");
 info.size=0xffffffffu;info.date=0;info.locked=0;strcpy(info.name83,"MAX.BIN");char lines[4][64];
 finder_file_info_lines(&info,lines);
 CHECK(!strcmp(lines[1],"Size: 4294967295 bytes")&&!strcmp(lines[2],"Modified: Not recorded"),"unsigned size and absent date are honest");
}
static unsigned dialog_step;
static uint16_t viewer_button(void *user,const char *const *text,const char *const *buttons,unsigned n)
{
 static const char *first[]={"first","fourth","seventh","fourth","first"};
 static const unsigned counts[]={2,3,2,3,2};static const uint16_t hits[]={2,2,2,3,1};
 unsigned step=dialog_step++;
 CHECK(step<5,"dialog button route terminates without a loop");if(step>=5)return 1;
 CHECK(!strcmp(text[0],"Clipboard")&&!strcmp(text[1],first[step]),"dialog button changes the actual visible snapshot page");
 CHECK(n==counts[step]&&!strcmp(buttons[0],"Close"),"Close is always the default first button");
 CHECK(!strcmp(buttons[1],step==2?"Previous":"Next"),"second button changes only when Next is no longer available");
 if(n==3)CHECK(!strcmp(buttons[2],"Previous"),"middle page has Previous in slot three");
 if(step==0)CHECK(FlairPutScrap(user,FLAIR_SCRAP_TYPE_TEXT,"Other",5)==0,"external scrap changes while dialog is open");
 return hits[step];
}
static __attribute__((unused)) void clipboard(void)
{
 static FlairScrap scrap;static finder_clipboard_t view;static unsigned char input[8192],actual[8192];
 FlairScrap_init(&scrap);CHECK(finder_clipboard_snapshot(&view,&scrap)==0&&!view.length&&!view.lines[0][0],"empty snapshot blank");
 CHECK(FlairPutScrap(&scrap,FLAIR_SCRAP_TYPE_PICT,"picture",7)==0,"PICT fixture");
 CHECK(finder_clipboard_snapshot(&view,&scrap)==0&&!view.length,"PICT-only snapshot blank, no false TEXT");
 const char fixture[]="first\rsecond\rthird\rfourth\rfifth\rsixth\rseventh\rUNIQUE-TAIL";
 CHECK(FlairPutScrap(&scrap,FLAIR_SCRAP_TYPE_TEXT,fixture,sizeof fixture-1)==0,"CR fixture");
 int32_t generation=FlairInfoScrap(&scrap);
 CHECK(finder_clipboard_snapshot(&view,&scrap)==0,"snapshot reads TEXT");
 CHECK(!strcmp(view.lines[0],"first")&&!strcmp(view.lines[1],"second")&&!strcmp(view.lines[2],"third"),"literal first page");
 CHECK(FlairInfoScrap(&scrap)==generation,"snapshot preserves generation");
 CHECK(FlairPutScrap(&scrap,FLAIR_SCRAP_TYPE_TEXT,"Changed",7)==0,"external change");
 finder_clipboard_next(&view);CHECK(!strcmp(view.lines[0],"fourth"),"Next uses old snapshot");
 finder_clipboard_next(&view);CHECK(!strcmp(view.lines[0],"seventh")&&!strcmp(view.lines[1],"UNIQUE-TAIL")&&view.next==view.length,"literal last page");
 uint32_t last=view.start;finder_clipboard_next(&view);CHECK(view.start==last,"Next clamps");
 finder_clipboard_previous(&view);CHECK(!strcmp(view.lines[0],"fourth"),"Previous returns prior page");
 finder_clipboard_previous(&view);finder_clipboard_previous(&view);CHECK(view.start==0&&!strcmp(view.lines[0],"first"),"Previous clamps");
 uint32_t n=0;char now[8];CHECK(FlairGetScrap(&scrap,FLAIR_SCRAP_TYPE_TEXT,now,sizeof now,&n)==7&&n==7&&!memcmp(now,"Changed",7),"viewer preserves current scrap");
 CHECK(FlairPutScrap(&scrap,FLAIR_SCRAP_TYPE_TEXT,fixture,sizeof fixture-1)==0,"dialog CR fixture");dialog_step=0;
 CHECK(finder_clipboard_show(&view,&scrap,viewer_button,&scrap)==0&&dialog_step==5,"Next/Previous/Close button routes traverse complete snapshot once");
 CHECK(FlairGetScrap(&scrap,FLAIR_SCRAP_TYPE_TEXT,now,sizeof now,&n)==5&&!memcmp(now,"Other",5),"modal controller preserves changed shared scrap");
 memset(input,'X',sizeof input);memcpy(input+sizeof input-11,"UNIQUE-TAIL!",11);
 CHECK(FLAIR_SCRAP_MAX_PAYLOAD==8192&&FlairPutScrap(&scrap,FLAIR_SCRAP_TYPE_TEXT,input,sizeof input)==0,"full public-capacity fixture");
 CHECK(finder_clipboard_snapshot(&view,&scrap)==0&&view.length==8192,"full capacity not truncated");
 unsigned count=0,pages=0;
 for(;;){
  for(unsigned r=0;r<3;r++)for(unsigned c=0;view.lines[r][c];c++)if(count<sizeof actual)actual[count++]=(unsigned char)view.lines[r][c];
  pages++;if(view.next==view.length||pages>8192)break;
  uint32_t old=view.start;finder_clipboard_next(&view);CHECK(view.start>old,"Next makes byte progress");
 }
 CHECK(count==8192&&!memcmp(input,actual,8192),"every byte visibly preserved in order including unique tail");
 uint32_t length=0;CHECK(FlairGetScrap(&scrap,FLAIR_SCRAP_TYPE_TEXT,actual,sizeof actual,&length)==8192&&!memcmp(input,actual,8192),"complete paste payload unchanged");
 scrap.flavors[1].length=8193;CHECK(finder_clipboard_snapshot(&view,&scrap)<0,"size/read error is never silent truncation");
 CHECK(finder_clipboard_snapshot(&view,NULL)<0,"unbound scrap is an error");
}
static __attribute__((unused)) void gestures(void)
{
 finder_click_track_t track;finder_desk_icon_t icons[1];finder_desk_t desk;
 finder_desk_init(&desk,icons,1,(rgn_rect_t){0,0,480,640});desk.n=1;
 const int kinds[]={FINDER_ICON_FILE,FINDER_ICON_FOLDER,FINDER_ICON_APP,FINDER_ICON_TRASH,FINDER_ICON_VOLUME};
 for(unsigned i=0;i<sizeof kinds/sizeof kinds[0];i++){
  icons[0].kind=(uint8_t)kinds[i];finder_click_reset(&track);
  CHECK(finder_desk_click_classify(&track,&desk,0,30,50,100)==FINDER_CLICK_SINGLE,"first click selects");
  CHECK(finder_desk_click_classify(&track,&desk,0,30,50,101)==(i?FINDER_CLICK_DOUBLE:FINDER_CLICK_SINGLE),"document selects; folder/app/Trash/volume double-click opens");
 }
}
