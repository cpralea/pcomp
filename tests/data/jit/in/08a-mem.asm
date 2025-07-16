    mov r1, 1
    mov r2, 2

    push r1
    push r2
    load r3, [sp + 8]
    load r4, [sp + 0]

    push r1
    mov r1, 1
    push r1
    call $sys_enter
    push r2
    mov r2, 1
    push r2
    call $sys_enter

    add r3, 2
    add r4, 2

    store [sp + 8], r3
    store [sp + 0], r4
    pop r2
    pop r1

    push r1
    mov r1, 1
    push r1
    call $sys_enter
    push r2
    mov r2, 1
    push r2
    call $sys_enter

    mov r0, 0
    push r0
    call $sys_enter
