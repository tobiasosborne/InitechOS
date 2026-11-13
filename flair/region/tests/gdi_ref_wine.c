/*
 * harness/proptest/gdi_ref_wine.c -- the wine-only translation unit for the
 * test-region-gdi L2 INDEPENDENT GOLDEN (ADR-0005 Amendment AM-1 Sec 7.2).
 *
 * This TU compiles the REAL wine server/region.c (host-only, gdi_ref_-namespaced
 * via gdi_ref_wine_shim.h) and exposes it to the oracle through the OPAQUE
 * gdi_ref_wine.h interface. It is a SEPARATE TU from the oracle so the wine
 * `struct region` never clashes with FLAIR's `struct region` (region_t).
 *
 * The vendored Wine source is required; an absent file fails compilation.
 * Test-only; never linked into the product.
 */
#include "gdi_ref_wine.h"

#define WINE_REGION_C "wine/region.c"
#include "gdi_ref_wine_shim.h"

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
