#!/bin/sh
set -eu
cc -std=c11 -O1 -Wall -Wextra -Werror -Ios/samir/include -Ios/samir/ui -Ispec \
    harness/diff/dbf_diff/xbase_prog_diff/oneshot/browse/host.c \
    os/samir/core/rt.c os/samir/core/value.c os/samir/core/lex.c \
    os/samir/core/parse.c os/samir/core/eval.c os/samir/core/fn_builtins.c \
    os/samir/core/screen.c os/samir/cmd/workarea.c os/samir/cmd/nav.c \
    os/samir/cmd/flow.c os/samir/cmd/query.c os/samir/cmd/mutate.c \
    os/samir/cmd/set.c os/samir/cmd/proc.c os/samir/cmd/report.c \
    os/samir/fs/dbf.c os/samir/fs/dbt.c os/samir/fs/ndx.c \
    os/samir/samir_main.c os/samir/pal/pal_host.c \
    os/samir/ui/kit.c os/samir/ui/browse.c -o build/oneshot_browse_host
build/oneshot_browse_host
root=$(pwd)
(cd build/oneshot/browse/host/acceptance && sh "$root/harness/diff/dbf_diff/xbase_prog_diff/oneshot/browse/CHECK")
python3 -I harness/diff/dbf_diff/dbf_ref.py --schema build/oneshot/browse/host/CLIENTS.DBF
python3 -I harness/diff/dbf_diff/dbf_ref.py --records build/oneshot/browse/host/CLIENTS.DBF
python3 -I harness/diff/dbf_diff/ndx_ref.py --index-dump build/oneshot/browse/host/CITY.NDX
python3 -I harness/diff/dbf_diff/ndx_ref.py --seek build/oneshot/browse/host/CITY.NDX oslo
