/* Retired source-layout META checks, split from test_region_gdi.c.
 * Not built. Paths and predicates are retained for historical reference. */
#include <stdio.h>
#include <stdlib.h>
#include "test_assert.h"
TEST_HARNESS();

/* ===========================================================================
 * L3 -- the single-engine structural grep gate (load-bearing per AM-1 Sec 2.3 /
 * Sec 7.3 / constraint C-7). FAIL LOUD if any region_op / xmerge / band-sweep
 * DEFINITION appears OUTSIDE os/flair/atkinson/region.c. We scan os/ for a
 * C-function-DEFINITION of those names (a return-type line ending in the name +
 * '(' that is NOT a call and NOT a prototype ';'); region.c is the one allowed
 * site. The wine engine's own static region_op is gdi_ref_-namespaced and lives
 * under ../win31-decomp (outside os/), so it is invisible to this scan by both
 * its namespace AND its location.
 *
 * Implemented as a portable grep via system(): we search os/ (the artifact tree)
 * for a DEFINITION pattern, EXCLUDING region.c, and require zero hits. A
 * second-engine definition (a new region_op/xmerge in os/) makes this RED.
 * ===========================================================================*/
static int l3_single_engine_gate(void)
{
    /* A definition looks like  "<type> region_op(" / "...xmerge(" at line start
     * (allowing static/inline qualifiers + a return type), and crucially is NOT
     * followed by ';' on the same logical line (a prototype) and is NOT a call
     * (calls are indented and preceded by '=', 'return', etc.). We approximate
     * "definition outside region.c" with: a line that matches the function-head
     * regex for region_op|xmerge|active_xlist (the band-sweep helpers), in any
     * os/ .c file whose path is NOT region.c. The engine's own region.c is the
     * sole legal home; everything else is a second-engine violation.
     *
     * grep -rnE over os/ ; --include='*.c' ; exclude region.c ; the ERE matches
     * a function head (start-of-line, optional qualifiers/type, the name, '('),
     * NOT a call (no leading '.'/'->'/'=' and not 'return'). We additionally
     * require the line to NOT end in ';' (prototype) by excluding ');' tails. */
    const char *cmd =
        "grep -rnE "
        "'^[A-Za-z_][A-Za-z0-9_ \\t\\*]*[ \\t\\*](region_op|xmerge)[ \\t]*\\(' "
        "os --include='*.c' "
        "| grep -v 'os/flair/atkinson/region.c' "
        "| grep -vE ';[ \\t]*$' "       /* drop prototype lines (end in ';')     */
        "> /dev/null 2>&1";
    int rc = system(cmd);
    /* grep exits 0 when it FOUND a match (=> a violation), 1 when none. We want
     * NONE outside region.c, so PASS iff grep found nothing (rc != 0). */
    int found = (rc == 0);
    if (found) {
        fprintf(stderr,
            "  L3 FAIL: a region_op/xmerge DEFINITION exists OUTSIDE "
            "os/flair/atkinson/region.c -- a SECOND region engine (C-7 violation):\n");
        /* re-run visibly so the operator sees the offending line(s). */
        (void)system(
            "grep -rnE "
            "'^[A-Za-z_][A-Za-z0-9_ \\t\\*]*[ \\t\\*](region_op|xmerge)[ \\t]*\\(' "
            "os --include='*.c' "
            "| grep -v 'os/flair/atkinson/region.c' "
            "| grep -vE ';[ \\t]*$' 1>&2");
        return 0;
    }
    return 1;
}

/* Positive control for L3: confirm the gate actually SEES region.c's region_op
 * definition (so a gate that matches nothing -- e.g. a broken regex -- is caught
 * as decoration, not silently green). */
static int l3_positive_control(void)
{
    const char *cmd =
        "grep -rnE "
        "'^[A-Za-z_][A-Za-z0-9_ \\t\\*]*[ \\t\\*](region_op|xmerge)[ \\t]*\\(' "
        "os/flair/atkinson/region.c "
        "| grep -vE ';[ \\t]*$' > /dev/null 2>&1";
    return system(cmd) == 0;   /* must FIND region.c's own definitions */
}

int main(void)
{
    /* ======================================================================
     * L3 -- single-engine structural gate.
     * ====================================================================== */
    CHECK(l3_positive_control(),
          "L3 [control]: the grep gate SEES region.c's own region_op/xmerge "
          "definitions (gate is not decoration)");
    CHECK(l3_single_engine_gate(),
          "L3 [structural]: no region_op/xmerge DEFINITION outside "
          "os/flair/atkinson/region.c (constraint C-7)");

    return TEST_SUMMARY("test-region-meta");
}
