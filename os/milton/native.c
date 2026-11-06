/* ADR-0009 DEC-03a.4, initech-jarzu: native first-fit blocks inside the high
 * reservation. Included by int21.c, separately exercised by test_native.c.
 * Each 16-byte header uses uint32 byte extents and full-width owner identity.
 * No legacy mcb_t change. Validate the WHOLE chain before any mutation. */
#include <stdint.h>
#include "../../spec/initechmz_extended.h"
typedef struct { uint32_t magic, span, owner, reserved; } native_block_t;
_Static_assert(sizeof(native_block_t) == INITECHMZ_NATIVE_HEADER_BYTES,
               "DEC-03a: native block metadata is 16 bytes");
typedef struct { uint8_t *mem; uint32_t base, bytes, owner; } native_arena_t;
#define NATIVE_MAGIC 0x58494d42u
static native_block_t *native_at(const native_arena_t *a, uint32_t off)
{ return (native_block_t *)(void *)(a->mem + off); }
static void native_header(native_arena_t *a, uint32_t off, uint32_t n, uint32_t owner)
{ native_block_t *b=native_at(a,off); b->magic=NATIVE_MAGIC; b->span=n; b->owner=owner; b->reserved=0; }
static void native_init(native_arena_t *a, void *mem, uint32_t base, uint32_t bytes, uint32_t owner)
{ a->mem=mem; a->base=base; a->bytes=bytes; a->owner=owner; native_header(a,0,bytes,0); }
static int native_valid(const native_arena_t *a)
{
    uint32_t off=0;
    if(!a->mem || a->bytes<16 || (a->bytes & 15u)) return 0;
    while(off<a->bytes) {
        native_block_t *b=native_at(a,off);
        if(b->magic!=NATIVE_MAGIC || b->reserved || b->span<16 || (b->owner && b->span<32) ||
           (b->span & 15u) || b->span>a->bytes-off) return 0;
        off+=b->span;
    }
    return off==a->bytes;
}
static void native_coalesce(native_arena_t *a)
{
    uint32_t off=0;
    while(off<a->bytes) {
        native_block_t *b=native_at(a,off);
        uint32_t next=off+b->span;
        if(!b->owner && next<a->bytes && !native_at(a,next)->owner)
            b->span+=native_at(a,next)->span;
        else off=next;
    }
}
/* op 48: count paragraphs; op 49/4a: exact usable pointer. Largest is full
 * width, including a block's following free run for a failed in-place grow. */
static uint16_t native_call(native_arena_t *a, uint32_t owner, unsigned op,
                            uint32_t ptr, uint32_t paras, uint32_t *out)
{
    uint32_t off=0, chosen=0xffffffffu, largest=0, need=0;
#ifdef NATIVE_MUT_COUNT16
    paras &= 0xffffu; /* Rule 6: a >1 MiB request becomes one paragraph. */
#endif
#ifdef NATIVE_MUT_OWNER
    owner=a->owner;   /* Rule 6: accept a foreign caller's free. */
#endif
    if(!native_valid(a)) return 7;
    if(owner!=a->owner || !owner) return 9;
    while(off<a->bytes) {
        native_block_t *b=native_at(a,off);
        if(!b->owner) {
            uint32_t free=(b->span-16u)/16u;
            if(free>largest) largest=free;
            if(op==0x48 && chosen==0xffffffffu && paras && paras<=free) chosen=off;
        } else if(op!=0x48 && a->base+off+16u==ptr && b->owner==owner) chosen=off;
        off+=b->span;
    }
    *out=largest;
    if(op!=0x48 && chosen==0xffffffffu) return 9;
    if(op==0x49) {
        native_at(a,chosen)->owner=0;
        native_coalesce(a);
        return 0;
    }
    if(op==0x4a) {
        native_block_t *b=native_at(a,chosen);
        uint32_t available=b->span;
        uint32_t next=chosen+b->span;
        if(next<a->bytes && !native_at(a,next)->owner) available+=native_at(a,next)->span;
        *out=(available-16u)/16u;
        if(!paras || paras>*out) return 8;
        need=paras*16u+16u; /* count bounded by physical available above */
        if(need>b->span) b->span=available;
        if(b->span-need>=32u) {
            native_header(a,chosen+need,b->span-need,0);
            b->span=need;
            native_coalesce(a);
        }
        return 0;
    }
    if(chosen==0xffffffffu) return 8;  /* also zero/wrapping/oversize counts */
    native_block_t *b=native_at(a,chosen);
    need=paras*16u+16u;
    if(b->span-need>=32u) {
        native_header(a,chosen+need,b->span-need,0);
        b->span=need;
    }
    b->owner=owner;
    *out=a->base+chosen+16u;
    return 0;
}
static int native_owns(const native_arena_t *a, uint32_t owner, uint32_t ptr, uint32_t len)
{
    if(!native_valid(a) || owner!=a->owner) return 0;
    for(uint32_t off=0;off<a->bytes;off+=native_at(a,off)->span) {
        native_block_t *b=native_at(a,off);
        uint32_t lo=a->base+off+16u, hi=a->base+off+b->span;
        if(b->owner==owner && ptr>=lo && ptr<hi && len<=hi-ptr) return 1;
    }
    return 0;
}
