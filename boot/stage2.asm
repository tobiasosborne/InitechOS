; stage2 -- physical two-file bootstrap, initech-kod.
; Ref: ADR-0003 DEC-01/07/08, Appendix B.1; IBM DOS 3.30 Technical
; Reference printed 1-7 (PDF24: root slots/order and loading order), 5-4
; (PDF111: volume layout), 5-7..8 (PDF114..115: FAT12 chains/EOC), 5-11
; (PDF118: HSR). MS-DOS 3.3 User's Guide printed119 (PDF204: SYS order,
; hidden files, fragmentation permitted), printed41 (PDF54: error text).
;
; MBR loads exactly 16 sectors at 8000. Bootstrap owns [8000,9000);
; IO.SYS owns [9000,A000). The embedded IO.SYS is ONLY for legacy raw
; disks whose partition table is empty. On a FAT boot disk no missing,
; misplaced or corrupt system file can fall back to the raw LBA17 run.
; The factory emits contiguous chains; this loader walks FAT12 chains,
; checks exact length/EOC, bounds, cycles, cross-links and both FAT copies.
; INITDOS.SYS is the existing padded flat kernel, bounce [10000,40000).
; IO.SYS retains the original BIOS, A20/GDT/PE/far-jump/copy sequence.
; Factory-only mutants bypass one named rule; never used by make image.
bits 16
org 0x8000
KERNEL_SECTORS equ 384
%ifndef MK_KERNEL_SECTORS
 %error "MK_KERNEL_SECTORS required"
%elif MK_KERNEL_SECTORS != KERNEL_SECTORS
 %error "KERNEL_SECTORS MISMATCH"
%endif
%if KERNEL_SECTORS*512 > 0x30000
 %error "KERNEL_SECTORS exceeds KERNEL_BOUNCE_CAP"
%endif
ROOT equ 0x6000
FAT equ 0x2000
VISITED equ 0xA000
COM1 equ 0x3f8
start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    cld
    mov [drive], dl
    sti
    ; Same queried-CHS BIOS contract as the original stage2, one sector per
    ; read so neither track nor DMA 64KiB boundaries can be crossed.
    mov ah, 8
    xor di, di
    int 0x13
    jc fail
    xor ax, ax
    mov ds, ax
    mov es, ax
    and cx, 63
    mov [spt], cx
    jz fail
    movzx ax, dh
    inc ax
    mov [heads], ax
    mov ax, ROOT/16
    mov es, ax
    xor ax, ax
    call read_sector
    mov si, ROOT+446
    mov cx, 64
    xor ax, ax
.table:
    or al, [si]
    inc si
    loop .table
    test al, al
    jz legacy
    ; The shipped BOOT volume is the first active FAT12 partition. Unsupported
    ; nonempty tables fail closed; they are never mistaken for old raw disks.
    cmp byte [ROOT+446], 0x80
    jne fail
    cmp byte [ROOT+450], 1
    jne fail
    cmp word [ROOT+456], 0
    jne fail
    cmp word [ROOT+460], 0
    jne fail
    mov ax, [ROOT+454]
    cmp ax, 17
    jb fail
    mov [base], ax
    mov dx, [ROOT+458]
    mov [volume_sectors], dx
    add dx, ax
    jc fail
    call read_sector
    cmp word [ROOT+510], 0xaa55
    jne fail
    cmp word [ROOT+11], 512
    jne fail
    cmp byte [ROOT+16], 2
    jne fail
    mov ax, [base]
    cmp [ROOT+28], ax
    jne fail
    cmp word [ROOT+30], 0
    jne fail
    mov ax, [volume_sectors]
    cmp [ROOT+19], ax
    jne fail
    movzx ax, byte [ROOT+13]
    test ax, ax
    jz fail
    cmp ax, 8
    ja fail
    mov bx, ax
    dec bx
    test bx, ax
    jnz fail
    mov [spc], ax
    mov ax, [ROOT+22]
    test ax, ax
    jz fail
    cmp ax, 16
    ja fail
    mov [spf], ax
    mov bx, [ROOT+14]
    test bx, bx
    jz fail
    add bx, [base]
    jc fail
    mov [fat_lba], bx
    add bx, ax
    jc fail
    add bx, ax
    jc fail
    mov [root_lba], bx
    mov ax, [ROOT+17]
    cmp ax, 2
    jb fail
    add ax, 15
    jc fail
    shr ax, 4
    add ax, bx
    jc fail
    mov [data_lba], ax
    sub ax, [base]
    mov bx, [volume_sectors]
    sub bx, ax
    jbe fail
    mov ax, bx
    xor dx, dx
    div word [spc]
    test ax, ax
    jz fail
    cmp ax, 4084
    ja fail
    inc ax
    mov [last_cluster], ax
    mov bx, ax
    shr bx, 1
    add bx, ax
    inc bx
    mov ax, [spf]
    shl ax, 9
    cmp bx, ax
    jae fail
    ; Cache both FAT copies below the root buffer. Compare every byte before
    ; following either chain. No disagreement can select a torn kernel.
    mov ax, [fat_lba]
    mov dx, FAT/16
    mov cx, [spf]
    shl cx, 1
