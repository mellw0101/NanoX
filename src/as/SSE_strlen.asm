section .text
  global SSE_strlen

SSE_strlen:
  mov rcx, rdi    ; Save original ptr.
  ; Check alignment.
  test rcx, 15
  jz .aligned

  .unaligned:
    mov al, [rcx]
    test al, al
    jz .end
    inc rcx
    test rcx, 15
    jnz .unaligned

  .aligned:
    .loop:
      movdqa xmm0, [rcx]
      pcmpeqb xmm1, xmm0
      pmovmskb edx, xmm1
      test edx, edx
      jnz .end_aligned
      add rcx, 16
      jmp .loop

  .end_aligned:
    bsf rdx, rdx
    add rcx, rdx

  .end:
    sub rcx, rdi
    mov rax, rcx
    ret

