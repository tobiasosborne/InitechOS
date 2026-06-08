; stage2.asm -- InitechOS tracer boot, stage2: VBE LFB + real->protected/flat.
;
; beads: initech-f8v.2 ("Tracer boot ..."). Shared with M1.
; Ref:   PRD Sec 5 (hardware contract): "VBE 2.0 linear framebuffer,
;        640x480 ... x8/x32", "boot in real mode, switch to 32-bit protected,
;        flat segmentation"; "Custom MBR -> stage2".
;        PRD Sec 11 (M1): "VESA LFB".
;        CLAUDE.md hallucination callout, verbatim: "Real mode -> 32-bit
;        protected/flat is a minefield. A20, GDT, the PE bit, the far jump to
;        flush the prefetch queue -- use a known-good sequence and verify in
;        the Bochs debugger ... A triple-fault in QEMU silently reboots; turn
;        on -d int,guest_errors,cpu_reset and watch for it." (The harness does.)
;        CLAUDE.md Rule 2 (fail loud), Rule 12 (ASCII only).
;        VBE: VESA BIOS Extensions 2.0/3.0 (INT 10h AX=4F00/4F01/4F02).
;        A20: fast A20 via system control port 0x92 (documented below).
;
; Loaded by the MBR at 0x0000:0x8000 in 16-bit real mode. We do all BIOS work
; (VBE is real-mode only) FIRST, then switch to 32-bit flat protected mode and
; fill the linear framebuffer with the seafoam color.
;
; Serial markers (PRD acceptance: "serial marks each boot stage"):
;   S2   -- stage2 entered
;   VBE  -- a suitable 640x480 LFB mode was found and set
;   A20  -- A20 gate enabled
;   GDT  -- flat GDT loaded
;   PM   -- protected mode entered (printed from 32-bit code, raw COM1 write)
;   LFB  -- about to fill the framebuffer (32-bit)
;   OK   -- framebuffer filled; entering the live hlt-loop
; Any failure path prints a loud marker (e.g. "ERR-VBE") and halts.

bits 16
org 0x8000

; -- Constants --------------------------------------------------------------
COM1            equ 0x3F8

; Seafoam: a single documented constant. Classic System-7-ish seafoam/teal.
; SEAFOAM_RGB = (0x6F, 0xA0, 0x8E) -> R=0x6F G=0xA0 B=0x8E.
; PLACEHOLDER (per task brief): to be reconciled later with the extracted
; palette sheet (beads initech-vcq / m0-5). For the tracer boot a fixed
; constant is sufficient. The 32-bit fill below packs this per the mode bpp.
SEAFOAM_R       equ 0x6F
SEAFOAM_G       equ 0xA0
SEAFOAM_B       equ 0x8E

; Desired VESA resolution.
WANT_W          equ 640
WANT_H          equ 480

; VBE info buffers (in low memory, below stage2 load addr).
VBE_CTRL_BUF    equ 0x7000      ; 512-byte VbeInfoBlock
VBE_MODE_BUF    equ 0x7200      ; 256-byte ModeInfoBlock

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00             ; reuse the MBR stack region (we replaced MBR's
                               ; role; 0x7C00 downward is free real-mode stack)
    sti

    mov si, msg_s2
    call serial_puts

    ; -- VBE: find + set a 640x480 linear-framebuffer mode -----------------
    call vbe_setup            ; on success: lfb_addr/lfb_pitch/lfb_bpp set
    ; (vbe_setup fails loud + halts on error, never returns on failure)

    mov si, msg_vbe
    call serial_puts

    ; -- A20 enable (fast A20 via port 0x92) -------------------------------
    ; Decision (Law 1): fast A20 via the PS/2 system control port 0x92 is
    ; sufficient and reliable under QEMU/Bochs. We read-modify-write so we set
    ; bit 1 (A20) without toggling bit 0 (fast reset) which would reboot.
    in al, 0x92
    test al, 0x02
    jnz .a20_done             ; already on
    or al, 0x02
    and al, 0xFE              ; never set bit0 (fast CPU reset)
    out 0x92, al
.a20_done:
    mov si, msg_a20
    call serial_puts

    ; -- Load the flat GDT --------------------------------------------------
    cli
    lgdt [gdt_descriptor]
    mov si, msg_gdt
    call serial_puts

    ; -- Set CR0.PE and far-jump to flush the prefetch queue ---------------
    mov eax, cr0
    or eax, 0x00000001        ; PE = 1
    mov cr0, eax
    jmp dword CODE_SEL:pm_entry  ; far jump -> CS = 0x08, flushes prefetch

