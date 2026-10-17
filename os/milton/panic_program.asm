; Test-only FAT-loaded .COM, ADR-0003 DEC-08 / Appendix A (3Ch/40h/3Eh).
; IBM DOS 3.30 Technical Reference pp.6-122,6-139 (PDF245,262), flat ABI
; per ADR-0003 DEC-04a. Save the actual ROM font before a real #UD or #GP.
; boot_info.h: BOOT_INFO_ADDR+20 = font_addr. No screen-derived golden.
bits 32
org 0x40100
    mov eax,0x3c00
    xor ecx,ecx
    mov edx,fontname
    int 0x21
    jc fail
    mov ebx,eax
    mov eax,0x4000
    mov ecx,4096
    mov edx,[0x514]
    int 0x21
    jc fail
    cmp eax,4096
    jne fail
    mov eax,0x3e00
    int 0x21
    jc fail
    mov eax,0x4000
    mov ebx,1
    mov ecx,armed_end-armed
    mov edx,armed
    int 0x21
    mov eax,0x12345678
    mov ebx,0x23456789
    mov ecx,0x3456789a
    mov edx,0x456789ab
%ifdef PANIC_FIXTURE_GP
    mov ax,0xfff8
    mov ds,ax                  ; selector beyond the GDT -> #GP, error FFF8
%else
    ud2                        ; invalid opcode -> vector 6, dummy error 0
%endif
fail:
    mov eax,0x4c01
    int 0x21
    jmp $
fontname: db 'FONT.BIN',0
armed: db 'PANIC-PROBE-ARMED',13,10
armed_end:
