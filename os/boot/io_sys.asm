; IO.SYS -- BIOS setup and the original real->protected/flat handoff.
; initech-kod: physical split, ADR-0003 DEC-01/08; IBM DOS 3.30 TR p1-7
; (PDF24). Stage2 loads IO at 0:9000 and INITDOS directly at 0x500000.
; DH=1 means the FAT loader has validated and loaded both complete files.
; DH must be 1; the raw-LBA17 entry has been removed.
; boot_info+36 contains the loaded byte count; the first 9 dwords are set here.
; A20/GDT/PE/far-jump follow BIOS setup; no whole-image copy remains.
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
; Loaded by stage2 at 0x0000:0x9000 in 16-bit real mode. We do all BIOS work
; (VBE is real-mode only) FIRST, then switch to 32-bit flat protected mode and
; hand off to the C kernel, which owns the framebuffer fill (the canonical
; Initech teal desktop; the pre-FLAIR text console paints its own background).
;
; Serial markers (PRD acceptance: "serial marks each boot stage"). The real
; success order is MBR S1/JMP2, then stage2 S2/VBE/FONT/E820|E801|E88/KLOAD/
; A20/GDT/PM/LFB/OK/KHI, then kernel KERNEL/KAT. Existing order is untouched;
; KHI/KAT are append-only additions (kernel-runway-relocation.md K2/K3; tdnl.29).
;   S2    -- stage2 entered
;   VBE   -- a suitable 640x480 LFB mode was found and set
;   FONT  -- VGA ROM font captured
;   E820  -- representative successful extended-memory probe marker
;   KLOAD -- stage2 completed the chunked high kernel load
;   A20   -- A20 gate enabled
;   GDT   -- flat GDT loaded
;   PM    -- protected mode entered (printed from 32-bit code, raw COM1 write)
;   LFB   -- framebuffer parameters are ready for the C kernel
;   OK    -- protected-mode handoff tail reached
;   KHI   -- checked kernel resident at 0x00500000
; Any failure path prints a loud marker (e.g. "ERR-VBE") and halts.

bits 16
%include "dos_messages.inc"
org 0x9000

; -- Constants --------------------------------------------------------------
COM1            equ 0x3F8

; Kernel high-runway constants. Ref: docs/design/kernel-runway-relocation.md
; K1/K2; bead initech-tdnl.29. These duplicate spec/memory_map.h because NASM
; cannot consume the C header. The FAT loader has populated KERNEL_BASE;
; this file sets up the BIOS handoff and makes the final far jump.
KERNEL_BASE        equ 0x00500000

; Initech desktop teal: the LOCKED Initech Color Canon desktop background
; (color_canon.json idx2 CIDX_DESKTOP). TEAL_RGB = (0x8D, 0xDC, 0xDC) ->
; R=0x8D G=0xDC B=0xDC. Supersedes the REVOKED OD-4 seafoam (0x6F,0xA0,0x8E)
; per ADR-0004-AMENDMENT-DEC-09 OD-4-REVOKED / FO-3 so no seafoam constant
; survives under the teal desktop. NOTE: stage2 no longer fills the LFB itself
; (the C kernel owns the framebuffer fill -- see the LFB marker below); these
; are kept as the documented canon constant for the boot color. R/G/B byte
; order preserved.
TEAL_R          equ 0x8D
TEAL_G          equ 0xDC
TEAL_B          equ 0xDC

; Desired VESA resolution.
WANT_W          equ 640
WANT_H          equ 480

; VBE info buffers (in low memory, below stage2 load addr).
VBE_CTRL_BUF    equ 0x7000      ; 512-byte VbeInfoBlock
VBE_MODE_BUF    equ 0x7200      ; 256-byte ModeInfoBlock

; INT 15h E820 SMAP scratch (24-byte entry buffer). Below the stage2 load addr
; (0x8000) and above the VBE ModeInfoBlock (0x7200..0x7300); 0x7400 is free
; real-mode low RAM. Sized 32 bytes (24 used + slack). (ADR-0004 DEC-03 probe.)
E820_BUF        equ 0x7400

; -- Handoff contract (boot_info.h / docs/research/boot-to-text-ground-truth.md
;    Sec 2-3). All physical low-memory addresses; flat == physical.
BOOT_INFO_ADDR  equ 0x0500      ; 40-byte boot_info struct (above the BDA)
                                ; append-only: 24->28 ext_mem_kb, then 28->32
                                ; kernel_base (kernel-runway-relocation.md K2;
                                ; bead initech-tdnl.29; boot_info.h)
