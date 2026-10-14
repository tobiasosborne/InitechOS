# LOCKED initech-uqah P3 traces. Same launch geometry and QMP injection as
# spec/flair_i123_traces.mk. Manual Copy PDF 69..73, Move PDF 191..192.
# Added deliberately for P3; no existing trace altered.
FLAIR_I123_MENU_LAUNCH := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:20,l1,l0,l1,l0
# A1=10, B1=-A1*2, C1=-B1*2; Copy B1..C1 to D1..E1, H1=-D1,
# Move D1..E1 to F1..G1. H1 must follow the moved range (80, not 0).
# End in /Range: both control-panel menu lines and MENU are pixel graded.
FLAIR_I123_MENU_OPS := $(FLAIR_I123_MENU_LAUNCH),k1,k0,kright,kminus,ka,k1,kshift-8,k2,kright,kminus,kb,k1,kshift-8,k2,kret,kslash,kc,kb,k1,kdot,kdot,kc,k1,kret,kd,k1,kret,kright,kright,kright,kright,kright,kminus,kd,k1,kret,kslash,km,kd,k1,kdot,kdot,ke,k1,kret,kf,k1,kret,kslash,kr
# Navigate every top-level branch, nested Worksheet/Global/Recalculation,
# Range/Format/Label/Name, File, Print/Printer/Options/Margins, Graph/Type,
# Data/Sort, Add-In and Quit. Esc unwinds; out-of-phase System refuses.
FLAIR_I123_MENU_TREE := $(FLAIR_I123_MENU_LAUNCH),kslash,kw,kg,kr,kright,kesc,kesc,kesc,kesc,kslash,kr,kf,kright,kesc,kl,kesc,kn,kright,kesc,kesc,kesc,kslash,kf,kright,kesc,kesc,kslash,kp,kp,ko,km,kright,kesc,kesc,kesc,kesc,kesc,kslash,kg,kt,kright,kesc,kesc,kesc,kslash,kd,ks,kright,kesc,kesc,kesc,kslash,ks,kesc,kslash,ka,kright,kesc,kesc,kslash,kq,kright,kesc,kesc,kslash,kright
