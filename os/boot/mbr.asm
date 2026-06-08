; mbr.asm -- InitechOS tracer boot, stage1 / MBR (512 bytes, sector 0).
;
; beads: initech-f8v.2 ("Tracer boot: MBR -> stage2 -> 32-bit flat -> LFB
;        seafoam fill"). Shared with M1 (M1 fleshes out robustness/modes).
; Ref:   PRD Sec 5 (hardware contract): "Boot: Custom MBR -> stage2 ...";
;        "boot in real mode, switch to 32-bit protected, flat segmentation".
;        PRD Sec 11 (M1): "MBR -> stage2 -> 32-bit protected/flat -> VESA LFB".
;        CLAUDE.md hallucination callout: "Real mode -> 32-bit protected/flat
;        is a minefield" -- that transition lives in stage2; the MBR's only
;        job is to load stage2 reliably and hand off.
;        CLAUDE.md Rule 2 (fail loud), Rule 12 (ASCII-only source).
;
; BIOS loads this 512-byte sector at physical 0x7C00 in 16-bit real mode and
; jumps to it with DL = the BIOS boot drive number. We:
;   1. set up segments + a stack below 0x7C00,
;   2. init COM1 (0x3F8) and print the stage-1 marker "S1\n" over serial,
;   3. load stage2 from the disk into 0x0000:0x8000 via INT 13h,
;   4. far-jump to stage2 at 0x0000:0x8000.
;
; Disk addressing decision (documented per Law 1): we use INT 13h **CHS**
; (AH=02h) reads, not LBA extensions. Rationale: the image is a small raw
; disk and QEMU's BIOS (SeaBIOS) presents it with a CHS geometry whose first
; track holds far more than the few sectors of stage2 we need; reading
; sectors 2.. of cylinder 0 / head 0 is the simplest universally-supported
; path and needs no EDD/INT 13h AH=42h probe. We hardcode that stage2 begins
; at LBA sector 1 (the sector right after the MBR), i.e. CHS C=0 H=0 S=2.
;
; Layout written by the image build (root Makefile):
;   sector 0            : this MBR (512 bytes, 0xAA55 signature)
;   sectors 1..N        : stage2 (padded to a whole number of sectors)
; STAGE2_SECTORS must cover stage2's size; the Makefile pads stage2 to that
; many sectors so the count is deterministic.

bits 16
org 0x7C00

; -- Constants --------------------------------------------------------------
COM1            equ 0x3F8
STAGE2_SEG      equ 0x0000
STAGE2_OFF      equ 0x8000      ; stage2 loaded at 0x0000:0x8000 (phys 0x8000)
STAGE2_SECTORS  equ 16          ; sectors to read (8 KiB); ample for stage2
STAGE2_LBA      equ 1           ; stage2 starts at sector 1 (CHS C0 H0 S2)

start:
    cli
    ; Normalize segment registers: CS is already implied by the org/jump.
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00              ; stack grows down from just below the MBR
    sti

    ; Save BIOS boot drive (DL) for the INT 13h read.
    mov [boot_drive], dl

    ; -- COM1 init (8N1, 115200) --------------------------------------------
    ; Real UART programming (unlike the harness fixture, which writes raw to
    ; QEMU's chardev). This keeps the boot honest on Bochs/86Box too.
    call serial_init

    ; -- Stage-1 marker "S1\n" ----------------------------------------------
    mov si, msg_s1
    call serial_puts

    ; -- Load stage2 via INT 13h CHS read -----------------------------------
    ; ES:BX = destination buffer (0x0000:0x8000).
    mov ax, STAGE2_SEG
    mov es, ax
    mov bx, STAGE2_OFF

    mov ah, 0x02               ; BIOS read sectors
    mov al, STAGE2_SECTORS     ; sector count
    mov ch, 0                  ; cylinder 0
    mov cl, STAGE2_LBA + 1     ; sector number is 1-based: LBA 1 -> S=2
    mov dh, 0                  ; head 0
    mov dl, [boot_drive]       ; BIOS boot drive
    int 0x13
    jc disk_error              ; CF set => read failed: fail loud

    ; -- Hand off to stage2 -------------------------------------------------
    mov si, msg_jmp
    call serial_puts
    jmp STAGE2_SEG:STAGE2_OFF

; ---------------------------------------------------------------------------
; disk_error -- fail loud over serial, then halt (Rule 2). No silent retry.
; ---------------------------------------------------------------------------
disk_error:
    mov si, msg_err
    call serial_puts
.halt:
    hlt
    jmp .halt

; ---------------------------------------------------------------------------
; serial_init -- program COM1 for 115200 8N1, no interrupts. Ref: 16550 UART.
; ---------------------------------------------------------------------------
serial_init:
    push ax
    push dx
    mov dx, COM1 + 1           ; IER
    mov al, 0x00
    out dx, al                 ; disable interrupts
    mov dx, COM1 + 3           ; LCR
    mov al, 0x80
    out dx, al                 ; DLAB = 1 (access divisor latch)
    mov dx, COM1 + 0           ; DLL
    mov al, 0x01
    out dx, al                 ; divisor low = 1 (115200 baud)
    mov dx, COM1 + 1           ; DLM
    mov al, 0x00
    out dx, al                 ; divisor high = 0
    mov dx, COM1 + 3           ; LCR
    mov al, 0x03
    out dx, al                 ; DLAB = 0, 8 bits, no parity, 1 stop
    mov dx, COM1 + 2           ; FCR
    mov al, 0xC7
    out dx, al                 ; enable+clear FIFOs, 14-byte trigger
    mov dx, COM1 + 4           ; MCR
    mov al, 0x0B
    out dx, al                 ; DTR, RTS, OUT2
    pop dx
    pop ax
    ret

; ---------------------------------------------------------------------------
; serial_putc -- AL = byte. Polls LSR THRE (bit 5) then writes. Clobbers none.
; ---------------------------------------------------------------------------
serial_putc:
    push ax
    push dx
    mov ah, al                 ; stash byte
.wait:
    mov dx, COM1 + 5           ; LSR
    in al, dx
    test al, 0x20              ; THRE: transmit holding register empty
    jz .wait
    mov dx, COM1 + 0           ; THR
    mov al, ah
    out dx, al
    pop dx
    pop ax
    ret

; ---------------------------------------------------------------------------
; serial_puts -- SI = NUL-terminated string. Clobbers nothing caller cares of.
; ---------------------------------------------------------------------------
serial_puts:
    push si
    push ax
.next:
    lodsb
    test al, al
    jz .done
    call serial_putc
    jmp .next
.done:
    pop ax
    pop si
    ret

; -- Data -------------------------------------------------------------------
boot_drive: db 0
msg_s1:  db "S1", 0x0A, 0
msg_jmp: db "JMP2", 0x0A, 0
msg_err: db "ERR-DISK", 0x0A, 0

; -- Boot signature ---------------------------------------------------------
times 510 - ($ - $$) db 0
dw 0xAA55