FONT_STASH      equ 0x1000      ; 4096-byte VGA ROM 8x16 font copy

; Only the checked FAT stage2 entry is supported (DH=1, DL=boot drive).
; INITDOS is already at KERNEL_BASE; boot_info+36 holds its exact byte count.

start:
    cli
    ; Only stage2's checked FAT entry may reach the final BIOS/PM setup.
    cmp dh, 1
    je .fat_entry
    mov si, msg_err_kernel
    call serial_puts
.invalid_entry:
    cli
    hlt
    jmp .invalid_entry
.fat_entry:
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

    ; -- Probe installed extended memory (real mode; MUST precede CR0.PE=1) --
    ; Ref: ADR-0004 DEC-03 / FO-G -- stage2 records installed RAM above 1 MiB in
    ;      boot_info_t.ext_mem_kb so the kernel can fail loud below FLAIR_HEAP_MIN.
    ;      RBIL / ACPI: INT 15h AX=E820h (SMAP) is the modern map; E801h and
    ;      AH=88h are the period BIOS fallbacks. mem_probe leaves the KiB above
    ;      1 MiB in EAX (0 if every probe failed -- fail-safe, Rule 2). INT 15h is
    ;      a BIOS service, so the probe goes HERE, before the PM switch. DS=0/ES=0
    ;      are restored after the font copy above.
    call mem_probe                     ; -> EAX = ext_mem_kb (0 on total failure)
%ifdef DEC03A_REPORT_KB
    mov eax, DEC03A_REPORT_KB ; GATE-ONLY exact boundary, never production
%endif
    mov [ext_mem_kb], eax
    ; ADR-0009 Amendment DEC-03a (RATIFIED 2026-11-06), initech-ayj36.
    ; DEC-03a.6: refuse BEFORE copying to the 5 MiB kernel, not just in C.
    cmp eax, 7168
    jae .memory_ok
    mov ax, 3
    int 0x10
    xor ax, ax
    mov ds, ax
    mov si, msg_memory
    call serial_puts
    ; BIOS text output after selecting mode 3 (no allocated high state).
    mov si, msg_memory
.memory_text:
    lodsb
    test al, al
    jz .memory_values
    mov ah, 0x0e
    mov bx, 7
    int 0x10
    jmp .memory_text
.memory_values:
    mov si, msg_memory_report
    call serial_puts
    mov eax, [ext_mem_kb]
    call serial_decimal32
    mov si, msg_memory_required
    call serial_puts
    cli
.memory_halt:
    hlt
    jmp .memory_halt
.memory_ok:

    ; -- Build boot_info at BOOT_INFO_ADDR (0x500), real mode, DS=0 ---------
    ; 40-byte struct, uint32 fields IN ORDER (boot_info.h / ground-truth Sec 2):
    ;   lfb_addr, lfb_pitch, lfb_bpp, lfb_width, lfb_height, font_addr,
    ;   ext_mem_kb, kernel_base, lfb_dac_bits. The placement field remains at
    ;   +28; DAC precision at +32. Stage2 writes loaded_bytes at +36.
    ; lfb_addr/pitch/bpp were captured by vbe_setup. width/height use the
    ; WANT_W/WANT_H constants -- valid because vbe_setup matched the mode to
    ; them before proceeding (stage2.asm XResolution/YResolution checks).
    ; ext_mem_kb was probed by mem_probe just above (ADR-0004 DEC-03).
    xor ax, ax
    mov ds, ax
    mov eax, [lfb_addr]
    mov [BOOT_INFO_ADDR + 0],  eax     ; lfb_addr
    mov eax, [lfb_pitch]
    mov [BOOT_INFO_ADDR + 4],  eax     ; lfb_pitch
    mov eax, [lfb_bpp]
    mov [BOOT_INFO_ADDR + 8],  eax     ; lfb_bpp
    mov eax, [lfb_width]
    mov [BOOT_INFO_ADDR + 12], eax     ; lfb_width  (640 VBE / 320 mode-0x13)
    mov eax, [lfb_height]
    mov [BOOT_INFO_ADDR + 16], eax     ; lfb_height (480 VBE / 200 mode-0x13)
    mov dword [BOOT_INFO_ADDR + 20], FONT_STASH   ; font_addr
    mov eax, [ext_mem_kb]
    mov [BOOT_INFO_ADDR + 24], eax     ; ext_mem_kb (KiB above 1 MiB; DEC-03)
    mov dword [BOOT_INFO_ADDR + 28], KERNEL_BASE ; kernel_base (append-only)
    mov eax, [lfb_dac_bits]
    mov [BOOT_INFO_ADDR + 32], eax   ; negotiated DAC width (initech-2gva)

    ; Stage2 already checked and loaded the kernel in chunks.
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
; with a linear framebuffer: 32 -> 24 -> 8 (0x101) -> VGA13 floor.
; Ref: PRD Sec 5; ADR-0004 OD-2/AM-7; vesa8-ground-truth.md (VBE 2.0
; pp6,13-14,16-20,25,30). FORCE_VBE_101 filters the BIOS offer ONLY in gate
; IO.SYS images; the query/validation/set/handoff path is production code.
; ===========================================================================
vbe_setup:
    mov dword [lfb_dac_bits], 6  ; VBE mode sets reset the DAC to six bits
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
    jne .err00
    ; Check "VESA" signature at buffer+0.
    mov eax, [es:VBE_CTRL_BUF]
    cmp eax, 0x41534556          ; "VESA"
    jne .errsig

