# Prepare a disposable master for a shell test with its own A: fixture.
prepare_session_boot() (
    source=$1
    data=$2
    target=$3
    cp "$source" "$target"
    volume=$target@@32256
    mdir -a -b -i "$volume" :: > "$target.files"
    # The two-file master starts from A: until media cutover at step 20.
    grep -Fxiq '::/COMMAND.COM' "$target.files" || exit 0

    mdir -a -b -i "$data" :: > "$target.workspace"
    # A broken startup can still complete transport from the C: prompt.
    if grep -Fxiq '::/DONE.COM' "$target.workspace"; then
        mcopy -o -i "$data" ::DONE.COM "$target.probe"
        mcopy -o -i "$volume" "$target.probe" ::DONE.COM
    fi
    if grep -Fxiq '::/AUTOEXEC.BAT' "$target.workspace"; then
        # Transfer to the same A: scenario after the primary starts on C:.
        printf '%s\r\n' '@echo off' 'A:' 'A:\AUTOEXEC.BAT' > "$target.startup"
        mcopy -o -i "$volume" "$target.startup" ::AUTOEXEC.BAT
    elif grep -Fxiq '::/AUTOEXEC.BAT' "$target.files"; then
        # Preserve the no-AUTOEXEC case for the DATE/TIME startup replay.
        mdel -i "$volume" ::AUTOEXEC.BAT
    fi
)

session_report_ok() {
    grep -Fq 'timed_out=0' "$1" || return 1
    if grep -Fxq '[harness] OK=1' "$1"; then
        grep -Fq 'triple_fault=0 cpu_reset=2 guest_errors=0' "$1"
    else
        grep -Fxq '[bochs] OK=1' "$1" &&
        grep -Eq '^\[bochs\] launched=1 rfb_unblocked=1 timed_out=0( |$)' "$1" &&
        grep -Fxq '[bochs] triple_fault=0' "$1"
    fi
}
