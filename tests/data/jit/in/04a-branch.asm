    jmp display_minus_one

display_one:
    mov r0, 1
    push r0
    mov r0, 1
    push r0
    call $sys_enter

exit:
    mov r0, 0
    push r0
    call $sys_enter

display_minus_one:
    mov r0, -1
    push r0
    mov r0, 1
    push r0
    call $sys_enter

    jmp display_one