%ifdef FORCE_VBE_101
    mov byte [wanted_bpp], 8
%endif
%ifdef VBE_MUT_8_FIRST
    mov byte [wanted_bpp], 8
%endif
.restart:
    ; Rescan the BIOS list at each depth: BIOS enumeration order is NOT
    ; preference order. All modes in the higher tier must be tried first.
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
    je .next_depth
    add word [mode_ptr_off], 2
    mov [cur_mode], cx
%ifdef FORCE_VBE_101
    cmp cx, 0x101
    jne .scan
%endif

    ; 3. Query ModeInfoBlock for this mode.
    xor ax, ax
    mov es, ax
    mov di, VBE_MODE_BUF
    mov ax, 0x4F01
    mov cx, [cur_mode]
    int 0x10
    cmp ax, 0x004F
    jne .scan                    ; couldn't query -> skip
%ifdef FORCE_VBE_101
    mov si, msg_101_info
    call serial_puts
    mov ax, [es:VBE_MODE_BUF]
    call serial_hex16
    movzx ax, byte [es:VBE_MODE_BUF + 0x19]
    call serial_hex16
    movzx ax, byte [es:VBE_MODE_BUF + 0x1B]
    call serial_hex16
    mov ax, [es:VBE_CTRL_BUF + 0x0A]
    call serial_hex16
%endif

    ; ModeAttributes (offset 0x00): bit0 supported, bit4 graphics, bit7 LFB.
    mov ax, [es:VBE_MODE_BUF + 0x00]
    test ax, 0x0001              ; mode supported in hardware?
    jz .scan
    test ax, 0x0080              ; linear framebuffer available?
    jz .scan
    test ax, 0x0010              ; graphics mode?
    jz .scan

    ; XResolution (0x12), YResolution (0x14).
    mov ax, [es:VBE_MODE_BUF + 0x12]
    cmp ax, WANT_W
    jne .scan
    mov ax, [es:VBE_MODE_BUF + 0x14]
    cmp ax, WANT_H
    jne .scan

    ; A genuine 0x101 packed-pixel VGA-compatible LFB is the indexed tier.
    ; Non-VGA palette programming needs a different hardware backend.
    mov al, [es:VBE_MODE_BUF + 0x19]
    cmp al, [wanted_bpp]
    jne .scan
    cmp al, 8
    jne .direct
%ifdef VBE_MUT_NO_101
    jmp .scan
%endif
    cmp word [cur_mode], 0x101
    jne .scan
    ; D5 may mark the extended mode non-VGA (SeaBIOS returns 00BBh).
    ; Controller compatibility alone does not promise mode-level registers:
    ; the kernel verifies the ONLY registers it needs by DAC readback.
    test byte [es:VBE_CTRL_BUF + 0x0A], 2 ; controller must be VGA-compatible
    jnz .scan
    cmp byte [es:VBE_MODE_BUF + 0x1B], 4 ; packed pixels
    jne .scan
    jmp .good
.direct:
    cmp byte [es:VBE_MODE_BUF + 0x1B], 6 ; direct color
    jne .scan

