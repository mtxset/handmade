.code

 asm_get_tick_count_64 PROC
  mov ecx, [7FFE0004h]   ; multiplier to ecx
	mov eax, [7FFE0320h]   ; tick count to eax
  shl rcx, 20h           ; shift left by 32 bits
	mov rax, rax           ;
	shl rax, 08h           ; shift left by 8 bits
  mul rcx                ; multiply?
	mov rax, rdx           ;
	ret                    ;
 asm_get_tick_count_64 ENDP
  
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

  rdtscp                        ; Read timestamp counter into edx:eax (edx is discarded)
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