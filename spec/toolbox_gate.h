/*
 * spec/toolbox_gate.h -- THE INITECH TOOLBOX GATE: the locked ABI a disk-
 * launched FLAIR tenant uses to reach the Toolbox (LOCKED SPEC-DATA, Rule 8).
 *
 * bead: initech-tdnl.14 (R3.7 app launch from disk). This file IS the Rule-8
 *       deliberate act: the vector, the gate attributes, the AX function map,
 *       the error convention, the argument-offset helper, the link-synthesized
 *       FlairTenantRec layout and the flat FlairEvent buffer are fixed HERE and
 *       nowhere else. Changing any value is a deliberate act with an issue +
 *       worklog note, never a silent edit to make a test pass.
 * bead: initech-cnpm (SETMBAR, a disk tenant's own menu bar). The deliberate
 *       Rule-8 act of THAT bead: TBX_SETMBAR 0x0050 promoted from reserved to
 *       implemented (Sec 3), its arity, and Sec 9 (the MenuBar resource layout
 *       + validation limits). No other value in this file moved.
 * bead: initech-tdnl.31 (a disk tenant HEARS its own menu choices). The
 *       deliberate Rule-8 act of THAT bead: TBX_MENUSELECT 0x0051 DISSOLVED
 *       (Sec 3; the choice is DELIVERED, Sec 6a), TBX_DRAWMENUBAR 0x0052 added
 *       (Sec 3, Sec 9), TBX_EVT_MENU added (Sec 6a), and Sec 9's "SETMBAR only
 *       before the first window" rule lifted -- the rule existed only because
 *       there was no DrawMenuBar trap (Sec 9 said so). No other value moved.
 * bead: initech-w96l (a C application is a disk tenant; Initech 123 rides on
 *       it). The deliberate Rule-8 act of THAT bead: TBX_DRAWCELLS 0x0033
 *       added (Sec 3, Sec 7a -- the fixed 8x16 cell text a worksheet needs),
 *       and Sec 8's carve now honours the image's e_minalloc (the BSS of a
 *       compiled tenant), bounded by TBX_TENANT_BSS_MAX. No other value moved.
 * bead: initech-tdnl.91. Deliberate Rule-8 addition: seven file codes 0x0080
 *       through 0x0086, their arities and Sec 10 policies; no old value moved.
 * bead: initech-fvnj (audit L008, the unsaved-work guard). Deliberate Rule-8
 *       addition: TBX_ALERT 0x0090 + its arity, the TBX_EVT_ALERT and
 *       TBX_EVT_GOAWAY deliveries and the NEWWINDOW goAway value
 *       TBX_GOAWAY_ASK (Sec 11). goAway 0 and 1 keep their meaning exactly;
 *       no old value moved.
 * bead: initech-jojk (InitechWord P2 slice 1, 2026-10-16). Deliberate Rule-8
 *       extension: DRAWCELLS's fg dword carries a txFace in bits 8..15
 *       exactly as TEXTDRAW's does since initech-tdnl.68 (Sec 7a STYLE, Sec
 *       7a TBX_CELL_BASELINE) -- WordPerfect's attribute codes drawn in the
 *       fixed cells. fg < TBX_COLOR_COUNT (face 0) is exactly the old call;
 *       no old value moved.
 * bead: initech-tdnl.16 (R4.2 InitechText, the SimpleText-alike; absorbs the
 *       initech-ww9c Scrap wiring). Deliberate Rule-8 additions: TEXTDRAW's
 *       fg bits 16..23 select the txFont (Sec 7 FONT; 0 is the old call), the
 *       Scrap codes 0x00A0/0x00A1 (Sec 12), Standard File 0x00B0/0x00B1 and
 *       TBX_EVT_SFREPLY (Sec 13), mouse tracking 0x00C0 and TBX_EVT_TRACK
 *       (Sec 14), the vertical scroll bar 0x00C1/0x00C2 + GLOBALTOLOCAL 0x00C3 (Sec 15) and
 *       TBX_ERR_NOTYPE. No old value moved.
 * bead: initech-bf2 (the FILE COPY dialog, PRD Sec 6.5 / Appendix B).
 *       Deliberate Rule-8 addition: TBX_FILECOPY 0x0091 + its arity, the
 *       TBX_EVT_FILECOPY delivery, the configuration path/key and the rate
 *       bounds (Sec 16). No old value moved.
 * bead: initech-tdnl.17 (the Calculator desk accessory, 2026-10-16).
 *       Deliberate Rule-8 extension: NEWWINDOW's goAway dword carries a
 *       WDEF request in bits 8..15 (TBX_WDEF_RDOC = rDocProc 16, the
 *       Calculator's rounded window; 0 = the zoomDocProc document window
 *       every tenant had) and TBX_CONTENTORIGIN 0x0023 is added (the
 *       GlobalToLocal a tenant needs: a mouseDown's where is GLOBAL, Sec 6,
 *       and a dragged window moves its content). Bits 0..7 keep goAway's
 *       0/1/2 exactly; no old value moved.
 * bead: initech-tdnl.22 (R5.2, the Turbo Initech FLAIR.PAS binding). Deliberate
 *       Rule-8 addition: the MenuItem byte-field offsets and the MenuInfo
 *       menuWidth offset of Sec 9, which its prose already stated (mark @4,
 *       cmdChar @5, style @6, enabled @7, is_divider @8, menuWidth @14), as
 *       named values -- the binding's hand-assembled stubs build the graph from
 *       them (tools/tbxgate_inc.c), tbxgate.c pins them to menu.h. No value
 *       moved.
 *
 * Ref: docs/design/GUI-remediation-ADR-reconciliation.md Part B
 *        DEC-AC3-1 (disk tenants; parameterized InitechMZ bases; the
 *                   code->handle->records->data carve and its strict reverse),
 *        DEC-AC3-2 (vector 0x81, trap gate 0x8F, DPL=0, selector 0x08; AX map
 *                   grouping; cdecl args above the 68-byte int_frame_t; result
 *                   in EAX, errors negative; one TENANT-GATE line per call),
 *        DEC-AC3-3 (push-callback delivery through the kernel-owned vtable;
 *                   yield-is-return; link-synthesized FlairTenantRec; no
 *                   KEEP_RESIDENT; no shipping watchdog),
 *        DEC-AC3-4 (windowless registration; foreground affirmation at the
 *                   first NEWWINDOW);
 *      Part C items 4 (evBufPtr, link-synthesized), 9 (gate attributes),
 *        14 (the arg-offset helper lives in locked spec-data);
 *      docs/design/GUI-remediation-D1-D2-D3-design.md D1.2 (gate), D2.1 (the
 *        trap inventory numbering this map follows), D2-2 (the flat FlairEvent);
 *      ADR-0003-AMENDMENT-DEC-04a DEC-04a.1 (software-int gate discipline) and
 *        DEC-04a.2 (0x28-0x37 reserved for hardware IRQs -- why not 0x2F);
 *      SETMBAR (initech-cnpm): D1-D2-D3-design D2.1 row 12 ("0x0050
 *        FLAIR_SETMBAR(barPtr) -> app->menubar = ptr + the
 *        flair_live_finish_tenant_switch redraw; barPtr points into the
 *        tenant's relocated image -- MZ fixups make tenant-resident MenuBar/
 *        MenuItem graphs directly consumable") and D1.5 "Menu-band ownership
 *        after death"; reconciliation DEC-AC3-4 ("SETMBAR before the first
 *        window is legal and takes effect at affirmation"); os/flair/menu.h
 *        Sec 3 (the verbatim Inside Macintosh MenuBar/MenuInfo/MenuItem).
 *
 * Freestanding: <stdint.h> only; consumed by the kernel (os/flair/tbxgate.c),
 * the hand-assembled fixture tenant (its constants are mirrored in the .asm and
 * cross-checked by the emu trace), and the host oracle. ASCII-clean (Rule 12).
 */
