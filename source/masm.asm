.code
 asm_get_thread_id PROC
  mov rax, gs:[30h]
  mov eax, [rax+48h]
  ret
 asm_get_thread_id ENDP

 asm_get_core_id PROC

    test byte ptr [7FFE0295h], 0FFh
    jz     skip_check_1           ; If zero, jump to skip_check_1

    rdpid  rcx                    ; Read processor ID into rcx
    xor    eax, eax               ; Clear eax
    mov    al, cl                 ; Move lower byte of rcx into al
    ret                           ; Return
skip_check_1:
    test byte ptr [7FFE0294h], 0FFh
    jz     skip_check_2           ; If zero, jump to skip_check_2

    rdtscp
    xor     eax, eax              ; Clear eax
    mov     al, cl                ; Move lower byte of rcx into al
    ret                           ; Return

skip_check_2:
    mov     eax, 53h              ; Set eax to 0x53
    lsl     eax, eax              ; Logical shift left eax by eax (eax = eax << eax)
    jnz     skip_ret              ; If not zero, jump to skip_ret

    shr     eax, 0Eh              ; Logical shift right eax by 14 bits
    ret                           ; Return

skip_ret:
    ret                           ; Return (if jump to skip_ret occurs)
 asm_get_core_id ENDP
END