/* DEC-03a.7, initech-jarzu: cold loader validation and refusal paths.
 * ADR-0014 DEC-14-6: desktop builds reach these via generated COLD stubs.
 * No IRQ hooks, transfer, allocator state or live stack moves into COLD.
 * DOS-only/host builds include the identical source directly in loader.c. */
#include "loader_cold.h"
#include "memory_map.h"
#ifdef LOADER_COLD_EXTERNAL
#define MZ_ONLY_PARSER
/* Private COLD helpers have distinct ELF names: the resident relocator keeps
 * its own tiny readers. ADR-0014's absence gate must see no moved body there. */
#define mz_rd16 mz_cold_rd16
#define mz_rd32 mz_cold_rd32
#include "mz.c"
#endif
/* Forward (low-address-first) byte move for the load-module down-shift. The MZ
 * load module is moved from file_at+header_bytes DOWN to file_at (dst < src), so
 * a forward copy can never clobber a not-yet-read source byte (dst[i] is written
 * before src[i+k] for any k>0 is read). Freestanding (loader.c pulls in no libc;
 * Rule 11 deterministic). NOT a general memmove -- it assumes dst <= src, which
 * the down-shift always satisfies. */
static void loader_cold_move_down(uint8_t *dst, const uint8_t *src, uint32_t n)
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
#ifdef LOADER_MUT_HIGH_LOW
    out->image_dst = PROGRAM_IMAGE; /* Rule 6: wrong content dispatch */
#endif
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
        return LOADER_ERR_BAD_FORMAT;   /* nothing to parse -- fail loud (Rule 2) */
    }
    if (load_base < PROGRAM_IMAGE || (load_base & 0xFu) != 0u ||
        arena_ceil > PROGRAM_ARENA_CEIL || arena_ceil <= load_base ||
        (!canonical && file_len > arena_ceil - load_base)) {
        return LOADER_ERR_NOMEM;
    }

    /* 1. Parse + validate the MZ container (DEC-08a.1/.5). The pure prep NEVER
     *    panics: it maps MZ_ERR_FOREIGN to a DISTINCT status the KERNEL call site
     *    panics on (DEC-08a.5), and every other parse error to bad-format. This
     *    keeps the panic at the kernel seam (host-observable here, Law 2). */
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
        loader_cold_move_down(file_at,file_at+img.load_module_off,img.load_module_len);
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
     * already proved image_bytes <= file_len, so this is belt-and-braces, Rule 2). */
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
     *    clobbered fixup (Rule 3: fix the root cause, not a placement-specific
     *    band-aid). The reloc offsets are MODULE-RELATIVE, so they index the module
     *    at load_module_off here exactly as they will at the load base after the
     *    move; mz_apply_relocs adds PROGRAM_IMAGE (the flat load base, DEC-08a.1),
     *    not the staging address, so the dwords are correct for the run location. */
#ifdef LOADER_MUTATE_MZ_NO_RELOC
    /* MUTANT (CLAUDE.md Rule 6; make test-mzload-mutant only): SKIP the flat-32
     * relocation entirely. The load module's absolute references keep their
     * unrelocated (link-time) value, so the relocated-dword assertion in
     * test_mzload.c (dword == original + PROGRAM_IMAGE) MUST go RED. This is the
     * EXACT bug -- a relocatable MZ run without its fixups -- the oracle exists to
     * catch. NEVER define in a real build. */
    (void)0;
#else
    if (img.reloc_count > 0u) {
        int rs = mz_apply_relocs(file_at + img.load_module_off,
                                 img.load_module_len,
#ifdef LOADER_MUT_TSR_EXEC_FIXED_RELOC
                                 (uint32_t)PROGRAM_IMAGE,
#else
                                 load_base,
#endif
                                 file_at + img.reloc_table_off, img.reloc_count);
        if (rs != MZ_OK) {
            return LOADER_ERR_BAD_FORMAT;   /* OOB reloc -- fail loud (Rule 2) */
        }
    }
#endif

    /* 4. Move the (now-relocated) LOAD MODULE down over the header to the flat
     *    load base (PROGRAM_IMAGE == file_at). DEC-08a.2: the MZ header is NEVER
     *    part of the loaded image. dst (file_at) <= src (file_at+load_module_off),
     *    so the forward loader_cold_move_down can never clobber an unread source byte.
     *    A zero header (load_module_off == 0; degenerate) needs no move. */
    if (img.load_module_off != 0u) {
        loader_cold_move_down(file_at, file_at + img.load_module_off,
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
#ifdef LOADER_MUTATE_MZ_ENTRY_EQUAL
    if (img.entry_off > img.load_module_len) {
#else
    if (img.entry_off >= img.load_module_len) {
#endif
        /* The entry must point INTO the loaded module (Rule 2: never JMP past it). */
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
            return;   /* UART not draining -> drop the byte, never hang (Rule 2) */
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
    /* Terminal: never proceed (Rule 2 / DEC-08a.5). */
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}


#endif

#define LOADER_OVL_CHUNK (LOADER_OVL_SCRATCH / 4u)

/* First pass validates every offset; second pass applies the SAME table.
 * The reader contract requires stable file bytes/FAT throughout the call.
 * An inconsistent second pass is an invariant violation, never a normal
 * validation refusal after modifying the caller's memory (Rule 2). */
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
#ifndef LOADER_MUTATE_OVL_NO_FACTOR
            if (mz_apply_relocs(dest, img->load_module_len, reloc_factor,
                                scratch, (uint16_t)n) != MZ_OK) {
                __builtin_trap(); /* source changed after validation */
            }
#endif
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
#ifdef LOADER_MUTATE_OVL_NO_FACTOR
        (void)dest; (void)reloc_factor;
#endif
        done += n;
    }
    return LOADER_OK;
}
