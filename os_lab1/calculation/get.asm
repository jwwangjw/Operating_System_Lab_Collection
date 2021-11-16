get:

    push ebp
    mov ebp, esp
    push eax
    push ebx
    push edx
    push ecx 
    push 43 

.loop:
    mov eax, 3
    mov ebx, 0
    mov ecx, temp
    mov edx, 1
    int 80h
    mov eax, [temp]
    cmp eax, 32
    je .change
    cmp eax, 10
    je .change
    cmp eax,45
    jne .info
    cmp eax,"0"
    jb .invalid
    cmp eax,"9"
    ja .invalid
    ;ebx is the ptr
    mov ebx, dword[ebp - 16]
    mov byte[ebx], 1
    jmp .loop
.invalid:
    call prints2
    syscall
.info:
    mov eax, [ebp - 20]
    mov ebx, input
    mov cl, byte[temp]
    mov byte[eax + ebx], cl
    mov eax, [ebp - 20]
    dec eax
    mov [ebp - 20], eax
    jmp .loop
.change:    
    mov ecx, 43
    mov ebx, [ebp - 20]
    sub ecx, ebx
.changeLoop:
    je .finished
    mov eax, 44
    sub eax, ecx
    mov ebx, input
    mov dl, byte[ebx + eax]
    sub edx, 30h
    mov eax, [ebp - 20]
    add eax, ecx
    mov ebx, [ebp - 16]
    mov byte[ebx + eax], dl

    dec ecx
    jmp .changeLoop

.finished:
    push eax
    push ebx
    mov eax, zeros
    mov ebx, input
    call strcpy
    pop ebx
    pop eax

    add esp, 8
    pop edx
    pop ebx
    pop eax 
    leave
    ret