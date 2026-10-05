/* initech-xrbj: authored PAL-contract oracle, NOT a dBASE differential.
 * Ref: os/samir/include/samir/pal.h conin_line; audit K13 INPUT.TXT.
 * Exercises the exact target's line framer with a byte-stream read stub.
 */
#include <stdio.h>
#include <string.h>
#include "test_assert.h"
#include "../../../os/samir/pal/line_input.h"
TEST_HARNESS();
typedef struct { const char *text; uint32_t pos, chunk; int error; } stream;
static int32_t read_stream(void *u,void *out,uint32_t cap)
{
    stream *s=u;
    uint32_t n=(uint32_t)strlen(s->text+s->pos);
    if(s->error) return -PAL_EACCES;
    CHECK(cap==SAMIR_INPUT_CHUNK,"full cooked-CON read capacity regardless of caller capacity");
    if(n>s->chunk) n=s->chunk;
    if(n>cap) n=cap;
    memcpy(out,s->text+s->pos,n);s->pos+=n;
    return (int32_t)n;
}
static void expect(samir_line_state *st,stream *s,const char *want)
{
    char buf[256];
    int32_t n=samir_line_read(st,read_stream,s,buf,sizeof(buf));
    CHECK(n==(int32_t)strlen(want),"one logical command length");
    if(n>=0) CHECK_STR_EQ(buf,want,"one logical command, no embedded newline");
}
int main(void)
{
    uint32_t chunk;
    for(chunk=1;chunk<=258;chunk=chunk==1?2:chunk==2?7:258) {
        samir_line_state st={0};
        stream s={"use clients.dbf\r\nlist\r\nquit\r\n",0,chunk,0};
        char buf[8];
        expect(&st,&s,"use clients.dbf");expect(&st,&s,"list");expect(&st,&s,"quit");
        CHECK(samir_line_read(&st,read_stream,&s,buf,sizeof(buf))<0,"clean EOF after three commands");
        if(chunk==258) break;
    }
    {
        samir_line_state st={0};stream s={"\r\n\n\rfinal",0,2,0};char buf[8];
        expect(&st,&s,"");expect(&st,&s,"");expect(&st,&s,"");expect(&st,&s,"final");
        CHECK(samir_line_read(&st,read_stream,&s,buf,sizeof(buf))<0,"unterminated final line then EOF");
    }
    {
        samir_line_state st={0};stream s={"123456789\nnext\n",0,258,0};char buf[4];
        CHECK(samir_line_read(&st,read_stream,&s,buf,sizeof(buf))<0,"overlong command fails, never executes a truncated command");
        expect(&st,&s,"next");
    }
    {
        samir_line_state st={0};stream s={"abc",0,258,1};char buf[8];
        CHECK(samir_line_read(&st,read_stream,&s,buf,sizeof(buf))==-PAL_EACCES,"transport error preserved (CF path)");
        CHECK(samir_line_read(&st,read_stream,&s,buf,0)<0,"zero capacity rejected");
    }
    return TEST_SUMMARY("test-samir-input");
}
