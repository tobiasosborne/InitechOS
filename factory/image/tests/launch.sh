#!/bin/sh
# Exercise the real launch targets, including make clean, on a source copy.
# Exclude operator disks and factory outputs before invoking either launcher.
set -eu
cd "$(dirname "$0")/../../.."
mkdir -p build
work=$(mktemp -d "$PWD/build/launch-test.XXXXXX")
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM
mkdir "$work/tree"
sh factory/testlib/snapshot.sh "$work/tree" --exclude=./build --exclude=./user-disks \
    --exclude='./.user-disks.*' --exclude=./.git --exclude=./.beads --exclude=./attic .
cd "$work/tree"
export LAUNCH_TEST_DIR="$work" PATH="$work:$PATH"

cat > "$work/qemu-system-i386" <<'EOF'
#!/bin/sh
set -eu
for arg do
    case "$arg" in
        file=*,format=raw,index=0)
            disk=${arg#file=}
            printf '%s\n' "${disk%,format=raw,index=0}" > "$LAUNCH_TEST_DIR/boot"
            ;;
        file=*,format=raw,index=1)
            disk=${arg#file=}
            printf '%s\n' "${disk%,format=raw,index=1}" > "$LAUNCH_TEST_DIR/data"
            ;;
    esac
done
EOF
cat > "$work/bochs" <<'EOF'
#!/bin/sh
set -eu
while [ "$1" != -f ]; do shift; done
sed -n 's/^ata0-master:.*path="\([^"]*\)".*/\1/p' "$2" > "$LAUNCH_TEST_DIR/boot"
sed -n 's/^ata0-slave:.*path="\([^"]*\)".*/\1/p' "$2" > "$LAUNCH_TEST_DIR/data"
# The harness expects a real Bochs RFB session. Record its disk paths and
# exit with a distinct status so the test can identify the deliberate stop.
exit 86
EOF
chmod +x "$work/qemu-system-i386" "$work/bochs"

start() {
    rm -f "$work/boot" "$work/data"
    if [ "$1" = run ]; then
        make run
    elif make run-bochs > "$work/bochs.log" 2>&1 ||
        ! grep -Fq 'ended abnormally (exit=86 ' "$work/bochs.log"; then
        cat "$work/bochs.log" >&2
        return 1
    fi
    boot=$(cat "$work/boot")
    data=$(cat "$work/data")
    test -f "$boot" && test -f "$data"
}

check_marker() {
    if ! mtype -i "$1" ::/LAUNCH.OK > "$work/readback" ||
        ! cmp "$work/marker" "$work/readback"; then
        echo "launch test: ${2}: the saved marker is missing or changed" >&2
        exit 1
    fi
}

master_hashes() {
    sha256sum build/image/boot.img build/image/data.img build/image/c.img build/image/a-data.img
}

start run
master_hashes > "$work/before"
cmp build/image/boot.img "$boot"
cmp build/image/data.img "$data"
# The boot FAT12 partition begins at LBA 63. mtools writes both volumes
# independently of InitechDOS, the build scripts and the emulators.
printf 'InitechOS restart marker\n' > "$work/marker"
mcopy -i "$data" "$work/marker" ::/LAUNCH.OK
mcopy -i "${boot}@@32256" "$work/marker" ::/LAUNCH.OK

make image
start run
check_marker "$data" 'A: on the second QEMU start'
check_marker "${boot}@@32256" 'C: on the second QEMU start'
start run-bochs
check_marker "$data" 'A: on the first Bochs start'
check_marker "${boot}@@32256" 'C: on the first Bochs start'

# Change a save between Bochs starts so the second start must see fresh
# data from the working disk set, including writes to the master disk.
printf 'InitechOS second restart marker\n' > "$work/marker"
mcopy -o -i "$data" "$work/marker" ::/LAUNCH.OK
mcopy -o -i "${boot}@@32256" "$work/marker" ::/LAUNCH.OK
make image
start run-bochs
check_marker "$data" 'A: on the second Bochs start'
check_marker "${boot}@@32256" 'C: on the second Bochs start'
master_hashes > "$work/after"
cmp "$work/before" "$work/after"

make clean
start run
check_marker "$data" 'A: following make clean'
check_marker "${boot}@@32256" 'C: following make clean'
echo 'Launch persistence: both drives survive QEMU, Bochs, make image and make clean'