; ===========================================================================
; vbe_setup -- query VBE controller, walk the mode list, find a 640x480 mode
; with a linear framebuffer at 32bpp (preferred) or 24bpp, set it with the LFB
; bit, and record lfb_addr / lfb_pitch / lfb_bpp. Fails loud + halts on error.
; ===========================================================================
vbe_setup:
    ; 1. VbeInfoBlock: request VBE 2.0 info ("VBE2" signature in the buffer).
    mov ax, VBE_CTRL_BUF >> 4    ; segment for the buffer (es:di)
    ; We placed buffers at flat low addresses; use es=0, di=offset instead.
    xor ax, ax
    mov es, ax
    mov di, VBE_CTRL_BUF
    ; Tag the buffer with "VBE2" so the BIOS returns VBE 2.0 fields (incl. the
    ; OEM/total-memory and the mode list as a real far pointer).
    mov dword [es:di], 0x32454256 ; "VBE2" little-endian: 'V''B''E''2'
    mov ax, 0x4F00
    int 0x10
    cmp ax, 0x004F
    jne .err
    ; Check "VESA" signature at buffer+0.
    mov eax, [es:VBE_CTRL_BUF]
    cmp eax, 0x41534556          ; "VESA"
    jne .err

    ; 2. VbeInfoBlock.VideoModePtr is a far pointer at offset 0x0E (off:seg).
    mov ax, [es:VBE_CTRL_BUF + 0x0E]   ; offset
    mov [mode_ptr_off], ax
    mov ax, [es:VBE_CTRL_BUF + 0x10]   ; segment
    mov [mode_ptr_seg], ax

.scan:
    ; Read next mode word from [mode_ptr_seg:mode_ptr_off].
    mov ax, [mode_ptr_seg]
    mov fs, ax
    mov si, [mode_ptr_off]
    mov cx, [fs:si]              ; mode number
    cmp cx, 0xFFFF
    je .err                      ; end of list, no suitable mode -> fail loud
    add word [mode_ptr_off], 2
    mov [cur_mode], cx

    ; 3. Query ModeInfoBlock for this mode.
    xor ax, ax
    mov es, ax
    mov di, VBE_MODE_BUF
    mov ax, 0x4F01
    mov cx, [cur_mode]
    int 0x10
    cmp ax, 0x004F
    jne .scan                    ; couldn't query -> skip

    ; ModeAttributes (offset 0x00): bit0 supported, bit4 graphics, bit7 LFB.
    mov ax, [es:VBE_MODE_BUF + 0x00]
    test ax, 0x0001              ; mode supported in hardware?
    jz .scan
    test ax, 0x0080              ; linear framebuffer available?
    jz .scan

    ; XResolution (0x12), YResolution (0x14).
    mov ax, [es:VBE_MODE_BUF + 0x12]
    cmp ax, WANT_W
    jne .scan
    mov ax, [es:VBE_MODE_BUF + 0x14]
    cmp ax, WANT_H
    jne .scan

    ; BitsPerPixel (0x19, byte). Accept 32 (preferred) or 24.
    mov al, [es:VBE_MODE_BUF + 0x19]
    cmp al, 32
    je .good
    cmp al, 24
    je .good
    jmp .scan

.good:
    ; Record bpp.
    mov al, [es:VBE_MODE_BUF + 0x19]
    movzx eax, al
    mov [lfb_bpp], eax

    ; Pitch (BytesPerScanLine) at offset 0x10 (word). With VBE 2.0+ and the
    ; LFB set, the linear pitch is this field (LinBytesPerScanLine at 0x32 is
    ; identical for these BIOSes; 0x10 is universally populated).
    movzx eax, word [es:VBE_MODE_BUF + 0x10]
    mov [lfb_pitch], eax

    ; PhysBasePtr (offset 0x28, dword): the linear framebuffer address.
    mov eax, [es:VBE_MODE_BUF + 0x28]
    mov [lfb_addr], eax

    ; 4. Set the mode with the LFB bit (0x4000) set.
    mov ax, 0x4F02
    mov bx, [cur_mode]
    or bx, 0x4000                ; request linear framebuffer
    int 0x10
    cmp ax, 0x004F
    jne .err
    ret

.err:
    mov si, msg_err_vbe
    call serial_puts
.halt:
    hlt
    jmp .halt

; ---------------------------------------------------------------------------
; 16-bit serial helpers (real UART; COM1 already inited by the MBR).
; ---------------------------------------------------------------------------
serial_putc:
    push ax
    push dx
    mov ah, al
.wait:
    mov dx, COM1 + 5
    in al, dx
    test al, 0x20
    jz .wait
    mov dx, COM1 + 0
    mov al, ah
    out dx, al
    pop dx
    pop ax
    ret

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