.good:
    ; Reject invalid geometry metadata before it can reach the kernel.
    movzx ax, byte [wanted_bpp]
    shr ax, 3
    imul ax, WANT_W
    cmp [es:VBE_MODE_BUF + 0x10], ax
    jb .scan
    cmp dword [es:VBE_MODE_BUF + 0x28], 0
    je .scan
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
    jne .scan                    ; try the remaining modes / lower tiers
%ifdef FORCE_VBE_101
    ; Gate-only padding: real VBE 4F06h logical scanline (VBE 2.0 Sec 4.9,
    ; p28), not manufactured boot_info. Keep 640 visible pixels, but request
    ; 648 virtual pixels so the live row copy must honor the returned stride.
    mov ax, 0x4F06
    xor bx, bx
    mov cx, 648
    int 0x10
    cmp ax, 0x004F
    jne .err02
    movzx eax, bx
    mov [lfb_pitch], eax
%endif
    ; Optional eight-bit DAC precision (VBE 2.0 Sec 4.11, p30). Retain the
    ; authentic six-bit path on fixed-width hardware. BH is the ACTUAL width,
    ; not a promise that the requested eight bits were supported.
    cmp dword [lfb_bpp], 8
    jne .mode_ready
    test byte [VBE_CTRL_BUF + 0x0A], 1
    jz .mode_ready
    mov ax, 0x4F08
    mov bx, 0x0800
    int 0x10
    cmp ax, 0x004F
    jne .mode_ready
    cmp bh, 8
    jne .mode_ready
    mov dword [lfb_dac_bits], 8
.mode_ready:
    ; VBE 640x480 LFB is live: record the resolution (the kernel reads
    ; width/height from boot_info, not from WANT_W/WANT_H constants, so the
    ; mode-0x13 fallback below can report a different geometry).
    mov dword [lfb_width],  WANT_W
    mov dword [lfb_height], WANT_H
    mov si, msg_vbe_mode
    call serial_puts
    mov ax, [cur_mode]
    call serial_hex16
    ret

.next_depth:
    cmp byte [wanted_bpp], 32
    jne .try8
    mov byte [wanted_bpp], 24
    jmp .restart
.try8:
    cmp byte [wanted_bpp], 24
    jne .errnomode
    mov byte [wanted_bpp], 8
    jmp .restart

    ; -- VBE error paths: print the specific marker, then fall back to a
    ; standard VGA mode (Rule 2 -- a loud, recorded path change, not a halt).
.err00:
    mov si, msg_vbe_e00
    call serial_puts
    jmp .vga_fallback
.errsig:
    mov si, msg_vbe_esig
    call serial_puts
    jmp .vga_fallback
.errnomode:
    mov si, msg_vbe_enomode
    call serial_puts
    jmp .vga_fallback
.err02:
    mov si, msg_vbe_e02
    call serial_puts
    jmp .vga_fallback

; ---------------------------------------------------------------------------
; .vga_fallback -- no VBE LFB on this BIOS (e.g. Bochs). Set the universally
; available standard VGA mode 0x13 (320x200x256, linear @ 0xA0000) via INT 10h
; and record an 8bpp boot_info. Fail loud (Rule 2) if even mode 0x13's
; framebuffer is not live. Ref: bd memory bochs-boot-solved-initech-6pj;
; os/milton/console.c 8bpp path (initech-6pj).
; ---------------------------------------------------------------------------
.vga_fallback:
    mov ax, 0x0013               ; AH=00 set video mode -> 320x200x256
    int 0x10
    ; Probe: the linear framebuffer at 0xA0000 must accept a write (Rule 2).
    push es
    mov ax, 0xA000
    mov es, ax
    mov byte [es:0x0000], 0xA5
    mov al, [es:0x0000]
    mov byte [es:0x0000], 0x00   ; leave it clean (the kernel clears it anyway)
    pop es
    cmp al, 0xA5
    jne .err_vga
    mov dword [lfb_addr],   0x000A0000
    mov dword [lfb_pitch],  320
    mov dword [lfb_bpp],    8
    mov dword [lfb_width],  320
    mov dword [lfb_height], 200
    mov si, msg_vga13
    call serial_puts
    ret

.err_vga:
    mov si, msg_err_vga
    call serial_puts
.vga_halt:
    hlt
    jmp .vga_halt

