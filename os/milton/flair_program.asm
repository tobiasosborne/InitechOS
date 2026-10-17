; flair_program.asm -- FLAIR.COM, the command that starts the desktop.
;
; bead:  initech-tdnl.106 (boot to InitechDOS, type FLAIR, Special > Exit to
;        InitechDOS returns to the prompt); ADR-0003 Amendment DEC-19.
; Ref:   spec/toolbox_gate.h Sec 1 (INT 81h, AX = the function, result in EAX,
;          errors negative), Sec 3 (TBX_DESKSTART = 0x0003), Sec 17 (the rule
;          and the two lines below -- NASM cannot read the C header, so these
;          bytes are DUPLICATED here and test-flair-cmd-host D3 checks the
;          built binary carries both lines exactly);
;        os/flair/tbxdesk.h (the gate rule); spec/memory_map.h PROGRAM_IMAGE
;          (the org below MUST equal it, like every flat program here);
;        IBM DOS 3.30 Technical Reference pp1-5/1-6 (PDF22-23): COMMAND.COM
;          EXECs an external command and regains control when it terminates;
;        DOS 3.3 PRM AH=09h (EDX -> '$'-terminated) and AH=4Ch (AL = code).
;
; THE PROGRAM. A launcher, the way Windows 3.x's WIN.COM is an external
; command rather than a COMMAND.COM internal: it asks the resident Toolbox to
; start the desktop (INT 81h AX=TBX_DESKSTART) and terminates with code 0. The
; desktop starts when the EXEC that ran this program returns, and COMMAND.COM
; regains the console only after Special > Exit to InitechDOS. If the Toolbox
; refuses (the desktop is already running; any other refusal) it says so on
; the console and terminates with code 1. The command tail is ignored.
; Flat .COM-equivalent (ADR-0003 DEC-08), nasm -f bin: reproducible, no
; timestamps (Rule 11); ASCII only (Rule 12).

bits 32
org 0x00040100                 ; == spec/memory_map.h PROGRAM_IMAGE

TBX_DESKSTART   equ 0x0003     ; == spec/toolbox_gate.h TBX_DESKSTART
TBX_ERR_BUSY    equ -4         ; == spec/toolbox_gate.h TBX_ERR_BUSY

start:
    mov eax, TBX_DESKSTART     ; AX = the function; no arguments
    int 0x81
    test eax, eax
    jnz refused
    mov ax, 0x4C00             ; armed: terminate, code 0 -- the desktop starts
    int 0x21                   ; when this program's EXEC returns
    int 0x20                   ; (Rule 2: never run on into the data)

refused:
    mov edx, msg_fail
    cmp eax, TBX_ERR_BUSY
    jne say
    mov edx, msg_busy
say:
    mov ah, 0x09
    int 0x21
    mov ax, 0x4C01
    int 0x21
    int 0x20

msg_busy:
    db "FLAIR is already running.", 0x0D, 0x0A, "$"
msg_fail:
    db "FLAIR cannot start the desktop.", 0x0D, 0x0A, "$"