#ifndef INITECH_SPEC_TOOLBOX_GATE_H
#define INITECH_SPEC_TOOLBOX_GATE_H

#include <stdint.h>

/* ---------------------------------------------------------------------------
 * 1. THE VECTOR AND GATE ATTRIBUTES (DEC-AC3-2; DEC-04a.1 discipline)
 *
 * Occupancy verified (reconciliation Part A.3): 0x00-0x1F CPU exceptions;
 * 0x20-0x26 DOS software interrupts; 0x28-0x37 hardware IRQs (DEC-04a.2 --
 * untouchable, which is why 0x2F was never a candidate); 0x80 the IDT
 * self-test. 0x81 is clear. A 32-bit TRAP gate keeps IF as the caller had it
 * (cooperative model: the tick keeps ticking inside a Toolbox call).
 * ------------------------------------------------------------------------- */
#define TBX_GATE_VECTOR     0x81u
#define TBX_GATE_TYPE_ATTR  0x8Fu   /* P=1, DPL=0, 32-bit trap gate */
#define TBX_GATE_SELECTOR   0x08u   /* the flat kernel code selector */

/* ---------------------------------------------------------------------------
 * 2. THE ARGUMENT-OFFSET HELPER (Part C item 14)
 *
 * A tenant pushes its arguments cdecl (arg 0 at the LOWEST address) and
 * executes `int 0x81` with the function code in AX. Ring 0 -> ring 0 delivery
 * pushes no SS:ESP, so the interrupted ESP -- where arg 0 sits -- is exactly
 * the end of the locked 68-byte int_frame_t (os/milton/idt.h, _Static_assert-
 * pinned). Arg i is the dword at frame + TBX_GATE_ARG_OFFSET + 4*i.
 * ------------------------------------------------------------------------- */
#define TBX_GATE_FRAME_BYTES  68u
#define TBX_GATE_ARG_OFFSET   TBX_GATE_FRAME_BYTES
#define TBX_GATE_ARG_ADDR(frame_base, i) \
    ((uint32_t)(frame_base) + TBX_GATE_ARG_OFFSET + 4u * (uint32_t)(i))

/* ---------------------------------------------------------------------------
 * 3. THE AX FUNCTION MAP (DEC-AC3-2 grouping; D2.1 numbering)
 *
 * Sparse and grouped: 0x00xx lifecycle, 0x10xx events, 0x20xx windows,
 * 0x30xx QuickDraw, 0x40xx controls, 0x50xx menus, 0x60xx scrap, 0x70xx misc.
 * D2.1 wrote the lifecycle/window/draw rows with the group digit in the LOW
 * byte (0x0020, 0x0030, 0x0070); those are the numbers the D2 Pascal stubs are
 * drafted against, so V1 keeps them verbatim rather than silently renumbering
 * an inventory another bead (tdnl.21) consumes.
 *
 * V1 (tdnl.14) implements exactly the rows marked V1; initech-cnpm adds the
 * row marked V2; initech-tdnl.31 adds the row marked V3; initech-w96l the row
 * marked V4; initech-tdnl.91 adds the Sec 10 file codes. Every other code --
 * including the reserved and DISSOLVED rows -- returns TBX_ERR_BADCODE (the
 * demux is total, never a silent no-op, Rule 2).
 * ------------------------------------------------------------------------- */
#define TBX_REGISTER      0x0001u  /* V1  (recPtr) -> 0 | err                   */
#define TBX_KEEPRESIDENT  0x0002u  /* DISSOLVED by DEC-AC3-3 (registration makes
                                    * the tenant resident) -- BADCODE forever   */
#define TBX_EVENTCHANNEL  0x0010u  /* RESERVED: the push-callback delivery
                                    * channel (kernel -> tenant). NOT a tenant-
                                    * callable trap (DEC-AC3-3 / Part C item 1:
                                    * a tenant never calls WaitNextEvent)      */
#define TBX_NEWWINDOW     0x0020u  /* V1  (l,t,r,b,goAway) -> window token     */
#define TBX_DISPOSEWINDOW 0x0021u  /* reserved (V2)                            */
#define TBX_SETWTITLE     0x0022u  /* V1  (win, strPtr) -> 0 | err             */
#define TBX_CONTENTORIGIN 0x0023u  /* (win) -> (top << 16) | left of the
                                    * window's content, GLOBAL (the tenant's
                                    * GlobalToLocal; bead initech-tdnl.17)    */
#define TBX_ARGC_CONTENTORIGIN 1u
/* NEWWINDOW goAway dword: bits 0..7 goAway (0/1/2, Sec 11), bits 8..15 the
 * WDEF: 0 = zoomDocProc (the document window), TBX_WDEF_RDOC = rDocProc 16
 * (the rounded desk-accessory window, os/flair/rdoc.h). Any other bit is
 * BADARG. Bead initech-tdnl.17. */
#define TBX_WDEF_SHIFT    8u
#define TBX_WDEF_RDOC     (16u << TBX_WDEF_SHIFT)
#define TBX_TEXTDRAW      0x0030u  /* V1  (win, x, y, strPtr, fg, bg) -> 0|err */
#define TBX_FILLRECT      0x0031u  /* V1  (win, l, t, r, b, color) -> 0 | err  */
#define TBX_FRAMERECT     0x0032u  /* reserved (V2)                            */
#define TBX_DRAWCELLS     0x0033u  /* V4  (win, x, y, strPtr, fg, bg) -> 0|err
                                    * (Sec 7a; bead initech-w96l)              */
#define TBX_SETMBAR       0x0050u  /* V2  (barPtr) -> 0 | err  (Sec 9). A
                                    * tenant that never calls it has no menubar
                                    * and band 2 shows the shell fallback bar
                                    * (kmain flair_live_tenant_bar)            */
#define TBX_MENUSELECT    0x0051u  /* DISSOLVED by initech-tdnl.31 (D2.1 row 13
                                    * drafted it as a tenant-called tracker for
                                    * the struck park/resume model). The chosen
                                    * (menuID<<16|item) is DELIVERED instead as
                                    * a TBX_EVT_MENU event (Sec 6a) -- BADCODE
                                    * forever, never reused                    */
#define TBX_DRAWMENUBAR   0x0052u  /* V3  () -> 0 | err  (Sec 9): redraw band 2
                                    * from the installed bar (IM DrawMenuBar)  */
#define TBX_EXIT          0x0070u  /* V1  (rc) -> 0 ; teardown runs when the
                                    * tenant next RETURNS to the kernel         */

/* Deliberate locked ABI extension, initech-tdnl.91. Sec 10 below. */
#define TBX_FILE_CREATE   0x0080u  /* (path) -> handle | err */
#define TBX_FILE_OPEN     0x0081u  /* (path, mode) -> handle | err */
#define TBX_FILE_READ     0x0082u  /* (handle, buffer, count) -> bytes | err */
#define TBX_FILE_WRITE    0x0083u  /* (handle, buffer, count) -> bytes | err */
#define TBX_FILE_SEEK     0x0084u  /* (handle, signed delta, origin) -> pos | err */
#define TBX_FILE_CLOSE    0x0085u  /* (handle) -> 0 | err */
#define TBX_FILE_DELETE   0x0086u  /* (path) -> 0 | err */
/* Deliberate locked ABI extension, initech-fvnj. Sec 11 below. */
#define TBX_ALERT         0x0090u  /* (textPtr, defPtr, cancelPtr, otherPtr)
                                    * -> 0 | err; the answer is DELIVERED  */
