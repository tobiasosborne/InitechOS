/*
 * harness/proptest/gdi_ref_wine_shim.h -- host-only extraction shim that lets
 * the REAL wine server-side region engine (../win31-decomp/oracles/wine/server/
 * region.c) compile STANDALONE for the test-region-gdi L2 differential
 * (ADR-0005 Amendment AM-1 Sec 7.2 / FO-D2-5). Included ONLY by gdi_ref_wine.c
 * (the wine-only TU); the oracle TU never sees this header (no struct clash).
 *
 * WHY THIS IS A GENUINE INDEPENDENT GOLDEN (Law 2; AM-1 Sec 2.3): we compile the
 * ACTUAL wine code UNCHANGED -- a banded-rect region representation from a
 * DIFFERENT heritage (X11/GDI lineage), NOT a re-derivation of ATKINSON's
 * inversion-list algorithm. The wine engine reaches the same SET via a wholly
 * different data structure and band-callback control flow (the 6-arg static
 * region_op at server/region.c:208, vs FLAIR's 4-arg region_op). Grading the
 * ATKINSON GDI facade against it cannot agree "by construction" -- the two
 * engines share no code. This shim supplies ONLY the host-side glue the wine
 * file needs (struct rectangle, mem_alloc->malloc, set_error->no-op, NTSTATUS
 * stubs, the rect helpers); it does NOT touch the region algorithm.
 *
 * HOW THE WINE SERVER HEADERS ARE NEUTRALIZED: the wine region.c #includes
 * "ntstatus.h" "winternl.h" "request.h" "user.h". ntstatus.h/winternl.h are
 * absent from the wine server dir, so the quoted include falls through to the
 * empty stubs in gdi_ref_winestubs/ (on the -I path). request.h/user.h DO exist
 * in the wine server dir (they pull a deep server dep chain); we pre-define
 * their include guards (__WINE_SERVER_REQUEST_H / __WINE_SERVER_USER_H) so their
 * bodies are SKIPPED -- region.c needs none of their contents (we supply the few
 * types/macros it touches directly below).
 *
 * NAMESPACING (Law 3; AM-1 Sec 2.3 / AM-5 / C-7): every EXTERNAL symbol the wine
 * file defines is #define-renamed to a gdi_ref_ prefix before the #include, so
 * nothing here can be confused with or linked against a FLAIR symbol. A same-
 * name region_op would NOT auto-collide at link (different signature, static
 * linkage -- AM-1 Sec 2.3), so the L3 grep gate is the real single-engine guard;
 * this namespacing is belt-and-suspenders. NOTE: we deliberately do NOT rename
 * the wine `struct region` (it stays `struct region` in THIS wine-only TU, which
 * carries no FLAIR header, so there is no clash) -- renaming the `region` token
 * would corrupt the WINE_REGION_C include PATH ("region.c" -> "gdi_ref_region.c")
 * since the path is built by stringizing unquoted tokens.
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11). TEST-ONLY (Law 3).
 */
#ifndef GDI_REF_WINE_SHIM_H
#define GDI_REF_WINE_SHIM_H

/* The including TU (gdi_ref_wine.c) must have formed WINE_REGION_C as a SINGLE
 * string literal pointing at the wine server/region.c, derived from the build's
 * -DWIN31_DECOMP=<base> (passed UNQUOTED so the whole path stringizes into one
 * literal -- adjacent string literals are NOT accepted in a #include directive).
 * The including TU also gates this header behind __has_include(WINE_REGION_C),
 * so reaching here means the file exists. */
#ifndef WINE_REGION_C
#error "gdi_ref_wine_shim.h requires WINE_REGION_C (a single string literal) -- form it from -DWIN31_DECOMP in the including TU"
#endif

#include <stdlib.h>   /* malloc / realloc / free  (the wine engine's allocator) */
#include <string.h>   /* memcpy / memcmp                                        */

/* ---------------------------------------------------------------------------
 * 1. Skip the wine server headers' bodies. ntstatus.h/winternl.h are absent in
 *    the wine dir -> the empty gdi_ref_winestubs/ copies (on -I) satisfy them.
 *    request.h/user.h exist in the wine dir and pull deep deps; pre-define their
 *    include guards so their bodies are skipped (region.c needs none of them).
 * ------------------------------------------------------------------------- */
#define __WINE_SERVER_REQUEST_H   /* skip the real server/request.h body */
#define __WINE_SERVER_USER_H      /* skip the real server/user.h body    */

/* ---------------------------------------------------------------------------
 * 2. The types / macros / helpers the wine region.c references.
 * ------------------------------------------------------------------------- */

/* The wine rectangle: field order (left, top, right, bottom) -- DISTINCT from
 * ATKINSON's rgn_rect_t (top, left, bottom, right). The oracle bridges the two
 * field orders explicitly at every boundary (Law 1: the documented trap). */
struct rectangle { int left, top, right, bottom; };

typedef unsigned int data_size_t;

/* NTSTATUS codes referenced by the file -- values are irrelevant (set_error is
 * a no-op for our host-only use; a NULL return propagates as the engine's own
 * failure path). */
