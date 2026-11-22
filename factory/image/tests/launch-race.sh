#!/bin/sh
# A second actor publishes a pair while the launcher's image build runs.
set -eu
launcher=$(cd "$(dirname "$0")/.." && pwd)/run.sh
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
cd "$work"
cat > make-image <<'MAKE'
#!/bin/sh
set -eu
[ "$1" = image ]
mkdir -p build/image user-disks
for disk in boot data; do
 printf 'master %s\n' "$disk" > "build/image/$disk.img"
 printf 'other %s\n' "$disk" > "user-disks/$disk.img"
done
MAKE
chmod +x make-image
rc=0
MAKEFLAGS= sh "$launcher" ./make-image run > transcript 2>&1 || rc=$?
[ "$rc" = 1 ] && grep -Fxq 'InitechOS: user-disks appeared during preparation. Start again to use it.' transcript ||
 { cat transcript >&2; echo 'FAIL missing launch race refusal' >&2; exit 1; }
for disk in boot data; do
 printf 'other %s\n' "$disk" > expected
 cmp expected "user-disks/$disk.img"
done
set -- .user-disks.*
[ ! -e "$1" ] || { echo 'FAIL leaked staging directory' >&2; exit 1; }
echo 'ok launcher reports the competing pair and preserves both disks'