#define TBX_ARGC_ALERT    4u
/* Deliberate locked ABI extension, initech-bf2. Sec 12 below. */
/* R4.5 deliberately extends this ABI in spec/cp_dialog.h (0x92..0x96). */
#include "cp_dialog.h"

#define TBX_FILECOPY      0x0091u  /* (captionPtr, srcPath, dstPath) -> 0 | err;
                                    * the result is DELIVERED              */
#define TBX_ARGC_FILECOPY 3u
#define TBX_ARGC_FILE_CREATE 1u
#define TBX_ARGC_FILE_OPEN   2u
#define TBX_ARGC_FILE_READ   3u
#define TBX_ARGC_FILE_WRITE  3u
#define TBX_ARGC_FILE_SEEK   3u
#define TBX_ARGC_FILE_CLOSE  1u
#define TBX_ARGC_FILE_DELETE 1u
#define TBX_FILE_PATH_MAX 128u
#define TBX_FILE_READ_ONLY  0u
#define TBX_FILE_WRITE_ONLY 1u
#define TBX_FILE_READ_WRITE 2u
#define TBX_FILE_FROM_START 0u
#define TBX_FILE_FROM_HERE  1u
#define TBX_FILE_FROM_END   2u
#define TBX_ERR_DOS(code) (-(256 + (int32_t)(code)))

/* Sec 10. FILES (authored, no local reference for the gate ABI).
 * Ref: PRD Sec 4/6.1; ADR-0013 Sec 3.4/AC-2; docs/design/toolbox-file-verbs.md.
 * Paths resolve from the mounted data-volume root, never the launch directory.
 * DOS 8.3 paths/subdirectories are served by the existing DOS layer; OPEN of a
 * character device is refused. CREATE refuses DOS device basenames before
 * touching disk, and truncates an existing regular file.
 * Only FILE handles owned by the kernel shadow JFT are exposed; standard handles
 * and closed/foreign handles are BADARG. Ownership never trusts the image PSP.
 * Every finish (exit/close/crash) reclaims the JFT before freeing the image.
 * File paths/buffers may live in module OR BSS, excluding PSP. Whole spans are
 * checked before access, using subtraction bounds; zero count touches nothing.
 * Paths require NUL within PATH_MAX bytes. Read/write count <= INT32_MAX;
 * zero WRITE retains DOS's truncate-at-current-position meaning.
 * SEEK signed delta + origin requires final 0..INT32_MAX (BADARG otherwise).
 * BADARG covers validation/access-mode failures; DOS errors are ERR_DOS(code).
 * Writes commit synchronously through DOS/FAT; CLOSE returns 0 on success.
 * Existing image-only drawing/registration/menu pointer rules are unchanged.
 */

/* Argument arity per V1 code (the dispatcher reads exactly this many dwords;
 * the per-call TENANT-GATE trace prints exactly this many). */
#define TBX_ARGC_REGISTER   1u
#define TBX_ARGC_NEWWINDOW  5u
#define TBX_ARGC_SETWTITLE  2u
#define TBX_ARGC_TEXTDRAW   6u
#define TBX_ARGC_FILLRECT   6u
#define TBX_ARGC_EXIT       1u
#define TBX_ARGC_SETMBAR    1u
#define TBX_ARGC_DRAWMENUBAR 0u
#define TBX_ARGC_DRAWCELLS  6u
#define TBX_ARGC_MAX        6u

/* ---------------------------------------------------------------------------
 * 4. RESULTS AND ERRORS (DEC-AC3-2: result in EAX, errors NEGATIVE)
 *
 * Errors are RETURNED to the tenant, never fatal to the desktop (a tenant must
 * survive a refused call). Registration-record corruption is the exception: a
 * record that lies about its own image is refused with TBX_ERR_BADREC AND
 * reported loudly (TENANT-REGISTER-BAD), because a scribbled record must never
 * reach the process list (Rule 2).
 * ------------------------------------------------------------------------- */
#define TBX_OK             0
#define TBX_ERR_BADCODE   (-1)  /* AX not a V1 tenant-callable code           */
#define TBX_ERR_NOTREG    (-2)  /* caller has not registered (or is dying)    */
#define TBX_ERR_BADARG    (-3)  /* a pointer outside the image / bad handle   */
#define TBX_ERR_BUSY      (-4)  /* slot occupied / V1 window cap reached      */
#define TBX_ERR_NOMEM     (-5)  /* master-heap carve failed (BC-5 fail-loud)  */
#define TBX_ERR_BADREC    (-6)  /* FlairTenantRec failed validation           */
#define TBX_ERR_NOTTENANT (-7)  /* no disk tenant is executing (stray int 81h)*/
#define TBX_ERR_NOTYPE  (-102)  /* the Scrap holds no such flavor: IM noTypeErr,
                                 * verbatim (Sec 12; initech-tdnl.16)         */

/* ---------------------------------------------------------------------------
 * 5. THE LINK-SYNTHESIZED REGISTRATION RECORD (DEC-AC3-3; Part C item 4)
 *
 * Emitted as DATA by the tenant's toolchain (seed_flair.ld / the TPS MZ-wrap
 * pass / a hand-assembled fixture); every pointer field is an absolute dword
 * the InitechMZ relocation table covers, so the loader's flat relocation makes
 * it correct at whatever base the image lands. No Pascal source ever forms an
 * address (ADR-0007 DEC-02 zero growth). The kernel validates, fail-loud:
 *   tag == FLAIR_TENANT_REC_TAG;
 *   imageBase == the base the loader actually used (a record that lies about
 *     where it lives is refused);
 *   imageLen  == the loaded module length;
 *   the record itself, namePtr (whole NUL-terminated string), evBufPtr (the
 *     whole FlairEvent) and eventProc all lie in [imageBase, imageBase+imageLen).
 * ------------------------------------------------------------------------- */
#define FLAIR_TENANT_REC_TAG      0x50544C46u   /* 'F','L','T','P' little-endian */
#define FLAIR_TENANT_REC_TAG_OFF  0u
#define FLAIR_TENANT_REC_NAME_OFF 4u     /* namePtr   : const char * (ASCIZ)      */
#define FLAIR_TENANT_REC_BASE_OFF 8u     /* imageBase : the image's own load base */
#define FLAIR_TENANT_REC_LEN_OFF  12u    /* imageLen  : module bytes              */
#define FLAIR_TENANT_REC_EVB_OFF  16u    /* evBufPtr  : FlairEvent the kernel fills*/
#define FLAIR_TENANT_REC_PROC_OFF 20u    /* eventProc : void (void), cdecl        */
#define FLAIR_TENANT_REC_BYTES    24u
#define FLAIR_TENANT_NAME_MAX     12u    /* app name incl. NUL (8.3 base + slack) */

/* ---------------------------------------------------------------------------
 * 6. THE FLAT FlairEvent BUFFER (D2-2; DEC-AC3-3 delivery)
 *
 * Six int32 scalars -- no nested record, so the Pascal side is a plain scalar
 * record passed by var. The kernel copies the cooked EventRecord field-wise
 * (widening) into evBufPtr and then calls eventProc; RETURNING from eventProc
 * is the cooperative yield (ADR-0013 Sec 3.4 verbatim).
 * ------------------------------------------------------------------------- */
