; Independent DEC-03a.8 link-bound fixture. No application code or packer.
bits 32
global _start
section .text._start
_start: ret
section .text.0ovl_file
times 16 db 0x90
section .bss
%ifdef RESERVE_OVERSIZE
resb 65536
%else
resb 16
%endif
section .note.GNU-stack noalloc noexec nowrite progbits
