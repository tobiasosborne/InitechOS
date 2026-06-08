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

; -- Handoff contract (boot_info.h / docs/research/boot-to-text-ground-truth.md
;    Sec 2-3). All physical low-memory addresses; flat == physical.
BOOT_INFO_ADDR  equ 0x0500      ; 24-byte boot_info struct (above the BDA)
FONT_STASH      equ 0x1000      ; 4096-byte VGA ROM 8x16 font copy

; -- Kernel load (INT 13h CHS). Image layout: MBR s0, stage2 s1..16, kernel
;    s17.. (Makefile). KERNEL_SECTORS is generous + deterministic (the Makefile
;    pads the kernel binary to exactly this many sectors). CHS geometry matches
;    what SeaBIOS presents for the raw image (the MBR already reads track 0).
KERNEL_SECTORS    equ 64        ; 64 * 512 = 32 KiB kernel window
KERNEL_LBA        equ 17        ; first kernel sector (1+16)
; SPT / heads are QUERIED at runtime via INT 13h AH=08h (geometry varies by
; emulator + image size); see the kernel-load block below.

start:
    cli
    ; Save the BIOS boot drive. The MBR far-jumps here with DL still holding
    ; [boot_drive] (it loaded DL for its own INT 13h read just before the jump,
    ; mbr.asm:77,84) -- BIOS convention: DL = boot drive at each boot stage.
    ; We need it for the kernel-load INT 13h below (Sec 3.3 of the brief).
    mov [boot_drive_s2], dl
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

    ; -- VGA ROM 8x16 font capture (real mode; MUST precede CR0.PE=1) -------
    ; Ref: IBM VGA Tech Ref / RBIL INT 10h AX=1130h BH=06h -> ES:BP points at
    ;      the 256*16-byte 8x16 ROM font (1 byte/row, MSB=leftmost pixel);
    ;      docs/research/boot-to-text-ground-truth.md Sec 1.1-1.3. INT 10h is a
    ;      BIOS service, unavailable after the PM switch, so it goes here.
    ;      Copy 4096 bytes to FONT_STASH (0x1000). Fail loud if ES:BP==0:0
    ;      (Rule 2; ground-truth Sec 1.4).
    mov ax, 0x1130
    mov bh, 0x06                ; 8x16 ROM BIOS font selector
    int 0x10
    ; ES:BP now = first byte of the 8x16 font table. Guard a null pointer.
    mov ax, es
    or ax, bp
    jz .err_font                ; ES:BP == 0:0 => font not available, fail loud
    ; Copy 4096 bytes. rep movsb copies [DS:SI] -> [ES:DI], so:
    ;   source = font ROM at fontseg:BP -> DS = ES(font), SI = BP
    ;   dest   = 0:FONT_STASH          -> ES = 0,         DI = FONT_STASH
    mov ax, es
    mov ds, ax                  ; DS = font segment returned by INT 10h
    mov si, bp                  ; SI = font offset
    xor ax, ax
    mov es, ax                  ; ES = 0 (flat low-memory destination)
    mov di, FONT_STASH
    mov cx, 4096
    cld
    rep movsb
    ; Restore DS=0 and ES=0 for the subsequent boot_info writes + BIOS calls.
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov si, msg_font
    call serial_puts
    jmp .font_done

.err_font:
    xor ax, ax
    mov ds, ax
    mov si, msg_err_font
    call serial_puts
.font_halt:
    hlt
    jmp .font_halt

.font_done:

    ; -- Build boot_info at BOOT_INFO_ADDR (0x500), real mode, DS=0 ---------
    ; 24-byte struct, uint32 fields IN ORDER (boot_info.h / ground-truth Sec 2):
    ;   lfb_addr, lfb_pitch, lfb_bpp, lfb_width, lfb_height, font_addr.
    ; lfb_addr/pitch/bpp were captured by vbe_setup. width/height use the
    ; WANT_W/WANT_H constants -- valid because vbe_setup matched the mode to
    ; them before proceeding (stage2.asm XResolution/YResolution checks).
    xor ax, ax
    mov ds, ax
    mov eax, [lfb_addr]
    mov [BOOT_INFO_ADDR + 0],  eax     ; lfb_addr
    mov eax, [lfb_pitch]
    mov [BOOT_INFO_ADDR + 4],  eax     ; lfb_pitch
    mov eax, [lfb_bpp]
    mov [BOOT_INFO_ADDR + 8],  eax     ; lfb_bpp
    mov dword [BOOT_INFO_ADDR + 12], WANT_W       ; lfb_width
    mov dword [BOOT_INFO_ADDR + 16], WANT_H       ; lfb_height
    mov dword [BOOT_INFO_ADDR + 20], FONT_STASH   ; font_addr

    ; -- Load the flat C kernel into 0x10000 (real mode, INT 13h CHS) -------
    ; Image layout: MBR s0, stage2 s1..16, kernel s17.. (Makefile). We read
    ; KERNEL_SECTORS sectors starting at LBA 17 into 0x1000:0x0000 (physical
    ; 0x10000). Ref: ground-truth Sec 3.1-3.3 (link/load at 0x10000; INT 13h is
    ; real-mode-only so it precedes the PM switch).
    ;
    ; The disk geometry that SeaBIOS/Bochs/86Box assign to a raw image is NOT
    ; fixed (a tiny image gets a tiny geometry; a 1.44M-shaped one gets 18/2).
    ; Hardcoding SPT=18/heads=2 fails when the BIOS picks something else, so we
    ; QUERY it with INT 13h AH=08h (Get Drive Parameters): on return CL[5:0] =
    ; max sector (== sectors/track), and DH = max head (heads = DH+1). This is
    ; portable across the tri-emulator set (Rule 5). KERNEL_SECTORS may cross a
    ; track boundary, so we read one track-chunk at a time, advancing CHS.
    push es                 ; AH=08h clobbers ES:DI (returns a DPT pointer)
    mov ah, 0x08
    mov dl, [boot_drive_s2]
    xor di, di
    mov es, di              ; ES:DI = 0:0 per the call's quirk guidance
    int 0x13
    pop es
    jc .err_kernel          ; cannot read geometry -> fail loud (Rule 2)
    ; SPT = CL & 0x3F (low 6 bits). heads = DH + 1.
    mov al, cl
    and al, 0x3F
    xor ah, ah
    mov [spt], ax
    movzx ax, dh
    inc ax
    mov [num_heads], ax
    ; Guard against a degenerate geometry (SPT==0) that would div-by-zero.
    cmp word [spt], 0
    je .err_kernel

    mov word [kload_remaining], KERNEL_SECTORS
    mov word [kload_lba], 17           ; first kernel LBA
    mov word [kload_seg], 0x1000       ; ES base for physical 0x10000