#define FLAIR_EVBUF_WHAT_OFF   0u
#define FLAIR_EVBUF_MSG_OFF    4u
#define FLAIR_EVBUF_MODS_OFF   8u
#define FLAIR_EVBUF_WHEN_OFF   12u
#define FLAIR_EVBUF_WHEREH_OFF 16u   /* GLOBAL coordinates, as the Mac delivers */
#define FLAIR_EVBUF_WHEREV_OFF 20u
#define FLAIR_EVBUF_BYTES      24u

/* ---------------------------------------------------------------------------
 * 6a. THE MENU EVENT (bead initech-tdnl.31): how a tenant HEARS a menu choice
 *
 * THE APPLICATION'S SIDE, Inside Macintosh: Macintosh Toolbox Essentials
 * (local: ../system7-decomp/refs/MacintoshToolboxEssentials.pdf): a mouseDown
 * that FindWindow puts inMenuBar goes DoMenuCommand(MenuSelect(where))
 * (Listing 3-18, p. 3-72); a Command-key keyDown goes
 * DoMenuCommand(MenuKey(key)) (Listing 2-6, p. 2-44); "In either case, the
 * application passes the function result returned by MenuSelect or MenuKey as
 * a parameter to the DoMenuCommand procedure" (p. 3-78, Listing 3-24), which
 * splits it HiWord = menuID, LoWord = item. ONE handler, ONE value, two routes.
 *
 * UNDER THE APP CONTRACT a tenant has no event loop and never runs a modal
 * loop: the menu band goes to the foreground app's bar -> MenuSelect run by
 * the SHELL (ADR-0013 Sec 3.3), modal tracking loops stay shell-owned helpers
 * and a tenant never calls WaitNextEvent (Sec 3.4), and the kernel PUSHES
 * every event into evBufPtr and calls eventProc (DEC-AC3-3). A tenant-called
 * MENUSELECT trap (D2.1 row 13) cannot be served under that model: the
 * shell's panel restore routes updateEvts to every damaged window's owner
 * (kmain flair_live_erase_menu_panel -> flair_live_content_phase ->
 * flair_route_updates), and the owner under the panel would be the very
 * tenant parked inside the trap -- tbxgate.c run_tenant refuses that
 * re-entry (Rule 2 panic). So the SHELL runs MenuSelect (band-2 click) and
 * MenuKey (a Ctrl/Cmd chord while the tenant is foreground) over the
 * tenant's OWN bar, exactly as it already does for the in-kernel Finder
 * (kmain flair_live_finder_key / _menu_result), and PUSHES the result word:
 *
 *   What      = TBX_EVT_MENU
 *   Message   = (menuID << 16) | item   -- the MenuSelect/MenuKey LONGINT,
 *               verbatim (menu.h Sec 4); never 0 (nothing chosen delivers
 *               nothing, as DoMenuCommand(0) would do nothing)
 *   Modifiers, When, WhereH, WhereV = those of the triggering mouseDown /
 *               keyDown (provenance only; DoMenuCommand reads Message)
 *
 * so the tenant's eventProc IS its DoMenuCommand(Message), reached by the
 * mouse and the key with the IDENTICAL What/Message. A choice made in any
 * OTHER app's bar is never delivered (bar identity). A Ctrl/Cmd chord that
 * MenuKey does not resolve (unbound, or its item disabled) is delivered as the
 * plain keyDown it was. The un-hilite (IM HiliteMenu(0)) is the shell's: the
 * panel restore already redraws the bar idle before the event is pushed.
 *
 * THE VALUE is authored (Inside Macintosh defines no menu event; there is no
 * local reference for a number): 0x0051 -- the AX code of the D2.1 row-13
 * MENUSELECT trap this delivery replaces, so a TENANT-EVT what=81 line names
 * its provenance -- chosen OUTSIDE every Event Manager code (0..15, MTE Table
 * 2-1, and kHighLevelEvent = 23) so it can never alias a real event.
 * ------------------------------------------------------------------------- */
#define TBX_EVT_MENU   0x0051u

/* ---------------------------------------------------------------------------
 * 7. DRAWING TOKENS (C-8 discipline: tenants name ROLES, never RGB/indices)
 *
 * TEXTDRAW/FILLRECT coordinates are CONTENT-LOCAL (origin = the window's
 * content top-left); TEXTDRAW's y is the top of the Chicago 12 line cell
 * (CHICAGO_CELL_H = 16 rows; baseline 12 rows below y). The run is
 * PROPORTIONAL: glyph k starts at x + the sum of the Chicago 12 advances of
 * glyphs 0..k-1 (the real System 7.0.1 NFNT 5478 owTable widths), and the bg
 * box behind it is exactly that sum wide (bead initech-tdnl.33; was the fixed
 * 8-px cell). Every
 * draw is clipped to (the window's VISIBLE region INTERSECT its content), and
 * additionally to its updateRgn while an updateEvt is being delivered.
 * ------------------------------------------------------------------------- */
#define TBX_COLOR_WHITE  0u   /* -> FLAIR_PART_CONTENT       */
#define TBX_COLOR_BLACK  1u   /* -> FLAIR_PART_FRAME         */
#define TBX_COLOR_GRAY   2u   /* -> FLAIR_PART_BTNFACE       */
#define TBX_COLOR_NAVY   3u   /* -> FLAIR_PART_CAPTION_NAVY  */
#define TBX_COLOR_COUNT  4u

/* TEXTDRAW STYLE (deliberate locked ABI extension, bead initech-tdnl.68,
 * 2026-10-15 -- the Font Manager's txFace synthesis reaches tenants; InitechWord
 * Face B is gated on it). TEXTDRAW's fg dword carries the colour token in bits
 * 0..7 and a QuickDraw txFace in bits 8..15 (grafport.h FLAIR_STYLE_*): only
 * bold 0x01, italic 0x02 and underline 0x04 are synthesized (os/flair/text.h);
 * any other face bit, or any bit 16..31 other than a Sec 7 FONT value (bead
 * initech-tdnl.16), is TBX_ERR_BADARG -- an unsupported
 * style fails loud, it is never drawn plain. The run's width is then the
 * STYLED width (bold adds 1 px per glyph); italic/underline do not change it.
 * fg < TBX_COLOR_COUNT (face 0) is exactly the V1 call. bg stays a bare token.
 * FILLRECT/DRAWCELLS retain their previous colour/style contracts. Bit16
 * selects Geneva 9 for TEXTDRAW only (R4.6 extension below). */
#define TBX_TEXT_FACE_SHIFT   8u
#define TBX_TEXT_FACE(f)      ((uint32_t)(f) << TBX_TEXT_FACE_SHIFT)
#define TBX_TEXT_BOLD         TBX_TEXT_FACE(0x01u)
#define TBX_TEXT_ITALIC       TBX_TEXT_FACE(0x02u)
#define TBX_TEXT_UNDERLINE    TBX_TEXT_FACE(0x04u)
#define TBX_TEXT_FACE_OK      0x07u   /* the synthesized faces */
/* R4.6 (initech-tdnl.20), PRD 6.4/6.5: the existing TEXTDRAW selects the
 * Font Manager's Geneva 9 with bit16. Zero retains Chicago 12. Bits17..31
 * remain invalid; DRAWCELLS retains its fixed-cell contract. */
