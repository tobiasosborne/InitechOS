/* Factory only. Rule 11; 4aql/ahp2/ugsk: fake DOS segments must not depend on
 * ASLR, collide with owner FREE/SYSTEM, or wrap within a test arena. Each
 * mapping uses a distinct paragraph offset and a disjoint virtual window.
 * MAP_FIXED_NOREPLACE fails loud on collision; it never overwrites host memory.
 */
#ifndef INITECH_TEST_LOW_MAPPING_H
#define INITECH_TEST_LOW_MAPPING_H
#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif
static void *alloc_low(size_t n)
{
#ifdef HOST_LOW_MUTATE_ZERO_OWNER
    static uintptr_t next = 0x10000000u;
#else
    static uintptr_t next = 0x10010000u;
#endif
    uintptr_t at = next;
    next += 0x101000u;
    if (!n || n > 0x80000u || at + n > 0xffffffffu) return NULL;
    void *p = mmap((void *)at, n, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (p == MAP_FAILED) return NULL;
    if ((uintptr_t)p != at) { munmap(p, n); return NULL; }
    return p;
}
#endif
