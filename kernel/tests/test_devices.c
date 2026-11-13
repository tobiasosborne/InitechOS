/* test_devices.c -- host unit oracle for the DOS character-device chain
 *                   (devices.c / devices.h).
 *
 * beads: initech-509.7 (device-driver chain -- module/oracle only).
 * Ref:   MS-DOS 3.3 Technical Reference, Chapter 4 "Installable Device
 *        Drivers"; Ralf Brown's Interrupt List, "Device Driver Request Packets";
 *        CLAUDE.md Law 2 (oracle is truth), Rule 1 (RED->GREEN), Rule 6
 *        (mutation-proven), Rule 12 (ASCII).
 *
 * Compiles HOSTED by #including devices.c directly (the same TU trick as
 * test_batch.c / test_ansi.c -- test_devices.c is the ONLY source file passed
 * to gcc; devices.c is NOT compiled separately).  All device handlers are pure
 * I/O-free C functions; the I/O seam is caller-supplied callbacks.
 *
 * MUTATION (Rule 6) -- driven by make test-devices-mutant:
 *   -DDEVICES_MUTATE_NO_DONE_BIT   : DONE bit never set; the DONE assertion
 *                                     for every successful command goes RED.
 *   -DDEVICES_MUTATE_NUL_READ_BYTE : NUL READ returns 1 byte; the NUL EOF
 *                                     assertion goes RED.
 *
 * When compiled with any mutant flag this binary MUST exit non-zero.
 * The clean build MUST exit 0 and print "<n> checks, 0 failures".
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>   /* memset, memcmp, strcmp -- libc OK in host test     */
#include <stdio.h>

#include "devices.h"
#include "test_assert.h"

/* Pull in the real artifact source (same TU trick as test_batch.c). */
#include "devices.c"

TEST_HARNESS();

/* ===========================================================================
 * Stub callbacks
 * =========================================================================*/

/* --- Console stubs -------------------------------------------------------- */

/* Circular buffer for injected CON input bytes. */
#define CON_BUF_CAP  64
static uint8_t  g_con_in_buf[CON_BUF_CAP];
static int      g_con_in_head = 0;
static int      g_con_in_tail = 0;
static int      g_con_in_len  = 0;

/* Output capture for CON. */
static uint8_t  g_con_out_buf[256];
static int      g_con_out_len = 0;

static void con_stubs_reset(void)
{
    memset(g_con_in_buf,  0, sizeof(g_con_in_buf));
    g_con_in_head = 0;
    g_con_in_tail = 0;
    g_con_in_len  = 0;
    memset(g_con_out_buf, 0, sizeof(g_con_out_buf));
    g_con_out_len = 0;
}

/* Inject bytes into the CON input queue. */
static void con_inject(const uint8_t *bytes, int n)
{
    int i;
    for (i = 0; i < n && g_con_in_len < CON_BUF_CAP; i++) {
        g_con_in_buf[g_con_in_tail] = bytes[i];
        g_con_in_tail = (g_con_in_tail + 1) % CON_BUF_CAP;
        g_con_in_len++;
    }
}

static int stub_con_read(uint8_t *buf, int len, void *ctx)
{
    int n = 0;
    (void)ctx;
    while (n < len && g_con_in_len > 0) {
        buf[n++] = g_con_in_buf[g_con_in_head];
        g_con_in_head = (g_con_in_head + 1) % CON_BUF_CAP;
        g_con_in_len--;
    }
    return n;
}

static int stub_con_write(const uint8_t *buf, int len, void *ctx)
{
    int i;
    (void)ctx;
    for (i = 0; i < len && g_con_out_len < (int)sizeof(g_con_out_buf); i++) {
        g_con_out_buf[g_con_out_len++] = buf[i];
    }
    return len;
}

static int stub_con_peek(void *ctx)
{
    (void)ctx;
    if (g_con_in_len <= 0) {
        return -1;
    }
    return (int)g_con_in_buf[g_con_in_head];
}

/* --- PRN stubs ------------------------------------------------------------ */

static uint8_t g_prn_out_buf[256];
static int     g_prn_out_len = 0;

static void prn_stubs_reset(void)
{
    memset(g_prn_out_buf, 0, sizeof(g_prn_out_buf));
    g_prn_out_len = 0;
}