/* TBX_TEXT_GENEVA9 is defined below as TBX_TEXT_FONT(1u) (Sec 7 FONT field, tdnl.16); same bit. */
/* Same NEWWINDOW verb, bounded two-document tenant capacity. Tokens are
 * stable creation-order 1..2, not a WindowPtr. All drawing/title verbs take
 * either token. Both windows remain owned by the ONE admitted app and are
 * reclaimed by the existing process teardown. Closing either asks with its
 * token when goAway=ASK. Ref: PRD6.5 ledger + pie; window_record.h list. */
#define TBX_TENANT_WINDOWS    2u

/* TEXTDRAW FONT (deliberate locked ABI extension, bead initech-tdnl.16 -- the
 * SimpleText-alike draws its document in the application font). fg bits
 * 16..23 carry a QuickDraw txFont (grafport.h: 0 systemFont = Chicago 12, 1
 * applFont = Geneva 9, the text.h text_font_from_txfont mapping; PRD Sec 6.4
 * "Geneva 9 (cell font)"); any other value is TBX_ERR_BADARG. The run, its
 * advances and its opaque bg box are then that font's (Geneva 9: the
 * GENEVA9_CELL_H = 11-row cell, its proportional advances); y is still the
 * top of the cell. Bits 24..31 stay BADARG. fg < 0x10000 is exactly the old
 * call (Chicago 12). */
#define TBX_TEXT_FONT_SHIFT   16u
#define TBX_TEXT_FONT(f)      ((uint32_t)(f) << TBX_TEXT_FONT_SHIFT)
#define TBX_TEXT_GENEVA9      TBX_TEXT_FONT(1u)
#define TBX_TEXT_FONT_MAX     1u      /* the strikes InitechOS owns */

/* ---------------------------------------------------------------------------
 * 7a. FIXED-CELL TEXT (TBX_DRAWCELLS; bead initech-w96l)
 *
 * A worksheet is a grid of character cells (the real 1-2-3 R2.2 screen:
 * 80 x 25 cells, ../lotus123-decomp/goldens/minted/MINT01.shots), which the
 * proportional Chicago run of TEXTDRAW cannot lay out. DRAWCELLS draws the
 * run in the FIXED 8x16 cell font InitechOS already owns: the VGA ROM 8x16
 * strike stage2 captures before protected mode (boot_info.font_addr; PRD
 * Sec 5 "80x25 rendered by blitting the VGA 8x16 ROM font"; the strike the
 * MILTON console blits, os/milton/console.h). Glyph k occupies the cell
 * [x + 8k, x + 8k + 8) x [y, y + 16) CONTENT-LOCAL; every cell is opaque
 * (bg, then the set bits in fg; MSB = leftmost pixel); at most
 * TBX_CELLS_MAX_RUN glyphs; the same clip as TEXTDRAW (visible INTERSECT
 * content [INTERSECT update]) and the same colour tokens (Sec 7). Bytes are
 * glyph indices, 0x20..0xFF drawn as the ROM draws them (code page 437).
 *
 * STYLE (deliberate locked ABI extension, bead initech-jojk, 2026-10-16 --
 * InitechWord Face A draws WordPerfect's [BOLD] / [UND] / [ITALC] text).
 * fg carries the colour token in bits 0..7 and a txFace in bits 8..15, the
 * TEXTDRAW rule word for word (Sec 7 STYLE): bold 0x01, italic 0x02,
 * underline 0x04 are synthesized by THE Font Manager rules (os/flair/text.h
 * text_synth_row: bold = the 1-px smear, italic = the 1:5 shear about the
 * baseline; underline = a 1-px line TEXT_UL_OFFSET rows below it, broken by
 * the TEXT_UL_SHADOW halo where ink touches it); any other face bit or any
 * bit 16..31 is TBX_ERR_BADARG. THE CELL GRID DOES NOT MOVE: a styled run
 * is exactly as wide as a plain one (8 px a glyph -- unlike TEXTDRAW, where
 * bold adds 1 px a glyph), so a smear or a shear that leaves the run's box
 * [x, x + 8n) is clipped there (the box is opaque: nothing outside it is
 * touched). TBX_CELL_BASELINE is the strike's baseline row (the first row
 * below the baseline, the text.h convention): the ROM 8x16 strike's
 * capitals and lower-case letters without descenders end on row 11 and its
 * descenders run on rows 12..14 [measured on the strike the emulators load:
 * test-flair-iword-codes leg STYLE checks the plain cells of the run it
 * grades end on row 11]. fg < TBX_COLOR_COUNT (face 0) is the unchanged
 * V4 call, pixel for pixel.
 * ------------------------------------------------------------------------- */
#define TBX_CELL_W          8u
#define TBX_CELL_H          16u
#define TBX_CELLS_MAX_RUN   80u
#define TBX_CELL_BASELINE   12u

/* ---------------------------------------------------------------------------
 * 8. THE V1 MEMORY MODEL (DEC-AC3-1; D1-3)
 *
 * ONE resident disk tenant (a kernel slot; a second launch fails loud with
 * TENANT-SLOT-BUSY and changes nothing). The code block is carved from the
 * master FLAIR heap (FLAIR_CLASS_GENERAL) as
 *     [ PSP (TBX_TENANT_PSP_BYTES) | load module | e_minalloc BSS ]
 * sized TBX_TENANT_PSP_BYTES + roundup16(file size) + e_minalloc * 16 (bead
 * initech-w96l: the BSS of a compiled C tenant -- the DOS EXEC meaning of
 * e_minalloc, "paragraphs needed beyond the image"; V1 counted only the MZ
 * header's slack, enough for a hand-assembled fixture and nothing else). The
 * loader reads e_minalloc from the file's header before it carves, refuses
 * more than TBX_TENANT_BSS_MAX (TENANT-LOAD-FAIL, never a partial carve),
 * and zeroes the BSS; still a constant for a given file, so the O-3
 * avail-stable reuse of the GENERAL slot holds. The BSS is NOT part of the
 * image the Sec 5 validation checks pointers against. Then the AC-2 trio
 * (handle / records / data) at registration.
 * ------------------------------------------------------------------------- */
#define TBX_TENANT_BSS_MAX        (512u * 1024u)  /* of the 4 MiB master heap */
#define TBX_TENANT_PSP_BYTES      256u            /* the PSP at the block head */
#define TBX_TENANT_IMAGE_MAX      (64u * 1024u)   /* V1 file-size cap          */
#define TBX_TENANT_RECORDS_BUDGET (16u * 1024u)   /* == FLAIR_TENANT_RECORDS_DEFAULT */
#define TBX_TENANT_DATA_BUDGET    (4u * 1024u)    /* V1: kernel-side only      */
#define TBX_TENANT_MAX_WINDOWS    1u              /* V1 window cap per tenant  */

