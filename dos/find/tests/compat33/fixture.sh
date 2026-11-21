# Sourced by the genuine capture and the native session.
find_fixture() {
    printf 'apple\r\nbanana\r\nAPPLE pie\r\n' > "$out/WORDS.TXT"
    printf 'pear\r\napple tart\r\napple pie\r\n' > "$out/OTHER.TXT"
    printf 'a"b\r\nab\r\na""b\r\n' > "$out/QUOTES.TXT"
    awk 'BEGIN {for (i=0; i<18000; ++i) printf "no\r\n"; printf "apple\r\n"}' > "$out/LATE.TXT"
    printf 'apple LF\napple CRLF\r\napple EOF' > "$out/TEXT.TXT"
    printf 'apple seen\r\n\032apple hidden\r\n' > "$out/CTRLZ.TXT"
    awk 'BEGIN {for (i=0; i<1365; ++i) printf "x\r\n"; printf "apple\r\n"}' > "$out/CROSS.TXT"
    mcopy -i "$out/data.img" "$out/WORDS.TXT" "$out/OTHER.TXT" "$out/QUOTES.TXT" "$out/LATE.TXT" "$out/TEXT.TXT" "$out/CTRLZ.TXT" "$out/CROSS.TXT" ::
    mmd -i "$out/data.img" ::SUB
    mcopy -i "$out/data.img" "$out/WORDS.TXT" ::SUB/WORDS.TXT
}

find_batches() {
    # Keep each group below the product shell's 4 KiB batch-file limit.
    for part in 1 2 3; do
        printf '%s\r\n' '@echo off' > "$out/CASES$part.BAT"
    done
    index=0
    while IFS='|' read -r name input args; do
        part=$((index / 9 + 1))
        index=$((index + 1))
        [ -z "${FIND_CASE:-}" ] || [ "$FIND_CASE" = "$name" ] || continue
        command="FIND <$input >$name.OUT $args"
        [ "$1" != reference ] || command="EXECUTE <$input >$name.OUT $name $args"
        printf '%s\r\n' "echo BEGIN-$name" "$command" \
            "if errorlevel 3 goto FAIL" "if errorlevel 2 goto 2$name" \
            "if errorlevel 1 goto 1$name" "echo 0>$name.RC" "goto E$name" \
            ":2$name" "echo 2>$name.RC" "goto E$name" \
            ":1$name" "echo 1>$name.RC" ":E$name" "echo END-$name" >> "$out/CASES$part.BAT"
    done < "$case_dir/cases"
    for part in 1 2 3; do
        printf '%s\r\n' 'goto DONE' ':FAIL' 'echo FIND-PROBE-FAIL' ':DONE' >> "$out/CASES$part.BAT"
        mcopy -i "$out/data.img" "$out/CASES$part.BAT" ::
    done
}

call_find_batches() {
    for part in 1 2 3; do
        printf '%s\r\n' "call CASES$part.BAT"
    done
}
