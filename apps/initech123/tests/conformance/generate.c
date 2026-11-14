/* Authority: lotus123-decomp/specs/file-formats/wk1-format.md sections 1,
 * 4.2, 4.4, 4.5, 7; formula bytecodes in that corpus's independent reader.
 * Synthetic stress inputs need real-Lotus capture; no expected UI invented. */
#include "bytes.h"
static void record(unsigned op,unsigned n){w16(used,op);w16(used+2,n);used+=4;}
static void begin(void){memset(bytes,0,sizeof bytes);used=0;record(0,2);w16(used,0x406);used+=2;}
static void end(const char *name){record(1,0);emit(name,used);}
static void integer(unsigned col,unsigned row){record(13,7);bytes[used]=0x7f;w16(used+1,col);w16(used+3,row);w16(used+5,42);used+=7;}
int main(int argc,char **argv){
 if(argc!=2)return 2;
 size_t n=input(argv[1]);emit("L01.WK1",n);
 emit("L02.WK1",0);emit("L03.WK1",5);emit("L04.WK1",n-6);
 emit("L05.WK1",n-4);bytes[n]=0x1a;emit("L06.WK1",n+1);
 begin();integer(255,2047);end("L07.WK1");
 begin();for(unsigned i=0;i<2048;i++)integer(i%256,i/256);end("L08.WK1");
 begin();for(unsigned i=0;i<2049;i++)integer(i%256,i/256);end("L09.WK1");
 begin();record(15,247);bytes[used]=0xff;bytes[used+5]=39;
 memset(bytes+used+6,'L',240);used+=247;end("L10.WK1");
 begin();for(unsigned i=0;i<128;i++){
  record(0x47,25);snprintf((char *)bytes+used,16,"NAME%03u",i);
  w16(used+16,0);w16(used+18,i);w16(used+20,0);w16(used+22,i);used+=25;
 }end("L11.WK1");
 begin();record(16,147);bytes[used]=0x7f;w16(used+13,132);
 bytes[used+15]=5;w16(used+16,1); /* integer constant 1 */
 memset(bytes+used+18,4,128);bytes[used+146]=3;used+=147;end("L12.WK1");
 return 0;
}
