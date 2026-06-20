/* devices.h -- InitechDOS heritage DOS character-device chain: device-header
 *              model, request-packet protocol, and the five resident character
 *              devices (CON, PRN, AUX, CLOCK$, NUL).
 *
 * beads: initech-509.7 (device-driver chain -- module/oracle only; int21 /
 *        sysinit wiring is a separate follow-up bead).
 *
 * Ref (Law 1):
 *   - Microsoft MS-DOS 3.3 Technical Reference, Chapter 4 "Installable Device
 *     Drivers": device-header layout (next-ptr, attribute word, Strategy entry,
 *     Interrupt entry, 8-char name); the attribute-word bit definitions; the
 *     request-packet header (length, unit, command, status, reserved[8]); the
 *     per-command data fields; the status-word DONE/BUSY/ERROR bits and the
 *     low-nibble error codes.
 *   - Ralf Brown's Interrupt List, "Device Driver Request Packets" section:
 *     command codes 0..0x19 for character devices; the Init (0), Read (4),
 *     Write (8), Nondestructive Read (5), Input Status (6), Input Flush (7),
 *     Output Status (9), Output Flush (0x0C) commands.
 *   - "Writing MS-DOS Device Drivers" (Eggebrecht, 2nd ed.), Ch. 3-4:
 *     attribute-bit meanings, character vs block device, status-word encoding.
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> /
 * <stddef.h> only.  No libc.
 *
 * HOST-TESTABILITY SEAM: all device handlers are pure C functions; I/O is
 * mediated by caller-supplied callbacks in devices_io_t.  test_devices.c
 * #includes devices.c directly (the same TU trick as test_batch.c).
 *
 * MUTATION hooks (CLAUDE.md Rule 6 -- driven by make test-devices-mutant):
 *   DEVICES_MUTATE_NO_DONE_BIT   -- devices_request() does NOT set the DONE
 *                                   bit (bit 8) in the status word after a
 *                                   successful dispatch; the oracle check for
 *                                   DONE set goes RED.  NEVER in a real build.
 *   DEVICES_MUTATE_NUL_READ_BYTE -- NUL's READ handler returns 1 (a byte was
 *                                   read) instead of 0 (EOF); the oracle check
 *                                   for NUL READ returning 0 bytes goes RED.
 *                                   NEVER in a real build.
 *
 * CLAUDE.md Law 1 (cite sources), Law 2 (oracle is truth), Law 3 (artifact C),
 * Rule 2 (fail loud), Rule 11 (deterministic), Rule 12 (ASCII).
 */
#ifndef INITECH_DEVICES_H
#define INITECH_DEVICES_H

#include <stdint.h>
#include <stddef.h>

/* ===========================================================================
 * DEVICE ATTRIBUTE WORD (devices_header_t.attribute)
 *
 * Ref: MS-DOS 3.3 Technical Reference, Chapter 4, Table 4-1 "Device
 *      Attribute Word Definitions":
 *
 *   Bit 15  CHAR_DEV   1 = character device, 0 = block device.
 *           All five resident devices are character devices.
 *   Bit 14  IOCTL_SUP  1 = device supports IOCTL read/write (command 3/12).
 *           Not implemented here; set 0 on all resident devices.
 *   Bit 13  Reserved / OUTPUT_UNTIL_BUSY (char dev only). Set 0.
 *   Bit 12  Reserved. Set 0.
 *   Bit 11  OPEN_CLOSE 1 = device supports Open/Close/RemoveMedia (cmds 13-15).
 *           Not implemented here; set 0 on all resident devices.
 *   Bits 10..4  Reserved. Set 0.
 *   Bit  3  CLOCK$_DEV 1 = this is the CLOCK$ device. Exactly one device sets
 *           this; DOS dispatches INT 21h AH=2Ah/2Bh/2Ch/2Dh through it.
 *   Bit  2  NUL_DEV    1 = this is the NUL device (the DOS null device).
 *           Exactly one device sets this bit.
 *   Bit  1  STDOUT     1 = this is the current standard-output (CON-out) device.
 *           DOS routes INT 21h AH=02h/09h output through the device with this.
 *   Bit  0  STDIN      1 = this is the current standard-input (CON-in) device.
 *           DOS routes INT 21h AH=01h/08h/0Ah input through the device with this.
 *
 * Note: PRN and AUX have NO special attribute bits beyond bit 15 (CHAR_DEV).
 * Their role is discovered by name lookup ("PRN     " / "AUX     ") in the
 * device chain.
 * =========================================================================*/
