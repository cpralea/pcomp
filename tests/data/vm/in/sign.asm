    call mov
    call add
    call sub
    call cmp

    mov r0, 0
    push r0
    call $sys_enter

mov:
    mov r0, -1
    push r0
    call print

    mov r0, 1
    push r0
    call print

    ret

add:
    mov r0, 0
    add r0, -2
    push r0
    call print

    mov r0, 0
    add r0, 2
    push r0
    call print

    ret

sub:
    mov r0, 0
    sub r0, 3
    push r0
    call print

    mov r0, 0
    sub r0, -3
    push r0
    call print

    ret

cmp:
    mov r0, -4
    cmp r0, 0
    jmpge .2nd
    push r0
    call print
.2nd:
    mov r0, 4
    cmp r0, 0
    jmple .done
    push r0
    call print

.done:
    ret

print:
    load r0, [sp + 8]
    push r0
    mov r0, 1
    push r0
    call $sys_enter

    add sp, 16
    load r0, [sp - 16]
    push r0
    ret
