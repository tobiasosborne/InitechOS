/* DEC-03a.7, initech-jarzu: cold loader validation and refusal paths.
 * ADR-0014 DEC-14-6: desktop builds reach these via generated COLD stubs.
 * No IRQ hooks, transfer, allocator state or live stack moves into COLD.
 * DOS-only/host builds include the identical source directly in loader.c. */
#include "loader_validation.h"
#include "memory_map.h"
/* Pre-existing EXEC validation is cold too (DEC-03a.7; ADR-0014).
 * It finishes before transfer; no allocator state or return frame lives here. */
#define TRANSIENT_MAGIC "COMMAND3"
#define TRANSIENT_MAGIC_LEN 8u
/* loader_prepare_core -- the shared validation + layout + psp_params + arena
 * computation behind BOTH loader_prepare (baked path: image copied later) and
 * loader_prepare_in_place (FAT path: image already at PROGRAM_IMAGE; beads
 * initech-za4m). `in_place` selects whether an image pointer is expected:
 *
 *   in_place == 0 (baked):    `image` MUST be non-NULL; out->image_src = image.
 *                             load_program copies image -> PROGRAM_IMAGE.
 *   in_place == 1 (FAT/za4m):  `image` is the RESIDENT image (looked at, never
 *                              copied; NULL is a caller bug); out->image_src = NULL.
 *                             The bytes are ALREADY at PROGRAM_IMAGE -- no copy.
 *
 * EITHER way the ONLY size bound is PROGRAM_IMAGE_MAX (~188 KiB = ENV_BLOCK -
 * PROGRAM_IMAGE). The old FAT path also rejected image_len > LOAD_STAGING_MAX
 * (64 KiB), because it staged the .COM through [0x80000,0x90000) before copying
 * down; za4m reads straight into PROGRAM_IMAGE, so that 64 KiB cap is GONE and
 * a 77 KiB INITECHBASE.COM now loads (it used to fail LOADER_ERR_TOO_BIG). */
int loader_image_is_transient(const uint8_t *image, uint32_t image_len)
{
    if (image == 0 || image_len < TRANSIENT_MAGIC_LEN) {
        return 0;
    }
    /* ADR-0014 DEC-14-2: C:\FLAIR.OVL (spec/overlay_format.h, magic
     * "FLAIROVL") is a kernel overlay, never a program: DOS EXEC error 11 like
     * the transient. Compared as two little-endian dwords ("FLAI", "ROVL"), so
     * the contiguous byte string stays out of every kernel that does not load
     * the overlay (tools/mk_boot_system.sh keys on it). MUTANT
     * LOADER_MUT_NO_OVERLAY_GUARD: test-loader-mutant only. */
    if (((uint32_t)image[0] | (uint32_t)image[1] << 8 | (uint32_t)image[2] << 16 |
         (uint32_t)image[3] << 24) == 0x49414C46u &&
        ((uint32_t)image[4] | (uint32_t)image[5] << 8 | (uint32_t)image[6] << 16 |
         (uint32_t)image[7] << 24) == 0x4C564F52u) {
        return 1;
    }
    uint32_t m = 0u;
    while (m < TRANSIENT_MAGIC_LEN && image[m] == (uint8_t)TRANSIENT_MAGIC[m]) {
        m++;
    }
    return m == TRANSIENT_MAGIC_LEN;
}

