# spec/flair_iword_traces.mk -- the LOCKED input traces of test-flair-iword
# (bead initech-l4tj, IWORD P1: InitechWord opens from the Finder as a disk
# application, takes a paragraph that wraps, saves it with F10, clears the
# screen through F7 and retrieves it with Shift-F10). Locked spec-data
# (CLAUDE.md Rule 8, Rule 11); bound by every convention of
# spec/flair_app_launch_traces.mk (cursor starts (320,240); "m<dx>:<dy>"
# relative, y down, int8-safe; "l1"/"l0" left button; "k<chord>" one
# keystroke, "kshift-h" a capital H, "kshift-f10" Shift-F10).
#
# THE SCENE: $(FLAIRTENANTS_IMG)'s desktop with the pump life widened
# ($(FLAIR_IWORD_GATE_IMG), FLAIR_TEN_TICK_BUDGET=1500: 114 tokens at ~40 ms
# outlive the 250-tick default) + a gate-local copy of $(FLAIR_DATA_IMG),
# whose APPS folder holds, in directory order, TENANTFX.EXE (grid cell 0),
# CTENANT.EXE (1), 123.EXE (2) and WORD.EXE (3).
#
# GEOMETRY (derived, never measured off a render):
#   APPS window content.left 41, content.top 102 (flair_app_launch_traces.mk);
#   grid cell 3: sprite_x = 41 + 18 + 68*3 = 263, sprite_y = 106
#     => WORD.EXE sprite (263,106)..(295,138), centre (279,122)
#     (finder_win_grid_cols: the 342-px content holds 4 columns, so cell 3
#     is the fourth icon of the first row)
#   the WORD window frame (0,40)..(640,480) (face_a.c WIN_*), content
#     (1,62)..(619,459); Face A cell (col c, line L) at content-local
#     (1 + 8c, 16L) -> GLOBAL (2 + 8c, 62 + 16L); the status line is line 23
#     (y 430..446).
#
# WAYPOINTS: A, B as flair_app_launch_traces.mk (the volume, then APPS), then
#   CWORD (123,102) -> WORD.EXE (279,122): m78:10,m78:10
#
# 1. FLAIR_IWORD_SPEC -- launch (A, B, CWORD + double-click), then type the
#    paragraph "Initech word processing is here. The quick brown fox jumps over the lazy dog."
#    (77 characters: the line breaks after "over", 63 columns, and the
#    space there becomes WordPerfect's soft return), F10 (Save) "memo"
#    Enter, F7 (Exit) n ("Save Document?" No) n ("Exit WP?" No: a clear
#    screen), Shift-F10 (Retrieve) "memo" Enter: the paragraph comes back.
#    Prompts: WordPerfect 5.1 Reference Manual PDF 213 / 38 / 137 (face_a.c).
#    114 tokens.
FLAIR_IWORD_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m78:10,m78:10,l1,l0,l1,l0,kshift-i,kn,ki,kt,ke,kc,kh,kspc,kw,ko,kr,kd,kspc,kp,kr,ko,kc,ke,ks,ks,ki,kn,kg,kspc,ki,ks,kspc,kh,ke,kr,ke,kdot,kspc,kshift-t,kh,ke,kspc,kq,ku,ki,kc,kk,kspc,kb,kr,ko,kw,kn,kspc,kf,ko,kx,kspc,kj,ku,km,kp,ks,kspc,ko,kv,ke,kr,kspc,kt,kh,ke,kspc,kl,ka,kz,ky,kspc,kd,ko,kg,kdot,kf10,km,ke,km,ko,kret,kf7,kn,kn,kshift-f10,km,ke,km,ko,kret

# 2. FLAIR_IWORD_TYPE_SPEC -- the record clip's trace (iword_typing, Rule 14):
#    launch and type the paragraph only (the wrap happening). 99 tokens.
FLAIR_IWORD_TYPE_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m78:10,m78:10,l1,l0,l1,l0,kshift-i,kn,ki,kt,ke,kc,kh,kspc,kw,ko,kr,kd,kspc,kp,kr,ko,kc,ke,ks,ks,ki,kn,kg,kspc,ki,ks,kspc,kh,ke,kr,ke,kdot,kspc,kshift-t,kh,ke,kspc,kq,ku,ki,kc,kk,kspc,kb,kr,ko,kw,kn,kspc,kf,ko,kx,kspc,kj,ku,km,kp,ks,kspc,ko,kv,ke,kr,kspc,kt,kh,ke,kspc,kl,ka,kz,ky,kspc,kd,ko,kg,kdot

# 3. FLAIR_IWORD_CODES_SPEC -- test-flair-iword-codes and the iword_codes clip
#    (bead initech-jojk, IWORD P2 slice 1): launch (A, B, CWORD + double-
#    click), type "bold under slant" PLAIN, Enter, then the same three words
#    STYLED with WordPerfect 5.1's own keys -- F6 "bold" F6, space, F8 "under"
#    F8, space, Ctrl-F8 2 4 "slant" Ctrl-F8 2 4 (Font, Appearance, Italc:
#    mint/wpMINT06.steps lines 5-15 of ../wordperfect51-decomp, the keys the
#    real WP.EXE was minted with) -- F10 "codes" Enter (saved), Alt-F3
#    (Reveal Codes). Line 1 is the PLAIN reference the pixel checker
#    synthesizes line 2 from (tools/ppm_iword_codes_check.c). 73 tokens.
FLAIR_IWORD_CODES_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m78:10,m78:10,l1,l0,l1,l0,kb,ko,kl,kd,kspc,ku,kn,kd,ke,kr,kspc,ks,kl,ka,kn,kt,kret,kf6,kb,ko,kl,kd,kf6,kspc,kf8,ku,kn,kd,ke,kr,kf8,kspc,kctrl-f8,k2,k4,ks,kl,ka,kn,kt,kctrl-f8,k2,k4,kf10,kc,ko,kd,ke,ks,kret,kalt-f3

# 4. FLAIR_IWORD_BLOCK_SPEC -- test-flair-iword-block and the iword_block clip
#    (bead initech-jojk, IWORD P2 slice 2): launch (A, B, CWORD + double-
#    click), then the keys of ../wordperfect51-decomp mint/wpMINT20.steps, the
#    document the real WP.EXE saved as MINT20 (re/mint-results-003.md): Return,
#    "Alpha Beta Gamma.", Left x11, Alt-F4 (Block on), Right x5 ("Beta "),
#    Ctrl-F4 1 1 (Move), End, Enter (retrieved), End, Return, "x", F6,
#    Backspace ("Delete [BOLD]? No (Yes)"), y, Tab, "tab", Ctrl-Enter (hard
#    page), "Page two.", F10 "block" Enter (saved), Alt-F3 (Reveal Codes).
#    "kctrl-ret" is Ctrl-Enter, "ktab" the Tab key (harness/emu/qemu.c).
#    90 tokens.
FLAIR_IWORD_BLOCK_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m78:10,m78:10,l1,l0,l1,l0,kret,kshift-a,kl,kp,kh,ka,kspc,kshift-b,ke,kt,ka,kspc,kshift-g,ka,km,km,ka,kdot,kleft,kleft,kleft,kleft,kleft,kleft,kleft,kleft,kleft,kleft,kleft,kalt-f4,kright,kright,kright,kright,kright,kctrl-f4,k1,k1,kend,kret,kend,kret,kx,kf6,kbspc,ky,ktab,kt,ka,kb,kctrl-ret,kshift-p,ka,kg,ke,kspc,kt,kw,ko,kdot,kf10,kb,kl,ko,kc,kk,kret,kalt-f3
