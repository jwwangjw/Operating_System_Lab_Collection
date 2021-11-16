mulchar:
    push ebp
    mov ebp, esp
    push ecx
    push edx
    push eax
    push ebx
    ;[[ebp - 12] + ecx] = byte[[ebp - 12] + ecx] * Byte + overflow
    ;edx stored overflows
    mov ecx, 43
    mov edx, 0
.mulcharLoop:
    ;results:ax，bl:Byte
    mov eax, dword[ebp -12]
    mov al, byte[eax + ecx]
    mov ebx, dword[ebp - 16]
    and eax, 0xFF
    and ebx, 0xFF
    mul bl
    add eax, edx
    cmp ax, 10d
    jae .mulOverflow
    mov edx, 0
.mulcharloopend:
    mov ebx, dword[ebp - 12]
    mov byte[ebx + ecx], al
    dec ecx
    jz .mulcharend
    jmp .mulcharLoop
.mulOverflow:
    mov dl, 10d
    and edx, 0xFF
    div dl;
    mov dl, al
    mov al, ah
    and eax, 0xFF
    and edx, 0xFF
    jmp .mulcharloopend
.mulcharend:
    add esp, 8d
    pop edx
    pop ecx
    leave
    ret
mulLI:
    push ebp
    mov ebp, esp
    push edx
    ;first num
    push eax
    ;second num
    push ebx
    ;result
    push ecx
    ;resultPtr = 0
    mov eax, zeros
    mov ebx, dword[ebp - 16]
    call strcpy
    pop ebx
    pop eax
    mov ecx, 1d
.mulLIloop:
    ;loop，ecx from 1 to 43
    push eax
    push ebx
    mov eax, dword[ebp - 16]
    mov bl, 10d
    and ebx, 0xFF
    call mulchar
    pop ebx
    pop eax
    ;[inputBuf] = [ptr1]
    push eax
    push ebx
    mov eax, [ebp - 8]
    mov ebx, input
    call strcpy
    pop ebx
    pop eax
    ;mulByte(inputBuf, [ptr2 + ecx])
    push eax
    push ebx
    mov eax, input
    mov ebx, [ebp - 12]
    mov bl, byte[ebx + ecx];ebx = [ptr2 + ecx]
    and ebx, 0xFF
    call mulchar
    pop ebx
    pop eax
    push eax
    push ebx
    push ecx
    mov eax, [ebp - 16];eax = resultPtr
    mov ebx, input;ebx = inputBuf
    mov ecx, [ebp - 16];ecx = resultPtr
    call addLI
    pop ecx
    pop ebx
    pop eax
    cmp ecx, 43d
    jz .mulLIEnd
    inc ecx
    jmp .mulLIloop

.mulLIEnd:
    ;esp:top,ebp:end
    add esp, 12
    pop edx
    leave
    ret
mul_new:
    push ebp
    mov ebp, esp
    push edx

    push eax;ptr1 [ebp - 8]
    push ebx;ptr2 [ebp - 12]
    push ecx;resultptr [ebp -16]

    push eax
    push ebx
    push ecx
    call mulLI
    pop ecx
    pop ebx
    pop eax
    mov eax, 0d
    mov ebx, dword[ebp - 8]
    mov al, byte[ebx]
    mov ebx, dword[ebp - 12]
    mov bl, byte[ebx]
    xor al, bl
    mov ebx, dword[ebp - 16]
    mov byte[ebx], al

    add esp, 12d
    pop edx
    leave
    ret
