; flairstub.asm -- the run-time half of the Turbo Initech FLAIR binding: the
; trap stubs behind os/tps/FLAIR.PAS, the tenant entry, the link-synthesized
; registration record and the event buffer (beads initech-tdnl.22 R5.2,
; initech-tdnl.23 R5.3).
;
; Ref:   spec/toolbox_gate.h -- THE locked gate ABI. Every number this file
;          uses comes from it through build/flairgate.inc, which
;          tools/tbxgate_inc.c GENERATES from the header (Rule 8: no value is
;          typed here a second time);
;        docs/design/GUI-remediation-D1-D2-D3-design.md D2.5 (the stub layer:
;          one regular stub per trap, fileio.asm register discipline, errors
;          returned never fatal) and D2-2 (zero subset growth);
;        docs/design/GUI-remediation-ADR-reconciliation.md Part B DEC-09/DEC-10
;          (the event handler is a NAMED Pascal procedure whose address the
;          toolchain puts in the link-synthesized FlairTenantRec) and DEC-AC3-3
;          (push-callback delivery; returning is the yield);
;        seed/rt/start_dos.asm (the entry-stub lineage: the first byte of the
;          image is the entry; the compiled body is pas_main) and
;          seed/rt/fileio.asm (cdecl entries preserving ebx/esi/edi/ebp; the
;          ShortString -> ASCIZ length-strip + rep movsb idiom);
;        os/apps/tbx/tbx_rt.c + tenant.ld (the C binding's identical record,
;          entry and link: the same tenant.ld links a Pascal tenant).
;
; LINKAGE. A Turbo Initech FLAIR program is os/tps/FLAIR.PAS + the
; application, compiled by the seed or by Turbo Initech to one object whose
; routines are public `pf_<name>` labels and whose main block is `pas_main`.
; This object supplies what FLAIR.PAS declares `external`: one cdecl entry
; `pf_flair<verb>` per binding. Arguments arrive as the compilers push them:
; argument i at [ebp+8+4*i] after the standard prologue; a var parameter is
; the address of the caller's variable; booleans and chars are zero-extended
; dwords; a string is a ShortString (byte 0 = length).
;
; THE TRAP. Each stub rebuilds the gate's argument block on its own stack
; (argument 0 at the lowest address, spec Sec 2), loads AX and executes
; INT 81h; the result is in EAX (errors negative, Sec 4) and is returned to
; Pascal unchanged. A string is first copied, NUL-terminated, into this
; object's own .data storage: the gate refuses a string outside the image
; (Sec 5/7), and Pascal strings live in the .bss or on the stack, never in
; the image. A string the stub's storage cannot hold is refused HERE with
; TBX_ERR_BADARG and no trap.
;
; THE MENU BAR (Sec 9) is built in this object's .data by FlairNewMenu /
; FlairAppendItem (no trap) and installed by FlairSetMenuBar (SETMBAR): the
; graph the kernel reads in place is always in the image.
;
; MUTANT KNOBS (Rule 6; -D on the assembly of a test copy only -- one per
; verb class, each turns test-flairstub RED on that class):
;   FLAIRSTUB_MUT_LIFECYCLE  EXIT traps with 0 instead of the program's code
;   FLAIRSTUB_MUT_WINDOW     NEWWINDOW passes r and b swapped
;   FLAIRSTUB_MUT_DRAW       TEXTDRAW drops the string's last character
;   FLAIRSTUB_MUT_MENU       FlairAppendItem forgets the command key
;   FLAIRSTUB_MUT_ALERT      ALERT passes the cancel and other labels swapped
;   FLAIRSTUB_MUT_SCRAP      SCRAP_PUT passes one byte too few
;   FLAIRSTUB_MUT_EVENT      the event trampoline hands Pascal evBufPtr + 4
;
; ASCII-clean (Rule 12). Deterministic (Rule 11).

bits 32

%include "flairgate.inc"

extern pas_main                ; the compiled main block (start_dos lineage)
extern pf_flaireventhandler    ; FLAIR.PAS: procedure FlairEventHandler(var Ev)
extern __image_start           ; os/apps/tbx/tenant.ld
extern __image_len             ; os/apps/tbx/tenant.ld (ABSOLUTE: a length)