/* ---------------------------------------------------------------------------
 * 9. THE MenuBar RESOURCE (TBX_SETMBAR; bead initech-cnpm; D2.1 row 12)
 *
 * The resource IS the FLAIR Menu Manager's own record graph (os/flair/menu.h
 * Sec 3, verbatim Inside Macintosh MenuBar / MenuInfo / MenuItem), emitted as
 * DATA inside the tenant image. Every pointer field is an absolute dword the
 * InitechMZ relocation table covers (exactly like the FlairTenantRec, Sec 5),
 * so after the loader's flat relocation the graph is directly consumable by
 * the kernel: SETMBAR copies NOTHING, it validates the graph and installs the
 * pointer as the app's menubar (FlairApp.menubar). The layout below is the
 * i386 SysV layout of those C records; os/flair/tbxgate.c _Static_asserts
 * every offset and size against menu.h, so the two can never drift.
 *
 *   MenuBar  (TBX_MBAR_BYTES)   menus @0 (MenuInfo *), n_menus @4 (u16),
 *                               has_apple @6 (u8), pad @7
 *   MenuInfo (TBX_MINFO_BYTES)  menuID @0 (i16), pad @2, title @4 (ASCIZ *),
 *                               items @8 (MenuItem *), n_items @12 (u16),
 *                               menuWidth @14 (i16, a cache: author 0)
 *   MenuItem (TBX_MITEM_BYTES)  text @0 (ASCIZ *), mark @4, cmdChar @5,
 *                               style @6, enabled @7, is_divider @8, pad @9..11
 *
 * VALIDATION (fail loud: refused with TBX_ERR_BADARG AND a serial
 * TENANT-SETMBAR-BAD why=<reason> line -- a resource that lies about where it
 * lives must never reach band 2, Rule 2). The WHOLE graph must lie in
 * [imageBase, imageBase+imageLen): the MenuBar record, the menus array, every
 * title string (incl. its NUL), every items array and every item text string.
 * Also: n_menus <= TBX_MBAR_MAX_MENUS; n_items <= TBX_MBAR_MAX_ITEMS; menuID
 * >= 1 (IM: a 0 high word means "nothing chosen"); titles 1..STR_MAX-1 chars,
 * item texts 0..STR_MAX-1; and the laid-out titles fit the screen (the menu.h
 * Sec 5 run: Apple slot + sum(StringWidth(title) + 14) <= FLAIR_SCREEN_W,
 * StringWidth = the proportional Chicago 12 width, the sum of the NFNT 5478
 * advances; bead initech-tdnl.33 -- was 8*len + 14). n_menus == 0 is
 * legal (an empty bar, IM ClearMenuBar).
 *
 * SEMANTICS (V2, V3). SetMenuBar, not DrawMenuBar: SETMBAR installs the bar;
 * band 2 shows it the next time the tenant is drawn as foreground -- its
 * affirmation at the first NEWWINDOW (DEC-AC3-4), or any later switch back to
 * it -- through flair_live_finish_tenant_switch, the ONE swap path -- or when
 * the tenant calls TBX_DRAWMENUBAR. V2 accepted SETMBAR only before the first
 * NEWWINDOW because, with no DrawMenuBar trap, a later swap would have left
 * band 2 showing a bar the pull-down no longer matched. V3 (initech-tdnl.31)
 * adds that trap and lifts the restriction: SETMBAR is legal whenever the
 * tenant is registered (same validation, same refusals), and the tenant
 * follows it with DRAWMENUBAR exactly as a Mac application follows SetMenuBar
 * with DrawMenuBar (MTE p. 3-41, "use the SetMenuBar procedure to set the
 * current menu list ... and use the DrawMenuBar procedure to update the menu
 * bar"; Listing 3-5). DRAWMENUBAR takes no arguments; the kernel redraws band
 * 2 from the foreground app's installed bar when the tenant RETURNS (tenant
 * code is never on the stack while the pump draws; the effect is visible
 * before the next event, the same instant a Mac DrawMenuBar's ink would be).
 * On EXIT / close / crash the app leaves the
 * process list and band 2 is redrawn from the NEW head (never the corpse's
 * bar); the graph dies with the image (D1.5). The resource must stay
 * unmodified while the tenant is registered: the kernel reads it in place on
 * every band-2 redraw and pull-down (ADR-0013: no inter-app protection).
 * ------------------------------------------------------------------------- */
#define TBX_MBAR_BYTES        8u
#define TBX_MBAR_MENUS_OFF    0u
#define TBX_MBAR_COUNT_OFF    4u
#define TBX_MBAR_APPLE_OFF    6u
#define TBX_MINFO_BYTES       16u
#define TBX_MINFO_ID_OFF      0u
#define TBX_MINFO_TITLE_OFF   4u
#define TBX_MINFO_ITEMS_OFF   8u
#define TBX_MINFO_COUNT_OFF   12u
#define TBX_MITEM_BYTES       12u
#define TBX_MITEM_TEXT_OFF    0u
#define TBX_MITEM_MARK_OFF    4u     /* initech-tdnl.22: the Sec 9 prose, named */
#define TBX_MITEM_CMD_OFF     5u
#define TBX_MITEM_STYLE_OFF   6u
#define TBX_MITEM_ENABLED_OFF 7u
#define TBX_MITEM_DIVIDER_OFF 8u
#define TBX_MINFO_WIDTH_OFF   14u
#define TBX_MBAR_MAX_MENUS    8u
#define TBX_MBAR_MAX_ITEMS    16u
#define TBX_MBAR_STR_MAX      32u    /* incl. the NUL                          */

/* ---------------------------------------------------------------------------
 * 11. THE ALERT AND THE ASKING CLOSE BOX (bead initech-fvnj; audit L008)
 *
 * WHY. A document application must be able to refuse to lose work: Quit and
 * the close box on an edited document ask first (Macintosh Human Interface
 * Guidelines practice; for Initech 123 the period behaviour is the Lotus 1-2-3
 * R2.2 Reference Manual p. 9-1: /Quit Yes over unsaved changes asks a second
 * No/Yes, 123.RI 0x370 "WORKSHEET CHANGES NOT SAVED!  End 1-2-3 anyway?").
 * Until this section a tenant could only EXIT, and its close box terminated it
 * with no word to the tenant (tbxgate.c disk_tenant_close).
 *
 * TBX_ALERT (textPtr, defPtr, cancelPtr, otherPtr) -> 0 | err. Under the App
 * Contract a tenant never runs a modal loop (Sec 6a), so -- like a menu choice
 * -- the alert is ASKED for by the trap and ANSWERED by a delivered event: the
 * verb validates and copies the strings and returns at once; once the tenant
 * has RETURNED the shell runs the modal alert (the Dialog Manager caution
 * alert of bead initech-6k12, the same frame and button row), restores what it
 * covered, and PUSHES
 *     What = TBX_EVT_ALERT, Message = the item hit:
 *            1 = default button (Return / Enter), 2 = cancel button (Escape),
 *            3 = the third button,
 * and Modifiers/When/Where zero. Strings: ASCIZ wholly inside the image (the
 * Sec 5/7 rule TEXTDRAW follows); the kernel COPIES them at the call, so the
 * tenant may reuse its buffers at once. textPtr: 1..TBX_ALERT_LINES lines
 * separated by CR (0x0D, the Dialog Manager static-text line break), each
 * 1..TBX_ALERT_LINE_MAX-1 bytes and at most TBX_ALERT_TEXT_W Chicago 12 pixels.
 * defPtr is required; cancelPtr may be 0 only when otherPtr is 0; a label is
 * 1..TBX_ALERT_LABEL_MAX-1 bytes and fits its button (TBX_ALERT_BTN_TEXT_W, or
 * TBX_ALERT_OTHER_TEXT_W for the third). A refused alert is BADARG; a second
 * alert while one is pending is BUSY. Nothing is shown for a refused call.
 * GEOMETRY of the shown alert: the corpus caution alert (../system7-decomp/
 * goldens/captures/s8_alert_modal.png: frame 133..506 x 87..190, message pen x
 * 211, Cancel 363..421, OK 435..493, buttons y 158..177) with the default
 * button in OK's place and cancel in Cancel's; the THIRD button's place
 * (x 211..294, left-aligned under the message) is a product adaptation -- the
 * corpus has no three-button alert (specs/desktop/alerts.md golden-resolves).
 * The value 0x0090 is authored (no local reference for a number): the alert
 * starts a new dialog group after the Sec 10 file codes, and TBX_EVT_ALERT
 * reuses it so a TENANT-EVT what=144 line names its provenance, exactly as
 * TBX_EVT_MENU reuses 0x0051 (Sec 6a); outside every Event Manager code.
 *
 * TBX_GOAWAY_ASK (NEWWINDOW goAway == 2): the window has a close box, and a
 * click released inside it does NOT terminate the tenant; the shell PUSHES
 *     What = TBX_EVT_GOAWAY, Message = the window token,
 * and the tenant decides (the Mac application's own inGoAway handler: ask,
 * then close or not). goAway 1 keeps the V1 meaning (the close box ends the
 * tenant at once, DEC-AC3-3). Values above 2 are BADARG. TBX_EVT_GOAWAY is
 * authored: 0x0020, the NEWWINDOW code whose goAway argument asked for it.
 * ------------------------------------------------------------------------- */
