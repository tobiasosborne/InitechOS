# Locked R4.4 traces (initech-tdnl.18); relative PS/2 deltas <=100.
# Apple rows: Calculator y50, Note Pad y66, Scrapbook y82. documentProc
# frame(160,80)..(480,360), content(166,102)..(459,339); footer corners
# prev(170,329), next(455,329); shared vertical bar x459..474 y101..339.
# Native DA captures absent: composed chrome is independently PNG-graded.
# Each gate starts on a fresh volume; close/reopen exercises the file path.
FLAIR_NOTEPAD_SPEC := m-100:-100,m-100:-70,m-50:0,l1,l0,w300,m-50:-61,l1,w200,m40:57,l0,w600,ka,kl,kp,kh,ka,w200,kctrl-a,kctrl-c,w200,m100:100,m100:100,m100:63,m95:0,l1,l0,w300,kctrl-v,k2,w200,kctrl-w,w400,m-100:-100,m-100:-100,m-100:-100,m-100:-20,m-35:0,l1,w200,m40:57,l0,w600,m100:100,m100:100,m100:100,m100:14,m40:0,w300
FLAIR_SCRAPBOOK_SPEC := m-100:-100,m-100:-70,m-50:0,l1,l0,w300,m-50:-61,l1,w200,m40:57,l0,w600,ka,kl,kp,kh,ka,w200,kctrl-a,kctrl-c,kctrl-w,w400,m-40:-57,l1,w200,m40:73,l0,w600,kctrl-v,w300,kctrl-v,w300,m100:28,m100:0,m100:0,m100:0,m6:0,l1,l0,w300,kctrl-c,kctrl-w,w400,m-100:-100,m-100:-1,m-100:0,m-100:0,m-46:0,l1,w200,m40:73,l0,w600,m100:100,m100:100,m100:98,m100:0,m40:0,w300
# Round trip: after TEXT copies alpha, close via the existing Don't Save
# button(250,168), reopen Note Pad, paste on page2, append r, close. The FAT
# oracle independently expects page1 alpha and page2 alphar.
# PM MERGE RE-KEY 2026-10-17 (zc68 DOS folder): TEXT.EXE is APPS cell 6 = (+88,+72) from the
# APPS double-click; after the launch the cursor moves (-136,+104) back to the old cell-12
# point (-48,+176) so every later relative gesture is unchanged.
FLAIR_NOTEPAD_ITEXT_SPEC := m-100:-100,m-100:-70,m-50:0,l1,l0,w300,m-50:-61,l1,w200,m40:57,l0,w600,ka,kl,kp,kh,ka,w200,kctrl-a,kctrl-c,kctrl-w,w400,m100:-2,m100:0,m100:0,m100:0,m100:0,m40:0,l1,l0,l1,l0,w200,m-100:38,m-100:0,m-100:0,m-100:0,m-77:0,l1,l0,l1,l0,w200,m88:72,l1,l0,l1,l0,m-100:100,m-36:4,w600,kctrl-v,w200,kctrl-a,kctrl-c,w300,m100:22,m100:0,m100:0,m100:0,m100:0,m5:0,kctrl-w,w500,m-100:-100,m-100:-32,m-100:0,m-30:0,l1,l0,w400,m-100:-100,m-100:-59,m-30:0,l1,w200,m40:57,l0,w600,m100:100,m100:100,m100:63,m95:0,l1,l0,w200,kctrl-v,kr,w200,kctrl-w,w300
FLAIR_NOTEPAD_RESTART_SPEC := m-100:-100,m-100:-70,m-50:0,l1,l0,w300,m-50:-61,l1,w200,m40:57,l0,w600,ka,kl,kp,kh,ka,w200,m100:100,m100:100,m100:63,m95:0,l1,l0,w200,kb,ke,kt,ka,w200,kctrl-w,w400,m-100:-29,m-100:0,m-100:0,m-100:0,m-15:0,l1,l0,w200,m100:-100,m30:-100,m0:-70,l1,m0:79,l0
# Fresh-boot reopen (cursor reset to320,240); used after the restart leg.
FLAIR_NOTEPAD_OPEN_SPEC := m-100:-100,m-100:-70,m-50:0,l1,l0,w300,m-50:-61,l1,w200,m40:57,l0,w600,m100:100,m100:100,m100:100,m100:14,m40:0,w300