#define DEV_ATTR_CHAR_DEV   0x8000u  /* bit 15: character device              */
#define DEV_ATTR_IOCTL_SUP  0x4000u  /* bit 14: IOCTL supported               */
#define DEV_ATTR_OPEN_CLOSE 0x0800u  /* bit 11: Open/Close/RemoveMedia        */
#define DEV_ATTR_CLOCK      0x0008u  /* bit  3: CLOCK$ device                 */
#define DEV_ATTR_NUL        0x0004u  /* bit  2: NUL device                    */
#define DEV_ATTR_STDOUT     0x0002u  /* bit  1: current stdout (CON-out)      */
#define DEV_ATTR_STDIN      0x0001u  /* bit  0: current stdin (CON-in)        */

/* ===========================================================================
 * REQUEST-PACKET COMMAND CODES
 *
 * Ref: MS-DOS 3.3 Technical Reference, Chapter 4, Table 4-2 "Request Header
 *      Command Codes" + Ralf Brown's Interrupt List, "Device Driver" section:
 *
 *   0x00  INIT          -- called once at boot to initialise the driver.
 *   0x04  READ          -- read `count` bytes from the device into `buffer`.
 *   0x05  NDREAD        -- Nondestructive Read: peek one byte WITHOUT consuming
 *                          it from the input queue (char devices only).
 *   0x06  INSTATUS      -- Input Status: report whether input is available.
 *   0x07  INFLUSH       -- Input Flush: discard any pending input.
 *   0x08  WRITE         -- write `count` bytes from `buffer` to the device.
 *   0x09  WRITEVERIFY   -- Write with Verify: same as WRITE but DOS verifies the
 *                          write afterwards. Treated as WRITE here.
 *   0x0A  OUTSTATUS     -- Output Status: report whether the device is ready.
 *   0x0C  OUTFLUSH      -- Output Flush: flush any pending output.
 *   0x0D  OPEN          -- device open (requires OPEN_CLOSE attribute bit).
 *   0x0E  CLOSE         -- device close (requires OPEN_CLOSE attribute bit).
 *
 * Commands above 0x0C not implemented in this round; an unknown command code
 * sets the ERROR bit with error code DEVEC_UNKNOWN (Rule 2).
 * =========================================================================*/
#define DEVCMD_INIT          0x00u
#define DEVCMD_READ          0x04u
#define DEVCMD_NDREAD        0x05u
#define DEVCMD_INSTATUS      0x06u
#define DEVCMD_INFLUSH       0x07u
#define DEVCMD_WRITE         0x08u
#define DEVCMD_WRITEVERIFY   0x09u
#define DEVCMD_OUTSTATUS     0x0Au
#define DEVCMD_OUTFLUSH      0x0Cu
#define DEVCMD_OPEN          0x0Du
#define DEVCMD_CLOSE         0x0Eu

/* ===========================================================================
 * STATUS WORD (device_request_t.status)
 *
 * Ref: MS-DOS 3.3 Technical Reference, Chapter 4, "Status Word" section:
 *
 *   Bit 15  ERROR -- 1 if an error occurred; low byte holds the error code.
 *   Bit  9  BUSY  -- 1 if the device is busy (character devices: input not
 *                    ready / output not done).
 *   Bit  8  DONE  -- 1 when the request has been processed (DOS polls this
 *                    bit after Strategy/Interrupt entry).
 *   Bits 3..0  Error code (valid only when ERROR is set):
 *     0x00  Write-protect violation
 *     0x01  Unknown unit
 *     0x02  Drive not ready
 *     0x03  Unknown command
 *     0x04  CRC error
 *     0x05  Bad drive-request structure length
 *     0x06  Seek error
 *     0x07  Unknown media type
 *     0x08  Sector not found
 *     0x09  Printer out of paper
 *     0x0A  Write fault
 *     0x0B  Read fault
 *     0x0C  General failure
 *
 *   InitechOS extension (not in DOS spec):
 *     0x03  Unknown command (we use 0x03 for unrecognised command codes because
 *           "Unknown unit" (0x01) is block-device oriented and "General failure"
 *           (0x0C) is too generic; 0x03 maps naturally to "Unknown command").
 * =========================================================================*/
#define DEVST_ERROR          0x8000u  /* bit 15: error flag                   */
#define DEVST_BUSY           0x0200u  /* bit  9: device busy                  */
#define DEVST_DONE           0x0100u  /* bit  8: request complete             */
#define DEVST_ERRMASK        0x00FFu  /* bits 7..0: error code field          */

/* Error codes (low byte of status when ERROR bit is set). */
#define DEVEC_UNKNOWN_CMD    0x03u    /* unknown / unsupported command        */
#define DEVEC_READ_FAULT     0x0Bu    /* read fault (e.g. PRN READ)           */
#define DEVEC_GENERAL        0x0Cu    /* general failure                      */

/* ===========================================================================
 * CLOCK$ 6-BYTE DATE/TIME RECORD
 *
 * Ref: MS-DOS 3.3 Technical Reference, Chapter 4, "CLOCK$ Device": a READ of
 * CLOCK$ returns exactly 6 bytes and a WRITE of CLOCK$ accepts exactly 6 bytes.
 *
 * Byte layout (little-endian):
 *   Bytes 0-1  Days since 1 January 1980 (uint16_t, little-endian).
 *   Byte  2    Minutes past midnight (0..1439, packed: this is the low byte of
 *              a uint16_t; see below for the standard interpretation).
 *   Actually the MS-DOS CLOCK$ record is laid out as:
 *     Offset  0  WORD: count of days since 1 Jan 1980
 *     Offset  2  BYTE: minutes (0..59)
 *     Offset  3  BYTE: hours (0..23)
 *     Offset  4  BYTE: hundredths of seconds (0..99)
 *     Offset  5  BYTE: seconds (0..59)
 *
 * Ref: Ralf Brown's Interrupt List, INT 21h AH=2Ch + the CLOCK$ device entry
 * (the kernel's AH=2Ah/2Ch/2Bh/2Dh dispatch reads/writes the CLOCK$ device
 * using exactly these 6 bytes when CLOCK$ is the clock source).
 * =========================================================================*/
#define DEVCLK_RECORD_LEN  6u

typedef struct {
    uint16_t days_since_1980;   /* days since 1 Jan 1980 (little-endian)     */
    uint8_t  minutes;           /* minutes (0..59)                           */
    uint8_t  hours;             /* hours (0..23)                             */
    uint8_t  hundredths;        /* hundredths of a second (0..99)            */
    uint8_t  seconds;           /* seconds (0..59)                           */
} dev_clock_rec_t;

/* ===========================================================================
 * CALLBACK INTERFACES
 *
 * All real I/O is mediated through caller-supplied callbacks stored in a
 * devices_io_t bundle.  This is the HOST-TESTABILITY SEAM: during kernel boot,
 * the sysinit wiring supplies real callbacks (console.c write/read, rtc.c
 * rtc_now/rtc_set, port-based AUX/PRN); the host oracle supplies stubs.
 *
 * All callbacks may be NULL (treated as "device not present"; the handler then
 * sets DEVST_ERROR | DEVEC_GENERAL in the status word).
 * =========================================================================*/

/* CON / PRN / AUX output: write `len` bytes from `buf`.  Returns the number
 * of bytes actually written (may be less on error). */
typedef int (*dev_write_fn)(const uint8_t *buf, int len, void *ctx);

/* CON / AUX input: fill `buf` with up to `len` bytes; block until at least
 * one byte is available.  Returns the number of bytes read (>= 1), or 0 on
 * EOF, or -1 on error. */
typedef int (*dev_read_fn)(uint8_t *buf, int len, void *ctx);

/* CON / AUX nondestructive peek: return the next byte without consuming it
 * (> 0 = peeked byte value, -1 = no data available yet). */
typedef int (*dev_peek_fn)(void *ctx);

