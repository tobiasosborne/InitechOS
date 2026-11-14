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
    # Word's owned stage publication; removed with the kernel replace
    # primitive (initech-7mb66), which removes this interim save sequence.
    await_marker 'path="WORD.N00" new="WORD.WP5" -> 0'
    capture word-second-save
    word_keys 'kspc,kr,ke,kv,k2,kf10,kret'
    await_marker 's="Replace A:\WORD.WP5? No (Yes)'
    word_keys 'ky'
    await_marker 'path="WORD.N00" new="WORD.WP5" -> 0'
    capture word-third-save
    word_keys 'kf7,kn,kn,kshift-f10'
    await_marker 's="Document to be Retrieved:'
    word_keys 'kw,ko,kr,kd,kdot,kb,kk,kshift-1,kret'
    await_marker 's="A:\WORD.BK!'
    capture word-retrieved-backup
    restore_pointer
    word_keys 'kf7,kn,ky'
    await_marker 'TENANT-EXIT rc=0 via=exit'
}
