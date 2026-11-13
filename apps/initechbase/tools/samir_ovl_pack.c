/* samir_ovl_pack.c -- split the SAMIR link into SAMIR.EXE's resident module
 * and SAMIR.OVL (host factory tool; InitechBase overlays Phase B, epic
 * initech-5etom; design docs/design/samir-overlays.md).
 *
 * Factory C (libc OK; never ships, Law 3). Deterministic (Rule 11): the
 * outputs are a pure function of the input ELF -- no time, no paths, no
 * environment. Fails loud (Rule 2) on anything it does not understand.
 *
 * INPUT: SAMIR.elf, linked by os/samir/boot/samir.ld at ORG 0 with
 * --emit-relocs (-q) and -ffunction-sections, so every relocation the linker
 * resolved is still listed, per function.
 *
 * OUTPUTS:
 *   --res-bin F   the resident load module: [0, _samir_load_end), with the
 *                 SAMIR.OVL stamp patched into `samir_ovl_stamp`.
 *   --res-rel F   its InitechMZ relocation list: one LE u32 module offset per
 *                 R_386_32 site (ADR-0003 DEC-08a.1: the loader adds the load
 *                 base to each); fed to tools/mzlink --reloc-file.
 *   --ovl F       SAMIR.OVL (spec/samir/ovl_format.h): header, directory, then
 *                 per overlay its module (the bytes of section .ovl_<name>) and
 *                 its slot-relative R_386_32 sites; zero padding to 512.
 *   --overlay .ovl_<name>   (repeated) the overlay sections, in id order.
 *
 * CHECKS (each dies with a reason; the mutant gates prove they bite):
 *   - relocation types: R_386_32 (fixed up), R_386_PC32/PLT32 (base-invariant:
 *     resident and slot move by the same B); nothing else, no absolute or
 *     undefined targets;
 *   - every overlay starts with its slot header {SAMIR_OVL_MAGIC, id, entry}
 *     and the entry dword is a relocation site;
 *   - the slot is exactly the largest overlay, 16-aligned;
 *   - THE CALL-GRAPH RULE (--forbid NAME, repeated): from the code of every
 *     overlay, follow direct calls and function-address references through
 *     the resident; reaching a forbidden function (the overlay manager, or an
 *     interpreter entry that could run another command and swap the slot) is
 *     an error. The first --forbid must exist in the ELF (it is the manager;
 *     a renamed manager must not silently disable the check).
 *
 * The linker already refused overlay-to-overlay and resident-to-overlay
 * references (samir.ld NOCROSSREFS / NOCROSSREFS_TO); this tool proves the
 * third rule, which a linker cannot express.
 *
 * ASCII-clean (Rule 12).
 */
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "samir/ovl_format.h"

static void die(const char *a, const char *b)
{
    fprintf(stderr, "samir_ovl_pack: %s%s%s\n", a, b ? ": " : "", b ? b : "");
    exit(1);
}

static uint8_t *img;          /* the whole ELF file */
static size_t img_len;
static Elf32_Ehdr *eh;
static Elf32_Shdr *sh;
static const char *shstr;
static Elf32_Sym *syms;
static uint32_t nsyms;
static const char *symstr;

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void wr32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static void wr16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

static uint32_t crc32(uint32_t crc, const uint8_t *p, size_t n)
{
    crc = ~crc;
    while (n--) {
        crc ^= *p++;
        for (int i = 0; i < 8; i++) crc = (crc >> 1) ^ (SAMIR_OVL_CRC_POLY & (0u - (crc & 1u)));
    }
    return ~crc;
}

static const char *secname(unsigned i) { return shstr + sh[i].sh_name; }
static int find_sec(const char *name)
{
    for (unsigned i = 0; i < eh->e_shnum; i++)
        if (!strcmp(secname(i), name)) return (int)i;
    return -1;
}
static const Elf32_Sym *find_sym(const char *name)
{
    for (uint32_t i = 0; i < nsyms; i++)
        if (!strcmp(symstr + syms[i].st_name, name)) return &syms[i];
    return NULL;
}
static uint32_t sym_val(const char *name)
{
    const Elf32_Sym *s = find_sym(name);
    if (!s) die("missing link symbol", name);
    return s->st_value;
}