/* CLOCK$ read: fill *rec with the current date/time.  Returns 1 on success,
 * 0 on failure (caller sets DEVST_ERROR | DEVEC_GENERAL). */
typedef int (*dev_clk_read_fn)(dev_clock_rec_t *rec, void *ctx);

/* CLOCK$ write: accept a new date/time from *rec.  Returns 1 on success,
 * 0 on failure. */
typedef int (*dev_clk_write_fn)(const dev_clock_rec_t *rec, void *ctx);

/* I/O bundle passed to devices_request().  Fields unused by a particular
 * command are ignored; NULL means "not available". */
typedef struct {
    /* CON callbacks + context */
    dev_write_fn  con_write;
    dev_read_fn   con_read;
    dev_peek_fn   con_peek;
    void         *con_ctx;

    /* PRN callbacks + context */
    dev_write_fn  prn_write;
    void         *prn_ctx;

    /* AUX callbacks + context */
    dev_write_fn  aux_write;
    dev_read_fn   aux_read;
    dev_peek_fn   aux_peek;
    void         *aux_ctx;

    /* CLOCK$ callbacks + context */
    dev_clk_read_fn  clk_read;
    dev_clk_write_fn clk_write;
    void            *clk_ctx;
} devices_io_t;

/* ===========================================================================
 * DEVICE REQUEST PACKET
 *
 * Ref: MS-DOS 3.3 Technical Reference, Chapter 4, "Request Header" (Table 4-3)
 * + "Character Device Requests":
 *
 *   Offset  0  BYTE  length   -- total length of this request packet in bytes.
 *   Offset  1  BYTE  unit     -- (block devices only; 0 for char devices).
 *   Offset  2  BYTE  command  -- one of the DEVCMD_* codes above.
 *   Offset  3  WORD  status   -- set by the driver: DEVST_DONE | error bits.
 *   Offset  5  8 BYTEs reserved (DOS internal; driver ignores/preserves).
 *   Offset 13  per-command data...
 *
 * This C struct collapses to a flat layout.  In the real DOS model the request
 * header lives at a far pointer; here it is a plain C struct for host
 * testability (no segment arithmetic needed).
 * =========================================================================*/
#define DEV_REQ_HDR_LEN  13u  /* bytes 0..12: fixed header portion           */

typedef struct {
    /* Fixed header (bytes 0..12). */
    uint8_t   length;       /* total packet length in bytes                  */
    uint8_t   unit;         /* block-device unit (0 for char devices)        */
    uint8_t   command;      /* DEVCMD_* code                                 */
    uint16_t  status;       /* driver fills this: DEVST_DONE | error         */
    uint8_t   reserved[8];  /* DOS internal; driver does not interpret       */

    /* Per-command data (union over the supported commands).
     * Ref: MS-DOS 3.3 Tech Ref, each command's "Data Fields" table. */
    union {
        /* INIT (command 0): bpb_ptr / end_of_driver (unused in this model). */
        struct {
            uint8_t  num_units;   /* char dev: always 0                      */
        } init;

        /* READ (4) / WRITE (8) / WRITEVERIFY (9):
         *   buffer  -- pointer to the data buffer.
         *   count   -- requested byte count on entry; actual bytes on return.
         * Ref: MS-DOS 3.3 Tech Ref, "BIOS Request Header for Read and Write". */
        struct {
            uint8_t  *buffer;     /* data buffer (caller-allocated)          */
            uint16_t  count;      /* in: requested bytes; out: actual bytes  */
        } rw;

        /* NDREAD (5) -- Nondestructive Read: return one peeked byte.
         * Ref: MS-DOS 3.3 Tech Ref, "Non-Destructive Read": on return the
         * driver places the peeked byte in nd_byte and sets DONE.  If no byte
         * is available it sets BUSY (and clears DONE). */
        struct {
            uint8_t  nd_byte;     /* peeked byte (valid when DONE set)       */
        } ndread;

        /* INSTATUS (6) / OUTSTATUS (0xA): no data fields beyond the header.
         * Driver sets BUSY if not ready, clears BUSY if ready; DONE is set.
         * Ref: MS-DOS 3.3 Tech Ref, "Input/Output Status". */

        /* INFLUSH (7) / OUTFLUSH (0xC): no data fields.
         * Ref: MS-DOS 3.3 Tech Ref, "Input/Output Flush". */
    } data;
} device_request_t;