.fats:
    mov es, dx
    call read_sector
    inc ax
    add dx, 32
    loop .fats
    ; The second FAT immediately follows the first, not necessarily 4000.
    mov si, FAT
    mov di, [spf]
    shl di, 9
    mov cx, di
    add di, FAT
    xor ax, ax
    mov es, ax
    repe cmpsb
    jne fail
    mov ax, ROOT/16
    mov es, ax
    mov ax, [root_lba]
    call read_sector
    xor ax, ax
    mov es, ax
    mov di, VISITED
    mov cx, 4096
    rep stosb

    mov si, ROOT
    mov di, io_name
    call lookup
    mov edx, 4096
    mov ax, 0x0900
    call load_file
    mov si, msg_io
    call puts
    mov si, msg_io_count
    call count

    mov si, ROOT+32
    mov di, dos_name
    call lookup
    cmp dword [si+28], KERNEL_SECTORS*512
    jne fail
    mov edx, KERNEL_SECTORS*512
    mov ax, 0x1000
    call load_file
    mov si, msg_dos
    call puts
    mov si, msg_dos_count
    call count
    mov dl, [drive]
    mov dh, 1
    jmp 0:0x9000
legacy:
    mov si, msg_raw
    call puts
    mov dl, [drive]
    xor dh, dh
    jmp 0:0x9000

; SI = required root slot, DI = DOS 8.3 name. ORDER mutant searches instead
; of enforcing slots 0/1, so moving both files one slot exposes the regression.
lookup:
%ifdef BOOT_MUT_ORDER
    mov si, ROOT
.search:
%endif
    push si
    push di
    mov cx, 11
.name:
    mov al, [si]
    cmp al, [di]
    jne .wrong
    inc si
    inc di
    loop .name
    pop di
    pop si
    ret
.wrong:
    pop di
    pop si
%ifdef BOOT_MUT_ORDER
    add si, 32
    cmp si, ROOT+512
    jb .search
%endif
    jmp fail

; SI dirent; AX destination segment; EDX maximum bytes. Walk the on-disk
; FAT, never a fixed sector run. The shared VISITED set rejects both cycles
; and cross-file overlap before IO.SYS is executed or CR0.PE is set.
load_file:
    mov [dest], ax
    mov al, [si+11]
%ifdef BOOT_MUT_ATTR_R
    and al, 0x1e
    cmp al, 6
%else
    and al, 0x1f
    cmp al, 7
%endif
    jne fail
    mov eax, [si+28]
    test eax, eax
    jz fail
    cmp eax, edx
    ja fail
    add eax, 511
    shr eax, 9
    mov [remaining], ax
    mov ax, [si+26]
    mov [cluster], ax
    mov word [clusters], 0
.cluster:
    mov bx, [cluster]
    cmp bx, 2
    jb fail
    cmp bx, [last_cluster]
    ja fail
    cmp byte [VISITED+bx], 0
    jne fail
    inc byte [VISITED+bx]
    inc word [clusters]
    mov ax, bx
    sub ax, 2
    mul word [spc]
    add ax, [data_lba]
    mov [lba], ax
    mov cx, [spc]
