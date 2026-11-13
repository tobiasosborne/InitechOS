; execguard_program.asm -- EXECGD.COM: a resident flat program that EXECs the
; command interpreter on the FLAGSHIP (bead initech-13ig).
;
; Ref:   MS-DOS 3.3 Reference pp.46-47 (COMMAND /C: "loads a second copy of
;        COMMAND.COM", runs one command, returns); IBM DOS 3.30 TR pp. 6-195..201
;        (AH=4Bh EXEC: EDX = path, EBX = parameter block {env, tail, fcb1, fcb2};
;        CF set + AX = error 2 file not found / 11 invalid format; AH=4Dh the
;        child's return code); spec/dos_structs.h exec_param_block_t;
;        os/milton/transient.h (the boot volume's COMMAND.COM is the COMMAND3
;        overlay, not a .COM). The same {count, " /C <cmd>", CR} tail as
;        os/samir/pal/pal_milton.c milton_exec (dBASE RUN / !, initech-n8q8).
;
; What it proves on serial (the console fans to serial after Exit to DOS):
;   EXECGUARD-BEGIN
;   EXECGUARD-CHILD-OK                  <- ECHOed by the COMMAND /C child
;   EXECGUARD-COMSPEC-OK | EXECGUARD-COMSPEC-ERR ax=XXXX | ...-RC ax=XXXX
;   EXECGUARD-OVERLAY-REFUSED ax=XXXX | EXECGUARD-OVERLAY-RAN
;   EXECGUARD-END
; The second EXEC names CMD2.COM, a byte copy of the boot volume's COMMAND.COM
; placed on the gate's data volume: the loader must refuse it (000Bh), never
; run its header. Position independent (call/pop); exits via AH=4Ch. ASCII only.
bits 32
org 0
start:
    call base
base:
    pop ebp
    sub ebp, base
    lea edx, [ebp+s_begin]
    call puts

    ; ---- 1. EXEC COMMAND.COM with the tail " /C ECHO EXECGUARD-CHILD-OK" ----
    lea eax, [ebp+tail_c]
    mov [ebp+blk+4], eax
    mov dword [ebp+blk], 0              ; env 0 = inherit (pal_milton.c)
    lea ebx, [ebp+blk]
    lea edx, [ebp+comspec]
    mov eax, 0x4b00
    int 0x21
    jc .comspec_err
    mov eax, 0x4d00
    int 0x21
    test ax, ax
    jnz .comspec_rc
    lea edx, [ebp+s_comspec_ok]
    call puts
    jmp .overlay
.comspec_err:
    push eax                            ; AH=09h returns AL='$' -- keep AX
    lea edx, [ebp+s_comspec_err]
    call puts
    pop eax
    call put_ax_crlf
    jmp .overlay
.comspec_rc:
    push eax                            ; AH=09h returns AL='$' -- keep AX
    lea edx, [ebp+s_comspec_rc]
    call puts
    pop eax
    call put_ax_crlf

    ; ---- 2. EXEC CMD2.COM (the overlay under another name): must be refused ----
.overlay:
    lea eax, [ebp+tail_0]
    mov [ebp+blk+4], eax
    mov dword [ebp+blk], 0
    lea ebx, [ebp+blk]
    lea edx, [ebp+cmd2]
    mov eax, 0x4b00
    int 0x21
    jc .overlay_refused
    lea edx, [ebp+s_overlay_ran]
    call puts
    jmp .done
.overlay_refused:
    push eax                            ; AH=09h returns AL='$' -- keep AX
    lea edx, [ebp+s_overlay_refused]
    call puts
    pop eax
    call put_ax_crlf
.done:
    lea edx, [ebp+s_end]
    call puts
    mov eax, 0x4c00
    int 0x21

; puts: EDX -> '$'-terminated string (AH=09h). Clobbers EAX.
puts:
    mov eax, 0x0900
    int 0x21
    ret

; put_ax_crlf: print AX as four upper-case hex digits + CRLF via AH=02h.
put_ax_crlf:
    movzx esi, ax
    mov ecx, 4
.digit:
    rol si, 4
    mov edx, esi
    and edx, 0x0f
    mov dl, [ebp+hexdigits+edx]
    mov eax, 0x0200
    int 0x21
    dec ecx
    jnz .digit
    mov edx, 13
    mov eax, 0x0200
    int 0x21
    mov edx, 10
    mov eax, 0x0200
    int 0x21
    ret

blk:                dd 0, 0, 0, 0       ; exec_param_block_t (16 bytes)
comspec             db 'COMMAND.COM', 0
cmd2                db 'CMD2.COM', 0
tail_c_text         equ tail_c_end - tail_c - 2
tail_c              db tail_c_text, ' /C ECHO EXECGUARD-CHILD-OK', 13
tail_c_end:
tail_0              db 0, 13
hexdigits           db '0123456789ABCDEF'
s_begin             db 'EXECGUARD-BEGIN', 13, 10, '$'
s_comspec_ok        db 'EXECGUARD-COMSPEC-OK', 13, 10, '$'
s_comspec_err       db 'EXECGUARD-COMSPEC-ERR ax=$'
s_comspec_rc        db 'EXECGUARD-COMSPEC-RC ax=$'
s_overlay_ran       db 'EXECGUARD-OVERLAY-RAN', 13, 10, '$'
s_overlay_refused   db 'EXECGUARD-OVERLAY-REFUSED ax=$'
s_end               db 'EXECGUARD-END', 13, 10, '$'
