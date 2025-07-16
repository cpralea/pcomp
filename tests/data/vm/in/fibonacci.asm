;
; Compute 16th Fibonacci number using
;
;     fibonacci(n) {
;         if (n <= 1)
;             return n;
;         return fibonacci(n-2) + fibonacci(n-1);
;     }
;

main:
    mov r0, 16
    push r0
    call fibonacci

    mov r0, 2
    push r0
    call $sys_enter

    mov r0, 0
    push r0
    call $sys_enter

fibonacci:
    load r0, [sp + 8]

    cmp r0, 1
    jmple .return

    load r0, [sp + 8]
    sub r0, 1
    push r0
    call fibonacci

    load r0, [sp + 16]
    sub r0, 2
    push r0
    call fibonacci

    pop r1
    pop r2
    
    mov r0, 0
    add r0, r1
    add r0, r2

.return:
    add sp, 16
    push r0
    load r0, [sp - 8]
    push r0
    ret
