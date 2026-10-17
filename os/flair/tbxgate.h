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
 *   TENANT-MENU menu=<id> item=<n> src=<mouse|key>
 *                                      (a menu choice in the tenant's OWN bar,
 *                                       pushed as TBX_EVT_MENU; it precedes
 *                                       that delivery's TENANT-EVT what=81;
 *                                       bead initech-tdnl.31)
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
#include "toolbox_gate.h" /* Sec 11 limits (initech-fvnj) */
#include "scrap.h"     /* FlairScrap: the desk scrap (spec Sec 12, tdnl.16) */

/* The kernel environment the host drives. Bound once, after the process list
 * and the shell WindowMgr exist. `puts` is the serial sink. */
typedef struct tbx_host {
    FlairProcessList *list;
    WindowMgr        *wm;
    flair_heap_t     *master;
    const bitmap_t   *surface;
    void            (*puts)(const char *s);
    const uint8_t    *cellfont;  /* the VGA ROM 8x16 strike (boot_info.font_addr;
                                  * spec Sec 7a); NULL -> DRAWCELLS draws the
                                  * cell backgrounds only (bead initech-w96l) */
    FlairScrap       *scrap;     /* the shell's ONE desk scrap (spec Sec 12,
                                  * bead initech-tdnl.16); NULL -> the Scrap
                                  * codes answer BADCODE */
} tbx_host_t;

void tbx_preferences_boot(uint32_t total_k);
int tbx_preferences_dirty(void);
void tbx_preferences_tick(void);
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
 * of a tenant whose entry returned WITHOUT registering. Returns 1 for normal
 * teardown, -1 for a crash, -2 for an unregistered entry (tdnl.66 notices),
 * or 0 when nothing happened. Every nonzero result needs the repaint cycle. */
int tbx_reap(void);

/* MENU CHOICES (bead initech-tdnl.31; spec/toolbox_gate.h Sec 6a).
 *
 * tbx_menu_choice: the MOUSE route. `bar` is the bar the shell's MenuSelect
 * tracked, `sel` its result word, `src` the band-2 mouseDown, `how` the
 * TENANT-MENU src= tag. When `bar` IS the resident tenant's installed bar and
 * something was chosen, emits TENANT-MENU and pushes the TBX_EVT_MENU event
 * through the tenant's eventProc (the tenant may EXIT inside it: the caller
 * runs its tenant service afterwards). Returns 1 when delivered, else 0 (and
 * nothing happened).
 *
 * tbx_menu_key: the KEY route. When the resident tenant is the FOREGROUND app
 * and `ev` is a Ctrl/Cmd keyDown that MenuKey resolves over its bar, delivers
 * exactly as above with src=key and returns 1 (the caller must NOT route the
 * keyDown onward); otherwise returns 0 and the key goes the ordinary way.
 *
 * tbx_take_mbar_redraw: 1 (once) when the tenant called DRAWMENUBAR since the
 * last take; the caller redraws band 2 from the foreground app's bar. */
int tbx_menu_choice(const struct MenuBar *bar, uint32_t sel,
                    const EventRecord *src, const char *how);
int tbx_menu_key(const EventRecord *ev);
int tbx_take_mbar_redraw(void);

/* THE TENANT DREW (bead initech-8zii). 1 (once) when a TEXTDRAW / FILLRECT /
 * DRAWCELLS reached the offscreen since the last take. The pump's tenant
 * service presents then, so what a tenant draws for a plain mouseDown or
 * keyDown -- outside any updateEvt -- reaches the screen when the tenant
 * returns, exactly as DRAWMENUBAR's band-2 redraw does (the pump owns the
 * present; tenant code is never on the stack while it runs). */
int tbx_take_drew(void);

/* THE QUIT REQUEST (bead initech-tdnl.78): the Finder's Restart / Shut Down
 * asks the resident disk tenant to quit by delivering its OWN File > Quit (the
 * enabled item titled "Quit" in its installed bar) exactly as a mouse choice
 * (TENANT-MENU ... src=quit). The tenant may EXIT, ask (an alert) or decline;
 * the caller runs its tenant service and then reads tbx_resident_name: still
 * resident == the application declined, and the shut-down is cancelled.
 * Returns 0 when no live disk tenant is resident (nothing to ask), 1 when the
 * request was delivered, -1 when the tenant has no enabled Quit item (it
 * cannot be asked; the caller cancels -- never terminate it, its work would
 * be lost).
 * tbx_resident_name: the resident live tenant's 8.3 file name, or NULL. */
