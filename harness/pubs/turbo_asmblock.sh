#!/bin/sh
# turbo_asmblock.sh LABEL FILE -- print the lines of a Turbo Initech TPSOUT.S
# from `LABEL:' up to (not including) the next top-level label, `global' or
# `section' line, without trailing blank lines (bead initech-ipu5.12; used
# by turbo_replay.sh for printed assembly excerpts).
set -eu
awk -v l="$1:" '
  $0 == l { on = 1; n = 0; buf[n++] = $0; next }
  on && (/^[A-Za-z_][A-Za-z0-9_]*:$/ || /^global / || /^section /) { exit }
  on { buf[n++] = $0 }
  END { while (n > 0 && buf[n-1] == "") n--; for (i = 0; i < n; i++) print buf[i] }' "$2"
