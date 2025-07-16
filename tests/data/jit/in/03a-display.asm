display_sint:
    mov r0, -1
    push r0
    mov r0, 1
    push r0
    call $sys_enter
display_uint:
    mov r0, -1
    push r0
    mov r0, 2
    push r0
    call $sys_enter

exit:
    mov r0, 0
    push r0
    call $sys_enter