#define STATUS_NO_MEMORY         ((int)0xC0000017)
#define STATUS_INVALID_PARAMETER ((int)0xC000000D)
#define STATUS_BUFFER_OVERFLOW   ((int)0x80000005)

/* The wine server allocator + error sink, host-only. */
#define mem_alloc(n)   malloc((size_t)(n))
#define set_error(x)   ((void)(x))

/* memdup: used only by get_region_data* (the differential never calls them) but
 * the file must compile + link; give it a real host implementation. */
static inline void *gdi_ref_memdup(const void *p, data_size_t n)
{
    void *q = malloc((size_t)n);
    if (q && p) memcpy(q, p, (size_t)n);
    return q;
}
#define memdup(p, n)   gdi_ref_memdup((p), (n))

#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif

/* Rect helpers wine's region.c uses. is_rect_empty / is_rect_equal are read by
 * the band engine; offset_rect/mirror_rect/scale_dpi_rect are referenced only by
 * offset_region/mirror_region/scale_region (never called by the differential)
 * but must COMPILE + LINK -- given correct host implementations regardless. */
static inline int gdi_ref_is_rect_empty(const struct rectangle *r)
{ return (r->left >= r->right) || (r->top >= r->bottom); }
static inline int gdi_ref_is_rect_equal(const struct rectangle *a,
                                        const struct rectangle *b)
{ return a->left == b->left && a->top == b->top &&
         a->right == b->right && a->bottom == b->bottom; }
static inline void gdi_ref_offset_rect(struct rectangle *r, int dx, int dy)
{ r->left += dx; r->right += dx; r->top += dy; r->bottom += dy; }
static inline void gdi_ref_mirror_rect(const struct rectangle *clip,
                                       struct rectangle *r)
{ int l = clip->left + clip->right - r->right;
  int rr = clip->left + clip->right - r->left; r->left = l; r->right = rr; }
static inline void gdi_ref_scale_dpi_rect(struct rectangle *r,
                                          unsigned int from, unsigned int to)
{ if (from == 0) return;
  r->left   = (int)((long long)r->left   * to / from);
  r->top    = (int)((long long)r->top    * to / from);
  r->right  = (int)((long long)r->right  * to / from);
  r->bottom = (int)((long long)r->bottom * to / from); }
#define is_rect_empty(r)         gdi_ref_is_rect_empty(r)
#define is_rect_equal(a, b)      gdi_ref_is_rect_equal((a), (b))
#define offset_rect(r, x, y)     gdi_ref_offset_rect((r), (x), (y))
#define mirror_rect(c, r)        gdi_ref_mirror_rect((c), (r))
#define scale_dpi_rect(r, f, t)  gdi_ref_scale_dpi_rect((r), (f), (t))

/* ---------------------------------------------------------------------------
 * 3. NAMESPACE every EXTERNAL symbol the wine region.c defines under gdi_ref_.
 *    (The internal statics -- region_op, add_rect, coalesce_region, ... -- have
 *    internal linkage and cannot collide across TUs; they need no rename, and
 *    crucially `region_op`/`xmerge` are NOT renamed-via-`region` so the include
 *    PATH stays clean. The `struct region` TYPE is left as-is: this TU has no
 *    FLAIR header so it cannot clash, and renaming the `region` token would
 *    corrupt WINE_REGION_C's "region.c" path component.)
 * ------------------------------------------------------------------------- */
#define create_empty_region          gdi_ref_create_empty_region
#define create_region_from_req_data  gdi_ref_create_region_from_req_data
#define free_region                  gdi_ref_free_region
#define set_region_rect              gdi_ref_set_region_rect
#define get_region_data              gdi_ref_get_region_data
#define get_region_data_and_free     gdi_ref_get_region_data_and_free
#define is_region_empty              gdi_ref_is_region_empty
#define is_region_equal              gdi_ref_is_region_equal
#define get_region_extents           gdi_ref_get_region_extents
#define offset_region                gdi_ref_offset_region
#define mirror_region                gdi_ref_mirror_region
#define scale_region                 gdi_ref_scale_region
#define copy_region                  gdi_ref_copy_region
#define intersect_region             gdi_ref_intersect_region
#define subtract_region              gdi_ref_subtract_region
#define union_region                 gdi_ref_union_region
#define xor_region                   gdi_ref_xor_region
#define point_in_region              gdi_ref_point_in_region
#define rect_in_region               gdi_ref_rect_in_region

/* ---------------------------------------------------------------------------
 * 4. Pull in the REAL wine engine. -Werror is on for the test build; the wine
 *    code is third-party and is not warning-clean under -Wall -Wextra. Quarantine
 *    its diagnostics so the harness's OWN code stays fully -Werror-checked. This
 *    suppresses warnings for the included file ONLY; it does not alter behaviour.
 * ------------------------------------------------------------------------- */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include WINE_REGION_C
#pragma GCC diagnostic pop

#endif /* GDI_REF_WINE_SHIM_H */