loader_status_t loader_prepare_core(const uint8_t *image,
                                           uint32_t image_len,
                                           const char *cmd_tail,
                                           uint32_t cmd_tail_len, int in_place,
                                           loader_plan_t *out)
{
    /* the repository contract: fail loud on every bad input. The loader NEVER copies a NULL or
     * oversized image into the program region -- that would clobber the kernel
     * stack / framebuffer and silently corrupt the machine. */
    if (out == 0) {
        return LOADER_ERR_NULL_OUT;
    }
    /* The baked path needs source bytes to copy FROM; the in-place path needs
     * the resident bytes to LOOK AT (the content refusal below) -- a NULL in
     * either mode is a caller bug, never a program (initech-13ig). */
    if (image == 0) {
        return LOADER_ERR_NULL_IMAGE;
    }
    if (image_len == 0) {
        return LOADER_ERR_ZERO_LEN;
    }
    if (image_len > PROGRAM_IMAGE_MAX) {
        /* Would overrun the program region into the env/stack (Sec 3.2 gap
         * proof). This is now the SOLE size bound on the FAT path too (za4m:
         * the 64 KiB LOAD_STAGING_MAX cap is gone -- the .COM no longer transits
         * the 64 KiB staging buffer; it is read straight into PROGRAM_IMAGE). */
        return LOADER_ERR_TOO_BIG;
    }

    /* THE TRANSIENT IS NOT A PROGRAM (bead initech-13ig). This is the ONE
     * choke point every flat image passes before loader_run_plan's JMP --
     * baked (load_program), FAT top-level and nested (load_program_from_fat ->
     * load_program_in_place / loader_prepare_in_place) -- so the flagship's
     * COMMAND.COM overlay (os/initechdos/transient.h: a COMMAND3 header linked at
     * TRANSIENT_BASE, not a .COM), under its own name or any copy, is refused
     * here: DOS EXEC error 11 "Invalid format" (IBM DOS 3.30 TR p. 6-200) at
     * the EXEC seam, dBASE error 92 above it. The kernel's EXEC-by-name route
     * to the RESIDENT interpreter lives in kmain.c and never reaches here. */
    if (loader_image_is_transient(image, image_len)) {
        return LOADER_ERR_BAD_FORMAT;
    }

    /* Layout from the LOCKED spec (kernel/include/memory_map.h; Sec 3.2). */
    out->psp_addr  = PROGRAM_BASE;
    out->image_dst = PROGRAM_IMAGE;   /* PSP + 0x100 -- the authentic .COM offset */
    out->entry     = PROGRAM_IMAGE;   /* flat entry EIP == image_dst              */
    out->extended = out->low_mcb_owned = out->reserve_end = 0u;
    out->stack_top = PROGRAM_STACK_TOP;
    out->image_len = image_len;
    /* In-place (za4m): no source to copy FROM -- the bytes are already at
     * out->image_dst (PROGRAM_IMAGE). load_program_in_place skips the copy and
     * MUST see a NULL image_src so a stray copy can never be attempted. */
    out->image_src = in_place ? (const uint8_t *)0 : image;

    /* The psp_build inputs (Sec 2.2 / 2.5 / 2.7 / 2.11; psp.h). The vestigial
     * segment fields are passed as flat LINEAR addresses; psp_build stores each
     * as a fake paragraph (linear >> 4). */
    out->params.alloc_end_linear  = PROGRAM_ALLOC_END;  /* 0x80000 -> seg 0x8000 */
    out->params.env_linear        = ENV_BLOCK;          /* 0x6F000 -> seg 0x6F00 (2og) */
    out->params.parent_psp_linear = 0u;                 /* no parent PSP yet (Sec 2.5) */
    out->params.cmd_tail          = cmd_tail;           /* may be NULL (no args) */
    out->params.cmd_tail_len      = cmd_tail_len;
    /* JFT inheritance (beads initech-bsy.9): the PURE prep is parent-agnostic
     * (it has no live PSP), so default to the CON standard handles (NULL). The
     * kernel loader_run_plan overrides this from g_cur_psp->jft at EXEC time --
     * exactly the pattern loader_decide_env uses for env inheritance. */
    out->params.parent_jft        = 0;

    /* Compute the AH=48h heap-arena window DISJOINT from the loaded program (beads
     * initech-1q4u; ADR-0009 DEC-04). The arena starts paragraph-rounded ABOVE the
     * loaded image + a conservative BSS reserve, and ends at PROGRAM_ARENA_CEIL
     * (== ENV_BLOCK), so a 48h ALLOC can never return memory inside the program
     * image+BSS, the env block, or the 64 KiB stack. See kernel/include/memory_map.h's ARENA
     * DISJOINTNESS INVARIANT for the proof. (Was: the arena overlaid the running
     * program -- the latent corruption INITECHBASE would have hit.) */
    {
        /* roundup_paragraph(PROGRAM_IMAGE + image_len + PROGRAM_BSS_RESERVE). The
         * +0xF then mask-to-16 rounds UP to the next paragraph boundary so the DOS
         * segment math (base_linear >> 4) is exact. */
        uint32_t image_end  = PROGRAM_IMAGE + image_len;
        uint32_t arena_base = (image_end + PROGRAM_BSS_RESERVE + 0xFu) & ~0xFu;
        uint32_t arena_ceil = PROGRAM_ARENA_CEIL;   /* == ENV_BLOCK (exclusive) */

        if (arena_base < arena_ceil &&
            (arena_ceil - arena_base) >= 32u) {     /* >= 2 paragraphs (hdr+data) */
            out->arena_base    = arena_base;
            out->arena_ceil    = arena_ceil;
            out->arena_present = 1u;
        } else {
            /* Image too large to host a heap below the ceiling: no arena (fail
             * loud -- 48h reports insufficient memory, never a corrupting
             * overlap). The image itself already fit (PROGRAM_IMAGE_MAX check
             * above); this only denies the *heap*, not the run. */
            out->arena_base    = 0u;
            out->arena_ceil    = 0u;
            out->arena_present = 0u;
        }
    }

    return LOADER_OK;
}