#define TBX_EVT_ALERT          0x0090u
#define TBX_EVT_GOAWAY         0x0020u
#define TBX_GOAWAY_NONE        0u
#define TBX_GOAWAY_CLOSE       1u
#define TBX_GOAWAY_ASK         2u
#define TBX_ALERT_LINES        3u
#define TBX_ALERT_LINE_MAX     48u    /* bytes per line incl. the NUL        */
#define TBX_ALERT_TEXT_W       285u   /* the measured message column 211..495 */
#define TBX_ALERT_LABEL_MAX    16u    /* bytes per button label incl. NUL    */
#define TBX_ALERT_BTN_TEXT_W   43u    /* a 59-px measured button, 8-px margins */
#define TBX_ALERT_OTHER_TEXT_W 68u    /* the 84-px third button, 8-px margins */
#define TBX_ALERT_DEFAULT      1u
#define TBX_ALERT_CANCEL       2u
#define TBX_ALERT_OTHER        3u

/* ---------------------------------------------------------------------------
 * 12. THE SCRAP (bead initech-tdnl.16, absorbing initech-ww9c)
 *
 * THE MODEL: the Inside Macintosh Scrap Manager (../system7-decomp/specs/
 * toolbox/textedit.md Sec 6 TECut/TECopy/TEPaste "clipboard ops"; os/flair/
 * scrap.h, the shell-owned FlairScrap of bead initech-b2vk): ONE desk scrap,
 * owned by the SHELL (allocated once at desktop bring-up, fail loud), shared
 * by every application and surviving each of them -- text cut in one tenant
 * pastes in the next one launched. An application's Cut/Copy is ZeroScrap +
 * PutScrap('TEXT'); its Paste is GetScrap('TEXT'). The gate carries the TEXT
 * flavor only (PICT and the reserve slots stay shell-internal).
 *
 *   TBX_SCRAP_PUT (bufPtr, len) -> 0 | err
 *       ZeroScrap, then PutScrap('TEXT', len bytes at bufPtr). len 0 ..
 *       FLAIR_SCRAP_MAX_PAYLOAD (scrap.h, 8192); larger is BADARG and the
 *       scrap is untouched (the check precedes the ZeroScrap). The buffer may
 *       live in the module or the BSS (the Sec 10 span rule, PSP excluded).
 *   TBX_SCRAP_GET (bufPtr, bufSize) -> length | err
 *       GetScrap('TEXT'): copies the whole flavor to bufPtr and returns its
 *       byte count. bufPtr == 0 copies nothing and returns the length (IM:
 *       GetScrap with a NIL destination "returns the length"). No TEXT in the
 *       scrap -> TBX_ERR_NOTYPE (IM noTypeErr, -102). bufSize smaller than
 *       the flavor -> BADARG, nothing copied (never a partial paste, Rule 2).
 * Serial: TENANT-SCRAP put n=<len> count=<scrapCount> | get n=<len|rc>.
 * Values authored (no local number): the next dialog-free group after the
 * Sec 11 alert.
 * ------------------------------------------------------------------------- */
#define TBX_SCRAP_PUT         0x00A0u
#define TBX_SCRAP_GET         0x00A1u
#define TBX_ARGC_SCRAP_PUT    2u
#define TBX_ARGC_SCRAP_GET    2u

/* ---------------------------------------------------------------------------
 * 13. STANDARD FILE (bead initech-tdnl.16; the landed model os/flair/stdfile.h
 *     of bead initech-gymo, IM-I SFGetFile / SFPutFile / SFReply)
 *
 * THE ONLY SANCTIONED WAY AN APPLICATION NAMES A FILE (gymo): a shell-owned
 * modal dialog over the mounted data volume -- the real FAT directory read,
 * folders listed and entered, the volume root the top. Under the App Contract
 * a tenant never runs a modal loop (Sec 6a), so -- exactly like TBX_ALERT
 * (Sec 11) -- the dialog is ASKED for by the trap and ANSWERED by an event:
 *
 *   TBX_SFGETFILE (replyPtr) -> 0 | err            the Open dialog
 *   TBX_SFPUTFILE (promptPtr, namePtr, replyPtr) -> 0 | err
 *                                                   the Save As dialog: the
 *       prompt (ASCIZ in the image, 1..TBX_SF_PROMPT_MAX-1 bytes) over the
 *       name field, which starts as namePtr (ASCIZ in the image,
 *       0..TBX_SF_NAME_MAX-1 bytes) selected.
 *   replyPtr: TBX_SF_REPLY_BYTES writable bytes in the module or the BSS (the
 *       Sec 10 span rule), validated at the CALL.
 * The verb validates and returns at once; once the tenant has RETURNED the
 * shell runs the dialog and PUSHES
 *     What = TBX_EVT_SFREPLY, Message = 1 (good: replyPtr now holds the
 *            chosen file's NUL-terminated DOS path from the volume root,
 *            e.g. "LETTER.TXT" or "APPS\LETTER.TXT", ready for the Sec 10
 *            file verbs) or 0 (Cancel: replyPtr untouched),
 * Modifiers/When/Where zero. GetFile returns only a FILE (Open on a folder
 * enters it, gymo's folder-on-open rule); PutFile returns the typed 8.3 name
 * in the folder shown, after the shell has asked "Replace existing" when that
 * name already exists (a refused Replace keeps the dialog up, IM SFPutFile).
 * A second ask while one is pending is BUSY. Nothing is shown for a refused
 * call. Values authored (no local number).
 * Serial: TENANT-SF get|put up dir=<path> n=<entries>, TENANT-SF-REPLY good=<0|1>
 * [path=<reply>].
 * ------------------------------------------------------------------------- */
#define TBX_SFGETFILE         0x00B0u
#define TBX_SFPUTFILE         0x00B1u
#define TBX_ARGC_SFGETFILE    1u
#define TBX_ARGC_SFPUTFILE    3u
#define TBX_EVT_SFREPLY       0x00B0u
#define TBX_SF_REPLY_BYTES    128u   /* == TBX_FILE_PATH_MAX */
#define TBX_SF_PROMPT_MAX     40u
#define TBX_SF_NAME_MAX       13u    /* 8 + '.' + 3 + NUL */

