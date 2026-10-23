/* Factory adapters for the real command.c batch state machine. The physical
 * file and CON bytes are independent fixtures; no copy of the driver here. */
static int21_break_recovery_t g_shell_break;
static find_data_t g_shell_dta;
static env_store_t g_master_env;
static int g_shell_exit;
static uint8_t g_errorlevel;
static const char *input;
static unsigned input_pos, input_count, loop_count, poll_count;
static char output[8192];
static unsigned output_n;
static const char *files[3];
static int offsets[3];
static void run_batch(const char *,const char *const [],int);
static void run_batch_invoke(const char *,const char *);
static int dispatch_control(const char *);
static int run_pipeline_or_redirect(const char *);
static void dos_puts_raw(const char *s)
{ while(*s && output_n+1<sizeof output)output[output_n++]=*s++;output[output_n]=0; }
static void dos_print(const char *s)
{ while(*s && *s!='$'){char c[2]={*s++,0};dos_puts_raw(c);} }
static uint32_t str_len(const char *s){return (uint32_t)strlen(s);}
static int dos_open(const char *s)
{
    if(!strcmp(s,"CON"))return 9;
    int n=strstr(s,"CHILD")?1:strstr(s,"PARENT")?2:0;
    offsets[n]=0;return files[n]?n:-1;
}
static int dos_open_mode(const char *s,uint8_t m){(void)m;return dos_open(s);}
static uint32_t dos_write_h(int h,const uint8_t *p,uint32_t n)
{ (void)h;for(uint32_t i=0;i<n;i++){char c[2]={(char)p[i],0};dos_puts_raw(c);}return n; }
#define DOS_READ_ERROR 0xffffffffu
static uint32_t dos_read(int h,uint8_t *p,uint32_t n)
{
    unsigned left=(unsigned)strlen(files[h]+offsets[h]);if(n>left)n=left;
    memcpy(p,files[h]+offsets[h],n);offsets[h]+=(int)n;return n;
}
static void dos_close(int h){(void)h;}
static uint8_t dos_conin(void)
{
    if(input_pos>=input_count){fprintf(stderr,"RED batch-break: unexpected CON read\n");exit(1);}
    uint8_t c=(uint8_t)input[input_pos++];
    if(c==3){g_shell_break.interrupted=1;return 0;}return c;
}
static void cwd_display(char *p){strcpy(p,"A:\\\\");}
static uint8_t dos_getdrive(void){return 0;}
static void dos_gettime(uint8_t *h,uint8_t *m,uint8_t *s,uint8_t *c){*h=*m=*s=*c=0;}
static void dos_getdate(uint16_t *y,uint8_t *m,uint8_t *d,uint8_t *w){*y=1987;*m=*d=1;*w=4;}
static void dos_setdta(void *p){(void)p;}
static int dos_findfirst(const char *p,uint16_t a){(void)p;(void)a;return 0;}
static int dos_findnext(void){return 0;}
static void del_path(char *p,const char *s,int n,const char *l){memcpy(p,s,(unsigned)n);strcpy(p+n,l);}
void int21_batch_checkpoint(void){poll_count++;if(loop_count)g_shell_break.interrupted=1;}
static void dos_getcwd(uint8_t d,char *p){(void)d;*p=0;}
static char prompt_drive(void){return 'A';}
static int bat_ends_with_bat(const char *p){size_t n=strlen(p);return n>=4 && !strcmp(p+n-4,".BAT");}
