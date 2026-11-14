/* keys.c -- turn a journey (MINT06 step grammar) into IDEKEYS.BIN.
 *
 * Usage: keys STEPS OUT.BIN SHOTLIST
 * Steps: type TEXT | key NAME | shot NAME | sleep/wait/wait-gone (ignored)
 * | # comment. NAME is a QEMU sendkey name as in the TP 5.5 mint.
 * Codes are what DOS INT 21h AH=07h delivers: the character, or 0 followed
 * by the scan code, written here as 256 + scan code. The scan codes come
 * from the IBM PC AT BIOS extended-code table (Technical Reference,
 * keyboard section), not from the IDE source. 65535 marks a screen dump,
 * 65534 the end. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct { const char *name; int code; } table[] = {
    { "ret", 13 }, { "esc", 27 }, { "backspace", 8 }, { "tab", 9 }, { "spc", 32 },
    { "ctrl-y", 25 },
    { "f1", 256 + 0x3b }, { "f2", 256 + 0x3c }, { "f3", 256 + 0x3d }, { "f4", 256 + 0x3e },
    { "f5", 256 + 0x3f }, { "f6", 256 + 0x40 }, { "f7", 256 + 0x41 }, { "f8", 256 + 0x42 },
    { "f9", 256 + 0x43 }, { "f10", 256 + 0x44 },
    { "alt-f9", 256 + 0x70 }, { "ctrl-f9", 256 + 0x66 }, { "alt-f5", 256 + 0x6c },
    { "alt-x", 256 + 0x2d }, { "alt-e", 256 + 0x12 }, { "alt-f", 256 + 0x21 },
    { "alt-r", 256 + 0x13 }, { "alt-c", 256 + 0x2e },
    { "home", 256 + 0x47 }, { "up", 256 + 0x48 }, { "pgup", 256 + 0x49 },
    { "left", 256 + 0x4b }, { "right", 256 + 0x4d }, { "end", 256 + 0x4f },
    { "down", 256 + 0x50 }, { "pgdn", 256 + 0x51 }, { "insert", 256 + 0x52 },
    { "delete", 256 + 0x53 }, { "ctrl-end", 256 + 0x75 }, { "ctrl-pgdn", 256 + 0x76 },
    { "ctrl-home", 256 + 0x77 }, { "ctrl-pgup", 256 + 0x84 },
};

static void put(FILE *f, int code) { fputc(code & 255, f); fputc(code >> 8, f); }

int main(int argc, char **argv)
{
    if (argc != 4) { fprintf(stderr, "keys STEPS OUT.BIN SHOTLIST\n"); return 2; }
    FILE *in = fopen(argv[1], "r"), *out = fopen(argv[2], "wb"), *list = fopen(argv[3], "w");
    if (!in || !out || !list) { perror("keys"); return 2; }
    char line[1024];
    int n = 0;
    while (fgets(line, sizeof line, in)) {
        n++;
        line[strcspn(line, "\r\n")] = 0;
        char *arg = strchr(line, ' ');
        if (arg) *arg++ = 0; else arg = line + strlen(line);
        if (!*line || *line == '#' || !strcmp(line, "sleep") || !strcmp(line, "wait") ||
            !strcmp(line, "wait-gone")) continue;
        if (!strcmp(line, "type")) {
            for (; *arg; arg++) put(out, (unsigned char)*arg);
        } else if (!strcmp(line, "key")) {
            int code = -1;
            for (size_t i = 0; i < sizeof table / sizeof table[0]; i++)
                if (!strcmp(arg, table[i].name)) code = table[i].code;
            if (code < 0 && strlen(arg) == 1) code = (unsigned char)arg[0];
            if (code < 0) { fprintf(stderr, "%s:%d: unknown key %s\n", argv[1], n, arg); return 2; }
            put(out, code);
        } else if (!strcmp(line, "shot")) {
            put(out, 65535);
            fprintf(list, "%s\n", arg);
        } else if (!strcmp(line, "end")) {
            break;
        } else { fprintf(stderr, "%s:%d: unknown step %s\n", argv[1], n, line); return 2; }
    }
    put(out, 65534);
    return fclose(out) || fclose(list) ? 2 : 0;
}
