set -eu
printf ">>> test-loader: loader_prepare layout (psp/image+0x100/entry/stack) + params + fail-loud validation\n"
build/test_loader
printf ">>> test-loader: green\n"
