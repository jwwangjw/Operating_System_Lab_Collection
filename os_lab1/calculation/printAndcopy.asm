printLI:
    push ebp
    mov ebp, esp
    push eax
    push ebx
    push edx
    ;ecx is the num of outputs
    push ecx
    mov ebx, [ebp - 16];ebx = ptr
    mov ecx, 0
    mov eax, [ebx + ecx];eax = [ptr + 1]
    cmp eax, 1;
    jne .printStart
.printStart:
    inc ecx
    cmp ecx, 44
    je .printfinished
    mov eax, [ebx + ecx]
    cmp al, 0
    je .printStart
.printLoop:
    mov al, byte[ebx + ecx]
    add eax, 30h
    mov byte[temp], al
    push ebx
    push ecx
    mov eax, 4
    mov ebx, 1
    mov ecx, temp
    mov edx, 1
    int 80h
    pop ecx
    pop ebx
    cmp ecx, 43
    je .printend
    inc ecx
    jmp .printLoop
.printfinished:
    mov eax, 4
    mov ebx, 1
    mov ecx,"0"
    mov edx, 1
    int 80h
.printend:
    ;enter
    mov al, 10
    mov byte[temp], al
    mov eax, 4
    mov ebx, 1
    mov ecx, temp
    mov edx, 1
    int 80h
    add esp, 4
    pop edx
    pop ecx
    pop ebx 
    leave
    ret


strcpy:
    push ebp
    mov ebp, esp
    push ecx
    push edx
    mov ecx, 10
.strcpyLoop:
    mov edx, dword[eax + ecx * 4]
    mov dword[ebx + ecx * 4], edx
    ;dest[ecx * 4] = src[ecx * 4]
    dec ecx
    js .strcpyend
    jmp .strcpyLoop
.strcpyend:
    pop edx
    pop ecx
    leave
    ret