/* ===========================================================================
 * DEVICE HEADER
 *
 * Ref: MS-DOS 3.3 Technical Reference, Chapter 4, "Device Header" (Table 4-1):
 *   Offset  0  DWORD  next_dev    -- far ptr to next driver in chain (or
 *                                    0xFFFFFFFF if last).  In this C model we
 *                                    use a plain pointer; NULL means "last".
 *   Offset  4  WORD   attribute   -- DEV_ATTR_* combination.
 *   Offset  6  WORD   strategy    -- offset of Strategy routine (far call).
 *   Offset  8  WORD   interrupt   -- offset of Interrupt routine (far call).
 *   Offset 10  8 BYTEs name       -- blank-padded (0x20), NOT NUL-terminated,
 *                                    for character devices; for block devices
 *                                    this byte is the unit count.
 *
 * In this C model Strategy and Interrupt are collapsed into a single C
 * function pointer (the entry that handles a fully-formed request_packet).
 * The real DOS distinction (Strategy stores the far pointer, Interrupt does
 * the work) is preserved conceptually but not modelled as two separate
 * entry points since we have no real-mode far-call machinery here.
 * =========================================================================*/
#define DEV_NAME_LEN  8u   /* 8 bytes, blank-padded, NOT NUL-terminated     */

/* Handler function type: receives the request packet and the I/O bundle;
 * fills pkt->status before returning. */
struct device_header;
typedef void (*dev_handler_fn)(struct device_header *dev,
                               device_request_t     *pkt,
                               const devices_io_t   *io);

typedef struct device_header {
    struct device_header *next_dev;           /* next in chain; NULL = last  */
    uint16_t              attribute;          /* DEV_ATTR_* combination       */
    dev_handler_fn        handler;            /* strategy+interrupt collapsed */
    char                  name[DEV_NAME_LEN]; /* 8 bytes, space-padded       */
} device_header_t;

/* ===========================================================================
 * PUBLIC API
 * =========================================================================*/

/* devices_init -- initialise the five resident character devices and wire the
 * chain.  After this call:
 *   devices_head() returns a pointer to the first device (NUL),
 *   and the chain is: NUL -> CON -> AUX -> PRN -> CLOCK$ -> NULL.
 *
 * This chain order matches the classic DOS 3.3 convention: NUL is always
 * searched first (so "NUL" device opens cannot accidentally match another
 * driver), then CON, AUX, PRN, CLOCK$.
 *
 * May be called multiple times (idempotent: resets the chain to its initial
 * state -- useful in unit tests). */
void devices_init(void);

/* devices_head -- return a pointer to the first device in the chain. */
device_header_t *devices_head(void);

/* devices_find -- walk the chain looking for a device whose name (space-
 * padded to 8 bytes) matches `name` (also space-padded to 8 bytes by the
 * caller, or a plain 8-char array).  Returns a pointer to the matching header,
 * or NULL if not found.
 *
 * Ref: MS-DOS 3.3 Tech Ref, AH=3Dh open-by-name: DOS walks the device chain
 * and compares the 8-byte name field (case-insensitive in real DOS; we do a
 * case-sensitive match here because all resident names are upper-case and the
 * kernel normalises before calling). */
device_header_t *devices_find(const char name[DEV_NAME_LEN]);

/* devices_request -- dispatch one request packet to `dev`.
 *
 * Sets pkt->status to DEVST_DONE on success (possibly with additional bits
 * set, e.g. DEVST_BUSY for a non-destructive read with no data).  Sets
 * DEVST_ERROR | DEVEC_UNKNOWN_CMD if the command code is unrecognised.
 *
 * The caller is responsible for filling pkt->command, pkt->data fields, and
 * pkt->length before calling.  pkt->status is written by this function.
 *
 * MUTANT DEVICES_MUTATE_NO_DONE_BIT: DONE bit is NOT set; the oracle that
 * checks DONE after a successful dispatch goes RED. */
void devices_request(device_header_t    *dev,
                     device_request_t   *pkt,
                     const devices_io_t *io);

#endif /* INITECH_DEVICES_H */
