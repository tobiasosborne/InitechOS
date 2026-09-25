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
; Build knobs (fixture VARIANTS for the mutant/crash legs, NEVER the shipped
; image): -DNO_REGISTER (the entry skips REGISTER -- the D1.7 NO_REGISTER mutant)
; and -DCRASH_ON_CLICK (a mouseDown executes UD2 -- the crash-triage leg).
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
    dd event_proc              ; eventProc      (reloc site)

appname: db "TENANTFX", 0
title:   db "TenantFix", 0
line1:   db "Loaded from disk", 0
line2:   db "Click here to quit", 0

align 4
evbuf:   times 6 dd 0          ; the flat FlairEvent (24 bytes)
hwin:    dd 0                  ; the window token NEWWINDOW returned
image_end:
