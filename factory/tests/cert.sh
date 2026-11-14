#!/bin/sh
# Exercise actual coordinator; expensive product stages are controlled successes.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -p "$work/bin"
cat > "$work/bin/make" <<'MAKE'
#!/bin/sh
case "$*" in
 '--no-print-directory -C a mutants') exec /usr/bin/make "$@";;
 '-s dependencies') printf 'a: b\nb: c\nc:\n';;
 *test)
 case $CASE in
 subset) echo "registered 3, run 1; elapsed 0s"; echo "SUBSET GREEN (not ALL GREEN)";;
 no-receipt) echo "ALL GREEN";;
 missing-host) echo "a: 1 tests, 0 failures"; echo "b: 1 tests, 0 failures"; echo "registered 3, run 3; elapsed 0s"; echo "ALL GREEN";;
 *) printf "a: 1 tests, 0 failures\nb: 1 tests, 0 failures\nc: 1 tests, 0 failures\nregistered 3, run 3; elapsed 0s\nALL GREEN\n";;
 esac;;
 status)
 case $CASE in
 red) printf 'a: imported\nb: red\nc: imported\n';;
 host-green) printf 'a: imported\nb: host-green\nc: imported\n';;
 missing) printf 'a: imported\nc: imported\n';;
 duplicate) printf 'a: imported\nb: imported\nb: imported\nc: imported\n';;
 unknown) printf 'a: imported\nb: imported\nc: imported\nd: imported\n';;
 empty) :;;
 malformed) printf 'a: imported\nb: pending\nc: imported\n';;
 *) printf 'a: imported\nb: imported\nc: imported\n';;
 esac;;
esac
MAKE
cat > "$work/bin/git" <<'GIT'
#!/bin/sh
case $1 in rev-parse) echo fixture;; esac
GIT
chmod +x "$work/bin/"*
for strict in 0 1; do
 for name in green red host-green missing duplicate unknown empty malformed pending subset no-receipt missing-host; do
  dir=$work/$strict-$name
  mkdir -p "$dir/factory/testlib" "$dir/a/mutants"
  cp "$root/factory/cert.sh" "$root/factory/mutants.sh" "$dir/factory/"
  cp "$root/factory/testlib/"*.awk "$dir/factory/testlib/"
  : > "$dir/a/mutants/active.patch"
  printf '.PHONY: mutants\nmutants:\n\t@echo "a: 1 mutants, 1 killed, 0 survived, 0 errors"\n' > "$dir/a/Makefile"
  if [ "$name" = pending ]; then
   mkdir -p "$dir/a/mutants/pending"; : > "$dir/a/mutants/pending/later.patch"
  fi
  rc=0
  (cd "$dir" && PATH="$work/bin:$PATH" CASE=$name CERT_STRICT=$strict sh factory/cert.sh) > "$dir/log" 2>&1 || rc=$?
  if [ "$name" = green ]; then
   [ "$rc" -eq 0 ] || { cat "$dir/log"; exit 1; }
   grep -q 'status: PASS' "$dir/log"
  elif [ "$name" = subset ] || [ "$name" = no-receipt ] || [ "$name" = missing-host ]; then
   [ "$rc" -ne 0 ] && grep -q "clean-test: FAIL" "$dir/log" && grep -q "status: PASS" "$dir/log" || { cat "$dir/log"; exit 1; }
  elif [ "$name" = pending ]; then
   [ "$rc" -ne 0 ] && grep -q "mutants: INCOMPLETE" "$dir/log" && grep -q "status: PASS" "$dir/log" || { cat "$dir/log"; exit 1; }
  else
   [ "$rc" -ne 0 ] && grep -q 'status: FAIL' "$dir/log" || { echo "FAIL cert accepted $name strict=$strict"; cat "$dir/log"; exit 1; }
  fi
  echo "ok cert $name strict=$strict"
 done
done
