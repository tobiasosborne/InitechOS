#!/bin/sh
# Independent tiny positive and negative fixtures; no product expected values.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
cd "$work"
printf 'all: artifact\nartifact: product.c\n' > graph
: > Makefile
printf '#ifdef PARKED_MUT_FAULT\n#endif\n#include "../private.h"\n' > parked.c
mkdir -p tests nested/tests mutants
printf '#ifdef TEST_MUT_FAULT\n#endif\n#include "../private.h"\n' > tests/fixture.c
cp tests/fixture.c nested/tests/fixture.c
cp tests/fixture.c mutants/fixture.c
cat > product.c <<'EOF'
/* A removed OLD_MUT_FAULT hook. */
const char *message = "OLD_MUT_FAULT";
/* multiline
#ifdef OLD_MUT_FAULT
#include "../retired.h"
*/
#include "local.h"
EOF
check() { sh "$root/factory/testlib/import-check.sh" '_(MUT|MUTATE|MUTANT)_' graph artifact '' '' Makefile > log 2>&1; }
printf '{ OLD_MUT_FAULT }\n(* OLD_MUT_FAULT *)\nbegin end.\n' > public.pas
printf 'artifact: public.pas\n' >> graph
check
printf '{$IFDEF LIVE_MUT_FAULT}\n' >> public.pas
if check; then echo 'FAIL Pascal hook accepted'; exit 1; fi
grep -q '^FAIL mutation:' log
printf 'begin end.\n' > public.pas
check
for line in '#ifdef LIVE_MUT_FAULT' '#if defined(LIVE_MUTATE_FAULT)' '#if LIVE_MUTANT_FAULT' '#include "../private.h"' '#include "attic/old.h"'; do
    cp product.c saved
    printf '%s\n' "$line" >> product.c
    if check; then echo "FAIL audit accepted $line"; exit 1; fi
    grep -q '^FAIL ' log
    mv saved product.c
done
printf 'all: other\nartifact: product.c\n' > graph
if check; then echo 'FAIL unreachable artifact accepted'; exit 1; fi
grep -q '^FAIL target:' log
echo 'audit fixtures: clean/comment/test fixtures green; live hooks, escapes and unreachable artifact red'
# A dependency is required for an actual external build path, including headers.
printf '/repo/build/other/header.h\n/repo/build/self/own.o\n' > deps
if awk -v root=/repo -v component=self -v components='self other' -v declared='' -f "$root/factory/testlib/build-deps.awk" deps > log; then
    echo 'FAIL undeclared build dependency accepted'; exit 1
fi
grep -q '^FAIL dependency:' log
awk -v root=/repo -v component=self -v components='self other' -v declared=other -f "$root/factory/testlib/build-deps.awk" deps
echo "dependency fixtures: undeclared red; declared green"
