;
; Compute 16! using
;
;     factorial(n) {
;         if (n <= 1)
;             return 1;
;         return n * factorial(n-1);
;     }
;

main:
    mov r0, 16
    push r0
    call factorial

    mov r0, 2
    push r0
    call $sys_enter

    mov r0, 0
    push r0
    call $sys_enter

factorial:
    load r0, [sp + 8]

    cmp r0, 1
    jmpeq .return

    push r0
    sub r0, 1
    push r0
    call factorial
    call multiply
    pop r0

.return:
    add sp, 16
    push r0
    load r0, [sp - 8]
    push r0
    ret

multiply:
    load r1, [sp + 16]
    load r2, [sp + 8]

    cmp r2, r1
    jmple .do_multiply
    mov r3, r1
    mov r1, r2
    mov r2, r3

.do_multiply:
    mov r0, 0
.loop:
    cmp r2, 0
    jmpeq .return
    add r0, r1
    sub r2, 1
    jmp .loop

.return:
    add sp, 24
    push r0
    load r0, [sp - 16]
    push r0
    ret
