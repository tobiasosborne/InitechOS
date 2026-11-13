#!/bin/sh
set -eu
# The quarantine is unbuilt; inspect only actual target inputs and their headers.
if grep -En '#[[:space:]]*include[[:space:]]*["<]\.\./' "$@" ./*.h; then
 echo 'FAIL desktop: include crosses component boundary' >&2; exit 1
fi
if grep -En '^[[:space:]]*#[[:space:]]*(if|ifdef|ifndef).*MUT' "$@" \
 desktop_internal.h finder_actions.h finder_ops*.h finder_windows*.h shell*.h boot_screen.h; then
 echo 'FAIL desktop: built mutation guard' >&2; exit 1
fi
if grep -n 'attic/' Makefile; then
 echo 'FAIL desktop: attic build input' >&2; exit 1
fi
# Public include roots are explicit in Makefile; no kernel private include root.
if grep -E -- '-I[^ ]*/kernel([[:space:]]|$)' Makefile; then exit 1; fi
echo 'ok desktop: target artifact, public include roots, no built mutants or attic input'