; ===========================================================================
; 32-bit protected/flat entry
; ===========================================================================
bits 32
pm_entry:
    ; Set all data segments to the flat data selector (0x10).
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x00090000        ; flat stack at 576 KiB (well below the LFB,
                               ; above our code/buffers, conventional free RAM)

    ; Marker "PM\n" -- raw COM1 write (FIFO has room after the real-mode init).
    mov esi, msg_pm32
    call serial_puts32

    ; Marker "LFB\n" before the fill.
    mov esi, msg_lfb32
    call serial_puts32

    ; -- Fill the entire LFB with seafoam ----------------------------------
    ; Total bytes = pitch * height. We fill pitch*HEIGHT bytes so trailing
    ; padding in each scanline is covered too (harmless, all seafoam).
    mov eax, [lfb_pitch]
    mov ebx, WANT_H
    mul ebx                    ; edx:eax = pitch * height (fits in 32 bits)
    mov ecx, eax               ; ecx = total byte count

    mov edi, [lfb_addr]        ; destination = physical LFB (flat == phys)

    ; Branch on bpp: 32 -> dword stores; 24 -> 3-byte stores.
    mov eax, [lfb_bpp]
    cmp eax, 32
    je .fill32
    ; -- 24bpp fill: write B,G,R bytes per pixel ---------------------------
    ; pixel count = total / 3
    mov eax, ecx
    xor edx, edx
    mov ebx, 3
    div ebx                    ; eax = pixel count
    mov ecx, eax
.fill24:
    mov byte [edi + 0], SEAFOAM_B
    mov byte [edi + 1], SEAFOAM_G
    mov byte [edi + 2], SEAFOAM_R
    add edi, 3
    dec ecx
    jnz .fill24
    jmp .filled

.fill32:
    ; 32bpp pixel = 0x00RRGGBB (XRGB8888; X/alpha byte = 0).
    mov eax, (SEAFOAM_R << 16) | (SEAFOAM_G << 8) | SEAFOAM_B
    shr ecx, 2                 ; dword count = bytes / 4
    rep stosd

.filled:
    ; Final marker "OK\n".
    mov esi, msg_ok32
    call serial_puts32

    ; Stay alive so the harness QMP screendump captures the LIVE framebuffer.
    ; Do NOT isa-debug-exit here (that would kill the guest before/at capture).
.hang:
    hlt
    jmp .hang

; ---------------------------------------------------------------------------
; 32-bit serial helpers. The UART is already programmed (real-mode init); we
; poll LSR THRE and write THR directly. Flat segments, so [esi] is physical.
; ---------------------------------------------------------------------------
serial_putc32:
    push eax
    push edx
    mov ah, al
.wait:
    mov dx, COM1 + 5
    in al, dx
    test al, 0x20
    jz .wait
    mov dx, COM1 + 0
    mov al, ah
    out dx, al
    pop edx
    pop eax
    ret

serial_puts32:
    push esi
    push eax
.next:
    mov al, [esi]
    test al, al
    jz .done
    call serial_putc32
    inc esi
    jmp .next
.done:
    pop eax
    pop esi
    ret

; ===========================================================================
; GDT -- flat model. null, 32-bit code (0x08), 32-bit data (0x10).
; base 0, limit 0xFFFFF with G=1 (4 KiB granularity) => 4 GiB.
; ===========================================================================
align 8
gdt_start:
    ; null descriptor
    dq 0x0000000000000000
gdt_code:
    ; base=0 limit=0xFFFFF, access=0x9A (present, ring0, code, exec/read),
    ; flags=0xC (G=1 4KiB, D/B=1 32-bit), high limit nibble=0xF.
    dw 0xFFFF        ; limit 15:0
    dw 0x0000        ; base 15:0
    db 0x00          ; base 23:16
    db 0x9A          ; access
    db 0xCF          ; flags(4) | limit 19:16
    db 0x00          ; base 31:24
gdt_data:
    ; base=0 limit=0xFFFFF, access=0x92 (present, ring0, data, read/write).
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1   ; limit (size - 1)
    dd gdt_start                 ; base (flat: linear == physical here)

CODE_SEL equ gdt_code - gdt_start    ; 0x08
DATA_SEL equ gdt_data - gdt_start    ; 0x10

; -- Data -------------------------------------------------------------------
; VBE scan state.
mode_ptr_off: dw 0
mode_ptr_seg: dw 0
cur_mode:     dw 0

; LFB parameters captured from the VBE mode-info block.
lfb_addr:  dd 0
lfb_pitch: dd 0
lfb_bpp:   dd 0

; Serial marker strings.
msg_s2:      db "S2", 0x0A, 0
msg_vbe:     db "VBE", 0x0A, 0
msg_a20:     db "A20", 0x0A, 0
msg_gdt:     db "GDT", 0x0A, 0
msg_err_vbe: db "ERR-VBE", 0x0A, 0
msg_pm32:    db "PM", 0x0A, 0
msg_lfb32:   db "LFB", 0x0A, 0
msg_ok32:    db "OK", 0x0A, 0
