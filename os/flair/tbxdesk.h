/* tbxdesk.h -- THE RESIDENT TOOLBOX ENTRY: the desk state and the gate rule
 * behind INT 81h AX=TBX_DESKSTART (bead initech-tdnl.106; ADR-0003 Amendment
 * DEC-19; spec/toolbox_gate.h Sec 17).
 *
 * THE MODEL (Law 1). DOS 3.30 Technical Reference pp1-5/1-6 (PDF22-23): the
 * resident portion of COMMAND.COM holds the EXEC routine and the transient
 * reload; the transient issues EXEC for an external command and regains the
 * console when the program terminates. FLAIR is such an external command:
 * FLAIR.COM (\DOS, like WIN.COM, a separate launcher program) asks the
 * resident Toolbox to start the desktop and terminates; the desktop then runs
 * in the resident system's context, inside the EXEC that ran FLAIR.COM, until
 * Special > Exit to InitechDOS hands the console back to COMMAND.COM.
 *
 * THE STATE MACHINE (three states, one owner -- the kernel's tbxgate.c):
 *
 *   DORMANT --TBX_DESKSTART from a running DOS program--> ARMED
 *   ARMED   --the EXEC that ran the program returns (tbx_desk_take)--> ACTIVE
 *   ACTIVE  --Special > Exit to InitechDOS (tbx_desk_leave)--> DORMANT
 *
 * THE GATE RULE (tbx_desk_start), in order:
 *   - the desktop is ARMED or ACTIVE      -> TBX_ERR_BUSY (one desktop: a
 *     second FLAIR while it runs is REFUSED, never nested, never restarted);
 *   - the caller is not a DOS program the loader is running (a stray int 81h
 *     from the kernel's own context)      -> TBX_ERR_BADCODE, nothing armed;
 *   - otherwise                           -> TBX_OK, ARMED.
 * A disk tenant can only run while the desktop is ACTIVE, so its call is the
 * BUSY case by construction (in_tenant is checked anyway: Rule 2).
 *
 * Header-only and freestanding so the kernel (os/flair/tbxgate.c) and the
 * host oracle (harness/proptest/test_flair_cmd.c) compile the SAME rule.
 *
 * MUTANTS (Rule 6; each defined ONLY by its mutant build, NEVER in a real one):
 *   TBXDESK_MUT_FLAIR_TWICE  the BUSY check is gone: a second FLAIR while the
 *                            desktop runs is accepted (test-flair-cmd-host D1
 *                            "second start" RED; test-flair-from-dos-mutant).
 *   TBXDESK_MUT_LEAVE_KEEPS  leave does not return the desk to DORMANT (the
 *                            host twin of KMAIN_MUT_EXIT_RESETS_CONSOLE_ONLY).
 * ASCII-clean (Rule 12); no state outside the caller's tbx_desk_t (Rule 11).
 */
#ifndef INITECH_FLAIR_TBXDESK_H
#define INITECH_FLAIR_TBXDESK_H

#include <stdint.h>
#include "toolbox_gate.h"

#define TBX_DESK_DORMANT 0u
#define TBX_DESK_ARMED   1u
#define TBX_DESK_ACTIVE  2u

typedef struct tbx_desk {
    uint8_t state;   /* TBX_DESK_DORMANT / _ARMED / _ACTIVE */
} tbx_desk_t;

/* INT 81h AX=TBX_DESKSTART. in_tenant: a disk tenant's own gate call is in
 * flight; in_program: the loader is running a DOS program (the caller). */
static inline int32_t tbx_desk_start(tbx_desk_t *d, int in_tenant,
                                     int in_program)
{
#ifndef TBXDESK_MUT_FLAIR_TWICE
    if (d->state != TBX_DESK_DORMANT || in_tenant)
        return TBX_ERR_BUSY;
#else
    (void)in_tenant;
#endif
    if (!in_program)
        return TBX_ERR_BADCODE;
    d->state = TBX_DESK_ARMED;
    return TBX_OK;
}

/* The EXEC that ran the arming program has returned: ARMED -> ACTIVE.
 * Returns 1 when the desktop must start now, 0 when nothing was armed. */
static inline int tbx_desk_take(tbx_desk_t *d)
{
    if (d->state != TBX_DESK_ARMED) return 0;
    d->state = TBX_DESK_ACTIVE;
    return 1;
}

/* Special > Exit to InitechDOS: the desktop is gone, the Toolbox dormant. */
static inline void tbx_desk_leave(tbx_desk_t *d)
{
#ifndef TBXDESK_MUT_LEAVE_KEEPS
    d->state = TBX_DESK_DORMANT;
#else
    (void)d;
#endif
}

#endif /* INITECH_FLAIR_TBXDESK_H */