loader_status_t loader_prepare(const uint8_t *image, uint32_t image_len,
                               const char *cmd_tail, uint32_t cmd_tail_len,
                               loader_plan_t *out)
{
    /* Baked path: image copied to PROGRAM_IMAGE by load_program (in_place = 0). */
    return loader_prepare_core(image, image_len, cmd_tail, cmd_tail_len,
                               /*in_place=*/0, out);
}

loader_status_t loader_prepare_in_place(const uint8_t *resident, uint32_t image_len,
                                        const char *cmd_tail, uint32_t cmd_tail_len,
                                        loader_plan_t *out)
{
    /* FAT path (beads initech-za4m): the .COM is ALREADY resident (at
     * PROGRAM_IMAGE, or a nested child's PSP+100h). Nothing is copied; the
     * bytes are looked at only for the content refusal (initech-13ig); the only
     * size bound is PROGRAM_IMAGE_MAX. */
    return loader_prepare_core(resident, image_len, cmd_tail,
                               cmd_tail_len, /*in_place=*/1, out);
}

/* ------------------------------------------------------------------------ *
 * InitechMZ (.EXE) PROLOGUE -- the pure, host-testable content path (beads
 * initech-wczy / dtw.2; ADR-0003 DEC-08a). Mutates the in-place buffer (memmove
 * the load module over the header + apply flat relocs) and produces the SAME
 * loader_plan_t loader_run_plan consumes -- a prologue on the single transfer
 * path (the repository contract). See loader.h for the full contract.
 * ------------------------------------------------------------------------ */

loader_status_t loader_prepare_mz(uint8_t *file_at, uint32_t file_len,
                                  const char *cmd_tail, uint32_t cmd_tail_len,
                                  loader_plan_t *out)
{
    return loader_prepare_mz_at(file_at, file_len, cmd_tail, cmd_tail_len,
                                 PROGRAM_IMAGE, PROGRAM_ARENA_CEIL, out);
}

/* Forward (low-address-first) byte move for the load-module down-shift. The MZ
 * load module is moved from file_at+header_bytes DOWN to file_at (dst < src), so
 * a forward copy can never clobber a not-yet-read source byte (dst[i] is written
 * before src[i+k] for any k>0 is read). Freestanding (loader.c pulls in no libc;
 * the repository contract deterministic). NOT a general memmove -- it assumes dst <= src, which
 * the down-shift always satisfies. */