/* ---- Function table for the call-graph rule ---- */
typedef struct { uint32_t lo, hi; unsigned sec; const char *name; } fn_t;
static fn_t *fns;
static unsigned nfns;
static int fn_at(unsigned sec, uint32_t addr)
{
    for (unsigned i = 0; i < nfns; i++)
        if (fns[i].sec == sec && addr >= fns[i].lo && addr < fns[i].hi) return (int)i;
    return -1;
}

typedef struct { unsigned from, to; } edge_t;
static edge_t *edges;
static unsigned nedges, capedges;
static void add_edge(unsigned a, unsigned b)
{
    if (nedges == capedges) {
        capedges = capedges ? capedges * 2u : 4096u;
        edges = realloc(edges, capedges * sizeof *edges);
        if (!edges) die("out of memory", NULL);
    }
    edges[nedges].from = a; edges[nedges].to = b; nedges++;
}

/* A growable u32 list. */
typedef struct { uint32_t *v; unsigned n, cap; } u32list;
static void push(u32list *l, uint32_t x)
{
    if (l->n == l->cap) {
        l->cap = l->cap ? l->cap * 2u : 1024u;
        l->v = realloc(l->v, l->cap * sizeof *l->v);
        if (!l->v) die("out of memory", NULL);
    }
    l->v[l->n++] = x;
}
static int cmp_u32(const void *a, const void *b)
{
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return x < y ? -1 : x > y;
}

static void write_file(const char *path, const uint8_t *p, size_t n)
{
    FILE *f = fopen(path, "wb");
    if (!f || fwrite(p, 1, n, f) != n || fclose(f)) die("cannot write", path);
}