; The binding's own storage bounds. FLAIR.PAS FlairMaxMenus / FlairMaxItems
; carry the same two numbers (test-flairstub compares them).
FLAIR_MAX_MENUS  equ 8
FLAIR_MAX_ITEMS  equ 32
FLAIR_TEXT_CAP   equ 256           ; a ShortString's 255 bytes + the NUL
%if FLAIR_MAX_MENUS > TBX_MBAR_MAX_MENUS
%error "FLAIR_MAX_MENUS exceeds spec/toolbox_gate.h TBX_MBAR_MAX_MENUS"
%endif

; cdecl frame: ebp, then the three callee-saved registers fileio.asm keeps.
%macro STUB_ENTER 0
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
%endmacro
%macro STUB_LEAVE 0
    lea esp, [ebp-12]
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
%endmacro
; push the Pascal argument number %1 (0-based) onto the gate block
%macro PUSHARG 1
    push dword [ebp + 8 + 4 * (%1)]
%endmacro
%macro TRAP 1
    mov eax, %1
    int TBX_GATE_VECTOR
%endmacro
; A stub whose arguments all pass through unchanged: %1 label, %2 code,
; %3 arity (from the header). The block is pushed last argument first.
%macro SCALAR_STUB 3
global %1
%1:
    STUB_ENTER
%assign flair_i %3
%rep %3
%assign flair_i flair_i - 1
    PUSHARG flair_i
%endrep
    TRAP %2
    STUB_LEAVE
%endmacro

; ===========================================================================
; THE ENTRY (module byte 0, ADR-0003 DEC-08a.2; tenant.ld places .text.entry
; first). The kernel calls it with EBX = the PSP; it runs the program's main
; block -- registration, menus, the window -- to its RETURN, which hands the
; machine back (ADR-0013 Sec 3.4). The kernel's trampoline saved every
; callee-saved register, so the compiled code may use them freely.
; ===========================================================================
section .text.entry progbits alloc exec nowrite align=1
global flair_entry
flair_entry:
    call pas_main
    ret

section .text

; ===========================================================================
; THE EVENT TRAMPOLINE: the record's eventProc. The kernel has copied the
; cooked event into flair_evbuf (spec Sec 6); hand Pascal the buffer as its
; var FlairEvent and return -- the return is the yield.
; ===========================================================================
flair_event_proc:
%ifdef FLAIRSTUB_MUT_EVENT
    push dword flair_evbuf + 4         ; MUTANT: the fields shifted by one
%else
    push dword flair_evbuf
%endif
    call pf_flaireventhandler
    add esp, 4
    ret

; ---------------------------------------------------------------------------
; flair_asciz: ShortString -> NUL-terminated copy.
; In:  ESI = the ShortString (byte 0 = length), EDI = destination,
;      EDX = destination capacity in bytes, the NUL included.
; Out: EAX = the length copied, or -1 when length + 1 > capacity (nothing is
;      copied). Clobbers ECX, ESI, EDI.
; ---------------------------------------------------------------------------
flair_asciz:
    movzx ecx, byte [esi]
    lea eax, [ecx + 1]
    cmp eax, edx
    ja .too_long
    mov eax, ecx
    inc esi
    cld
    rep movsb
    mov byte [edi], 0
    ret
.too_long:
    mov eax, -1
    ret

; ===========================================================================
; LIFECYCLE
; ===========================================================================
; function FlairRegister(var Name: string): integer;  REGISTER (recPtr)
global pf_flairregister
pf_flairregister:
    STUB_ENTER
    mov esi, [ebp + 8]
    mov edi, flair_name
    mov edx, FLAIR_TENANT_NAME_MAX
    call flair_asciz
    test eax, eax
    js .refuse
    push dword flair_rec
    TRAP TBX_REGISTER
    STUB_LEAVE
.refuse:
    mov eax, TBX_ERR_BADARG
    STUB_LEAVE

; function FlairExit(Code: integer): integer;  EXIT (rc)
global pf_flairexit
pf_flairexit:
    STUB_ENTER
%ifdef FLAIRSTUB_MUT_LIFECYCLE
    push dword 0                       ; MUTANT: the program's code is lost
