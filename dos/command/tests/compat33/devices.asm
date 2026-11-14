; Read-only device-name probe: open, IOCTL device flag, close; no device I/O.
%ifdef REAL_DOS
bits 16
org 100h
%define RAX ax
%define RBX bx
%define RCX cx
%define RDX dx
%define RSI si
%else
bits 32
org 40100h
%define RAX eax
%define RBX ebx
%define RCX ecx
%define RDX edx
%define RSI esi
%endif
mov RSI,names
next:
cmp byte [RSI],0
je done
mov RDX,RSI
mov RAX,0900h
int 21h
mov RDX,RSI
scan:
cmp byte [RSI],'$'
je found
inc RSI
jmp scan
found:
mov byte [RSI],0
mov RAX,3d02h
int 21h
mov byte [RSI],'$'
inc RSI
jc failed
mov RBX,RAX
mov RAX,4400h
int 21h
push RDX
mov RAX,3e00h
int 21h
pop RDX
test dl,80h
mov RDX,regular
jz print
mov RDX,device
jmp print
failed:
mov RDX,error
print:
mov RAX,0900h
int 21h
jmp next
done:
mov RAX,4c00h
int 21h
names db 'CON$', 'SUB\CON.TXT$', 'SUB\PRN$', 'SUB\NUL$', 'SUB\AUX$', 'MISSING\NUL$', 'Z:\NUL$', 'CON:$', 0
regular db ':FILE',13,10,'$'
device db ':DEVICE',13,10,'$'
error db ':ERROR',13,10,'$'
