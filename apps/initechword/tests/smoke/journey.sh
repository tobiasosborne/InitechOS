# F6/F10/F7 are real WP5.1 commands;
# tests/test_iword_face.c F7 and the MINT06 capture pin the Bold pair.
journey() {
    launch_app WORD.EXE
    inject 'ko,kf,kf,ki,kc,ke,kspc,kf6,kr,ke,kp,ko,kr,kt,ks,kf6,kspc,ka,kr,ke,kspc,kr,ke,ka,kd,ky,kdot'
    capture word
    inject 'kf10,kw,ko,kr,kd,kdot,kw,kp,k5,kret,w300'
    capture word-saved
    # Restore the common pointer before the final exit event.
    restore_pointer
    inject 'kf7,kn,ky'
    await_marker 'TENANT-EXIT rc=0 via=exit'
}
