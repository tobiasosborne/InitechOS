/* Resident console entries must outlive any child redirects. Ref: IBM DOS
 * 3.30 TR p.4-15 (handle ownership); invariant, not DOS-internal layout parity.
 * Uses the existing resident-device representation (sft_inherit/close_process).
 */
#include <stdio.h>
#include <string.h>
#include "sft.h"
#include "psp.h"
#include "test_assert.h"
TEST_HARNESS();
int main(void)
{
    for (unsigned saved = 0; saved < 2; saved++) {
        psp_t parent, child;
        psp_params_t params = {0};
        uint8_t backup = JFT_CLOSED;
        psp_build(&parent, &params);
        sft_init();
        if (saved) CHECK(sft_dup(&parent, 1, &backup) == SFT_OK, "save parent's console");
        memcpy(&child, &parent, sizeof child);
        sft_inherit(&child);
        uint8_t slot = sft_alloc();
        g_sft[slot].kind = SFT_KIND_FILE;
        g_sft[slot].ref_count = 1;
        child.jft[19] = slot;
        /* Replace every inherited device alias, including the saved DUP. */
        for (unsigned h = 0; h < 19; h++)
            if (child.jft[h] != JFT_CLOSED)
                CHECK(sft_dup2(&child, 19, h) == SFT_OK, "redirect child device alias");
        for (unsigned h = 0; h < 5; h++)
            CHECK(g_sft[parent.jft[h]].kind == SFT_KIND_DEVICE,
                  "parent's console/AUX/PRN entry remains live after child redirection");
        /* Avoid dereferencing the corrupt parent's JFT on the red run. */
        if (g_sft[SFT_SLOT_CON_OUT].kind == SFT_KIND_FREE)
            return TEST_SUMMARY("console-inherit");
        sft_close_process(&child);
        sft_close_process(&child);
        CHECK(g_sft[slot].kind == SFT_KIND_FREE, "child file references reclaimed exactly once");
        if (saved) CHECK(sft_dup2(&parent, backup, 1) == SFT_OK, "restore parent's console");
        for (unsigned h = 0; h < 5; h++)
            CHECK(sft_from_handle(&parent, h)->kind == SFT_KIND_DEVICE,
                  "parent uses devices after child exit");
    }
    return TEST_SUMMARY("console-inherit");
}
