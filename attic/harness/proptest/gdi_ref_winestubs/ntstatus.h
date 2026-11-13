/* harness/proptest/gdi_ref_winestubs/ntstatus.h -- EMPTY host-only stub.
 * The wine server/region.c #includes the wine server headers (ntstatus.h and peers).
 * For the host-only L2 differential we supply ONLY the handful of types/macros
 * the region engine actually touches (in gdi_ref_wine_shim.h); these wine
 * headers are stubbed EMPTY so the file compiles standalone. Test-only, NEVER
 * linked into a kernel/artifact line (Law 3). ASCII-clean (Rule 12). */
#ifndef GDI_REF_WINESTUB_NTSTATUS_H
#define GDI_REF_WINESTUB_NTSTATUS_H
#endif
