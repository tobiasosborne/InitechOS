/* devices.c -- InitechDOS heritage DOS character-device chain: resident
 *              character devices CON, PRN, AUX, CLOCK$, NUL.
 *
 * beads: initech-509.7 (device-driver chain -- module/oracle only; int21 /
 *        sysinit wiring is a separate follow-up bead).
 *
 * Ref (Law 1):
 *   - Microsoft MS-DOS 3.3 Technical Reference, Chapter 4 "Installable Device
 *     Drivers": device-header attribute word (Table 4-1), request-packet
 *     command codes (Table 4-2), per-command data field layouts.
 *   - Ralf Brown's Interrupt List, "Device Driver Request Packets": command
 *     codes 0 (INIT), 4 (READ), 5 (NDREAD), 6 (INSTATUS), 7 (INFLUSH),
 *     8 (WRITE), 9 (WRITEVERIFY), 0x0A (OUTSTATUS), 0x0C (OUTFLUSH);
 *     status word layout (DONE=bit8, BUSY=bit9, ERROR=bit15, code=bits7..0).
 *   - "Writing MS-DOS Device Drivers" (Eggebrecht, 2nd ed.), Ch. 3-4:
 *     NUL device is the bit-bucket; PRN READ is an error; CLOCK$ READ/WRITE
 *     use the 6-byte date/time record; CON attribute bits STDIN|STDOUT.
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> /
 * <stddef.h> only.  No libc.
 *
 * HOST-TESTABILITY SEAM: no asm, no port I/O; all real I/O goes through
 * the devices_io_t callback bundle.  test_devices.c #includes this file
 * directly (the same TU trick as test_batch.c / test_ansi.c).
 *
 * ARCHITECTURE NOTE: Device handlers set pkt->status to ONLY the
 * non-DONE bits (error flags, BUSY, error codes).  The DONE bit is
 * applied by devices_request() AFTER the handler returns.  This
 * centralisation is what makes the DEVICES_MUTATE_NO_DONE_BIT mutant
 * effective: removing the single OR-in in devices_request() causes every
 * DONE assertion in the oracle to go RED simultaneously.  Handlers that
 * need to signal BUSY also leave DONE absent; devices_request() adds DONE
 * unconditionally so the kernel always sees DONE set (poll terminates).
 *
 * MUTATION hooks (CLAUDE.md Rule 6):
 *   DEVICES_MUTATE_NO_DONE_BIT   -- devices_request() omits the DONE bit
 *                                   OR-in; the oracle check for DONE set
 *                                   goes RED on every successful command.
 *   DEVICES_MUTATE_NUL_READ_BYTE -- NUL READ returns count=1 instead of 0;
 *                                   the NUL EOF oracle check goes RED.
 *
 * CLAUDE.md Law 1, Law 2, Law 3, Rule 2, Rule 11, Rule 12.
 */

#include "devices.h"

/* ===========================================================================
 * Freestanding helpers (no libc; same discipline as env.c / batch.c)
 * =========================================================================*/