/* ---------------------------------------------------------------------------
 * 14. MOUSE TRACKING (bead initech-tdnl.16 -- TEClick's drag)
 *
 * A Mac application tracks a press itself (TEClick loops on StillDown, IM-I
 * TextEdit); under the App Contract the loop is the SHELL's (Sec 6a: modal
 * tracking loops stay shell-owned helpers). So, again asked-then-answered:
 *
 *   TBX_TRACKMOUSE () -> 0 | err   legal only while a mouseDown is being
 *       delivered to the tenant (else BADARG). When the tenant RETURNS the
 *       shell follows the pointer until the button comes up, PUSHING
 *         What = TBX_EVT_TRACK, Message = 0, Where = the pointer (GLOBAL)
 *       for every sample whose position changed, then
 *         What = TBX_EVT_TRACK, Message = 1, Where = the release point,
 *       presenting what the tenant drew after each. A press the image's life
 *       ends inside is released at the last position (the tracker cut,
 *       bead initech-tdnl.59).
 * Serial: TENANT-TRACK up h=<h> v=<v> moves=<n>.
 * ------------------------------------------------------------------------- */
#define TBX_TRACKMOUSE        0x00C0u
#define TBX_ARGC_TRACKMOUSE   0u
#define TBX_EVT_TRACK         0x00C0u

/* ---------------------------------------------------------------------------
 * 15. THE VERTICAL SCROLL BAR (bead initech-tdnl.16; os/flair/winscroll.h,
 *     bead initech-tdnl.35 -- the standard bars every document window draws)
 *
 * A tenant window's standard bars are DISABLED until the tenant gives the
 * vertical one a range (winscroll.h: a window with no attached record has
 * nothing to scroll). The SHELL tracks every click on the bar (arrows, page
 * regions, thumb: winscroll.h's rules), and when the value changes it
 * invalidates the content, so the tenant hears an ordinary updateEvt and
 * reads the value back:
 *
 *   TBX_SETSCROLL (win, contentLen, value) -> the stored value | err
 *       the document is contentLen pixels tall (0..32767): the bar's range
 *       becomes contentLen minus the content height (winscroll.h
 *       wscroll_set_extent; 0 when the document fits = the DISABLED bar),
 *       value is clamped into it and returned; units are content PIXELS
 *       scrolled (the arrow step WSCROLL_LINE_DEFAULT, the page step the view
 *       less one step -- winscroll.h's stated choices).
 *   TBX_GETSCROLL (win) -> value | err
 * The horizontal bar stays disabled.
 *
 *   TBX_GLOBALTOLOCAL (win, h, v) -> (localV << 16) | (localH & 0xFFFF)
 *       IM GlobalToLocal: a GLOBAL event point (Sec 6) in the window's
 *       CONTENT-LOCAL coordinates (the TEXTDRAW/FILLRECT origin, Sec 7), packed
 *       as the IM Point LONGINT (v in the high word, h in the low; both int16).
 *       The window can be dragged by the shell at any time, so an application
 *       converts every mouse point through this verb, never through a
 *       remembered origin (IM-I QuickDraw GlobalToLocal; the content-click handlers of MTE ch. 2). Values outside int16 are
 *       BADARG.
 * ------------------------------------------------------------------------- */
#define TBX_SETSCROLL         0x00C1u
#define TBX_GETSCROLL         0x00C2u
#define TBX_GLOBALTOLOCAL     0x00C3u
#define TBX_ARGC_SETSCROLL    3u
#define TBX_ARGC_GETSCROLL    1u
#define TBX_ARGC_GLOBALTOLOCAL 3u

/* -------------------------------------------------------------------------
 * 16. THE FILE COPY DIALOG (bead initech-bf2; PRD Sec 6.3/6.5, Appendix A/B)
 *
 * WHY. PRD Sec 6.3 puts "the FILE COPY box" in the Dialog Manager and "the
 * progress bar" in the Control Manager; Sec 6.5 / Appendix B: "progress bar
 * fills at the rate set in michael_bolton.conf -> PROGRESS_BAR_RATE". Under
 * the App Contract a tenant never runs a modal loop (Sec 6a/11), so -- like
 * TBX_ALERT -- the copy is ASKED for by the trap and ANSWERED by an event.
 *
 * TBX_FILECOPY (captionPtr, srcPath, dstPath) -> 0 | err. The verb validates
 * and COPIES the three strings and returns at once: caption ASCIZ wholly in
 * the image, ONE line (no CR) of 1..TBX_ALERT_LINE_MAX-1 bytes measuring at
 * most TBX_FILECOPY_TEXT_W Chicago 12 pixels; paths ASCIZ in the image, 1..
 * TBX_FILE_PATH_MAX-1 bytes, resolved from the data-volume root exactly like
 * Sec 10 (8.3 names, '\' separators). BUSY while an ALERT or another
 * FILECOPY is pending. Once the tenant has RETURNED the shell:
 *   1. reads TBX_FILECOPY_CFG_PATH from the data volume THROUGH THE DOS
 *      HANDLE API and takes the decimal value of the first line that starts
 *      "PROGRESS_BAR_RATE=" -- percent of the bar per second, valid
 *      TBX_FILECOPY_RATE_MIN..MAX; missing file/key/out-of-range refuses the
 *      copy (nothing is shown, the delivered result is the error);
 *   2. opens srcPath for reading and CREATES dstPath (DOS 3Dh / 3Ch, the
 *      tenant's kernel-owned JFT, Sec 10), then runs the modal FILE COPY box
 *      (dialog.h FileCopyDialog: title "FILE COPY", the caption as its
 *      statText, the Platinum CDEF 5 progress bar, spec/progress_platinum.h);
 *   3. each tick sets the bar to v = min(100, rate * elapsed_ticks / 100)
 *      percent (PIT 100 Hz) and copies src -> dst (DOS 3Fh / 40h, chunks of
 *      TBX_FILECOPY_CHUNK) until size * v / 100 bytes are on disk -- the bar
 *      never runs ahead of the bytes; closes both files at v == 100;
 *   4. disposes the box, restores what it covered and PUSHES
 *        What = TBX_EVT_FILECOPY, Message = bytes copied (>= 0) or the
 *        negative TBX_ERR_* / TBX_ERR_DOS(code) that stopped it,
 *      Modifiers/When/Where zero.
 * The value 0x0091 is authored (no local reference for a number): the next
 * code in the Sec 11 dialog group; TBX_EVT_FILECOPY reuses it so a TENANT-EVT
 * what=145 line names its provenance (outside every Event Manager code).
 * MICHAEL.CFG is the 8.3 ALIAS of the PRD's michael_bolton.conf (a DOS 3.3
 * volume holds only 8.3 names); it lives in APPS beside the applications
 * because the data volume's root listing is locked by every R3.2/R3.3 gate.
 * ------------------------------------------------------------------------- */
#define TBX_EVT_FILECOPY       0x0091u
#define TBX_FILECOPY_TEXT_W    332u   /* FileCopyDialog statText 14..346     */
#define TBX_FILECOPY_CFG_PATH  "APPS\\MICHAEL.CFG"
#define TBX_FILECOPY_RATE_KEY  "PROGRESS_BAR_RATE="
#define TBX_FILECOPY_RATE_MIN  1u
#define TBX_FILECOPY_RATE_MAX  100u
#define TBX_FILECOPY_CHUNK     64u    /* bytes per DOS read/write            */
#define TBX_FILECOPY_CFG_MAX   256u   /* configuration bytes examined        */

#endif /* INITECH_SPEC_TOOLBOX_GATE_H */
