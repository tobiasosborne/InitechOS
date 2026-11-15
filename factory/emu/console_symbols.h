/* The live text console in a matching unstripped i386 kernel ELF (InitechBase
 * stage 2). kernel/console.c keeps the 4000-byte 80x25 page in the static
 * g_console_page; kernel/startup.c's g_int21_con points at the console_t.
 * Two symbols are named g_console_page (int21.c has a 4-byte function
 * pointer of that name), so the page is the one whose size is 4000. Flat
 * identity mapping: symbol values are linear addresses. */
#ifndef INITECH_CONSOLE_SYMBOLS_H
#define INITECH_CONSOLE_SYMBOLS_H
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int console_symbols(const char *path, uint32_t *page, uint32_t *con)
{
    FILE *f = fopen(path, "rb");
    long size;
    unsigned char *b;
    Elf32_Ehdr eh;
    *page = *con = 0;
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) || (size = ftell(f)) < (long)sizeof eh || size > 64L * 1024 * 1024) { fclose(f); return -1; }
    rewind(f);
    b = malloc((size_t)size);
    if (!b || fread(b, 1, (size_t)size, f) != (size_t)size) { fclose(f); free(b); return -1; }
    fclose(f);
    memcpy(&eh, b, sizeof eh);
    if (memcmp(eh.e_ident, ELFMAG, SELFMAG) || eh.e_ident[EI_CLASS] != ELFCLASS32 ||
        eh.e_machine != EM_386 || eh.e_shentsize != sizeof(Elf32_Shdr) ||
        (uint64_t)eh.e_shoff + (uint64_t)eh.e_shnum * sizeof(Elf32_Shdr) > (uint64_t)size) { free(b); return -1; }
    for (unsigned i = 0; i < eh.e_shnum; i++) {
        Elf32_Shdr sh, st;
        memcpy(&sh, b + eh.e_shoff + i * sizeof sh, sizeof sh);
        if (sh.sh_type != SHT_SYMTAB || sh.sh_link >= eh.e_shnum || sh.sh_entsize != sizeof(Elf32_Sym) ||
            (uint64_t)sh.sh_offset + sh.sh_size > (uint64_t)size) continue;
        memcpy(&st, b + eh.e_shoff + sh.sh_link * sizeof st, sizeof st);
        if ((uint64_t)st.sh_offset + st.sh_size > (uint64_t)size) continue;
        for (uint32_t j = 0; j + sizeof(Elf32_Sym) <= sh.sh_size; j += sizeof(Elf32_Sym)) {
            Elf32_Sym sym;
            memcpy(&sym, b + sh.sh_offset + j, sizeof sym);
            if (sym.st_name >= st.sh_size || !memchr(b + st.sh_offset + sym.st_name, 0, st.sh_size - sym.st_name)) continue;
            const char *name = (const char *)b + st.sh_offset + sym.st_name;
            if (!strcmp(name, "g_console_page") && sym.st_size == 4000u) *page = sym.st_value;
            if (!strcmp(name, "g_int21_con") && sym.st_size == 4u) *con = sym.st_value;
        }
    }
    free(b);
    return *page && *con ? 0 : -1;
}
#endif
