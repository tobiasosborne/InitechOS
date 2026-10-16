; Factory-only boot fixture, initech-cqej / PRD Sec 8. Same 8042 pulse as
; os/milton/power.h kernel_reset; INT 13h sector I/O and UART programming
; follow os/boot/mbr.asm and stage2.asm. Sector 2 is a boot counter, so no
; assumption about BIOS preserving RAM across RESET enters the oracle.
; First boot emits READY, commits counter=1, resets. Second emits DONE then
; READY (ready is the completion boundary). The gate requires TWO READYs.
; BOCHS_FIXTURE_NO_RESET is the NEGATIVE stimulus for the driver's no-wait
; mutant: one READY then a permanent halt, so host scheduling cannot let a
; second boot race past the intentionally broken wait.
bits 16
org 0x7c00
    jmp 0:start
start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    cld
    sti
    mov [drive], dl
    mov dx, 0x3fb
    mov al, 0x80
    out dx, al
    mov dx, 0x3f8
    mov al, 1
    out dx, al
    inc dx
    xor al, al
    out dx, al
    mov dx, 0x3fb
    mov al, 3
    out dx, al
    mov bx, 0x7e00
    mov ax, 0x0201
    mov cx, 2
    xor dh, dh
    mov dl, [drive]
    int 0x13
    jc fail
    cmp byte [0x7e00], 1
    je second
    mov si, ready
    call puts
%ifdef BOCHS_FIXTURE_NO_RESET
    jmp halt
%endif
    mov byte [0x7e00], 1
    mov bx, 0x7e00
    mov ax, 0x0301
    mov cx, 2
    xor dh, dh
    mov dl, [drive]
    int 0x13
    jc fail
    mov si, reset
    call puts
    cli
    mov ecx, 0x100000
wait_kbc:
    in al, 0x64
    test al, 2
    jz pulse
    dec ecx
    jnz wait_kbc
    jmp fail
pulse:
    mov al, 0xfe
    out 0x64, al
    mov ecx, 0x100000
wait_reset:
    in al, 0x64
    dec ecx
    jnz wait_reset
fail:
    mov si, failed
    call puts
    jmp halt
second:
    mov si, done
    call puts
    mov si, ready
    call puts
halt:
    cli
    hlt
    jmp halt
puts:
    lodsb
    test al, al
    jz .done
    mov ah, al
    mov dx, 0x3fd
.wait:
    in al, dx
    test al, 0x20
    jz .wait
    mov al, ah
    mov dx, 0x3f8
    out dx, al
    jmp puts
.done:
    ret
drive db 0
ready db 'BOCHS-REBOOT-READY', 10, 0
reset db 'BOCHS-REBOOT-PULSE', 10, 0
done db 'BOCHS-REBOOT-DONE', 10, 0
failed db 'BOCHS-REBOOT-FAIL', 10, 0
times 510-($-$$) db 0
dw 0xaa55
