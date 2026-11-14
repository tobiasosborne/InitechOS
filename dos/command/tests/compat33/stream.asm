; Test-only stdout copy. The 16-bit build executes on genuine DOS 3.30.
%ifdef REAL_DOS
bits 16
org 100h
%define RAX ax
%define RBX bx
%define RCX cx
%define RDX dx
%else
bits 32
org 40100h
%define RAX eax
%define RBX ebx
%define RCX ecx
%define RDX edx
%endif
again:
mov RAX,3f00h
xor RBX,RBX
mov RCX,128
mov RDX,buffer
int 21h
jc failed
test RAX,RAX
jz done
mov RCX,RAX
mov RAX,4000h
mov RBX,1
mov RDX,buffer
int 21h
jc failed
jmp again
failed:
mov RAX,4c01h
int 21h
done:
mov RAX,4c00h
int 21h
buffer times 128 db 0
