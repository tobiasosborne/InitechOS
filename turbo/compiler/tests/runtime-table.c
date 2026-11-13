/* Factory only. Turn NASM/ld's runtime .text into Pascal byte constants.
 * Dynamic dwords are replaced explicitly by the native layout/fixup encoder.
 * No object reader or assembler is part of Turbo Initech. */
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
    unsigned char data[4096];
    if (argc != 3) return 2;
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 2;
    size_t n = fread(data, 1, sizeof data, f);
    if (ferror(f) || !feof(f) || fclose(f) || n != 434) return 2;
    puts("{ BEGIN NASM RUNTIME TABLE -- tests/runtime-table.c; do not edit. }");
    printf("procedure %s();\nbegin\n", argv[2]);
    for (size_t i=0; i<n;) {
        const char *expr = NULL;
        if (i==2) expr="Native[4]";
        if (i==7) expr="Native[5]";
        if (i==35) expr="Native[6] - 39";
        if (i==401) expr="Native[3]";
        if (expr) {
            printf("  %s(%s);\n", i==35 ? "NativeWord" : "NativeAbsolute", expr); i+=4;
        } else {
            printf("  GenText := '");
            size_t end = i+48;
            while(i<n && i<end && i!=2 && i!=7 && i!=35 && i!=401)
                printf("%02X", data[i++]);
            puts("'; NativeHex(GenText)");
            /* Pascal requires separators between all calls. */
            puts("  ;");
        }
    }
    puts("  while Native[2] < 448 do NativeByte(144)\nend;");
    puts("{ END NASM RUNTIME TABLE }");
    return 0;
}