static void loader_validation_move_down(uint8_t *dst, const uint8_t *src, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        dst[i] = src[i];
    }
}

/* DEC-03a.2-.4, initech-jarzu. Validate the ENTIRE high footprint before
 * any destination write. Finite maxalloc caps arena bytes; FFFF means all.
 * minalloc pins total arena bytes; minimum_heap is usable, net of metadata. */
loader_status_t loader_high_plan(const mz_image_t *img,
                                       const char *tail, uint32_t tail_len,
                                       loader_plan_t *out)
{
    uint32_t base, available, heap_need;
    if (!out) return LOADER_ERR_NULL_OUT;
    if (!img->load_module_len || img->entry_off >= img->load_module_len)
        return LOADER_ERR_BAD_FORMAT;
    if (img->load_module_len > INITECHMZ_EXT_MODULE_MAX)
        return LOADER_ERR_NOMEM;
    available = EXT_APP_GUARD_BASE - EXT_APP_BASE;
    if (img->reserve_bytes > available || img->load_module_len > available-img->reserve_bytes)
        return LOADER_ERR_NOMEM;
    base = (EXT_APP_BASE + img->load_module_len + img->reserve_bytes + 15u) & ~15u;
    if (base > EXT_APP_GUARD_BASE) return LOADER_ERR_NOMEM;
    available = EXT_APP_GUARD_BASE - base;
    if (img->max_alloc_paras != 0xffffu && available > (uint32_t)img->max_alloc_paras*16u)
        available = (uint32_t)img->max_alloc_paras*16u;
    if (available < INITECHMZ_NATIVE_HEADER_BYTES || (uint32_t)img->min_alloc_paras*16u > available ||
        img->minimum_heap_bytes > available-16u) return LOADER_ERR_NOMEM;
    heap_need = (img->minimum_heap_bytes+15u) & ~15u;
    if (heap_need > available-16u) return LOADER_ERR_NOMEM;
    *out = (loader_plan_t){0};
    out->extended = out->low_mcb_owned = 1u;
    out->psp_addr = PROGRAM_BASE; /* runtime supplies a small low MCB control */
    out->image_dst = EXT_APP_BASE;
    out->entry = out->image_dst + img->entry_off;
    out->image_len = img->load_module_len;
    out->reserve_end = base;
    out->arena_base = base;
    out->arena_ceil = base+available;
    out->arena_present = 1;
    out->stack_top = EXT_APP_STACK_TOP;
    out->params.alloc_end_linear = PROGRAM_BASE+0x1100u;
    out->params.env_linear = PROGRAM_BASE+0x100u;
    out->params.cmd_tail = tail;
    out->params.cmd_tail_len = tail_len;
    return LOADER_OK;
}

