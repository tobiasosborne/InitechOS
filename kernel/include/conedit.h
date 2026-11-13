/* Shared DOS template editor. Ref: ADR-0003 Appendix A; MS-DOS 3.3 User's
 * Guide printed pp.154-158,165-169 (PDF239-243,250-254); IBM DOS 3.30
 * Technical Reference pp.6-11--6-12 (PDF134-135), 00,scan key encoding.
 * The template and
 * current line have independent cursors. EDLIN and the resident AH=0Ah/CON
 * reader use this same code, with their own Ctrl-C and echo policies.
 */
#ifndef INITECHDOS_CONEDIT_H
#define INITECHDOS_CONEDIT_H
#include <stdint.h>
typedef struct {
    uint8_t text[255];
    unsigned length,pos,count;
    int insert,extended,target;
} conedit_t;
typedef void (*conedit_emit_fn)(uint8_t,void *);
static void conedit_init(conedit_t *s,const uint8_t *text,unsigned n) {
    if(n>254)n=254;
    for(unsigned i=0;i<n;i++)s->text[i]=text[i];
    s->length=n;s->pos=s->count=0;s->insert=s->extended=s->target=0;
}
static void conedit_erase(conedit_t *s,conedit_emit_fn emit,void *ctx) {
    if(s->count){s->count--;if(s->pos&&!s->insert)s->pos--;emit(8,ctx);emit(' ',ctx);emit(8,ctx);}
}
static void conedit_copy(conedit_t *s,uint8_t *out,unsigned cap,unsigned stop,conedit_emit_fn emit,void *ctx) {
    while(s->pos<stop&&s->pos<s->length){
        if(s->count>=cap){emit(7,ctx);break;}
        uint8_t c=s->text[s->pos++];out[s->count++]=c;emit(c,ctx);
    }
    s->insert=0;
}
/* One byte of DOS input (extended keys are 00,scan). Return 1 on Enter.
 * F2/F4's target is a literal byte, not another editing command. */
static int conedit_key(conedit_t *s,uint8_t c,uint8_t *out,unsigned cap,conedit_emit_fn emit,void *ctx) {
    if(s->target){
        unsigned stop=s->pos;int action=s->target;s->target=0;
        while(stop<s->length&&s->text[stop]!=c)stop++;
        if(stop==s->length)return 0; /* missing target: leave template cursor */
        if(action==2){
            conedit_copy(s,out,cap,stop,emit,ctx);
        }else{
            s->pos=stop;s->insert=0;
        }
        return 0;
    }
    if(s->extended){
        s->extended=0;
        switch(c){
        case 0x52:
            s->insert=!s->insert;
            break;
        case 0x53:
            if(s->pos<s->length)s->pos++;
            break;
        case 0x4b:conedit_erase(s,emit,ctx);break;
        case 0x3b:
            conedit_copy(s,out,cap,s->pos+1,emit,ctx);
            break;
        case 0x3c:s->target=2;break;
        case 0x3e:s->target=4;break;
        case 0x3d:{unsigned end=s->length;
            conedit_copy(s,out,cap,end,emit,ctx);break;}
        case 0x3f:
            s->length=s->count;
            for(unsigned i=0;i<s->length;i++)s->text[i]=out[i];
            emit('@',ctx);emit(13,ctx);emit(10,ctx);
            s->count=s->pos=0;s->insert=0;break;
        default:break;
        }
        return 0;
    }
    if(!c){s->extended=1;return 0;}
    if(c==13){emit(13,ctx);emit(10,ctx);return 1;}
    if(c==27){
        emit('\\',ctx);emit(13,ctx);emit(10,ctx);
        s->count=s->pos=0;s->insert=0;return 0;
    }
    if(c==8){conedit_erase(s,emit,ctx);return 0;}
    if(s->count>=cap){emit(7,ctx);return 0;}
    out[s->count++]=c;emit(c,ctx);
    if(!s->insert&&s->pos<s->length)s->pos++;
    return 0;
}
#endif
