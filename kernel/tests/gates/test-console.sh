set -eu
printf ">>> test-console: 8x16 glyph blit (MSB-left, bpp 32/24) + cursor/wrap/scroll\n"
build/test_console
printf ">>> test-console: green\n"
