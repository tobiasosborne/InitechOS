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
    if grep -Fxiq '::/AUTOEXEC.BAT' "$target.workspace"; then
        # Transfer to the same A: scenario after the primary starts on C:.
        printf '%s\r\n' '@echo off' 'A:' 'A:\AUTOEXEC.BAT' > "$target.startup"
        mcopy -o -i "$volume" "$target.startup" ::AUTOEXEC.BAT
    elif grep -Fxiq '::/AUTOEXEC.BAT' "$target.files"; then
        # Preserve the no-AUTOEXEC case for the DATE/TIME startup replay.
        mdel -i "$volume" ::AUTOEXEC.BAT
    fi
)
