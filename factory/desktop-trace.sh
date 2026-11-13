# Production geometry from legacy-final flair_exit_dos_traces.mk and
# flair_i123_{traces,file_traces}.mk. The new disk has APPS first and 123.EXE
# first within it. No scenario code is linked into the product.
open_disk='m100:-98,m100:-98,m80:0,l1,l0,l1,l0'
open_apps='m-100:8,m-100:8,m-100:8,m-100:8,m-100:8,m-44:-2,l1,l0,l1,l0'
launch_123='m22:20,l1,l0,l1,l0'
# A1=123, B1=1.5, C1=@SUM(A1..B1), then /File Save SLICE.
enter_sum='k1,k2,k3,kright,k1,kdot,k5,kright,kshift-2,ks,ku,km,kshift-9,ka,k1,kdot,kdot,kb,k1,kshift-0,kret'
save_123='kslash,kf,ks,ks,kl,ki,kc,ke,kret'
exit_desktop='m92:-92,l1,m0:100,m0:33,l0,WFINDER-EXIT-DOS-ALERT,kret'
