#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
root=$PWD
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -p "$work/src" "$work/dst" "$work/bin"
printf 'independent source bytes\n' > "$work/src/file"
sh factory/testlib/snapshot.sh "$work/dst" -C "$work/src" .
cmp "$work/src/file" "$work/dst/file"
# Tiny trees keep tier 0 independent of warm build size or emulator availability;
# each entry point and the acquisition helper are the actual production scripts.
mkdir -p "$work/tree/factory/testlib" "$work/tree/factory/mutants/gates" "$work/tree/build"
cp factory/testlib/snapshot.sh "$work/tree/factory/testlib/"
cp factory/gate-mutants.sh factory/red-desktop.sh factory/smoke-mutants.sh factory/navigation-mutants.sh "$work/tree/factory/"
mkdir -p "$work/tree/apps/initechbase/tools"
cp apps/initechbase/tools/run-target-mutants.sh "$work/tree/apps/initechbase/tools/"
: > "$work/tree/factory/mutants/gates/control.patch"
printf 'independent reused build bytes\n' > "$work/tree/build/file"
cd "$work/tree"
cat > "$work/bin/tar" <<'TAR'
#!/bin/sh
/usr/bin/tar "$@" || exit "$?"
for arg do
 case $arg in -cf) echo 'injected tar source-read failure after complete archive' >&2; exit 2;; esac
done
TAR
chmod +x "$work/bin/tar"
for script in gate-mutants.sh red-desktop.sh smoke-mutants.sh navigation-mutants.sh; do
 rc=0
 PATH="$work/bin:$PATH" sh "factory/$script" > "$work/$script.log" 2>&1 || rc=$?
 [ "$rc" -ne 0 ] || { echo "FAIL snapshot: $script accepted failed producer"; cat "$work/$script.log"; exit 1; }
 grep -q 'FAIL snapshot: source archive creation failed' "$work/$script.log" || { echo "FAIL snapshot: $script did not reject the producer at acquisition"; cat "$work/$script.log"; exit 1; }
 # Acquisition fails before any semantic kills are credited.
 if grep -q 'killed .*semantic' "$work/$script.log"; then cat "$work/$script.log"; exit 1; fi
 echo "ok snapshot: $script rejects source-read failure"
done
cat > "$work/bin/no-make" <<'MAKE'
#!/bin/sh
touch "$MAKE_CALLED"
exit 2
MAKE
chmod +x "$work/bin/no-make"
rc=0
(cd apps/initechbase && MAKE_CALLED="$work/make-called" PATH="$work/bin:$PATH" sh tools/run-target-mutants.sh "$work/tree" "$work/base-run" "$work/bin/no-make" unused.patch) > "$work/base.log" 2>&1 || rc=$?
[ "$rc" -ne 0 ] && [ ! -e "$work/make-called" ] && grep -q 'FAIL snapshot: source archive creation failed' "$work/base.log" || {
 echo 'FAIL snapshot: InitechBase target runner accepted failed producer before build'; cat "$work/base.log"; exit 1;
}
echo 'ok snapshot: InitechBase target runner rejects source-read failure before make'
echo 'ok snapshot: intact independent bytes, failed producers stop all five graders'
# The two runners reusing build outputs must check that second producer too.
cat > "$work/bin/tar" <<'TAR'
#!/bin/sh
case " $* " in
 *' -C build '*) /usr/bin/tar "$@" || exit "$?"; exit 2;;
esac
exec /usr/bin/tar "$@"
TAR
for script in smoke-mutants.sh navigation-mutants.sh; do
 rc=0
 PATH="$work/bin:$PATH" sh "factory/$script" > "$work/$script.log" 2>&1 || rc=$?
 [ "$rc" -ne 0 ] && grep -q 'FAIL snapshot: source archive creation failed' "$work/$script.log" || { cat "$work/$script.log"; exit 1; }
 if grep -q 'killed .*semantic' "$work/$script.log"; then cat "$work/$script.log"; exit 1; fi
 echo "ok snapshot: $script rejects reused-build producer failure"
done