static int stub_prn_write(const uint8_t *buf, int len, void *ctx)
{
    int i;
    (void)ctx;
    for (i = 0; i < len && g_prn_out_len < (int)sizeof(g_prn_out_buf); i++) {
        g_prn_out_buf[g_prn_out_len++] = buf[i];
    }
    return len;
}

/* --- AUX stubs ------------------------------------------------------------ */

#define AUX_BUF_CAP 64
static uint8_t g_aux_in_buf[AUX_BUF_CAP];
static int     g_aux_in_head = 0;
static int     g_aux_in_tail = 0;
static int     g_aux_in_len  = 0;

static uint8_t g_aux_out_buf[256];
static int     g_aux_out_len = 0;

static void aux_stubs_reset(void)
{
    memset(g_aux_in_buf,  0, sizeof(g_aux_in_buf));
    g_aux_in_head = 0;
    g_aux_in_tail = 0;
    g_aux_in_len  = 0;
    memset(g_aux_out_buf, 0, sizeof(g_aux_out_buf));
    g_aux_out_len = 0;
}

static void aux_inject(const uint8_t *bytes, int n)
{
    int i;
    for (i = 0; i < n && g_aux_in_len < AUX_BUF_CAP; i++) {
        g_aux_in_buf[g_aux_in_tail] = bytes[i];
        g_aux_in_tail = (g_aux_in_tail + 1) % AUX_BUF_CAP;
        g_aux_in_len++;
    }
}

static int stub_aux_read(uint8_t *buf, int len, void *ctx)
{
    int n = 0;
    (void)ctx;
    while (n < len && g_aux_in_len > 0) {
        buf[n++] = g_aux_in_buf[g_aux_in_head];
        g_aux_in_head = (g_aux_in_head + 1) % AUX_BUF_CAP;
        g_aux_in_len--;
    }
    return n;
}

static int stub_aux_write(const uint8_t *buf, int len, void *ctx)
{
    int i;
    (void)ctx;
    for (i = 0; i < len && g_aux_out_len < (int)sizeof(g_aux_out_buf); i++) {
        g_aux_out_buf[g_aux_out_len++] = buf[i];
    }
    return len;
}

static int stub_aux_peek(void *ctx)
{
    (void)ctx;
    if (g_aux_in_len <= 0) {
        return -1;
    }
    return (int)g_aux_in_buf[g_aux_in_head];
}

/* --- CLOCK$ stubs --------------------------------------------------------- */

static dev_clock_rec_t g_clk_stored;
static int             g_clk_write_called = 0;

static void clk_stubs_reset(void)
{
    memset(&g_clk_stored, 0, sizeof(g_clk_stored));
    g_clk_write_called = 0;
}

static void clk_set_fixed(uint16_t days, uint8_t hours, uint8_t mins,
                          uint8_t secs, uint8_t hund)
{
    g_clk_stored.days_since_1980 = days;
    g_clk_stored.hours     = hours;
    g_clk_stored.minutes   = mins;
    g_clk_stored.seconds   = secs;
    g_clk_stored.hundredths = hund;
}

static int stub_clk_read(dev_clock_rec_t *rec, void *ctx)
{
    (void)ctx;
    *rec = g_clk_stored;
    return 1;
}

static int stub_clk_write(const dev_clock_rec_t *rec, void *ctx)
{
    (void)ctx;
    g_clk_stored = *rec;
    g_clk_write_called = 1;
    return 1;
}

/* --- Shared IO bundle ----------------------------------------------------- */

static devices_io_t make_io(void)
{
    devices_io_t io;
    memset(&io, 0, sizeof(io));
    io.con_write = stub_con_write;
    io.con_read  = stub_con_read;
    io.con_peek  = stub_con_peek;
    io.con_ctx   = NULL;
    io.prn_write = stub_prn_write;
    io.prn_ctx   = NULL;
    io.aux_write = stub_aux_write;
    io.aux_read  = stub_aux_read;
    io.aux_peek  = stub_aux_peek;
    io.aux_ctx   = NULL;
    io.clk_read  = stub_clk_read;
    io.clk_write = stub_clk_write;
    io.clk_ctx   = NULL;
    return io;
}

/* Helper: build a minimal well-formed request packet for READ/WRITE. */
static device_request_t make_rw_pkt(uint8_t cmd,
                                    uint8_t *buf, uint16_t count)
{
    device_request_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.length  = (uint8_t)sizeof(pkt);
    pkt.command = cmd;
    pkt.data.rw.buffer = buf;
    pkt.data.rw.count  = count;
    return pkt;
}

