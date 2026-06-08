/* psp.c -- InitechDOS Program Segment Prefix (PSP) construction.
 *
 * beads: initech-509.4 ("PSP full 256-byte construction (DEC-05 / App B.2)").
 *
 * Ref:   docs/research/psp-loader-ground-truth.md Sec 2 (THE field-by-field
 *        value map; the "Option B" flat-mode handling of the vestigial SEGMENT
 *        fields: flat-address-in-fake-paragraph-units, linear >> 4);
 *        spec/dos_structs.h lines 91-107 (the LOCKED psp_t, 256 bytes,
 *        _Static_assert + the documented offsets); ADR-0003 DEC-05 (Sec 5.5) +
 *        Appendix B.2. CLAUDE.md Law 1 (cite), Law 2 (oracle), Law 3 (artifact
 *        = C), Rule 2 (fail loud), Rule 8 (specs-as-data), Rule 11
 *        (deterministic -- no timestamps / host paths / nondeterminism),
 *        Rule 12 (ASCII).
 *
 * DESIGN STANCE (ADR-0003 Sec 5.5): vestigial structures are populated IN FULL.
 * The PSP is constructed completely; fields whose backing subsystem is deferred
 * are EXPLICITLY zero-filled with the deferral noted (saved_vectors -> 509.8;
 * the two default FCBs -> 509.9). This is not a stub -- it is the spec-complete
 * 256-byte block a freshly-loaded program sees.
 *
 * This TU compiles BOTH freestanding (-ffreestanding -nostdlib, the kernel) and
 * HOSTED (the factory oracle, test_psp.c). It is free of I/O (no console, no
 * serial, no malloc) so it is a pure, host-unit-testable function.
 */

#include "psp.h"

/* ------------------------------------------------------------------------ *
 * Fail-loud (CLAUDE.md Rule 2). psp_build() never silently no-ops on a bad
 * argument. Hosted: abort() (the oracle observes a non-zero exit). Freestanding:
 * __builtin_trap() raises #UD, which the IDT routes to the panic path (serial
 * register dump + the PC LOAD LETTER banner, idt.h:129-132) and halts -- a loud
 * stop, never a corrupt PSP handed to the loader. psp.c stays dependency-free:
 * it does not pull in panic.c / console.c, keeping it purely host-testable.
 * ------------------------------------------------------------------------ */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__
#include <stdlib.h> /* abort */
#define PSP_FAIL_LOUD() abort()
#else
#define PSP_FAIL_LOUD() __builtin_trap()
#endif

/* Local zero-fill: psp.c is freestanding, so no <string.h> memset is assumed
 * present. A tiny deterministic byte loop is sufficient and reproducible. */
static void psp_zero(uint8_t *p, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        p[i] = 0;
    }
}

/* Store a flat LINEAR address as a "fake paragraph" value (linear >> 4) in a
 * little-endian uint16_t slot. This is the brief's Option B (Sec 2.2 / 2.5 /
 * 2.7): a real DOS program reading the field and computing (value << 4) gets a
 * plausible flat address back. A linear address of 0 stores 0x0000 (the honest
 * null sentinel). psp_t is #pragma pack(1); we write through the typed field. */
static uint16_t flat_to_fake_paragraph(uint32_t linear)
{
    return (uint16_t)((linear >> 4) & 0xFFFFu);
}

uint32_t psp_build(psp_t *psp, const psp_params_t *params)
{
    /* Rule 2: fail loud on NULL -- never write through a null pointer, never
     * silently succeed with no PSP. */
    if (psp == 0 || params == 0) {
        PSP_FAIL_LOUD();
        return 0; /* not reached; keeps the compiler happy in hosted builds */
    }
    /* A non-zero command-tail length with a NULL pointer is a caller bug. */
    if (params->cmd_tail_len != 0 && params->cmd_tail == 0) {
        PSP_FAIL_LOUD();
        return 0; /* not reached */
    }

    /* Belt-and-suspenders: the locked spec asserts sizeof(psp_t)==256, but
     * re-assert here so a future struct edit that slipped the static assert
     * cannot quietly under-fill the block. */
    _Static_assert(sizeof(psp_t) == 256, "psp_t must be 256 bytes (App B.2)");

    /* Start from a fully-zeroed 256-byte block. Every field below is written
     * explicitly; the zero base covers all reserved/deferred regions. */
    psp_zero((uint8_t *)psp, (uint32_t)sizeof(*psp));

    /* --- 00h int20[2]: the INT 20h instruction (legacy termination). -------
     * Bytes CD 20 = `int 0x20`. Populated IN FULL (ADR-0003 Sec 5.5 stance);
     * a program doing a near RET to PSP:0 would execute this. The 0x20 trap
     * gate that makes it physically callable is the LOADER's job (deferred).
     * Ref: psp-loader-ground-truth.md Sec 2.1; dos_structs.h:92. */
    psp->int20[0] = 0xCD;
#ifdef PSP_MUTATE_INT20
    /* Rule-6 mutant: wrong terminator byte (0x21 not 0x20). The int20 oracle
     * MUST go RED. A mutant that passes proves the assertion is decoration. */
    psp->int20[1] = 0x21;
#else
    psp->int20[1] = 0x20;
#endif

    /* --- 02h alloc_end_seg: segment of first byte beyond allocated memory. --
     * Option B: flat allocation-end address in fake paragraph units
     * (linear >> 4). The loader passes the actual ceiling (e.g. 0x70000 ->
     * 0x7000). Ref: Sec 2.2 / Sec 3.2; dos_structs.h:93. */
    psp->alloc_end_seg = flat_to_fake_paragraph(params->alloc_end_linear);

    /* --- 04h reserved_04[6]: the real-DOS offset-0x05 CP/M far-call stub. ---
     * Zero-filled this milestone; the 9A <far-ptr> dispatcher trampoline is
     * deferred (no flat trampoline exists yet). Already zero from psp_zero;
     * left implicit and documented. Ref: Sec 2.3; dos_structs.h:94. */

    /* --- 0Ah saved_vectors[12]: saved INT 22h/23h/24h vectors. -------------
     * DEFERRED to initech-509.8 (INT 22/23/24 handlers + PSP vector save,
     * DEC-10). No kernel handlers for those vectors exist yet, so there is
     * nothing to save; zero-filled. Ref: Sec 2.4; dos_structs.h:95. */

    /* --- 16h parent_psp: parent process's PSP. -----------------------------
     * Option B fake paragraph of the parent PSP linear address, or 0x0000 for
     * "no parent yet" (the brief recommends 0x0000 this milestone: no
     * COMMAND.COM PSP exists). The loader decides via params.
     * Ref: Sec 2.5; dos_structs.h:96. */
    psp->parent_psp = flat_to_fake_paragraph(params->parent_psp_linear);

    /* --- 18h jft[20]: the Job File Table (20 one-byte handle entries). ------
     * Entries 0,1,2 = inherited standard handles as sentinel SFT indices,
     * aligned to int21.h INT21_HANDLE_STDOUT(=1)/STDERR(=2):
     *   jft[0]=0x00 stdin  -> SFT slot 0 (CON read)
     *   jft[1]=0x01 stdout -> SFT slot 1 (CON write)
     *   jft[2]=0x01 stderr -> SFT slot 1 (CON write; shared with stdout)
     * Entries 3..19 = 0xFF (unused/closed -- the real-DOS sentinel; AUX/PRN
     * are deferred per 509.7, so NOT given SFT slots). The real SFT backing is
     * initech-509.3. Ref: Sec 2.6; dos_structs.h:97. */
    psp->jft[0] = 0x00;
    psp->jft[1] = 0x01;
    psp->jft[2] = 0x01;
    for (uint32_t i = 3; i < 20; i++) {
        psp->jft[i] = 0xFF;
    }

    /* --- 2Ch env_seg: environment-block segment. ---------------------------
     * Option B fake paragraph of the environment-block linear address, or
     * 0x0000 when the loader passes 0 (no environment block: a program walking
     * env_seg:0 reads a '\0' first byte = empty environment, a valid DOS
     * interpretation). Ref: Sec 2.7 / Sec 3.2; dos_structs.h:98. */
    psp->env_seg = flat_to_fake_paragraph(params->env_linear);

    /* --- 2Eh reserved_2e[34]: unspecified gap. -----------------------------
     * Zero-filled (already zero). Nothing in this milestone depends on it.
     * Ref: Sec 2.8; dos_structs.h:99. */

    /* --- 50h int21_entry[12]: the CP/M-style INT 21h far-return stub. -------
     * Bytes CD 21 CB = `int 0x21` then `retf`. Populated IN FULL; the rest of
     * the 12 bytes stay zero. No program is expected to CALL PSP:0x50, but the
     * field is complete per the design stance. Ref: Sec 2.9; dos_structs.h:100. */
    psp->int21_entry[0] = 0xCD;
    psp->int21_entry[1] = 0x21;
    psp->int21_entry[2] = 0xCB;

    /* --- 5Ch reserved_5c[16] (FCB #1) / 6Ch reserved_6c[20] (FCB #2). -------
     * DEFERRED to initech-509.9 (full FCB pre-parsing, vestigial DEC-04).
     * Zero-filled: drive byte 0 = default drive, name fields all-zero = no
     * file -- correct for a no-tail launch. Ref: Sec 2.10; dos_structs.h:101-102. */

    /* --- 80h cmd_tail[128]: command-tail length + text; also default DTA. ---
     * Layout: byte 0 = count of text chars (NOT counting the length byte or
     * the trailing CR); bytes 1..count = the text; byte count+1 = 0x0D (CR);
     * remainder zero. No-argument launch: count=0, byte 1 = 0x0D.
     * Clamp the text to PSP_CMD_TAIL_MAX_TEXT so the trailing CR always lands
     * at offset <= 127 (Rule 2: never overflow past offset 0xFF). The clamp is
     * loud -- the dropped-byte count is the function's return value.
     * Ref: Sec 2.11; dos_structs.h:103. */
    uint32_t want = params->cmd_tail_len;
    uint32_t copy = want;
    uint32_t dropped = 0;
    if (copy > PSP_CMD_TAIL_MAX_TEXT) {
        dropped = copy - PSP_CMD_TAIL_MAX_TEXT;
        copy = PSP_CMD_TAIL_MAX_TEXT;
    }
#ifdef PSP_MUTATE_CMDTAIL_LEN
    /* Rule-6 mutant: off-by-one count byte. The tail-length oracle MUST go RED.
     * (The text + CR are still written correctly, so this isolates the count.) */
    psp->cmd_tail[0] = (uint8_t)(copy + 1);
#else
    psp->cmd_tail[0] = (uint8_t)copy; /* count <= 126, fits a byte */
#endif
    for (uint32_t i = 0; i < copy; i++) {
        psp->cmd_tail[1 + i] = (uint8_t)params->cmd_tail[i];
    }
    psp->cmd_tail[1 + copy] = 0x0D; /* CR terminator; offset <= 127 */

    return dropped;
}
