pf_add:
    push ebp
    mov ebp, esp
    sub esp, 4
    lea eax, [ebp-4]
    push eax
    lea eax, [ebp+8]
    mov eax, [eax]
    push eax
    lea eax, [ebp+12]
    mov eax, [eax]
    mov ecx, eax
    pop eax
    add eax, ecx
    pop edx
    mov [edx], eax
    mov eax, [ebp-4]
    leave
    ret