/* Helper: build a packet with no data fields (status/flush commands). */
static device_request_t make_simple_pkt(uint8_t cmd)
{
    device_request_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.length  = (uint8_t)sizeof(pkt);
    pkt.command = cmd;
    return pkt;
}

/* ===========================================================================
 * Test: chain structure
 * =========================================================================*/
static void test_chain(void)
{
    /* After devices_init() the chain must link exactly 5 devices in order:
     * NUL -> CON -> AUX -> PRN -> CLOCK$ -> NULL.
     *
     * Ref: MS-DOS 3.3 Tech Ref, Ch. 4: DOS searches the device chain from
     * head to tail; the order here matches the classic DOS 3.3 convention. */
    devices_init();
    device_header_t *h = devices_head();

    /* 1. NUL */
    CHECK(h != NULL, "chain: NUL is first (not NULL)");
    CHECK(memcmp(h->name, "NUL     ", DEV_NAME_LEN) == 0,
          "chain: first device name == 'NUL     '");
    CHECK((h->attribute & DEV_ATTR_CHAR_DEV) != 0,
          "chain: NUL has CHAR_DEV bit (bit 15)");
    CHECK((h->attribute & DEV_ATTR_NUL) != 0,
          "chain: NUL has NUL_DEV bit (bit 2)");
    CHECK((h->attribute & DEV_ATTR_STDIN) == 0,
          "chain: NUL does NOT have STDIN bit");
    CHECK((h->attribute & DEV_ATTR_STDOUT) == 0,
          "chain: NUL does NOT have STDOUT bit");
    CHECK((h->attribute & DEV_ATTR_CLOCK) == 0,
          "chain: NUL does NOT have CLOCK bit");
    h = h->next_dev;

    /* 2. CON */
    CHECK(h != NULL, "chain: CON is second");
    CHECK(memcmp(h->name, "CON     ", DEV_NAME_LEN) == 0,
          "chain: second device name == 'CON     '");
    CHECK((h->attribute & DEV_ATTR_CHAR_DEV) != 0,
          "chain: CON has CHAR_DEV bit");
    CHECK((h->attribute & DEV_ATTR_STDIN) != 0,
          "chain: CON has STDIN bit (bit 0)");
    CHECK((h->attribute & DEV_ATTR_STDOUT) != 0,
          "chain: CON has STDOUT bit (bit 1)");
    CHECK((h->attribute & DEV_ATTR_NUL) == 0,
          "chain: CON does NOT have NUL bit");
    CHECK((h->attribute & DEV_ATTR_CLOCK) == 0,
          "chain: CON does NOT have CLOCK bit");
    h = h->next_dev;

    /* 3. AUX */
    CHECK(h != NULL, "chain: AUX is third");
    CHECK(memcmp(h->name, "AUX     ", DEV_NAME_LEN) == 0,
          "chain: third device name == 'AUX     '");
    CHECK((h->attribute & DEV_ATTR_CHAR_DEV) != 0,
          "chain: AUX has CHAR_DEV bit");
    CHECK((h->attribute & DEV_ATTR_STDIN) == 0,
          "chain: AUX does NOT have STDIN bit");
    CHECK((h->attribute & DEV_ATTR_STDOUT) == 0,
          "chain: AUX does NOT have STDOUT bit");
    h = h->next_dev;

    /* 4. PRN */
    CHECK(h != NULL, "chain: PRN is fourth");
    CHECK(memcmp(h->name, "PRN     ", DEV_NAME_LEN) == 0,
          "chain: fourth device name == 'PRN     '");
    CHECK((h->attribute & DEV_ATTR_CHAR_DEV) != 0,
          "chain: PRN has CHAR_DEV bit");
    CHECK((h->attribute & DEV_ATTR_STDIN) == 0,
          "chain: PRN does NOT have STDIN bit");
    CHECK((h->attribute & DEV_ATTR_STDOUT) == 0,
          "chain: PRN does NOT have STDOUT bit");
    h = h->next_dev;

    /* 5. CLOCK$ */
    CHECK(h != NULL, "chain: CLOCK$ is fifth");
    CHECK(memcmp(h->name, "CLOCK$  ", DEV_NAME_LEN) == 0,
          "chain: fifth device name == 'CLOCK$  '");
    CHECK((h->attribute & DEV_ATTR_CHAR_DEV) != 0,
          "chain: CLOCK$ has CHAR_DEV bit");
    CHECK((h->attribute & DEV_ATTR_CLOCK) != 0,
          "chain: CLOCK$ has CLOCK bit (bit 3)");
    CHECK((h->attribute & DEV_ATTR_STDIN) == 0,
          "chain: CLOCK$ does NOT have STDIN bit");
    CHECK((h->attribute & DEV_ATTR_STDOUT) == 0,
          "chain: CLOCK$ does NOT have STDOUT bit");
    h = h->next_dev;

    /* End of chain. */
    CHECK(h == NULL, "chain: CLOCK$.next_dev == NULL (end of chain)");
}

