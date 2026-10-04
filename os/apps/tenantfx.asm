; tenantfx.asm -- TENANTFX.EXE, the disk-shipped FIXTURE TENANT (THE ARTIFACT's
; first disk-launched FLAIR app; bead initech-tdnl.14, GUI-remediation R3.7).
;
; D1.7 names it TENANTFIX.EXE; "TENANTFIX" is 9 characters and is not a legal
; 8.3 base name (mtools would mint a VFAT long name + a TENANT~1 alias), so it
; ships as TENANTFX.EXE. Same fixture, legal name.
;
; What it does (D1.7 verbatim): registers, opens ONE window, draws one text run
; (two lines), and exits on the first mouseDown. It is event-driven -- a named
; eventProc the kernel CALLS with the cooked event already in evbuf, and whose
; RETURN is the cooperative yield (reconciliation DEC-AC3-3; ADR-0013 Sec 3.4).
; It never loops, never calls anything WaitNextEvent-shaped.
;
; Ref:   spec/toolbox_gate.h -- EVERY constant below mirrors it (the vector, the
;          AX codes, the argument order, the FlairTenantRec layout, the flat
;          FlairEvent offsets, the colour tokens). The emu launch-trace golden
;          prints each call's decoded arguments, so a drift between this file
;          and the spec shows up as a golden diff, never silently.
;        docs/design/GUI-remediation-ADR-reconciliation.md Part B DEC-AC3-1..4.
;        ADR-0003-AMENDMENT-DEC-08a (the InitechMZ container mzlink wraps this
;          flat module in).
;
; RELOCATION: assembled TWICE -- org 0 (the payload) and org 0x01010110 -- and
; `mzlink --reloc-diff` derives every absolute dword (the FlairTenantRec
; pointers, `push dword label`, `mov eax,[label]`) from the difference. Nothing
; here is position-dependent except through those dwords, which is exactly what
; lets the kernel load it at any heap address.
;
; SETMBAR (bead initech-cnpm; spec/toolbox_gate.h Sec 9): right after REGISTER
; and BEFORE its window, the entry hands the kernel `mbar`, its own MenuBar
; resource (Apple slot + File / Edit / Fixture), a MenuBar/MenuInfo/MenuItem
; graph whose every pointer is a reloc site. Band 2 shows it from the
; affirmation on, and shows the Finder's bar again after the exit.
;
; MENU CHOICES (bead initech-tdnl.31; spec/toolbox_gate.h Sec 6a): the kernel
; PUSHES a choice made in this tenant's own bar -- by the mouse in band 2 or
; by its command key (Ctrl-Q) -- as one TBX_EVT_MENU event whose Message is
; the MenuSelect/MenuKey word (menuID << 16) | item. event_proc2 is the
; DoMenuCommand (MTE Listing 3-24): File > Quit EXITs; Fixture > About draws
; an About line in the window and replaces the bar AFTER the window exists
; (SETMBAR mbar2, which adds an "Info" menu) followed by DRAWMENUBAR, exactly
; as a Mac application follows SetMenuBar with DrawMenuBar (MTE p. 3-41).
; All of it is APPENDED after hwin, so every earlier offset (rec, mbar,
; bad_nested, evbuf, hwin) is unchanged; only rec's eventProc dword now names
; event_proc2, which falls through to the V1 event_proc for everything else.
;
; Build knobs (fixture VARIANTS for the mutant/crash legs, NEVER the shipped
; image): -DNO_REGISTER (the entry skips REGISTER -- the D1.7 NO_REGISTER mutant),
; -DCRASH_ON_CLICK (a mouseDown executes UD2 -- the crash-triage leg) and
; -DBAD_MBAR (the SETMBAR refusal leg: two resources that lie about where they
; live -- `bad_nested`, in the image but its menus array is NOT, and $$-8, a
; MenuBar record in the PSP below imageBase -- each refused TENANT-SETMBAR-BAD,
; then `bad_nested` offered again AFTER the window -- refused the same way,
; because SETMBAR after the first window is legal since initech-tdnl.31 but
; still validated; the tenant runs with no bar of its own, so band 2 keeps the
; shell fallback).
; ASCII-clean (Rule 12); nasm -f bin is reproducible (Rule 11).

bits 32
%ifndef ORG
%define ORG 0
%endif
org ORG

%define TBX_GATE       0x81
%define TBX_REGISTER   0x0001
%define TBX_NEWWINDOW  0x0020
%define TBX_SETWTITLE  0x0022
%define TBX_TEXTDRAW   0x0030
%define TBX_FILLRECT   0x0031
%define TBX_EXIT       0x0070
%define TBX_SETMBAR    0x0050
%define TBX_DRAWMENUBAR 0x0052
%define EVT_MENU       0x0051  ; TBX_EVT_MENU (spec Sec 6a)
%define MENU_QUIT      (129 << 16) | 1   ; File > Quit
%define MENU_ABOUT     (131 << 16) | 1   ; Fixture > About TenantFix
%define COLOR_WHITE    0
%define COLOR_BLACK    1
%define EVT_MOUSEDOWN  1
%define EVT_UPDATE     6

