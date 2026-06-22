/*
 * harness/proptest/gdi_ref_wine.h -- the host-only INDEPENDENT-GOLDEN interface.
 *
 * The L2 leg of test-region-gdi (ADR-0005 Amendment AM-1 Sec 7.2) grades the
 * ATKINSON GDI facade against the REAL wine banded-rect region engine. To keep
 * the wine engine's `struct region` from colliding with FLAIR's `struct region`
 * (region_t) inside the oracle translation unit, the wine engine is compiled in
 * its OWN translation unit (gdi_ref_wine.c, which #includes the unmodified wine
 * server/region.c via gdi_ref_wine_shim.h). The oracle TU sees ONLY this opaque
 * interface -- it never sees the wine `struct region` definition, so there is no
 * type clash, and the wine code stays gdi_ref_-namespaced + host-only (Law 3).
 *
 * The wine rectangle field order is (left, top, right, bottom); the accessors
 * below hand back l/t/r/b explicitly so the oracle bridges to ATKINSON's
 * (top,left,bottom,right) rgn_rect_t at one well-marked seam (Law 1 honesty).
 *
 * GDI_REF_WINE_AVAILABLE is 1 iff the wine TU found and compiled the wine source
 * (../win31-decomp present); 0 otherwise. The oracle reads it to LOUD-SKIP L2.
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11). TEST-ONLY (never in a
 * kernel/artifact line, Law 3).
 */
#ifndef GDI_REF_WINE_H
#define GDI_REF_WINE_H

/* 1 if the wine independent golden was compiled in; 0 -> L2 LOUD-SKIPS. Defined
 * as a real symbol in gdi_ref_wine.c so the value is decided at build time by
 * __has_include over WIN31_DECOMP, not guessed by the oracle. */
extern const int gdi_ref_wine_available;

/* Opaque handle to a wine region (a `struct region *` under the hood; the oracle
 * never dereferences it -- it uses the accessors below). */
typedef void *gdi_ref_rgn;

/* Lifecycle. */
gdi_ref_rgn gdi_ref_wine_create(void);            /* empty region; 0 on OOM      */
void        gdi_ref_wine_free(gdi_ref_rgn r);

/* Build: set `r` to the rectangle [left,right) x [top,bottom) (wine l,t,r,b). */
void gdi_ref_wine_set_rect(gdi_ref_rgn r, int left, int top, int right, int bottom);

/* Boolean ops: dst = src1 OP src2 (dst may equal a source per wine contract).
 * Mirror the four CombineRgn set ops + COPY. Return 1 on success, 0 on failure. */
int gdi_ref_wine_union    (gdi_ref_rgn dst, gdi_ref_rgn s1, gdi_ref_rgn s2);
int gdi_ref_wine_intersect(gdi_ref_rgn dst, gdi_ref_rgn s1, gdi_ref_rgn s2);
int gdi_ref_wine_subtract (gdi_ref_rgn dst, gdi_ref_rgn s1, gdi_ref_rgn s2);
int gdi_ref_wine_xor      (gdi_ref_rgn dst, gdi_ref_rgn s1, gdi_ref_rgn s2);
int gdi_ref_wine_copy     (gdi_ref_rgn dst, gdi_ref_rgn s1);

/* Queries (the wine authorities the oracle grades ATKINSON against). */
int  gdi_ref_wine_num_rects(gdi_ref_rgn r);
/* Fetch band-rect i as (l,t,r,b). Returns 1 if i is in range, else 0. */
int  gdi_ref_wine_get_rect (gdi_ref_rgn r, int i, int *l, int *t, int *rr, int *b);
int  gdi_ref_wine_point_in (gdi_ref_rgn r, int x, int y);            /* point_in_region */
int  gdi_ref_wine_rect_in  (gdi_ref_rgn r, int l, int t, int rr, int b); /* rect_in_region (OVERLAP) */

#endif /* GDI_REF_WINE_H */
