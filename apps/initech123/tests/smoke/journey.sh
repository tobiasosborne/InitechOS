# Legacy-final flair_i123_{traces,file_traces}.mk, unchanged input and oracle.
journey() {
    launch_app 123.EXE
    inject 'k1,k2,k3,kright,k1,kdot,k5,kright,kshift-2,ks,ku,km,kshift-9,ka,k1,kdot,kdot,kb,k1,kshift-0,kret'
    capture sum
    inject 'kslash,kf,ks,ks,kl,ki,kc,ke,kret,w300'
    restore_pointer
    inject kctrl-q
    await_marker 'TENANT-EXIT rc=0 via=exit'
}
