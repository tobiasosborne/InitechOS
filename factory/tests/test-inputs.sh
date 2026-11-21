#!/bin/sh
# Tier 0 runs before any component is built (top-level test-start), so no
# recipe in factory's `test` closure may name another component's build
# output: on a clean checkout it is not there (initech-qw018). Read from
# make's own database, the closure as make sees it, not a list kept here.
# Usage: test-inputs.sh [makefile-dir]  (default: factory/)
set -eu
cd "$(dirname "$0")/.."
dir=${1:-.}
db=$(mktemp) out=$(mktemp)
trap 'rm -f "$db" "$out"' EXIT
make -qp -C "$dir" -f Makefile __initech_no_such_target__ > "$db" 2>/dev/null || true
grep -q '^test:' "$db" || { echo "FAIL test inputs: no test target in $dir/Makefile's database"; exit 1; }
awk '
  /^# Not a target/ { skip = 1; next }
  /^[^#\t ][^:=]*:([^=]|$)/ && !/^[^:]*:=/ {
    if (skip) { skip = 0; cur = ""; next }
    cur = substr($0, 1, index($0, ":") - 1)
    rest = substr($0, index($0, ":") + 1); sub(/\|/, " ", rest)
    deps[cur] = deps[cur] " " rest; next
  }
  /^\t/ && cur != "" { recipe[cur] = recipe[cur] "\n" $0; next }
  /^$/ { cur = ""; skip = 0 }
  END {
    n = split("test", queue, " "); seen["test"] = 1
    for (i = 1; i <= n; i++) {
      t = queue[i]; m = split(deps[t], d, " ")
      for (j = 1; j <= m; j++) if (!(d[j] in seen) && (d[j] in deps)) { seen[d[j]] = 1; queue[++n] = d[j] }
    }
    bad = 0
    for (t in seen) {
      r = recipe[t]
      while (match(r, /(\$\(TOP\)|\.\.)\/build\/[A-Za-z0-9_.\/-]*/)) {
        p = substr(r, RSTART, RLENGTH); r = substr(r, RSTART + RLENGTH)
        q = p; sub(/^(\$\(TOP\)|\.\.)\/build\//, "", q)
        if (q !~ /^factory(\/|$)/) { print "FAIL test inputs: " t " reads " p " (another component'"'"'s build output)"; bad = 1 }
      }
    }
    printf "INITECH_CHECKS %d failures=%d\n", length(seen), bad
    exit bad
  }' "$db" > "$out" || { cat "$out"; exit 1; }
cat "$out"
