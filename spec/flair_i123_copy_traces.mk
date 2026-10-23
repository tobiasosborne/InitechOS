# Deliberately added B11 replay. Independent real R2.2 screens/WK1 files:
# fixtures/i123-copy (Reference pp.3-4/5, pp.1-24/25). No old trace changed.
# A1=7 -> D1..D3, save SINGLE; then exercise automatic anchor, period,
# arrows, Esc back to anchor, Backspace back to POINT entry; end A1..B2.
FLAIR_I123_COPY := $(FLAIR_I123_FILE_LAUNCH),WA1:,\
k7,kret,kslash,kc,ka,k1,kret,kd,k1,kdot,kdot,kd,k3,kret,\
kslash,kf,ks,ks,ki,kn,kg,kl,ke,kret,\
kslash,kc,kesc,kright,kdot,kdown,kright,kesc,\
kdot,kdown,kright,kbspc,kdot,kdown,kright