%else
    PUSHARG 0
%endif
    TRAP TBX_EXIT
    STUB_LEAVE

; ===========================================================================
; WINDOWS
; ===========================================================================
; function FlairNewWindow(L, T, R, B, GoAway: integer): integer;
global pf_flairnewwindow
pf_flairnewwindow:
    STUB_ENTER
    PUSHARG 4
%ifdef FLAIRSTUB_MUT_WINDOW
    PUSHARG 2                          ; MUTANT: r and b swapped
    PUSHARG 3
%else
    PUSHARG 3
    PUSHARG 2
%endif
    PUSHARG 1
    PUSHARG 0
    TRAP TBX_NEWWINDOW
    STUB_LEAVE

; function FlairSetWTitle(Win: integer; var Title: string): integer;
global pf_flairsetwtitle
pf_flairsetwtitle:
    STUB_ENTER
    mov esi, [ebp + 12]
    mov edi, flair_text
    mov edx, FLAIR_TEXT_CAP
    call flair_asciz
    push dword flair_text
    PUSHARG 0
    TRAP TBX_SETWTITLE
    STUB_LEAVE

SCALAR_STUB pf_flaircontentorigin, TBX_CONTENTORIGIN, TBX_ARGC_CONTENTORIGIN
SCALAR_STUB pf_flairglobaltolocal, TBX_GLOBALTOLOCAL, TBX_ARGC_GLOBALTOLOCAL

; ===========================================================================
; DRAWING
; ===========================================================================
; The run (Win, X, Y, S, Fg, Bg) of TEXTDRAW and DRAWCELLS: S at Pascal
; argument 3 becomes the ASCIZ copy.
%macro TEXT_STUB 2
global %1
%1:
    STUB_ENTER
    mov esi, [ebp + 20]
    mov edi, flair_text
    mov edx, FLAIR_TEXT_CAP
    call flair_asciz
%ifdef FLAIRSTUB_MUT_DRAW
%if %2 == TBX_TEXTDRAW
    test eax, eax                      ; MUTANT: the last character dropped
    jz %%keep
    mov byte [flair_text + eax - 1], 0
%%keep:
%endif
%endif
    PUSHARG 5
    PUSHARG 4
    push dword flair_text
    PUSHARG 2
    PUSHARG 1
    PUSHARG 0
    TRAP %2
    STUB_LEAVE
%endmacro

TEXT_STUB pf_flairtextdraw, TBX_TEXTDRAW
TEXT_STUB pf_flairdrawcells, TBX_DRAWCELLS
SCALAR_STUB pf_flairfillrect, TBX_FILLRECT, TBX_ARGC_FILLRECT

; ===========================================================================
; MENUS (spec Sec 9: the MenuBar / MenuInfo / MenuItem graph, in .data)
; ===========================================================================
; function FlairNewMenu(MenuID: integer; var Title: string): integer;
global pf_flairnewmenu
pf_flairnewmenu:
    STUB_ENTER
    mov ebx, [flair_nmenus]
    cmp ebx, FLAIR_MAX_MENUS
    jae .refuse
    mov edi, ebx                       ; the title's string slot
    imul edi, edi, TBX_MBAR_STR_MAX
    add edi, flair_mstr
    push edi
    mov esi, [ebp + 12]
    mov edx, TBX_MBAR_STR_MAX
    call flair_asciz
    pop ecx
    test eax, eax
    js .refuse
    mov edi, ebx                       ; the MenuInfo record
    imul edi, edi, TBX_MINFO_BYTES
    add edi, flair_minfo
    mov eax, [ebp + 8]
    mov word [edi + TBX_MINFO_ID_OFF], ax
    mov word [edi + TBX_MINFO_ID_OFF + 2], 0
    mov [edi + TBX_MINFO_TITLE_OFF], ecx
    mov eax, [flair_nitems]            ; its items start at the next item
    imul eax, eax, TBX_MITEM_BYTES
    add eax, flair_mitem
    mov [edi + TBX_MINFO_ITEMS_OFF], eax
    mov word [edi + TBX_MINFO_COUNT_OFF], 0
    mov word [edi + TBX_MINFO_WIDTH_OFF], 0
    inc dword [flair_nmenus]
    mov eax, TBX_OK
    STUB_LEAVE
