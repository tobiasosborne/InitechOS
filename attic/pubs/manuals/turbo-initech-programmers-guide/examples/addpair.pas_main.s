pas_main:
    push ebp
    mov ebp, esp
    mov eax, v_total
    push eax
    mov eax, 2
    push eax
    mov eax, 3
    push eax
    mov eax, [esp+0]
    push eax
    mov eax, [esp+8]
    push eax
    call pf_add
    add esp, 16
    pop edx
    mov [edx], eax
    mov eax, v_total
    mov eax, [eax]
    call serial_put_int
    mov al, 10
    call serial_putc
    leave
    ret