/* ===========================================================================
 * Test: devices_find
 * =========================================================================*/
static void test_find(void)
{
    devices_init();

    /* Exact 8-byte name lookups. */
    device_header_t *d;
    d = devices_find("NUL     ");
    CHECK(d != NULL, "find: 'NUL     ' found");
    CHECK(memcmp(d->name, "NUL     ", DEV_NAME_LEN) == 0,
          "find: returned NUL device");

    d = devices_find("CON     ");
    CHECK(d != NULL, "find: 'CON     ' found");
    CHECK((d->attribute & DEV_ATTR_STDIN) != 0,
          "find: CON has STDIN bit");

    d = devices_find("CLOCK$  ");
    CHECK(d != NULL, "find: 'CLOCK$  ' found");
    CHECK((d->attribute & DEV_ATTR_CLOCK) != 0,
          "find: CLOCK$ has CLOCK bit");

    /* Non-existent device. */
    d = devices_find("FOO     ");
    CHECK(d == NULL, "find: 'FOO     ' returns NULL (not in chain)");
}

/* ===========================================================================
 * Test: NUL device
 * =========================================================================*/
static void test_nul(void)
{
    devices_init();
    devices_io_t io = make_io();
    device_header_t *nul = devices_find("NUL     ");
    CHECK(nul != NULL, "nul: device found");

    uint8_t buf[16];
    memset(buf, 0xAA, sizeof(buf));

    /* NUL READ: must return 0 bytes (EOF) and set DONE.
     * Ref: MS-DOS 3.3 Tech Ref, Ch. 4 + Eggebrecht: NUL is the bit-bucket;
     * reads return EOF immediately.
     * MUTANT DEVICES_MUTATE_NUL_READ_BYTE: count == 1 -> oracle goes RED. */
    device_request_t pkt = make_rw_pkt(DEVCMD_READ, buf, 8u);
    devices_request(nul, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0,
          "nul read: DONE bit set (RED under DEVICES_MUTATE_NO_DONE_BIT)");
    CHECK((pkt.status & DEVST_ERROR) == 0,
          "nul read: ERROR bit NOT set");
    CHECK(pkt.data.rw.count == 0u,
          "nul read: count == 0 (EOF) (RED under DEVICES_MUTATE_NUL_READ_BYTE)");

    /* NUL WRITE: must succeed and report all bytes consumed. */
    memset(buf, 'A', 8);
    pkt = make_rw_pkt(DEVCMD_WRITE, buf, 8u);
    devices_request(nul, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0,
          "nul write: DONE bit set (RED under DEVICES_MUTATE_NO_DONE_BIT)");
    CHECK((pkt.status & DEVST_ERROR) == 0,
          "nul write: ERROR bit NOT set");
    CHECK(pkt.data.rw.count == 8u,
          "nul write: count unchanged (all bytes consumed)");

    /* NUL NDREAD: device reports BUSY (no input). */
    pkt = make_simple_pkt(DEVCMD_NDREAD);
    devices_request(nul, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0,
          "nul ndread: DONE set");
    CHECK((pkt.status & DEVST_BUSY) != 0,
          "nul ndread: BUSY set (no input available from NUL)");

    /* NUL INSTATUS: always BUSY (NUL never has input data). */
    pkt = make_simple_pkt(DEVCMD_INSTATUS);
    devices_request(nul, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0, "nul instatus: DONE set");
    CHECK((pkt.status & DEVST_BUSY) != 0, "nul instatus: BUSY set");

    /* NUL OUTSTATUS: always ready (bit-bucket never blocks). */
    pkt = make_simple_pkt(DEVCMD_OUTSTATUS);
    devices_request(nul, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0, "nul outstatus: DONE set");
    CHECK((pkt.status & DEVST_BUSY) == 0, "nul outstatus: NOT BUSY");
}

/* ===========================================================================
 * Test: CON device
 * =========================================================================*/
