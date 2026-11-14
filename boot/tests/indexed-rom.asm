; External PC option ROM: expose only VBE 0101h mode information.
; VBE Core 2.0 rev 1.1 pp6,16-20: AX=4F01h/CX=mode, AX=014Fh
; means unsupported. All other video operations chain to the real VGA BIOS.
; The IBM PC expansion-ROM header is 55 AA, size in 512-byte units,
; entry at +3, byte checksum zero. This ROM never goes on a product disk.
; Firmware authorities: coreboot/seabios src/optionroms.c is_valid_rom/callrom
; and bochs-emu/Bochs bochs/bios/rombios.c rom_scan/rom_checksum (upstream).
bits 16
org 0
db 0x55, 0xaa, 1
    jmp init
handler:
    cmp ax, 0x4f02
    je observe
%ifdef INDEXED_ONLY
    cmp ax, 0x4f01
    jne .chain
    cmp cx, 0x101
    je .chain
    mov ax, 0x014f
    iret
%endif
.chain:
    int 0x60
    iret
observe:
    ; Query the ACTUAL BIOS descriptor selected by IO, independently of its
    ; bookkeeping. 7600h is test-firmware scratch below the 7C00h stack.
    pusha
    push es
    mov cx, bx
    and cx, 0x3fff
    xor ax, ax
    mov es, ax
    mov di, 0x7600
    mov ax, 0x4f01
    int 0x60
    cmp ax, 0x004f
    jne .done
    mov dx, 0x3f8
    mov al, 'B'
    out dx, al
    mov al, 'P'
    out dx, al
    out dx, al
    mov al, '='
    out dx, al
    mov al, [es:di+0x19]
    aam 10
    mov bx, ax
    mov al, bh
    add al, '0'
    out dx, al
    mov al, bl
    add al, '0'
    out dx, al
    mov al, 10
    out dx, al
.done:
    pop es
    popa
    int 0x60
    iret
init:
    push ax
    push ds
    xor ax, ax
    mov ds, ax
    ; INT60 is an otherwise unused BIOS vector, preserved until PM entry.
    mov ax, [0x40]
    mov [0x180], ax
    mov ax, [0x42]
    mov [0x182], ax
    mov word [0x40], handler
    mov ax, cs
    mov [0x42], ax
    pop ds
    pop ax
    retf
times 511-($-$$) db 0
db 0 ; checksum filled by the independent ROM reader