; The window frame, GLOBAL coordinates (l,t,r,b). Chosen (a) clear of the APPS
; window's TENANTFX.EXE icon SPRITE (59,106)..(91,138) and its label band (to
; y=152), so the icon stays clickable while the tenant is resident (the
; DOUBLE_LAUNCH and EXIT_LEAK legs re-double-click it), and (b) CLOSE to that
; icon, so every icon <-> content hop is ONE int8-safe move: the serial legs
; must fit the flagship pump's 250-tick budget (FLAIR_TEN_TICK_BUDGET) on a
; loaded host. Clear of the volume icon (584,48)..(616,80), the Trash
; (584,404)..(616,436) and the cursor park rect at (620,460).
%define WIN_L 100
%define WIN_T 160
%define WIN_R 400
%define WIN_B 340

; ---------------------------------------------------------------------------
; ENTRY (DEC-08a.2: byte 0 of the module, EBX = PSP). Runs to its RETURN.
; ---------------------------------------------------------------------------
entry:
%ifndef NO_REGISTER
    push dword rec
    mov  eax, TBX_REGISTER
    int  TBX_GATE
    add  esp, 4
    test eax, eax
    jnz  .out                  ; refused -> just return (the kernel reaps)
%endif
%ifndef BAD_MBAR
    push dword mbar            ; our own bar, before the window (DEC-AC3-4:
    mov  eax, TBX_SETMBAR      ; it takes effect at the affirmation)
    int  TBX_GATE
    add  esp, 4
%else
    push dword bad_nested      ; refused: menus-outside-image
    mov  eax, TBX_SETMBAR
    int  TBX_GATE
    add  esp, 4
    push dword $$ - 8          ; refused: bar-outside-image (the PSP tail)
    mov  eax, TBX_SETMBAR
    int  TBX_GATE
    add  esp, 4
%endif
    push dword 1               ; goAway
    push dword WIN_B
    push dword WIN_R
    push dword WIN_T
    push dword WIN_L
    mov  eax, TBX_NEWWINDOW
    int  TBX_GATE
    add  esp, 20
    cmp  eax, 0
    jle  .out
    mov  [hwin], eax
%ifdef BAD_MBAR
    push dword bad_nested      ; refused again after the first window: SETMBAR
    mov  eax, TBX_SETMBAR      ; is legal there since tdnl.31, still validated
    int  TBX_GATE
    add  esp, 4
%endif
    push dword title
    push dword [hwin]
    mov  eax, TBX_SETWTITLE
    int  TBX_GATE
    add  esp, 8
    call paint
.out:
    ret

; paint -- the whole content: white field, two black text lines. The kernel
; clips every call to (visible INTERSECT content [INTERSECT update]).
paint:
    push dword COLOR_WHITE
    push dword 400             ; b (content-local; clamped to the content box)
    push dword 400             ; r
    push dword 0               ; t
    push dword 0               ; l
    push dword [hwin]
    mov  eax, TBX_FILLRECT
    int  TBX_GATE
    add  esp, 24
    push dword COLOR_WHITE     ; bg
    push dword COLOR_BLACK     ; fg
    push dword line1
    push dword 16              ; y
    push dword 16              ; x
    push dword [hwin]
    mov  eax, TBX_TEXTDRAW
    int  TBX_GATE
    add  esp, 24
    push dword COLOR_WHITE
    push dword COLOR_BLACK
    push dword line2
    push dword 40
    push dword 16
    push dword [hwin]
    mov  eax, TBX_TEXTDRAW
    int  TBX_GATE
    add  esp, 24
    ret

; ---------------------------------------------------------------------------
; eventProc -- the kernel copied the cooked event into evbuf, then called us.
; ---------------------------------------------------------------------------
event_proc:
    mov  eax, [evbuf]          ; What (FLAIR_EVBUF_WHAT_OFF = 0)
    cmp  eax, EVT_UPDATE
    je   .update
    cmp  eax, EVT_MOUSEDOWN
    je   .click
    ret                        ; everything else: nothing to do (yield)
.update:
    call paint
    ret
.click:
%ifdef CRASH_ON_CLICK
    ud2                        ; the crash-triage leg: #UD inside the image
%endif
    push dword 0               ; rc
    mov  eax, TBX_EXIT
    int  TBX_GATE
    add  esp, 4
    ret

; ---------------------------------------------------------------------------
; THE LINK-SYNTHESIZED FlairTenantRec (spec/toolbox_gate.h Sec 5)
; ---------------------------------------------------------------------------
align 4
rec:
    dd 0x50544C46              ; tag 'FLTP'
    dd appname                 ; namePtr        (reloc site)
    dd $$                      ; imageBase      (reloc site: == the load base)
    dd image_end - $$          ; imageLen       (a length: NOT relocated)
    dd evbuf                   ; evBufPtr       (reloc site)
    dd event_proc2             ; eventProc      (reloc site; tdnl.31)

