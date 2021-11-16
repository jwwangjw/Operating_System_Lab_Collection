;author:wjw
%include 'get.asm'
%include 'printAndcopy.asm'
%include'add.asm'
%include 'mul_new.asm'
SECTION .data
msg db "input x and y:",0ah
msgl equ ($ - msg)
msg2 db "invalid num",0ah
msgl2 equ ($-msg2)

SECTION .bss
str1  resb    44
str2  resb    44
input    resb    44
Result_a   resb    44
Result_m   resb    44
temp resb    1
zeros resb    44

SECTION .text
global _start


_start:
    mov ebp, esp; for correct debugging
    call prints1

    ;input is stored in ecx
    mov ecx, str1
    call get
    mov ecx, str2
    call get

    ;add
    push eax
    push ebx
    push ecx
    mov eax, str1
    mov ebx, str2
    mov ecx, Result_a
    call addL
    mov ecx, Result_a
    call printLI
    pop ecx
    pop ebx
    pop eax

    ;mul
    push eax
    push ebx
    push ecx
    mov eax, str1
    mov ebx, str2
    mov ecx, Result_m
    call mul_new
    mov ecx, Result_m
    call printLI
    pop ecx
    pop ebx
    pop eax
  
    ;finish
    mov ebx, 0
    mov eax, 1
    int 80h


prints1:
    push ebp
    mov ebp, esp
    push eax
    push ebx
    push ecx
    push edx

    mov eax, 4
    mov ebx, 1
    mov ecx, msg
    mov edx, msgl
    int 80h

    pop edx
    pop ecx
    pop ebx
    pop eax 
    leave
    ret
prints2:
    push ebp
    mov ebp, esp
    push eax
    push ebx
    push ecx
    push edx

    mov eax, 4
    mov ebx, 1
    mov ecx, msg2
    mov edx, msgl2
    int 80h

    pop edx
    pop ecx
    pop ebx
    pop eax 
    leave
    ret