static void test_con(void)
{
    devices_init();
    con_stubs_reset();
    devices_io_t io = make_io();
    device_header_t *con = devices_find("CON     ");
    CHECK(con != NULL, "con: device found");

    /* CON WRITE: bytes must arrive in the output capture buffer. */
    const uint8_t msg[] = { 'H', 'e', 'l', 'l', 'o' };
    g_con_out_len = 0;
    device_request_t pkt = make_rw_pkt(DEVCMD_WRITE,
                                       (uint8_t *)(uintptr_t)msg,
                                       (uint16_t)sizeof(msg));
    devices_request(con, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0,
          "con write: DONE bit set (RED under DEVICES_MUTATE_NO_DONE_BIT)");
    CHECK((pkt.status & DEVST_ERROR) == 0,
          "con write: ERROR bit NOT set");
    CHECK(pkt.data.rw.count == (uint16_t)sizeof(msg),
          "con write: count == 5 (all bytes written)");
    CHECK(g_con_out_len == (int)sizeof(msg),
          "con write: output callback received 5 bytes");
    CHECK(memcmp(g_con_out_buf, msg, sizeof(msg)) == 0,
          "con write: output bytes match input");

    /* CON READ: inject bytes then read them back. */
    const uint8_t inject[] = { 'A', 'B', 'C' };
    con_inject(inject, (int)sizeof(inject));
    uint8_t rbuf[8];
    memset(rbuf, 0, sizeof(rbuf));
    pkt = make_rw_pkt(DEVCMD_READ, rbuf, 3u);
    devices_request(con, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0,
          "con read: DONE bit set");
    CHECK((pkt.status & DEVST_ERROR) == 0,
          "con read: ERROR bit NOT set");
    CHECK(pkt.data.rw.count == 3u,
          "con read: count == 3 (all injected bytes read)");
    CHECK(memcmp(rbuf, inject, sizeof(inject)) == 0,
          "con read: bytes match injected data");

    /* CON NDREAD: no data -> BUSY; inject one byte -> not BUSY. */
    pkt = make_simple_pkt(DEVCMD_NDREAD);
    devices_request(con, &pkt, &io);
    CHECK((pkt.status & DEVST_BUSY) != 0,
          "con ndread (empty): BUSY set");

    const uint8_t one = 0x42u;
    con_inject(&one, 1);
    pkt = make_simple_pkt(DEVCMD_NDREAD);
    devices_request(con, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0, "con ndread (data avail): DONE set");
    CHECK((pkt.status & DEVST_BUSY) == 0, "con ndread (data avail): NOT BUSY");
    CHECK(pkt.data.ndread.nd_byte == 0x42u,
          "con ndread: peeked byte == 0x42");

    /* The byte must still be in the queue (nondestructive). */
    memset(rbuf, 0, sizeof(rbuf));
    pkt = make_rw_pkt(DEVCMD_READ, rbuf, 1u);
    devices_request(con, &pkt, &io);
    CHECK(pkt.data.rw.count == 1u, "con read after ndread: 1 byte available");
    CHECK(rbuf[0] == 0x42u, "con read after ndread: byte is 0x42");

    /* CON INSTATUS: no data -> BUSY; inject -> not BUSY. */
    pkt = make_simple_pkt(DEVCMD_INSTATUS);
    devices_request(con, &pkt, &io);
    CHECK((pkt.status & DEVST_BUSY) != 0,
          "con instatus (empty): BUSY set");

    const uint8_t two = 0x55u;
    con_inject(&two, 1);
    pkt = make_simple_pkt(DEVCMD_INSTATUS);
    devices_request(con, &pkt, &io);
    CHECK((pkt.status & DEVST_BUSY) == 0,
          "con instatus (data avail): NOT BUSY");

    /* CON OUTSTATUS: always ready (synchronous write). */
    pkt = make_simple_pkt(DEVCMD_OUTSTATUS);
    devices_request(con, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0, "con outstatus: DONE set");
    CHECK((pkt.status & DEVST_BUSY) == 0, "con outstatus: NOT BUSY");

    /* CON with NULL write callback -> ERROR. */
    devices_io_t io_nowrt = io;
    io_nowrt.con_write = NULL;
    g_con_out_len = 0;
    pkt = make_rw_pkt(DEVCMD_WRITE, (uint8_t *)(uintptr_t)msg,
                      (uint16_t)sizeof(msg));
    devices_request(con, &pkt, &io_nowrt);
    CHECK((pkt.status & DEVST_ERROR) != 0,
          "con write (no callback): ERROR set");
}

