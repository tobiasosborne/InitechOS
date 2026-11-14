#!/bin/sh
# Actual make query, literal graph invariant and injected acquisition failures.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -p "$work/self"
printf 'all: artifact\nartifact:\n\t@touch $@\ntest: all\n' > "$work/self/Makefile"
query() { (cd "$work" && sh "$root/factory/testlib/dependency-query.sh" "$work" self "$1" "$work/query.make"); }
query '/usr/bin/make'
awk -v root="$work" -v component=self -v components='self other' -v declared='' -f "$root/factory/testlib/build-deps.awk" "$work/query.make.parent"
touch "$work/self/artifact"
query '/usr/bin/make'
# Child databases must not be attributed to this component.
printf '# Make data base, child\nCURDIR := elsewhere\n/repo/build/undeclared/fault\n' > "$work/nested"
cat "$work/query.make" >> "$work/nested"
awk -v expected="$work/self" -f "$root/factory/testlib/make-database.awk" "$work/nested" > "$work/parent"
! grep -q undeclared "$work/parent"
for fault in error empty malformed truncated wrong-directory; do
 cat > "$work/fake-make" <<'FAKE'
#!/bin/sh
case $FAULT in
 error) exit 2;;
 empty) exit 1;;
 malformed) echo not-a-graph; exit 0;;
 truncated) sed '/^# Finished Make data base/,$d' "$GRAPH"; exit 1;;
 wrong-directory) sed 's/^CURDIR := .*/CURDIR := wrong/' "$GRAPH"; exit 0;;
esac
FAKE
 chmod +x "$work/fake-make"
 cp "$work/query.make" "$work/saved"
 if FAULT=$fault GRAPH=$work/saved query "$work/fake-make" > "$work/log" 2>&1; then
  echo "FAIL dependency: accepted $fault"; exit 1
 fi
 grep -q 'FAIL dependency:' "$work/log"
 echo "ok dependency rejects $fault"
 # Restore independently produced, complete make data for the next control.
 cp "$work/saved" "$work/query.make"
done
echo 'ok dependency: real query0/1, parent-only graph, acquisition failures'