.sector:
    mov dx, [dest]
    mov es, dx
    mov ax, [lba]
    call read_sector
    add word [dest], 32
    inc word [lba]
    dec word [remaining]
    jz .next
    loop .sector
.next:
%ifdef BOOT_MUT_CHAIN
    mov ax, [cluster]
    inc ax
    cmp word [remaining], 0
    jne .decoded
    mov ax, 0xfff
%else
    mov bx, [cluster]
    mov ax, bx
    shr bx, 1
    add bx, ax
    mov dx, [FAT+bx]
    test ax, 1
    jz .even
    shr dx, 4
.even:
    mov ax, dx
    and ax, 0xfff
%endif
.decoded:
    cmp word [remaining], 0
    je .end
    cmp ax, 0xff0
    jae fail
    mov [cluster], ax
    jmp .cluster
.end:
    cmp ax, 0xff8
    jb fail
    ret

; AX absolute disk LBA (16-bit bounded volume); ES:0 destination. Preserve
; all caller registers. Same INT13 CHS interface as legacy, including the
; cylinder high bits, with no track crossing or 64KiB boundary crossing.
read_sector:
    pushad
    xor dx, dx
    div word [spt]
    mov cl, dl
    inc cl
    xor dx, dx
    div word [heads]
    cmp ax, 1023
    ja fail
    mov dh, dl
    mov ch, al
    shr ax, 2
    and al, 0xc0
    or cl, al
    mov dl, [drive]
    xor bx, bx
    mov ax, 0x0201
    int 0x13
    jc fail
    popad
    ret

count:
    call puts
    mov ax, [clusters]
    mov cx, 4
.hex:
    rol ax, 4
    push ax
    and al, 15
    add al, '0'
    cmp al, '9'
    jbe .digit
    add al, 7
.digit:
    call putc
    pop ax
    loop .hex
    mov si, newline
    jmp puts
putc:
    push ax
    push dx
    mov ah, al
.wait:
    mov dx, COM1+5
    in al, dx
    test al, 0x20
    jz .wait
    mov dx, COM1
    mov al, ah
    out dx, al
    pop dx
    pop ax
    ret
puts:
    push si
    push ax
.loop:
    lodsb
    test al, al
    jz .done
    call putc
    jmp .loop
.done:
    pop ax
    pop si
    ret
fail:
    xor ax, ax
    mov ds, ax
    mov si, error
    call puts
    ; Render the exact period text as well as serial. Failure alone selects
    ; BIOS text mode; the successful VBE/protected-mode sequence is untouched.
    mov ax, 3
    int 0x10
    xor ax, ax
    mov ds, ax
    mov si, error
.print:
    lodsb
    test al, al
    jz .halt
    mov ah, 0x0e
    mov bx, 7
    int 0x10
    jmp .print
.halt:
    cli
    hlt
    jmp .halt
io_name: db 'IO      SYS'
dos_name: db 'INITDOS SYS'
msg_io: db 'IOSYS-FOUND',10,0
msg_dos: db 'INITDOS-FOUND',10,0
msg_io_count: db 'IO-CLUSTERS=',0
msg_dos_count: db 'INITDOS-CLUSTERS=',0
msg_raw: db 'RAW-FALLBACK',10,0
newline: db 10,0
error: db 'Non-System disk or disk error',13,10
       db 'Replace and strike any key when ready',13,10,0
drive: db 0
spt: dw 0
heads: dw 0
base: dw 0
volume_sectors: dw 0
spc: dw 0
spf: dw 0
fat_lba: dw 0
root_lba: dw 0
data_lba: dw 0
last_cluster: dw 0
dest: dw 0
remaining: dw 0
cluster: dw 0
clusters: dw 0
lba: dw 0

%if ($-$$) > 4096
 %error "stage2 FAT bootstrap exceeds its 4096-byte real-mode budget"
%endif
times 4096-($-$$) db 0
; The raw fallback is the same independently built IO.SYS. The FAT path
; overwrites it with the actual file from disk, then enters that loaded file.
incbin IO_SYS_PATH
%if ($-$$) > 8192
 %error "stage2 + embedded IO.SYS exceeds MBR's 16-sector (8192-byte) budget"
%endif
times 8192-($-$$) db 0
