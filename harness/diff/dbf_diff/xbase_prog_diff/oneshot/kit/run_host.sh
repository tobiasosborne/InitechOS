#!/bin/sh
# Run the fake-PAL acceptance suite from any working directory.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../../../../../.." && pwd)
cd "$root"
mkdir -p build
${CC:-cc} -std=c11 -Wall -Wextra -Werror -DSAMIR_W4 -Ios/samir/include \
    ${KIT_TEST_CFLAGS:-} os/samir/ui/kit.c \
    harness/diff/dbf_diff/xbase_prog_diff/oneshot/kit/kit_host_test.c \
    -o build/kit_host_test
build/kit_host_test
