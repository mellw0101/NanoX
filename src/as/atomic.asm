section .text

; Atomic add.
global asm_atomic_add
;		rdi: Pointer to the integer.
;		rsi: Value to add.
asm_atomic_add:
	lock add [rdi], rsi
	ret

; Atomic sub.
global asm_atomic_sub
;		rdi: Pointer to the integer.
; 	rsi: Value to sub.
asm_atomic_sub:
	lock sub [rdi], rsi
	ret

; Atomic xchg
global asm_atomic_xchg
asm_atomic_xchg:
	xchg rsi, [rdi]
	mov rax, rsi 	; Return the old value.
	ret
