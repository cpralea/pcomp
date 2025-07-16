.l1:
    mov r0, 1
    cmp r0, 1
    jmpeq .ok1
    jmp .l2
.ok1:
    push r0
    call print

.l2:
    mov r0, 2
    cmp r0, 1
    jmpne .ok2
    jmp .l3
.ok2:
    push r0
    call print

.l3:
    mov r0, 3
    cmp r0, 2
    jmpgt .ok3
    jmp .l4
.ok3:
    push r0
    call print

.l4:
    mov r0, 4
    cmp r0, 5
    jmplt .ok4
    jmp .l5
.ok4:
    push r0
    call print

.l5:
    mov r0, 5
    cmp r0, 5
    jmpge .ok5
    jmp .l6
.ok5:
    push r0
    call print

.l6:
    mov r0, 6
    cmp r0, 5
    jmpge .ok6
    jmp .l7
.ok6:
    push r0
    call print

.l7:
    mov r0, 7
    cmp r0, 7
    jmple .ok7
    jmp .l8
.ok7:
    push r0
    call print

.l8:
    mov r0, 8
    cmp r0, 9
    jmple .ok8
    jmp .l9
.ok8:
    push r0
    call print

.l9:
exit:
    mov r0, 0
    push r0
    call $sys_enter

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
