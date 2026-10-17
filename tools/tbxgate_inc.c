/*
 * tools/tbxgate_inc.c -- emit the NASM view of the Initech Toolbox Gate
 * (factory C; beads initech-tdnl.22 R5.2).
 *
 * The Turbo Initech binding's hand-assembled stubs (seed/rt/flairstub.asm)
 * need the gate's vector, AX codes, arities, the FlairTenantRec and FlairEvent
 * layouts and the MenuBar resource layout. Those numbers are locked spec-data
 * in spec/toolbox_gate.h (Rule 8); NASM cannot include a C header, and typing
 * them a second time into the .asm is exactly the drift Rule 8 forbids. So
 * this tool #includes the header and PRINTS every value the stubs use as a
 * NASM %define: the .inc is generated at build time (build/flairgate.inc),
 * never edited, never checked in. A value the stub needs that is not printed
 * here is an undefined NASM symbol -- the assembly fails loud.
 *
 * Usage: tbxgate_inc > flairgate.inc
 * Deterministic (Rule 11): a pure function of the header. ASCII (Rule 12).
 */
#include <stdio.h>
#include <stdint.h>
#include "toolbox_gate.h"

#define D(name) printf("%%define %-26s %ld\n", #name, (long)(int64_t)(name))

int main(void)
{
    printf("; flairgate.inc -- GENERATED from spec/toolbox_gate.h by "
           "tools/tbxgate_inc.c.\n; Do not edit (Rule 8: the header is the "
           "only copy of these values).\n");
    /* Sec 1-2: the gate */
    D(TBX_GATE_VECTOR);
    D(TBX_GATE_FRAME_BYTES);
    /* Sec 3: the codes the binding traps, and their arities */
    D(TBX_REGISTER);      D(TBX_ARGC_REGISTER);
    D(TBX_EXIT);          D(TBX_ARGC_EXIT);
    D(TBX_NEWWINDOW);     D(TBX_ARGC_NEWWINDOW);
    D(TBX_SETWTITLE);     D(TBX_ARGC_SETWTITLE);
    D(TBX_CONTENTORIGIN); D(TBX_ARGC_CONTENTORIGIN);
    D(TBX_GLOBALTOLOCAL); D(TBX_ARGC_GLOBALTOLOCAL);
    D(TBX_TEXTDRAW);      D(TBX_ARGC_TEXTDRAW);
    D(TBX_FILLRECT);      D(TBX_ARGC_FILLRECT);
    D(TBX_DRAWCELLS);     D(TBX_ARGC_DRAWCELLS);
    D(TBX_SETMBAR);       D(TBX_ARGC_SETMBAR);
    D(TBX_DRAWMENUBAR);   D(TBX_ARGC_DRAWMENUBAR);
    D(TBX_ALERT);         D(TBX_ARGC_ALERT);
    D(TBX_SCRAP_PUT);     D(TBX_ARGC_SCRAP_PUT);
    D(TBX_SCRAP_GET);     D(TBX_ARGC_SCRAP_GET);
    /* Sec 4 */
    D(TBX_OK);
    D(TBX_ERR_BADARG);
    /* Sec 5: the link-synthesized record */
    D(FLAIR_TENANT_REC_TAG);
    D(FLAIR_TENANT_REC_TAG_OFF);
    D(FLAIR_TENANT_REC_NAME_OFF);
    D(FLAIR_TENANT_REC_BASE_OFF);
    D(FLAIR_TENANT_REC_LEN_OFF);
    D(FLAIR_TENANT_REC_EVB_OFF);
    D(FLAIR_TENANT_REC_PROC_OFF);
    D(FLAIR_TENANT_REC_BYTES);
    D(FLAIR_TENANT_NAME_MAX);
    /* Sec 6: the flat event */
    D(FLAIR_EVBUF_BYTES);
    /* Sec 9: the MenuBar resource */
    D(TBX_MBAR_BYTES);
    D(TBX_MBAR_MENUS_OFF);
    D(TBX_MBAR_COUNT_OFF);
    D(TBX_MBAR_APPLE_OFF);
    D(TBX_MINFO_BYTES);
    D(TBX_MINFO_ID_OFF);
    D(TBX_MINFO_TITLE_OFF);
    D(TBX_MINFO_ITEMS_OFF);
    D(TBX_MINFO_COUNT_OFF);
    D(TBX_MITEM_BYTES);
    D(TBX_MITEM_TEXT_OFF);
    D(TBX_MITEM_MARK_OFF);
    D(TBX_MITEM_CMD_OFF);
    D(TBX_MITEM_STYLE_OFF);
    D(TBX_MITEM_ENABLED_OFF);
    D(TBX_MITEM_DIVIDER_OFF);
    D(TBX_MINFO_WIDTH_OFF);
    D(TBX_MBAR_MAX_MENUS);
    D(TBX_MBAR_STR_MAX);
    /* Sec 11: the alert's label bound */
    D(TBX_ALERT_LABEL_MAX);
    return 0;
}
