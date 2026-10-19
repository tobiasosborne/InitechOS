# LOCKED B13 / initech-twxf entry trace. Deliberately added, no golden mint.
# Ref: R2.2 Reference Manual pp. 1-16/17: type data, Enter commits; /File
# Save pp. 5-19..22. Launch geometry: flair_i123_traces.mk. Four alphabets
# are a 104-character label in A1; save ENTRY.WK1 for independent wk1_ref.py.
# The brief requires tail scrolling. No long-entry capture is in the corpus;
# manual p. 1-4 places the current entry on control-panel line two but does
# not specify a scroll increment. The expected 75-character tail below is
# the native 76-column viewport with one position reserved for its cursor.
I123_ENTRY_LABEL := abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz
I123_ENTRY_TAIL := defghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz_
# Guest-state barriers keep each burst within the event ring on a loaded
# host. Every W names a distinct rendered prefix/tail, including its cursor;
# these are completion handshakes, not extra wall-clock timeout allowances.
FLAIR_I123_ENTRY := $(FLAIR_I123_FILE_LAUNCH),\
ka,kb,kc,kd,ke,kf,kg,kh,Wabcdefgh_,\
ki,kj,kk,kl,km,kn,ko,kp,Wabcdefghijklmnop_,\
kq,kr,ks,kt,ku,kv,kw,kx,Wabcdefghijklmnopqrstuvwx_,\
ky,kz,ka,kb,kc,kd,ke,kf,Wabcdefghijklmnopqrstuvwxyzabcdef_,\
kg,kh,ki,kj,kk,kl,km,kn,Wabcdefghijklmnopqrstuvwxyzabcdefghijklmn_,\
ko,kp,kq,kr,ks,kt,ku,kv,Wabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuv_,\
kw,kx,ky,kz,ka,kb,kc,kd,Wabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcd_,\
ke,kf,kg,kh,ki,kj,kk,kl,Wabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijkl_,\
km,kn,ko,kp,kq,kr,ks,kt,Wabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrst_,\
ku,kv,kw,kx,ky,kz,ka,kb,Wfghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzab_,\
kc,kd,ke,kf,kg,kh,ki,kj,Wnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghij_,\
kk,kl,km,kn,ko,kp,kq,kr,Wvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqr_,\
ks,kt,ku,kv,kw,kx,ky,kz,kret,kslash,kf,ks,ke,kn,kt,kr,ky,kret