/* ===========================================================================
 * Test: CLOCK$ device
 * =========================================================================*/
static void test_clock(void)
{
    devices_init();
    clk_stubs_reset();
    devices_io_t io = make_io();
    device_header_t *clk = devices_find("CLOCK$  ");
    CHECK(clk != NULL, "clock: device found");

    /* CLOCK$ READ: inject a known date/time and read it back.
     * Ref: Ralf Brown's IL + MS-DOS 3.3 Tech Ref Ch. 4: the 6-byte record
     * layout is: WORD days-since-1980, BYTE minutes, BYTE hours,
     * BYTE hundredths, BYTE seconds. */
    clk_set_fixed(/*days=*/16586u,  /* 1 Jan 2025 relative to 1 Jan 1980:
                                      * 2025-1980 = 45 years; approximate. */
                  /*hours=*/9u, /*mins=*/30u, /*secs=*/15u, /*hund=*/50u);

    uint8_t rbuf[8];
    memset(rbuf, 0xFF, sizeof(rbuf));
    device_request_t pkt = make_rw_pkt(DEVCMD_READ, rbuf,
                                       (uint16_t)DEVCLK_RECORD_LEN);
    devices_request(clk, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0,
          "clock read: DONE bit set (RED under DEVICES_MUTATE_NO_DONE_BIT)");
    CHECK((pkt.status & DEVST_ERROR) == 0,
          "clock read: ERROR NOT set");
    CHECK(pkt.data.rw.count == (uint16_t)DEVCLK_RECORD_LEN,
          "clock read: count == 6 (DEVCLK_RECORD_LEN)");

    /* Verify the returned record bytes.
     * Layout: bytes 0-1 = days (little-endian), byte 2 = minutes, byte 3 = hours,
     *         byte 4 = hundredths, byte 5 = seconds. */
    uint16_t days_got;
    memcpy(&days_got, rbuf + 0, sizeof(days_got));
    CHECK(days_got == 16586u, "clock read: days_since_1980 == 16586");
    CHECK(rbuf[2] == 30u,    "clock read: minutes == 30");
    CHECK(rbuf[3] == 9u,     "clock read: hours == 9");
    CHECK(rbuf[4] == 50u,    "clock read: hundredths == 50");
    CHECK(rbuf[5] == 15u,    "clock read: seconds == 15");

    /* CLOCK$ WRITE: send a new date/time record and verify it was accepted. */
    uint8_t wbuf[6];
    uint16_t new_days = 16587u;
    memcpy(wbuf + 0, &new_days, sizeof(new_days));
    wbuf[2] = 45u;   /* minutes */
    wbuf[3] = 14u;   /* hours   */
    wbuf[4] = 0u;    /* hundredths */
    wbuf[5] = 0u;    /* seconds */
    g_clk_write_called = 0;
    pkt = make_rw_pkt(DEVCMD_WRITE, wbuf, (uint16_t)DEVCLK_RECORD_LEN);
    devices_request(clk, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0, "clock write: DONE set");
    CHECK((pkt.status & DEVST_ERROR) == 0, "clock write: ERROR NOT set");
    CHECK(g_clk_write_called == 1, "clock write: clk_write callback invoked");
    CHECK(g_clk_stored.days_since_1980 == new_days,
          "clock write: days updated to 16587");
    CHECK(g_clk_stored.minutes == 45u, "clock write: minutes updated to 45");
    CHECK(g_clk_stored.hours   == 14u, "clock write: hours updated to 14");

    /* CLOCK$ READ with NULL callback -> ERROR. */
    devices_io_t io_noclk = io;
    io_noclk.clk_read = NULL;
    memset(rbuf, 0, sizeof(rbuf));
    pkt = make_rw_pkt(DEVCMD_READ, rbuf, (uint16_t)DEVCLK_RECORD_LEN);
    devices_request(clk, &pkt, &io_noclk);
    CHECK((pkt.status & DEVST_ERROR) != 0,
          "clock read (no callback): ERROR set");

    /* CLOCK$ READ with count too small -> ERROR. */
    pkt = make_rw_pkt(DEVCMD_READ, rbuf, (uint16_t)(DEVCLK_RECORD_LEN - 1u));
    devices_request(clk, &pkt, &io);
    CHECK((pkt.status & DEVST_ERROR) != 0,
          "clock read (count too small): ERROR set");

    /* CLOCK$ WRITE with count too small -> ERROR. */
    pkt = make_rw_pkt(DEVCMD_WRITE, wbuf, (uint16_t)(DEVCLK_RECORD_LEN - 1u));
    devices_request(clk, &pkt, &io);
    CHECK((pkt.status & DEVST_ERROR) != 0,
          "clock write (count too small): ERROR set");
}