int tbx_ask_quit(void);
const char *tbx_resident_name(void);

/* 1 when `w` is the resident disk tenant's window (for the pump's policy
 * decisions; ownership itself is always the refCon demux in process.c). */
int tbx_owns_window(const WindowRecord *w);

/* THE ALERT AND THE ASKING CLOSE BOX (bead initech-fvnj; spec Sec 11).
 *
 * tbx_close_ask: when `w` is the resident tenant's window made with goAway ==
 * TBX_GOAWAY_ASK, pushes TBX_EVT_GOAWAY (Message = the window token) and
 * returns 1 -- the caller must NOT close the window; the tenant decides (and
 * the caller runs its tenant service afterwards). Otherwise returns 0.
 *
 * tbx_take_alert: 1 (once) when the tenant asked TBX_ALERT since the last
 * take; fills the copied message lines (n of them) and button labels (nb:
 * default, cancel, other). The caller runs the modal alert, then
 * tbx_alert_answer(item) pushes TBX_EVT_ALERT (Message = item, 1..nb). Never
 * for a dying tenant. */
int  tbx_close_ask(const WindowRecord *w);
int  tbx_take_alert(const char *lines[TBX_ALERT_LINES], uint32_t *n,
                    const char *btns[3], uint32_t *nb);
void tbx_alert_answer(uint32_t item);

/* STANDARD FILE, TRACKING, THE BAR (bead initech-tdnl.16; spec Sec 13-15).
 *
 * tbx_take_sf: 1 (once) when the tenant asked SFGETFILE (*put 0) or
 * SFPUTFILE (*put 1, with the copied prompt and default name) since the last
 * take. The caller runs the shell's Standard File dialog and then
 * tbx_sf_answer(good, path): good copies `path` (the DOS path from the volume
 * root) into the reply buffer validated at the ask and pushes TBX_EVT_SFREPLY
 * Message 1; !good pushes Message 0. The caller services the tenant after.
 *
 * tbx_take_track: 1 (once) when the tenant asked TRACKMOUSE inside its
 * mouseDown (*from = that mouseDown's point). The caller follows the pointer to mouseUp, calling
 * tbx_track_push(0, where) per moved sample and tbx_track_push(1, where) at
 * the release, presenting after each (tbx_take_drew).
 *
 * tbx_take_bar_dirty: 1 (once) when SETSCROLL changed the window's bar; the
 * caller repaints the chrome damage and presents. */
int  tbx_take_sf(int *put, const char **prompt, const char **name);
void tbx_sf_answer(int good, const char *path);
int  tbx_take_track(flair_point_t *from);
void tbx_track_push(int up, flair_point_t where);
int  tbx_take_bar_dirty(void);
/* THE FILE COPY BOX (spec/toolbox_gate.h Sec 16; bead initech-bf2).
 * tbx_take_filecopy: 1 (once) when the tenant asked TBX_FILECOPY; returns
 * the copied caption and paths. tbx_fc_rate: PROGRESS_BAR_RATE read from
 * TBX_FILECOPY_CFG_PATH through the DOS handle API (1..100, or an error).
 * tbx_fc_open opens the source and creates the destination (size out);
 * tbx_fc_step(upto) copies until `upto` bytes are on disk (returns the
 * running total or an error); tbx_fc_close closes both. Every handle is in
 * the tenant's kernel-owned JFT. tbx_filecopy_answer pushes
 * TBX_EVT_FILECOPY (Message = bytes copied or the error). */
int     tbx_take_filecopy(const char **caption, const char **src, const char **dst);
int32_t tbx_fc_rate(void);
int32_t tbx_fc_open(uint32_t *size);
int32_t tbx_fc_step(uint32_t upto);
void    tbx_fc_close(void);
void    tbx_filecopy_answer(int32_t result);

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
