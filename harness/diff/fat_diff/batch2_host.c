/* Factory oracle: ADR-0003 Appendix A (3Bh/47h/3Fh/4Eh/4Fh).
 * MS-DOS 3.3 Reference pp.41-42,132,144-148,159. The shell driver
 * is graded separately in QEMU; disk operations here use the real INT21/FAT. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "int21.h"
#include "dos_structs.h"
#include "find_data.h"
#include "psp.h"
#include "sft.h"
#include "fileio_fat.h"
#include "blockdev_file.h"
#include "batch.h"

static psp_t process;
static char path[128], cwd[64], bytes[128], output[256];
static unsigned output_n, input_n, trace_n;
static char trace[64];
static void trace_sink(char c) { if (trace_n+1<sizeof trace) trace[trace_n++]=c; trace[trace_n]=0; }
static const unsigned char *input;
static find_data_t dta;
static void fail(const char *rule) { fprintf(stderr,"batch2-host RED: %s\n",rule); exit(1); }
static void sink(char c) { if(output_n+1<sizeof output) output[output_n++]=c; output[output_n]=0; }
static int get(void) { return input[input_n] ? input[input_n++] : '\r'; }
static unsigned call(unsigned ax, unsigned bx, unsigned cx, void *ptr)
{
    int_frame_t f={0};
    f.eax=ax; f.ebx=bx; f.ecx=cx; f.edx=(uint32_t)(uintptr_t)ptr;
    if (ax==0x4700) { f.edx=0; f.esi=(uint32_t)(uintptr_t)ptr; }
    int21_dispatch(&f);
    if(f.eflags&1) fail("INT21 call failed");
    return f.eax;
}
static void chdir(const char *name, const char *want)
{
    strcpy(path,name); call(0x3b00,0,0,path); call(0x4700,0,0,cwd);
    if(strcmp(cwd,want)) fail("CD root/parent/absolute/getcwd");
}
int main(int argc,char **argv)
{
    if(argc!=2) return 2;
    blockdev_file_t disk; fat12_volume_t vol; uint8_t sector[512];
    psp_params_t params={0}; params.alloc_end_linear=0x70000;
    if(blockdev_file_open_rw(&disk,argv[1]) || fat12_mount(&vol,&disk.dev,sector) || fileio_fat_bind(&vol)) return 2;
    psp_build(&process,&params); sft_init(); int21_set_psp(&process);
    chdir("\\DIR\\SUB","DIR\\SUB"); chdir("..","DIR"); chdir("\\","");
    /* Relative writes after CD root must be in root, as mcopy independently is. */
    strcpy(path,"ROOT.TXT"); unsigned h=call(0x3c00,0,0,path);
    strcpy(bytes,"root\r\n"); call(0x4000,h,6,bytes); call(0x3e00,h,0,0);
    chdir("\\DIR\\SUB","DIR\\SUB");
    call(0x1a00,0,0,&dta); strcpy(path,"*.TXT"); call(0x4e00,0,0,path);
    do { printf("MATCH %s\n",dta.fname); int_frame_t f={0}; f.eax=0x4f00; int21_dispatch(&f); if(f.eflags&1) break; } while(1);
    /* Parameter substitution and directive classification, independent of the
     * I/O driver, include the original quoted-empty regression. */
    char expanded[128]; const char *av[]={"LOOP"}; const char *body=0;
    if(batch_expand("IF \"%1\"==\"\" GOTO X",av,1,0,0,expanded,sizeof expanded)<0 ||
       !batch_eval_if(expanded+3,0,0,0,&body) || strcmp(body,"GOTO X")) fail("IF quoted empty parameter");
    const char *commands[]={"GOTO X","SHIFT","CALL CHILD","PAUSE","IF x==x ECHO OK","FOR %f IN (one) DO ECHO %f"};
    const batch_line_kind_t kinds[]={BL_GOTO,BL_SHIFT,BL_CALL,BL_PAUSE,BL_IF,BL_FOR};
    for(unsigned i=0;i<sizeof kinds/sizeof kinds[0];i++) { batch_parsed_t p; batch_classify(commands[i],&p); if(p.kind!=kinds[i]) fail("internal body classification"); }
    int21_set_sink(sink); int21_set_conin(get,0); int21_set_break_trace(trace_sink);
    input=(const unsigned char *)"hello\032\r"; input_n=output_n=0;
    if(call(0x3f00,0,sizeof bytes,bytes)!=5 || memcmp(bytes,"hello",5)) fail("Ctrl-Z EOF bytes");
    if(strcmp(output,"hello^Z\r\n")) fail("Ctrl-Z caret echo");
    if(call(0x3f00,0,sizeof bytes,bytes)!=0) fail("Ctrl-Z final EOF");
    int21_break_recovery_t recovery={&process,0}; int21_set_break_recovery(&recovery);
    input=(const unsigned char *)"bad\003"; input_n=output_n=0;
    if(call(0x3f00,0,sizeof bytes,bytes)!=0 || !recovery.interrupted) fail("Ctrl-C abort");
    if(strcmp(output,"bad^C\r\n")) fail("Ctrl-C caret echo / serial-only marker");
    if(strcmp(trace,"INT23-BREAK\n")) fail("serial break trace lost");
    blockdev_file_close(&disk);
    puts("PASS batch2-host: batch helpers, CWD+find, cooked EOF/abort"); return 0;
}