loader_status_t loader_prepare_mz_at(uint8_t *file_at, uint32_t file_len,
                                     const char *cmd_tail, uint32_t cmd_tail_len,
                                     uint32_t load_base, uint32_t arena_ceil,
                                     loader_plan_t *out)
{
    mz_image_t img;
    int ps;
    int canonical = load_base == PROGRAM_IMAGE && arena_ceil == PROGRAM_ARENA_CEIL;

    if (out == 0) {
        return LOADER_ERR_NULL_OUT;
    }
    if (file_at == 0 || file_len == 0u) {
        return LOADER_ERR_BAD_FORMAT;   /* nothing to parse -- fail loud (the repository contract) */
    }
    if (load_base < PROGRAM_IMAGE || (load_base & 0xFu) != 0u ||
        arena_ceil > PROGRAM_ARENA_CEIL || arena_ceil <= load_base ||
        (!canonical && file_len > arena_ceil - load_base)) {
        return LOADER_ERR_NOMEM;
    }

    /* 1. Parse + validate the MZ container (DEC-08a.1/.5). The pure prep NEVER
     *    panics: it maps MZ_ERR_FOREIGN to a DISTINCT status the KERNEL call site
     *    panics on (DEC-08a.5), and every other parse error to bad-format. This
     *    keeps the panic at the kernel seam (host-observable here, the repository contract). */
    ps = mz_parse_header(file_at, file_len, &img);
    if (ps == MZ_ERR_FOREIGN) {
        /* DEC-08a.5: a genuine untagged 16-bit DOS MZ. The kernel will PANIC
         * (loader_panic_foreign_mz) rather than relocate-and-misexecute 16-bit
         * code in 32-bit flat mode. NEVER produce a runnable plan. */
        return LOADER_ERR_FOREIGN_MZ;
    }
    if (ps != MZ_OK) {
        return LOADER_ERR_BAD_FORMAT;   /* truncated / bad header / OOB reloc */
    }

    if (img.extended) {
        loader_status_t hs=loader_high_plan(&img,cmd_tail,cmd_tail_len,out);
        if(hs!=LOADER_OK) return hs;
        /* Host/in-memory sibling of the streaming kernel path. */
        for(uint32_t i=0;i<img.reloc_count;i++) {
            uint32_t off=mz_rd32(file_at+img.reloc_table_off+i*4u);
            if(off>img.load_module_len || img.load_module_len-off<4u ||
               mz_rd32(file_at+img.load_module_off+off)>0xffffffffu-out->image_dst)
                return LOADER_ERR_BAD_FORMAT;
        }
        if (mz_apply_relocs(file_at+img.load_module_off,img.load_module_len,
                           out->image_dst,file_at+img.reloc_table_off,img.reloc_count)!=MZ_OK)
            return LOADER_ERR_BAD_FORMAT;
        loader_validation_move_down(file_at,file_at+img.load_module_off,img.load_module_len);
        return LOADER_OK;
    }

    /* 2. Bound-check the LOAD MODULE against the program region (DEC-08a.2). The
     *    module lands at PROGRAM_IMAGE; it must fit below the env/stack exactly
     *    like a flat .COM. (load_module_len, not file_len: the header + reloc
     *    table are dropped, so a file slightly over the cap may still fit.) */
    if (img.load_module_len == 0u || img.load_module_len > PROGRAM_IMAGE_MAX) {
        return LOADER_ERR_BAD_FORMAT;
    }
    /* The load module must lie wholly within the resident file (mz_parse_header
     * already proved image_bytes <= file_len, so this is belt-and-braces, the repository contract). */
    if (img.load_module_off + img.load_module_len < img.load_module_off ||
        img.load_module_off + img.load_module_len > file_len) {
        return LOADER_ERR_BAD_FORMAT;
    }

    /* 3. Apply flat-32 relocations to the module IN ITS ORIGINAL FILE POSITION
     *    (at file_at + load_module_off), reading the reloc table from its original
     *    file offset -- THEN move. Relocating BEFORE the move makes the order
     *    robust to ANY reloc-table placement: a real MZ commonly puts the reloc
     *    table in the header gap (reloc_table_off < load_module_off), which the
     *    down-move's DESTINATION window [0, load_module_len) would overwrite if we
     *    relocated after moving. Applying first -- while header, reloc table, and
     *    module all sit untouched at their file offsets -- can never read a
     *    clobbered fixup (the repository contract: fix the root cause, not a placement-specific
     *    band-aid). The reloc offsets are MODULE-RELATIVE, so they index the module
     *    at load_module_off here exactly as they will at the load base after the
     *    move; mz_apply_relocs adds PROGRAM_IMAGE (the flat load base, DEC-08a.1),
     *    not the staging address, so the dwords are correct for the run location. */
    if (img.reloc_count > 0u) {
        int rs = mz_apply_relocs(file_at + img.load_module_off,
                                 img.load_module_len,
                                 load_base,
                                 file_at + img.reloc_table_off, img.reloc_count);
        if (rs != MZ_OK) {
            return LOADER_ERR_BAD_FORMAT;   /* OOB reloc -- fail loud (the repository contract) */
        }
    }

    /* 4. Move the (now-relocated) LOAD MODULE down over the header to the flat
     *    load base (PROGRAM_IMAGE == file_at). DEC-08a.2: the MZ header is NEVER
     *    part of the loaded image. dst (file_at) <= src (file_at+load_module_off),
     *    so the forward loader_validation_move_down can never clobber an unread source byte.
     *    A zero header (load_module_off == 0; degenerate) needs no move. */
    if (img.load_module_off != 0u) {
        loader_validation_move_down(file_at, file_at + img.load_module_off,
                         img.load_module_len);
    }

    /* 5. Lay out the plan exactly like the in-place .COM path (PSP @ PROGRAM_BASE,
     *    image @ PROGRAM_IMAGE, the heap arena DISJOINT above the load module).
     *    image_len is the LOAD MODULE length, so the arena base sits above the
     *    relocated module, not the whole file (DEC-08a.3). */
    {
        /* The relocated load module is RESIDENT at file_at (== PROGRAM_IMAGE)
         * after step 4, so it is the in-place image the core looks at for the
         * content refusal (initech-13ig); the core still records image_src ==
         * NULL for every in-place plan. A NULL here has been LOADER_ERR_NULL_IMAGE
         * since 7850d4d2 and refused EVERY InitechMZ (initech-zxzi). */
        loader_status_t cs = loader_prepare_core(file_at,
                                                 img.load_module_len, cmd_tail,
                                                 cmd_tail_len, /*in_place=*/1, out);
        if (cs != LOADER_OK) {
            return cs;
        }
    }

    /* 6. Flat MZ entry (DEC-08a.2): entry = PROGRAM_IMAGE + (e_cs*16 + e_ip). The
     *    canonical InitechMZ emits cs=ip=0 (entry == load base, identical to a
     *    .COM), but a non-zero entry_off is honored. ESP stays PROGRAM_STACK_TOP
     *    (loader_prepare_core set it); e_ss:e_sp are advisory this release. */
    if (img.entry_off >= img.load_module_len) {
        /* The entry must point INTO the loaded module (the repository contract: never JMP past it). */
        return LOADER_ERR_BAD_FORMAT;
    }
    out->low_mcb_owned = !canonical;
    out->psp_addr = load_base - 0x100u;
    out->image_dst = load_base;
    out->entry = load_base + img.entry_off;
    /* h4bk: top-level placement above a resident is independent of nested
     * EXEC. Keep the canonical, disjoint stack (DEC-08b) and compute the heap
     * from the actual module base (DEC-08a.1/.3; ADR-0009 DEC-04). */
    if (!canonical) {
        uint32_t base = (load_base + img.load_module_len +
                         PROGRAM_BSS_RESERVE + 15u) & ~15u;
        if (base > arena_ceil) return LOADER_ERR_NOMEM;
        out->arena_base = base;
        out->arena_ceil = arena_ceil;
        out->arena_present = arena_ceil - base >= 32u;
    }

    /* 7. e_minalloc TEETH (DEC-08a.3): the program declares it needs at least
     *    min_alloc_paras paragraphs of heap PAST its image. If the disjoint arena
     *    cannot satisfy that minimum, fail loud (DOS 08h insufficient memory) --
     *    never run a heap-starved program. e_maxalloc (typically 0xFFFF) is
     *    naturally clamped: the arena already ends at PROGRAM_ARENA_CEIL, so a
     *    max-hungry request can never reach past the ceiling. */
    {
        uint32_t need_bytes = (uint32_t)img.min_alloc_paras * 16u;
        uint32_t have_bytes = out->arena_present
                                  ? (out->arena_ceil - out->arena_base)
                                  : 0u;
        /* The trimmed image block leaves one MCB header before the free
         * heap's data. Canonical minalloc accounting is retained verbatim. */
        if (!canonical && have_bytes >= 16u) have_bytes -= 16u;
        if (need_bytes > have_bytes) {
            if (!canonical) return LOADER_ERR_NOMEM;
            return LOADER_ERR_BAD_FORMAT;   /* maps to DOS 08h at the EXEC seam */
        }
    }

    return LOADER_OK;
}