/* ===========================================================================
 * Test: PRN device
 * =========================================================================*/
static void test_prn(void)
{
    devices_init();
    prn_stubs_reset();
    devices_io_t io = make_io();
    device_header_t *prn = devices_find("PRN     ");
    CHECK(prn != NULL, "prn: device found");

    /* PRN WRITE: bytes must arrive in the PRN output buffer. */
    const uint8_t msg[] = { 'P', 'R', 'N', '\r', '\n' };
    device_request_t pkt = make_rw_pkt(DEVCMD_WRITE,
                                       (uint8_t *)(uintptr_t)msg,
                                       (uint16_t)sizeof(msg));
    devices_request(prn, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0,
          "prn write: DONE bit set (RED under DEVICES_MUTATE_NO_DONE_BIT)");
    CHECK((pkt.status & DEVST_ERROR) == 0,
          "prn write: ERROR NOT set");
    CHECK(pkt.data.rw.count == (uint16_t)sizeof(msg),
          "prn write: count == 5");
    CHECK(g_prn_out_len == (int)sizeof(msg),
          "prn write: output callback received 5 bytes");
    CHECK(memcmp(g_prn_out_buf, msg, sizeof(msg)) == 0,
          "prn write: output bytes match");

    /* PRN READ: not supported -> ERROR with READ_FAULT code.
     * Ref: "Writing MS-DOS Device Drivers" (Eggebrecht), Ch. 4: PRN is
     * output-only; READ is an error. */
    uint8_t rbuf[4];
    memset(rbuf, 0, sizeof(rbuf));
    pkt = make_rw_pkt(DEVCMD_READ, rbuf, 4u);
    devices_request(prn, &pkt, &io);
    CHECK((pkt.status & DEVST_ERROR) != 0,
          "prn read: ERROR bit set (PRN is output-only)");
    CHECK((pkt.status & DEVST_ERRMASK) == DEVEC_READ_FAULT,
          "prn read: error code == DEVEC_READ_FAULT (0x0B)");

    /* PRN INSTATUS: always BUSY (no input). */
    pkt = make_simple_pkt(DEVCMD_INSTATUS);
    devices_request(prn, &pkt, &io);
    CHECK((pkt.status & DEVST_BUSY) != 0,
          "prn instatus: BUSY set (PRN has no input)");

    /* PRN OUTSTATUS: ready if write callback present. */
    pkt = make_simple_pkt(DEVCMD_OUTSTATUS);
    devices_request(prn, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0, "prn outstatus: DONE set");
    CHECK((pkt.status & DEVST_BUSY) == 0, "prn outstatus: NOT BUSY");
}

/* ===========================================================================
 * Test: AUX device
 * =========================================================================*/
static void test_aux(void)
{
    devices_init();
    aux_stubs_reset();
    devices_io_t io = make_io();
    device_header_t *aux = devices_find("AUX     ");
    CHECK(aux != NULL, "aux: device found");

    /* AUX WRITE: bytes routed to aux_write callback. */
    const uint8_t msg[] = { 0x41u, 0x54u, '\r', '\n' };  /* "AT\r\n" */
    device_request_t pkt = make_rw_pkt(DEVCMD_WRITE,
                                       (uint8_t *)(uintptr_t)msg,
                                       (uint16_t)sizeof(msg));
    devices_request(aux, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0,
          "aux write: DONE bit set (RED under DEVICES_MUTATE_NO_DONE_BIT)");
    CHECK((pkt.status & DEVST_ERROR) == 0,
          "aux write: ERROR NOT set");
    CHECK(g_aux_out_len == (int)sizeof(msg),
          "aux write: output callback received 4 bytes");
    CHECK(memcmp(g_aux_out_buf, msg, sizeof(msg)) == 0,
          "aux write: bytes match");

    /* AUX READ: inject bytes then read them. */
    const uint8_t inject[] = { 0x4Fu, 0x4Bu };  /* "OK" */
    aux_inject(inject, (int)sizeof(inject));
    uint8_t rbuf[4];
    memset(rbuf, 0, sizeof(rbuf));
    pkt = make_rw_pkt(DEVCMD_READ, rbuf, 2u);
    devices_request(aux, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0, "aux read: DONE set");
    CHECK((pkt.status & DEVST_ERROR) == 0, "aux read: ERROR NOT set");
    CHECK(pkt.data.rw.count == 2u, "aux read: 2 bytes read");
    CHECK(memcmp(rbuf, inject, sizeof(inject)) == 0,
          "aux read: bytes match injected data");

    /* AUX OUTSTATUS: ready if aux_write present. */
    pkt = make_simple_pkt(DEVCMD_OUTSTATUS);
    devices_request(aux, &pkt, &io);
    CHECK((pkt.status & DEVST_DONE) != 0, "aux outstatus: DONE set");
    CHECK((pkt.status & DEVST_BUSY) == 0, "aux outstatus: NOT BUSY");
}

