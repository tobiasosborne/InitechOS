bits TARGET_BITS
mov ax, 0x4C00 + EXIT_STATUS
int 0x21
times 32-($-$$) db 0