#if !defined(__STDC_HOSTED__) || !__STDC_HOSTED__
#include "io.h"
#include "panic.h"
#define LDR_COM1_THR  0x3F8u
#define LDR_COM1_LSR  0x3FDu
#define LDR_LSR_THRE  0x20u
#define LDR_SERIAL_SPIN_MAX 100000u

static void ldr_serial_putc(char c)
{
    uint32_t spins = 0u;
    while ((inb(LDR_COM1_LSR) & LDR_LSR_THRE) == 0u) {
        if (++spins >= LDR_SERIAL_SPIN_MAX) {
            return;   /* UART not draining -> drop the byte, never hang (the repository contract) */
        }
    }
    outb(LDR_COM1_THR, (uint8_t)c);
}

void ldr_serial_puts(const char *s)
{
    while (*s) {
        ldr_serial_putc(*s++);
    }
}

__attribute__((noreturn))
void loader_panic_foreign_mz(const char *name83)
{
    /* Grep-able marker FIRST (the oracle keys on "PANIC"). */
    ldr_serial_puts("PANIC foreign-mz: untagged 16-bit DOS .EXE -- cannot run in flat mode\n");
    if (name83) {
        ldr_serial_puts("  name=");
        ldr_serial_puts(name83);
        ldr_serial_putc('\n');
    }
    ldr_serial_puts("  DEC-08a.5: refusing to relocate-and-misexecute 16-bit code\n");
    panic_show_reason(PANIC_REASON_FOREIGN_MZ, name83);
    ldr_serial_puts("HALTED\n");
    /* Terminal: never proceed (the repository contract / DEC-08a.5). */
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}


