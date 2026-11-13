; tsr_program.asm -- the test-tsr fixture programs (bead initech-6fdt).
;
; Default build -> TSR.COM: a minimal SideKick-shaped resident. Classic TSR
;   layout: the resident handlers + data come FIRST, the transient installer
;   LAST, so AH=31h keeps only [PSP, resident_end). It hooks
;     vector 0x29 (INT 09h-equivalent, IRQ1): chains to the previous handler by
;       simulating an interrupt, then reads the BIOS keyboard flag byte at
;       0x417; Ctrl+Alt held (SideKick 1.5's chord) arms a pop-up. It never
;       calls DOS from the IRQ path.
;     vector 0x38 (INT 28h-equivalent, DOS idle): with a pop-up armed and the
;       InDOS byte (AH=34h) at most 1, it saves the 80x25 page (vector 0x39
;       AL=0), writes its window + the marker through INT 21h AH=40h (a
;       function above 0Ch, allowed from the idle hook), restores the page
;       (AL=1), re-saves and compares, then chains to the previous handler.
;   Then AH=31h with DX = the resident paragraphs.
; -DTSR_WAITKEY -> WAITKEY.COM: the foreground program the pop-up interrupts. It
;   announces WAITKEY-WAITING, blocks in AH=08h, and reports the key it got.
;   Position independent (EIP-relative data): it loads ABOVE the resident.
;
; Ref: spec/tsr_contract.h (every vector, address, size); IBM DOS 3.30 TR
;   pp. 6-105/106 (AH=31h), p. 6-26 (TSR layout, DX = resident size); Borland
;   SideKick 1.5 Owner's Handbook (Mar 1985) p. 13 (Ctrl+Alt). CLAUDE.md Rule 11
;   (nasm -f bin is reproducible), Rule 12 (ASCII).

bits 32
org 0x00040100                 ; == spec/memory_map.h PROGRAM_IMAGE

%ifdef TSR_WAITKEY
start:
    call .base
.base:
    pop ebp
    lea edx, [ebp + msg_wait - .base]
    mov ah, 0x09
    int 0x21
    mov ah, 0x08               ; console input, no echo: raises the idle vector
    int 0x21
    mov [ebp + got - .base], al
    lea edx, [ebp + msg_got - .base]
    mov ah, 0x09
    int 0x21
    mov ax, 0x4C00
    int 0x21
msg_wait: db "WAITKEY-WAITING", 0x0D, 0x0A, "$"
msg_got:  db "WAITKEY-GOT="
got:      db "?", 0x0D, 0x0A, "$"
%else

start:
    jmp init

; ---------------------------- resident part ----------------------------------
old09:   dd 0
old38:   dd 0
indos:   dd 0
pending: db 0

kbd_hook:                       ; entered through a 0x8E gate: IF clear
    pushfd
    push cs
    call dword [old09]          ; the kernel IRQ1 service: port 60h read + EOI
    push eax
    mov al, [0x417]             ; TSR_KBD_FLAGS_LINEAR
    and al, 0x0C                ; TSR_CHORD_DEFAULT (Ctrl | Alt)
    cmp al, 0x0C
    jne .out
    mov byte [pending], 1
.out:
    pop eax
    iretd

idle_hook:
    cmp byte [pending], 0
    je .chain
    pushad
    mov eax, [indos]
    mov al, [eax]
    cmp al, 1
    ja .later                   ; deeper inside DOS: wait for a later idle
    add al, '0'
    mov [mark_indos], al
    mov byte [pending], 0
    mov eax, 0x0000             ; SAVE the page
    mov edx, page
    int 0x39
    mov ah, 0x40                ; draw the window through CON
    mov ebx, 1
    mov ecx, win_len
    mov edx, win
    int 0x21
    mov eax, 0x0001             ; RESTORE the page
    mov edx, page
    int 0x39
    mov eax, 0x0000             ; re-save and compare: the page came back
    mov edx, page2
    int 0x39
    mov esi, page
    mov edi, page2
    mov ecx, 4002
    repe cmpsb
    je .same
    mov dword [mark_screen], 'DIRT'
.same:
    mov ah, 0x40                ; the marker (the screen is already restored)
    mov ebx, 1
    mov ecx, mark_len
    mov edx, mark
    int 0x21
    mov eax, 0x0001             ; and put the page back once more
    mov edx, page
    int 0x39
.later:
    popad
.chain:
    jmp dword [old38]

win:      db 0x0D, 0x0A, "+-- INITECH SIDEKICK: Notepad Calculator Calendar --+", 0x0D, 0x0A
win_len   equ $ - win
mark:     db 0x0D, 0x0A, "SK-POPUP INDOS="
mark_indos: db "?", " SCREEN="
mark_screen: db "SAME", 0x0D, 0x0A
mark_len  equ $ - mark
page:     times 4002 db 0       ; TSR_SCREEN_BYTES
page2:    times 4002 db 0
resident_end:

; ---------------------------- transient installer ----------------------------
init:
    mov [psp], ebx              ; the loader passes the PSP in EBX
    mov ax, 0x3529              ; GETVECT 29h -> EBX
    int 0x21
    mov [old09], ebx
    mov ax, 0x3538
    int 0x21
    mov [old38], ebx
    mov ah, 0x34                ; InDOS address -> EBX
    int 0x21
    mov [indos], ebx
    mov ax, 0x2529              ; SETVECT 29h = kbd_hook (interrupt gate)
    mov edx, kbd_hook
    int 0x21
    mov ax, 0x2538              ; SETVECT 38h = idle_hook
    mov edx, idle_hook
    int 0x21
    mov edx, msg_inst
    mov ah, 0x09
    int 0x21
    mov edx, resident_end       ; DX = paragraphs from the PSP
    sub edx, [psp]
    add edx, 15
    shr edx, 4
    mov ax, 0x3100              ; KEEP, return code 0
    int 0x21
    int 0x20                    ; not reached: 31h does not return
psp:      dd 0
msg_inst: db "SK-INSTALLED", 0x0D, 0x0A, "$"
%endif
