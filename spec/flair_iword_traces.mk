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

# 5. FLAIR_IWORD_FORMAT_SPEC -- test-flair-iword-format and the iword_format
#    clip (bead initech-jojk, IWORD P2 slice 3; added 2026-10-16, a new locked
#    trace, Rule 8): launch (A, B, CWORD + double-click), then the keys of
#    ../wordperfect51-decomp mint/wpMINT32.steps, the document the real WP.EXE
#    saved as MINT32 (re/mint-results-004.md): Return, Shift-F8 1 (Format:
#    Line) 6 "2" Enter (Line Spacing 2) 3 2 (Justification Center) F7,
#    "Centered.", Return, "Bold me.", Alt-F4 (Block on), Home, Left (WP's Home
#    prefix: the line's start), F6 (the block bold), F10 "fmt" Enter (saved),
#    Alt-F3 (Reveal Codes). "khome" is the Home key (harness/emu/qemu.c).
#    59 tokens.
FLAIR_IWORD_FORMAT_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m78:10,m78:10,l1,l0,l1,l0,kret,kshift-f8,k1,k6,k2,kret,k3,k2,kf7,kshift-c,ke,kn,kt,ke,kr,ke,kd,kdot,kret,kshift-b,ko,kl,kd,kspc,km,ke,kdot,kalt-f4,khome,kleft,kf6,kf10,kf,km,kt,kret,kalt-f3

# P3 first RED: F9 must dispatch to the period-grayed "End Field".
# Reference printed pp. 963-966, Template pp. 5-6. No document/file edits.
FLAIR_IWORD_LAUNCH := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m78:10,m78:10,l1,l0,l1,l0
# All 48 slots; F1 backs out of prompts, F7 n n clears the document.
# F11/F12 aliases are also driven with Block on. Final F9: pixel waypoint.
# RULE 8, 2026-10-17 (bead initech-8q6j, IWORD P4, deliberate): Shift-F7
# Print, Alt-F7 Columns/Table and Ctrl-F7 Footnote are live screens / menus
# now, so each is left with F1 Cancel ("Back out of a WordPerfect menu",
# Reference p. 55) before the next key --
# the three "kf1" after them are the only change.
FLAIR_IWORD_FKEYS_SPEC := $(FLAIR_IWORD_LAUNCH),kf1,kf1,kf2,kf1,kf3,kf4,kf5,kf6,kf7,kn,kn,kf8,kf9,kf10,kf1,kf11,kf11,kf12,kf12,kshift-f1,kf1,kshift-f2,kf1,kshift-f3,kshift-f4,kshift-f5,kshift-f6,kshift-f7,kf1,kshift-f8,kf1,kshift-f9,kshift-f10,kf1,kshift-f11,kshift-f12,kalt-f1,kalt-f2,kf1,kalt-f3,kalt-f3,kalt-f4,kf1,kalt-f5,kalt-f6,kalt-f7,kf1,kalt-f8,kalt-f9,kalt-f10,kalt-f11,kalt-f12,kctrl-f1,kctrl-f2,kctrl-f3,kctrl-f4,kf1,kctrl-f5,kctrl-f6,kctrl-f7,kf1,kctrl-f8,w200,kf1,w150,kctrl-f9,kctrl-f10,kctrl-f11,kctrl-f12,kf9

# P3 search oracle first: Search forward for the Bold On code, backward
# for 'plain', then fail a search for 'missing'. No Save, no disk writes.
# Reference printed pp. 571-573; code bytes from existing MINT06/MINT08.
FLAIR_IWORD_SEARCH_SPEC := $(FLAIR_IWORD_LAUNCH),kp,kl,ka,ki,kn,kspc,kf6,kb,ko,kl,kd,kf6,kspc,ke,kn,kd,khome,khome,kup,kf2,kf6,kf2,kshift-f2,kp,kl,ka,ki,kn,kf2,kf2,km,ki,ks,ks,ki,kn,kg,kf2

