/*
 * test_flair_cmd.c -- FLAIR AS A COMMAND: the two rules under it (HOST oracle).
 *
 * bead: initech-tdnl.106 (boot to InitechDOS, type FLAIR, Exit returns to the
 *       prompt); ADR-0003 Amendment DEC-19 "The resident Toolbox entry".
 *
 * D1 THE GATE RULE (os/flair/tbxdesk.h, the code tbxgate.c compiles for INT
 *    81h AX=TBX_DESKSTART; spec/toolbox_gate.h Sec 17):
 *      - a DOS program arms a dormant desktop; the EXEC's return starts it;
 *      - a second FLAIR while it is armed or running is REFUSED (BUSY), from a
 *        DOS program or from a disk tenant, and refusing changes nothing;
 *      - a stray int 81h from the kernel's own context arms nothing;
 *      - Exit makes it dormant again and FLAIR works again (the round trip,
 *        twice);
 *      - an ordinary program's EXEC return starts nothing.
 *    The function value is hand-typed (0x0003) and checked against every
 *    other code of the Sec 3 map, hand-typed from the spec text.
 * D2 THE TRANSIENT CHECKSUM RULE (os/milton/transient.h; IBM DOS 3.30
 *    Technical Reference p1-5, PDF22: "a checksum methodology determines if
 *    the program has caused the transient portion to be overlaid. If the
 *    transient portion is overlaid, it is reloaded."):
 *      - the word checksum of hand-typed bytes equals hand-computed sums;
 *      - over a synthetic transient laid out like transient_hdr_t, any one
 *        byte changed in the header/.text/.rodata span is an overlay, while a
 *        byte changed in .data (the interpreter's live state) is not.
 * D3 FLAIR.COM CARRIES THE TWO LOCKED LINES (argv[1] = the built binary):
 *    TBX_DESK_MSG_BUSY and TBX_DESK_MSG_FAIL each occur in it exactly once,
 *    each followed by CR LF '$' (the AH=09h form).
 *
 * Expectations are HAND-TYPED (Law 2): states, return codes, sums.
 * MUTANTS (Rule 6; -D on this TU):
 *   TBXDESK_MUT_FLAIR_TWICE  -> D1 "second FLAIR while running" RED
 *   TBXDESK_MUT_LEAVE_KEEPS  -> D1 "FLAIR again after Exit" RED
 *   TRANSIENT_MUT_NO_CHECK   -> D2 "code byte" RED
 *   TRANSIENT_MUT_SUM_DATA   -> D2 "data byte" RED
 * ASCII-clean (Rule 12).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tbxdesk.h"
#include "memory_map.h"
#include "transient.h"
#include "test_assert.h"

TEST_HARNESS();

static void d1_gate_rule(void)
{
    tbx_desk_t d = { 0 };
    /* Sec 3, hand-typed from the spec text: every other function code. */
    static const uint32_t others[] = {
        0x0001u, 0x0002u, 0x0010u, 0x0020u, 0x0021u, 0x0022u, 0x0023u,
        0x0030u, 0x0031u, 0x0032u, 0x0033u, 0x0050u, 0x0051u, 0x0052u,
        0x0070u, 0x0080u, 0x0081u, 0x0082u, 0x0083u, 0x0084u, 0x0085u,
        0x0086u, 0x0090u, 0x0091u, 0x0092u, 0x0093u, 0x0094u, 0x0095u,
        0x0096u, 0x00A0u, 0x00A1u, 0x00B0u, 0x00B1u, 0x00C0u, 0x00C1u,
        0x00C2u, 0x00C3u
    };
    unsigned k;

    CHECK(TBX_DESKSTART == 0x0003u, "D1 TBX_DESKSTART is 0x0003");
    for (k = 0; k < sizeof others / sizeof others[0]; k++)
        CHECK(others[k] != TBX_DESKSTART, "D1 TBX_DESKSTART aliases no other code");

    CHECK(d.state == 0u, "D1 the Toolbox boots dormant");
    CHECK(tbx_desk_take(&d) == 0 && d.state == 0u,
          "D1 an ordinary program's EXEC return starts nothing");
    CHECK(tbx_desk_start(&d, 0, 0) == -1 && d.state == 0u,
          "D1 a stray int 81h from the kernel context arms nothing (BADCODE)");

    /* Round trip 1: FLAIR at the prompt. */
    CHECK(tbx_desk_start(&d, 0, 1) == 0 && d.state == 1u,
          "D1 FLAIR.COM arms the dormant desktop (OK, ARMED)");
    CHECK(tbx_desk_start(&d, 0, 1) == -4 && d.state == 1u,
          "D1 a second FLAIR while armed is refused (BUSY)");
    CHECK(tbx_desk_take(&d) == 1 && d.state == 2u,
          "D1 the EXEC's return starts the armed desktop (ACTIVE)");
    CHECK(tbx_desk_take(&d) == 0 && d.state == 2u,
          "D1 a second EXEC return does not start it twice");
    CHECK(tbx_desk_start(&d, 0, 1) == -4 && d.state == 2u,
          "D1 a second FLAIR while running is refused (BUSY)");
    CHECK(tbx_desk_start(&d, 1, 1) == -4 && d.state == 2u,
          "D1 a disk tenant's TBX_DESKSTART is refused (BUSY)");
    tbx_desk_leave(&d);
    CHECK(d.state == 0u, "D1 Exit to InitechDOS makes the Toolbox dormant");

    /* Round trip 2: FLAIR again after Exit. */
    CHECK(tbx_desk_start(&d, 0, 1) == 0 && d.state == 1u,
          "D1 FLAIR again after Exit arms the desktop again");
    CHECK(tbx_desk_take(&d) == 1 && d.state == 2u,
          "D1 FLAIR again after Exit starts the desktop again");
    tbx_desk_leave(&d);
    CHECK(d.state == 0u, "D1 the second Exit makes it dormant again");
}

