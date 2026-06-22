/*
 * harness/proptest/gdi_ref_wine.c -- the wine-only translation unit for the
 * test-region-gdi L2 INDEPENDENT GOLDEN (ADR-0005 Amendment AM-1 Sec 7.2).
 *
 * This TU compiles the REAL wine server/region.c (host-only, gdi_ref_-namespaced
 * via gdi_ref_wine_shim.h) and exposes it to the oracle through the OPAQUE
 * gdi_ref_wine.h interface. It is a SEPARATE TU from the oracle so the wine
 * `struct region` never clashes with FLAIR's `struct region` (region_t).
 *
 * If ../win31-decomp is absent (the wine source is not found), this TU compiles
 * to gdi_ref_wine_available = 0 plus stub accessors, and the oracle LOUD-SKIPS
 * L2 (never silent-passes). Test-only; never linked into a kernel/artifact line
 * (Law 3). ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#include "gdi_ref_wine.h"

/* Form WINE_REGION_C as a SINGLE string literal from -DWIN31_DECOMP=<base>
 * (UNQUOTED). Gate the whole wine include behind __has_include so an absent
 * tree LOUD-SKIPS at build time rather than failing the compile. */
#ifdef WIN31_DECOMP
#  define GDI_STR2(s) #s
#  define GDI_STR(s)  GDI_STR2(s)
#  define WINE_REGION_PATH WIN31_DECOMP/oracles/wine/server/region.c
#  define WINE_REGION_C    GDI_STR(WINE_REGION_PATH)
#  if defined(__has_include)
#    if __has_include(WINE_REGION_C)
#      define GDI_REF_HAVE_WINE 1
#    endif
#  endif
#endif

#ifdef GDI_REF_HAVE_WINE

#include "gdi_ref_wine_shim.h"   /* stubs + namespacing + #include the wine .c */

const int gdi_ref_wine_available = 1;

/* The accessors. `gdi_ref_rgn` is a void* alias for `struct region *`. */

gdi_ref_rgn gdi_ref_wine_create(void)
{ return (gdi_ref_rgn)gdi_ref_create_empty_region(); }

void gdi_ref_wine_free(gdi_ref_rgn r)
{ if (r) gdi_ref_free_region((struct region *)r); }

void gdi_ref_wine_set_rect(gdi_ref_rgn r, int left, int top, int right, int bottom)
{
    struct rectangle rc;
    rc.left = left; rc.top = top; rc.right = right; rc.bottom = bottom;
    gdi_ref_set_region_rect((struct region *)r, &rc);
}

int gdi_ref_wine_union(gdi_ref_rgn dst, gdi_ref_rgn s1, gdi_ref_rgn s2)
{ return gdi_ref_union_region((struct region *)dst,
                              (struct region *)s1, (struct region *)s2) != 0; }
int gdi_ref_wine_intersect(gdi_ref_rgn dst, gdi_ref_rgn s1, gdi_ref_rgn s2)
{ return gdi_ref_intersect_region((struct region *)dst,
                                  (struct region *)s1, (struct region *)s2) != 0; }
int gdi_ref_wine_subtract(gdi_ref_rgn dst, gdi_ref_rgn s1, gdi_ref_rgn s2)
{ return gdi_ref_subtract_region((struct region *)dst,
                                 (struct region *)s1, (struct region *)s2) != 0; }
int gdi_ref_wine_xor(gdi_ref_rgn dst, gdi_ref_rgn s1, gdi_ref_rgn s2)
{ return gdi_ref_xor_region((struct region *)dst,
                            (struct region *)s1, (struct region *)s2) != 0; }
int gdi_ref_wine_copy(gdi_ref_rgn dst, gdi_ref_rgn s1)
{ return gdi_ref_copy_region((struct region *)dst, (struct region *)s1) != 0; }

int gdi_ref_wine_num_rects(gdi_ref_rgn r)
{ return ((const struct region *)r)->num_rects; }

int gdi_ref_wine_get_rect(gdi_ref_rgn r, int i, int *l, int *t, int *rr, int *b)
{
    const struct region *reg = (const struct region *)r;
    if (i < 0 || i >= reg->num_rects) return 0;
    *l  = reg->rects[i].left;  *t = reg->rects[i].top;
    *rr = reg->rects[i].right; *b = reg->rects[i].bottom;
    return 1;
}

int gdi_ref_wine_point_in(gdi_ref_rgn r, int x, int y)
{ return gdi_ref_point_in_region((struct region *)r, x, y) ? 1 : 0; }

int gdi_ref_wine_rect_in(gdi_ref_rgn r, int l, int t, int rr, int b)
{
    struct rectangle rc;
    rc.left = l; rc.top = t; rc.right = rr; rc.bottom = b;
    return gdi_ref_rect_in_region((struct region *)r, &rc) ? 1 : 0;
}

#else  /* !GDI_REF_HAVE_WINE -- ../win31-decomp absent: stubs, L2 LOUD-SKIPS */

const int gdi_ref_wine_available = 0;

gdi_ref_rgn gdi_ref_wine_create(void) { return 0; }
void gdi_ref_wine_free(gdi_ref_rgn r) { (void)r; }
void gdi_ref_wine_set_rect(gdi_ref_rgn r, int left, int top, int right, int bottom)
{ (void)r; (void)left; (void)top; (void)right; (void)bottom; }
int gdi_ref_wine_union(gdi_ref_rgn d, gdi_ref_rgn a, gdi_ref_rgn b)
{ (void)d; (void)a; (void)b; return 0; }
int gdi_ref_wine_intersect(gdi_ref_rgn d, gdi_ref_rgn a, gdi_ref_rgn b)
{ (void)d; (void)a; (void)b; return 0; }
int gdi_ref_wine_subtract(gdi_ref_rgn d, gdi_ref_rgn a, gdi_ref_rgn b)
{ (void)d; (void)a; (void)b; return 0; }
int gdi_ref_wine_xor(gdi_ref_rgn d, gdi_ref_rgn a, gdi_ref_rgn b)
{ (void)d; (void)a; (void)b; return 0; }
int gdi_ref_wine_copy(gdi_ref_rgn d, gdi_ref_rgn a)
{ (void)d; (void)a; return 0; }
int gdi_ref_wine_num_rects(gdi_ref_rgn r) { (void)r; return 0; }
int gdi_ref_wine_get_rect(gdi_ref_rgn r, int i, int *l, int *t, int *rr, int *b)
{ (void)r; (void)i; (void)l; (void)t; (void)rr; (void)b; return 0; }
int gdi_ref_wine_point_in(gdi_ref_rgn r, int x, int y)
{ (void)r; (void)x; (void)y; return 0; }
int gdi_ref_wine_rect_in(gdi_ref_rgn r, int l, int t, int rr, int b)
{ (void)r; (void)l; (void)t; (void)rr; (void)b; return 0; }

#endif /* GDI_REF_HAVE_WINE */
