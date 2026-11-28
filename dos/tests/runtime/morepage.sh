#!/bin/sh
# Observe the booted console at the same interaction boundaries as DOS 3.30.
set -eu
root=${1:?root} boot=${2:?boot} programs=${3:?programs} out=${4:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
rm -f "$out/transport.ok" "$out/assertion" "$out/result"
. "$root/dos/tests/runtime/verdict.sh"
. "$root/dos/tests/runtime/utility-session.sh"
case_dir=$root/dos/more/tests/compat33
. "$case_dir/fixture.sh"
export LC_ALL=C TZ=UTC INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
export INITECH_TEXT_SCREEN_ELF="$root/build/system/system.elf"
export INITECH_TEXT_SCREEN_TOOL="$root/build/apps/initechbase/host/screen_capture"
make --no-print-directory -s -C apps/initechbase native-tools > "$out/tools.log" 2>&1 ||
    error 'cannot build the console capture tool'
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
more_fixture
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/DONE.COM" "$programs/MORE.COM" ::
printf '%s\r\n' '@echo off' 'MORE <PAGES.TXT >COPY.OUT' 'TYPE PAGES.TXT | MORE | MORE >PIPE.OUT' \
    'MORE /Z <SHORT.TXT >BAD.OUT' 'IF ERRORLEVEL 1 ECHO MORE-REFUSED' \
    'PROMPT P$G' 'ECHO PAGER-READY' > "$out/AUTOEXEC.BAT"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT

type_keys() { printf '%s\n' "$1" | "$root/build/factory/keys"; }
mouse_keys() { type_keys "$1" | sed 's/^/k/; s/,/,k/g'; }
cpu=${DOS_UTILITY_EMULATOR:-qemu}
case $cpu in
 qemu)
    # Gate on the numbered content so a broken cue reaches the assertion.
    spec="$(mouse_keys 'more<pages.txt'),WPAGE-L24,w350,D$out/first.ppm,w400,D$out/still.ppm"
    spec="$spec,kret,WPAGE-L47,w350,D$out/second.ppm,kx,WPAGE-L60,w350,D$out/after.ppm"
    spec="$spec,$(mouse_keys 'more<stop.txt'),WSTOP-L24,w350,D$out/cancel-wait.ppm"
    spec="$spec,kctrl-c,w400,D$out/cancelled.ppm,$(mouse_keys done)"
    set -- --mouse "$spec";;
 bochs)
    # The RFB key parser has no '<' token; run the same two commands via
    # one-line batches on A: and observe the same console states.
    printf '%s\r\n' '@MORE <PAGES.TXT' > "$out/PAGER.BAT"
    printf '%s\r\n' '@MORE <STOP.TXT' > "$out/STOPPER.BAT"
    mcopy -i "$out/data.img" "$out/PAGER.BAT" "$out/STOPPER.BAT" ::
    make --no-print-directory -s -f factory/desktop-tools.mk "$root/build/factory/desktop-bochs" \
        >> "$out/tools.log" 2>&1 || error 'cannot build the ordered Bochs driver'
    export DOS_UTILITY_ORDERED=1 DESKTOP_SERIAL="$out/morepage.serial" DESKTOP_WAIT_MS=30000
    spec="$(type_keys pager),WPAGE-L24,w350,Tfirst,w400,Tstill,ret,WPAGE-L47,w350,Tsecond,x,WPAGE-L60,w350,Tafter"
    spec="$spec,$(type_keys stopper),WSTOP-L24,w350,Tcancel-wait,ctrl-c,w500,Tcancelled,$(type_keys done)"
    set -- --keys "$spec";;
 *) error "unknown emulator $cpu";;
esac
utility_session "$boot" "$out/data.img" "$out" \
    --keys-after PAGER-READY "$@" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE \
    --timeout-ms 45000 --name morepage > "$out/driver.log" 2> "$out/report" ||
    error 'MORE console transport failed'
utility_transport "$out/report"

for name in first still second after cancel-wait cancelled; do
    case $cpu in qemu) screen=$out/$name.ppm.screen;; bochs) screen=$out/$name.screen;; esac
    [ -s "$screen" ] || error "no $name console capture"
    if [ "$cpu" = qemu ]; then
        sed -n '3,27{s/ *$//;p;}' "$screen" > "$out/$name.txt"
    else
        # Bochs T returns character/attribute pairs, followed by the cursor.
        od -An -v -tu1 -N4000 "$screen" |
            awk '{for(i=1;i<=NF;i++){if(n%2==0) printf "%c",$i; if(++n%160==0) print ""}}' |
            sed 's/ *$//' > "$out/$name.txt"
    fi
done
require 'MORE shows the first captured cue' cmp "$case_dir/expected/first.txt" "$out/first.txt"
require 'MORE waits for a key without advancing the screen' cmp "$out/first.txt" "$out/still.txt"
require 'MORE continues and displays the second captured cue' cmp "$case_dir/expected/second.txt" "$out/second.txt"
# Compare the pager-owned content. COMMAND can add a blank before P>.
for side in "$case_dir/expected" "$out"; do
    stem=expected
    [ "$side" != "$out" ] || stem=actual
    sed -n "/^PAGE-L47$/,/^PAGE-L60$/p" "$side/after.txt" > "$out/$stem-after"
    sed -n '/^STOP-L20$/,$p' "$side/cancelled.txt" | sed '/^$/d' > "$out/$stem-cancelled"
done
require 'MORE advances past the cue and echoes the continuation key' \
    cmp "$out/expected-after" "$out/actual-after"
require 'MORE returns to the shell when the file ends' grep -Fx 'P>' "$out/after.txt"
require 'MORE shows the cue before Ctrl-C' cmp "$case_dir/expected/cancel-wait.txt" "$out/cancel-wait.txt"
require 'MORE Ctrl-C returns to the prompt without publishing further lines' \
    cmp "$out/expected-cancelled" "$out/actual-cancelled"

for name in COPY PIPE; do
    mcopy -o -i "$out/data.img" "::$name.OUT" "$out/$name.OUT" || error "missing $name output"
    require "MORE $name output is byte-exact" cmp "$out/PAGES.TXT" "$out/$name.OUT"
done
# DOS 3.30 MOREZ ignores /Z. GOLDEN-MASTER item 2 calls
# for an explicit refusal.
mtype -i "$out/data.img" ::BAD.OUT > "$out/refusal" || error 'MORE switch output missing'
printf 'Invalid parameter\r\n' > "$out/refusal-expected"
require 'MORE explicitly refuses unsupported switches' cmp "$out/refusal-expected" "$out/refusal"
require 'MORE rejects /Z with nonzero exit' grep -F 'MORE-REFUSED' "$out/morepage.serial"
echo 'GREEN MORE waits, continuation, Ctrl-C and byte-exact file and pipe output'
