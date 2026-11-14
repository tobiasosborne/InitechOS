#!/bin/sh
# Drive the top recipe with complete synthetic parent databases or broken queries.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -p "$work/tree/factory/testlib" "$work/tree/mk"
cp "$root/Makefile" "$work/tree/"
cp "$root/mk/dependencies.mk" "$work/tree/mk/"
cp "$root/factory/testlib/"*.awk "$root/factory/testlib/"*.sh "$work/tree/factory/testlib/"
cat > "$work/make" <<'MAKE'
#!/bin/sh
component=$4
case $QUERY_CASE in
 error) exit 2;;
 empty) exit 1;;
 malformed) echo not-a-graph; exit 0;;
esac
# Recursive child output must not be attributed to the parent component.
printf '# Make data base, child\nCURDIR := child\n%s/build/forbidden/child.o: source.c\n# Finished Make data base\n' "$ROOT"
printf '# Make data base, parent\nCURDIR := %s/%s\n# Files\nall: artifact\ntest: all\nartifact: source.c\n' "$ROOT" "$component"
case $QUERY_CASE in
 truncated) exit 1;;
 wrong-directory) echo 'CURDIR := wrong';;
 undeclared) printf '%s/build/apps/initech123/header.h: source.c\n' "$ROOT";;
esac
echo '# Finished Make data base'
case $QUERY_CASE in fresh) exit 1;; esac
MAKE
chmod +x "$work/make"
for mode in clean fresh error empty malformed truncated wrong-directory undeclared; do
 rc=0
 (cd "$work/tree" && ROOT="$work/tree" QUERY_CASE=$mode make -s --no-print-directory dependency-check MAKE="$work/make") > "$work/log" 2>&1 || rc=$?
 case $mode in
 clean|fresh) [ "$rc" -eq 0 ] || { cat "$work/log"; exit 1; };;
 *) [ "$rc" -ne 0 ] && grep -q 'FAIL dependency:' "$work/log" || { echo "FAIL dependency: accepted $mode"; cat "$work/log"; exit 1; };;
 esac
 echo "ok dependency query $mode"
done
