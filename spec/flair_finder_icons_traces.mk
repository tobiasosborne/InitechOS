# tdnl.65 re-key: consume root activation before selecting its document.
# spec/flair_finder_icons_traces.mk -- LOCKED injected traces for the Finder
# icon-art emulator gates (beads initech-tdnl.53 / tdnl.74 / tdnl.87;
# docs/worklog/WL-0140). Token grammar: harness/emu/qemu.c (mDX:DY relative
# moves, l1/l0 the left button). The pointer starts at (400,240).
#
# 1. FLAIR_FINDER_ICONS_SPEC -- every icon kind on one screen
#    A. open the volume: (400,240) -> (600,64), double-click
#       (the FLAIR_ICON_OPEN_SPEC prefix, spec/flair_desktop_icons_traces.mk)
#       -> FINDER-OPEN-VOLUME win=0 n=2; root window frame (20,60), README.TXT
#          sprite (44,86), APPS sprite (112,86).
#    B. (600,64) -> APPS centre (123,102): m-100:10 x3, m-100:8, m-77:0
#       (sum -477,+38); double-click -> FINDER-OPEN-FOLDER name=APPS win=1
#       singleton=0, the APPS window cascaded at (40,80).
#    C. (123,102) -> its title band (300,90): m100:-12, m77:0; drag by
#       (+210,+100): l1, m100:50, m100:50, m10:0, l0 -> FLAIR-DRAG win 0
#       (40,80)->(250,180) (the FLAIR_FOLDER_NAV_SPEC leg C/D pattern with a
#       shorter drop, so the window (250,180)..(610,400) stays clear of the
#       desktop Trash sprite (584..615, 404..435)). APPS cell 0
#       (TENANTFX.EXE, an .EXE: the APPLICATION kind) is then at
#       (250+24, 180+26) = (274,206).
#    D. (510,190) -> README.TXT centre (60,102): m-100:-20 x4, m-50:-8
#       (sum -450,-88); one click -> FINDER-WIN-SELECT win=0
#       name=README.TXT count=1 (README selected; APPS unselected; the
#       volume stays selected on the desktop).
#    E. PARK (60,102) -> (620,460): m100:72 x4, m100:70, m60:0 (sum +560,+358).
#    The dump (after the README select) shows: the desktop VOLUME and TRASH,
#    the root window's DOCUMENT (selected) and FOLDER, the APPS window's
#    APPLICATION -- every captured desktop/window kind on one frame.
FLAIR_FINDER_ICONS_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m100:-12,m77:0,l1,m100:50,m100:50,m10:0,l0,m-100:-20,m-100:-20,m-100:-20,m-100:-20,m-50:-8,l1,l0,l1,l0,m100:72,m100:72,m100:72,m100:72,m100:70,m60:0

# 2. FLAIR_TRASH_FULL_SPEC -- stage README.TXT, then park clear of the Trash
#    The FLAIR_EMPTY_ALERT_SPEC staging prefix (spec/flair_trash_traces.mk
#    Sec 3): open the volume, drag README.TXT onto the Trash (600,422),
#    release -> FINDER-TRASH name=README.TXT origin=0, DESK-TRASH-ICON full.
#    PARK (600,422) -> (620,460): m10:19, m10:19 (below the Trash sprite
#    (584..615, 404..435), clear of every graded cell).
FLAIR_TRASH_FULL_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,m10:19,m10:19