/* ===========================================================================
 * Test: unknown command -> ERROR (Rule 2: fail loud)
 * =========================================================================*/
static void test_unknown_command(void)
{
    devices_init();
    devices_io_t io = make_io();

    /* Use an invalid command code (0xFF) on each device. */
    const char *names[] = { "NUL     ", "CON     ", "AUX     ",
                             "PRN     ", "CLOCK$  " };
    int n = (int)(sizeof(names) / sizeof(names[0]));
    int i;
    for (i = 0; i < n; i++) {
        device_header_t *dev = devices_find(names[i]);
        CHECK(dev != NULL, "unknown cmd: device found");
        device_request_t pkt = make_simple_pkt(0xFFu);
        devices_request(dev, &pkt, &io);
        CHECK((pkt.status & DEVST_ERROR) != 0,
              "unknown cmd: ERROR bit set (Rule 2: fail loud)");
        CHECK((pkt.status & DEVST_ERRMASK) == DEVEC_UNKNOWN_CMD,
              "unknown cmd: error code == DEVEC_UNKNOWN_CMD (0x03)");
        CHECK((pkt.status & DEVST_DONE) != 0,
              "unknown cmd: DONE bit still set (driver finished, error reported)");
    }

    /* NULL device -> graceful: pkt.status gets ERROR, no crash. */
    device_request_t pkt = make_simple_pkt(DEVCMD_READ);
    devices_request(NULL, &pkt, &io);
    CHECK((pkt.status & DEVST_ERROR) != 0,
          "null device ptr: ERROR set (no crash)");
}

/* ===========================================================================
 * Test: unsupported command on CLOCK$ sets ERROR
 * =========================================================================*/
static void test_clock_unsupported(void)
{
    devices_init();
    devices_io_t io = make_io();
    device_header_t *clk = devices_find("CLOCK$  ");
    CHECK(clk != NULL, "clock unsupported: device found");

    /* CLOCK$ does not support NDREAD. */
    device_request_t pkt = make_simple_pkt(DEVCMD_NDREAD);
    devices_request(clk, &pkt, &io);
    CHECK((pkt.status & DEVST_ERROR) != 0,
          "clock ndread: ERROR set (unsupported)");
}

/* ===========================================================================
 * Test: INIT command on all devices
 * =========================================================================*/
static void test_init_all(void)
{
    devices_init();
    devices_io_t io = make_io();

    const char *names[] = { "NUL     ", "CON     ", "AUX     ",
                             "PRN     ", "CLOCK$  " };
    int n = (int)(sizeof(names) / sizeof(names[0]));
    int i;
    for (i = 0; i < n; i++) {
        device_header_t *dev = devices_find(names[i]);
        CHECK(dev != NULL, "init: device found");
        device_request_t pkt = make_simple_pkt(DEVCMD_INIT);
        devices_request(dev, &pkt, &io);
        CHECK((pkt.status & DEVST_DONE) != 0,
              "init: DONE set (RED under DEVICES_MUTATE_NO_DONE_BIT)");
        CHECK((pkt.status & DEVST_ERROR) == 0,
              "init: ERROR NOT set");
    }
}

/* ===========================================================================
 * main
 * =========================================================================*/
int main(void)
{
    test_chain();
    test_find();
    test_nul();
    test_con();
    test_clock();
    test_prn();
    test_aux();
    test_unknown_command();
    test_clock_unsupported();
    test_init_all();
    return TEST_SUMMARY("test_devices");
}
