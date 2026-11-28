#!/bin/sh
# Top-level smoke leg for dos's shell-fidelity cases (initech-ptb32), run
# on the image the top smoke has already built. `make -C dos
# smoke-shell-fidelity` rebuilds the image first, and the parallel legs
# are reading that image, so the leg runs the scenarios itself. The case
# list stays in dos/tests.mk; an empty or missing list fails.
set -eu
cd "$(dirname "$0")/.."
root=$PWD
cases=$(sed -n 's/^SHELL_FIDELITY_CASES := //p' dos/tests.mk)
[ -n "$cases" ] || { echo 'FAIL smoke-shell-fidelity: no SHELL_FIDELITY_CASES in dos/tests.mk' >&2; exit 1; }
mkdir -p build/smoke
out=$(mktemp -d "$root/build/smoke/shell-fidelity.XXXXXX")
echo "shell-fidelity evidence: $out"
n=0
for scenario in $cases; do
    [ -f "dos/tests/runtime/$scenario.sh" ] ||
        { echo "FAIL smoke-shell-fidelity: dos/tests/runtime/$scenario.sh missing" >&2; exit 1; }
    sh "dos/tests/runtime/$scenario.sh" "$root" "$root/build/image/boot.img" "$out/$scenario"
    n=$((n+1))
done
echo "GREEN shell-fidelity: $n cases"