#define MAXOVL SAMIR_OVL_MAX
int main(int argc, char **argv)
{
    const char *in = NULL, *res_bin = NULL, *res_rel = NULL, *ovl_out = NULL;
    const char *ovl_names[MAXOVL];
    const char *forbid[16];
    unsigned novl = 0, nforbid = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--res-bin") && i + 1 < argc) res_bin = argv[++i];
        else if (!strcmp(argv[i], "--res-rel") && i + 1 < argc) res_rel = argv[++i];
        else if (!strcmp(argv[i], "--ovl") && i + 1 < argc) ovl_out = argv[++i];
        else if (!strcmp(argv[i], "--overlay") && i + 1 < argc) {
            if (novl == MAXOVL) die("too many overlays", NULL);
            ovl_names[novl++] = argv[++i];
        } else if (!strcmp(argv[i], "--forbid") && i + 1 < argc) {
            if (nforbid == 16) die("too many --forbid", NULL);
            forbid[nforbid++] = argv[++i];
        } else if (argv[i][0] != '-' && !in) in = argv[i];
        else die("bad argument", argv[i]);
    }
    if (!in || !res_bin || !res_rel || !ovl_out || !novl || !nforbid)
        die("usage: samir_ovl_pack SAMIR.elf --res-bin F --res-rel F --ovl F "
            "--overlay .ovl_x [...] --forbid MANAGER [--forbid NAME ...]", NULL);

    /* ---- Load the ELF ---- */
    FILE *f = fopen(in, "rb");
    if (!f) die("cannot open", in);
    if (fseek(f, 0, SEEK_END)) die("seek", in);
    img_len = (size_t)ftell(f);
    rewind(f);
    img = malloc(img_len);
    if (!img || fread(img, 1, img_len, f) != img_len) die("short read", in);
    fclose(f);
    eh = (Elf32_Ehdr *)img;
    if (img_len < sizeof *eh || memcmp(eh->e_ident, ELFMAG, SELFMAG) ||
        eh->e_ident[EI_CLASS] != ELFCLASS32 || eh->e_machine != EM_386 || eh->e_type != ET_EXEC)
        die("not an i386 ELF32 executable", in);
    if (eh->e_shoff + (size_t)eh->e_shnum * sizeof(Elf32_Shdr) > img_len) die("bad section table", in);
    sh = (Elf32_Shdr *)(img + eh->e_shoff);
    shstr = (const char *)img + sh[eh->e_shstrndx].sh_offset;
    for (unsigned i = 0; i < eh->e_shnum; i++)
        if (sh[i].sh_type == SHT_SYMTAB) {
            syms = (Elf32_Sym *)(img + sh[i].sh_offset);
            nsyms = sh[i].sh_size / sizeof(Elf32_Sym);
            symstr = (const char *)img + sh[sh[i].sh_link].sh_offset;
        }
    if (!syms) die("no symbol table", in);

    uint32_t load_end = sym_val("_samir_load_end");
    uint32_t slot = sym_val("__ovl_slot");
    uint32_t slot_end = sym_val("__ovl_slot_end");
    if (sym_val("__samir_base") != 0u) die("the image must be linked at ORG 0", NULL);
    if (slot != load_end || (slot & 15u) || slot_end <= slot) die("slot does not follow the module", NULL);

    /* ---- Classify sections ---- */
    int ovl_sec[MAXOVL];
    uint32_t maxlen = 0;
    for (unsigned k = 0; k < novl; k++) {
        ovl_sec[k] = find_sec(ovl_names[k]);
        if (ovl_sec[k] < 0) die("overlay section missing", ovl_names[k]);
        if (sh[ovl_sec[k]].sh_addr != slot) die("overlay not linked at the slot", ovl_names[k]);
        if (sh[ovl_sec[k]].sh_type != SHT_PROGBITS || sh[ovl_sec[k]].sh_size < SAMIR_OVL_SLOT_HDR_SIZE)
            die("overlay empty", ovl_names[k]);
        if (sh[ovl_sec[k]].sh_size > maxlen) maxlen = sh[ovl_sec[k]].sh_size;
    }
    if (((maxlen + 15u) & ~15u) != slot_end - slot) die("the slot is not exactly the largest overlay", NULL);
    for (unsigned i = 1; i < eh->e_shnum; i++) {
        int isovl = 0;
        for (unsigned k = 0; k < novl; k++) isovl |= (int)i == ovl_sec[k];
        if (!(sh[i].sh_flags & SHF_ALLOC) || isovl || !sh[i].sh_size) continue;
        if (strcmp(secname(i), ".text") && strcmp(secname(i), ".rodata") &&
            strcmp(secname(i), ".data") && strcmp(secname(i), ".bss"))
            die("unexpected allocated section (an orphan of samir.ld, or an unnamed overlay)", secname(i));
        if (sh[i].sh_type == SHT_NOBITS) continue;
        if (sh[i].sh_addr + sh[i].sh_size > load_end) die("loaded section beyond _samir_load_end", secname(i));
    }

    /* ---- The resident module ---- */
    uint8_t *res = calloc(1, load_end ? load_end : 1);
    if (!res) die("out of memory", NULL);
    for (unsigned i = 1; i < eh->e_shnum; i++) {
        int isovl = 0;
        for (unsigned k = 0; k < novl; k++) isovl |= (int)i == ovl_sec[k];
        if (!(sh[i].sh_flags & SHF_ALLOC) || isovl || sh[i].sh_type != SHT_PROGBITS) continue;
        memcpy(res + sh[i].sh_addr, img + sh[i].sh_offset, sh[i].sh_size);
    }

    /* ---- Functions (for the call graph) ---- */
    fns = calloc(nsyms ? nsyms : 1, sizeof *fns);
    for (uint32_t i = 0; i < nsyms; i++) {
        const Elf32_Sym *s = &syms[i];
        if (ELF32_ST_TYPE(s->st_info) != STT_FUNC || !s->st_size || s->st_shndx == SHN_UNDEF ||
            s->st_shndx >= SHN_LORESERVE) continue;
        fns[nfns].lo = s->st_value; fns[nfns].hi = s->st_value + s->st_size;
        fns[nfns].sec = s->st_shndx; fns[nfns].name = symstr + s->st_name; nfns++;
    }

    /* ---- Relocations ---- */
    u32list rrel = {0}, orel[MAXOVL];
    memset(orel, 0, sizeof orel);
    for (unsigned i = 1; i < eh->e_shnum; i++) {
        if (sh[i].sh_type == SHT_RELA) die("RELA relocations are not i386 ELF", secname(i));
        if (sh[i].sh_type != SHT_REL) continue;
        unsigned tsec = sh[i].sh_info;
        if (!(sh[tsec].sh_flags & SHF_ALLOC)) continue;   /* debug/discarded */
        int k = -1;
        for (unsigned j = 0; j < novl; j++) if ((int)tsec == ovl_sec[j]) k = (int)j;
        int resident = k < 0 && sh[tsec].sh_type == SHT_PROGBITS;
        if (k < 0 && !resident) die("relocations against a NOLOAD section", secname(tsec));
        const Elf32_Rel *r = (const Elf32_Rel *)(img + sh[i].sh_offset);
        unsigned n = sh[i].sh_size / sizeof *r;
        for (unsigned e = 0; e < n; e++) {
            unsigned type = ELF32_R_TYPE(r[e].r_info), si = ELF32_R_SYM(r[e].r_info);
            uint32_t site = r[e].r_offset;
            if (si >= nsyms) die("bad relocation symbol", secname(i));
            const Elf32_Sym *s = &syms[si];
            if (site < sh[tsec].sh_addr || site + 4u > sh[tsec].sh_addr + sh[tsec].sh_size)
                die("relocation site outside its section", secname(tsec));
            const uint8_t *field = img + sh[tsec].sh_offset + (site - sh[tsec].sh_addr);
            uint32_t target;
            if (type == R_386_32) {
                if (s->st_shndx == SHN_UNDEF) die("undefined symbol", symstr + s->st_name);
                if (s->st_shndx == SHN_ABS) die("absolute R_386_32 target (would be wrongly relocated)", symstr + s->st_name);
                if (k < 0) push(&rrel, site);
                else push(&orel[k], site - slot);
                target = rd32(field);
            } else if (type == R_386_PC32 || type == R_386_PLT32) {
                if (s->st_shndx == SHN_UNDEF) die("undefined symbol", symstr + s->st_name);
                target = rd32(field) + site + 4u;    /* call/jmp rel32: A = -4 */
            } else {
                char m[64];
                snprintf(m, sizeof m, "type %u in %s", type, secname(i));
                die("unsupported relocation", m);
            }
            /* Call-graph edge: site's function -> target's function. */
            int from = fn_at(tsec, site);
            if (from >= 0 && s->st_shndx < SHN_LORESERVE) {
                int to = fn_at(s->st_shndx, target);
                if (to >= 0) add_edge((unsigned)from, (unsigned)to);
            }
        }
    }

    /* ---- The call-graph rule ---- */
    {
        char *reach = calloc(nfns ? nfns : 1, 1);
        unsigned *stack = malloc((nfns ? nfns : 1) * sizeof *stack), sp = 0;
        int *via = malloc((nfns ? nfns : 1) * sizeof *via);
        for (unsigned i = 0; i < nfns; i++) {
            via[i] = -1;
            for (unsigned k = 0; k < novl; k++)
                if ((int)fns[i].sec == ovl_sec[k]) { reach[i] = 1; stack[sp++] = i; }
        }
        while (sp) {
            unsigned a = stack[--sp];
            for (unsigned e = 0; e < nedges; e++)
                if (edges[e].from == a && !reach[edges[e].to]) {
                    reach[edges[e].to] = 1; via[edges[e].to] = (int)a; stack[sp++] = edges[e].to;
                }
        }
        for (unsigned j = 0; j < nforbid; j++) {
            size_t L = strlen(forbid[j]);
            int seen = 0;
            for (unsigned i = 0; i < nfns; i++) {
                if (strncmp(fns[i].name, forbid[j], L) || (fns[i].name[L] && fns[i].name[L] != '.')) continue;
                seen = 1;
                if (reach[i]) {
                    fprintf(stderr, "samir_ovl_pack: overlay code reaches %s:", fns[i].name);
                    for (int c = via[i]; c >= 0; c = via[c]) fprintf(stderr, " <- %s", fns[c].name);
                    fputc('\n', stderr);
                    die("no-returning-into-an-evicted-overlay rule violated", forbid[j]);
                }
            }
            if (!seen && j == 0) die("the overlay manager is not in the image", forbid[0]);
        }
        free(reach); free(stack); free(via);
    }

    /* ---- Overlay headers ---- */
    for (unsigned k = 0; k < novl; k++) {
        const uint8_t *m = img + sh[ovl_sec[k]].sh_offset;
        int entry_rel = 0;
        if (rd32(m) != SAMIR_OVL_MAGIC || rd32(m + 4) != k) die("overlay slot header missing or out of id order", ovl_names[k]);
        for (unsigned i = 0; i < orel[k].n; i++) entry_rel |= orel[k].v[i] == 8u;
        if (!entry_rel) die("overlay entry pointer is not relocated", ovl_names[k]);
        qsort(orel[k].v, orel[k].n, sizeof(uint32_t), cmp_u32);
    }
    qsort(rrel.v, rrel.n, sizeof(uint32_t), cmp_u32);

    /* ---- SAMIR.OVL ---- */
    size_t body = SAMIR_OVL_HDR_SIZE + (size_t)novl * SAMIR_OVL_DIR_SIZE;
    uint32_t moff[MAXOVL];
    for (unsigned k = 0; k < novl; k++) {
        moff[k] = (uint32_t)body;
        body += sh[ovl_sec[k]].sh_size + (size_t)orel[k].n * 4u;
    }
    size_t flen = (body + SAMIR_OVL_ALIGN - 1u) / SAMIR_OVL_ALIGN * SAMIR_OVL_ALIGN;
    uint8_t *ovl = calloc(1, flen);
    if (!ovl) die("out of memory", NULL);
    memcpy(ovl + SAMIR_OVL_HDR_SIG_OFF, SAMIR_OVL_SIG, SAMIR_OVL_SIG_LEN);
    wr16(ovl + SAMIR_OVL_HDR_VER_OFF, SAMIR_OVL_VERSION);
    wr16(ovl + SAMIR_OVL_HDR_COUNT_OFF, novl);
    wr32(ovl + SAMIR_OVL_HDR_SLOT_OFF, slot_end - slot);
    wr32(ovl + SAMIR_OVL_HDR_FLEN_OFF, (uint32_t)flen);
    for (unsigned k = 0; k < novl; k++) {
        uint32_t len = sh[ovl_sec[k]].sh_size;
        uint8_t *d = ovl + SAMIR_OVL_HDR_SIZE + k * SAMIR_OVL_DIR_SIZE, *m = ovl + moff[k];
        memcpy(m, img + sh[ovl_sec[k]].sh_offset, len);
        for (unsigned i = 0; i < orel[k].n; i++) wr32(m + len + 4u * i, orel[k].v[i]);
        wr32(d + SAMIR_OVL_DIR_OFF_OFF, moff[k]);
        wr32(d + SAMIR_OVL_DIR_LEN_OFF, len);
        wr32(d + SAMIR_OVL_DIR_NREL_OFF, orel[k].n);
        wr32(d + SAMIR_OVL_DIR_CRC_OFF, crc32(0, m, len + (size_t)orel[k].n * 4u));
    }
    uint32_t stamp = crc32(0, ovl + SAMIR_OVL_HDR_SIZE, flen - SAMIR_OVL_HDR_SIZE);
    wr32(ovl + SAMIR_OVL_HDR_STAMP_OFF, stamp);

    /* ---- Patch the stamp into the resident ---- */
    {
        const Elf32_Sym *s = find_sym("samir_ovl_stamp");
        if (!s || s->st_size != 4u || s->st_value + 4u > load_end) die("samir_ovl_stamp missing from the resident module", NULL);
        if (rd32(res + s->st_value)) die("samir_ovl_stamp is not 0 before patching", NULL);
        for (unsigned i = 0; i < rrel.n; i++)
            if (rrel.v[i] + 4u > s->st_value && rrel.v[i] < s->st_value + 4u) die("samir_ovl_stamp overlaps a relocation", NULL);
        wr32(res + s->st_value, stamp);
    }

    /* ---- Write ---- */
    write_file(res_bin, res, load_end);
    {
        uint8_t *rl = malloc(rrel.n ? rrel.n * 4u : 1u);
        for (unsigned i = 0; i < rrel.n; i++) wr32(rl + 4u * i, rrel.v[i]);
        write_file(res_rel, rl, rrel.n * 4u);
        free(rl);
    }
    write_file(ovl_out, ovl, flen);

    fprintf(stderr, "samir_ovl_pack: resident %u B (%u relocs), slot %u B, SAMIR.OVL %zu B stamp %08X;",
            (unsigned)load_end, rrel.n, (unsigned)(slot_end - slot), flen, stamp);
    for (unsigned k = 0; k < novl; k++)
        fprintf(stderr, " %s %u B/%u relocs", ovl_names[k], (unsigned)sh[ovl_sec[k]].sh_size, orel[k].n);
    fprintf(stderr, "; call graph %u functions %u edges\n", nfns, nedges);
    return 0;
}
