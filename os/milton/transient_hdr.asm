; transient_hdr.asm -- the header of the TRANSIENT COMMAND.COM (bead
; initech-3p1a). Layout = os/milton/transient.h transient_hdr_t, byte for byte.
; Linked FIRST by os/milton/transient.ld at TRANSIENT_BASE, against the
; running kernel's symbols (ld --just-symbols), so every dd below is the
; address in THAT kernel build. The kernel compares the import addresses with
; its own before it ever jumps here (Rule 2). ASCII only (Rule 12).
bits 32
section .transient_hdr progbits alloc noexec nowrite align=4

extern command_repl
extern _tr_load_end
extern _tr_bss_end
extern _tr_code_end
extern fileio_fat_volume_label
extern int21_directory_exists
extern int21_get_psp
extern int21_same_file
extern int21_set_break_recovery

global transient_hdr
transient_hdr:
    db 'COMMAND3'                    ; TRANSIENT_MAGIC
    dd command_repl                  ; entry
    dd _tr_load_end                  ; load_end
    dd _tr_bss_end                   ; bss_end
    dd _tr_code_end                  ; code_end (the checksummed span, DEC-19)
    dd 5                             ; n_imports (TRANSIENT_N_IMPORTS)
    dd fileio_fat_volume_label
    dd int21_directory_exists
    dd int21_get_psp
    dd int21_same_file
    dd int21_set_break_recovery

section .note.GNU-stack noalloc noexec nowrite progbits
