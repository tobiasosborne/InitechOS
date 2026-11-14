/* Included by face_a.c: private File chooser/list/session-prefix machinery.
 * R2.2 Reference 5-13..18; tests/fixtures/files captures precede this code.
 * Every pointer passed through the file gate refers to module/BSS storage. */
#define FILE_CAP 511u
static tbx_find_rec_t g_files[FILE_CAP+1],g_probe_rec;
static char g_probe[TBX_FILE_PATH_MAX],g_folder[TBX_FILE_PATH_MAX];
static char g_pattern[13],g_selected[TBX_FILE_PATH_MAX];
static unsigned g_file_count,g_file_sel,g_file_op,g_file_category;
static int g_file_full,g_file_confirm;
static const char *const FILE_FILTERS[]={"*.WK?","*.PRN","*.PIC","*.*"};
static char upper(char c){return c>='a'&&c<='z'?(char)(c-32):c;}
static int resolve_path(const char *input,char *out)
{
    char source[TBX_FILE_PATH_MAX],result[TBX_FILE_PATH_MAX];
    uint32_t n=0,at=0;
    while(input[n]){if(n>=sizeof source-1)return 0;source[n]=input[n];n++;}source[n]=0;
    if(source[0]&&source[1]==':'){if(upper(source[0])!='A')return 0;at=2;}
    uint32_t len=3;result[0]='A';result[1]=':';result[2]='\\';result[3]=0;
    if(source[at]!='\\'&&source[at]!='/') {
        len=slen(g_directory);for(uint32_t i=0;i<=len;i++)result[i]=g_directory[i];
    }
    while(source[at]) {
        while(source[at]=='\\'||source[at]=='/')at++;
        uint32_t first=at;while(source[at]&&source[at]!='\\'&&source[at]!='/')at++;
        uint32_t part=at-first;if(!part)break;
        if(part==1&&source[first]=='.')continue;
        if(part==2&&source[first]=='.'&&source[first+1]=='.') {
            while(len>3&&result[len-1]!='\\')len--;
            if(len>3)len--;
            result[len]=0;continue;
        }
        if(len>3){if(len>=sizeof result-1)return 0;result[len++]='\\';}
        if(len+part>=sizeof result)return 0;
        for(uint32_t i=0;i<part;i++){char c=source[first+i];if(c<=' '||c==':'||c=='"'||c=='<'||c=='>'||c=='|')return 0;result[len++]=c;}
        result[len]=0;
    }
    for(uint32_t i=0;i<=len;i++)out[i]=result[i];
    return 1;
}
static int directory_exists(const char *path)
{
    if(same(path,"A:\\"))return 1;
    for(uint32_t i=0;path[i];i++)if(path[i]=='*'||path[i]=='?')return 0;
    copy_text(g_probe,path);
    return tbx_file_find(g_probe,16,&g_probe_rec,1)==1&&(g_probe_rec.attr&16);
}
static int join_path(const char *folder,const char *name,char *out)
{
    uint32_t n=slen(folder),m=slen(name),slash=n&&folder[n-1]!='\\';
    if(n+slash+m>=TBX_FILE_PATH_MAX)return 0;
    for(uint32_t i=0;i<n;i++)out[i]=folder[i];
    if(slash)out[n++]='\\';
    for(uint32_t i=0;i<=m;i++)out[n+i]=name[i];
    return 1;
}
/* DOS 8.3 wildcard fields are padded, so '?' also matches a padding space.
 * This keeps *.WK? faithful to the captured DOS search, rather than a glob. */
static void dos_field(const char *s,char field[11])
{
    unsigned at=0,limit=8;for(unsigned i=0;i<11;i++)field[i]=' ';
    for(;*s;s++) {
        if(*s=='.'){at=8;limit=11;continue;}
        if(*s=='*'){while(at<limit)field[at++]='?';continue;}
        if(at<limit)field[at++]=upper(*s);
    }
}
static int file_match(const char *pattern,const char *name)
{
    char p[11],n[11];dos_field(pattern,p);dos_field(name,n);
    for(unsigned i=0;i<11;i++)if(p[i]!='?'&&p[i]!=n[i])return 0;
    return 1;
}
static int file_before(const tbx_find_rec_t *a,const tbx_find_rec_t *b)
{
    if((a->attr&16)!=(b->attr&16))return !(a->attr&16);
    for(unsigned i=0;i<13;i++){if(a->name[i]!=b->name[i])return (uint8_t)a->name[i]<(uint8_t)b->name[i];if(!a->name[i])break;}
    return 0;
}
static int file_scan(void)
{
    if(!join_path(g_folder,"*.*",g_probe)){notice("File name too long");return 0;}
    int32_t n=tbx_file_find(g_probe,16,g_files,FILE_CAP+1);
    if(n<0){file_error(n);return 0;}
    if(n>=(int32_t)(FILE_CAP+1)){notice("Too many files (511 maximum)");return 0;}
    g_file_count=0;g_file_sel=0;
    for(int32_t i=0;i<n;i++) {
        tbx_find_rec_t *f=&g_files[i];
        if(f->attr&14u)continue;
        if(f->name[0]=='.'||!f->name[0])continue;
        if(!(f->attr&16)&&!file_match(g_pattern,f->name))continue;
        g_files[g_file_count++]=*f;
    }
    for(unsigned i=1;i<g_file_count;i++) {
        tbx_find_rec_t f=g_files[i];unsigned j=i;
        while(j&&file_before(&f,&g_files[j-1])){g_files[j]=g_files[j-1];j--;}
        g_files[j]=f;
    }
    return 1;
}
static int file_front_editing(void){return g_nlen||!g_file_count;}
static void file_decimal(char *out,unsigned at,uint32_t value,unsigned digits)
{
    for(unsigned i=0;i<digits;i++){out[at+digits-1-i]=(char)('0'+value%10);value/=10;}
}
static void file_front_paint(void)
{
    if(g_mode==M_FILE_ERASE) {
        panel_line(1,"No  Yes");unsigned x=g_file_confirm?4u:0u;
        put_line(1,(int)x,g_file_confirm?"Yes":"No",g_file_confirm?3u:2u,TBX_COLOR_WHITE,TBX_COLOR_NAVY);
        panel_line(2,g_file_confirm?"Erase the file":"Cancel command -- Do not erase the file");return;
    }
    char line[TBX_FILE_PATH_MAX+80];uint32_t n=0;
    const char *prompt=g_mode==M_DIRECTORY?"Enter current directory: ":g_file_op==1?"Enter extension of files to list: ":g_file_op==2?"Enter name of file to import: ":"Enter name of file to erase: ";
    for(unsigned i=0;prompt[i];i++)line[n++]=prompt[i];
    const char *value=g_nlen?g_name:g_mode==M_DIRECTORY?g_directory:g_folder;
    for(unsigned i=0;value[i]&&n<sizeof line-1;i++)line[n++]=value[i];
    if(g_mode!=M_DIRECTORY&&!g_nlen) {
        if(n&&line[n-1]!='\\')line[n++]='\\';
        for(unsigned i=0;g_pattern[i]&&n<sizeof line-1;i++)line[n++]=g_pattern[i];
    }
    line[n]=0;panel_line(1,line);panel_line(2,"");
    if(g_mode==M_DIRECTORY||g_nlen)return;
    unsigned slots=(cols()+4u)/16u;if(!slots)slots=1;
    unsigned rows=g_file_full?(g_lines>4u?g_lines-4u:1u):1u;
    unsigned page=slots*rows,first=g_file_sel/page*page;
    if(g_file_full) {
        if(g_file_count) {
            const tbx_find_rec_t *f=&g_files[g_file_sel];
            char meta[SCR_COLS+1];for(unsigned i=0;i<SCR_COLS;i++)meta[i]=' ';meta[SCR_COLS]=0;
            unsigned len=slen(f->name);for(unsigned i=0;i<len;i++)meta[11+i]=f->name[i];
            if(f->attr&16)meta[11+len]='\\';
            unsigned year=1980u+(f->date>>9),digits=year<2000?2u:4u;
            file_decimal(meta,26,(f->date>>5)&15,2);meta[28]='/';file_decimal(meta,29,f->date&31,2);meta[31]='/';file_decimal(meta,32,year,digits);
            file_decimal(meta,41,f->time>>11,2);meta[43]=':';file_decimal(meta,44,(f->time>>5)&63,2);
            if(f->attr&16){const char *dir="<DIR>";for(unsigned i=0;dir[i];i++)meta[56+i]=dir[i];}
            else {uint32_t size=f->size;unsigned p=56;do{meta[p--]=(char)('0'+size%10);size/=10;}while(size&&p>46);}
            panel_line(2,meta);
        }
        for(unsigned r=3;r<(unsigned)LINE_STATUS;r++)panel_line((int)r,"");
    }
    for(unsigned i=0;i<page&&first+i<g_file_count;i++) {
        char name[16];unsigned k=0;const tbx_find_rec_t *f=&g_files[first+i];
        while(f->name[k]&&k<13){name[k]=f->name[k];k++;}
        if(f->attr&16)name[k++]='\\';
        name[k]=0;
        unsigned x=i%slots*16u,y=(g_file_full?3u:2u)+i/slots;
        put_line((int)y,(int)x,name,k,first+i==g_file_sel?TBX_COLOR_WHITE:TBX_COLOR_BLACK,first+i==g_file_sel?TBX_COLOR_NAVY:TBX_COLOR_WHITE);
    }
}
static void import_text(void)
{
    int32_t h=tbx_file_open(g_selected,TBX_FILE_READ_ONLY);
    if(h<0){file_error(h);return;}
    int32_t size=tbx_file_seek(h,0,TBX_FILE_FROM_END),rc=size<0?size:tbx_file_seek(h,0,TBX_FILE_FROM_START);
    uint32_t n=0;
    if(rc>=0&&size<=(int32_t)sizeof g_wk)while(n<(uint32_t)size) {
        rc=tbx_file_read(h,g_wk+n,(uint32_t)size-n);if(rc<=0||(uint32_t)rc>(uint32_t)size-n)break;n+=(uint32_t)rc;
    }
    int32_t closed=tbx_file_close(h);
    if(size>(int32_t)sizeof g_wk){notice("File too large");return;}
    if(rc<0||closed<0){file_error(rc<0?rc:closed);return;}
    if(n!=(uint32_t)size){notice("File read incomplete");return;}
    i123_sheet_t next;if(spare(&next)){notice("Memory full");return;}
    uint32_t row=i123_cursor_row(&g_sheet),col=i123_cursor_col(&g_sheet),len=0,written=0;
    char line[ENTRY_MAX];const char *error=0;
    /* Captured CR records, tab stops, literal controls and prefix publication.
     * An EOF/Ctrl-Z commits its final unterminated line. Row 8192 truncates. */
    for(uint32_t pos=0;pos<=n&&row<I123_MAX_ROWS;pos++) {
        uint8_t c=pos<n?g_wk[pos]:26;
        if(c==10||c==0)continue;
        if(c==13||c==26) {
            if(len) {
                const i123_cell_t *old=i123_find(&next,(uint16_t)col,(uint16_t)row);
                uint8_t fmt=old?old->fmt:WK1_FMT_FRESH_CELL;
                if(next.protec&&(fmt&128u)){error="Protected cell";break;}
                rc=i123_set_label(&next,(uint16_t)col,(uint16_t)row,'\'',line,len);
                if(rc){error="Memory full";break;}written++;
            }
            len=0;row++;if(c==26)break;continue;
        }
        unsigned amount=c==9?8u-len%8u:1u;
        if(len+amount>ENTRY_MAX){error="Input line too long (240 characters max)";break;}
        while(amount--)line[len++]=c==9?' ':(char)c;
    }
    if(written)changed(&next);else ready();
    if(error)notice(error);
}
static int file_choose(const char *name)
{
    if(!join_path(g_folder,name,g_selected)){notice("File name too long");return 0;}
    if(directory_exists(g_selected)) {
        copy_text(g_folder,g_selected);g_nlen=0;g_name[0]=0;
        if(file_scan())paint_all();
        return 0;
    }
    if(g_file_op==1){ready();paint_all();return 1;}
    if(g_file_op==2){import_text();return 1;}
    g_file_confirm=0;g_mode=M_FILE_ERASE;paint_all();return 1;
}
static void file_front_start(unsigned op,unsigned category)
{
    g_file_op=op;g_file_category=category;g_file_full=op==1;
    g_nlen=0;g_name[0]=0;copy_text(g_folder,g_directory);copy_text(g_pattern,FILE_FILTERS[category]);
    g_mode=M_BROWSE;if(file_scan())paint_all();
}
static void file_input_accept(void)
{
    if(g_mode==M_DIRECTORY) {
        if(!g_nlen){ready();paint_all();return;}
        if(!resolve_path(g_name,g_selected)||!directory_exists(g_selected)){notice("Directory does not exist");return;}
        for(uint32_t i=0;g_selected[i];i++)g_selected[i]=upper(g_selected[i]);
        copy_text(g_directory,g_selected);ready();paint_all();return;
    }
    if(!g_nlen) {if(g_file_count)(void)file_choose(g_files[g_file_sel].name);return;}
    /* Resolve typed paths relative to the browser, without changing the
     * session directory. This prefix is private; no global chdir occurs. */
    char saved[TBX_FILE_PATH_MAX];copy_text(saved,g_directory);copy_text(g_directory,g_folder);
    int ok=resolve_path(g_name,g_selected);copy_text(g_directory,saved);
    if(!ok) {
        /* R2.2 asks No/Yes even for an explicit name on a missing drive;
         * only Yes attempts deletion and reports File does not exist. */
        if(g_file_op==0&&g_name[0]&&g_name[1]==':'&&upper(g_name[0])!='A') {
            int wild=0;for(unsigned i=0;g_name[i];i++)if(g_name[i]=='*'||g_name[i]=='?')wild=1;
            if(!wild){copy_text(g_selected,g_name);g_file_confirm=0;g_mode=M_FILE_ERASE;paint_all();return;}
        }
        notice("Invalid file name");return;
    }
    if(directory_exists(g_selected)){copy_text(g_folder,g_selected);g_nlen=0;g_name[0]=0;if(file_scan())paint_all();return;}
    uint32_t split=slen(g_selected);while(split>3&&g_selected[split-1]!='\\')split--;
    char name[13];uint32_t m=slen(g_selected+split);if(m>12){notice("Invalid file name");return;}
    copy_text(name,g_selected+split);
    if(split>3)split--;
    g_selected[split]=0;
    if(g_file_op!=0&&!directory_exists(g_selected)){notice("Directory does not exist");return;}
    copy_text(g_folder,g_selected);
    int dot=0,wild=0;for(unsigned i=0;name[i];i++){if(name[i]=='.')dot=1;if(name[i]=='*'||name[i]=='?')wild=1;}
    if(!dot&&g_file_category<3) {
        const char *ext=FILE_FILTERS[g_file_category]+1;unsigned len=slen(name),extra=slen(ext);
        if(len+extra>12){notice("Invalid file name");return;}
        for(unsigned i=0;i<=extra;i++)name[len+i]=ext[i];
    }
    if(wild) {copy_text(g_pattern,name);g_nlen=0;g_name[0]=0;if(file_scan())paint_all();return;}
    if(g_file_op==1){ready();paint_all();return;}
    if(g_file_category==0&&!dot) {
        copy_text(g_pattern,name);
        if(directory_exists(g_folder)) {
            if(!file_scan())return;
            for(unsigned i=0;i<g_file_count;i++)if(!(g_files[i].attr&16)){(void)file_choose(g_files[i].name);return;}
        }
        unsigned len=slen(name);if(len&&name[len-1]=='?')name[len-1]='1';
    }
    (void)file_choose(name);
}
static void file_front_key(char ch,uint8_t vk)
{
    if(vk==VK_ESC||ch==27) {
        if(g_nlen){g_nlen=0;g_name[0]=0;paint_all();return;}
        if(g_mode==M_FILE_ERASE){ready();paint_all();return;}
        menu_mode();paint_all();return;
    }
    if(g_mode==M_FILE_ERASE) {
        if(vk==VK_LEFT||vk==VK_RIGHT||ch==' ')g_file_confirm=!g_file_confirm;
        else if(vk==VK_ENTER||ch=='\r'||ch=='n'||ch=='N'||ch=='y'||ch=='Y') {
            int yes=ch=='y'||ch=='Y'||((vk==VK_ENTER||ch=='\r')&&g_file_confirm);
            if(!yes){ready();paint_all();return;}
            int32_t rc=tbx_file_delete(g_selected);
            if(rc<0){notice(rc==TBX_ERR_DOS(5)?"File or directory is read-only":
                (rc==TBX_ERR_DOS(2)||rc==TBX_ERR_DOS(3)||rc==TBX_ERR_DOS(15))?"File does not exist":"File I/O error");return;}
            ready();paint_all();return;
        }
        command_paint();return;
    }
    if(vk==VK_ENTER||ch=='\r'){file_input_accept();return;}
    if(vk==VK_BSPC||ch==8){if(g_nlen)g_nlen--;g_name[g_nlen]=0;paint_all();return;}
    if(ch>=' '&&ch<='~') {
        if(g_nlen>=sizeof g_name-1){(void)tbx_trap(TBX_BEEP,0,0);return;}
        g_name[g_nlen++]=ch;g_name[g_nlen]=0;paint_all();return;
    }
    if(g_mode==M_DIRECTORY||g_nlen)return;
    unsigned slots=(cols()+4u)/16u;if(!slots)slots=1;
    unsigned rows=g_file_full?(g_lines>4u?g_lines-4u:1u):1u,page=rows*slots;
    if(vk==0x3d){g_file_full=!g_file_full;paint_all();return;} /* F3 */
    if(g_file_count) {
        if(vk==VK_HOME)g_file_sel=0;
        else if(vk==VK_END)g_file_sel=g_file_count-1;
        else if(vk==VK_LEFT&&g_file_sel)g_file_sel--;
        else if(vk==VK_RIGHT&&g_file_sel+1<g_file_count)g_file_sel++;
        else if(vk==VK_UP&&g_file_sel>=slots)g_file_sel-=slots;
        else if(vk==VK_DOWN&&g_file_sel+slots<g_file_count)g_file_sel+=slots;
        else if(vk==VK_PGUP)g_file_sel=g_file_sel>=page?g_file_sel-page:g_file_sel%slots;
        else if(vk==VK_PGDN) {
            unsigned next=g_file_sel+page;
            if(next>=g_file_count)next=(g_file_count-1)/slots*slots+g_file_sel%slots;
            g_file_sel=next<g_file_count?next:g_file_count-1;
        }
    }
    paint_all();
}
