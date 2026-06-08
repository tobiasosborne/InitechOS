/*
 * initechc.c -- thin driver for the seed Pascal cross-compiler front end.
 *
 * beads: initech-znb ("Step A of the InitechOS seed cross-compiler")
 * Ref:   PRD Sec 6.7 (Turbo Initech: single-pass Pascal -> freestanding x86),
 *        PRD Sec 4 (the SEED compiler -- C-hosted, bootstraps the resident
 *        compiler). CLAUDE.md Law 1 / Law 2 / Rule 2 / Rule 12.
 *
 * Default mode (front end): reads a .pas file (or stdin with '-'), lexes +
 * parses, and either prints "parse OK" plus a compact S-expression AST dump,
 * or prints the located error to stderr and exits non-zero.
 *
 * --emit-asm mode (Step B, beads initech-znb): parse, then walk the AST and
 * emit nasm Intel-syntax 32-bit assembly (PRD Sec 6.7 stack-machine codegen)
 * to stdout or to a file given with -o. The "parse OK"/AST-dump behaviour is
 * unchanged in the default mode.
 *
 * ASCII-clean (Rule 12). No timestamps / nondeterminism (Rule 11).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "parser.h"
#include "codegen.h"

/* Slurp an entire stream into a malloc'd buffer. Returns 0 on success and sets
 * *out_buf / *out_len; the caller frees *out_buf. Returns non-zero on error. */
static int slurp(FILE *fp, char **out_buf, size_t *out_len)
{
    size_t cap = 64 * 1024;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf)
        return 1;
    for (;;) {
        if (len == cap) {
            cap *= 2;
            char *nb = realloc(buf, cap);
            if (!nb) {
                free(buf);
                return 1;
            }
            buf = nb;
        }
        size_t got = fread(buf + len, 1, cap - len, fp);
        len += got;
        if (got == 0) {
            if (ferror(fp)) {
                free(buf);
                return 1;
            }
            break; /* EOF */
        }
    }
    *out_buf = buf;
    *out_len = len;
    return 0;
}

int main(int argc, char **argv)
{
    const char *path = NULL;
    const char *out_path = NULL;
    int emit_asm = 0;
    FILE *fp = stdin;

    /* Parse options. Recognised:
     *   --emit-asm        emit nasm assembly instead of an AST dump
     *   -o <file>         write emitted asm to <file> (default stdout)
     *   <file.pas> | -    input source (default stdin) */
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (strcmp(a, "--emit-asm") == 0) {
            emit_asm = 1;
        } else if (strcmp(a, "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "%s: -o needs an argument\n", argv[0]);
                return 2;
            }
            out_path = argv[++i];
        } else if (strcmp(a, "-") == 0) {
            path = NULL; /* explicit stdin */
        } else if (a[0] == '-' && a[1] != '\0') {
            fprintf(stderr, "%s: unknown option: %s\n", argv[0], a);
            fprintf(stderr,
                    "usage: %s [--emit-asm] [-o out.s] [file.pas | -]\n",
                    argv[0]);
            return 2;
        } else {
            if (path) {
                fprintf(stderr, "%s: multiple input files\n", argv[0]);
                return 2;
            }
            path = a;
        }
    }

    if (path) {
        fp = fopen(path, "rb");
        if (!fp) {
            fprintf(stderr, "initechc: cannot open '%s'\n", path);
            return 2;
        }
    }

    char *src = NULL;
    size_t len = 0;
    if (slurp(fp, &src, &len) != 0) {
        fprintf(stderr, "initechc: read error\n");
        if (fp != stdin)
            fclose(fp);
        return 2;
    }
    if (fp != stdin)
        fclose(fp);

    AstArena arena;
    ParseResult r;
    ast_arena_init(&arena);
    int rc = parse_program(src, len, &arena, &r);

    if (rc != 0) {
        fprintf(stderr, "%s:%d:%d: error: %s\n",
                path ? path : "<stdin>", r.line, r.col, r.error);
        ast_arena_free(&arena);
        free(src);
        return 1;
    }

    if (emit_asm) {
        /* Step B (beads initech-znb): walk the AST and emit nasm Intel-syntax
         * 32-bit assembly (PRD Sec 6.7 stack-machine codegen). */
        FILE *out = stdout;
        if (out_path) {
            out = fopen(out_path, "wb");
            if (!out) {
                fprintf(stderr, "initechc: cannot open output '%s'\n",
                        out_path);
                ast_arena_free(&arena);
                free(src);
                return 2;
            }
        }
        int crc = codegen_emit(r.ast, out);
        if (out != stdout) {
            if (fclose(out) != 0 && crc == 0) {
                fprintf(stderr, "initechc: error closing '%s'\n", out_path);
                crc = 1;
            }
        }
        ast_arena_free(&arena);
        free(src);
        return crc == 0 ? 0 : 1;
    }

    printf("parse OK\n");
    ast_dump(r.ast, stdout);
    printf("\n");

    ast_arena_free(&arena);
    free(src);
    return 0;
}
