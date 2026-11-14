#!/bin/sh
# CLAUDE.md, How the tree is organised: a mutant is a patch file. Print every
# macro name a checked-in build can define, one per line. The source audit
# fails a product conditional on any other name, whatever it is called.
# Sources: compiler predefines; -D in the component's flags and make database;
# #define, %define, %assign, %macro and {$DEFINE} in the component's own
# files (tests included, mutants never) and in files under its -I directories.
set -eu
database=$1 flags=$2
target_cc=${IMPORT_TARGET_CC:-gcc} host_cc=${IMPORT_HOST_CC:-cc}
dirs=.
for word in $flags; do
    case $word in -I?*) dir=${word#-I}; [ ! -d "$dir" ] || dirs="$dirs $dir";; esac
done
# Standard headers define their limits (UINTPTR_MAX) as the platform's, not ours.
freestanding='float.h iso646.h limits.h stdalign.h stdarg.h stdbool.h stddef.h stdint.h stdnoreturn.h'
hosted="$freestanding assert.h ctype.h errno.h inttypes.h signal.h stdio.h stdlib.h string.h time.h fcntl.h unistd.h sys/stat.h sys/types.h"
headers() { for h in $1; do printf '#include <%s>\n' "$h"; done; }
{
    headers "$freestanding" | $target_cc -m32 -ffreestanding -dM -E -x c -
    headers "$hosted" | $host_cc -dM -E -x c -
} | awk '$1=="#define" {name=$2; sub(/\(.*/, "", name); print name}'
# The C++ language predefines this for headers shared with C++ compilers.
echo __cplusplus
printf '%s\n' "$flags" | cat - "$database" |
    grep -oE -- '-D[[:space:]]*[A-Za-z_][A-Za-z0-9_]*' | sed 's/^-D[[:space:]]*//' || :
# shellcheck disable=SC2086
find $dirs -type d \( -name mutants -o -name build -o -name attic -o -name .git \) -prune \
    -o -type f \( -name '*.c' -o -name '*.h' -o -name '*.asm' -o -name '*.inc' -o -name '*.pas' \) -print |
    sort -u | while IFS= read -r file; do cat "$file"; echo; done |
    grep -ioE '^[[:space:]]*(#[[:space:]]*define|%[ix]?define|%assign|%macro|\{\$define)[[:space:]]+[A-Za-z_][A-Za-z0-9_]*' |
    awk '{name=$NF; if ($0 ~ /\{\$/) name=toupper(name); print name}' || :
