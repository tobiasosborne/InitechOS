/*
 * os/flair/tbxgate.h -- the Initech Toolbox Gate + the disk-tenant host
 * (THE ARTIFACT; bead initech-tdnl.14, GUI-remediation R3.7).
 *
 * WHAT THIS IS. The kernel side of "double-click an app on disk and it runs as
 * a FLAIR tenant": the one-slot disk-tenant host (load through MILTON's
 * InitechMZ loader into a block carved from the master FLAIR heap, run the
 * entry, reap on exit/close/crash) and the INT 81h dispatcher the tenant
 * reaches the Toolbox through. Everything routes through the EXISTING spines:
 * registration is FlairProcess_admit, foreground affirmation is
 * FlairProcess_activate, teardown is FlairProcess_terminate / _kill, event
 * delivery is flair_app_dispatch calling this module's kernel-owned vtable
 * (ADR-0006 E-D2 / ADR-0013 BC-2 single spine -- the gate adds TRANSPORT, not
 * policy).
 *
 * Ref: docs/design/GUI-remediation-ADR-reconciliation.md Part B DEC-AC3-1..4 +
 *      Part C deltas 1-7 (implemented from THESE, not the D1.3/D1.4 prose);
 *      docs/design/GUI-remediation-D1-D2-D3-design.md D1.5 (anti-8fhu), D1.6
 *      (one slot), D1.7 (markers + mutants); spec/toolbox_gate.h (the locked
 *      ABI); ADR-0013 Sec 3.1-3.4 + AC-2 (the tenant ABI, split arena, yield
 *      is return, no shipping watchdog).
 *
 * SERIAL MARKERS (D1.7, the locked launch-trace vocabulary):
 *   TENANT-LOAD name=<8.3> len=<module bytes> base=0x<load base>
 *   TENANT-LOAD-FAIL name=<8.3> rc=<loader_status_t>
 *   TENANT-SLOT-BUSY name=<8.3>
 *   TENANT-GATE ax=0x<code> <decoded args> -> <rc>
 *   TENANT-REGISTER ok app=<name>      | TENANT-REGISTER-BAD why=<reason>
 *   TENANT-SETMBAR-BAD why=<reason>    (SETMBAR refused: the MenuBar graph is
 *                                       not wholly in the image or breaks a
 *                                       spec Sec 9 limit; bead initech-cnpm)
 *   TENANT-UNREGISTERED name=<8.3>     (entry returned without REGISTER)
 *   TENANT-EVT what=<n>                (one per push-callback delivery)
 *   TENANT-CRASH vec=<n> eip=+0x<image offset>
 *   TENANT-EXIT rc=<n> via=<exit|close|crash>
 *   TENANT-HEAPAVAIL n=<master flair_heap_avail after the code-block free>
 *
 * Kernel-only in V1 (tenant pointers are flat-32; the O-1-style host suite is
 * a named follow-up). ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_TBXGATE_H
#define INITECH_OS_FLAIR_TBXGATE_H

#include <stdint.h>

#include "process.h"   /* FlairProcessList, FlairApp, WindowMgr, flair_heap_t */
#include "surface.h"   /* bitmap_t */

/* The kernel environment the host drives. Bound once, after the process list
 * and the shell WindowMgr exist. `puts` is the serial sink. */
typedef struct tbx_host {
    FlairProcessList *list;
    WindowMgr        *wm;
    flair_heap_t     *master;
    const bitmap_t   *surface;
    void            (*puts)(const char *s);
} tbx_host_t;

void tbx_bind(const tbx_host_t *host);

/* Launch the disk tenant `name83` from the directory whose first cluster is
 * `dir_start` (0 == root): slot check (TENANT-SLOT-BUSY, nothing changes),
 * load (TENANT-LOAD / TENANT-LOAD-FAIL), run the entry. Returns TBX_OK or a
 * negative TBX_ERR_*. The caller then runs tbx_take_affirm + tbx_reap. */
int tbx_launch(const char *name83, uint16_t dir_start);

/* DEC-AC3-4: the tenant's first NEWWINDOW asks for foreground affirmation.
 * Returns the app to affirm (clearing the request) once the tenant is back in
 * the kernel, or NULL. The caller runs FlairProcess_activate + its post-switch
 * repaint/menubar/present policy. Never returns a dying tenant. */
FlairApp *tbx_take_affirm(void);

/* Complete any pending teardown between pump iterations (ADR-0013 Sec 3.4):
 * EXIT -> FlairProcess_terminate; crash -> FlairProcess_kill; a close-box
 * terminate already run by FlairProcess_close_window -> nothing more to tear.
 * Then free the code block (strict reverse carve order, DEC-AC3-1), emit
 * TENANT-EXIT + TENANT-HEAPAVAIL, and free the slot. Also frees the code block
 * of a tenant whose entry returned WITHOUT registering. Returns 1 when the
 * desktop needs the damage/content/band-2/present cycle, else 0. */
int tbx_reap(void);

/* 1 when `w` is the resident disk tenant's window (for the pump's policy
 * decisions; ownership itself is always the refCon demux in process.c). */
int tbx_owns_window(const WindowRecord *w);

/* THE GATE (called by os/milton/tbx_gate.asm with &int_frame_t). */
void tbx_gate_dispatch(uint8_t *frame);

/* Crash triage (DEC-AC3-3 / Part C item 7), installed as the panic path's
 * fault hook. If a CPU exception was taken while tenant code is executing AND
 * the faulting EIP lies inside the resident tenant's image, record the crash,
 * emit TENANT-CRASH, and unwind to the kernel call site (never returns). Any
 * other fault returns so the panic path HALTS exactly as before (fail-loud
 * preserved for every non-tenant fault). */
void tbx_fault_triage(uint32_t vector, uint32_t eip);

#endif /* INITECH_OS_FLAIR_TBXGATE_H */