; ===========================================================================
; mem_probe -- probe installed extended memory (KiB above the 1 MiB line).
; Returns the total in EAX (0 if every probe failed -- fail-safe; the kernel
; then PANICS below FLAIR_HEAP_MIN, ADR-0004 DEC-03 / FO-G). Real mode only.
;
; Strategy (most reliable first, with period-BIOS fallbacks; RBIL INT 15h):
;   1. AX=E820h (SMAP): walk sorted usable extents for contiguous coverage;
;      refuse known holes in required RAM (DEC-03a.6), never sum fragments.
;      Historical implementation summed TYPE-1 (usable)
;      ranges that lie at or above 0x100000. The authoritative modern map
;      (SeaBIOS/QEMU, Bochs LGPL BIOS). Emits "E820".
;   2. AX=E801h: AX/CX = KiB in the 1..16 MiB window; BX/DX = 64-KiB blocks
;      ABOVE 16 MiB. total_kib = AX + DX*64. Emits "E801".
;   3. AH=88h: AX = KiB above 1 MiB (capped ~64 MiB on real period BIOSes).
;      The last resort. Emits "E88".
; Each method falls through to the next on failure; total failure -> EAX=0.
;
; Verified to work on BOTH SeaBIOS (QEMU; E820 succeeds) and the Bochs LGPL
; BIOS (E820 succeeds there too; E801/88h remain as the period-hardware path).
; All scratch is real-mode (DS=ES=0 on entry, preserved on exit). ASCII-clean.
; ===========================================================================
mem_probe:
    push bx
    push cx
    push dx
    push si
    push di
    push bp

    ; ---- Method 1: INT 15h AX=E820h (SMAP) -----------------------------
    ; ES:DI -> a 24-byte buffer; EBX = continuation (0 to start); EDX = 'SMAP';
    ; ECX = buffer size (24). On each call AX=0x534D4150 ('SMAP'), CF clear,
    ; ECX = bytes stored. Entry: base(8) length(8) type(4) [ext-attr(4)].
    xor ax, ax
    mov es, ax
    mov di, E820_BUF
    xor ebx, ebx                 ; continuation = 0 (first call)
    xor ebp, ebp                 ; running total: KiB above 1 MiB (32-bit)
    mov dword [mem_low_covered], 0x500
    mov dword [mem_covered], 0x100000
    mov byte [mem_hole], 0
    xor si, si                   ; entry count (detect "no entries" failure)
.e820_loop:
    mov eax, 0x0000E820
    mov edx, 0x534D4150          ; 'SMAP'
    mov ecx, 24                  ; buffer size
    mov dword [es:di + 20], 1    ; force a valid ACPI-3 ext-attr (some BIOSes)
    int 0x15
    jc .e820_done                ; CF set on first call => unsupported
    cmp eax, 0x534D4150          ; AX must read back 'SMAP'
    jne .e820_done
    cmp ecx,20
    jb .e820_done               ; invalid partial SMAP data must refuse
    inc si                       ; count this entry
    cmp ecx,24
    jb .entry_enabled
    test byte [es:di+20],1
    jz .e820_next                ; ACPI disabled entry is not usable coverage
.entry_enabled:
    ; CRITICAL: EBX is the BIOS continuation token and MUST survive to the next
    ; INT 15h call -- so the per-entry math below NEVER touches EBX (it reads all
    ; fields straight from the buffer [es:di+...] and uses only EAX/EDX, which
    ; are reloaded at the top of the loop). EBP carries the running KiB total
    ; (preserved). type at [di+16] (dword): only TYPE 1 (usable) counts.
%ifdef DEC03A_KNOWN_HOLE
    ; GATE-ONLY: introduce a real map gap [1,2) MiB into the BIOS's usable
    ; extent. With 16 MiB installed, a sum still exceeds the 8 MiB floor.
    cmp dword [es:di],0x100000
    jne .hole_injected
    add dword [es:di],0x100000
    sub dword [es:di+8],0x100000
.hole_injected:
%endif
%ifdef DEC03A_LOW_HOLE
    ; GATE-ONLY: remove required conventional RAM from the first usable entry.
    cmp dword [es:di],0
    jne .low_hole_injected
    add dword [es:di],0x50000
    sub dword [es:di+8],0x50000
