/* test_sft.c -- host unit oracle for the SFT/JFT handle layer (beads
 * initech-509.3). Factory test: libc OK, reuses seed/test_assert.h. Compiles
 * HOSTED against the REAL artifact sft.c + psp.c (the same code the kernel runs;
 * both compile freestanding and hosted).
 *
 * Ref: ADR-0003 Sec 5.6 (DEC-06: 20-entry JFT -> system SFT; handles 0-4
 *      predefined), Appendix D.2 (FILES=20); docs/research/
 *      fs-mount-sft-ground-truth.md Sec 3 (the model + DUP/DUP2 algorithms);
 *      DOS 3.3 Programmer's Reference Manual AH=45h/46h. CLAUDE.md Law 2 (oracle
 *      is truth), Rule 1 (RED->GREEN), Rule 6 (mutation-prove), Rule 12 (ASCII).
 *
 * Strategy: build a standard process PSP (psp_build lays the predefined JFT) +
 * sft_init() (lays the device entries), then exercise:
 *   - the predefined handles 0-4 resolve to the correct device SFT slots;
 *   - handles 5..19 are closed; out-of-range/closed handles resolve to NULL;
 *   - jft_alloc / sft_alloc find the lowest free slot and saturate when full;
 *   - DUP duplicates a handle (new lowest slot, shared SFT entry, ref_count++);
 *   - DUP2 redirects a handle (incl. stdout->a FILE slot), releasing the old
 *     target and updating ref counts; src==dst is a no-op; bad handles error.
 *
 * MUTATION (Rule 6), driven by make:
 *   -DSFT_MUTATE_DUP_NO_REFCOUNT  : DUP forgets to bump ref_count -> the DUP
 *                                   ref_count assertion goes RED.
 *   -DSFT_MUTATE_DUP2_NO_RELEASE  : DUP2 forgets to release the old target ->
 *                                   the released-slot assertion goes RED.
 * A mutant that PASSES means the oracle is decoration.
 */

#include <stdint.h>
#include <string.h>

#include "sft.h"
#include "psp.h"
#include "test_assert.h"

TEST_HARNESS();

/* Build a standard process PSP (the predefined JFT: jft[0]=0,jft[1]=1,jft[2]=1,
 * jft[3]=2,jft[4]=3, rest 0xFF) into *p, and (re-)establish the device SFT. */
static void fresh_process(psp_t *p)
{
    psp_params_t params;
    params.alloc_end_linear  = 0x00070000u;
    params.env_linear        = 0u;
    params.parent_psp_linear = 0u;
    params.cmd_tail          = (const char *)0;
    params.cmd_tail_len      = 0u;
    (void)psp_build(p, &params);
    sft_init();
}

