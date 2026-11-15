# WP5.1 F6/F10/F7; MINT06 pins Bold and dataloss pins Original Backup.
# Pace user input so the cooperative guest can finish painting between keys.
word_keys() {
    for word_key in $(printf '%s' "$1" | tr ',' ' '); do
        inject "$word_key,w150"
    done
}
journey() {
    launch_app WORD.EXE
    word_keys 'ko,kf,kf,ki,kc,ke,kspc,kf6,kr,ke,kp,ko,kr,kt,ks,kf6,kspc,ka,kr,ke,kspc,kr,ke,ka,kd,ky,kdot'
    capture word
    word_keys 'kf10'
    await_marker 's="Document to be Saved:'
    word_keys 'kw,ko,kr,kd,kdot,kw,kp,k5,kret'
    await_marker 's="A:\WORD.WP5'
    capture word-saved
    word_keys 'kshift-f1'
    await_marker 's="Setup '
    word_keys 'k3'
    await_marker 's="Setup: Environment'
    word_keys 'k1'
    await_marker 's="Setup: Backup'
    word_keys 'k2'
    await_marker 's="Original Document Backup?'
    word_keys 'ky'
    await_marker 's="2 Original Document Backup  Yes'
    capture word-backup-yes
    word_keys 'kf7,kf7,kf7'
    inject 'w300'
    word_keys 'kspc,kr,ke,kv,k1,kf10,kret'
    await_marker 's="Replace A:\WORD.WP5? No (Yes)'
    word_keys 'ky'
    # The shared helper's FILE_REPLACE (gate 0x8b) commits the new version.
    await_marker 'ax=0x008b path="WORD.$$$" new="WORD.WP5" -> 0'
    capture word-second-save
    word_keys 'kspc,kr,ke,kv,k2,kf10,kret'
    await_marker 's="Replace A:\WORD.WP5? No (Yes)'
    word_keys 'ky'
    await_marker 'ax=0x008b path="WORD.$$$" new="WORD.WP5" -> 0'
    capture word-third-save
    word_keys 'kf7,kn,kn,kshift-f10'
    await_marker 's="Document to be Retrieved:'
    word_keys 'kw,ko,kr,kd,kdot,kb,kk,kshift-1,kret'
    await_marker 's="A:\WORD.BK!'
    capture word-retrieved-backup
    # Clear the edit screen, find the independently checked primary by its
    # real List Files Name Search route, retrieve and save an exact copy.
    word_keys 'kf7,kn,kn,kf5,kret,kn,kw,ko,kr,kd,kdot,kw,kp,k5,kret,k1'
    await_marker 's="A:\WORD.WP5'
    # WP5.1 prefix0x103 records whether the cursor precedes EOF (MINT17/19).
    # Put it at the same EOF position as the primary before byte comparison.
    word_keys 'khome,khome,kdown,kf10,ks,ke,ka,kr,kc,kh,kdot,kw,kp,k5,kret'
    await_marker 's="A:\SEARCH.WP5'
    restore_pointer
    word_keys 'kf7,kn'
    # Completion follows this key; no trailing wait may remain for Bochs.
    inject 'ky'
    await_marker 'TENANT-EXIT rc=0 via=exit'
    # Finder's retained Get Info is a file query, including an EXE as a file.
    word_keys 'kup,kw,ko,kr,kd,kdot,ke,kx,ke,kctrl-i'
    await_marker 'FINDER-INFO Name: WORD.EXE'
    capture finder-info
    if [ "$emulator" = qemu ]; then word_keys kesc; else word_keys kret; fi
    # Copy a literal CR fixture from Note Pad page3 of the fresh product disk.
    # Restore page1 so the Note Pad/Calculator journeys retain their start.
    launch_app NOTEPAD.EXE
    move_by 58 167
    move_by 285 0
    inject 'l1,l0'
    await_marker 's="2 of 8"'
    inject 'l1,l0'
    await_marker 's="3 of 8"'
    move_by -343 -167
    clip_y=4
    for clip_line in first second third fourth fifth sixth seventh uniquetail; do
        clip_prefix=
        clip_pending=0
        # Bound input to two characters per paint acknowledgement. This drains
        # the cooperative queue and fits the frontend's distinct-wait capacity.
        for letter in $(printf '%s\n' "$clip_line" | fold -w1); do
            clip_prefix=$clip_prefix$letter
            word_keys "k$letter"
            clip_pending=$((clip_pending+1))
            if [ "$clip_pending" -eq 2 ] || [ "$clip_prefix" = "$clip_line" ]; then
                await_marker "TENANT-GATE ax=0x0030 win=1 x=5 y=$clip_y s=\"$clip_prefix\""
                clip_pending=0
            fi
        done
        [ "$clip_line" = uniquetail ] || word_keys kret
        clip_y=$((clip_y+11))
    done
    word_keys kctrl-a
    await_marker 'TENANT-GATE ax=0x0030 win=1 x=5 y=81 s="uniquetail"'
    word_keys kctrl-c
    await_marker 'TENANT-SCRAP put n=56'
    word_keys kctrl-x
    await_marker 'TENANT-SCRAP put n=56'
    word_keys kctrl-q
    await_marker 'TENANT-EXIT rc=0 via=exit'
    move_by -17 -93
    inject 'l1,m0:38,l0'
    move_by 17 55
    await_marker 'FINDER-CLIPBOARD first'
    capture finder-clipboard-first
    move_by 281 66
    inject 'l1,l0'
    await_marker 'FINDER-CLIPBOARD fourth'
    inject 'l1,l0'
    await_marker 'FINDER-CLIPBOARD uniquetail'
    capture finder-clipboard-last
    # Last page's second button is Previous. Escape always means Close.
    inject 'l1,l0'
    await_marker 'FINDER-CLIPBOARD fourth'
    if [ "$emulator" = qemu ]; then word_keys kesc; else word_keys kret; fi
    move_by -281 -66
    launch_app NOTEPAD.EXE
    word_keys kctrl-v
    inject w500
    move_by 58 167
    inject 'l1,l0,w500,l1,l0,w500'
    move_by -58 -167
    word_keys kctrl-q
    await_marker 'TENANT-EXIT rc=0 via=exit'
}
journey_exits=3