.low_hole_injected:
%endif
    ; DEC-03a.6: never trust a sum of disconnected TYPE-1 ranges.
    cmp dword [es:di+4], 0
    jne .e820_next               ; addresses >=4 GiB cannot back our windows
    mov eax, [es:di]
    mov edx, eax
    add edx, [es:di+8]
    jc .range_huge
    cmp dword [es:di+12], 0
    je .range_end
.range_huge:
    mov edx, 0xffffffff
.range_end:
    ; Required conventional control/load windows also need coverage. Ordinary
    ; PC E820 starts at zero; do not count the reserved VGA/ROM/EBDA aperture.
    cmp dword [es:di+16],1
    jne .reserved_range
    cmp eax,[mem_low_covered]
    ja .usable_high
    cmp edx,[mem_low_covered]
    jbe .usable_high
    mov [mem_low_covered],edx
.usable_high:
    cmp eax,[mem_covered]
    ja .e820_next
    cmp edx,[mem_covered]
    jbe .e820_next
    mov [mem_covered],edx
    jmp .e820_next
.reserved_range:
    cmp eax,0x90000
    jae .reserved_high
    cmp edx,0x500
    ja .required_hole
.reserved_high:
    cmp eax,0x800000
    jae .e820_next
    cmp edx,0x100000
    jbe .e820_next
.required_hole:
    mov byte [mem_hole],1
.e820_next:
    test ebx, ebx                ; EBX==0 => that was the last entry
    jz .e820_finish
    jmp .e820_loop
.e820_finish:
    test si, si                  ; got at least one entry?
    jz .e820_done                ; zero entries => treat as failure, fall back
    mov si, msg_e820
    call serial_puts
    mov eax, [mem_covered]
    sub eax, 0x100000
    shr eax, 10
    cmp dword [mem_low_covered],0x90000
    jb .coverage_bad
    cmp byte [mem_hole], 0
    je .coverage_ok
.coverage_bad:
    xor eax, eax                ; known hole is a refusal, never BIOS fallback
.coverage_ok:
    jmp .mp_done
.e820_done:
    ; Partial/invalid maps must not erase known holes through a fallback.
    test si,si
    jz .no_e820
    xor eax,eax
    jmp .mp_done
.no_e820:

    ; ---- Method 2: INT 15h AX=E801h ------------------------------------
    ; Ref: RBIL INT 15h AX=E801h. Returns TWO register pairs reporting the same
    ; thing in two unit ranges:
    ;   AX/CX = configured extended memory 1..16 MiB, in KiB
    ;   BX/DX = configured extended memory above 16 MiB, in 64-KiB blocks
    ; The canonical consumer rule: use CX/DX if CX is non-zero (the BIOS-filled
    ; pair on most machines), else fall back to AX/BX. total_kib = lo + hi*64.
    xor cx, cx                   ; pre-zero so a BIOS that sets only AX/BX is
    xor dx, dx                   ; detected (CX stays 0 -> use AX/BX)
    mov ax, 0xE801
    int 0x15
    jc .e801_done
    ; Choose the populated pair: CX/DX if CX != 0, else AX/BX.
    test cx, cx
    jz .e801_use_axbx
    mov ax, cx                   ; lo = KiB in 1..16 MiB  (from CX)
    mov bx, dx                   ; hi = 64-KiB blocks above 16 MiB (from DX)
.e801_use_axbx:
    ; total_kib = AX + BX*64. Build in EBP (32-bit) to avoid 16-bit overflow.
    movzx ebp, ax                ; KiB in 1..16 MiB
    movzx edx, bx                ; 64-KiB blocks above 16 MiB
    shl edx, 6                   ; *64 -> KiB
    add ebp, edx
    test ebp, ebp
    jz .e801_done                ; both zero => failure, fall back to AH=88h
    mov si, msg_e801
    call serial_puts
    mov eax, ebp
    jmp .mp_done
.e801_done:

    ; ---- Method 3: INT 15h AH=88h --------------------------------------
    ; Returns AX = KiB of extended memory above 1 MiB (capped ~63/64 MiB on
    ; period BIOSes). CF set or AX==0 => unavailable.
    mov ax, 0x8800               ; AH=0x88, AL=0 (AL ignored by this call)
    int 0x15
    jc .e88_done
    test ax, ax
    jz .e88_done
    mov si, msg_e88
    call serial_puts
    movzx eax, ax                ; EAX = KiB above 1 MiB
    jmp .mp_done