.refuse:
    mov eax, TBX_ERR_BADARG
    STUB_LEAVE

; function FlairAppendItem(var Text: string; CmdChar: char;
;                          Enabled: boolean): integer;
; Appends to the menu FlairNewMenu made last. Text '-' is a dividing line
; (IM AppendMenu).
global pf_flairappenditem
pf_flairappenditem:
    STUB_ENTER
    mov ebx, [flair_nmenus]
    test ebx, ebx
    jz .refuse
    mov ecx, [flair_nitems]
    cmp ecx, FLAIR_MAX_ITEMS
    jae .refuse
    mov edi, ecx                       ; the text's string slot
    add edi, FLAIR_MAX_MENUS
    imul edi, edi, TBX_MBAR_STR_MAX
    add edi, flair_mstr
    push edi
    mov esi, [ebp + 8]
    mov edx, TBX_MBAR_STR_MAX
    call flair_asciz
    pop esi                            ; esi = the slot
    test eax, eax
    js .refuse
    mov edi, [flair_nitems]            ; the MenuItem record
    imul edi, edi, TBX_MITEM_BYTES
    add edi, flair_mitem
    mov [edi + TBX_MITEM_TEXT_OFF], esi
    mov byte [edi + TBX_MITEM_MARK_OFF], 0
%ifdef FLAIRSTUB_MUT_MENU
    xor ecx, ecx                       ; MUTANT: the command key forgotten
%else
    mov ecx, [ebp + 12]
%endif
    mov [edi + TBX_MITEM_CMD_OFF], cl
    mov byte [edi + TBX_MITEM_STYLE_OFF], 0
    mov ecx, [ebp + 16]
    mov [edi + TBX_MITEM_ENABLED_OFF], cl
    xor ecx, ecx
    cmp eax, 1
    jne .not_divider
    cmp byte [esi], '-'
    jne .not_divider
    mov cl, 1
.not_divider:
    mov [edi + TBX_MITEM_DIVIDER_OFF], cl
    mov byte [edi + TBX_MITEM_DIVIDER_OFF + 1], 0
    mov byte [edi + TBX_MITEM_DIVIDER_OFF + 2], 0
    mov byte [edi + TBX_MITEM_DIVIDER_OFF + 3], 0
    lea edx, [ebx - 1]                 ; the menu it joins: n_items + 1
    imul edx, edx, TBX_MINFO_BYTES
    add edx, flair_minfo
    inc word [edx + TBX_MINFO_COUNT_OFF]
    inc dword [flair_nitems]
    mov eax, TBX_OK
    STUB_LEAVE
.refuse:
    mov eax, TBX_ERR_BADARG
    STUB_LEAVE

; function FlairSetMenuBar(HasApple: boolean): integer;  SETMBAR (barPtr)
global pf_flairsetmenubar
pf_flairsetmenubar:
    STUB_ENTER
    mov eax, [flair_nmenus]
    mov word [flair_mbar + TBX_MBAR_COUNT_OFF], ax
    mov eax, [ebp + 8]
    mov byte [flair_mbar + TBX_MBAR_APPLE_OFF], al
    push dword flair_mbar
    TRAP TBX_SETMBAR
    STUB_LEAVE

SCALAR_STUB pf_flairdrawmenubar, TBX_DRAWMENUBAR, TBX_ARGC_DRAWMENUBAR

; ===========================================================================
; THE ALERT (Sec 11)
; function FlairAlert(var Text, DefLabel, CancelLabel, OtherLabel: string):
;          integer;  ALERT (textPtr, defPtr, cancelPtr, otherPtr)
; An EMPTY cancel or other label passes 0 (no such button).
; ===========================================================================
global pf_flairalert
pf_flairalert:
    STUB_ENTER
    mov esi, [ebp + 8]
    mov edi, flair_alert_text
    mov edx, FLAIR_TEXT_CAP
    call flair_asciz
    mov esi, [ebp + 12]
    mov edi, flair_alert_def
    mov edx, TBX_ALERT_LABEL_MAX
    call flair_asciz
    test eax, eax
    js .refuse
    mov esi, [ebp + 16]
    mov edi, flair_alert_cancel
    mov edx, TBX_ALERT_LABEL_MAX
    call flair_asciz
    test eax, eax
    js .refuse
    mov ebx, flair_alert_cancel
    test eax, eax
    jnz .cancel_set
    xor ebx, ebx