; ---------------------------------------------------------------------------
; THE MenuBar RESOURCE (spec/toolbox_gate.h Sec 9 = os/flair/menu.h Sec 3)
; ---------------------------------------------------------------------------
%macro MITEM 3              ; text, cmdChar, enabled
    dd %1
    db 0, %2, 0, %3, 0      ; mark, cmdChar, style, enabled, is_divider
    db 0, 0, 0              ; pad (MenuItem is 12 bytes)
%endmacro
%macro MDIV 0
    dd s_dash
    db 0, 0, 0, 0, 1
    db 0, 0, 0
%endmacro
%macro MINFO 4              ; menuID, title, items, n_items
    dw %1, 0                ; menuID, pad
    dd %2                   ; title
    dd %3                   ; items
    dw %4, 0                ; n_items, menuWidth (a kernel cache: author 0)
%endmacro
align 4
mbar:
    dd mbar_menus           ; menus     (reloc site)
    dw 3                    ; n_menus
    db 1, 0                 ; has_apple, pad
mbar_menus:
    MINFO 129, t_file, items_file, 1
    MINFO 130, t_edit, items_edit, 5
    MINFO 131, t_fixture, items_fixture, 1
items_file:
    MITEM s_quit, 'Q', 1
items_edit:
    MITEM s_undo, 'Z', 0
    MDIV
    MITEM s_cut, 'X', 0
    MITEM s_copy, 'C', 0
    MITEM s_paste, 'V', 0
items_fixture:
    MITEM s_about, 0, 1
%ifdef BAD_MBAR
bad_nested:                 ; in the image -- but its menus array is not
    dd $$ - 16              ; menus -> the PSP (reloc site)
    dw 1
    db 1, 0
%endif
t_file:    db "File", 0
t_edit:    db "Edit", 0
t_fixture: db "Fixture", 0
s_quit:    db "Quit", 0
s_undo:    db "Undo", 0
s_dash:    db "-", 0
s_cut:     db "Cut", 0
s_copy:    db "Copy", 0
s_paste:   db "Paste", 0
s_about:   db "About TenantFix", 0

appname: db "TENANTFX", 0
title:   db "TenantFix", 0
line1:   db "Loaded from disk", 0
line2:   db "Click here to quit", 0

align 4
evbuf:   times 6 dd 0          ; the flat FlairEvent (24 bytes)
hwin:    dd 0                  ; the window token NEWWINDOW returned

; ===========================================================================
; MENU CHOICES (bead initech-tdnl.31) -- appended; see the banner.
; ===========================================================================
event_proc2:
    mov  eax, [evbuf]          ; What
    cmp  eax, EVT_MENU
    je   .menu
    cmp  eax, EVT_UPDATE
    jne  event_proc            ; every other event: the V1 handler, unchanged
    call paint
    cmp  dword [about_shown], 0
    je   .done
    call paint_about
.done:
    ret
.menu:                         ; DoMenuCommand(Message)
    mov  eax, [evbuf + 4]      ; Message (FLAIR_EVBUF_MSG_OFF = 4)
    cmp  eax, MENU_QUIT
    je   .quit
    cmp  eax, MENU_ABOUT
    je   .about
    ret
.quit:
    push dword 0               ; rc
    mov  eax, TBX_EXIT
    int  TBX_GATE
    add  esp, 4
    ret
.about:
    mov  dword [about_shown], 1
    call paint_about
    push dword mbar2           ; SetMenuBar ... after the window exists
    mov  eax, TBX_SETMBAR
    int  TBX_GATE
    add  esp, 4
    mov  eax, TBX_DRAWMENUBAR  ; ... then DrawMenuBar
    int  TBX_GATE
    ret

paint_about:
    push dword COLOR_WHITE     ; bg
    push dword COLOR_BLACK     ; fg
    push dword line3
    push dword 64              ; y (content-local)
    push dword 16              ; x
    push dword [hwin]
    mov  eax, TBX_TEXTDRAW
    int  TBX_GATE
    add  esp, 24
    ret

align 4
mbar2:                      ; the bar About installs: mbar + an "Info" menu
    dd mbar2_menus
    dw 4
    db 1, 0
mbar2_menus:
    MINFO 129, t_file, items_file, 1
    MINFO 130, t_edit, items_edit, 5
    MINFO 131, t_fixture, items_fixture, 1
    MINFO 132, t_info, items_info, 1
items_info:
    MITEM s_version, 0, 0
t_info:    db "Info", 0
s_version: db "TenantFix 1.0", 0
line3:     db "TenantFix 1.0, a disk app", 0

align 4
about_shown: dd 0
image_end:
