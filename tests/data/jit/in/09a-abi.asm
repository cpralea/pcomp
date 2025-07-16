    mov r0, 1
    push r0
    mov r0, 2
    push r0
    call add
    call print
    jmp exit

add:
    load r0, [sp + 16]
    load r1, [sp + 8]
    add r0, r1
.return:
    add sp, 24
    push r0
    load r0, [sp - 16]
    push r0
    ret

print:
    load r0, [sp + 8]
    push r0
    mov r0, 1
    push r0
    call $sys_enter
.return:
    add sp, 16
    load r0, [sp - 16]
    push r0
    ret

exit:
    mov r0, 0
    push r0
    call $sys_enter
