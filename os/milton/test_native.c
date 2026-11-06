/* Independent DEC-03a.4 boundary/property vectors; byte snapshots on refusals. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "native.c"
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"native RED line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void)
{
    uint8_t *mem=calloc(1,0x1e0000), *copy=malloc(0x1e0000);
    native_arena_t a;
    uint32_t p,q,n;
    CHECK(mem && copy);
    native_init(&a,mem,0x610000,0x1e0000,0x4001);
    CHECK(native_call(&a,0x4001,0x48,0,0x10001,&p)==0 && p==0x610010);
    memset(mem+16,0xa5,0x100010);
    CHECK(native_call(&a,0x4001,0x48,0,16,&q)==0 && q==0x710030);
    memcpy(copy,mem,0x1e0000);
    CHECK(native_call(&a,0x4002,0x49,p,0,&n)==9 && !memcmp(copy,mem,0x1e0000));
    CHECK(native_call(&a,0x4001,0x49,p+16,0,&n)==9 && !memcmp(copy,mem,0x1e0000));
    CHECK(native_call(&a,0x4001,0x4a,p,0xffffffff,&n)==8 && n==0x10001 && !memcmp(copy,mem,0x1e0000));
    CHECK(native_call(&a,0x4001,0x48,0,0,&n)==8 && !memcmp(copy,mem,0x1e0000));
    CHECK(native_call(&a,0x4001,0x48,0,0xffffffff,&n)==8 && !memcmp(copy,mem,0x1e0000));
    CHECK(native_owns(&a,0x4001,p,0x100010));
    CHECK(!native_owns(&a,0x4001,p,0x100011) && !native_owns(&a,0x4002,p,1));
    CHECK(native_call(&a,0x4001,0x4a,p,1,&n)==0 && mem[16]==0xa5);
    CHECK(native_call(&a,0x4001,0x4a,p,0x10001,&n)==0 && mem[16]==0xa5);
    CHECK(native_call(&a,0x4001,0x49,q,0,&n)==0);
    CHECK(native_call(&a,0x4001,0x49,p,0,&n)==0);
    CHECK(native_call(&a,0x4001,0x49,p,0,&n)==9);
    CHECK(native_call(&a,0x4001,0x48,0,0x1dfff,&p)==0 && p==0x610010);
    CHECK(native_call(&a,0x4001,0x48,0,1,&n)==8 && n==0);
    CHECK(native_call(&a,0x4001,0x49,p,0,&n)==0);
    mem[0]^=1; memcpy(copy,mem,0x1e0000);
    CHECK(native_call(&a,0x4001,0x48,0,1,&n)==7 && !memcmp(copy,mem,0x1e0000));
    native_init(&a,mem,0x610000,16,0x4001);
    CHECK(native_valid(&a));
    CHECK(native_call(&a,0x4001,0x48,0,1,&n)==8 && n==0);
    CHECK(native_call(&a,0x4001,0x48,0,0,&n)==8 && n==0);
    native_at(&a,0)->owner=0x4001;
    CHECK(native_call(&a,0x4001,0x49,0x610010,0,&n)==7);
    free(mem); free(copy);
    puts(">>> test-native: green (full width, first-fit, coalesce, grow/shrink, exact identity, zero/wrap, foreign owner, corruption, no mutation on refusal)");
    return 0;
}