int main(void)
{
    /* ============================ predefined handles ===================== */
    {
        psp_t p;
        fresh_process(&p);

        /* The JFT psp_build laid down maps handles 0-4 onto device SFT slots. */
        CHECK(p.jft[0] == SFT_SLOT_CON_IN,  "jft[0] stdin  -> SFT slot 0 (CON in)");
        CHECK(p.jft[1] == SFT_SLOT_CON_OUT, "jft[1] stdout -> SFT slot 1 (CON out)");
        CHECK(p.jft[2] == SFT_SLOT_CON_OUT, "jft[2] stderr -> SFT slot 1 (CON out, shared)");
        CHECK(p.jft[3] == SFT_SLOT_AUX,     "jft[3] aux    -> SFT slot 2 (AUX)");
        CHECK(p.jft[4] == SFT_SLOT_PRN,     "jft[4] prn    -> SFT slot 3 (PRN)");

        sft_entry_t *in  = sft_from_handle(&p, 0);
        sft_entry_t *out = sft_from_handle(&p, 1);
        sft_entry_t *err = sft_from_handle(&p, 2);
        sft_entry_t *aux = sft_from_handle(&p, 3);
        sft_entry_t *prn = sft_from_handle(&p, 4);
        CHECK(in  && in->kind == SFT_KIND_DEVICE && in->dev_id == SFT_DEV_CON
              && in->open_mode == SFT_MODE_READ, "handle 0 = CON device, read");
        CHECK(out && out->kind == SFT_KIND_DEVICE && out->dev_id == SFT_DEV_CON
              && out->open_mode == SFT_MODE_WRITE, "handle 1 = CON device, write");
        CHECK(err == out, "handle 2 (stderr) resolves to the SAME entry as handle 1 (stdout)");
        CHECK(aux && aux->kind == SFT_KIND_DEVICE && aux->dev_id == SFT_DEV_AUX,
              "handle 3 = AUX device");
        CHECK(prn && prn->kind == SFT_KIND_DEVICE && prn->dev_id == SFT_DEV_PRN,
              "handle 4 = PRN device");

        /* CON-out is referenced by stdout AND stderr -> ref_count 2. */
        CHECK(out->ref_count == 2u, "CON-out slot ref_count = 2 (stdout + stderr)");
        CHECK(in->ref_count == 1u,  "CON-in slot ref_count = 1 (stdin)");

        /* Handles 5..19 are closed (jft == 0xFF) -> resolve to NULL. */
        CHECK(p.jft[5] == JFT_CLOSED, "jft[5] is the closed sentinel");
        CHECK(sft_from_handle(&p, 5) == 0, "closed handle 5 resolves to NULL");
        CHECK(sft_from_handle(&p, 19) == 0, "closed handle 19 resolves to NULL");

        /* Out-of-range handle and a NULL psp resolve to NULL (not a crash). */
        CHECK(sft_from_handle(&p, 20) == 0, "handle 20 (out of range) -> NULL");
        CHECK(sft_from_handle(&p, 255) == 0, "handle 255 (out of range) -> NULL");
        CHECK(sft_from_handle((const psp_t *)0, 1) == 0, "NULL psp -> NULL");
    }

    /* ============================ allocators ============================= */
    {
        psp_t p;
        fresh_process(&p);

        /* Lowest free JFT slot is 5 (0-4 predefined). */
        CHECK(jft_alloc(&p) == 5u, "jft_alloc -> lowest free slot 5");

        /* Lowest free SFT slot is 4 (0-3 are the device slots). */
        CHECK(sft_alloc() == SFT_FIRST_FILE, "sft_alloc -> lowest free file slot 4");

        /* Fill every JFT slot; jft_alloc must then saturate to JFT_CLOSED. */
        for (uint8_t i = 5; i < (uint8_t)JFT_MAX_ENTRIES; i++) {
            p.jft[i] = SFT_SLOT_CON_OUT; /* point at a live slot (any live idx) */
        }
        CHECK(jft_alloc(&p) == JFT_CLOSED, "jft_alloc saturates to 0xFF when full");

        /* Fill every file SFT slot; sft_alloc must saturate to SFT_MAX_ENTRIES. */
        for (uint8_t i = (uint8_t)SFT_FIRST_FILE; i < (uint8_t)SFT_MAX_ENTRIES; i++) {
            g_sft[i].kind = SFT_KIND_FILE;
        }
        CHECK(sft_alloc() == (uint8_t)SFT_MAX_ENTRIES,
              "sft_alloc saturates to SFT_MAX_ENTRIES when full");
    }

    /* ============================ DUP (45h) ============================== */
    {
        psp_t p;
        fresh_process(&p);

        uint8_t newh = 0xEE;
        uint16_t rc = sft_dup(&p, 1, &newh); /* duplicate stdout */
        CHECK(rc == SFT_OK, "DUP(stdout) succeeds");
        CHECK(newh == 5u, "DUP -> lowest free handle 5");
        CHECK(p.jft[5] == p.jft[1], "DUP'd handle aliases the SAME SFT slot as stdout");
        /* stdout slot now referenced by jft[1], jft[2] (stderr), jft[5] = 3. */
        CHECK(g_sft[SFT_SLOT_CON_OUT].ref_count == 3u,
              "DUP bumps the shared SFT slot ref_count to 3");

        /* DUP of a closed handle -> invalid handle, out param untouched. */
        uint8_t before = newh;
        rc = sft_dup(&p, 9, &newh);
        CHECK(rc == SFT_ERR_INVALID_HANDLE, "DUP(closed handle) -> invalid handle");
        CHECK(newh == before, "DUP error leaves out_handle untouched");
    }

    /* ===================== DUP exhausts the JFT ========================== */
    {
        psp_t p;
        fresh_process(&p);
        /* Occupy 5..18, leaving exactly slot 19 free, then DUP twice. */
        for (uint8_t i = 5; i < 19; i++) {
            p.jft[i] = SFT_SLOT_CON_OUT;
        }
        uint8_t h = 0;
        CHECK(sft_dup(&p, 1, &h) == SFT_OK && h == 19u, "DUP fills the last free slot 19");
        CHECK(sft_dup(&p, 1, &h) == SFT_ERR_TOO_MANY_OPEN,
              "DUP with no free JFT slot -> too many open files (0x0004)");
    }

    /* ============================ DUP2 (46h) ============================= */
    {
        psp_t p;
        fresh_process(&p);

        /* Stand up a FILE entry (simulating a future AH=3Dh OPEN result) and
         * point a fresh handle (6) at it, so we can redirect stdout to it. */
        uint8_t fslot = sft_alloc();
        CHECK(fslot == SFT_FIRST_FILE, "file opens at SFT slot 4");
        memset(&g_sft[fslot], 0, sizeof(g_sft[fslot]));
        g_sft[fslot].kind      = SFT_KIND_FILE;
        g_sft[fslot].open_mode = SFT_MODE_WRITE;
        g_sft[fslot].ref_count = 1u;
        p.jft[6] = fslot;

        uint16_t out_before = g_sft[SFT_SLOT_CON_OUT].ref_count; /* 2 */
        uint16_t file_before = g_sft[fslot].ref_count;           /* 1 */

        /* DUP2(src=file handle 6, dst=stdout handle 1): redirect stdout to the
         * file. The old stdout target (CON-out slot) loses a reference; the file
         * slot gains one; handle 1 now resolves to the FILE entry. */
        uint16_t rc = sft_dup2(&p, 6, 1);
        CHECK(rc == SFT_OK, "DUP2(file -> stdout) succeeds");
        CHECK(p.jft[1] == fslot, "stdout (handle 1) now points at the file SFT slot");
        sft_entry_t *now = sft_from_handle(&p, 1);
        CHECK(now && now->kind == SFT_KIND_FILE,
              "handle 1 resolves to the FILE entry after redirect");
        CHECK(g_sft[fslot].ref_count == (uint16_t)(file_before + 1u),
              "DUP2 bumps the file slot ref_count");
        CHECK(g_sft[SFT_SLOT_CON_OUT].ref_count == (uint16_t)(out_before - 1u),
              "DUP2 releases one reference from the old stdout (CON-out) slot");

        /* stderr (handle 2) still points at CON-out -> the slot is NOT freed. */
        CHECK(p.jft[2] == SFT_SLOT_CON_OUT, "stderr still on CON-out after stdout redirect");
        CHECK(g_sft[SFT_SLOT_CON_OUT].kind == SFT_KIND_DEVICE,
              "CON-out slot stays live (stderr still references it)");
    }

    /* ============ DUP2 frees the old slot when its last ref goes ========= */
    {
        psp_t p;
        fresh_process(&p);
        /* Open a file at slot 4, attach handle 6 to it (ref_count 1). Then
         * DUP2(stdin -> handle 6): handle 6 was the file's ONLY reference, so
         * the file slot must be freed. */
        uint8_t fslot = sft_alloc();
        memset(&g_sft[fslot], 0, sizeof(g_sft[fslot]));
        g_sft[fslot].kind = SFT_KIND_FILE;
        g_sft[fslot].ref_count = 1u;
        p.jft[6] = fslot;

        CHECK(sft_dup2(&p, 0, 6) == SFT_OK, "DUP2(stdin -> handle 6) succeeds");
        CHECK(g_sft[fslot].kind == SFT_KIND_FREE,
              "the file slot is FREED when DUP2 drops its last reference");
        CHECK(p.jft[6] == p.jft[0], "handle 6 now aliases stdin's SFT slot");
    }

    /* ============================ DUP2 edge cases ======================== */
    {
        psp_t p;
        fresh_process(&p);

        /* src == dst is a no-op success; ref counts unchanged. */
        uint16_t before = g_sft[SFT_SLOT_CON_OUT].ref_count;
        CHECK(sft_dup2(&p, 1, 1) == SFT_OK, "DUP2(h,h) is a success no-op");
        CHECK(g_sft[SFT_SLOT_CON_OUT].ref_count == before,
              "DUP2(h,h) leaves ref_count unchanged");

        /* Bad source handle -> invalid handle. */
        CHECK(sft_dup2(&p, 9, 1) == SFT_ERR_INVALID_HANDLE,
              "DUP2 with a closed source -> invalid handle");
        /* Out-of-range destination -> invalid handle. */
        CHECK(sft_dup2(&p, 1, 20) == SFT_ERR_INVALID_HANDLE,
              "DUP2 with an out-of-range destination -> invalid handle");

        /* DUP2 onto a CLOSED destination (handle 7): no release, just alias. */
        CHECK(sft_dup2(&p, 1, 7) == SFT_OK, "DUP2 onto a closed dst succeeds");
        CHECK(p.jft[7] == p.jft[1], "DUP2 aliases the closed dst to the source slot");
    }

    return TEST_SUMMARY("test_sft");
}
