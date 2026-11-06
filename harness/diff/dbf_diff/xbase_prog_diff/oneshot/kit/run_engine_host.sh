#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../../../../../.." && pwd)
cd "$root"
mkdir -p build/kit_engine
${CC:-cc} -std=c11 -Wall -Wextra -Werror -O1 -DSAMIR_W4 -DUI_HAVE_browse -DUI_HAVE_edit \
    -Ios/samir/include -Ios/samir/ui -Ispec \
    harness/diff/dbf_diff/xbase_prog_diff/oneshot/kit/kit_engine_test.c \
    os/samir/samir_main.c os/samir/core/rt.c os/samir/core/value.c \
    os/samir/core/lex.c os/samir/core/parse.c os/samir/core/eval.c \
    os/samir/core/fn_builtins.c os/samir/fs/dbf.c os/samir/fs/dbt.c \
    os/samir/fs/ndx.c os/samir/cmd/*.c os/samir/pal/pal_host.c \
    os/samir/ui/ui.c os/samir/ui/kit.c os/samir/ui/browse.c os/samir/ui/edit.c \
    -o build/kit_engine_test
cp -f build/CLIENTS.DBF build/kit_engine/CLIENTS.DBF
cd build/kit_engine
../kit_engine_test
mkdir -p files
cp -f CLIENTS.DBF files/CLIENTS.DBF
sh "$root/harness/diff/dbf_diff/xbase_prog_diff/oneshot/kit/CHECK"