.cancel_set:
    mov esi, [ebp + 20]
    mov edi, flair_alert_other
    mov edx, TBX_ALERT_LABEL_MAX
    call flair_asciz
    test eax, eax
    js .refuse
    mov edx, flair_alert_other
    test eax, eax
    jnz .other_set
    xor edx, edx
.other_set:
%ifdef FLAIRSTUB_MUT_ALERT
    push ebx                           ; MUTANT: cancel and other swapped
    push edx
%else
    push edx
    push ebx
%endif
    push dword flair_alert_def
    push dword flair_alert_text
    TRAP TBX_ALERT
    STUB_LEAVE
.refuse:
    mov eax, TBX_ERR_BADARG
    STUB_LEAVE

; ===========================================================================
; THE SCRAP, TEXT flavor (Sec 12), ShortString payloads (D2-4)
; ===========================================================================
; function FlairScrapPut(var S: string): integer;  SCRAP_PUT (bufPtr, len)
global pf_flairscrapput
pf_flairscrapput:
    STUB_ENTER
    mov esi, [ebp + 8]
    movzx ecx, byte [esi]
    mov ebx, ecx
    inc esi
    mov edi, flair_scrap
    cld
    rep movsb
%ifdef FLAIRSTUB_MUT_SCRAP
    test ebx, ebx                      ; MUTANT: one byte too few
    jz .short_done
    dec ebx
.short_done:
%endif
    push ebx
    push dword flair_scrap
    TRAP TBX_SCRAP_PUT
    STUB_LEAVE

; function FlairScrapGet(var S: string): integer;  SCRAP_GET (bufPtr, size)
; The copy lands in S (length byte + content) only when the gate answered a
; length; an error leaves S untouched and is returned.
global pf_flairscrapget
pf_flairscrapget:
    STUB_ENTER
    push dword FLAIR_TEXT_CAP - 1
    push dword flair_scrap
    TRAP TBX_SCRAP_GET
    test eax, eax
    js .done
    cmp eax, FLAIR_TEXT_CAP - 1
    ja .bad_length
    mov edi, [ebp + 8]
    mov [edi], al
    inc edi
    mov esi, flair_scrap
    mov ecx, eax
    cld
    rep movsb
.done:
    STUB_LEAVE
.bad_length:
    mov eax, TBX_ERR_BADARG
    STUB_LEAVE

; ---------------------------------------------------------------------------
; Serial helpers -- duplicated VERBATIM from seed/rt/start.asm (grep marker:
; SEED-RT-SERIAL-HELPERS; a fix lands in every file carrying it). A compiled
; program always names them (the string intrinsic __str_write, Write and
; Writeln); a FLAIR tenant runs in ring 0, so a Writeln it makes goes to COM1
; -- the serial log -- exactly as a .COM program's does.
; ---------------------------------------------------------------------------
COM1 equ 0x3F8

; -- serial_putc: write AL to COM1 -----------------------------------------
; In:  AL = byte. Clobbers: DX. Preserves nothing else of interest.
global serial_putc
serial_putc:
    mov dx, COM1
    out dx, al
    ret

; -- serial_puts: write a NUL-terminated string at EAX to COM1 -------------
; In:  EAX = pointer to NUL-terminated bytes. Clobbers: EAX, DX, ESI.
global serial_puts
serial_puts:
    push esi
    mov esi, eax
.next:
    mov al, [esi]
    test al, al
    jz .done
    mov dx, COM1
    out dx, al
    inc esi
    jmp .next
.done:
    pop esi
    ret

