/* mzlink.c -- the InitechMZ (.EXE) container linker (host factory tool).
 *
 * beads: initech-0kiq (dtw.2-emu -- the in-emulator EXEC proof). Wraps a
 *        nasm-assembled FLAT 32-bit blob (the load module) plus a list of known
 *        u32 reloc-site offsets into a valid InitechMZ container the kernel
 *        loader parses+relocates+runs (load_program_from_fat -> mz_is_mz ->
 *        load_program_mz_in_place -> loader_prepare_mz). Factory C (libc OK; the
 *        FACTORY cc, NOT KERNEL_CC -- this never ships in the artifact, Law 3).
 *
 * Ref:   docs/adr/ADR-0003-AMENDMENT-DEC-08a-MZ-EXE-Flat-Loader.md
 *          DEC-08a.1: real MZ header ('MZ'); the load module is flat 32-bit code;
 *            the reloc table is a list of uint32 FLAT offsets into the module.
 *          DEC-08a.5: the InitechMZ tag word (0x4943) at e_res[0] (offset 0x1C)
 *            marks a flat-32 InitechMZ; an UNTAGGED MZ panics in the loader.
 *        os/milton/mz.h (MZ_INITECH_TAG = 0x4943; mz_parse_header field model);
 *        os/milton/mz.c (mz_apply_relocs: add load_base to the u32 dword);
 *        os/milton/test_mzload.c (the EXACT host fixture layout this mirrors --
 *          32-byte header, e_cparhdr=2, a TRAILER reloc table past image_bytes);
 *        spec/memory_map.h (PROGRAM_IMAGE = 0x30100, the flat load base the
 *          LOADER adds at runtime -- NOT a value mzlink writes here).
 *        CLAUDE.md Law 1 (cite), Law 2 (the container drives the runtime oracle),
 *        Rule 2 (fail loud on bad input), Rule 11 (deterministic: byte-for-byte
 *        identical output for identical input -- NO timestamps/paths), Rule 12
 *        (ASCII).
 *
 * LAYOUT (mirrors test_mzload.c's "trailer reloc table" case, which the host
 * oracle already proves loader_prepare_mz parses + relocates correctly):
 *
 *   [0x00 .. 0x20)              32-byte MZ header (e_cparhdr = 2 paragraphs)
 *   [0x20 .. 0x20 + mod_len)    the load module (the nasm flat blob, verbatim)
 *   [0x20 + mod_len .. +rc*4)   the reloc table -- rc little-endian u32 offsets,
 *                                a TRAILER past image_bytes (e_lfarlc points here)
 *
 *   image_bytes = 0x20 + mod_len      (so load_module_len == mod_len; the loader
 *                                      drops the 0x20 header, leaving the module)
 *   header_bytes = 0x20  (e_cparhdr = 2)   entry: cs=ip=0  (entry == load base)
 *
 * THE no-reloc MUTANT (Rule 6): --no-reloc writes e_crlc = 0 and emits NO reloc
 * table. The container is otherwise byte-identical, but the loader applies no
 * fixup, so the org-0 absolute in the module keeps its (wrong) link-time value
 * and the marker is ABSENT at runtime -- proving the EMU gate bites (the runtime
 * sibling of -DLOADER_MUTATE_MZ_NO_RELOC).
 *
 * USAGE:
 *   mzlink <in.bin> <out.exe> [--reloc OFF[,OFF...]] [--no-reloc]
 *                             [--minalloc PARAS] [--entry OFF]
 *     <in.bin>     the flat 32-bit load module (nasm -f bin, org 0).
 *     <out.exe>    the InitechMZ container to write.
 *     --reloc      comma-separated u32 reloc-site offsets WITHIN the module
 *                  (each names a 4-byte dword the loader += PROGRAM_IMAGE).
 *     --no-reloc   force e_crlc=0, emit no reloc table (the mutation variant).
 *     --foreign    write e_res[0]=0 (an UNTAGGED 16-bit DOS .EXE) so the kernel
 *                  panics 'PANIC foreign-mz' and never runs it (DEC-08a.5).
 *     --minalloc   e_minalloc paragraphs (default 0).
 *     --entry      entry offset within the module -> e_ip (e_cs stays 0; default 0).
 *
 * Strictly ASCII (Rule 12). Deterministic (Rule 11): no time(), no getenv(),
 * no argv paths leaked into the output bytes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MZ_INITECH_TAG   0x4943u   /* mz.h: 'I','C' LE -- the flat-32 sentinel */