# Part 2: Reference pp. 551-553. Replace-all then No/Yes confirmation.
FLAIR_IWORD_REPLACE_SPEC := $(FLAIR_IWORD_LAUNCH),kshift-d,ko,kg,kspc,kd,ko,kg,khome,khome,kup,kalt-f2,kn,kd,ko,kg,kf2,kc,ka,kt,kf2,kspc,khome,khome,kup,kalt-f2,ky,kc,ka,kt,kf2,kf,ko,kx,kf2,kn,ky,kf1,kf6,kf6,khome,khome,kup,kalt-f2,kn,kf6,kf6,kf2,kf2,kf1,kf10,kr,ke,kp,kl,ka,kc,ke,kret
# Reference pp. 697-698: four groups, third retained deletion, preview, Restore.
FLAIR_IWORD_UNDO_SPEC := $(FLAIR_IWORD_LAUNCH),ka,kb,kc,kd,kbspc,kleft,kright,kbspc,kleft,kright,kbspc,kleft,kright,kbspc,kf1,k2,k2,k1,kf10,ku,kn,kd,ko,kret,kf7,kn,kn,ka,kb,kc,kd,kbspc,kshift-f2,ka,kf2,kf2,kb,kc,kf2,kbspc,kf1,k1,kf10,km,ko,kt,ki,ko,kn,kret
# Setup choices persist. Exit WORD cleanly and re-open the same Finder icon.
FLAIR_IWORD_SETUP_SPEC := $(FLAIR_IWORD_LAUNCH),kshift-f1,k3,k8,k1,kc,k2,kp,kf7,k1,k2,ky,kf7,kf7,kf7,kf7,kn,ky,w200,l1,l0,l1,l0,kshift-f1,k3,k8,kf7,k1

# Short Rule-14 demonstrations; the full acceptance traces above also cover
# persistence and boundary cases. Keeping a clip below a minute avoids a
# wall-injection/RTC minute-boundary race without masking clock pixels or
# relaxing record-flair-repro's byte equality. Reference pp. 552, 697, 698-699.
FLAIR_IWORD_REPLACE_CLIP_SPEC := $(FLAIR_IWORD_LAUNCH),kf6,kf6,kk,ke,ke,kp,khome,khome,khome,kup,kalt-f2,ky,kf6,kf6,kf2,kf2,ky,kf1
FLAIR_IWORD_UNDO_CLIP_SPEC := $(FLAIR_IWORD_LAUNCH),ka,kb,kc,kd,kbspc,kleft,kright,kbspc,kleft,kright,kbspc,kleft,kright,kbspc,kf1,k2,k2,k1
FLAIR_IWORD_SETUP_CLIP_SPEC := $(FLAIR_IWORD_LAUNCH),kshift-f1,k3,k1,k2,ky,kf7,k8,k1,kc,k2,kp

# ---- IWORD P4 (bead initech-8q6j; added 2026-10-17, new locked traces, Rule 8)
# Each types the keys of a real WordPerfect 5.1 mint session (../wordperfect51-
# decomp mint/wpMINTnn.steps; ledger re/mint-results-005.md) after the launch,
# saves it with F10 and opens Reveal Codes at the top (Home Home Home Up,
# Alt-F3). "kctrl-b" is Ctrl-B (the page number), "kshift-tab" Shift-Tab.
# 1. headers / footers: mint/wpMINT34.steps -- Shift-F8 2 3 1 2 "Initech" F7,
#    4 1 2 "Page " Ctrl-B F7, F7, "One." Ctrl-Enter "Two." -- F10 "hf".
FLAIR_IWORD_HF_SPEC := $(FLAIR_IWORD_LAUNCH),kshift-f8,k2,k3,k1,k2,kshift-i,kn,ki,kt,ke,kc,kh,kf7,k4,k1,k2,kshift-p,ka,kg,ke,kspc,kctrl-b,kf7,kf7,kshift-o,kn,ke,kdot,kctrl-ret,kshift-t,kw,ko,kdot,kf10,kh,kf,kret,khome,khome,khome,kup,kalt-f3
# 2. footnotes / endnotes: mint/wpMINT36.steps -- "Alpha" Ctrl-F7 1 1 "First
#    note." F7 " beta" Ctrl-F7 1 1 "Second note." F7 " gamma" Ctrl-F7 2 1 "End
#    note." F7 "." -- F10 "notes".
FLAIR_IWORD_NOTES_SPEC := $(FLAIR_IWORD_LAUNCH),kshift-a,kl,kp,kh,ka,kctrl-f7,k1,k1,kshift-f,ki,kr,ks,kt,kspc,kn,ko,kt,ke,kdot,kf7,kspc,kb,ke,kt,ka,kctrl-f7,k1,k1,kshift-s,ke,kc,ko,kn,kd,kspc,kn,ko,kt,ke,kdot,kf7,kspc,kg,ka,km,km,ka,kctrl-f7,k2,k1,kshift-e,kn,kd,kspc,kn,ko,kt,ke,kdot,kf7,kdot,kf10,kn,ko,kt,ke,ks,kret,khome,khome,khome,kup,kalt-f3
# 3. newspaper columns: mint/wpMINT37.steps -- Alt-F7 1 3 F7 1, "The quick
#    brown fox jumps over the lazy dog." Ctrl-Enter "Second column." Alt-F7 1 2
#    "After." -- F10 "cols".
FLAIR_IWORD_COLS_SPEC := $(FLAIR_IWORD_LAUNCH),kalt-f7,k1,k3,kf7,k1,kshift-t,kh,ke,kspc,kq,ku,ki,kc,kk,kspc,kb,kr,ko,kw,kn,kspc,kf,ko,kx,kspc,kj,ku,km,kp,ks,kspc,ko,kv,ke,kr,kspc,kt,kh,ke,kspc,kl,ka,kz,ky,kspc,kd,ko,kg,kdot,kctrl-ret,kshift-s,ke,kc,ko,kn,kd,kspc,kc,ko,kl,ku,km,kn,kdot,kalt-f7,k1,k2,kshift-a,kf,kt,ke,kr,kdot,kf10,kc,ko,kl,ks,kret,khome,khome,khome,kup,kalt-f3
# 4. tables: mint/wpMINT38.steps -- Alt-F7 2 1 "3" Enter "2" Enter F7, A1 Tab
#    B1 Tab C1 Tab A2 Tab B2 Tab C2, Home Home Down "After." -- F10 "tbl".
FLAIR_IWORD_TABLE_SPEC := $(FLAIR_IWORD_LAUNCH),kalt-f7,k2,k1,k3,kret,k2,kret,kf7,kshift-a,k1,ktab,kshift-b,k1,ktab,kshift-c,k1,ktab,kshift-a,k2,ktab,kshift-b,k2,ktab,kshift-c,k2,khome,khome,kdown,kshift-a,kf,kt,ke,kr,kdot,kf10,kt,kb,kl,kret,khome,khome,khome,kup,kalt-f3
# 5. print: the keys of mint/wpMINT34.steps (the document), then those of
#    mint/wpPRINT1.steps -- Shift-F7 S 3 2 8 "PRINT.TXT" Enter F7 F7 -- and 1
#    Full Document (mint/print_one.sh); then the port back to 1 LPT 1 and 1
#    again: the device -- PC LOAD LETTER.
FLAIR_IWORD_PRINT_SPEC := $(FLAIR_IWORD_LAUNCH),kshift-f8,k2,k3,k1,k2,kshift-i,kn,ki,kt,ke,kc,kh,kf7,k4,k1,k2,kshift-p,ka,kg,ke,kspc,kctrl-b,kf7,kf7,kshift-o,kn,ke,kdot,kctrl-ret,kshift-t,kw,ko,kdot,kshift-f7,ks,k3,k2,k8,kshift-p,kshift-r,kshift-i,kshift-n,kshift-t,kdot,kshift-t,kshift-x,kshift-t,kret,kf7,kf7,k1,kshift-f7,ks,k3,k2,k1,kf7,kf7,k1
