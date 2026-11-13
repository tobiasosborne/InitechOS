#!/bin/sh
# MS-DOS 3.3 User's Reference printed pp.57-58 (PDF142-143), wildcards
# pp.10-11 (PDF95-96). Same host matcher as test-80k, mdir is independent.
set -efu
export INITECH_QMP_STDIO=1 LC_ALL=C TZ=UTC
mode=$1
boot=${2:-unused}
tag=${3:-green}
out=build/dir_match_${mode}_${tag}
mkdir -p "$out"
fail() { printf 'RED dir-match: %s\n' "$*" >&2; exit 1; }
dd if=/dev/zero of="$out/data.img" bs=512 count=2880 status=none
mformat -i "$out/data.img" -f 1440 -N 102 ::
printf 'fixture\r\n' > "$out/file"
for f in ALPHA.TXT ALPHA.BIN A.TXT A FOO.TXT FOO1.TXT FOO12.TXT FOOA.BIN ZED.BIN; do
    mcopy -i "$out/data.img" "$out/file" "::$f"
done
cat > "$out/autoexec" <<'BAT'
@echo off
echo TXT-BEGIN
dir *.TXT
echo TXT-END
echo A-BEGIN
dir A*.*
echo A-END
echo FOO-BEGIN
dir FOO?.*
echo FOO-END
echo DIR-MATCH-DONE
BAT
mcopy -i "$out/data.img" "$out/autoexec" ::AUTOEXEC.BAT
cp -f "$out/data.img" "$out/reference.img"
mdir -b -i "$out/reference.img" :: > "$out/all"
for row in 'TXT *.TXT' 'A A*.*' 'FOO FOO?.*'; do
    set -- $row
    # $row is two literal fields; never expand its DOS wildcard in the shell.
    name=$1 pattern=$2
    # mtools uses host glob semantics: '?' does not consume DOS pad spaces,
    # and '*.*' excludes extensionless names. Query the missing exact short
    # name explicitly; this adapter does not reuse the artifact matcher.
    case "$name" in
        TXT) mdir -b -i "$out/reference.img" '::*.TXT' ;;
        A) mdir -b -i "$out/reference.img" '::A*' ;;
        FOO) mdir -b -i "$out/reference.img" '::FOO?.*' '::FOO.*' ;;
    esac | sed 's@.*/@@' | sort -u > "$out/$name.expected"
    "${DIR_MATCH_HOST:-build/dir_match}" "$pattern" < "$out/all" | sort > "$out/$name.host"
    cmp "$out/$name.expected" "$out/$name.host" || fail "$pattern host != mdir"
done
# These short-name cases distinguish DOS padded '?' from POSIX '?' semantics.
grep -qx 'FOO.TXT' "$out/FOO.expected" || fail 'reference omitted padded question mark'
grep -qx 'A' "$out/A.expected" || fail 'reference omitted extensionless star'
if [ "$mode" = emu ]; then
    build/qemu_harness --disk "$boot" --disk2 "$out/data.img" --out "$out" --name run \
        --rtc-base 1987-07-06T13:30:58 --rtc-deterministic --timeout-ms 20000 \
        --keys-after SHELL-READY --keys 'e,x,i,t,ret' --quit-after SHELL-DONE \
        2> "$out/report" || true
    grep -q '^SHELL-READY$' "$out/run.serial" || fail 'shell did not boot'
    grep -q 'quit_after_found=1' "$out/report" || fail 'injected EXIT did not finish'
    grep -q 'keys_sent=5' "$out/report" || fail 'trace incomplete'
    ! grep -q 'triple_fault=1' "$out/report" || fail 'triple fault'
    tr -d '\r' < "$out/run.serial" > "$out/text"
    grep -qx 'DIR-MATCH-DONE' "$out/text" || fail 'batch incomplete'
    for name in TXT A FOO; do
        sed -n "/^$name-BEGIN$/,/^$name-END$/p" "$out/text" |
            awk '/^[A-Z0-9_]+ +[A-Z0-9 ]* +[0-9]+ +[0-9]+-[0-9]+-[0-9]+/ {
                n=substr($0,1,8); e=substr($0,10,3); gsub(/ +/,"",n);gsub(/ +/,"",e);
                print n (e==""?"":"." e)
            }' | sort > "$out/$name.actual"
        cmp -s "$out/$name.expected" "$out/$name.actual" || fail "$name shell membership != mdir/test-80k matcher"
    done
fi
cmp "$out/reference.img" "$out/data.img" || fail 'read-only DIR changed disk'
fsck.fat -n "$out/data.img" > "$out/fsck.log" 2>&1 || fail 'dosfstools found damage'
printf 'GREEN dir-match-%s: *.TXT / A*.* / FOO?.* == mdir + test-80k matcher; identical image, fsck.fat clean\n' "$mode"