#define HDR_SIZE         0x20u     /* e_cparhdr = 2 paragraphs (canonical fixture) */
#define MAX_RELOCS       64u

/* Fail loud (Rule 2): print to stderr and exit non-zero. */
static void die(const char *msg)
{
    fprintf(stderr, "mzlink: %s\n", msg);
    exit(1);
}

static void put16(uint8_t *b, uint32_t off, uint16_t v)
{
    b[off + 0] = (uint8_t)(v & 0xFFu);
    b[off + 1] = (uint8_t)(v >> 8);
}

static void put32(uint8_t *b, uint32_t off, uint32_t v)
{
    b[off + 0] = (uint8_t)(v         & 0xFFu);
    b[off + 1] = (uint8_t)((v >>  8) & 0xFFu);
    b[off + 2] = (uint8_t)((v >> 16) & 0xFFu);
    b[off + 3] = (uint8_t)((v >> 24) & 0xFFu);
}

/* Parse a comma-separated list of unsigned decimal/hex offsets into out[].
 * Returns the count; dies on overflow or a malformed token (Rule 2). */
static uint32_t parse_offsets(const char *s, uint32_t *out, uint32_t cap)
{
    uint32_t n = 0;
    const char *p = s;
    while (*p) {
        char *end = NULL;
        unsigned long v = strtoul(p, &end, 0);   /* base 0: 0x.. hex or decimal */
        if (end == p) {
            die("--reloc: malformed offset list");
        }
        if (n >= cap) {
            die("--reloc: too many reloc offsets");
        }
        out[n++] = (uint32_t)v;
        p = end;
        if (*p == ',') {
            p++;
        } else if (*p != '\0') {
            die("--reloc: expected ',' between offsets");
        }
    }
    return n;
}

int main(int argc, char **argv)
{
    const char *in_path  = NULL;
    const char *out_path = NULL;
    uint32_t reloc_off[MAX_RELOCS];
    uint32_t reloc_count = 0;
    int      no_reloc    = 0;
    int      foreign     = 0;
    uint32_t minalloc    = 0;
    uint32_t entry_off   = 0;

    /* ---- Parse argv (Rule 2: fail loud on anything unexpected). ---- */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--reloc") == 0) {
            if (++i >= argc) die("--reloc needs an argument");
            reloc_count = parse_offsets(argv[i], reloc_off, MAX_RELOCS);
        } else if (strcmp(argv[i], "--foreign") == 0) {
            /* Emit an UNTAGGED MZ (e_res[0] == 0) -- a genuine 16-bit DOS .EXE as
             * far as the loader can tell. The kernel MUST panic 'PANIC foreign-mz'
             * on this and NEVER run it (DEC-08a.5 fail-loud honesty gate). Used by
             * the BOOT_MZFOREIGN emu leg to prove the panic fires in-emulator. */
            foreign = 1;
        } else if (strcmp(argv[i], "--no-reloc") == 0) {
            no_reloc = 1;
        } else if (strcmp(argv[i], "--minalloc") == 0) {
            if (++i >= argc) die("--minalloc needs an argument");
            minalloc = (uint32_t)strtoul(argv[i], NULL, 0);
        } else if (strcmp(argv[i], "--entry") == 0) {
            if (++i >= argc) die("--entry needs an argument");
            entry_off = (uint32_t)strtoul(argv[i], NULL, 0);
        } else if (argv[i][0] == '-') {
            die("unknown option");
        } else if (in_path == NULL) {
            in_path = argv[i];
        } else if (out_path == NULL) {
            out_path = argv[i];
        } else {
            die("too many positional arguments");
        }
    }
    if (in_path == NULL || out_path == NULL) {
        die("usage: mzlink <in.bin> <out.exe> [--reloc OFF[,OFF...]] "
            "[--no-reloc] [--foreign] [--minalloc PARAS] [--entry OFF]");
    }

    /* ---- Read the flat load module. ---- */
    FILE *fin = fopen(in_path, "rb");
    if (fin == NULL) die("cannot open input");
    if (fseek(fin, 0, SEEK_END) != 0) die("seek failed");
    long mod_len_l = ftell(fin);
    if (mod_len_l < 0) die("ftell failed");
    if (mod_len_l == 0) die("empty load module");
    if (fseek(fin, 0, SEEK_SET) != 0) die("seek failed");
    uint32_t mod_len = (uint32_t)mod_len_l;

    uint8_t *module = (uint8_t *)malloc(mod_len);
    if (module == NULL) die("out of memory");
    if (fread(module, 1, mod_len, fin) != mod_len) die("short read");
    fclose(fin);

    /* If --no-reloc, drop any reloc table entirely (the mutation variant). */
    if (no_reloc) {
        reloc_count = 0;
    }

    /* ---- Validate the reloc offsets against the module bounds (Rule 2). The
     * loader's mz_apply_relocs also bounds-checks, but failing here gives a clear
     * factory-side error rather than a runtime LOADER_ERR_BAD_RELOC. ---- */
    for (uint32_t i = 0; i < reloc_count; i++) {
        if (reloc_off[i] + 4u > mod_len || reloc_off[i] + 4u < reloc_off[i]) {
            die("--reloc offset + 4 runs past the module");
        }
    }
    if (entry_off > mod_len) {
        die("--entry offset past the module");
    }

    /* ---- Compute the container layout (the trailer-reloc model). ---- */
    uint32_t image_bytes  = HDR_SIZE + mod_len;            /* header + module */
    uint32_t reloc_table  = image_bytes;                   /* trailer past image */
    uint32_t total        = reloc_table + reloc_count * 4u;

    uint8_t *buf = (uint8_t *)calloc(1, total);
    if (buf == NULL) die("out of memory");

    /* ---- Build the MZ header (mirrors test_mzload.c build_hdr). ---- */
    buf[0x00] = 0x4Du; buf[0x01] = 0x5Au;                  /* 'MZ' magic */

    /* image_bytes -> (e_cp pages, e_cblp last-page bytes). Full-page model: if
     * image_bytes is a 512-multiple, e_cblp=0; else the last page is partial. */
    uint16_t e_cp, e_cblp;
    if ((image_bytes % 512u) == 0u) {
        e_cp   = (uint16_t)(image_bytes / 512u);
        e_cblp = 0u;
    } else {
        e_cp   = (uint16_t)(image_bytes / 512u + 1u);
        e_cblp = (uint16_t)(image_bytes % 512u);
    }
    put16(buf, 0x02, e_cblp);                              /* e_cblp */
    put16(buf, 0x04, e_cp);                                /* e_cp   */
    put16(buf, 0x06, (uint16_t)reloc_count);               /* e_crlc */
    put16(buf, 0x08, (uint16_t)(HDR_SIZE / 16u));          /* e_cparhdr = 2 */
    put16(buf, 0x0A, (uint16_t)minalloc);                  /* e_minalloc */
    put16(buf, 0x0C, 0xFFFFu);                             /* e_maxalloc (clamped) */
    put16(buf, 0x0E, 0u);                                  /* e_ss (advisory) */
    put16(buf, 0x10, 0u);                                  /* e_sp (advisory) */
    put16(buf, 0x14, (uint16_t)entry_off);                 /* e_ip -> entry_off */
    put16(buf, 0x16, 0u);                                  /* e_cs (entry == base) */
    put16(buf, 0x18, (uint16_t)reloc_table);               /* e_lfarlc (reloc tbl) */
    /* e_res[0] -- the InitechMZ tag. --foreign writes 0 (an untagged 16-bit DOS
     * .EXE), which the loader rejects fail-loud (DEC-08a.5: 'PANIC foreign-mz'). */
    put16(buf, 0x1C, (uint16_t)(foreign ? 0u : MZ_INITECH_TAG));

    /* ---- Place the load module after the header. ---- */
    memcpy(buf + HDR_SIZE, module, mod_len);

    /* ---- Emit the trailer reloc table (rc LE u32 module-relative offsets). ---- */
    for (uint32_t i = 0; i < reloc_count; i++) {
        put32(buf, reloc_table + i * 4u, reloc_off[i]);
    }

    /* ---- Write the container. ---- */
    FILE *fout = fopen(out_path, "wb");
    if (fout == NULL) die("cannot open output");
    if (fwrite(buf, 1, total, fout) != total) die("short write");
    if (fclose(fout) != 0) die("close failed");

    /* Diagnostic to stderr (NOT into the output bytes -- determinism, Rule 11). */
    fprintf(stderr,
            "mzlink: wrote %s (%u bytes): hdr=0x%X mod_len=%u relocs=%u%s "
            "minalloc=%u entry_off=%u tag=0x%04X\n",
            out_path, total, HDR_SIZE, mod_len, reloc_count,
            no_reloc ? " (NO-RELOC mutant)" : (foreign ? " (FOREIGN/untagged)" : ""),
            minalloc, entry_off,
            (unsigned)(foreign ? 0u : MZ_INITECH_TAG));

    free(buf);
    free(module);
    return 0;
}
