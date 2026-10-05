/* SAMIR logical input lines. Ref: include/samir/pal.h conin_line contract.
 * Transport framing (CR, LF, CRLF) is authored, no local reference to a
 * historical dBASE shell-redirection promise. Used by the target PAL and
 * byte-stream oracle; the cooked CON transport must receive a full-size read.
 */
#ifndef SAMIR_LINE_INPUT_H
#define SAMIR_LINE_INPUT_H
#include <stdint.h>
#include "samir/pal.h"
#define SAMIR_INPUT_CHUNK 258u
typedef struct {
    uint8_t bytes[SAMIR_INPUT_CHUNK];
    uint32_t pos, len;
    int skip_lf;
} samir_line_state;
typedef int32_t (*samir_input_read)(void *, void *, uint32_t);
static int32_t samir_line_read(samir_line_state *st, samir_input_read read,
                              void *user, char *buf, uint32_t cap)
{
#ifdef SAMIR_MUTATE_STDIN_LINES
    int32_t got;
    uint32_t len,i;
    (void)st;
    if(!buf || !cap) return -PAL_EACCES;
    got=read(user,st->bytes,SAMIR_INPUT_CHUNK);
    if(got<=0) return got<0 ? got : -1;
    len=(uint32_t)got;
    while(len && (st->bytes[len-1]=='\r' || st->bytes[len-1]=='\n')) len--;
    if(len>=cap) len=cap-1;
    for(i=0;i<len;i++) buf[i]=(char)st->bytes[i];
    buf[len]='\0';
    return (int32_t)len;
#else
    uint32_t used=0;
    int overflow=0;
    if(!buf || !cap) return -PAL_EACCES;
    buf[0]='\0';
    for(;;) {
        uint8_t ch;
        if(st->pos==st->len) {
            int32_t got=read(user,st->bytes,SAMIR_INPUT_CHUNK);
            st->pos=st->len=0;
            if(got<0) return got;
            if(!got) {
                if(overflow) return -PAL_EACCES;
                buf[used]='\0';
                return used ? (int32_t)used : -1;
            }
            if((uint32_t)got>SAMIR_INPUT_CHUNK) return -PAL_EACCES;
            st->len=(uint32_t)got;
        }
        ch=st->bytes[st->pos++];
        if(st->skip_lf) {
            st->skip_lf=0;
            if(ch=='\n') continue;
        }
        if(ch=='\r' || ch=='\n') {
            st->skip_lf=ch=='\r';
            buf[used]='\0';
            return overflow ? -PAL_EACCES : (int32_t)used;
        }
        if(used+1u<cap) buf[used++]=(char)ch;
        else overflow=1; /* Drain this line; never return a partial command. */
    }
#endif
}
#endif