/* A synthetic transient: [header | code ... | data ...], header-relative. */
#define SYN_BASE   0x00580000u
#define SYN_CODE   0x0301u          /* odd: the last code byte is a low byte */
#define SYN_DATA   0x0100u

static void d2_checksum_rule(void)
{
    static const uint8_t five[5] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    static const uint8_t wrap[4] = { 0xFF, 0xFF, 0x02, 0x00 };
    uint8_t *img = (uint8_t *)calloc(1u, SYN_CODE + SYN_DATA);
    transient_hdr_t h;
    uint32_t n, i;
    uint16_t sum;
    static const uint32_t probes[] = { 0u, 9u, sizeof(transient_hdr_t),
                                       0x0200u, SYN_CODE - 2u, SYN_CODE - 1u };

    /* Hand-computed: 0x0201 + 0x0403 + 0x0005 = 0x0609. */
    CHECK(transient_checksum(five, 5u) == 0x0609u, "D2 word checksum of 01..05 is 0x0609");
    /* 0xFFFF + 0x0002 = 0x10001 -> 16 bits: 0x0001. */
    CHECK(transient_checksum(wrap, 4u) == 0x0001u, "D2 the sum wraps at 16 bits");
    CHECK(transient_checksum(five, 0u) == 0u, "D2 an empty span sums to 0");

    if (img == NULL) { CHECK(0, "D2 calloc"); return; }
    for (i = 0; i < SYN_CODE + SYN_DATA; i++)
        img[i] = (uint8_t)(i * 7u + 3u);
    memset(&h, 0, sizeof h);
    memcpy(h.magic, "COMMAND3", 8);
    h.entry    = SYN_BASE + 0x40u;
    h.code_end = SYN_BASE + SYN_CODE;
    h.load_end = SYN_BASE + SYN_CODE + SYN_DATA;
    h.bss_end  = h.load_end;
    h.n_imports = TRANSIENT_N_IMPORTS;
    memcpy(img, &h, sizeof h);

    n = transient_sum_len(&h, SYN_BASE);
    CHECK(n == SYN_CODE, "D2 the summed span is header + .text + .rodata");
    sum = transient_checksum(img, n);
    CHECK(!transient_overlaid(img, n, sum), "D2 an untouched transient is intact");

    for (i = 0; i < sizeof probes / sizeof probes[0]; i++) {
        img[probes[i]] ^= 0x5Au;
        CHECK(transient_overlaid(img, n, sum),
              "D2 code byte changed: the transient is overlaid (reload)");
        img[probes[i]] ^= 0x5Au;
    }
    CHECK(!transient_overlaid(img, n, sum), "D2 restored: intact again");

    img[SYN_CODE] ^= 0x5Au;                 /* first .data byte */
    img[SYN_CODE + SYN_DATA - 1u] ^= 0x33u; /* last .data byte  */
    CHECK(!transient_overlaid(img, transient_sum_len(&h, SYN_BASE), sum),
          "D2 data byte changed: the live state is not an overlay");
    free(img);
}

static int count_line(const uint8_t *buf, long len, const char *text)
{
    size_t tl = strlen(text);
    int hits = 0;
    long i;

    for (i = 0; i + (long)tl + 3 <= len; i++)
        if (memcmp(buf + i, text, tl) == 0 && buf[i + (long)tl] == '\r' &&
            buf[i + (long)tl + 1] == '\n' && buf[i + (long)tl + 2] == '$')
            hits++;
    return hits;
}

static void d3_flair_com(const char *path)
{
    FILE *f = fopen(path, "rb");
    uint8_t buf[4096];
    long len;

    if (f == NULL) { CHECK(0, "D3 FLAIR.COM opens"); return; }
    len = (long)fread(buf, 1u, sizeof buf, f);
    fclose(f);
    CHECK(len > 0 && len < 512, "D3 FLAIR.COM is a small flat program (< 512 bytes)");
    CHECK(count_line(buf, len, TBX_DESK_MSG_BUSY) == 1,
          "D3 FLAIR.COM carries TBX_DESK_MSG_BUSY + CR LF $ once");
    CHECK(count_line(buf, len, TBX_DESK_MSG_FAIL) == 1,
          "D3 FLAIR.COM carries TBX_DESK_MSG_FAIL + CR LF $ once");
}

int main(int argc, char **argv)
{
    d1_gate_rule();
    d2_checksum_rule();
    if (argc > 1)
        d3_flair_com(argv[1]);
    else
        CHECK(0, "D3 usage: test_flair_cmd <FLAIR.COM>");
    return TEST_SUMMARY("test_flair_cmd");
}
