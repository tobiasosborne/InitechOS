#!/bin/sh
# Run from repository root. Uses the actual editor, interpreter and host PAL.
set -eu
. tools/lane-env.sh
cc -std=c11 -O1 -g -Wall -Wextra -Werror -DSAMIR_W4 \
    -Ios/samir/include -Ios/samir/ui -Ispec -o build/host_CREATE \
    harness/diff/dbf_diff/xbase_prog_diff/oneshot/create/host_replay.c \
    os/samir/samir_main.c os/samir/core/rt.c os/samir/core/value.c \
    os/samir/core/lex.c os/samir/core/parse.c os/samir/core/eval.c \
    os/samir/core/fn_builtins.c os/samir/fs/dbf.c os/samir/fs/dbt.c \
    os/samir/fs/ndx.c os/samir/cmd/workarea.c os/samir/cmd/nav.c \
    os/samir/cmd/flow.c os/samir/cmd/query.c os/samir/cmd/mutate.c \
    os/samir/cmd/set.c os/samir/cmd/proc.c os/samir/cmd/report.c \
    os/samir/pal/pal_host.c os/samir/ui/*.c -lm
root=$PWD
for app in create modcmd; do
    out=build/oneshot/$app-host
    mkdir -p "$out/files"
    rm -f "$out/files/WIDGETS.DBF" "$out/files/WIDGETS.BAK" \
        "$out/files/HELLO.PRG" "$out/files/HELLO.BAK"
    (cd "$out/files" && ../../../host_CREATE "$app" > ../after.txt)
    (cd "$out" && OLDPWD="$root" sh "$root/harness/diff/dbf_diff/xbase_prog_diff/oneshot/$app/CHECK")
done
