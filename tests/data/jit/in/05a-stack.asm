    mov r0, 1
    mov r1, 2

    push r0
    push r1

    pop r0
    pop r1

    push r0
    mov r0, 1
    push r0
    call $sys_enter
    push r1
    mov r1, 1
    push r1
    call $sys_enter

    mov r0, 0
    push r0
    call $sys_enter
