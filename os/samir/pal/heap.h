/* Fixed first-fit heap for independently owned database storage. Free coalesces
 * neighbours; scratch alloc/reset remains a separate bump arena.
 * Authored, no local reference for allocator policy. Period C, no OS calls.
 * Ref: PAL fixed-memory contract; Using III+ work areas U5-269/U7-7. */
#ifndef SAMIR_PAL_HEAP_H
#define SAMIR_PAL_HEAP_H
#include <stdint.h>
typedef struct pal_heap_block {
    uint32_t size, free;
    struct pal_heap_block *next;
} pal_heap_block;
static inline pal_heap_block *pal_heap_init(void *memory,uint32_t n)
{
    pal_heap_block *b=(pal_heap_block *)memory;
    if(n<=sizeof(*b)) return (void *)0;
    b->size=n-(uint32_t)sizeof(*b);b->free=1;b->next=(void *)0;return b;
}
static inline void *pal_heap_alloc(pal_heap_block *b,uint32_t n)
{
    if(n>0xfffffff0u)return (void *)0;
    n=(n+7u)&~7u;if(!n)n=8;
    for(;b;b=b->next)if(b->free && b->size>=n){
        if(b->size>=n+sizeof(*b)+8u){
            pal_heap_block *tail=(pal_heap_block *)((uint8_t *)(b+1)+n);
            tail->size=b->size-n-(uint32_t)sizeof(*b);tail->free=1;tail->next=b->next;
            b->next=tail;b->size=n;
        }
        b->free=0;return b+1;
    }
    return (void *)0;
}
static inline int pal_heap_free(pal_heap_block *head,void *memory)
{
#ifdef SAMIR_MUTATE_HEAP_FREE
    (void)head;(void)memory;return 0; /* mutant: closed storage never reusable */
#endif
    pal_heap_block *b;int found=memory==0;
    for(b=head;b;b=b->next)if(memory==b+1){
        uint32_t i;if(b->free)return -1;
        for(i=0;i<b->size;i++)((uint8_t *)memory)[i]=0xa5;
        b->free=1;found=1;break;
    }
    if(!found)return -1;
    for(b=head;b && b->next;){
        if(b->free && b->next->free){b->size+=(uint32_t)sizeof(*b)+b->next->size;b->next=b->next->next;}
        else b=b->next;
    }
    return 0;
}
#endif
