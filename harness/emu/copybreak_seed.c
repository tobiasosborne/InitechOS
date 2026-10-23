/* Independent one-clip fixture: spec/notepad/store.json. Factory only. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv)
{
    unsigned char b[32848] = {0};
    uint32_t h = 2166136261u;
    if (argc != 2) return 2;
    memcpy(b, "SBK1", 4);
    b[4] = 1; b[6] = 1; b[10] = 32;
    b[16] = 5; memcpy(b + 18, "alpha", 5);
    for (unsigned i = 0; i < sizeof b; ++i) h = (h ^ b[i]) * 16777619u;
    for (unsigned i = 0; i < 4; ++i) b[12+i] = (unsigned char)(h >> (8*i));
    FILE *f = fopen(argv[1], "wb");
    if (!f) return 2;
    int ok = fwrite(b, 1, sizeof b, f) == sizeof b;
    return fclose(f) || !ok;
}