.e88_done:

    ; ---- Total failure: report 0 (fail-safe; kernel panics below min) --
    mov si, msg_emem_fail
    call serial_puts
    xor eax, eax

.mp_done:
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    ret

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

serial_decimal32:
    xor cx,cx
    mov ebx,10
.divide:
    xor edx,edx
    div ebx
    push dx
    inc cx
    test eax,eax
    jnz .divide
.digit:
    pop ax
    add al,'0'
    call serial_putc
    loop .digit
    ret

; Actual selected mode, four ASCII hex digits; serial diagnostics only.
serial_hex16:
    push cx
    mov cx, 4
.digit:
    rol ax, 4
    push ax
    and al, 15
    add al, '0'
    cmp al, '9'
    jbe .emit
    add al, 7
.emit:
    call serial_putc
    pop ax
    loop .digit
    mov al, 10
    call serial_putc
    pop cx
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

    ; Stage2's INT 15h/87h sink has already populated the high image.
    mov esi, msg_khi32
    call serial_puts32

    ; -- Far-jump into the flat C kernel at physical KERNEL_BASE ------------
    ; Ref: kernel-runway-relocation.md K2 / bead initech-tdnl.29. CS is
    ; already CODE_SEL (0x08) from the PM far jump; an intra-segment jump to a
    ; far address is fine, but we use a far jump to be explicit about the
    ; selector. The kernel's kstart.asm _start sets its own ESP + calls
    ; kernel_main; it never returns.
    jmp dword CODE_SEL:KERNEL_BASE

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
wanted_bpp:   db 32

; LFB parameters captured from the VBE mode-info block, OR set by the standard
; VGA mode-0x13 fallback (.vga_fallback) when no VBE LFB is available.
lfb_addr:   dd 0
lfb_pitch:  dd 0
lfb_bpp:    dd 0
lfb_width:  dd 0
lfb_height: dd 0
lfb_dac_bits: dd 6

; Extended-memory probe result (KiB above 1 MiB), filled by mem_probe; written
; to boot_info_t.ext_mem_kb. 0 means every INT 15h probe failed (ADR-0004
; DEC-03 / FO-G; the kernel panics below FLAIR_HEAP_MIN).
mem_low_covered: dd 0
mem_covered: dd 0
mem_hole: db 0
msg_memory: db "PC LOAD LETTER  (", MSG_DOS_0035, ")",13,10,0
msg_memory_report: db "  ext_mem_kb=",0
msg_memory_required: db " required_kb=7168",10,"HALTED",10,0
ext_mem_kb: dd 0

; Serial marker strings.
msg_s2:      db "S2", 0x0A, 0
msg_vbe:     db "VBE", 0x0A, 0
msg_vbe_mode: db "VBE-MODE=", 0
%ifdef FORCE_VBE_101
msg_101_info: db "VBE101-INFO attr/bpp/model/caps", 10, 0
%endif
msg_a20:     db "A20", 0x0A, 0
msg_gdt:     db "GDT", 0x0A, 0
msg_err_vbe: db "ERR-VBE", 0x0A, 0
msg_vbe_e00:     db "VBE-E00", 0x0A, 0
msg_vbe_esig:    db "VBE-ESIG", 0x0A, 0
msg_vbe_enomode: db "VBE-ENOMODE", 0x0A, 0
msg_vbe_e02:     db "VBE-E02", 0x0A, 0
msg_vga13:       db "VGA13", 0x0A, 0
msg_err_vga:     db "ERR-VGA", 0x0A, 0
msg_pm32:    db "PM", 0x0A, 0
msg_lfb32:   db "LFB", 0x0A, 0
msg_ok32:    db "OK", 0x0A, 0
msg_khi32:   db "KHI", 0x0A, 0
msg_font:        db "FONT", 0x0A, 0
msg_err_font:    db "ERR-FONT", 0x0A, 0
msg_kload:       db "KLOAD", 0x0A, 0
msg_err_kernel:  db "ERR-KERNEL", 0x0A, 0
; Extended-memory probe markers (mem_probe; ADR-0004 DEC-03). Exactly one of the
; first three (or EMEM-FAIL) prints, recording which INT 15h method succeeded.
msg_e820:        db "E820", 0x0A, 0
msg_e801:        db "E801", 0x0A, 0
msg_e88:         db "E88", 0x0A, 0
msg_emem_fail:   db "EMEM-FAIL", 0x0A, 0
