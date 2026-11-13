# LOCKED B11 / initech-69mz keyboard trace (Rule 8). Deliberately added.
# Ref: Lotus 1-2-3 R2.2 Reference Manual pp. 1-9..1-11 (PDF 25..27) and
# 1-23; 123.RI 0x5a5a "Enter address to go to: "; the REAL R2.2 under
# DOSBox-X, fixtures/i123-keys/keys*.steps (02_f2, 03/04 Esc, 06..08 F5 Z100,
# 12_pgdn, 16_tab). Launch geometry: flair_i123_traces.mk.
# Scene: A1 "Hello" Enter; F2 shows "'Hello" in EDIT; Esc erases, Esc READY
# (A1 unchanged); F5 z100 Enter puts Z100 at the window's upper-left; Home;
# PgDn lands on A20 (the native window shows 19 rows; R2.2's 20-row screen
# gives A21, 12_pgdn); Home; Tab lands on I1 with column I first (16_tab).
# W tokens are completion handshakes on rendered text (no commas allowed;
# the harness trims each token's edge spaces), never timeout allowances.
FLAIR_I123_KEYS := $(FLAIR_I123_FILE_LAUNCH),\
kshift-h,ke,kl,kl,ko,kret,WA1: 'Hello,\
kf2,W'Hello_,kesc,kesc,\
kf5,WEnter address to go to: A1_,kz,k1,k0,k0,kret,WZ100:,\
khome,kpgdn,WA20:,\
khome,ktab,WI1:
