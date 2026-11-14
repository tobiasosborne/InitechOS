#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    unsigned char b[512];
    FILE *f = fopen(argv[1], "r+b");
    if (!f) return 2;
    if (fread(b, 1, sizeof b, f) != sizeof b || fgetc(f) != EOF ||
        b[0] != 0x55 || b[1] != 0xaa || b[2] != 1) return 2;
    unsigned sum = 0;
    for (size_t i = 0; i < sizeof b - 1; i++) sum += b[i];
    b[511] = (unsigned char)-sum;
    rewind(f);
    if (fwrite(b, 1, sizeof b, f) != sizeof b) return 2;
    return fclose(f) ? 2 : 0;
}
