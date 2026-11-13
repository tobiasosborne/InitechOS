; initech-drm4v: MS-DOS Encyclopedia AH=07h returns extended keys as two
; reads: zero then scan. Output between the reads must preserve the scan.
; Only this normal disk program knows the test; no kernel scenario hooks.
bits 32
org 0x40100
%ifndef SCAN
%error SCAN must name the independent expected keyboard scan
%endif
start:
    mov eax,0x0900
    mov edx,ready
    int 0x21
    mov eax,0x0700
    int 0x21
    mov [pair],al
    ; The output poll used to consume scan 10h/13h as Ctrl-P/Ctrl-S.
    mov eax,0x0900
    mov edx,between
    int 0x21
    mov eax,0x0700
    int 0x21
    mov [pair+1],al

    ; A real Ctrl-N followed by Enter ends any mutant-induced printer echo
    ; before the verdict. The captured pair above is already immutable.
    mov eax,0x0a00
    mov edx,cleanup
    int 0x21

    mov eax,0x3c00
    xor ecx,ecx
    mov edx,name
    int 0x21
    jc io_failed
    mov ebx,eax
    mov eax,0x4000
    mov ecx,2
    mov edx,pair
    int 0x21
    jc io_failed
    cmp eax,2
    jne io_failed
    mov eax,0x3e00
    int 0x21
    jc io_failed
    cmp byte [pair],0
    jne bad_pair
    cmp byte [pair+1],SCAN
    jne bad_pair
    mov edx,passed
    xor esi,esi
    jmp verdict
bad_pair:
    mov edx,failed
    mov esi,1
    jmp verdict
io_failed:
    mov edx,io_error
    mov esi,2
verdict:
    mov eax,0x0900
    int 0x21
    mov eax,0x0900
    mov edx,done
    int 0x21
    mov eax,0x4c00
    or eax,esi
    int 0x21
ready: db 'CONPAIR-READY',13,10,'$'
between: db 'CONPAIR-OUTPUT-BETWEEN-READS',13,10,'$'
passed: db 'CONPAIR-PASS',13,10,'$'
failed: db 'CONPAIR-FAIL',13,10,'$'
io_error: db 'CONPAIR-IO-FAIL',13,10,'$'
done: db 'CONPAIR-DONE',13,10,'$'
name: db 'PAIR.BIN',0
pair: times 2 db 0ffh
cleanup: db 8,0
    times 9 db 0
