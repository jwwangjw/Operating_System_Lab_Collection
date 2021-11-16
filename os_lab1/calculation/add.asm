addL:
    push ebp
    ;ebp:ptr,ebp:top
    mov ebp, esp
    push edx
    push eax
    push ebx
    push ecx
    ;stored in 4 bytes
    mov al, byte[eax]
    add al, 0
    mov al, byte[ebx]
    add al, 0
    mov eax, dword[ebp - 8]
    mov ebx, dword[ebp - 12]
    mov ecx, dword[ebp - 16]
    call addLI
    push ecx
    pop ebx
    pop eax
    add esp, 12
    pop edx
    leave
    ret
addLI:
    push ebp
    mov ebp, esp
    push edx
    push eax
    push ebx
    push ecx
    mov ecx, 43
    mov edx, 0
.addLILoop:
    mov eax, 0
    mov ebx, dword[ebp - 8]
    mov bl, byte[ebx + ecx]
    ;final byte of ebx
    and ebx, 0xFF
    add eax, ebx
    ;again stored by 4 bytes
    mov ebx, dword[ebp - 12]
    mov bl, byte[ebx + ecx]
    and ebx, 0xFF
    add eax, ebx
    ;add pre-overflow
    add eax, edx
    cmp eax, 10
    jae .addoverflow
    mov edx, 0
.addLILoopend:
    ;result address
    mov ebx, dword[ebp - 16]
    mov byte[ebx + ecx], al;
    dec ecx
    jz .addLIEnd
    jmp .addLILoop
.addoverflow:
    ;edx stores overflow information
    mov edx, 1
    sub eax, 10
    jmp .addLILoopend
.addLIEnd:
    sub esp, 12
    pop edx
    leave
    ret