/* Copy exactly `n` bytes from `src` to `dst`.  Safe for n == 0. */
static void dev_memcpy(void *dst, const void *src, int n)
{
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    int i;
    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

/* Fill `n` bytes starting at `dst` with `val`. */
static void dev_memset(void *dst, uint8_t val, int n)
{
    uint8_t *d = (uint8_t *)dst;
    int i;
    for (i = 0; i < n; i++) {
        d[i] = val;
    }
}

/* Compare two byte arrays of length `n`; return 0 if equal, 1 if not. */
static int dev_memcmp(const void *a, const void *b, int n)
{
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    int i;
    for (i = 0; i < n; i++) {
        if (pa[i] != pb[i]) {
            return 1;
        }
    }
    return 0;
}

/* Write a blank-padded (0x20) device name into `dst` (exactly DEV_NAME_LEN
 * bytes, NOT NUL-terminated).  `src` is a NUL-terminated C string; characters
 * beyond DEV_NAME_LEN are silently truncated; any remaining slots are 0x20.
 *
 * Ref: MS-DOS 3.3 Tech Ref, Ch. 4 device-header: name field is 8 bytes,
 * space-padded, NOT NUL-terminated. */
static void dev_set_name(char dst[DEV_NAME_LEN], const char *src)
{
    int i;
    dev_memset((uint8_t *)dst, 0x20u, (int)DEV_NAME_LEN);
    for (i = 0; i < (int)DEV_NAME_LEN && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
}

/* ===========================================================================
 * Internal helper: mark the request as failed with `code`.
 *
 * Ref: MS-DOS 3.3 Tech Ref, Ch. 4, "Status Word": bit 15 = ERROR; bits 7..0
 * = error code.  We set ERROR | code but NOT DONE here -- devices_request()
 * always OR's in DONE after the handler returns, so the kernel always sees
 * DONE set when the dispatch completes (the poll in the kernel terminates).
 * =========================================================================*/
static void dev_set_error(device_request_t *pkt, uint16_t code)
{
    pkt->status = (uint16_t)(DEVST_ERROR | (code & (uint16_t)DEVST_ERRMASK));
}

/* ===========================================================================
 * NUL device handler
 *
 * Ref: "Writing MS-DOS Device Drivers" (Eggebrecht), Ch. 3: "The NUL device
 * is a bit bucket.  Any write to NUL succeeds immediately with count bytes
 * consumed.  Any read from NUL returns 0 bytes (EOF) immediately."
 * MS-DOS 3.3 Tech Ref Ch. 4: NUL device has attribute bits CHAR_DEV | NUL.
 *
 * Handler sets ONLY non-DONE status bits.  DONE is applied by devices_request.
 * =========================================================================*/
static void dev_nul_handler(device_header_t *dev,
                            device_request_t *pkt,
                            const devices_io_t *io)
{
    (void)dev;
    (void)io;

    switch (pkt->command) {
    case DEVCMD_INIT:
        /* NUL needs no initialisation. */
        pkt->status = 0u;
        break;

    case DEVCMD_READ:
        /* NUL READ: return EOF immediately (0 bytes transferred).
         * MUTANT DEVICES_MUTATE_NUL_READ_BYTE: return 1 instead of 0 so the
         * oracle that checks NUL READ yields 0 bytes goes RED (Rule 6). */
#ifdef DEVICES_MUTATE_NUL_READ_BYTE
        pkt->data.rw.count = 1u;  /* mutant: claim one byte was read (wrong) */
#else
        pkt->data.rw.count = 0u;  /* correct: EOF, zero bytes                */
#endif
        pkt->status = 0u;
        break;

    case DEVCMD_WRITE:
    case DEVCMD_WRITEVERIFY:
        /* NUL WRITE: discard all bytes, report all consumed. */
        /* count stays as-is (all bytes "written" to the bit-bucket). */
        pkt->status = 0u;
        break;

    case DEVCMD_NDREAD:
        /* NUL NDREAD: no data ever available -- set BUSY.
         * Ref: MS-DOS 3.3 Tech Ref: if no input data, driver sets BUSY. */
        pkt->status = DEVST_BUSY;
        break;

    case DEVCMD_INSTATUS:
        /* NUL always appears busy for input (no data ever arrives).
         * Ref: MS-DOS 3.3 Tech Ref, "Input Status": BUSY = not ready. */
        pkt->status = DEVST_BUSY;
        break;

    case DEVCMD_INFLUSH:
        /* Nothing to flush for NUL. */
        pkt->status = 0u;
        break;

    case DEVCMD_OUTSTATUS:
        /* NUL is always ready for output. */
        pkt->status = 0u;
        break;

    case DEVCMD_OUTFLUSH:
        /* Nothing to flush. */
        pkt->status = 0u;
        break;

    default:
        /* Unknown command: set ERROR + code (Rule 2: fail loud). */
        dev_set_error(pkt, DEVEC_UNKNOWN_CMD);
        break;
    }
}

/* ===========================================================================
 * CON device handler
 *
 * Ref: MS-DOS 3.3 Tech Ref, Ch. 4: CON is the console device; attribute bits
 * CHAR_DEV | STDIN | STDOUT.  DOS routes AH=01h/08h/0Ah input to the STDIN
 * device and AH=02h/09h output to the STDOUT device (both are CON by default).
 * All real I/O goes through the io->con_* callbacks (host-testability seam).
 *
 * Handler sets ONLY non-DONE status bits.  DONE is applied by devices_request.
 * =========================================================================*/
static void dev_con_handler(device_header_t *dev,
                            device_request_t *pkt,
                            const devices_io_t *io)
{
    (void)dev;

    switch (pkt->command) {
    case DEVCMD_INIT:
        pkt->status = 0u;
        break;

    case DEVCMD_READ:
        /* CON READ: pull bytes from the input callback. */
        if (io == NULL || io->con_read == NULL) {
            dev_set_error(pkt, DEVEC_GENERAL);
            break;
        }
        {
            int got = io->con_read(pkt->data.rw.buffer,
                                   (int)pkt->data.rw.count,
                                   io->con_ctx);
            if (got < 0) {
                dev_set_error(pkt, DEVEC_READ_FAULT);
            } else {
                pkt->data.rw.count = (uint16_t)got;
                pkt->status = 0u;
            }
        }
        break;

    case DEVCMD_WRITE:
    case DEVCMD_WRITEVERIFY:
        /* CON WRITE: push bytes to the output callback. */
        if (io == NULL || io->con_write == NULL) {
            dev_set_error(pkt, DEVEC_GENERAL);
            break;
        }
        {
            int written = io->con_write(pkt->data.rw.buffer,
                                        (int)pkt->data.rw.count,
                                        io->con_ctx);
            if (written < 0) {
                dev_set_error(pkt, DEVEC_GENERAL);
            } else {
                pkt->data.rw.count = (uint16_t)written;
                pkt->status = 0u;
            }
        }
        break;

    case DEVCMD_NDREAD:
        /* CON Nondestructive Read: peek one byte without consuming.
         * Ref: MS-DOS 3.3 Tech Ref: returns peeked byte in the nd_byte
         * field; if no data, sets BUSY. */
        if (io == NULL || io->con_peek == NULL) {
            pkt->status = DEVST_BUSY;
            break;
        }
        {
            int b = io->con_peek(io->con_ctx);
            if (b < 0) {
                /* No data available yet. */
                pkt->status = DEVST_BUSY;
            } else {
                pkt->data.ndread.nd_byte = (uint8_t)b;
                pkt->status = 0u;
            }
        }
        break;

    case DEVCMD_INSTATUS:
        /* Input ready if con_peek returns >= 0. */
        if (io == NULL || io->con_peek == NULL) {
            pkt->status = DEVST_BUSY;
            break;
        }
        {
            int b = io->con_peek(io->con_ctx);
            pkt->status = (b >= 0) ? 0u : (uint16_t)DEVST_BUSY;
        }
        break;

    case DEVCMD_INFLUSH:
        /* Flush: no persistent queue in this model; just report done. */
        pkt->status = 0u;
        break;

    case DEVCMD_OUTSTATUS:
        /* Console output is always ready (synchronous write). */
        pkt->status = 0u;
        break;

    case DEVCMD_OUTFLUSH:
        pkt->status = 0u;
        break;

    default:
        dev_set_error(pkt, DEVEC_UNKNOWN_CMD);
        break;
    }
}

/* ===========================================================================
 * PRN device handler
 *
 * Ref: MS-DOS 3.3 Tech Ref, Ch. 4: PRN (printer) is a character device with
 * only CHAR_DEV in its attribute word (no STDIN/STDOUT/CLOCK/NUL bits).  DOS
 * routes AH=05h print-char calls here.  READ on PRN is an error (printers
 * are output-only in DOS 3.3).
 *
 * Handler sets ONLY non-DONE status bits.  DONE is applied by devices_request.
 * =========================================================================*/
static void dev_prn_handler(device_header_t *dev,
                            device_request_t *pkt,
                            const devices_io_t *io)
{
    (void)dev;

    switch (pkt->command) {
    case DEVCMD_INIT:
        pkt->status = 0u;
        break;

    case DEVCMD_READ:
    case DEVCMD_NDREAD:
        /* PRN READ is not supported: the printer is output-only in DOS 3.3.
         * Ref: "Writing MS-DOS Device Drivers" (Eggebrecht), Ch. 4:
         * "PRN is a write-only device; a read from it is an error." */
        dev_set_error(pkt, DEVEC_READ_FAULT);
        break;

    case DEVCMD_INSTATUS:
    case DEVCMD_INFLUSH:
        /* PRN has no input side; BUSY on input status queries. */
        pkt->status = DEVST_BUSY;
        break;

    case DEVCMD_WRITE:
    case DEVCMD_WRITEVERIFY:
        if (io == NULL || io->prn_write == NULL) {
            dev_set_error(pkt, DEVEC_GENERAL);
            break;
        }
        {
            int written = io->prn_write(pkt->data.rw.buffer,
                                        (int)pkt->data.rw.count,
                                        io->prn_ctx);
            if (written < 0) {
                dev_set_error(pkt, DEVEC_GENERAL);
            } else {
                pkt->data.rw.count = (uint16_t)written;
                pkt->status = 0u;
            }
        }
        break;

    case DEVCMD_OUTSTATUS:
        /* Printer ready if write callback is available. */
        pkt->status = (io != NULL && io->prn_write != NULL)
                      ? 0u
                      : (uint16_t)DEVST_BUSY;
        break;

    case DEVCMD_OUTFLUSH:
        pkt->status = 0u;
        break;

    default:
        dev_set_error(pkt, DEVEC_UNKNOWN_CMD);
        break;
    }
}

/* ===========================================================================
 * AUX device handler
 *
 * Ref: MS-DOS 3.3 Tech Ref, Ch. 4: AUX (first serial port, COM1) is a
 * bidirectional character device.  Attribute: CHAR_DEV only.  Both READ and
 * WRITE are supported via the aux_read / aux_write callbacks.
 *
 * Handler sets ONLY non-DONE status bits.  DONE is applied by devices_request.
 * =========================================================================*/
static void dev_aux_handler(device_header_t *dev,
                            device_request_t *pkt,
                            const devices_io_t *io)
{
    (void)dev;

    switch (pkt->command) {
    case DEVCMD_INIT:
        pkt->status = 0u;
        break;

    case DEVCMD_READ:
        if (io == NULL || io->aux_read == NULL) {
            dev_set_error(pkt, DEVEC_READ_FAULT);
            break;
        }
        {
            int got = io->aux_read(pkt->data.rw.buffer,
                                   (int)pkt->data.rw.count,
                                   io->aux_ctx);
            if (got < 0) {
                dev_set_error(pkt, DEVEC_READ_FAULT);
            } else {
                pkt->data.rw.count = (uint16_t)got;
                pkt->status = 0u;
            }
        }
        break;

    case DEVCMD_WRITE:
    case DEVCMD_WRITEVERIFY:
        if (io == NULL || io->aux_write == NULL) {
            dev_set_error(pkt, DEVEC_GENERAL);
            break;
        }
        {
            int written = io->aux_write(pkt->data.rw.buffer,
                                        (int)pkt->data.rw.count,
                                        io->aux_ctx);
            if (written < 0) {
                dev_set_error(pkt, DEVEC_GENERAL);
            } else {
                pkt->data.rw.count = (uint16_t)written;
                pkt->status = 0u;
            }
        }
        break;

    case DEVCMD_NDREAD:
        if (io == NULL || io->aux_peek == NULL) {
            pkt->status = DEVST_BUSY;
            break;
        }
        {
            int b = io->aux_peek(io->aux_ctx);
            if (b < 0) {
                pkt->status = DEVST_BUSY;
            } else {
                pkt->data.ndread.nd_byte = (uint8_t)b;
                pkt->status = 0u;
            }
        }
        break;

    case DEVCMD_INSTATUS:
        if (io == NULL || io->aux_peek == NULL) {
            pkt->status = DEVST_BUSY;
            break;
        }
        {
            int b = io->aux_peek(io->aux_ctx);
            pkt->status = (b >= 0) ? 0u : (uint16_t)DEVST_BUSY;
        }
        break;

    case DEVCMD_INFLUSH:
        pkt->status = 0u;
        break;

    case DEVCMD_OUTSTATUS:
        pkt->status = (io != NULL && io->aux_write != NULL)
                      ? 0u
                      : (uint16_t)DEVST_BUSY;
        break;

    case DEVCMD_OUTFLUSH:
        pkt->status = 0u;
        break;

    default:
        dev_set_error(pkt, DEVEC_UNKNOWN_CMD);
        break;
    }
}

/* ===========================================================================
 * CLOCK$ device handler
 *
 * Ref: MS-DOS 3.3 Technical Reference, Ch. 4, "CLOCK$ Device":
 *   - Attribute word: CHAR_DEV | CLOCK (bit 3).
 *   - READ (command 4): `count` must be >= DEVCLK_RECORD_LEN (6); the driver
 *     fills the buffer with the 6-byte date/time record and sets count = 6.
 *     Ref: RLBI section on CLOCK$ device: layout is
 *       WORD days-since-1-Jan-1980, BYTE minutes, BYTE hours,
 *       BYTE hundredths, BYTE seconds.
 *   - WRITE (command 8): buffer contains the new 6-byte record; the driver
 *     programs the system clock accordingly.
 *   - Other commands (NDREAD, INSTATUS, etc.) are not meaningful for a clock
 *     device; we return 0 for status/flush commands, and ERROR for the rest.
 *
 * Handler sets ONLY non-DONE status bits.  DONE is applied by devices_request.
 * =========================================================================*/
static void dev_clk_handler(device_header_t *dev,
                            device_request_t *pkt,
                            const devices_io_t *io)
{
    (void)dev;

    switch (pkt->command) {
    case DEVCMD_INIT:
        pkt->status = 0u;
        break;

    case DEVCMD_READ:
        /* CLOCK$ READ: fill the 6-byte record from the clock source callback.
         * Ref: MS-DOS 3.3 Tech Ref, Ch. 4: count is set to 6 on return. */
        if (pkt->data.rw.buffer == NULL ||
            pkt->data.rw.count < (uint16_t)DEVCLK_RECORD_LEN) {
            dev_set_error(pkt, DEVEC_GENERAL);
            break;
        }
        if (io == NULL || io->clk_read == NULL) {
            dev_set_error(pkt, DEVEC_GENERAL);
            break;
        }
        {
            dev_clock_rec_t rec;
            dev_memset(&rec, 0, (int)sizeof(rec));
            if (!io->clk_read(&rec, io->clk_ctx)) {
                dev_set_error(pkt, DEVEC_GENERAL);
                break;
            }
            dev_memcpy(pkt->data.rw.buffer, &rec, (int)DEVCLK_RECORD_LEN);
            pkt->data.rw.count = (uint16_t)DEVCLK_RECORD_LEN;
            pkt->status = 0u;
        }
        break;

    case DEVCMD_WRITE:
    case DEVCMD_WRITEVERIFY:
        /* CLOCK$ WRITE: accept a new date/time record.
         * Ref: MS-DOS 3.3 Tech Ref, Ch. 4: buffer is exactly DEVCLK_RECORD_LEN
         * bytes carrying the new date/time. */
        if (pkt->data.rw.buffer == NULL ||
            pkt->data.rw.count < (uint16_t)DEVCLK_RECORD_LEN) {
            dev_set_error(pkt, DEVEC_GENERAL);
            break;
        }
        if (io == NULL || io->clk_write == NULL) {
            dev_set_error(pkt, DEVEC_GENERAL);
            break;
        }
        {
            dev_clock_rec_t rec;
            dev_memcpy(&rec, pkt->data.rw.buffer, (int)DEVCLK_RECORD_LEN);
            if (!io->clk_write(&rec, io->clk_ctx)) {
                dev_set_error(pkt, DEVEC_GENERAL);
                break;
            }
            pkt->data.rw.count = (uint16_t)DEVCLK_RECORD_LEN;
            pkt->status = 0u;
        }
        break;

    case DEVCMD_OUTSTATUS:
    case DEVCMD_OUTFLUSH:
    case DEVCMD_INSTATUS:
    case DEVCMD_INFLUSH:
        /* CLOCK$ has no meaningful status/flush semantics but DOS may send
         * these; respond with success (0). */
        pkt->status = 0u;
        break;

    case DEVCMD_NDREAD:
        /* CLOCK$ does not support nondestructive read. */
        dev_set_error(pkt, DEVEC_UNKNOWN_CMD);
        break;

    default:
        dev_set_error(pkt, DEVEC_UNKNOWN_CMD);
        break;
    }
}

/* ===========================================================================
 * Static device-header storage for the five resident devices.
 *
 * Chain order (matches DOS 3.3 convention):
 *   NUL -> CON -> AUX -> PRN -> CLOCK$ -> NULL
 *
 * The chain is searched in order on AH=3Dh open-by-name.  NUL first ensures
 * a "NUL" open can never accidentally hit another driver.
 * =========================================================================*/
static device_header_t g_nul_dev;
static device_header_t g_con_dev;
static device_header_t g_aux_dev;
static device_header_t g_prn_dev;
static device_header_t g_clk_dev;

/* ===========================================================================
 * devices_init
 * =========================================================================*/
void devices_init(void)
{
    /* NUL: CHAR_DEV | NUL_DEV (bit 2).
     * Ref: MS-DOS 3.3 Tech Ref, Table 4-1, bit 2 = NUL device. */
    dev_set_name(g_nul_dev.name, "NUL");
    g_nul_dev.attribute = (uint16_t)(DEV_ATTR_CHAR_DEV | DEV_ATTR_NUL);
    g_nul_dev.handler   = dev_nul_handler;
    g_nul_dev.next_dev  = &g_con_dev;

    /* CON: CHAR_DEV | STDIN (bit 0) | STDOUT (bit 1).
     * Ref: MS-DOS 3.3 Tech Ref, Table 4-1, bits 0+1 = current stdin/stdout. */
    dev_set_name(g_con_dev.name, "CON");
    g_con_dev.attribute = (uint16_t)(DEV_ATTR_CHAR_DEV |
                                     DEV_ATTR_STDIN |
                                     DEV_ATTR_STDOUT);
    g_con_dev.handler   = dev_con_handler;
    g_con_dev.next_dev  = &g_aux_dev;

    /* AUX: CHAR_DEV only (no special-role bits).
     * Ref: MS-DOS 3.3 Tech Ref, Ch. 4: AUX is COM1; discovered by name. */
    dev_set_name(g_aux_dev.name, "AUX");
    g_aux_dev.attribute = (uint16_t)DEV_ATTR_CHAR_DEV;
    g_aux_dev.handler   = dev_aux_handler;
    g_aux_dev.next_dev  = &g_prn_dev;

    /* PRN: CHAR_DEV only (no special-role bits).
     * Ref: MS-DOS 3.3 Tech Ref, Ch. 4: PRN is LPT1; discovered by name. */
    dev_set_name(g_prn_dev.name, "PRN");
    g_prn_dev.attribute = (uint16_t)DEV_ATTR_CHAR_DEV;
    g_prn_dev.handler   = dev_prn_handler;
    g_prn_dev.next_dev  = &g_clk_dev;

    /* CLOCK$: CHAR_DEV | CLOCK$ (bit 3).
     * Ref: MS-DOS 3.3 Tech Ref, Table 4-1, bit 3 = CLOCK$ device.
     * Name is "CLOCK$  " (6 chars + 2 spaces to fill 8 bytes). */
    dev_set_name(g_clk_dev.name, "CLOCK$");
    g_clk_dev.attribute = (uint16_t)(DEV_ATTR_CHAR_DEV | DEV_ATTR_CLOCK);
    g_clk_dev.handler   = dev_clk_handler;
    g_clk_dev.next_dev  = NULL;                /* last in chain        */
}

/* ===========================================================================
 * devices_head
 * =========================================================================*/
device_header_t *devices_head(void)
{
    return &g_nul_dev;
}

/* ===========================================================================
 * devices_find
 *
 * Ref: MS-DOS 3.3 Tech Ref, AH=3Dh OPEN: DOS walks the device chain,
 * comparing the 8-byte name field.  Name comparison here is byte-exact
 * (case-sensitive) because: (a) all resident names are upper-case, and
 * (b) the kernel normalises the name to upper-case before calling.
 * =========================================================================*/
device_header_t *devices_find(const char name[DEV_NAME_LEN])
{
    device_header_t *dev = &g_nul_dev;
    while (dev != NULL) {
        if (dev_memcmp(dev->name, name, (int)DEV_NAME_LEN) == 0) {
            return dev;
        }
        dev = dev->next_dev;
    }
    return NULL;
}

/* ===========================================================================
 * devices_request
 *
 * Central dispatch: calls the per-device handler, then unconditionally OR's
 * in the DONE bit so the kernel's poll-for-DONE loop terminates.
 *
 * This is the ONLY place DONE is set.  Handlers set pkt->status to ONLY the
 * non-DONE bits (BUSY, ERROR | code, or 0 for success).
 *
 * MUTANT DEVICES_MUTATE_NO_DONE_BIT: the DONE OR-in is omitted; every oracle
 * assertion for DONE set goes RED simultaneously (Rule 6).  The mutant is a
 * one-branch RUNTIME perturbation that still compiles under -Werror.
 * NEVER in a real build.
 * =========================================================================*/
void devices_request(device_header_t    *dev,
                     device_request_t   *pkt,
                     const devices_io_t *io)
{
    if (dev == NULL || pkt == NULL) {
        /* Null device or null packet: fail loud (Rule 2). */
        if (pkt != NULL) {
            dev_set_error(pkt, DEVEC_GENERAL);
            pkt->status |= (uint16_t)DEVST_DONE;
        }
        return;
    }

    /* Zero the status before dispatch so the handler starts clean. */
    pkt->status = 0u;

    if (dev->handler == NULL) {
        dev_set_error(pkt, DEVEC_UNKNOWN_CMD);
        pkt->status |= (uint16_t)DEVST_DONE;
        return;
    }

    dev->handler(dev, pkt, io);

    /* DONE bit: applied here and ONLY here, after the handler returns.
     * This centralised application is what makes DEVICES_MUTATE_NO_DONE_BIT
     * effective: removing this one OR-in breaks every DONE assertion.
     *
     * MUTANT DEVICES_MUTATE_NO_DONE_BIT: skip this OR-in -- the oracle goes
     * RED on every check for DONE (Rule 6).  NEVER in a real build. */
#ifndef DEVICES_MUTATE_NO_DONE_BIT
    pkt->status |= (uint16_t)DEVST_DONE;
#endif
}