#endif

#define LOADER_OVL_CHUNK (LOADER_OVL_SCRATCH / 4u)

/* First pass validates every offset; second pass applies the SAME table.
 * The reader contract requires stable file bytes/FAT throughout the call.
 * An inconsistent second pass is an invariant violation, never a normal
 * validation refusal after modifying the caller's memory (the repository contract). */
loader_status_t loader_ovl_relocs(loader_ovl_read_fn rd, void *ctx,
                                         const mz_image_t *img, uint8_t *dest,
                                         uint32_t reloc_factor,
                                         uint8_t *scratch, int apply)
{
    uint32_t done = 0u;
    while (done < img->reloc_count) {
        uint32_t n = img->reloc_count - done;
        if (n > LOADER_OVL_CHUNK) n = LOADER_OVL_CHUNK;
        if (rd(ctx, img->reloc_table_off + done * 4u, n * 4u, scratch) != 0) {
            return LOADER_ERR_READ;
        }
        if (apply) {
            if (mz_apply_relocs(dest, img->load_module_len, reloc_factor,
                                scratch, (uint16_t)n) != MZ_OK) {
                __builtin_trap(); /* source changed after validation */
            }
        } else {
            for (uint32_t i = 0u; i < n; i++) {
                uint32_t off = mz_rd32(scratch + i * 4u);
                if (off > img->load_module_len || img->load_module_len - off < 4u)
                    return LOADER_ERR_BAD_FORMAT;
                if(img->extended) {
                    uint8_t word[4];
                    if(rd(ctx,img->load_module_off+off,4,word)!=0) return LOADER_ERR_READ;
                    if(mz_rd32(word)>0xffffffffu-EXT_APP_BASE) return LOADER_ERR_BAD_FORMAT;
                }
            }
        }
        done += n;
    }
    return LOADER_OK;
}
