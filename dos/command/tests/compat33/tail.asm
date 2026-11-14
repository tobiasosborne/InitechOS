; PSP tail dumper: raw length byte followed by tail and CRLF. Test only.
%ifdef REAL_DOS
bits 16
org 100h
mov bx,1
xor cx,cx
mov cl,[80h]
inc cx
mov dx,80h
mov ah,40h
int 21h
mov dx,eol
mov cx,2
mov ah,40h
int 21h
mov ax,4c00h
int 21h
%else
bits 32
org 40100h
mov ebx,1
movzx ecx,byte [40080h]
inc ecx
mov edx,40080h
mov eax,4000h
int 21h
mov edx,eol
mov ecx,2
mov eax,4000h
int 21h
mov eax,4c00h
int 21h
%endif
eol db 13,10