.kload_loop:
    mov ax, [kload_remaining]
    test ax, ax
    jz .kload_done
    ; Convert kload_lba -> CHS using the QUERIED geometry:
    ;   sector   = (LBA mod SPT) + 1
    ;   head     = (LBA / SPT) mod HEADS
    ;   cylinder = (LBA / SPT) / HEADS
    mov ax, [kload_lba]
    xor dx, dx
    div word [spt]          ; ax = LBA/SPT, dx = LBA mod SPT
    mov cl, dl
    inc cl                  ; CL = sector (1-based)
    ; ax = LBA / SPT ; split into head and cylinder
    xor dx, dx
    div word [num_heads]    ; ax = cylinder, dx = head
    mov ch, al              ; CH = cylinder low 8 bits (image is tiny: <256 cyl)
    mov dh, dl              ; DH = head
    ; How many sectors can we read in this track without crossing its end?
    ;   per_track_left = SPT - (sector-1) = SPT - (CL-1)
    mov ax, [spt]
    movzx bx, cl
    dec bx
    sub ax, bx              ; AX = sectors left in this track
    mov bx, ax              ; BX = per_track_left
    ; clamp to remaining
    mov ax, [kload_remaining]
    cmp ax, bx
    jbe .kload_count_ok
    mov ax, bx             ; ax = min(remaining, per_track_left)
.kload_count_ok:
    mov [kload_chunk], ax  ; chunk sector count for this read
    ; Set up ES:BX destination, then INT 13h read.
    push ax                ; preserve chunk count across reg loads
    mov bx, [kload_seg]
    mov es, bx
    xor bx, bx             ; ES:BX = seg:0
    pop ax                 ; AL = chunk count (count <= 18, fits in AL)
    mov ah, 0x02           ; BIOS read sectors
    ; CH/CL/DH already set (cylinder/sector/head); set drive.
    mov dl, [boot_drive_s2]
    int 0x13
    jc .err_kernel         ; CF set => read failed: fail loud (Rule 2)
    ; Advance: LBA += chunk, remaining -= chunk, seg += chunk*512/16 = chunk*32.
    mov ax, [kload_chunk]
    add [kload_lba], ax
    sub [kload_remaining], ax
    mov bx, ax
    shl bx, 5              ; bx = chunk * 32 (paragraphs per sector = 512/16)
    add [kload_seg], bx
    jmp .kload_loop

.err_kernel:
    mov si, msg_err_kernel
    call serial_puts
.kernel_halt:
    hlt
    jmp .kernel_halt

.kload_done:
    mov si, msg_kload
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

    ; Marker "LFB\n" -- LFB params are captured + handed off; the C kernel now
    ; owns the framebuffer fill (initech-d00 CONSTRAINT: the kernel re-paints
    ; seafoam so the screendump oracle stays green).
    mov esi, msg_lfb32
    call serial_puts32

    ; Marker "OK\n" -- stage2 done: boot_info built, font stashed, kernel loaded,
    ; protected/flat mode entered. Hand control to the flat C kernel.
    mov esi, msg_ok32
    call serial_puts32

    ; -- Far-jump into the flat C kernel at physical 0x00010000 ------------
    ; Ref: ground-truth Sec 3 (kernel linked + loaded at 0x10000). CS is
    ; already CODE_SEL (0x08) from the PM far jump; an intra-segment jump to a
    ; far address is fine, but we use a far jump to be explicit about the
    ; selector. The kernel's kstart.asm _start sets its own ESP + calls
    ; kernel_main; it never returns.
    jmp dword CODE_SEL:0x00010000

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

; Boot drive (DL at stage2 entry; used by the kernel-load INT 13h).
boot_drive_s2: db 0

; Kernel-load (multi-track INT 13h CHS) state.
kload_remaining: dw 0    ; sectors still to read
kload_lba:       dw 0    ; current LBA
kload_seg:       dw 0    ; current ES destination segment
kload_chunk:     dw 0    ; sectors read in the current track-chunk
spt:             dw 0    ; sectors/track (queried via INT 13h AH=08h)
num_heads:       dw 0    ; number of heads (queried via INT 13h AH=08h)

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
msg_font:        db "FONT", 0x0A, 0
msg_err_font:    db "ERR-FONT", 0x0A, 0
msg_kload:       db "KLOAD", 0x0A, 0
msg_err_kernel:  db "ERR-KERNEL", 0x0A, 0
