/* Factory bridge for command.c's actual builtins. ADR-0003 App A/DEC-04a.
 * Bounce native host stack pointers into the low flat INT21 aperture. */
static unsigned char bounce[8192], second_path[128];
static find_data_t g_shell_dta;
static int21_break_recovery_t g_shell_break;
static char g_out[64];
static unsigned verify_reads;
static char last_created[128];
static unsigned char verify_handle[256];
static int call_host(unsigned a,unsigned b,unsigned c,uintptr_t d,uintptr_t di)
{
    int_frame_t f={0}; unsigned ah=a>>8;
    if(ah==0x40 && c==0 && getenv("SHELL_REFUSE_TRUNCATE"))return -0x1d;
    f.eax=a; f.ebx=b; f.ecx=c; f.edx=(uint32_t)d; f.edi=(uint32_t)di;
    f.eflags=0x202;
    int path=ah==0x39 || ah==0x3b || ah==0x3c || ah==0x3d || ah==0x4e || ah==0x56 || ah==0x5b;
    if(path) { strcpy((char *)bounce,(const char *)d); f.edx=(uint32_t)(uintptr_t)bounce; }
    if(ah==0x56) { strcpy((char *)second_path,(const char *)di); f.edi=(uint32_t)(uintptr_t)second_path; }
    if(ah==0x40) { memcpy(bounce,(const void *)d,c); f.edx=(uint32_t)(uintptr_t)bounce; }
    if(ah==0x3f) f.edx=(uint32_t)(uintptr_t)bounce;
    int21_dispatch(&f);
    if(!(f.eflags&1) && ah==0x3c) strcpy(last_created,(const char *)d);
    if(!(f.eflags&1) && ah==0x3d)
        verify_handle[f.eax&255]=(unsigned char)(!strcmp(last_created,(const char *)d));
    if(ah==0x3e)verify_handle[b&255]=0;
    if(!(f.eflags&1) && ah==0x3f && verify_handle[b&255] && f.eax) {
        verify_reads++;
        if(getenv("SHELL_CORRUPT_VERIFY"))bounce[0]^=1;
    }
    if(!(f.eflags&1) && ah==0x3f) memcpy((void *)d,bounce,f.eax);
    if(f.eflags&1) return -(int)(f.eax&65535);
    return (int)f.eax;
}
static void dos_print(const char *s) { while(*s && *s!='$') putchar(*s++); }
static void dos_write(const char *s,uint32_t n) { fwrite(s,1,n,stdout); }
static uint32_t str_len(const char *s) { return (uint32_t)strlen(s); }
static void dos_puts_raw(const char *s) { fputs(s,stdout); }
static int dos_open(const char *s) { return call_host(0x3d00,0,0,(uintptr_t)s,0); }
static int dos_open_mode(const char *s,uint8_t m) { return call_host(0x3d00|m,0,0,(uintptr_t)s,0); }
static int dos_creat(const char *s) { return call_host(0x3c00,0,0,(uintptr_t)s,0); }
#define DOS_READ_ERROR 0xffffffffu
static uint32_t dos_read(int h,uint8_t *p,uint32_t n)
{ int r=call_host(0x3f00,(unsigned)h,n,(uintptr_t)p,0); return r<0 ? DOS_READ_ERROR:(uint32_t)r; }
#define DOS_WRITE_ERROR 0xffffffffu
static uint32_t dos_write_h(int h,const uint8_t *p,uint32_t n)
{ int r=call_host(0x4000,(unsigned)h,n,(uintptr_t)p,0); return r<0 ? DOS_WRITE_ERROR:(uint32_t)r; }
static int dos_seteof(int h) { return call_host(0x4000,(unsigned)h,0,0,0)<0 ? -1:0; }
static void dos_close(int h) { call_host(0x3e00,(unsigned)h,0,0,0); }
static int dos_lseek_end(int h) { return call_host(0x4202,(unsigned)h,0,0,0)<0 ? -1:0; }
static int dos_seek_absolute(int h,uint32_t n) { return call_host(0x4200,(unsigned)h,n>>16,n&65535,0)<0 ? -1:0; }
static int dos_filetime(int h,unsigned set,uint16_t *t,uint16_t *d)
{
    int_frame_t f={0}; f.eax=0x5700|set; f.ebx=(unsigned)h; f.ecx=*t; f.edx=*d;
    int21_dispatch(&f); if(f.eflags&1)return -(int)(f.eax&65535);
    *t=(uint16_t)f.ecx; *d=(uint16_t)f.edx; return 0;
}
static void dos_setdta(void *p) { call_host(0x1a00,0,0,(uintptr_t)p,0); }
static uint16_t dos_findfirst_result(const char *s,uint16_t a)
{ int r=call_host(0x4e00,0,a,(uintptr_t)s,0);return r<0 ? (uint16_t)-r:0; }
static int dos_findfirst(const char *s,uint16_t a) { return !dos_findfirst_result(s,a); }
static int dos_findnext(void) { return call_host(0x4f00,0,0,0,0)>=0; }
static uint16_t dos_rename(const char *a,const char *b)
{ int r=call_host(0x5600,0,0,(uintptr_t)a,(uintptr_t)b);return r<0 ? (uint16_t)-r:0; }