; -- serial_put_int: write signed 32-bit EAX as decimal to COM1 ------------
; In:  EAX = signed value. Clobbers: EAX, EBX, ECX, EDX, ESI.
; Method: handle sign, then divide by 10 building digits on the stack (least
; significant first), then emit them in reverse. INT_MIN (-2147483648) is
; handled correctly because we negate into an UNSIGNED magnitude path: we
; track the sign flag, then operate on the magnitude via unsigned division.
global serial_put_int
serial_put_int:
    push esi
    xor esi, esi                ; esi = sign flag (1 if negative)
    test eax, eax
    jns .positive
    mov esi, 1
    neg eax                     ; magnitude (for INT_MIN this wraps to the
                                ; correct unsigned 0x80000000 magnitude)
.positive:
    ; Build decimal digits onto the stack. ESP grows down; we count digits.
    mov ecx, 0                  ; digit count
    mov ebx, 10                 ; divisor
.div_loop:
    xor edx, edx                ; clear high dividend word
    div ebx                     ; unsigned: edx:eax / 10 -> eax, rem in edx
    add dl, '0'                 ; remainder -> ASCII digit
    push edx                    ; stash digit byte (in dl) on the stack
    inc ecx
    test eax, eax
    jnz .div_loop

    ; Emit sign if negative.
    test esi, esi
    jz .emit
    mov al, '-'
    mov dx, COM1
    out dx, al
.emit:
    ; Pop ecx digits, most significant first (they were pushed LS-first).
.emit_loop:
    pop eax                     ; digit byte in al (high bytes are remainder)
    mov dx, COM1
    out dx, al
    dec ecx
    jnz .emit_loop

    pop esi
    ret

; ===========================================================================
; STORAGE -- all in the IMAGE (.data), because the gate validates every
; pointer it is given against [imageBase, imageBase + imageLen) (Sec 5/7/9).
; ===========================================================================
section .data
align 4
; The FlairTenantRec (Sec 5), link-synthesized: every pointer an absolute
; dword the InitechMZ relocation table covers. Each field is placed at its
; header offset; a field written past it makes `times` negative and the
; assembly fails (the layout cannot drift from the header).
flair_rec:
    times FLAIR_TENANT_REC_TAG_OFF - ($ - flair_rec) db 0
    dd FLAIR_TENANT_REC_TAG
    times FLAIR_TENANT_REC_NAME_OFF - ($ - flair_rec) db 0
    dd flair_name
    times FLAIR_TENANT_REC_BASE_OFF - ($ - flair_rec) db 0
    dd __image_start
    times FLAIR_TENANT_REC_LEN_OFF - ($ - flair_rec) db 0
    dd __image_len
    times FLAIR_TENANT_REC_EVB_OFF - ($ - flair_rec) db 0
    dd flair_evbuf
    times FLAIR_TENANT_REC_PROC_OFF - ($ - flair_rec) db 0
    dd flair_event_proc
    times FLAIR_TENANT_REC_BYTES - ($ - flair_rec) db 0

align 4
flair_evbuf:        times FLAIR_EVBUF_BYTES db 0          ; Sec 6
flair_name:         times FLAIR_TENANT_NAME_MAX db 0      ; Sec 5 namePtr
align 4
flair_mbar:                                               ; Sec 9 MenuBar
    times TBX_MBAR_MENUS_OFF - ($ - flair_mbar) db 0
    dd flair_minfo
    times TBX_MBAR_BYTES - ($ - flair_mbar) db 0
flair_nmenus:       dd 0
flair_nitems:       dd 0
flair_minfo:        times FLAIR_MAX_MENUS * TBX_MINFO_BYTES db 0
flair_mitem:        times FLAIR_MAX_ITEMS * TBX_MITEM_BYTES db 0
flair_mstr:         times (FLAIR_MAX_MENUS + FLAIR_MAX_ITEMS) * TBX_MBAR_STR_MAX db 0
flair_text:         times FLAIR_TEXT_CAP db 0
flair_alert_text:   times FLAIR_TEXT_CAP db 0
flair_alert_def:    times TBX_ALERT_LABEL_MAX db 0
flair_alert_cancel: times TBX_ALERT_LABEL_MAX db 0
flair_alert_other:  times TBX_ALERT_LABEL_MAX db 0
flair_scrap:        times FLAIR_TEXT_CAP db 0

section .note.GNU-stack noalloc noexec nowrite progbits
