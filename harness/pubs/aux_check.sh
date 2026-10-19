#!/bin/sh
# Ref: initech-46u4's stale-minitoc reproduction; PUB-REF-0001 Sec 11.2,
# GC-07 and CLAUDE.md Rules 1/6. Use real LuaLaTeX, never a stub golden.
set -eu
out=$1/aux-check
book=initechword-install
doc=$1/$book
make=${MAKE:-make}
mkdir -p "$out"
rm -rf "$doc"
"$make" --no-print-directory -j3 "pubs-$book" > "$out/baseline.log" 2>&1 || {
    cat "$out/baseline.log"; exit 1;
}
trap 'exit 1' HUP INT TERM
for style in pubs/class/initech-manual.cls pubs/class/initech-wpc.sty pubs/common/initech-logo.tex; do
    touch -r "$style" "$out/style.time"
    trap 'touch -r "$out/style.time" "$style"' EXIT
    # Preserve the aux date while planting a line from the obsolete minitoc
    # interface, so touching the dependency really makes it newer than main.aux.
    touch -r "$doc/main.aux" "$out/aux.time"
    printf '\\global\\mtcsecondpartfalse\n' >> "$doc/main.aux"
    touch -r "$out/aux.time" "$doc/main.aux"
    printf 'stale minitoc data\n' > "$doc/main.maf"
    touch "$doc/.aux-clean-sentinel"
    touch "$style"
    [ "$style" -nt "$doc/main.aux" ] || {
        printf '!!! test-pubs-aux RED: fixture dependency is not newer\n'; exit 1;
    }
    log=$out/rebuild-$(basename "$style").log
    if ! "$make" --no-print-directory -j3 "pubs-$book" > "$log" 2>&1; then
        cat "$log"
        printf '!!! test-pubs-aux RED: newer dependency left an incompatible main.aux\n'
        exit 1
    fi
    [ ! -e "$doc/.aux-clean-sentinel" ] && [ ! -e "$doc/main.maf" ] || {
        printf '!!! test-pubs-aux RED: stale document directory was not cleaned\n'; exit 1;
    }
    grep -F ">>> pubs: cleaned $book " "$log"
    touch -r "$out/style.time" "$style"
    trap - EXIT
done
touch "$doc/.aux-clean-sentinel"
"$make" --no-print-directory -j3 "pubs-$book" > "$out/unchanged.log" 2>&1 || {
    cat "$out/unchanged.log"; exit 1;
}
[ -f "$doc/.aux-clean-sentinel" ] || {
    printf '!!! test-pubs-aux RED: unchanged document directory was cleaned\n'; exit 1;
}
# A newer unused family style must also retain the directory. The recorder
# distinguishes shared inputs from styles this document never loaded.
style=pubs/class/initech-borland.sty
touch -r "$style" "$out/style.time"
trap 'touch -r "$out/style.time" "$style"' EXIT
touch "$style"
"$make" --no-print-directory -j3 "pubs-$book" > "$out/unrelated.log" 2>&1 || {
    cat "$out/unrelated.log"; exit 1;
}
[ -f "$doc/.aux-clean-sentinel" ] || {
    printf '!!! test-pubs-aux RED: unused style cleaned the document directory\n'; exit 1;
}
rm -f "$doc/.aux-clean-sentinel"
printf '>>> test-pubs-aux: green (newer class/style/shared input cleaned stale aux/maf; unchanged and unused-style builds retained their directory)\n'
