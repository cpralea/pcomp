    mov r1, 1
    mov r2, 2
    add r1, r2
    add r1, 3

    push r1
    mov r1, 1
    push r1
    call $sys_enter

    mov r1, 1
    mov r2, 2
    sub r1, r2
    sub r1, 3

    push r1
    mov r1, 1
    push r1
    call $sys_enter

    mov r0, 0
    push r0
    call $sys_enter
