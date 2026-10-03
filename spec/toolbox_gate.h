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
 * row marked V2. Every other code -- including the reserved rows -- returns
 * TBX_ERR_BADCODE (the demux is total, never a silent no-op, Rule 2).
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
#define TBX_TEXTDRAW      0x0030u  /* V1  (win, x, y, strPtr, fg, bg) -> 0|err */
#define TBX_FILLRECT      0x0031u  /* V1  (win, l, t, r, b, color) -> 0 | err  */
#define TBX_FRAMERECT     0x0032u  /* reserved (V2)                            */
#define TBX_SETMBAR       0x0050u  /* V2  (barPtr) -> 0 | err  (Sec 9). A
                                    * tenant that never calls it has no menubar
                                    * and band 2 shows the shell fallback bar
                                    * (kmain flair_live_tenant_bar)            */
#define TBX_EXIT          0x0070u  /* V1  (rc) -> 0 ; teardown runs when the
                                    * tenant next RETURNS to the kernel         */

/* Argument arity per V1 code (the dispatcher reads exactly this many dwords;
 * the per-call TENANT-GATE trace prints exactly this many). */
#define TBX_ARGC_REGISTER   1u
#define TBX_ARGC_NEWWINDOW  5u
#define TBX_ARGC_SETWTITLE  2u
#define TBX_ARGC_TEXTDRAW   6u
#define TBX_ARGC_FILLRECT   6u
#define TBX_ARGC_EXIT       1u
#define TBX_ARGC_SETMBAR    1u
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
 * 7. DRAWING TOKENS (C-8 discipline: tenants name ROLES, never RGB/indices)
 *
 * TEXTDRAW/FILLRECT coordinates are CONTENT-LOCAL (origin = the window's
 * content top-left); TEXTDRAW's y is the top of the Chicago 8x16 cell. Every
 * draw is clipped to (the window's VISIBLE region INTERSECT its content), and
 * additionally to its updateRgn while an updateEvt is being delivered.
 * ------------------------------------------------------------------------- */
#define TBX_COLOR_WHITE  0u   /* -> FLAIR_PART_CONTENT       */
#define TBX_COLOR_BLACK  1u   /* -> FLAIR_PART_FRAME         */
#define TBX_COLOR_GRAY   2u   /* -> FLAIR_PART_BTNFACE       */
#define TBX_COLOR_NAVY   3u   /* -> FLAIR_PART_CAPTION_NAVY  */
#define TBX_COLOR_COUNT  4u

/* ---------------------------------------------------------------------------
 * 8. THE V1 MEMORY MODEL (DEC-AC3-1; D1-3)
 *
 * ONE resident disk tenant (a kernel slot; a second launch fails loud with
 * TENANT-SLOT-BUSY and changes nothing). The code block is carved from the
 * master FLAIR heap (FLAIR_CLASS_GENERAL) as
 *     [ PSP (TBX_TENANT_PSP_BYTES) | load module | e_minalloc BSS ]
 * sized TBX_TENANT_PSP_BYTES + roundup16(file size); the MZ header the module
 * is moved down over supplies the BSS slack, and an e_minalloc that does not
 * fit is refused (TENANT-LOAD-FAIL), never overrun. Then the AC-2 trio
 * (handle / records / data) at registration.
 * ------------------------------------------------------------------------- */
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
 * Sec 5 run: Apple slot + sum(8*len + 14) <= FLAIR_SCREEN_W). n_menus == 0 is
 * legal (an empty bar, IM ClearMenuBar).
 *
 * SEMANTICS (V2). SetMenuBar, not DrawMenuBar: SETMBAR installs the bar; band
 * 2 shows it the next time the tenant is drawn as foreground -- its
 * affirmation at the first NEWWINDOW (DEC-AC3-4), or any later switch back to
 * it -- through flair_live_finish_tenant_switch, the ONE swap path. Accepted
 * only BEFORE the tenant's first NEWWINDOW (later -> TBX_ERR_BUSY): with no
 * DrawMenuBar trap yet, a later swap would leave band 2 showing a bar the
 * pull-down no longer matches. On EXIT / close / crash the app leaves the
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
#define TBX_MBAR_MAX_MENUS    8u
#define TBX_MBAR_MAX_ITEMS    16u
#define TBX_MBAR_STR_MAX      32u    /* incl. the NUL                          */

#endif /* INITECH_SPEC_TOOLBOX_GATE_H */
